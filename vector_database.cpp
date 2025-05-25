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

#include "constants.h"
#include "vector_database.h"
#include "logger.h"
#include "index_factory.h"
#include "faiss_index.h"
#include "hnswlib_index.h"
#include "filter_index.h"
#include "http_server.h"
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

/**
 * @brief 构造函数
 * @param dbPath 数据库存储路径
 */
VectorDatabase::VectorDatabase(const std::string &dbPath, const std::string &walLogPath)
    : scalarStorage(dbPath)
{
    persistence.init(walLogPath);
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
 * 4. 更新过滤索引
 * 5. 更新标量存储中的数据
 */
void VectorDatabase::upsert(uint64_t id, const rapidjson::Document &data,
                            IndexFactory::IndexType indexType)
{
    // 打印插入或更新请求的内容
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    globalLogger->info("Upsert data: {}", buffer.GetString());

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
        // 打印删除旧向量的日志
        globalLogger->info("try to remove old index");
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

    // 从JSON数据中提取新向量的数据插入索引
    std::vector<float> newVector(data["vectors"].Size());
    for (rapidjson::SizeType i = 0; i < data["vectors"].Size(); i++)
    {
        newVector[i] = data["vectors"][i].GetFloat();
    }

    // 打印添加新向量的日志
    globalLogger->info("try to add new index");

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

    // 打印添加新过滤器的日志
    globalLogger->info("try to add new filter");

    FilterIndex *filterIndex = static_cast<FilterIndex *>(
        getGlobalIndexFactory()->getIndex(IndexFactory::IndexType::FILTER));

    // 检查客户写入的数据中是否有 int 类型的 JSON 字段
    for (auto it = data.MemberBegin(); it != data.MemberEnd(); ++it)
    {
        std::string fieldName = it->name.GetString();
        globalLogger->debug("try to filter member {} {}",
                            it->value.IsInt(), fieldName);
        // 如果字段是int类型，则添加到过滤器中
        if (it->value.IsInt() && fieldName != "id")
        {
            int64_t fieldValue = it->value.GetInt64();
            int64_t *oldFieldValuePointer = nullptr;
            // 如果存在现有向量，则从 FilterIndex 中更新 int 类型字段
            if (existingData.IsObject())
            {
                oldFieldValuePointer = (int64_t *)malloc(sizeof(int64_t));
                *oldFieldValuePointer = existingData[fieldName.c_str()].GetInt64();
            }
            filterIndex->updateIntFieldFilter(fieldName, oldFieldValuePointer,
                                              fieldValue, id);
            if (oldFieldValuePointer)
            {
                free(oldFieldValuePointer);
            }
        }
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

/**
 * @brief 搜索数据
 * @param jsonRequest 包含搜索请求的JSON文档
 * @return 返回搜索结果
 */
std::pair<std::vector<long>, std::vector<float>> VectorDatabase::search(
    const rapidjson::Document &jsonRequest)
{
    // 从JSON请求中提取搜索参数
    std::vector<float> searchParams;
    for (const auto &s : jsonRequest[REQUEST_VECTORS].GetArray())
    {
        searchParams.push_back(s.GetFloat());
    }
    int k = jsonRequest[REQUEST_K].GetInt();

    // 从JSON请求中提取索引类型
    IndexFactory::IndexType indexType = IndexFactory::IndexType::UNKNOWN;
    if (jsonRequest.HasMember(REQUEST_INDEX_TYPE) &&
        jsonRequest[REQUEST_INDEX_TYPE].IsString())
    {
        std::string indexTypeStr = jsonRequest[REQUEST_INDEX_TYPE].GetString();
        if (indexTypeStr == INDEX_TYPE_FLAT)
        {
            indexType = IndexFactory::IndexType::FLAT;
        }
        else if (indexTypeStr == INDEX_TYPE_HNSW)
        {
            indexType = IndexFactory::IndexType::HNSW;
        }
    }

    // 从JSON请求中提取过滤索引
    roaring_bitmap_t *filterBitmap = nullptr;
    if (jsonRequest.HasMember(INDEX_TYPE_FILTER) &&
        jsonRequest[INDEX_TYPE_FILTER].IsObject())
    {
        const auto &filter = jsonRequest[INDEX_TYPE_FILTER];
        std::string fieldName = filter["fieldName"].GetString();
        std::string opStr = filter["op"].GetString();
        int64_t value = filter["value"].GetInt64();

        FilterIndex::Operation op = (opStr == "=")
                                        ? FilterIndex::Operation::EQUAL
                                        : FilterIndex::Operation::NOT_EQUAL;
        // 获取FilterIndex
        FilterIndex *filterIndex = static_cast<FilterIndex *>(
            getGlobalIndexFactory()->getIndex(IndexFactory::IndexType::FILTER));
        filterBitmap = roaring_bitmap_create();
        filterIndex->getIntFieldFilterBitmap(fieldName, op, value, filterBitmap);
    }

    // 从全局索引工厂获取索引对象
    void *index = getGlobalIndexFactory()->getIndex(indexType);

    // 根据索引类型初始化相应的索引对象并选择相应的search操作
    std::pair<std::vector<long>, std::vector<float>> results;
    switch (indexType)
    {
    case IndexFactory::IndexType::FLAT:
    {
        FaissIndex *faissIndex = static_cast<FaissIndex *>(index);
        results = faissIndex->searchVectors(searchParams, k, filterBitmap);
        break;
    }
    case IndexFactory::IndexType::HNSW:
    {
        HNSWLibIndex *hnswIndex = static_cast<HNSWLibIndex *>(index);
        results = hnswIndex->searchVectors(searchParams, k, filterBitmap);
        break;
    }
    // TODO: 添加其他索引类型的支持
    default:
        break;
    }

    // 释放过滤索引的bitmap
    if (filterBitmap)
    {
        roaring_bitmap_free(filterBitmap);
    }

    return results;
}

/**
 * @brief 重新加载数据库中的数据
 * @details 该函数执行以下操作：
 *          1. 读取 WAL 日志中的每一条记录
 *          2. 根据操作类型执行相应的操作
 *          3. 清空 jsonData 对象
 *          4. 读取下一条 WAL 日志
 */
void VectorDatabase::reloadDatabase(){
    globalLogger->info("Entering VectorDatabase::reloadDatabase()");

    std::string operationType;
    rapidjson::Document jsonData;
    
    // 第一次读取WAL日志
    persistence.readNextWALLog(&operationType, &jsonData);

    // 循环处理WAL日志，直到operationType为空（没有更多日志）
    while (!operationType.empty()){
        // 在处理前检查jsonData是否有效，防止readNextWALLog读取失败但operationType不为空的情况
        if (!jsonData.IsObject()){
            globalLogger->debug("jsonData is not an object after reading, stopping reload.");
            break; 
        }
        
        globalLogger->info("operation type: {}", operationType);

        // 打印读取的一行内容
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        jsonData.Accept(writer);
        globalLogger->info("Read Line: {}", buffer.GetString());

        // 根据操作类型执行相应的操作
        if (operationType == "upsert"){
            uint64_t id = jsonData[REQUEST_ID].GetUint64();
            IndexFactory::IndexType indexType = getIndexTypeFromRequest(jsonData);
            // 调用 VectorDatabase::upsert 接口重建数据
            upsert(id, jsonData, indexType);
        }

        // 清空 jsonData 对象，为下一次读取做准备
        rapidjson::Document().Swap(jsonData);

        // 读取下一条 WAL 日志
        operationType.clear(); // 清空operationType，确保readNextWALLog能正确设置其状态
        persistence.readNextWALLog(&operationType, &jsonData);
    }
    
    // WAL 重放完毕
    globalLogger->info("Exiting VectorDatabase::reloadDatabase()");
}

/**
 * @brief 写入 WAL 日志
 * @param operationType 操作类型
 * @param jsonData 包含向量数据的JSON文档
 */
void VectorDatabase::writeWALLog(const std::string &operationType,
                                 const rapidjson::Document &jsonData){
    // 自定义版本号
    std::string verison = "1.0";
    // 将version传递给 persistence 对象的 writeWALLog 方法
    persistence.writeWALLog(operationType, jsonData, verison);
}

/**
 * @brief 执行数据库快照
 *
 * 调用持久化模块的takeSnapshot方法，传入scalarStorage以便保存快照。
 */
void VectorDatabase::takeSnapshot(){
    // 调用持久化模块执行快照
    persistence.takeSnapshot(scalarStorage);
}

/**
 * @brief 从请求中获取索引类型(出于模块化考虑，将该函数从 http_server.h 中复制过来)
 * @param jsonRequest JSON请求文档对象
 * @return IndexFactory::IndexType 返回解析出的索引类型
 */
IndexFactory::IndexType VectorDatabase::getIndexTypeFromRequest(const rapidjson::Document &jsonRequest){
    // 如果请求中包含 indexType 字段
    if (jsonRequest.HasMember(REQUEST_INDEX_TYPE))
    {
        // 获取索引类型字符串
        std::string indexTypeStr = jsonRequest[REQUEST_INDEX_TYPE].GetString();
        // 根据字符串值返回对应的索引类型
        if (indexTypeStr == INDEX_TYPE_FLAT)
        {
            return IndexFactory::IndexType::FLAT;
        }
        else if (indexTypeStr == INDEX_TYPE_HNSW)
        {
            return IndexFactory::IndexType::HNSW;
        }
        // TODO: 支持其他索引类型
    }
    // 如果请求中不包含 indexType 字段或类型未知，返回 UNKNOWN
    return IndexFactory::IndexType::UNKNOWN;
}