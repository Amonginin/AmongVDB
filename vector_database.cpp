/**
 * @file vector_database.cpp
 * @brief 向量数据库实现文件
 * 
 * 该文件实现了向量数据库的核心功能，包括：
 * 1. 向量的插入和更新（upsert）
 * 2. 向量的查询
 * 3. 支持多种索引类型（FLAT和HNSW）
 * 4. 与标量存储的集成
 */

#include "vector_database.h"
#include "logger.h"
#include "index_factory.h"
#include "faiss_index.h"
#include "hnswlib_index.h"
#include <vector>
#include <rapidjson/document.h>

/**
 * @brief 构造函数
 * @param dbPath 数据库存储路径
 */
VectorDatabase::VectorDatabase(const std::string &dbPath)
    : scalarStorage(dbPath)
{
}

/**
 * @brief 插入或更新向量数据
 * @param id 向量ID
 * @param data 包含向量数据的JSON文档
 * @param indexType 索引类型（FLAT或HNSW）
 * 
 * 该函数执行以下操作：
 * 1. 检查向量是否已存在
 * 2. 如果存在，从索引中删除旧向量
 * 3. 将新向量插入到索引中
 * 4. 更新标量存储中的数据
 */
void VectorDatabase::upsert(uint64_t id, const rapidjson::Document &data,
                            IndexFactory::IndexType indexType)
{
    // 检查标量存储中是否存在指定id的向量
    rapidjson::Document existingData;
    try
    {
        existingData = scalarStorage.getScalar(id);
    }
    catch (const std::runtime_error &e)
    {
        // 如果向量不存在，记录日志，继续执行插入操作
        globalLogger->info("向量不存在于标量存储中，继续执行插入操作");
    }

    // 如果向量存在，则从索引中删除它
    if (existingData.IsObject())
    {
        // 从JSON数据中提取vectors字段得到向量
        std::vector<float> existingVector(existingData["vectors"].Size());
        for (rapidjson::SizeType i = 0; i < existingData["vectors"].Size(); i++)
        {
            existingVector[i] = existingData["vectors"][i].GetFloat();
        }

        // 根据索引类型选择相应的删除操作
        void *index = getGlobalIndexFactory()->getIndex(indexType);
        switch (indexType)
        {
        case IndexFactory::IndexType::FLAT:
        {
            FaissIndex *faissIndex = static_cast<FaissIndex *>(index);
            faissIndex->removeVectors({static_cast<long>(id)});
            break;
        }
        case IndexFactory::IndexType::HNSW:
        {
            HNSWLibIndex *hnswIndex = static_cast<HNSWLibIndex *>(index);
            // hnswIndex->removeVectors({static_cast<long>(id)});
            break;
        }
        default:
            break;
        }
    }

    // 从JSON数据中提取新向量的数据
    std::vector<float> newVector(data["vectors"].Size());
    for (rapidjson::SizeType i = 0; i < data["vectors"].Size(); i++)
    {
        newVector[i] = data["vectors"][i].GetFloat();
    }

    // 根据索引类型选择相应的插入操作
    void *index = getGlobalIndexFactory()->getIndex(indexType);
    switch (indexType)
    {
    case IndexFactory::IndexType::FLAT:
    {
        FaissIndex *faissIndex = static_cast<FaissIndex *>(index);
        faissIndex->insertVectors(newVector, id);
        break;
    }
    case IndexFactory::IndexType::HNSW:
    {
        HNSWLibIndex *hnswIndex = static_cast<HNSWLibIndex *>(index);
        hnswIndex->insertVectors(newVector, id);
        break;
    }
    default:
        break;
    }

    // 更新标量存储中的向量数据
    scalarStorage.insertScalar(id, data);
}

/**
 * @brief 查询指定ID的数据
 * @param id 要查询的ID
 * @return 返回包含向量数据的JSON文档
 */
rapidjson::Document VectorDatabase::query(uint64_t id)
{
    return scalarStorage.getScalar(id);
}