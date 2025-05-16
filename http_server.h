#pragma once

#include "faiss_index.h"
#include "httplib/httplib.h"
#include "index_factory.h"
#include "rapidjson/document.h"
#include <string>

class HttpServer
{
public:
    enum class CheckType
    {
        SEARCH,
        INSERT,
        UNKNOWN = -1
    };

    // 构造函数，初始化服务器对象
    HttpServer(const std::string &host, int port);

    // 启动服务器
    void start();

private:
    void searchHandler(const httplib::Request &req, httplib::Response &res);
    void insertHandler(const httplib::Request &req, httplib::Response &res);
    void setJsonResponse(const rapidjson::Document &json_response, httplib::Response &res);
    void setErrorJsonResponse(httplib::Response &res, int error_code, const std::string &error_message);
    bool isRequestValid(const rapidjson::Document &json_request, CheckType check_type);
    IndexFactory::IndexType getIndexTypeFromRequest(const rapidjson::Document &json_request);

    httplib::Server server;
    std::string host;
    int port;
};