#include <bits/stdc++.h>
using namespace std;

/* ---------------- 结点定义 ---------------- */
struct Node
{
    int pa = -1;         // parent index  (-1 == root)
    int lch = -1;        // left  child
    int rch = -1;        // right child
    int sepL = 0, sepR = 0;   // interval [sepL , sepR]
    vector<int> children;     // multi-way children (结果使用)

    bool isEmpty() const { return sepL == sepR; }
};

/* ---------- 工具：增删孩子(多叉向量里) ---------- */
static inline void addChild(Node &p, int c)
{
    if (c != -1) p.children.push_back(c);
}
static inline void removeChild(Node &p, int c)
{
    if (p.lch == c) p.lch = -1;
    if (p.rch == c) p.rch = -1;
    auto &vec = p.children;
    vec.erase(remove(vec.begin(), vec.end(), c), vec.end());
}

/* ---------------- 压缩主体 ----------------
 *  root  : 当前根下标，调用后可能被修改
 *  nodes : 所有结点表
 *  del   : 标记已删除
 *  返回值: (可能更新后的) 根下标
 */
int dfsCompress(int u, int &root, vector<Node> &nodes, vector<char> &del)
{
    if (u == -1 || del[u]) return root;

    // 先递归两边（后序遍历）
    dfsCompress(nodes[u].lch, root, nodes, del);
    dfsCompress(nodes[u].rch, root, nodes, del);

    /* --------- 若 u 是空结点，执行“删除 + 接线” ---------- */
    if (nodes[u].isEmpty())
    {
        int p = nodes[u].pa;      // 父亲

        // 1. 把有效孩子转接给父亲
        for (int c : {nodes[u].lch, nodes[u].rch})
        {
            if (c != -1 && !del[c])
            {
                if (p != -1) addChild(nodes[p], c);
                nodes[c].pa = p;       // 更新父指针
            }
        }

        // 2. 从父亲的二叉 / 多叉引用里删掉 u
        if (p != -1) removeChild(nodes[p], u);

        // 3. 如果删掉的是根，重新选一个新根
        if (u == root)
        {
            int newRoot = -1;
            for (int c : {nodes[u].lch, nodes[u].rch})
                if (c != -1 && !del[c]) { newRoot = c; break; }

            root = newRoot;
            if (root != -1) nodes[root].pa = -1;
        }
        del[u] = 1;                 // 标记删除
    }
    return root;
}

/* ----------- 对整棵树的封装调用 ------------ */
int compressTree(int root, vector<Node> &nodes)
{
    vector<char> deleted(nodes.size(), 0);
    if (root == -1) return root;
    dfsCompress(root, root, nodes, deleted);

    /* 可选：把仍然存在的二叉孩子也放进 children 数组，便于统一遍历 */
    for (size_t i = 0; i < nodes.size(); ++i)
        if (!deleted[i])
        {
            for (int c : {nodes[i].lch, nodes[i].rch})
                if (c != -1 && !deleted[c])
                    addChild(nodes[i], c);
        }

    return root;   // 返回新的根
}

/* ------------------ DEMO ------------------ */
int main()
{
    /* 按题目里的例子构造原始二叉树 */
    /*
        idx  pa  lch rch   sep
        0    4   -1  -1    0/1
        1    3   -1  -1    1/2
        2    3   -1  -1    2/4
        3    4    1   2    4/5
        4    8    0   3    5/5   <-- 空结点
        5    7   -1  -1    5/6
        6    7   -1  -1    6/9
        7    8    5   6    9/10
        8   -1    4   7    10/11
    */
    const int N = 9;
    vector<Node> nodes(N);

    auto set = [&](int i,int pa,int lch,int rch,int l,int r)
    { nodes[i].pa=pa; nodes[i].lch=lch; nodes[i].rch=rch; nodes[i].sepL=l; nodes[i].sepR=r; };

    set(0,4,-1,-1,0,1);
    set(1,3,-1,-1,1,2);
    set(2,3,-1,-1,2,4);
    set(3,4,1,2,4,5);
    set(4,8,0,3,5,5);     // 空结点
    set(5,7,-1,-1,5,6);
    set(6,7,-1,-1,6,9);
    set(7,8,5,6,9,10);
    set(8,-1,4,7,10,11);

    int root = 8;         // 原来的根

    /* 运行压缩 */
    root = compressTree(root, nodes);

    /* ------------ 打印结果检查 ------------- */
    cout << "new root = " << root << "\n\n";
    cout << "idx  pa   children   sepL/sepR\n";
    cout << "---------------------------------\n";
    for (int i = 0; i < N; ++i)
    {
        if (nodes[i].sepL == nodes[i].sepR) continue;      // 已删除的空结点
        cout << setw(2) << i << "  "
             << setw(2) << nodes[i].pa << "   ";

        cout << "[";
        for (size_t k = 0; k < nodes[i].children.size(); ++k)
        {
            if (k) cout << ",";
            cout << nodes[i].children[k];
        }
        cout << "]     ";
        cout << nodes[i].sepL << "/" << nodes[i].sepR << "\n";
    }
    return 0;
}
