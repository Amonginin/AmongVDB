/**
 * @file constants.h
 * @brief 全局常量定义文件
 * @details 定义了系统中使用的所有常量，包括日志、请求参数、响应字段等
 */

#pragma once

// 日志配置
#define LOGGER_NAME "GlobalLogger"  // 全局日志记录器名称

// HTTP响应相关字段
#define RESPONSE_VECTORS "vectors"      // 返回的向量数据字段名
#define RESPONSE_DISTANCES "distances"  // 返回的距离数据字段名

// HTTP请求相关字段
#define REQUEST_VECTORS "vectors"       // 请求中的向量数据字段名
#define REQUEST_K "k"                   // 请求中的K值字段名（用于KNN搜索）
#define REQUEST_ID "id"                 // 请求中的ID字段名
#define REQUEST_INDEX_TYPE "indexType"  // 请求中的索引类型字段名

// 响应状态码相关
#define RESPONSE_RETCODE "retcode"           // 返回状态码字段名
#define RESPONSE_RETCODE_SUCCESS 0           // 成功状态码
#define RESPONSE_RETCODE_ERROR -1            // 错误状态码

// 响应其他字段
#define RESPONSE_ERROR_MSG "errorMsg"              // 错误信息字段名
#define RESPONSE_CONTENT_TYPE_JSON "application/json"  // HTTP响应Content-Type

// 索引类型
#define INDEX_TYPE_FLAT "FLAT"
#define INDEX_TYPE_HNSW "HNSW"
#define INDEX_TYPE_FILTER "filter"

// TODO: 过滤器类型
#define FILTER_TYPE_INT "INT"
#define FILTER_TYPE_STRING "STRING"
#define FILTER_TYPE_FLOAT "FLOAT"
