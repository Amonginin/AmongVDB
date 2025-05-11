#include "index_factory.h"
#include "faiss/Index.h"
#include "faiss/IndexIDMap.h"

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
 * @param type 索引类型，当前支持FLAT类型索引
 * @param dim 向量维度
 * @param metric 距离度量方式（L2欧氏距离或内积）
 * 
 * @note 此函数会根据指定的索引类型、维度和度量方式创建相应的FAISS索引
 */
void IndexFactory::init(IndexType type, int dim, MetricType metric)
{
    // 根据传入的度量类型参数，确定使用FAISS的哪种度量方式
    // MetricType::L2 对应欧氏距离，否则使用内积（余弦相似度）
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;
    
    // 根据索引类型选择不同的索引实现
    switch (type)
    {
    case IndexType::FLAT:
        // 创建一个平坦索引（暴力搜索，无压缩）
        // 步骤：
        // 1. 创建基础的IndexFlat对象，指定维度和度量方式
        // 2. 用IndexIDMap包装，以支持自定义ID映射
        // 3. 用FaissIndex进一步包装，适配我们系统的接口
        // 4. 存入索引映射表，以便后续通过类型访问
        index_map[type] = new FaissIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
        break;
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
    // 使用find函数在索引映射表index_map中查找键为type的索引对象
    auto it = index_map.find(type);
    
    // 如果找到，则返回对应的索引对象
    if (it != index_map.end())
    {
        return it->second;
    }
    // 如果没有找到，则返回nullptr
    else
    {
        return nullptr;
    }
}