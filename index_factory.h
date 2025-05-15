/**
 * 考虑到后续可能会有其他向量索引类型需要增加到系统中，
 * 因此统一新建一个 IndexFactory 类来管理系统中的索引向量类型。
 */

#pragma once

#include <map>
#include "faiss_index.h"

class IndexFactory {
public:
    enum class IndexType {
        FLAT,
        UNKNOWN = -1
    };
    enum class MetricType {
        L2,
        INNER_PRODUCT,
        COSINE,
        UNKNOWN = -1
    };
    void init(IndexType  type, int dim, MetricType metric=MetricType::L2);
    void *getIndex(IndexType type) const;

private:
    // 存储系统中已经初始化的向量索引
    // 使用void*类型来兼容不同类型的索引对象
    std::map<IndexType, void *> index_map;
};

// 全局函数，方便系统中其他模块通过该函数获取到系统唯一的 IndexFactory 对象
IndexFactory* getGlobalIndexFactory();