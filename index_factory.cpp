#include "index_factory.h"
#include "hnswlib_index.h"
#include "faiss/IndexFlat.h"
#include "faiss/IndexIDMap.h"
#include "filter_index.h"
#include "logger.h"
#include <string.h>
// 创建一个名为 globalIndexFactory 的 IndexFactory 类型的全局实例
// 单例模式+工厂模式，但多线程下需要配合互斥锁防止并发问题
namespace
{
    // namespace { } 是一个匿名命名空间（unnamed namespace）。
    // 在 C++ 中，匿名命名空间中声明的所有内容只在当前文件（编译单元）中可见，类似于使用 static 修饰符来限制符号的可见性。
    IndexFactory globalIndexFactory;
}

IndexFactory *getGlobalIndexFactory()
{
    // 返回全局的 IndexFactory 实例
    return &globalIndexFactory;
}

/**
 * @brief 初始化向量索引
 *
 * @param type 索引类型，当前支持FLAT、HNSW 类型索引
 * @param dim 向量维度
 * @param numData 索引能容纳的最大向量数量
 * @param metric 距离度量方式（默认L2欧氏距离）
 *
 * @note 此函数会根据指定的索引类型、维度和度量方式创建相应的FAISS索引
 */
void IndexFactory::init(IndexType type, int dim, int numData, MetricType metric)
{
    // 根据传入的度量类型参数，确定FAISS索引使用的哪种度量方式
    // 因为FAISS的度量方式和我们的度量方式不一致，所以需要转换
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;

    // 根据索引类型创建相应的索引实例
    switch (type)
    {
    case IndexType::FLAT:
        // 创建一个扁平索引（暴力搜索，无压缩）
        // 1. 创建基础的IndexFlat对象，指定维度和度量方式
        // 2. 用IndexIDMap包装，以支持自定义ID映射
        // 3. 用FaissIndex进一步包装，适配我们系统的接口
        // 4. 存入索引映射表，以便后续通过类型访问
        indexMap[type] = new FaissIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
        break;
    case IndexType::HNSW:
        // 创建一个HNSW索引
        // 1. 创建HNSWLibIndex对象
        // 2. 存入索引映射表，以便后续通过类型访问
        indexMap[type] = new HNSWLibIndex(dim, numData, metric, 16, 200);
        break;
    case IndexType::FILTER:
        // 创建一个过滤索引
        // 1. 创建HNSWLibIndex对象
        // 2. 存入索引映射表，以便后续通过类型访问
        indexMap[type] = new FilterIndex();
        break;
    case IndexType::UNKNOWN:
    default:
        // 未知索引类型不做处理
        break;
    }
}

/**
 * @brief 获取指定类型的索引对象
 *
 * @param type 请求的索引类型（如FLAT等）
 * @return void* 返回对应类型的索引对象指针，若不存在则返回nullptr
 *
 * @note 返回的是void*类型，调用者使用前需转换为实际的对象进行使用
 */
void *IndexFactory::getIndex(IndexType type) const
{
    // 使用find函数在索引映射表indexMap中查找键为type的索引对象
    auto it = indexMap.find(type);

    // 如果找到，则返回对应的索引对象
    if (it != indexMap.end())
    {
        return it->second;
    }
    // 如果没有找到，则返回nullptr
    else
    {
        return nullptr;
    }
}

/**
 * @brief 保存所有创建的索引到指定文件夹
 * @param folderPath 索引文件保存的文件夹路径
 * @param scalarStorage 用于保存Scalar索引的存储对象
 *
 * 遍历indexMap中的每个索引，根据其类型生成文件名并调用相应的saveIndex方法。
 * Filter索引需要ScalarStorage来保存数据。
 */
void IndexFactory::saveIndex(const std::string &folderPath, ScalarStorage &scalarStorage)
{
    // 确保快照目录存在
    if (mkdir(folderPath.c_str(), 0755) == -1) {
        if (errno != EEXIST) {
            globalLogger->error("Failed to create snapshot directory {}: {}", folderPath, strerror(errno));
            return;
        }
    }
    globalLogger->debug("Snapshot directory {} ensured", folderPath);

    // 遍历所有已创建的索引
    for (const auto &indexEntry : indexMap)
    {
        IndexType type = indexEntry.first;
        void *index = indexEntry.second;

        // 为每个索引类型生成一个文件名，使用枚举值的字符串表示
        std::string fileName = folderPath + "/" +
                               std::to_string(static_cast<int>(type)) +
                               ".index";

        globalLogger->debug("Saving index type {} to file {}", static_cast<int>(type), fileName);

        // 根据索引类型调用相应的 saveIndex 方法
        switch (type)
        {
        case IndexType::FLAT:
            // 将void*指针转换为FaissIndex*并调用saveIndex
            static_cast<FaissIndex *>(index)->saveIndex(fileName);
            break;
        case IndexType::HNSW:
            // 将void*指针转换为HNSWLibIndex*并调用saveIndex
            static_cast<HNSWLibIndex *>(index)->saveIndex(fileName);
            break;
        case IndexType::FILTER:
            // 将void*指针转换为FilterIndex*并调用saveIndex，需要传入ScalarStorage
            static_cast<FilterIndex *>(index)->saveIndex(scalarStorage,fileName);
            break;
        case IndexType::UNKNOWN:
        default:
            // 未知或默认类型，跳过保存
            break;
        }
    }
    globalLogger->info("Completed saving all indices to {}", folderPath);
}

/**
 * @brief 从指定文件夹加载索引
 * @param folderPath 索引文件所在的文件夹路径
 * @param scalarStorage 用于加载Scalar索引的存储对象
 *
 * 遍历indexMap中的每个索引占位符，根据其类型生成文件名并尝试调用相应的loadIndex方法加载。
 * Filter索引需要ScalarStorage来加载数据。
 */
void IndexFactory::loadIndex(const std::string& folderPath, ScalarStorage &scalarStorage)
{
    // 遍历所有已知的索引占位符 (虽然此处遍历的是已创建的，但加载通常用于初始化，此处逻辑可能需要根据实际加载流程调整)
    for (const auto &indexEntry : indexMap)
    {
        IndexType type = indexEntry.first;
        void *index = indexEntry.second;

        // 为每个索引类型生成一个文件名，使用枚举值的字符串表示
        std::string fileName = folderPath + "/" +
                               std::to_string(static_cast<int>(type)) +
                               ".index";

        // 根据索引类型调用相应的 loadIndex 方法
        switch (type)
        {
        case IndexType::FLAT:
            // 将void*指针转换为FaissIndex*并调用loadIndex
            static_cast<FaissIndex *>(index)->loadIndex(fileName);
            break;
        case IndexType::HNSW:
            // 将void*指针转换为HNSWLibIndex*并调用loadIndex
            static_cast<HNSWLibIndex *>(index)->loadIndex(fileName);
            break;
        case IndexType::FILTER:
            // 将void*指针转换为FilterIndex*并调用loadIndex，需要传入ScalarStorage
            static_cast<FilterIndex *>(index)->loadIndex(scalarStorage,fileName);
            break;
        case IndexType::UNKNOWN:
        default:
            // 未知或默认类型，跳过加载
            break;
        }
    }
}   