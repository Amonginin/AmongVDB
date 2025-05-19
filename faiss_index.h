#pragma once

#include <vector>
#include "faiss/Index.h"

class FaissIndex
{
public:
    // 构造函数，接收一个指向FAISS索引对象的指针
    FaissIndex(faiss::Index *index);

    // 公共成员函数，用于将向量数据和对应的标签写入索引中  
    void insertVectors(const std::vector<float> &data, uint64_t label);

    // 公共成员函数，用于在索引中查询与要查询的向量最近邻的k个向量
    // 返回一个包含两个动态数组的pair，第一个动态数组是找到的向量的标签，第二个动态数组是相应的距离
    std::pair<std::vector<long>, std::vector<float>> searchVectors(const std::vector<float> &query, int k);

    // 公共成员函数，用于从索引中删除指定id的向量
    void removeVectors(const std::vector<long>& ids);

private:
    // 私有成员变量，指向FAISS索引对象的指针
    faiss::Index *index; 
};
