/**
 * @file http_server.h
 * @brief HTTP服务器类的头文件
 * 
 * 该文件定义了HTTP服务器类，用于处理向量数据库的HTTP请求。
 * 主要功能包括：
 * 1. 处理向量的插入、更新、搜索和查询请求
 * 2. 验证请求参数的合法性
 * 3. 生成JSON格式的响应
 */

#pragma once

#include "faiss_index.h"
#include "vector_database.h"
#include "httplib/httplib.h"
#include "index_factory.h"
#include "rapidjson/document.h"
#include <string>

/**
 * @class HttpServer
 * @brief HTTP服务器类，处理向量数据库的HTTP请求
 * 
 * 该类使用cpp-httplib库实现HTTP服务器功能，提供以下接口：
 * - 向量插入（/insert）
 * - 向量更新（/upsert）
 * - 向量搜索（/search）
 * - 向量查询（/query）
 */
class HttpServer
{
public:
    /**
     * @enum CheckType
     * @brief 请求验证类型枚举
     * 
     * 用于标识不同类型的请求验证：
     * - SEARCH: 搜索请求验证
     * - INSERT: 插入请求验证
     * - UPSERT: 更新请求验证
     * - UNKNOWN: 未知类型
     */
    enum class CheckType
    {
        SEARCH,     ///< 搜索请求验证
        INSERT,     ///< 插入请求验证
        UPSERT,     ///< 更新请求验证
        UNKNOWN = -1 ///< 未知类型
    };

    /**
     * @brief 构造函数
     * @param host 服务器主机地址
     * @param port 服务器端口号
     * @param vectorDatabase 向量数据库实例指针
     * 
     * 初始化HTTP服务器，设置监听地址和端口，并关联向量数据库实例
     */
    HttpServer(const std::string &host, int port, VectorDatabase *vectorDatabase);

    /**
     * @brief 启动HTTP服务器
     * 
     * 开始监听指定的主机地址和端口，处理HTTP请求
     */
    void start();

private:
    /**
     * @brief 处理搜索请求
     * @param req HTTP请求对象
     * @param res HTTP响应对象
     * 
     * 处理向量搜索请求，返回最相似的向量
     */
    void searchHandler(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief 处理插入请求
     * @param req HTTP请求对象
     * @param res HTTP响应对象
     * 
     * 处理向量插入请求，将新向量添加到数据库中
     */
    void insertHandler(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief 处理更新请求
     * @param req HTTP请求对象
     * @param res HTTP响应对象
     * 
     * 处理向量更新请求，更新已存在的向量或插入新向量
     */
    void upsertHandler(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief 处理查询请求
     * @param req HTTP请求对象
     * @param res HTTP响应对象
     * 
     * 处理向量查询请求，返回指定ID的向量信息
     */
    void queryHandler(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief 处理快照请求
     * @param req HTTP请求对象
     * @param res HTTP响应对象
     */
    void snapshotHandler(const httplib::Request &req, httplib::Response &res);

    /**
     * @brief 设置JSON格式的响应
     * @param json_response JSON响应文档
     * @param res HTTP响应对象
     * 
     * 将JSON文档转换为HTTP响应
     */
    void setJsonResponse(const rapidjson::Document &json_response, httplib::Response &res);

    /**
     * @brief 设置错误响应
     * @param res HTTP响应对象
     * @param error_code 错误代码
     * @param error_message 错误信息
     * 
     * 生成包含错误信息的JSON响应
     */
    void setErrorJsonResponse(httplib::Response &res, int error_code, const std::string &error_message);

    /**
     * @brief 验证请求参数的合法性
     * @param json_request JSON请求文档
     * @param check_type 验证类型
     * @return bool 验证是否通过
     * 
     * 根据不同的请求类型验证必要参数的存在性和格式
     */
    bool isRequestValid(const rapidjson::Document &json_request, CheckType check_type);

    /**
     * @brief 从请求中获取索引类型
     * @param json_request JSON请求文档
     * @return IndexFactory::IndexType 索引类型
     * 
     * 解析请求中的索引类型参数，返回对应的索引类型枚举值
     */
    IndexFactory::IndexType getIndexTypeFromRequest(const rapidjson::Document &json_request);

    httplib::Server server;           ///< HTTP服务器实例
    std::string host;                 ///< 服务器主机地址
    int port;                         ///< 服务器端口号
    VectorDatabase *vectorDatabase;   ///< 向量数据库实例指针
};