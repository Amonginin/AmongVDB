#pragma once

#include "hnswlib/hnswlib.h"
#include "index_factory.h"
#include "roaring/roaring.h"
#include <vector>

/**
 * @brief HNSW索引类，用于高效的高维向量搜索
 * 
 * 该类封装了HNSW（Hierarchical Navigable Small World）算法，
 * 用于在高维空间中快速搜索最近邻向量。
 */
class HNSWLibIndex
{
public:
    /**
     * @brief 构造函数
     * @param dim 向量维度
     * @param maxElements 索引能容纳的最大向量数量
     * @param metric 距离度量类型
     * @param M 索引节点的最大近邻数，默认为16
     * @param efConstruction 构建最大近邻时的最大候选邻居数，默认为200
     */
    HNSWLibIndex(int dim, size_t maxElements, IndexFactory::MetricType metric,
                 int M = 16, int efConstruction = 200);

    /**
     * @brief 向索引中插入向量数据
     * @param data 待插入的向量数据
     * @param label 向量的标签
     */
    void insertVectors(const std::vector<float> &data, uint64_t label);

    /**
     * @brief 在索引中查询与待查询向量最近邻的k个向量
     * @param query 待查询向量
     * @param k 返回的最近邻数量
     * @param efSearch 查询k近邻时的最大候选邻居数，默认为50
     * @return 返回一个pair，包含最近邻的标签和对应的距离
     */
    std::pair<std::vector<long>, std::vector<float>> searchVectors(
        const std::vector<float> &query, int k, 
        const roaring_bitmap_t *bitmap = nullptr, int efSearch = 50);

    /**
     * @brief 保存索引到文件
     * @param filePath 文件路径
     */
    void saveIndex(const std::string &filePath);

    /**
     * @brief 从文件加载索引
     * @param filePath 文件路径
     */
    void loadIndex(const std::string &filePath);

    /**
     * @brief 基于 Roaring Bitmap 的 ID 过滤器
     * 该类继承自 hnswlib::BaseFilterFunctor，用于通过 Roaring Bitmap 判断某个ID是否在集合中。
     */
    class RoaringBitmapIDFilter : public hnswlib::BaseFilterFunctor
    {
    public:
        /**
         * @brief 构造函数
         * @param bitmap 指向 Roaring Bitmap 的指针
         */
        RoaringBitmapIDFilter(const roaring_bitmap_t *bitmap) : bitmap(bitmap) {}
        /**
         * @brief 重载()运算符
         * @param label 标签
         * @return 如果ID在集合中返回true，否则返回false
         */
        bool operator()(hnswlib::labeltype label) {
            return roaring_bitmap_contains(bitmap, static_cast<uint32_t>(label));
        }
    private:
        /**
         * @brief 指向 Roaring Bitmap 的指针
         */
        const roaring_bitmap_t *bitmap;
    };

private:                                  
    ///< 向量空间接口，用于计算向量数据之间的距离的相似度
    hnswlib::SpaceInterface<float> *space;     
    ///< HNSW索引，用于存储向量数据和执行查询操作
    hnswlib::HierarchicalNSW<float> *index;   

    ///< 索引能容纳的最大向量数量
    size_t maxElements;

};
