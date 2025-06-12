#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

void extract_upper_triangular(
    int n, 
    const vector<int>& row_ptr,
    const vector<int>& col_index,
    const vector<double>& values,
    vector<int>& new_row_ptr,
    vector<int>& new_col_index,
    vector<double>& new_values,
    int& new_nnz) 
{
    new_row_ptr.resize(n+1, 0);
    new_col_index.clear();
    new_values.clear();

    // 逐行处理原始矩阵
    for (int row = 0; row < n; ++row) {
        int row_start = row_ptr[row];
        int row_end = row_ptr[row+1];
        int element_count = 0;

        // 遍历该行的所有元素
        for (int idx = row_start; idx < row_end; ++idx) {
            int col = col_index[idx];
            
            // 保留列号 >= 行号的元素（上三角+对角线）
            if (col >= row) {
                new_col_index.push_back(col);
                new_values.push_back(values[idx]);
                element_count++;
            }
        }

        // 更新row_ptr
        new_row_ptr[row+1] = new_row_ptr[row] + element_count;
    }

    new_nnz = new_col_index.size();
}

int main() {
    /******************************
     *         读入csr文件
    *******************************/
    const string filein = "test_csr.txt";  // 输入文件
    ifstream fin(filein);
    if (!fin.is_open()) {
        cerr << "Error: Could not open file " << filein << endl;
        return 1;
    }

    int n, nnz;
    fin >> n >> nnz;
    vector<int> row_ptr(n+1), col_index(nnz);
    vector<double> values(nnz);
    
    for(int i=0; i<=n; i++) fin >> row_ptr[i];
    for(int i=0; i<nnz; i++) fin >> col_index[i];
    for(int i=0; i<nnz; i++) fin >> values[i];
    fin.close();

    /******************************
     *    输出csr的上三角文件
    *******************************/
    const string fileout_upper = "test_csr_upper.txt";  // 输入文件
    ofstream fout_upper(fileout_upper);
    if (!fout_upper.is_open()) {
        cerr << "Error: Could not create output file " << fileout_upper << endl;
        return 1;
    }

    vector<int> new_row_ptr, new_col_index;
    vector<double> new_values;
    int new_nnz;

    extract_upper_triangular(n, row_ptr, col_index, values, new_row_ptr, new_col_index, new_values, new_nnz);

    fout_upper << n << endl << new_nnz << endl; 
    for (int x : new_row_ptr) fout_upper << x << " ";
    fout_upper << endl;;
    for (int x : new_col_index) fout_upper << x << " ";
    fout_upper << endl;
    for (double x : new_values) fout_upper << x << " ";
    fout_upper << endl;


    /******************************
     *       处理CSR成为CSC
    *******************************/
    // 1. 计算每列的非零元数量
    vector<int> col_counts(n, 0);
    for(int col : col_index) {
        col_counts[col]++;
    }

    // 2. 构建col_ptr（列指针数组）
    vector<int> col_ptr(n+1, 0);
    for(int i=0; i<n; i++) {
        col_ptr[i+1] = col_ptr[i] + col_counts[i];
    }

    // 3. 创建并填充row_index和csc_values
    vector<int> row_index(nnz);
    vector<double> csc_values(nnz);
    vector<int> current_pos(n, 0); // 跟踪每列的当前位置

    // 遍历CSR的每一行
    for(int row=0; row<n; row++) {
        // 遍历该行的所有非零元素
        for(int i=row_ptr[row]; i<row_ptr[row+1]; i++) {
            int col = col_index[i];
            double val = values[i];
            // 计算在CSC中的存储位置
            int pos = col_ptr[col] + current_pos[col];
            row_index[pos] = row;
            csc_values[pos] = val;
            current_pos[col]++;
        }
    }

    /******************************
     *         输出csc文件
    *******************************/
    const string fileout = "test_csc.txt";
    ofstream fout(fileout);
    if (!fout.is_open()) {
        cerr << "Error: Could not create output file " << fileout << endl;
        return 1;
    }

    // 输出CSC格式
    fout << n << endl;
    fout << nnz << endl;
    for(int x : col_ptr) fout << x << " ";
    fout << endl;
    for(int x : row_index) fout << x << " ";
    fout << endl;
    for(double x : csc_values) fout << x << " ";
    fout << endl;
    return 0;
}
