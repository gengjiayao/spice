#include <iostream>
#include <vector>
#include <stack>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <algorithm>

struct Node {
    int id;
    Node* parent = nullptr;
    std::vector<Node*> children;
};

// 非递归收集所有节点并设置 parent
void collectNodes(Node* root, std::vector<Node*>& all) {
    if (!root) return;
    std::stack<Node*> st;
    st.push(root);
    while (!st.empty()) {
        Node* cur = st.top(); st.pop();
        all.push_back(cur);
        for (Node* c : cur->children) {
            c->parent = cur;
            st.push(c);
        }
    }
}

// 波次并行后序
void layeredPostOrder(Node* root) {
    if (!root) return;
    // 1. 收集节点
    std::vector<Node*> all;
    collectNodes(root, all);

    // 2. 原子化剩余子节点计数
    std::unordered_map<Node*, std::atomic<int>> remain;
    remain.reserve(all.size());
    for (auto* n : all) {
        remain[n].store(int(n->children.size()), std::memory_order_relaxed);
    }

    // 3. 初始化第一波：所有叶子节点
    std::vector<Node*> current;
    for (auto* n : all) {
        if (remain[n].load(std::memory_order_relaxed) == 0) {
            current.push_back(n);
        }
    }

    std::mutex                 ids_mtx;
    std::vector<Node*>         next_wave;
    std::vector<int>           wave_ids;

    // 4. 波次循环
    while (!current.empty()) {
        wave_ids.clear();

        // 4.1 并发执行本波所有节点
        for (Node* n : current) {
            wave_ids.push_back(n->id);
        }

        // 4.2 打印本波结果（可选排序保证顺序）
        std::sort(wave_ids.begin(), wave_ids.end());
        std::cout << "Compute node";
        for (int id : wave_ids) {
            std::cout << " " << id;
        }
        std::cout << "\n";

        // 4.3 收集下一波就绪节点
        next_wave.clear();
        for (Node* n : current) {
            if (Node* p = n->parent) {
                // 子节点计数减 1，若从 1 变 0 则就绪
                if (remain[p].fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    next_wave.push_back(p);
                }
            }
        }

        // 4.4 进入下一波
        current.swap(next_wave);
    }
}

int main() {
    // 构建示例树
    //        1
    //       / \
    //      2   3
    //     / \
    //    4   5
    Node n1{1}, n2{2}, n3{3}, n4{4}, n5{5};
    n1.children = {&n2, &n3};
    n2.children = {&n4, &n5};

    layeredPostOrder(&n1);
    return 0;
}
