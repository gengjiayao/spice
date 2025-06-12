// LUHelper.h
#ifndef LUHelper_H
#define LUHelper_H

#include <string>
#include <vector>
#include "CSQ.h"

class LUHelper {
public:
    LUHelper(const std::string&, const std::string&, const std::string&);
    std::pair<std::vector<int>, std::vector<int>> bucket_by_index_fast (const std::vector<int>&, const std::vector<int>&);
    int run();

    std::vector<CSQ> csq;

private:
    int  readCSR();        // 读 CSR
    int  readSEP();        // 读 seps
    void buildCSQ();       // 划分 CSQ
    void buildLayers();    // 波前法分层
    void meanQuantile();   // 层的切分调度

private:
    const std::string csq_path_, sep_path_, tree_path_;

    int n_{0}, nnz_{0};
    std::vector<int>    row_ptr_;
    std::vector<int>    col_idx_;
    std::vector<double> val_;

    // 快速指针区间
    std::vector<interval> fast_ptr_;

    // SEP 列表
    std::vector<int> seps_;

    // 最终层次结构
    std::vector<std::vector<int>> layers_; // 存储的是CSQ的块号
};

#endif // APP_H
