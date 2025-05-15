/**
 * @file logger.cpp
 * @brief 日志系统实现文件
 * @details 实现全局日志记录器的初始化和配置功能
 */

#include "logger.h"
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
    
// 定义全局日志记录器
std::shared_ptr<spdlog::logger> globalLogger;

void initGlobalLogger() {
    try {
        // 创建支持彩色输出的控制台日志记录器
        // 参数"amongvdb"是日志记录器的名称
        // stdout_color_mt表示创建一个线程安全的、支持彩色输出的日志记录器
        globalLogger = spdlog::stdout_color_mt("amongvdb");
        
        // 设置日志输出格式
        // %Y-%m-%d %H:%M:%S.%e: 时间戳，精确到毫秒
        // %^%l%$: 日志级别，使用不同颜色显示
        // %t: 线程ID
        // %v: 实际的日志消息
        globalLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
        
        // 设置默认日志级别为debug
        // 需要更改时再手动设置
        globalLogger->set_level(spdlog::level::debug);
        
        // 输出初始化成功信息
        globalLogger->info("日志系统初始化成功");
    } catch (const spdlog::spdlog_ex& ex) {
        // 如果初始化失败，输出错误信息到标准错误流
        std::cerr << "日志系统初始化失败: " << ex.what() << std::endl;
        
        // 尝试创建基本的文件日志记录器
        try {
            // 创建基本的文件日志记录器，不包含彩色输出等高级特性
            auto file_logger = spdlog::basic_logger_mt("error_logger", "error.log");
            file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            file_logger->error("日志系统初始化失败: {}", ex.what());
            // 确保日志被写入文件
            file_logger->flush();
        } catch (const std::exception& e) {
            // 如果文件日志也失败，只能输出到标准错误流
            std::cerr << "无法创建错误日志文件: " << e.what() << std::endl;
        }
    }
}

void setLogLevel(spdlog::level::level_enum level) {
    // 检查全局日志记录器是否已初始化
    if (globalLogger) {
        // 设置新的日志级别
        globalLogger->set_level(level);
        
        // 输出日志级别变更信息
        globalLogger->info("日志级别已设置为: {}", spdlog::level::to_string_view(level));
    }
}