/**
 * @file scalar_storage.cpp
 * @brief 标量数据存储实现文件
 * @details 使用RocksDB实现标量数据的持久化存储
 */

#include "scalar_storage.h"
#include "logger.h"
#include "rocksdb/db.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

/**
 * @brief 构造函数
 * @param dbPath RocksDB数据库文件路径
 * @throws std::runtime_error 当数据库打开失败时抛出异常
 */
ScalarStorage::ScalarStorage(const std::string &dbPath)
{
    // 配置RocksDB选项
    rocksdb::Options options;
    options.create_if_missing = true;  // 如果数据库不存在则创建
    
    // 打开数据库
    rocksdb::Status status = rocksdb::DB::Open(options, dbPath, &db);
    if (!status.ok())
    {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }
}

/**
 * @brief 析构函数
 * @details 释放RocksDB数据库资源
 */
ScalarStorage::~ScalarStorage()
{
    delete db;
}

/**
 * @brief 插入标量数据
 * @param id 数据ID
 * @param data 要存储的JSON数据
 * @details 将JSON数据序列化后存储到RocksDB中
 */
void ScalarStorage::insertScalar(uint64_t id, const rapidjson::Document &data)
{
    // 将JSON数据序列化为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    std::string value = buffer.GetString();

    // 将数据写入RocksDB
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(id), value);
    if (!status.ok())
    {
        globalLogger->error("Failed to insert scalar: {}", status.ToString());
    }
}

/**
 * @brief 获取标量数据
 * @param id 数据ID
 * @return rapidjson::Document 返回解析后的JSON数据
 * @details 从RocksDB中读取数据并解析为JSON格式
 */
rapidjson::Document ScalarStorage::getScalar(uint64_t id)
{
    std::string value;
    // 从RocksDB中读取数据
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), std::to_string(id), &value);
    if (!status.ok())
    {
        globalLogger->error("Failed to get scalar: {}", status.ToString());
        return rapidjson::Document();  // 返回空文档
    }

    // 解析JSON数据
    rapidjson::Document data;
    data.Parse(value.c_str());

    // 记录调试信息
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    globalLogger->debug("Data retrieved from ScalarStorage: {}, RocksDB status: {}",
                        buffer.GetString(), status.ToString());

    return data;
}

/**
 * @brief 存储键值对
 * @param key 键
 * @param value 值
 *
 * 使用RocksDB存储给定的键值对。
 */
void ScalarStorage::put(const std::string &key, const std::string &value)
{
    // 调用RocksDB的Put方法存储数据
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), key, value);
    // 检查RocksDB操作是否成功
    if (!status.ok())
    {
        // 记录错误日志
        globalLogger->error("Failed to put key-value pair: {}", status.ToString());
    }
}

/**
 * @brief 根据键获取值
 * @param key 键
 * @return 返回对应的字符串值，如果键不存在或失败则返回空字符串
 *
 * 使用RocksDB根据键检索存储的值。
 */
std::string ScalarStorage::get(const std::string &key)
{
    std::string value; // 用于存储获取到的值
    // 调用RocksDB的Get方法获取数据
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
    // 检查RocksDB操作是否成功
    if (!status.ok())
    {
        // 记录错误日志
        globalLogger->error("Failed to get value for key {}: {}", key, status.ToString());
    }
    return value; // 返回获取到的值 (失败时返回空字符串)
}