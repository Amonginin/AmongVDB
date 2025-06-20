/**
 * @file vdb_server.cpp
 * @brief 向量数据库服务器主程序
 * @details 实现向量数据库服务器的启动和初始化流程
 */

#include "http_server.h"
#include "index_factory.h"
#include "logger.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h> // For checking errno
#include <string.h> // For strerror

/**
 * @brief 主函数
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码：0表示正常退出，非0表示异常退出
 * @details 程序执行流程：
 *          1. 初始化日志系统
 *          2. 设置日志级别
 *          3. 初始化索引工厂
 *          4. 启动HTTP服务器
 */
int main(int argc, char* argv[]) {
    // 初始化全局日志系统
    initGlobalLogger();
    // 设置日志级别为debug，用于开发调试
    setLogLevel(spdlog::level::debug);
    globalLogger->info("Global logger initialized");

    // 设置向量维度
    int dim = 1;
    // 设置hnsw索引能容纳的最大向量数量
    int numData = 1000;
    
    // 获取全局索引工厂实例
    IndexFactory* globalIndexFactory = getGlobalIndexFactory();
    if (globalIndexFactory == nullptr) {
        // 如果获取索引工厂失败，记录错误并退出
        globalLogger->info("Failed to get global index factory");
        return 1;
    }
    
    // 初始化FLAT类型的索引
    globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
    // 初始化HNSW类型的索引
    globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim, numData);
    // 初始化FILTER类型的索引
    globalIndexFactory->init(IndexFactory::IndexType::FILTER);
    globalLogger->info("Global index factory initialized");

    // 初始化VectorDatabase对象
    std::string dbPath = "ScalarStorage"; // 数据库路径，本项目的ScalarStorage目录下
    std::string walLogPath = "WALLogStorage/WALLog"; // WAL日志路径，本项目的WALLogStorage目录下

    // 确保WAL日志目录存在
    const char* logDir = "WALLogStorage";
    if (mkdir(logDir, 0755) == -1) {
        if (errno != EEXIST) {
            globalLogger->error("Failed to create WAL log directory {}: {}", logDir, strerror(errno));
            return 1; // Exit if directory creation fails for reasons other than existence
        }
    }

    VectorDatabase vectorDatabase(dbPath, walLogPath);

    // 重新加载数据库中的数据
    vectorDatabase.reloadDatabase();
    globalLogger->info("VectorDatabase initialized");

    // 创建HTTP服务器实例，监听本地9729端口
    HttpServer http_server("localhost", 9729, &vectorDatabase);
    globalLogger->info("HTTP server created");
    // 启动HTTP服务器
    http_server.start();
    globalLogger->info("HTTP server started");
    
    return 0;   
}