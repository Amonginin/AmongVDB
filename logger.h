/**
 * @file logger.h
 * @brief 日志系统头文件
 * @details 提供全局日志记录器的声明和初始化函数
 */

#pragma once

#include "spdlog/spdlog.h"

/**
 * @brief 全局日志记录器指针
 * @details 用于在整个应用程序中共享同一个日志记录器实例
 * @note 使用extern声明，实际定义在logger.cpp中
 */
extern std::shared_ptr<spdlog::logger> global_logger;

/**
 * @brief 初始化全局日志记录器
 * @details 创建并配置全局日志记录器，设置输出格式和默认日志级别
 * @note 应该在程序启动时调用此函数
 */
void init_global_logger();

/**
 * @brief 设置日志级别
 * @param level 要设置的日志级别
 * @details 可选的日志级别：
 *          - trace: 最详细的调试信息
 *          - debug: 调试信息
 *          - info: 一般信息
 *          - warn: 警告信息
 *          - error: 错误信息
 *          - critical: 严重错误
 *          - off: 关闭所有日志
 */
void set_log_level(spdlog::level::level_enum level);