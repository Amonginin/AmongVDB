/**
 * @file vdb_server.cpp
 * @brief 向量数据库服务器主程序
 * @details 实现向量数据库服务器的启动和初始化流程
 */

#include "http_server.h"
#include "index_factory.h"
#include "logger.h"

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
    
    // 获取全局索引工厂实例
    IndexFactory* globalIndexFactory = getGlobalIndexFactory();
    if (globalIndexFactory == nullptr) {
        // 如果获取索引工厂失败，记录错误并退出
        globalLogger->info("Failed to get global index factory");
        return 1;
    }
    
    // 初始化FLAT类型的索引，用于向量存储和检索
    globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
    globalLogger->info("Global index factory initialized");

    // 创建HTTP服务器实例，监听本地9729端口
    HttpServer http_server("localhost", 9729);
    globalLogger->info("HTTP server created");
    // 启动HTTP服务器
    http_server.start();
    globalLogger->info("HTTP server started");
    
    return 0;   
}