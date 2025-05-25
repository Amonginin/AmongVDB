/**
 * @file vector_database.h
 * @brief 向量数据库头文件
 *
 * 该文件定义了向量数据库的接口，提供了向量数据的存储和检索功能。
 * 支持多种索引类型，包括FLAT和HNSW，并集成了标量存储功能。
 */

#pragma once

#include "scalar_storage.h"
#include "index_factory.h"
#include <string>
#include <vector>
#include "rapidjson/document.h"
#include "persistence.h"

/**
 * @class VectorDatabase
 * @brief 向量数据库类
 *
 * 该类实现了向量数据库的核心功能，包括：
 * 1. 向量的插入和更新
 * 2. 向量的查询
 * 3. 与标量存储的集成
 */
class VectorDatabase
{
public:
    /**
     * @brief 构造函数
     * @param dbPath 数据库存储路径
     * @param walLogPath WAL日志存储路径
     */
    VectorDatabase(const std::string &dbPath, const std::string &walLogPath);

    /**
     * @brief 插入或更新向量数据
     * @param id 向量ID
     * @param data 包含向量数据的JSON文档
     * @param indexType 索引类型（FLAT或HNSW）
     *
     * 该函数用于插入新的向量数据或更新已存在的向量数据。
     * 如果向量已存在，会先删除旧数据再插入新数据。
     */
    void upsert(uint64_t id, const rapidjson::Document &data,
                IndexFactory::IndexType indexType);

    /**
     * @brief 查询数据
     * @param id 要查询的ID
     * @return 返回包含向量数据的JSON文档
     *
     * 该函数用于根据ID查询向量数据，返回JSON格式的向量信息。
     */
    rapidjson::Document query(uint64_t id);

    /**
     * @brief 搜索数据
     * @param jsonRequest 包含搜索请求的JSON文档
     * @return 返回搜索结果
     */
    std::pair<std::vector<long>, std::vector<float>> search(
        const rapidjson::Document &jsonRequest);



    /**
     * @brief 重新加载数据库中的数据
     */
    void reloadDatabase();

    /**
     * @brief 写入WAL日志
     * @param operationType 操作类型
     * @param jsonData 包含向量数据的JSON文档
     */
    void writeWALLog(const std::string &operationType,
                     const rapidjson::Document &jsonData);

    /**
     * @brief 执行数据库快照
     *
     * 调用持久化模块执行当前数据库状态的快照操作。
     */
    void takeSnapshot();

    /**
     * @brief 从请求中获取索引类型(出于模块化考虑，将该函数从 http_server.h 中复制过来)
     * @param jsonRequest JSON请求文档对象
     * @return IndexFactory::IndexType 返回解析出的索引类型
     */
    IndexFactory::IndexType getIndexTypeFromRequest(const rapidjson::Document &jsonRequest);

private:
    ScalarStorage scalarStorage; ///< 标量存储对象，用于存储向量相关的元数据
    Persistence persistence; ///< 持久化对象，用于持久化向量数据
};