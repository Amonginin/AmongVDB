#include "http_server.h"
#include "faiss_index.h"
#include "hnswlib_index.h"
#include "index_factory.h"
#include "constants.h"
#include "logger.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// NOTE: 括号内的都是传入的参数，括号外的是成员变量
// 使用cpp-httplib库创建HTTP服务器对象server，并设置监听的主机和端口
HttpServer::HttpServer(const std::string &host, int port, VectorDatabase *vectorDatabase)
    : host(host), port(port), vectorDatabase(vectorDatabase)
{
    // NOTE: lambda表达式写法
    // 当请求路径为 "/insert" 时，调用 insertHandler 函数处理请求
    server.Post("/insert", [&](const httplib::Request &req, httplib::Response &res)
                { insertHandler(req, res); });
    // 当请求路径为 "/search" 时，调用 searchHandler 函数处理请求
    server.Post("/search", [&](const httplib::Request &req, httplib::Response &res)
                { searchHandler(req, res); });
    // 当请求路径为 "/upsert" 时，调用 upsertHandler 函数处理请求
    server.Post("/upsert", [&](const httplib::Request &req, httplib::Response &res)
                { upsertHandler(req, res); });
    // 当请求路径为 "/query" 时，调用 queryHandler 函数处理请求
    server.Post("/query", [&](const httplib::Request &req, httplib::Response &res)
                { queryHandler(req, res); });
    server.Post("/admin/snapshot", [&](const httplib::Request &req, httplib::Response &res)
                { snapshotHandler(req, res); });
}

void HttpServer::start()
{
    server.listen(host.c_str(), port);
}

/**
 * @brief 将 RapidJSON Document 对象转换为 HTTP 响应
 * @details 该方法执行以下步骤：
 *          1. 创建一个字符串缓冲区用于存储 JSON 字符串
 *          2. 使用 RapidJSON Writer 将 Document 对象序列化为 JSON 字符串
 *          3. 设置 HTTP 响应的内容类型和主体
 * @param jsonResponse RapidJSON Document 对象，其具体内容是要发送的 JSON 数据
 * @param res HTTP 响应对象，用于设置响应内容
 */
void HttpServer::setJsonResponse(const rapidjson::Document &jsonResponse, httplib::Response &res)
{
    // 1.创建 StringBuffer 对象，用于存储序列化后的 JSON 字符串
    rapidjson::StringBuffer buffer;

    // 2.创建 Writer 对象，用于将 Document 对象写入 buffer
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    // 3.使用 Accept 方法遍历 Document 对象的所有节点
    // 并通过 Writer 将其转换为 JSON 字符串
    jsonResponse.Accept(writer);

    // 4.设置 HTTP 响应
    // - 使用 buffer.GetString() 获取生成的 JSON 字符串
    // - RESPONSE_CONTENT_TYPE_JSON 指定内容类型为 "application/json"
    res.set_content(buffer.GetString(), RESPONSE_CONTENT_TYPE_JSON);
}

/**
 * @brief 设置错误响应的 JSON 格式
 * @details 该方法用于构建标准的错误响应格式，包含以下功能：
 *          1. 创建一个新的 JSON Document 对象
 *          2. 添加错误码（retcode）
 *          3. 添加错误信息（errorMsg）
 *          4. 将 JSON 转换为 HTTP 响应
 *
 * @param res HTTP响应对象的引用，用于存储最终的响应内容
 * @param error_code 错误码，表示错误类型
 * @param error_message 错误信息描述
 */
void HttpServer::setErrorJsonResponse(httplib::Response &res, int error_code, const std::string &error_message)
{
    // 创建 JSON 文档对象
    rapidjson::Document jsonResponse;
    // 设置为对象类型
    jsonResponse.SetObject();
    // 获取分配器，用于内存分配
    rapidjson::Document::AllocatorType &allocator = jsonResponse.GetAllocator();

    // 添加错误码字段
    jsonResponse.AddMember(RESPONSE_RETCODE, error_code, allocator);
    // 添加错误信息字段，使用 StringRef 避免字符串拷贝
    jsonResponse.AddMember(RESPONSE_ERROR_MSG, rapidjson::StringRef(error_message.c_str()), allocator);

    // 将 JSON 文档转换为 HTTP 响应
    setJsonResponse(jsonResponse, res);
}

/**
 * @brief 验证请求参数的合法性
 * @details 根据不同的检查类型验证JSON请求中必要参数的存在性和格式：
 *          - SEARCH类型：检查vectors、k和index_type参数
 *          - INSERT类型：检查vectors、id和index_type参数
 * @param jsonRequest JSON请求文档对象
 * @param check_type 检查类型（SEARCH或INSERT）
 * @return bool 如果所有必要参数都存在且格式正确则返回true，否则返回false
 */
bool HttpServer::isRequestValid(const rapidjson::Document &jsonRequest,
                                CheckType check_type)
{
    switch (check_type)
    {
    case CheckType::SEARCH:
        // 检查搜索请求必要参数：
        // 1. vectors字段必须存在
        return jsonRequest.HasMember(REQUEST_VECTORS) &&
               // 2. k字段必须存在
               jsonRequest.HasMember(REQUEST_K) &&
               // 3. index_type字段如果存在必须是字符串类型
               (!jsonRequest.HasMember(REQUEST_INDEX_TYPE) ||
                jsonRequest[REQUEST_INDEX_TYPE].IsString());
    case CheckType::INSERT:
        // 检查插入请求必要参数：
        // 1. vectors字段必须存在（待插入的向量数据）
        return jsonRequest.HasMember(REQUEST_VECTORS) &&
               // 2. id字段必须存在（向量的唯一标识）
               jsonRequest.HasMember(REQUEST_ID) &&
               // 3. index_type字段如果存在必须是字符串类型
               (!jsonRequest.HasMember(REQUEST_INDEX_TYPE) ||
                jsonRequest[REQUEST_INDEX_TYPE].IsString());
    case CheckType::UPSERT:
        // 检查更新请求必要参数：
        // 1. vectors字段必须存在（待更新向量数据）
        return jsonRequest.HasMember(REQUEST_VECTORS) &&
               // 2. id字段必须存在（向量的唯一标识）
               jsonRequest.HasMember(REQUEST_ID) &&
               // 3. index_type字段如果存在必须是字符串类型
               (!jsonRequest.HasMember(REQUEST_INDEX_TYPE) ||
                jsonRequest[REQUEST_INDEX_TYPE].IsString());
    default:
        return false;
    }
}

/**
 * @brief 从请求中获取索引类型
 * @details 该函数从 JSON 请求中解析索引类型参数：
 *          1. 如果请求中包含 indexType 字段，则根据其值返回对应的索引类型
 *          2. 如果请求中不包含 indexType 字段，则返回 UNKNOWN 类型
 * @param jsonRequest JSON 请求文档对象
 * @return IndexFactory::IndexType 返回解析出的索引类型
 * @note 目前仅支持 FLAT 类型的索引，其他类型将返回 UNKNOWN
 */
IndexFactory::IndexType HttpServer::getIndexTypeFromRequest(const rapidjson::Document &jsonRequest)
{
    // 如果请求中包含 indexType 字段
    if (jsonRequest.HasMember(REQUEST_INDEX_TYPE))
    {
        // 获取索引类型字符串
        std::string indexTypeStr = jsonRequest[REQUEST_INDEX_TYPE].GetString();
        // 根据字符串值返回对应的索引类型
        if (indexTypeStr == INDEX_TYPE_FLAT)
        {
            return IndexFactory::IndexType::FLAT;
        }
        else if (indexTypeStr == INDEX_TYPE_HNSW)
        {
            return IndexFactory::IndexType::HNSW;
        }
        // TODO: 支持其他索引类型
    }
    // 如果请求中不包含 indexType 字段或类型未知，返回 UNKNOWN
    return IndexFactory::IndexType::UNKNOWN;
}

/**
 * @brief 处理向量搜索请求的处理器函数
 * @details 该函数处理客户端发送的向量搜索请求，主要功能包括：
 *          1. 解析和验证请求中的JSON数据
 *          2. 提取搜索参数（查询向量、返回数量k、索引类型）
 *          3. 调用相应的索引进行向量搜索
 *          4. 将搜索结果格式化为JSON响应
 * @param req HTTP请求对象，包含客户端发送的请求数据
 * @param res HTTP响应对象，用于向客户端发送响应
 */
void HttpServer::searchHandler(const httplib::Request &req, httplib::Response &res)
{
    // 打印接收到了搜索请求
    globalLogger->debug("Received search request");

    // 解析请求体中的JSON请求内容
    rapidjson::Document jsonRequest;
    jsonRequest.Parse(req.body.c_str());

    // 打印用户的输入参数
    globalLogger->info("Search request parameters: {}", req.body);

    // 检查JSON文档是否为有效的对象
    if (!jsonRequest.IsObject())
    {
        globalLogger->error("Invalid JSON request");
        res.status = 400; // Bad Request - 请求格式错误
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Invalid JSON request");
        return;
    }

    // 检查请求参数的合法性（vectors和k参数是否存在且格式正确）
    if (!isRequestValid(jsonRequest, CheckType::SEARCH))
    {
        globalLogger->error(
            "Missing vectors or k parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Missing vectors or k parameters in the request");
        return;
    }

    // 获取请求中的查询参数：vectors待查询向量
    // TODO: 支持查询多个向量（目前仅支持单个，即vectors字段的值仅为单个向量）
    std::vector<float> query;
    for (const auto &q : jsonRequest[REQUEST_VECTORS].GetArray())
    {
        // 将待查询向量的每个元素添加到query数组中
        query.push_back(q.GetFloat());
    }
    // 获取请求中的查询参数：k返回的结果向量的数量
    int k = jsonRequest[REQUEST_K].GetInt();

    globalLogger->debug("Query parameters: k = {}", k);

    // 获取请求中的查询参数：indexType索引类型
    IndexFactory::IndexType index_type = getIndexTypeFromRequest(jsonRequest);
    if (index_type == IndexFactory::IndexType::UNKNOWN)
    {
        globalLogger->error(
            "Invalid indexType parameter in the request");
        // 如果索引类型无效，返回错误响应
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Invalid indexType parameter in the request");
        return;
    }

    // 使用VectorDatabase 的 search 接口执行查询
    std::pair<std::vector<long>, std::vector<float>> results = vectorDatabase->search(jsonRequest);

    // 将结果转换为JSON格式
    rapidjson::Document jsonResponse;
    jsonResponse.SetObject();
    rapidjson::Document::AllocatorType &allocator = jsonResponse.GetAllocator();

    // 检查搜索结果的有效性并构建响应数据
    bool valid_results = false;
    rapidjson::Value vectors(rapidjson::kArrayType);   // 存储找到的向量ID
    rapidjson::Value distances(rapidjson::kArrayType); // 存储对应的距离值

    // 遍历搜索结果，只添加有效的结果（ID != -1）
    for (size_t i = 0; i < results.first.size(); i++)
    {
        if (results.first[i] != -1) // -1表示无效结果
        {
            valid_results = true;
            vectors.PushBack(results.first[i], allocator);
            distances.PushBack(results.second[i], allocator);
        }
    }

    // 如果存在有效结果，将结果添加到响应中
    if (valid_results)
    {
        jsonResponse.AddMember(RESPONSE_VECTORS, vectors.Move(), allocator);
        jsonResponse.AddMember(RESPONSE_DISTANCES, distances.Move(), allocator);
    }
    // 设置返回码为成功
    jsonResponse.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
    // 设置JSON响应
    setJsonResponse(jsonResponse, res);
}

/**
 * @brief 处理向量插入请求
 * @param req HTTP请求对象，包含插入请求的参数
 * @param res HTTP响应对象，用于返回处理结果
 * 
 * 该函数处理向量的插入请求，主要功能包括：
 * 1. 解析JSON格式的请求体
 * 2. 验证请求参数的合法性
 * 3. 提取向量数据和ID
 * 4. 根据索引类型选择对应的索引实例
 * 5. 执行向量插入操作
 * 6. 返回处理结果
 */
void HttpServer::insertHandler(const httplib::Request &req,
                               httplib::Response &res)
{
    // 打印接收到了插入请求
    globalLogger->debug("Received insert request");

    // 解析请求体中的JSON请求内容
    rapidjson::Document jsonRequest;
    jsonRequest.Parse(req.body.c_str());

    // 打印用户的输入参数
    globalLogger->info("Insert request parameters: {}", req.body);

    // 检查JSON文档是否为有效的对象
    if (!jsonRequest.IsObject())
    {
        globalLogger->error("Invalid JSON request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Invalid JSON request");
        return;
    }

    // 检查请求参数的合法性（vectors和label参数是否存在且格式正确）
    if (!isRequestValid(jsonRequest, CheckType::INSERT))
    {
        globalLogger->error(
            "Missing vectors or id parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Missing vectors or k parameter in the request");
        return;
    }

    // 获取请求中的插入参数：data待插入向量
    std::vector<float> data;
    for (const auto &d : jsonRequest[REQUEST_VECTORS].GetArray())
    {
        data.push_back(d.GetFloat());
    }
    // 获取请求中的插入参数：id待插入向量的唯一标识
    uint64_t id = jsonRequest[REQUEST_ID].GetUint64();
    globalLogger->debug("Insert parameters: id = {}", id);

    // 获取请求中的插入参数：indexType索引类型
    IndexFactory::IndexType indexType = getIndexTypeFromRequest(jsonRequest);
    if (indexType == IndexFactory::IndexType::UNKNOWN)
    {
        globalLogger->error(
            "Invalid indexType parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Invalid indexType parameter in the request");
        return;
    }

    // 从全局索引工厂获取对应类型的索引实例
    void *index = getGlobalIndexFactory()->getIndex(indexType);

    // 根据索引类型初始化索引对象并调用insert_vectors函数
    switch (indexType)
    {
    case IndexFactory::IndexType::FLAT:
    {
        FaissIndex *faissIndex = static_cast<FaissIndex *>(index);
        faissIndex->insertVectors(data, id);
        break;
    }
    case IndexFactory::IndexType::HNSW:
    {
        HNSWLibIndex *hnswIndex = static_cast<HNSWLibIndex *>(index);
        hnswIndex->insertVectors(data, id);
        break;
    }
    // TODO: 支持其他索引类型
    case IndexFactory::IndexType::UNKNOWN:
    default:
        break;
    }

    // 设置返回码为成功
    rapidjson::Document jsonResponse;
    jsonResponse.SetObject();
    rapidjson::Document::AllocatorType &allocator = jsonResponse.GetAllocator();
    jsonResponse.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
    // 设置JSON响应
    setJsonResponse(jsonResponse, res);
}

/**
 * @brief 处理向量更新请求
 * @param req HTTP请求对象，包含更新请求的参数
 * @param res HTTP响应对象，用于返回处理结果
 * 
 * 该函数处理向量的更新请求，主要功能包括：
 * 1. 解析JSON格式的请求体
 * 2. 验证请求参数的合法性
 * 3. 提取向量ID和索引类型
 * 4. 调用向量数据库的upsert方法执行更新操作
 * 5. 返回处理结果
 */
void HttpServer::upsertHandler(const httplib::Request &req, httplib::Response &res)
{
    // 打印接收到了更新请求
    globalLogger->debug("Received upsert request");

    // 解析请求体中的JSON请求内容
    rapidjson::Document jsonRequest;
    jsonRequest.Parse(req.body.c_str());

    // 打印用户的输入参数
    globalLogger->info("Upsert request parameters: {}", req.body);

    // 检查JSON文档是否为有效的对象
    if (!jsonRequest.IsObject())
    {
        globalLogger->error("Invalid JSON request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Invalid JSON request");
        return;
    }

    // 检查请求参数的合法性（vectors和id参数是否存在且格式正确）
    if (!isRequestValid(jsonRequest, CheckType::UPSERT)){
        globalLogger->error("Missing vectors or id parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Missing vectors or id parameters in the request");
        return;
    }

    // 获取请求中的更新参数：id待更新向量的唯一标识
    uint64_t id = jsonRequest[REQUEST_ID].GetUint64();
    globalLogger->debug("Upsert parameters: id = {}", id);
    // 获取请求参数中的索引类型
    IndexFactory::IndexType indexType = getIndexTypeFromRequest(jsonRequest);

    // 调用 VectorDatabase::upsert 接口执行更新操作
    vectorDatabase->upsert(id, jsonRequest, indexType);
    // 调用 VectorDatabase::writeWALLog 接口写入 WAL 日志
    vectorDatabase->writeWALLog("upsert", jsonRequest);

    rapidjson::Document jsonResponse;
    jsonResponse.SetObject();
    rapidjson::Document::AllocatorType &allocator = jsonResponse.GetAllocator();
    jsonResponse.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
    setJsonResponse(jsonResponse, res);
}

/**
 * @brief 处理向量查询请求
 * @param req HTTP请求对象，包含查询请求的参数
 * @param res HTTP响应对象，用于返回处理结果
 * 
 * 该函数处理向量的查询请求，主要功能包括：
 * 1. 解析JSON格式的请求体
 * 2. 验证请求参数的合法性
 * 3. 提取向量ID
 * 4. 调用向量数据库的query方法执行查询操作
 * 5. 将查询结果转换为JSON格式并返回
 */
void HttpServer::queryHandler(const httplib::Request &req, httplib::Response &res)
{
    // 打印接收到了查询请求
    globalLogger->debug("Received query request");

    // 解析请求体中的JSON请求内容
    rapidjson::Document jsonRequest;
    jsonRequest.Parse(req.body.c_str());

    // 打印用户的输入参数
    globalLogger->info("Query request parameters: {}", req.body);

    // 检查JSON文档是否为有效的对象
    if (!jsonRequest.IsObject())
    {
        globalLogger->error("Invalid JSON request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                             "Invalid JSON request");
        return;
    }

    // 从JSON请求中获取查询参数：id待查询数据的唯一标识
    uint64_t id = jsonRequest[REQUEST_ID].GetUint64();
    globalLogger->debug("Query parameters: id = {}", id);

    // 查询JSON数据
    rapidjson::Document jsonData = vectorDatabase->query(id);

    // 将结果转换为JSON格式
    rapidjson::Document jsonResponse;
    jsonResponse.SetObject();
    rapidjson::Document::AllocatorType &allocator = jsonResponse.GetAllocator();

    // 如果查询到向量，则将jsonData对象的内容合并到jsonResponse中
    if (!jsonData.IsNull())
    {
        for (auto it = jsonData.MemberBegin(); it != jsonData.MemberEnd(); ++it)
        {
            jsonResponse.AddMember(it->name, it->value, allocator);
        }
    }
    // 设置返回码为成功
    jsonResponse.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
    // 设置JSON响应
    setJsonResponse(jsonResponse, res);
}

void HttpServer::snapshotHandler(const httplib::Request &req, httplib::Response &res)
{
    // 打印接收到了快照请求
    globalLogger->debug("Received snapshot request");

    vectorDatabase->takeSnapshot();

    // 将结果转换为JSON格式
    rapidjson::Document jsonResponse;
    jsonResponse.SetObject();
    rapidjson::Document::AllocatorType &allocator = jsonResponse.GetAllocator();
    jsonResponse.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
    setJsonResponse(jsonResponse, res);
}