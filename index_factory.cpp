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

void IndexFactory::init(IndexType type, int dim, MetricType metric)
{
    //
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;
    switch (type)
    {
    case IndexType::FLAT:
        index_map[type] = new FaissIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
        break;
    default:
        break;
    }
}

void *IndexFactory::getIndex(IndexType type) const
{
    // 使用find函数在索引映射表index_map中查找键为 type 的索引对象
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