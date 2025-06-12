#include <bits/stdc++.h>
using namespace std;

int main() {
    ifstream tree_fin("/home/qingyangwang/Linux_Gutai2/jiayao/spice/strumpack/build/tree_data.txt");
    int N, tmp;
    tree_fin >> N;
    vector<int> parent(N - 1);
    for (int i = 0; i < N - 1; i++) {
        tree_fin >> parent[i];
    }

    // 1. children[i] 存放节点 i 的所有孩子
    vector<vector<int>> children(N+1);
    for(int i = 0; i < N; i++) 
        children[parent[i]].push_back(i);

    // 2. 计算节点的度 deg[i]
    vector<int> deg(N+1, 0);
    for(int i = 0; i < N; i++) 
        deg[i] = children[i].size();

    // 3. 叶子节点入队
    queue<int> q;
    for (int i = 0; i < N; i++) 
        if (deg[i] == 0) q.push(i);

    vector<vector<int>> layers;
    while(!q.empty()) {
        int sz = q.size();
        layers.emplace_back();
        while(sz--) {
            int u = q.front(); q.pop();
            layers.back().push_back(u);
            int p = parent[u];
            if (p != 0 && --deg[p] == 0) q.push(p);
        }
    }

    // 4. 输出各层节点
    for(size_t i = 0; i < layers.size(); i++) {
        cout << "(" << layers[i].size() << ")" << " Layer " << i;
        // cout << ": ";
        // for(int x : layers[i]) {
        //     cout << x << " ";
        // }
        cout << endl;
    }

    return 0;
}
