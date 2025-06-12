// tools.h

#ifndef TOOLS_H
#define TOOLS_H

#include "CSQ.h"
#include <vector>
#include <map>
#include <stdexcept>

/***********************************************
 * vector_stats 函数：接收所有 CSQ 阶数数组，
 * 统计 CSQ 阶数情况，包括中位数、最小值、最大值等信息
 **********************************************/
template<typename T>
struct Stats {
    T        min;
    T        max;
    double   median;
    std::vector<T> mode;
    size_t count;
    double total_size;
    size_t bound_nums;
};

template<typename T>
Stats<T> vector_stats(const std::vector<T>& v, const int& bound);

double cal_gemm_cnt(const int&);
double cal_one_csq_time(const std::vector<std::vector<int>>&, int, double&, double&, double&);
double cal_csq_time(const std::vector<std::vector<int>>&, const std::vector<CSQ>&);


template<typename T>
Stats<T> vector_stats(const std::vector<T>& v, const int& bound) {
    if (v.empty()) {
        throw std::invalid_argument("输入向量不能为空。");
    }
    std::vector<T> v_copy(v);

    size_t n = v_copy.size();
    sort(v_copy.begin(), v_copy.end());
    T min_val = v_copy.front();
    T max_val = v_copy.back();

    double median_val;
    if (n % 2 == 1) {
        median_val = static_cast<double>(v_copy[n / 2]);
    } else {
        median_val = (static_cast<double>(v_copy[n/2 - 1]) + static_cast<double>(v_copy[n/2])) / 2.0;
    }
    
    std::map<T, size_t> freq;
    for (const auto& x : v_copy) {
        ++freq[x];
    }
    size_t max_count = 0;
    for (auto const& kv : freq) {
        if (kv.second > max_count) {
            max_count = kv.second;
        }
    }

    std::vector<T> modes;
    for (auto const& kv : freq) {
        if (kv.second == max_count) {
            modes.push_back(kv.first);
        }
    }

    size_t bound_nums = abs(v_copy.begin() - upper_bound(v_copy.begin(), v_copy.end(), bound));

    double total_size = 0;
    for (auto const & x : v_copy) {
        total_size += x * x * 8;
    }

    total_size /= (1024 * 1024);

    return Stats<T> { min_val, max_val, median_val, modes, n, total_size, bound_nums };
}

#endif