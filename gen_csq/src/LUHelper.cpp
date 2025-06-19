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

    test();
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


/************************************
 * 读入依赖树数据，插入 dummy root，
 * 压缩空节点，波前法生成每一层的节点
 * dummy 不存数据，也不进入 layers_
 ************************************/
void LUHelper::buildCompressedLayers() {
    ifstream in(tree_path_);
    if (!in) {
        cerr << "打开 Tree 文件失败: " << tree_path_ << "\n";
        return;
    }

    // 1. —— 读 parent 数组，找出原始根 ——
    int N;
    in >> N;
    vector<int> parent(N + 1, -1);
    int orig_root = -1;
    for (int i = 0; i < N; ++i) {
        in >> parent[i];
        if (parent[i] < 0) orig_root = i;
    }
    in.close();

    // 2. —— 构建 children 列表 ——
    vector<vector<int>> children(N + 1);
    for (int i = 0; i < N; ++i) {
        if (parent[i] > 0) {
            children[parent[i]].push_back(i);
        }
    }

    // 3. —— 扩容，插入 dummy root D = N ——
    const int D = N;
    parent[orig_root] = D;
    children[D].push_back(orig_root);

    // 准备 DFS 压缩
    csq.resize(N + 1);
    vector<char> deleted(N + 1, 0);
    csq[D].seps = {0, 1}; // 确保seps不是"空"，DFS不会删它

    // 4. —— DFS 压缩空节点 ——
    int root = D;
    function<void(int)> dfs = [&](int u) {
        if (deleted[u]) return;
        // 先递归走完子树
        for (int c : children[u]) {
            dfs(c);
        }

        // 检查是否"空"节点（seps.first==seps.second）且不是 dummy
        auto &S = csq[u].seps;
        if (u != D && S.first == S.second) {
            int p = parent[u];
            // 所有活孩子挂到 p
            for (int c : children[u]) {
                if (!deleted[c]) {
                    parent[c] = p;
                }
            }
            // 如果删的是当前根，挑个活孩子为新根
            if (u == root) {
                root = -1;
                for (int c : children[u]) {
                    if (!deleted[c]) {
                        root = c;
                        break;
                    }
                }
                if (root >= 0) parent[root] = -1;
            }
            deleted[u] = 1;
        }
    };
    dfs(root);

    // Debug: 打印删除的空节点数
    {
        int cnt = 0;
        for (int i = 0; i < N; ++i)
            if (deleted[i]) ++cnt;
        // cout << "empty csq nums = " << cnt << "\n";
    }

    // —— 4. 删除对临时 children 的占用 —— 
    children.clear();
    children.shrink_to_fit();

    // —— 5. 线性重建 alive、deg、csq[].children —— 
    vector<char> alive(N + 1, 0);
    vector<int> deg(N + 1, 0);

    for (int i = 0; i <= N; ++i) {
        csq[i].children.clear();
    }

    // 标记 alive，并累加父节点度数
    for (int u = 0; u <= N; ++u) {
        if (u == D || deleted[u]) continue;
        alive[u] = 1;
        csq[u].alive = true;
    }

    // 扫一遍 parent 数组：把每个活节点 u 挂到其活父亲 p
    for (int u = 0; u <= N; ++u) {
        if (!alive[u]) continue;
        int p = parent[u];
        if (p >= 0 && alive[p]) {
            csq[p].children.push_back(u);
            ++deg[p];
        }
    }

    // —— 6. 波前法遍历 —— 
    queue<int> q;
    for (int i = 0; i <= N; ++i) {
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
            buildUpdateNums(u);
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
            cout << "(" << layers_[i].size() << ")" << " Layer " << i << "\t";
            nn++;
            // cout << ":";
            for (int x: layers_[i]) {
                // cout << "CSQ" << x << "(" << "dim=" << csq[x].n << ")" << " ";
            }
            cout<< endl;
            // if (nn % 5 == 0) cout << endl;
        }
        cout << endl;
    #endif
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

void LUHelper::buildUpdateNums(int u) {
    int bound = csq[u].seps.second;
    const auto &assoc = csq[u].all_rows;

    auto &Inter = csq[u].inter_pairs;
    auto &Rem = csq[u].rem_pairs;
    Inter.clear();
    Rem.clear();

    unordered_map<int, int> idx_inter, idx_rem; // 保证快速查找orig
    idx_inter.reserve(csq[u].children.size() * 2);
    idx_rem.reserve(csq[u].children.size() * 2);

    // 1. 来自子节点的 rem_pairs
    for (int v : csq[u].children) {
        if (!csq[v].alive) continue;
        for (auto &pr : csq[v].rem_pairs) {
            int orig = pr.first;
            for (int val : pr.second) {
                bool isInter = binary_search(assoc.begin(), assoc.end(), val);
                auto &mp_idx = isInter ? idx_inter : idx_rem;
                auto &V = isInter ? Inter : Rem;

                auto it = mp_idx.find(orig);
                if (it == mp_idx.end()) {
                    int new_pos = V.size();
                    mp_idx[orig] = new_pos; // 建立快速查找映射
                    V.emplace_back(orig, vector<int>{});
                    it = mp_idx.find(orig);
                }
                V[it->second].second.push_back(val);
            }
        }
    }

    // 2. 本节点 assoc >= bound 也算 Rem (origin = u)
    auto it0 = lower_bound(assoc.begin(), assoc.end(), bound);
    for (; it0 != assoc.end(); ++it0) {
        int val = *it0, orig = u;
        auto it = idx_rem.find(orig);
        if (it == idx_rem.end()) {
            int new_pos = Rem.size();
            idx_rem[orig] = new_pos; // 建立快速查找映射
            Rem.emplace_back(orig, vector<int>{});
            it = idx_rem.find(orig);
        }
        Rem[it->second].second.push_back(val);
    }
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

void LUHelper::test() {
    // cout << "=====================parent=====================" << endl;
    
    // for (int i = 0; i < csq.size(); ++i) {
    //     if (csq[i].children.size() != 0) cout << "csq " << i << ": ";
    //     for (auto c : csq[i].children) {
    //         cout << c << " ";
    //     }
    //     if (csq[i].children.size() != 0) cout << endl;
    // }

    cout << "=====================UpdateNums=====================" << endl;
    for (int i = 0; i < csq.size(); ++i) {
        if (csq[i].children.size() != 0) cout << "csq " << i << ": ";
        for (auto c : csq[i].inter_pairs) {
            cout << c.first << " -> [ ";
            for (auto cc : c.second)
                cout << cc << " ";
            cout << "] \t";
        }
        if (csq[i].children.size() != 0) cout << endl;
    }

    // cout << "=====================UpRem=====================" << endl;
    // for (int i = 0; i < csq.size(); ++i) {
    //     if (!csq[i].alive) continue;
    //     cout << "csq " << i << ": ";
    //     for (auto c : csq[i].rem_pairs) {
    //         cout << c.first << " -> [ ";
    //         for (auto cc : c.second)
    //             cout << cc << " ";
    //         cout << "] \t";
    //     }
    //     cout << endl;
    // }
}
