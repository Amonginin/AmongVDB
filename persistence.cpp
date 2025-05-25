/**
 * @file persistence.cpp
 * @brief 持久化管理实现文件
 * @details 实现了向量数据库的WAL（Write-Ahead Logging）日志管理功能
 *          提供数据持久化、事务日志记录和数据恢复等核心功能
 */

#include "persistence.h"
#include "logger.h"
#include "index_factory.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
#include <sstream>

/**
 * @brief 构造函数实现
 * @details 设置日志ID计数器的初始值为1，确保生成的日志ID从1开始递增
 *          设置最后一条快照ID计数器的初始值为0，确保快照ID从0开始递增
 */
Persistence::Persistence()
{
    currentID = 1;      // 初始化日志ID计数器，从1开始
    lastSnapshotID = 0; // 初始化最后一条快照ID计数器，从0开始
}

/**
 * @brief 析构函数实现
 * @details 清理资源，确保WAL日志文件正确关闭
 */
Persistence::~Persistence()
{
    // 检查WAL日志文件是否仍然打开
    if (walLogFile.is_open())
    {
        walLogFile.close(); // 关闭文件，确保所有缓冲的数据都被写入磁盘
    }
}

/**
 * @brief 初始化WAL日志文件实现
 * @param localPath WAL日志文件的本地存储路径
 * @details 以读写追加模式打开WAL日志文件，支持以下操作：
 *          - std::ios::in: 支持从文件读取（用于数据恢复）
 *          - std::ios::out: 支持向文件写入（用于记录新操作）
 *          - std::ios::app: 追加模式，新内容添加到文件末尾
 *
 *          如果文件不存在，系统会自动创建；如果文件已存在，新内容会追加到末尾
 * @throws std::runtime_error 当文件打开失败时抛出异常
 */
void Persistence::init(const std::string &localPath)
{
    // 以 读、写、追加 模式打开WAL日志文件
    walLogFile.open(localPath, std::ios::in | std::ios::out | std::ios::app);

    // 检查文件是否成功打开
    if (!walLogFile.is_open())
    {
        // 记录错误日志，包含系统错误信息
        globalLogger->error("An error occurred while writing the WAL log entry. Reason: {}", std::strerror(errno));

        // 抛出运行时异常，中断程序执行
        throw std::runtime_error("Failed to open WAL log file at path: " + localPath);
    }

    loadLastSnapshotID();
}

/**
 * @brief 递增并获取下一个日志ID的实现
 * @return uint64_t 返回递增后的新日志ID
 * @details 该函数是线程不安全的，在多线程环境下需要额外的同步机制
 *          每次调用都会使内部计数器递增1，并返回新的值
 *          返回的ID用于唯一标识每个WAL日志条目
 */
uint64_t Persistence::increaseID()
{
    currentID++;      // 递增内部计数器
    return currentID; // 返回新的日志ID
}

/**
 * @brief 获取当前日志ID的实现
 * @return uint64_t 返回当前的日志ID值（不进行递增操作）
 * @details 该函数用于查询当前的日志ID状态，不会修改内部计数器
 */
uint64_t Persistence::getID() const
{
    return currentID; // 返回当前日志ID，不递增
}

/**
 * @brief 写入WAL日志条目的实现
 * @param operationType 操作类型字符串（如"upsert"、"delete"、"query"）
 * @param jsonData 包含操作数据的JSON文档对象
 * @param version 数据版本号字符串
 * @details 该函数执行以下步骤：
 *          1. 生成新的日志ID
 *          2. 将JSON文档序列化为字符串
 *          3. 按照固定格式写入日志文件
 *          4. 检查写入状态并处理错误
 *          5. 强制刷新缓冲区以确保数据持久化
 *
 *          WAL日志条目格式：logID|version|operationType|jsonDataString
 *          使用管道符(|)作为字段分隔符
 */
void Persistence::writeWALLog(const std::string &operationType,
                              const rapidjson::Document &jsonData,
                              const std::string &version)
{
    // 生成新的日志ID
    uint64_t logID = increaseID();

    // 将JSON文档序列化为字符串
    rapidjson::StringBuffer buffer;                            // 创建字符串缓冲区
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer); // 创建JSON写入器
    jsonData.Accept(writer);                                   // 将JSON文档写入缓冲区

    // 按照WAL日志格式写入文件：logID|version|operationType|jsonDataString
    walLogFile << logID << "|" << version << "|" << operationType << "|" << buffer.GetString() << std::endl;

    // 检查写入操作是否成功
    if (walLogFile.fail())
    {
        // 记录错误日志
        globalLogger->error("An error occurred while writing the WAL log entry. Reason: {}", 
        std::strerror(errno));
    }
    else
    {
        // 记录成功写入的调试信息
        globalLogger->debug("Successfully wrote WAL log entry: logID={}, version={}, operationType={}, jsonDataStr={}",
                            logID, version, operationType, buffer.GetString());

        // 强制将缓冲区中的数据刷新到磁盘，确保数据持久化
        walLogFile.flush();
    }
}

/**
 * @brief 读取下一条WAL日志条目的实现
 * @param operationType 输出参数指针，用于返回操作类型
 * @param jsonData 输出参数指针，用于返回解析后的JSON数据
 * @details 该函数执行以下步骤：
 *          1. 从文件中读取一行完整的日志记录
 *          2. 使用管道符(|)分割各个字段
 *          3. 解析并提取logID、version、operationType和jsonData
 *          4. 更新内部的currentID以保持同步
 *          5. 将解析后的数据通过指针参数返回给调用者
 *
 *          如果没有更多日志条目可读，operationType将保持为空字符串
 *          该函数主要用于系统启动时的数据恢复过程
 */
void Persistence::readNextWALLog(std::string *operationType,
                                 rapidjson::Document *jsonData)
{
    globalLogger->debug("Reading next WAL log entry");

    std::string line;

    // 尝试从文件中读取一行
    while (std::getline(walLogFile, line))
    {
        // 使用字符串流来分割日志行
        std::istringstream iss(line);
        std::string logIDStr, version, jsonDataStr;

        // 按照WAL日志格式分割各个字段：logID|version|operationType|jsonDataString
        std::getline(iss, logIDStr, '|');       // 提取日志ID字符串
        std::getline(iss, version, '|');        // 提取版本号
        std::getline(iss, *operationType, '|'); // 提取操作类型（通过指针返回）
        std::getline(iss, jsonDataStr, '|');    // 提取JSON数据字符串

        // 将日志ID字符串转换为uint64_t类型
        uint64_t logID = std::stoull(logIDStr);

        // 如果读取到的日志ID大于当前ID，则更新currentID以保持同步
        if (logID > currentID)
        {
            currentID = logID;
        }

        if (logID > lastSnapshotID)
        {
            jsonData->Parse(jsonDataStr.c_str());
            globalLogger->debug("Read WAL log entry: logID={}, version={}, operationType={}, jsonDataStr={}",
                                logID, version, *operationType, jsonDataStr);
            return;
        }
        else
        {
            globalLogger->debug("Skip read WAL log entry: logID={}, version={}, operationType={}, jsonDataStr={}",
                                logID, version, *operationType, jsonDataStr);
        }
    }

    operationType->clear();
    // 没有更多日志条目可读，清除文件流的错误状态
    walLogFile.clear();

    // 记录调试信息
    globalLogger->debug("No more WAL log entries to read");
}

/**
 * @brief 执行快照操作，保存当前所有索引和最后ID
 * @param scalarStorage 用于保存Scalar索引
 */
void Persistence::takeSnapshot(ScalarStorage &scalarStorage)
{
    // 记录日志
    globalLogger->debug("Taking snapshot");

    // 更新最后快照ID为当前ID
    lastSnapshotID = currentID;

    // 定义快照文件夹路径
    std::string snapshotFolderPath = "snapshots";

    // 获取全局索引工厂
    IndexFactory *indexFactory = getGlobalIndexFactory();
    // 调用索引工厂保存所有索引
    indexFactory->saveIndex(snapshotFolderPath, scalarStorage);

    // 保存最后快照ID到文件
    saveLastSnapshotID();
}

/**
 * @brief 从快照文件加载索引
 * @param scalarStorage 用于加载Scalar索引
 */
void Persistence::loadSnapshot(ScalarStorage &scalarStorage)
{
    // 记录日志
    globalLogger->debug("Loading snapshot");

    // 定义快照文件夹路径
    std::string snapshotFolderPath = "snapshots";

    // 获取全局索引工厂
    IndexFactory *indexFactory = getGlobalIndexFactory();
    // 调用索引工厂加载所有索引
    indexFactory->loadIndex(snapshotFolderPath, scalarStorage);
}

/**
 * @brief 保存最后快照ID到文件
 *
 * 将lastSnapshotID保存到名为"lastSnapshotID"的文件中。
 */
void Persistence::saveLastSnapshotID()
{
    // 打开文件进行写入
    std::ofstream file("lastSnapshotID");
    if (file.is_open())
    {
        // 写入ID
        file << lastSnapshotID;
        // 关闭文件
        file.close();
    }
    else
    {
        // 记录错误日志
        globalLogger->error("Failed to open file lastSnapshotID for writing");
    }
    // 记录日志
    globalLogger->debug("Last snapshot ID saved: {}", lastSnapshotID);
}

/**
 * @brief 从文件加载最后快照ID
 *
 * 从名为"lastSnapshotID"的文件中读取lastSnapshotID。
 */
void Persistence::loadLastSnapshotID()
{
    // 打开文件进行读取
    std::ifstream file("lastSnapshotID");
    if (file.is_open())
    {
        // 读取ID
        file >> lastSnapshotID;
        // 关闭文件
        file.close();
    }
    else
    {
        // 记录错误日志
        globalLogger->error("Failed to open file lastSnapshotID for reading");
    }
    // 记录日志
    globalLogger->debug("Last snapshot ID loaded: {}", lastSnapshotID);
}