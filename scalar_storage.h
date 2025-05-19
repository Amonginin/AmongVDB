/**
 * @file scalar_storage.h
 * @brief 标量数据存储头文件
 * @details 定义用于存储和检索标量数据的类接口
 */

#pragma once

#include "rocksdb/db.h"
#include <string>
#include <vector>
#include "rapidjson/document.h"

/**
 * @class ScalarStorage
 * @brief 标量数据存储类
 * @details 使用RocksDB作为底层存储引擎，提供标量数据的持久化存储和检索功能
 *          支持JSON格式数据的存储和读取
 */
class ScalarStorage
{
public:
    /**
     * @brief 构造函数
     * @param dbPath RocksDB数据库文件路径
     * @throws std::runtime_error 当数据库打开失败时抛出异常
     */
    ScalarStorage(const std::string &dbPath);

    /**
     * @brief 析构函数
     * @details 释放RocksDB数据库资源
     */
    ~ScalarStorage();

    /**
     * @brief 插入标量数据
     * @param id 数据ID，用于唯一标识存储的数据
     * @param data 要存储的JSON数据
     * @details 将JSON数据序列化后存储到RocksDB中
     */
    void insertScalar(uint64_t id, const rapidjson::Document &data);

    /**
     * @brief 获取标量数据
     * @param id 数据ID
     * @return rapidjson::Document 返回解析后的JSON数据
     * @details 从RocksDB中读取数据并解析为JSON格式
     *          如果数据不存在或读取失败，返回空文档
     */
    rapidjson::Document getScalar(uint64_t id);
    
private:
    rocksdb::DB *db;  ///< RocksDB数据库实例指针
};