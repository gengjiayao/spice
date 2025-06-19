// CSQ.h
#ifndef CSQ_H
#define CSQ_H

#include <memory>
#include <vector>
#include <unordered_map>

struct interval {
    int start;
    int end;  
};

/***************
 * CSQ 类的定义
 ***************/
class CSQ {
public:
    void merge_rows(const std::vector<int>&, const std::vector<int>&);
    void extract_data(const std::vector<double>&, const std::vector<int>&, const std::vector<int>&, std::vector<interval>&);
    void establish_rows_hash();
    void print();

public:
    static int max_csq_dim;

public:
    int index; // 编号
    int n; // CSQ矩阵维度
    int level; // 波前法所在层级
    bool alive {false}; // 标记是否为空

    std::vector<int> all_rows; // 所在变换后CSR矩阵中包含的行，有序
    std::unordered_map<int ,int> rows_index_hash; // 原行与现行的映射
    std::pair<int, int> seps; // sep(start, end)
    std::unique_ptr<double[]> matrix; // 实际矩阵数据

    std::vector<int> children; // 对应的孩子节点
    std::vector<std::pair<int, std::vector<int>>> inter_pairs; // 需更新部分
    std::vector<std::pair<int, std::vector<int>>> rem_pairs; // 向上传递的部分
};

#endif