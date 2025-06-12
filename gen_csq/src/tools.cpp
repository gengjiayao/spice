// tools.cpp
#include <iostream>
#include "tools.h"
#include "globals.h"

using namespace std;

/***********************************************
 * cal_gemm_cnt函数：统计1个csq中gemm算子的计算时间
 **********************************************/
double cal_gemm_cnt(const int& tile_number) {
    double ans = 0;
    for (int i = tile_number - 1; i >= 1; --i) {
        ans += i * i * 1.0;
    }
    return ans;
}

/***********************************************
 * cal_one_csq_time函数：统计1个csq计算时间
 **********************************************/
double cal_one_csq_time(const vector<vector<int>>& layers, int n, double& lu, double& tsolve, double& gemm) {
    // n表示矩阵阶数
    double csq_time = 0;
    int tile_number = n / TILE_DIM;
    if (n != 0 && n % TILE_DIM != 0) tile_number++;

    double lu_time = (4 * TILE_DIM - 2) * tile_number;
    double tsolve_time = 2 * TILE_DIM * tile_number * (tile_number - 1);
    double gemm_time = (5 * TILE_DIM - 2) * cal_gemm_cnt(tile_number);
    lu += lu_time;
    tsolve += tsolve_time;
    gemm += gemm_time;

    csq_time = lu_time + tsolve_time + gemm_time;
    double speed = FREQUENCY * 1e3;
    csq_time = csq_time / speed;

    return csq_time / PE_NUM;
}

/***********************************************
 * cal_csq_time函数：统计全体csq理论计算时间
 **********************************************/
double cal_csq_time(const vector<vector<int>>& layers, const vector<CSQ>& csq) {
    double total_time = 0;
    double lu = 0, tsolve = 0, gemm = 0;
    for (auto layer : layers) {
        for (auto csq_idx : layer) {
            total_time += cal_one_csq_time(layers, csq[csq_idx].n, lu, tsolve, gemm);
        }
    }
    cout << "lu time = " << lu / (FREQUENCY * 1e3 * PE_NUM) << "ms" << endl;
    cout << "tsolve time = " << tsolve / (FREQUENCY * 1e3 * PE_NUM) << "ms" << endl;
    cout << "gemm time = " << gemm / (FREQUENCY * 1e3 * PE_NUM) << "ms" << endl;
    return total_time;
}