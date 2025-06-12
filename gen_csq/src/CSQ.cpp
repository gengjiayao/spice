// CSQ.cpp
#include <set>
#include <iostream>
#include "CSQ.h"

using namespace std;

int CSQ::max_csq_dim = 0;

/*************************************
 * merge_rows函数：合并多行col_idx并集
 *************************************/
void CSQ::merge_rows (
    const vector<int>& row_ptr, 
    const vector<int>& col_idx
) {
    set<int> col_set;
    for (int row_i = this->seps.first; row_i < this->seps.second; row_i++) {
        // 找到不小于row开始的vector-index
        int start = row_ptr[row_i], to = row_ptr[row_i + 1]; // 第row_i行有序数组下标范围
        auto it = lower_bound(col_idx.begin() + start, col_idx.begin() + to, row_i);
        for (; it != col_idx.begin() + to; ++it) {
            col_set.emplace(*it);
        }
    }
    this->all_rows = vector<int>(col_set.begin(), col_set.end());
}

/*************************************
 * establish_rows_hash函数：建立行哈希
 *************************************/
void CSQ::establish_rows_hash() {
    for (int i = 0; i < this->n; ++i) {
        rows_index_hash[this->all_rows[i]] = i; 
    }
}

/***********************
 * 提取 val 到 CSQ 中
 ***********************/
// todo: 可以优化访存逻辑
void CSQ::extract_data(
    const vector<double> &val,
    const vector<int>& row_ptr,
    const vector<int>& col_idx,
    vector<interval>& fast_ptr
) {
    // 1. 遍历所有的并集行
    for (auto row : this->all_rows) {

        // 2. 检查行在sep内
        if (row >= this->seps.first && row < this->seps.second) {
            // 3. 提取从fast_idx[row]之后的全部元素
            for (int i = fast_ptr[row].start; i < fast_ptr[row].end; ++i) {
                int matrix_j = this->rows_index_hash[col_idx[i]]; // 密集矩阵列
                int matrix_i = this->rows_index_hash[row]; // 密集矩阵行
                this->matrix[matrix_i * this->n + matrix_j] = val[i];
            }
            // 4. 更新fast_ptr
            fast_ptr[row].start = fast_ptr[row].end;
        }

        // 3. 检查行不在sep内
        else {
            // 4. 提取指定元素
            while (true) {
                int i = fast_ptr[row].start;
                int idx = col_idx[i];
                if (idx < this->seps.second) {
                    int matrix_j = this->rows_index_hash[idx];
                    int matrix_i = this->rows_index_hash[row];
                    this->matrix[matrix_i * this->n + matrix_j] = val[i];
                    fast_ptr[row].start++;
                } else {
                    break;
                }
            }
        }
    }
}

/***********************
 * 打印 CSQ 数据用于调试
 ***********************/
void CSQ::print() {
    cout << "-------------------------------------------------" << endl;
    cout << "CSQ" << this->index << " (dim=" << this->n << ")" << endl;
    for (int i = 0; i < n; ++i) {
        if (i != 0) cout << "," << endl;
        cout << "[";
        for (int j = 0; j < n; j++) {
            if (j == 0) printf("%.6lf", this->matrix[i *n + j]);
            else printf(", %.6lf", this->matrix[i *n + j]);
        }
        cout << "]";
    }
    cout << endl;
}