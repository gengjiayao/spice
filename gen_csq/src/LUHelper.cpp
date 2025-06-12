// LUHelper.cpp
#include <fstream>
#include <iostream>
#include <queue>
#include <algorithm>
#include <cassert>
#include "LUHelper.h"
#include "globals.h"

using namespace std;

LUHelper::LUHelper(const string &csq_data, const string &sep_data, const string &tree_data)
    : csq_path_(csq_data), sep_path_(sep_data), tree_path_(tree_data) {}

int LUHelper::run() {
    if (readCSR() != 0) return 1;
    if (readSEP() != 0) return 1;

    buildCSQ();
    // buildLayers();           // 没有去掉空节点的波前法
    buildCompressedLayers();    // 去掉空节点的波前法
    meanQuantile();

    // test();
/*
    double total_time = cal_csq_time(layers_, csq);
    cout << "total time = " << total_time << "ms" << endl;
*/
    return 0;
}


/********************
*   读入CSR数据信息
********************/
int LUHelper::readCSR() {
    ifstream in(csq_path_);
    if (!in) {
        cerr << "打开 CSR 文件失败: " << csq_path_ << "\n";
        return 1;
    }

    in >> n_ >> nnz_;
    if (!in) {
        cerr << "读取 n/nnz 失败\n";
        return 1;
    }
    
    row_ptr_.resize(n_ + 1);
    col_idx_.resize(nnz_);
    val_.resize(nnz_);

    for (int i = 0; i < n_+1; ++i) {
        in >> row_ptr_[i];
        if (!in) { cerr<<"读取 row_ptr["<<i<<"] 失败\n"; return 1; }
    }

    for (int i = 0; i < nnz_; ++i) {
        in >> col_idx_[i];
        if (!in) { cerr<<"读取 col_idx["<<i<<"] 失败\n"; return 1; }
    }
  
    for (int i = 0; i < nnz_; ++i) {
        in >> val_[i];
        if (!in) { cerr<<"读取 val["<<i<<"] 失败\n"; return 1; }
    }

    in.close();

    // CSR->CSQ HashTable -> (start, end)
    fast_ptr_.resize(n_);
    for (int i = 0; i < n_; ++i) {
        fast_ptr_[i].start = row_ptr_[i];
        fast_ptr_[i].end   = row_ptr_[i + 1];
    }
    return 0;
}

/********************
*   读入SEP数据信息
********************/
int LUHelper::readSEP() {
    ifstream in(sep_path_);
    if (!in) {
        cerr << "打开 SEP 文件失败: " << sep_path_ << "\n";
        return 1;
    }
    int t;
    while (in >> t) {
        seps_.push_back(t);
    }
    in.close();
    return 0;
}

/*******************
*   通过SEP划分CSQ
********************/
void LUHelper::buildCSQ() {
    const size_t csq_cnt = seps_.size() / 2;
    csq.resize(csq_cnt);

#ifdef PRINT_ROWS_STATISTICS
    vector<int> all_dims;
#endif

    for (size_t i = 0; i < csq_cnt; ++i) {
        csq[i].seps = make_pair(seps_[i << 1], seps_[(i << 1) + 1]);

        csq[i].merge_rows(row_ptr_, col_idx_);

        csq[i].index = int(i);
        csq[i].n = csq[i].all_rows.size();
        CSQ::max_csq_dim = max(csq[i].n, CSQ::max_csq_dim); // Get the csq's max_dim

        #ifdef PRINT_ROWS_STATISTICS
            all_dims.push_back(csq[i].n);
        #endif

        csq[i].establish_rows_hash();
        // 矩阵空间堆分配
        csq[i].matrix = make_unique<double[]>(csq[i].n * csq[i].n);

        csq[i].extract_data(val_, row_ptr_, col_idx_, fast_ptr_);
        
        #ifdef PRINT_CSQ
            csq[i].print();
        #endif
    }

    #ifdef PRINT_ROWS_STATISTICS
    {   
        try {
            auto s = vector_stats(all_dims, TILE_BOUND);
            cout << "CSQ的阶数数据如下:" << endl; 
            cout << "最小值min: "    << s.min    << endl
                    << "最大值max: "    << s.max    << endl
                    << "中位数median: " << s.median << endl
                    << "众数mode: ";
            for (size_t i = 0; i < s.mode.size(); ++i) {
                cout << s.mode[i]
                        << (i + 1 < s.mode.size() ? ", " : "\n");
            }
            cout << "总个数count: "  << s.count  << endl;
            cout << "空间大小size: " << s.total_size << " MB" << endl;
            cout << "小于" << TILE_BOUND << "阶数的有" << s.bound_nums << "个" << endl; 

        } catch (exception& e) {
            cerr << "错误: " << e.what() << endl;
        }
    }
    #endif
}


/*************************************
* 读入依赖树数据，波前法生成每一层的节点
*************************************/
void LUHelper::buildLayers() {
    ifstream in(tree_path_);
    if (!in) {
        cerr << "打开 Tree 文件失败: " << tree_path_ << "\n";
        return;
    }
    int N;
    in >> N;
    vector<int> parent(N - 1);

    for (int i = 0; i < N-1; ++i) in >> parent[i];
    // 1. children[i] 存放节点 i 的所有孩子
    vector<vector<int>> children(N + 1);
    for (int i = 0; i < N - 1; ++i)
        children[parent[i]].push_back(i);

    // 2. 计算节点的度 deg[i]
    vector<int> deg(N + 1, 0);
    for (int i = 0; i < N; ++i)
        deg[i] = children[i].size();

    // 3. 叶子节点入队
    queue<int> q;
    for (int i = 0; i < N; ++i)
        if (deg[i] == 0) q.push(i);

    while (!q.empty()) {
        int sz = q.size();
        layers_.emplace_back();
        while (sz--) {
            int u = q.front(); q.pop();
            layers_.back().push_back(u);
            int p = parent[u];
            if (p != 0 && --deg[p] == 0) q.push(p);
        }
    }
    in.close();
    
    #ifdef PRINT_LAYER
    int nn = 0;
        for (size_t i = 0; i < layers_.size(); ++i) {
            cout << "(" << layers_[i].size() << ")" << " Layer " << i << "\t";
            nn++;
            // cout << ":";
            for (int x: layers_[i]) {
                // cout << "CSQ" << x << "(" << "dim=" << csq[x].n << ")" << " ";
            }
            // cout<< endl;
            if (nn % 5 == 0) cout << endl;
        }
        cout << endl;
    #endif
}


/***********************************************
* 读入依赖树数据，压缩空节点，波前法生成每一层的节点
************************************************/
void LUHelper::buildCompressedLayers() {
    ifstream in(tree_path_);
    if (!in) {
        cerr << "打开 Tree 文件失败: " << tree_path_ << "\n";
        return;
    }

    // 1. 读parent数组，找出根
    int N;
    in >> N;
    vector<int> parent(N);
    int root = -1;
    for (int i = 0; i < N; ++i) {
        in >> parent[i];
        if (parent[i] < 0) root = i;
    }
    in.close();
    
    // 2. children[i] 存放节点 i 的所有孩子
    vector<vector<int>> children(N);
    for (int i = 0; i < N; ++i) {
        int p = parent[i];
        if (p >= 0) children[p].push_back(i);
    }

    // 3. 后续DFS压缩空结点
    vector<char> deleted(N, 0);
    function<void(int)> dfs = [&](int u) {
        if (u < 0 || deleted[u]) return;
        // 3.1 拷贝一份原始孩子列表
        auto orig = children[u];
        for (int c : orig) {
            dfs(c);
        }
        // 3.2 如果是“空节点”(seps.first==seps.second)，就 splice out
        auto &S = csq[u].seps;
        if (S.first == S.second) {
            int p = parent[u];
            // 3.3 把活孩子接到 p 上
            for (int c : orig) {
                if (c >= 0 && !deleted[c]) {
                    parent[c] = p;
                    if (p >= 0)
                        children[p].push_back(c);
                }
            }
            // 3.4 从 p 的 children 把 u 删除
            if (p >= 0) {
                auto &vc = children[p];
                vc.erase(remove(vc.begin(), vc.end(), u), vc.end());
            }
            // 3.5 如果 u 是根，挑一个活孩子当新根
            if (u == root) {
                int new_root = -1;
                for (int c : orig)
                    if (c >= 0 && !deleted[c]) { new_root = c; break; }
                root = new_root;
                if (root >= 0) parent[root] = -1;
            }
            // 3.6 标记删除并清空自己的 children
            deleted[u] = 1;
            children[u].clear();
        }
    };
    if (root >= 0) dfs(root);
    // cout << "root = " << root << endl;

    int cnt = 0;
    for (int i = 0; i < deleted.size(); i++) {
        if (deleted[i] == 1) {
            cnt++;
        }
    }
    cout << "empty csq nums = " << cnt << endl;

    // 4. 计算节点的度
    vector<int> deg(N, 0);
    vector<char> alive(N, 0);
    for (int i = 0; i < N; ++i) {
        if (deleted[i]) continue;
        alive[i] = 1;
        for (int c : children[i]) {
            if (!deleted[c]) deg[i]++;
        }
    }

    // 5. 入队，开始波前法遍历
    queue<int> q;
    for (int i = 0; i < N; ++i) {
        if (alive[i] && deg[i] == 0) {
            q.push(i);
        }
    }

    layers_.clear();
    while (!q.empty()) {
        int sz = q.size();
        layers_.emplace_back();
        while (sz--) {
            int u = q.front(); q.pop();
            layers_.back().push_back(u);
            csq[u].level = (int)layers_.size() - 1;
            int p = parent[u];
            if (p >= 0 && alive[p] && --deg[p] == 0) {
                q.push(p);
            }
        }
    }

    #ifdef PRINT_LAYER
    int nn = 0;
        for (size_t i = 0; i < layers_.size(); ++i) {
            // cout << "(" << layers_[i].size() << ")" << " Layer " << i << "\t";
            nn++;
            // cout << ":";
            for (int x: layers_[i]) {
                // cout << "CSQ" << x << "(" << "dim=" << csq[x].n << ")" << " ";
            }
            // cout<< endl;
            // if (nn % 5 == 0) cout << endl;
        }
        // cout << endl;
    #endif
}

pair<vector<int>, vector<int>> LUHelper::bucket_by_index_fast (
    const vector<int>& csq_index,
    const vector<int>& divide
) {
    int nC = csq_index.size();
    int B  = int(divide.size()) - 1;
    vector<int> cnt(B, 0);

    // 1. 计数：看每个 A[C[k]] 属于哪个桶
    for (int idx : csq_index) {
        int x = csq[idx].n;
        int j = int(upper_bound(divide.begin(), divide.end(), x) - divide.begin()) - 1;
        if (0 <= j && j < B) {
            cnt[j]++;
        }
    }

    // 2. 前缀和 -> 计算每个桶在 out 中的起始位置
    vector<int> bucket_start(B + 1, 0);
    for (int i = 0; i < B; i++) {
        bucket_start[i + 1] = bucket_start[i] + cnt[i];
    }
    // bucket_start[B] == 所有落在有效区间的元素总数 ≤ nC

    // 3. scatter 写入
    vector<int> out(nC);
    vector<int> wp = bucket_start;  // 写指针，防止覆盖 bucket_start
    for (int idx : csq_index) {
        int x = csq[idx].n;
        int j = int(upper_bound(divide.begin(), divide.end(), x) - divide.begin()) - 1;
        if (0 <= j && j < B) {
            out[wp[j]++] = idx;
        }
    }
    return {out, bucket_start};
}

void LUHelper::meanQuantile() {
    /********************************
    * gen data from 8 to max_csq_dim
    * if max_csq_dim = 90
    * DEVIDE = [8, 16, 32, 64, 90]
    *********************************/
    int max_csq_dim = CSQ::max_csq_dim;
    // cout << "CSQ's max dim = " << CSQ::max_csq_dim << endl;

    vector<int> DIVIDE;
    DIVIDE.emplace_back(0); // 初始为0
    int n = 8;
    while (n < max_csq_dim) {
        DIVIDE.emplace_back(n + 1); // +1 左闭右开
        n = n << 1;
    }
    DIVIDE.emplace_back(max_csq_dim + 1); // +1 左闭右开

    /********************************
    * gen layer schedule vector
    *********************************/
    vector<pair<vector<int>, vector<int>>> scheduler(layers_.size());
    int layer = 0;
    for (const auto &layer_arr : layers_) {
        scheduler[layer++] = bucket_by_index_fast(layer_arr, DIVIDE);
    }
    int cnts = 0; // tensor数
    for (auto [out, start] : scheduler) {
        for (int i = 0; i < start.size() - 1; ++i) {
            for (int j = start[i]; j < start[i + 1]; ++j) {
            }
            if (start[i + 1] - start[i] > 0) {
                cnts++;
            }
        }
    }
}

void LUHelper::test() {
    
}
