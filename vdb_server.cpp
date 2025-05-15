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
    init_global_logger();
    // 设置日志级别为debug，用于开发调试
    set_log_level(spdlog::level::debug);

    // 设置向量维度
    int dim = 1;
    
    // 获取全局索引工厂实例
    IndexFactory* glovalIndexFactory = getGlobalIndexFactory();
    if (glovalIndexFactory == nullptr) {
        // 如果获取索引工厂失败，记录错误并退出
        LOG_ERROR("Failed to get global index factory");
        return 1;
    }
    
    // 初始化FLAT类型的索引，用于向量存储和检索
    glovalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);

    // 创建HTTP服务器实例，监听本地9729端口
    HttpServer http_server("localhost", 9729);
    // 启动HTTP服务器
    http_server.start();
    
    return 0;   
}