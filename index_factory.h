/**
 * @brief 索引工厂类，用于管理和创建不同类型的向量索引
 * 
 * 该类负责创建和管理不同类型的向量索引，支持多种索引类型和距离度量方式。
 * 使用工厂模式来统一管理不同类型的索引实现。
 */

#pragma once

#include <map>
#include "faiss_index.h"

class IndexFactory {
public:
    /**
     * @brief 索引类型枚举
     */
    enum class IndexType {
        FLAT,       ///< 扁平索引
        HNSW,       ///< HNSW索引
        FILTER,     ///< 过滤索引
        UNKNOWN = -1 ///< 未知索引类型
    };

    /**
     * @brief 距离度量类型枚举
     */
    enum class MetricType {
        L2,             ///< 欧氏距离
        INNER_PRODUCT,  ///< 内积距离
        COSINE,         ///< 余弦相似度
        UNKNOWN = -1    ///< 未知度量类型
    };

    /**
     * @brief 初始化指定类型的索引
     * @param type 索引类型
     * @param dim 向量维度
     * @param numData 索引能容纳的最大向量数量
     * @param metric 距离度量类型，默认为L2距离
     */
    void init(IndexType type, int dim, int numData = 0, MetricType metric=MetricType::L2);

    /**
     * @brief 获取指定类型的索引实例
     * @param type 索引类型
     * @return 返回对应类型的索引实例指针
     */
    void *getIndex(IndexType type) const;

private:
    ///< 存储系统中已经初始化的向量索引
    ///< 使用void*类型来兼容不同类型的索引对象
    std::map<IndexType, void *> index_map;
};

/**
 * @brief 获取全局唯一的索引工厂实例
 * @return 返回全局索引工厂对象的指针
 * 
 * 该函数用于获取系统中唯一的 IndexFactory 实例，
 * 确保整个系统使用同一个索引工厂来管理所有索引。
 */
IndexFactory* getGlobalIndexFactory();