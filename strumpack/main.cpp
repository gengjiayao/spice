#include "StrumpackSparseSolver.hpp"
#include <fstream>
#include <iostream>

using namespace strumpack;

// #define FROM_FILE

int main () {

    std::ofstream ofs("/home/qingyangwang/Linux_Gutai2/jiayao/spice/strumpack/build/csq_tree_data.txt", std::ios::out | std::ios::trunc);

    if (!ofs) {
        std::cerr << "打开文件失败" << std::endl;
        return 1;
    }

    // 自定义
    // const int n = 6;
    // const int nnz = 24;
    // int row_ptr[n + 1] = {0,3,7,10,14,20,24};
    // int col_idx[nnz] = {0,1,4,0,1,4,5,2,3,4,2,3,4,5,0,1,2,3,4,5,1,3,4,5};
    // double val[nnz] = {0.0836934,1.12727,0.187802,-1.42143,1.19542,0.387129,-0.970534,-0.882302,1.19896,1.04938,-0.648679,-0.44891,-0.477946,0.967105,-0.39246,-0.0650677,1.61644,0.529946,-2.26298,1.47651,0.013506,-0.750811,0.718305,-0.834928};

#ifndef FROM_FILE
    // 2023
    const int n = 11;
    const int nnz = 41;
    int row_ptr[n + 1] = {0,4,6,8,10,12,16,22,26,31,35,41};
    int col_idx[nnz] = {0,5,6,8,1,7,2,6,3,10,4,9,0,5,6,8,0,2,5,6,8,10,1,7,9,10,0,5,6,8,10,4,7,9,10,3,6,7,8,9,10};
    double val[nnz] = {-0.312433,-0.325333,-0.637391,0.370436,-1.01483,0.77067,-0.850102,1.0208,0.409216,0.825835,-0.14324,0.549566,0.440441,1.55771,-0.115204,-0.614328,0.666769,-1.30408,-1.01304,-0.98591,-0.0264229,0.931419,-1.61803,1.96652,0.755415,0.240542,-0.282654,0.100373,0.830403,-0.196284,-0.218874,0.539863,0.937918,-0.925955,0.245517,0.60077,0.130312,0.823878,0.909776,-0.261248,-0.643998};
    
    // const int n = 11;
    // const int nnz = 40;
    // int row_ptr[n + 1] = {0,3,5,7,9,11,15,21,25,30,34,40};
    // int col_idx[nnz] = {0,5,8,1,7,2,6,3,10,4,9,0,5,6,8,0,2,5,6,8,10,1,7,9,10,0,5,6,8,10,4,7,9,10,3,6,7,8,9,10};
    // double val[nnz] = {-1.97569,-0.263068,0.529229,0.826941,0.595868,-0.342254,1.78861,-2.20734,0.23353,-1.52555,0.875888,0.332623,-0.356207,-0.0695941,1.75976,0.292812,2.16302,0.62565,1.14111,-1.12023,-0.851143,-0.205868,-0.188916,0.487644,0.998044,1.01086,0.909252,0.218778,1.02541,0.292745,0.154616,-0.155424,-0.872216,-0.00961973,-0.798456,-0.937766,1.25044,-0.912698,1.63824,0.463819};
#endif    

#ifdef FROM_FILE
    // from file
    std::ifstream fin("/home/qingyangwang/Linux_Gutai2/jiayao/spice/sort_csr/bbd300_csr.txt");
    if (!fin.is_open()) {
        std::cerr << "打开文件失败" << std::endl;
        return 1;
    }

    int n_rows, n_cols, nnz;
    fin >> n_rows >> n_cols >> nnz;
    if (!fin) {
        std::cerr << "读取矩阵尺寸和nnz失败" << std::endl;
        return 1;
    }

    int n = n_rows;

    int * row_ptr = new int[n + 1];
    int * col_idx = new int[nnz];
    double * val = new double[nnz];

    for (int i = 0; i < n + 1; i++) {
        fin >> row_ptr[i];
        if (!fin) {
            std::cerr << "读取 row_ptr[" << i << "] 失败" << std::endl;
            return 1;
        }
    }

    for (int i = 0; i < nnz; i++) {
        fin >> col_idx[i];
        if (!fin) {
            std::cerr << "读取 col_idx[" << i << "] 失败" << std::endl;
            return 1;
        }
    }

    for (int i = 0; i < nnz; i++) {
        fin >> val[i];
        if (!fin) {
            std::cerr << "读取 val[" << i << "] 失败" << std::endl;
            return 1;
        }
    }

    fin.close();
#endif

    CSRMatrix<double, int> A(n, row_ptr, col_idx, val, false);
    StrumpackSparseSolver<double, int> solver(true, true); // 第二个参数表示是否为根MPI进程，verbose只打印根进程

    solver.options().set_matching(MatchingJob::NONE); // 是否放缩元素值，改变非零元数量
    solver.options().set_reordering_method(ReorderingStrategy::METIS); // 列变换排序，不改变非零元数量
    solver.options().set_verbose(true); // 是否允许打印信息

    solver.set_matrix(A);
    solver.factor();

#ifdef FROM_FILE
    delete[] row_ptr;
    delete[] col_idx;
    delete[] val;
#endif

    ofs.close();
    return 0;
}
