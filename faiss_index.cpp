#include "faiss_index.h"
#include "logger.h"
#include "constants.h"
#include "faiss/IndexIDMap.h"
#include <iostream>
#include <vector>

FaissIndex::FaissIndex(faiss::Index *index) : index(index){}

/**
 * @brief 向FAISS索引中插入单个向量及其关联标签
 *
 * @param data 待写入扁平索引的向量数据
 * @param label data对应的向量ID
 *
 * @note 当前版本每次只写入一条数据
 */
void FaissIndex::insertVectors(const std::vector<float> &data, uint64_t label)
{
    // 将标签转换为long类型，以符合FAISS索引的要求
    long id = static_cast<long>(label);

    // 1表示写入单个向量，data.data()是数据的指针,&id提供向量的ID
    index->add_with_ids(1, data.data(), &id);

    // std::cout << "Inserted vector with ID: " << id << std::endl;
}

/**
 * @brief 向量相似性搜索函数
 *
 * @param query 查询向量数据，格式为一维浮点数数组，可包含多个查询向量
 * @param k 每个查询需要返回的最近邻居数量
 * @return std::pair<std::vector<long>, std::vector<float>> 返回一个对，第一个元素是匹配向量的ID，第二个元素是对应的距离值
 *
 * @note 返回的向量ID和距离值按照查询结果顺序排列
 */
std::pair<std::vector<long>, std::vector<float>> FaissIndex::searchVectors(const std::vector<float> &query, int k)
{
    // 从索引的维度属性中获取待查询向量的维度
    int dim = index->d;

    // 用待查询向量数组的长度 除以 每个待查询向量的维度 来计算待查询向量的数量
    int num_queries = query.size() / dim;

    // 创建一个动态数组来存储所有查询结果向量的ID，大小为待查询向量的数量乘以k
    std::vector<long> indices(num_queries * k); 

    // 创建一个存储所有查询结果距离的动态数组，大小也为查询向量的数量乘以k
    std::vector<float> distances(num_queries * k);

    // 执行查询操作，传入查询向量的数量、数据、k值、距离和向量ID结果的指针
    index->search(num_queries, query.data(), k, distances.data(), indices.data());

    // 打印查询结果
    globalLogger->debug("Retrieved values:");
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] != -1) {
            globalLogger->debug("ID: {}, Distance: {}", indices[i], distances[i]);
        } else {
            globalLogger->debug("No specific value found");
        }
    }

    return {indices, distances};
}

/**
 * @brief 从FAISS索引中删除指定ID的向量
 * 
 * @param ids 要删除的向量ID列表
 * 
 * @note 该函数要求底层的FAISS索引必须是IndexIDMap类型
 * @note 如果底层索引不是IndexIDMap类型，将抛出运行时异常
 * 
 * @throws std::runtime_error 当底层索引不是IndexIDMap类型时抛出异常
 */
void FaissIndex::removeVectors(const std::vector<long> &ids)
{
    // 将底层索引转换为IndexIDMap类型
    faiss::IndexIDMap *idMap = static_cast<faiss::IndexIDMap *>(index);
    if (idMap)
    {
        // 创建一个IDSelectorBatch对象，用于指定要删除的ID
        faiss::IDSelectorBatch idSelectorBatch(ids.size(), ids.data());
        // 使用IDSelectorBatch删除指定的向量
        idMap->remove_ids(idSelectorBatch);
    }
    else
    {
        // 记录错误日志
        globalLogger->error("Faiss removeVectors failed: Underlying index is not an IndexIDMap");
        // 抛出运行时异常
        throw std::runtime_error("Underlying Faiss index is not an IndexIDMap");
    }
}
