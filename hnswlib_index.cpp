#include "hnswlib_index.h"
#include "logger.h"
#include <vector>
#include <fstream>
#include <iostream>

/**
 * @brief 构造函数
 * @param dim 向量维度
 * @param maxElements 索引能容纳的最大向量数量
 * @param metric 距离度量类型
 * @param M 索引节点的最大近邻数，默认为16
 * @param efConstruction 构建最大近邻时的最大候选邻居数，默认为200
 *
 * 初始化HNSW索引，创建向量空间和索引结构。
 * 目前仅支持L2距离度量和内积距离度量
 */
HNSWLibIndex::HNSWLibIndex(int dim, size_t maxElements, IndexFactory::MetricType metric,
                           int M, int efConstruction) : maxElements(maxElements)
{
    hnswlib::SpaceInterface<float> *space;

    // 根据度量类型创建对应的向量空间
    if (metric == IndexFactory::MetricType::L2)
    {
        space = new hnswlib::L2Space(dim);
    }
    else if (metric == IndexFactory::MetricType::INNER_PRODUCT)
    {
        space = new hnswlib::InnerProductSpace(dim);
    }
    else
    {
        throw std::runtime_error("Unsupported metric type");
    }
    // 创建HNSW索引结构
    index = new hnswlib::HierarchicalNSW<float>(space, maxElements, M, efConstruction);
}

/**
 * @brief 向索引中插入向量数据
 * @param data 待插入的向量数据
 * @param label 向量的标签
 */
void HNSWLibIndex::insertVectors(const std::vector<float> &data, uint64_t label)
{
    index->addPoint(data.data(), static_cast<hnswlib::labeltype>(label));
}

/**
 * @brief 在索引中查询与待查询向量最近邻的k个向量
 * @param query 待查询向量
 * @param k 返回的最近邻数量
 * @param efSearch 查询k近邻时的最大候选邻居数，默认为50
 * @return 返回一个pair，包含最近邻的标签和对应的距离
 */
std::pair<std::vector<long>, std::vector<float>> HNSWLibIndex::searchVectors(
    const std::vector<float> &query, int k, 
    const roaring_bitmap_t *bitmap, int efSearch)
{
    // 设置搜索参数
    index->setEf(efSearch);

    // 创建ID过滤器
    RoaringBitmapIDFilter* filter = nullptr;
    if (bitmap != nullptr)
    {
        filter = new RoaringBitmapIDFilter(bitmap);
    }
    // 执行k近邻搜索
    auto result = index->searchKnn(query.data(), k, filter);

    // 准备返回结果
    std::vector<long> indices;
    std::vector<float> distances;

    // 从优先队列中提取结果
    while (!result.empty())
    {
        auto item = result.top();
        indices.push_back(item.second);
        distances.push_back(item.first);
        result.pop();
    }

    // 释放过滤器
    if (filter != nullptr)
    {
        delete filter;
    }    

    return {indices, distances};
}

/**
 * @brief 保存索引到文件
 * @param filePath 保存索引文件的路径
 *
 * 将当前的HNSWLib索引保存到指定的文件路径。
 */
void HNSWLibIndex::saveIndex(const std::string &filePath)
{
    // 调用底层HNSWlib库的saveIndex方法保存索引
    index->saveIndex(filePath);
}

/**
 * @brief 从文件加载索引
 * @param filePath 索引文件的路径
 *
 * 从指定的文件路径加载HNSWLib索引。加载前会检查文件是否存在，
 * 如果文件不存在，会打印警告信息并跳过加载。
 */
void HNSWLibIndex::loadIndex(const std::string &filePath)
{
    // 创建文件流并检查文件是否存在
    std::ifstream file(filePath);
    if (file.good())
    {
        file.close(); // 关闭文件流
        // 从文件加载索引，需要提供文件路径、空间接口和最大元素数
        index->loadIndex(filePath, space, maxElements);
    }else{
        // 文件未找到，打印警告
        globalLogger->warn("HNSW index file not found: {}. Skipping load HNSW index.",
                           filePath);
    }
}
