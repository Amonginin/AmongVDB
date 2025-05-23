/**
 * @file persistence.h
 * @brief 持久化管理头文件
 * @details 定义了持久化管理类，负责向量数据库的WAL（Write-Ahead Logging）日志管理
 *          支持事务日志的写入和读取，确保数据的持久性和一致性
 */

#pragma once

#include <string>
#include <fstream>
#include <cstdint> // 包含 <cstdint> 以使用 uint64_t 类型
#include "rapidjson/document.h"

/**
 * @class Persistence
 * @brief 持久化管理类
 * @details 该类负责管理向量数据库的WAL日志系统，提供以下功能：
 *          1. WAL日志文件的初始化和管理
 *          2. 操作日志的写入（支持upsert、delete、query等操作）
 *          3. 操作日志的读取（用于数据库重启后的数据恢复）
 *          4. 日志ID的生成和管理
 * 
 * WAL日志格式: logID|version|operationType|jsonData
 * 其中：
 * - logID: 日志唯一标识符（递增）
 * - version: 数据版本号
 * - operationType: 操作类型（如upsert、delete、query）
 * - jsonData: 操作相关的JSON数据
 */
class Persistence
{
public:
    /**
     * @brief 构造函数
     * @details 初始化持久化管理器，设置初始日志ID为1
     */
    Persistence();

    /**
     * @brief 析构函数
     * @details 确保WAL日志文件正确关闭，释放文件资源
     */
    ~Persistence();

    /**
     * @brief 初始化WAL日志文件
     * @param localPath WAL日志文件的本地存储路径
     * @details 以读写追加模式打开WAL日志文件，如果文件不存在则创建
     *          打开失败时会抛出运行时异常
     * @throws std::runtime_error 当文件打开失败时抛出异常
     */
    void init(const std::string &localPath);

    /**
     * @brief 递增并获取下一个日志ID
     * @return uint64_t 返回递增后的新日志ID
     * @details 该函数用于生成唯一的日志标识符，每次调用都会使内部计数器递增
     *          返回的ID用于标识WAL日志条目的顺序
     */
    uint64_t increaseID();

    /**
     * @brief 获取当前日志ID
     * @return uint64_t 返回当前的日志ID（不递增）
     * @details 该函数用于查询当前的日志ID值，不会修改内部状态
     */
    uint64_t getID() const;

    /**
     * @brief 写入WAL日志条目
     * @param operationType 操作类型（如"upsert"、"delete"、"query"）
     * @param jsonData 操作相关的JSON数据文档
     * @param version 数据版本号字符串
     * @details 将一个完整的操作记录写入WAL日志文件
     *          日志格式：logID|version|operationType|jsonDataString
     *          写入后会强制刷新到磁盘以确保持久性
     * @throws std::runtime_error 当写入失败时抛出异常
     */
    void writeWALLog(const std::string &operationType,
                     const rapidjson::Document &jsonData,
                     const std::string &version);

    /**
     * @brief 读取下一条WAL日志条目
     * @param operationType 输出参数，用于返回操作类型
     * @param jsonData 输出参数，用于返回解析后的JSON数据
     * @details 从WAL日志文件中顺序读取下一条日志记录
     *          如果读取成功，会更新内部的currentID以保持同步
     *          如果没有更多日志条目，operationType将为空字符串
     * 
     * 该函数主要用于数据库启动时的数据恢复过程，通过重放WAL日志
     * 来恢复系统到最后一次一致的状态
     */
    void readNextWALLog(std::string *operationType,
                        rapidjson::Document *jsonData);

private:
    uint64_t currentID;        ///< 当前日志ID计数器，用于生成唯一的日志标识符
    std::fstream walLogFile;   ///< WAL日志文件流对象，支持读写操作
};
