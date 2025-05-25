#include "filter_index.h"
#include "logger.h"
#include <sstream>

// @brief 构造函数
FilterIndex::FilterIndex()
{
}

/**
 * @brief 添加整数字段过滤条件
 * @param fieldName 字段名
 * @param value 字段值
 * @param id 记录ID
 */
void FilterIndex::addIntFieldFilter(const std::string &fieldName,
                                    int64_t value,
                                    uint64_t id)
{
    // 创建新的bitmap对象，用于存储满足过滤条件的recordID
    roaring_bitmap_t *bitmap = roaring_bitmap_create();
    // 添加recordID
    roaring_bitmap_add(bitmap, id);
    // 将bitmap对象添加到intFieldFilter中
    intFieldFilter[fieldName][value] = bitmap;
    // 记录日志
    globalLogger->debug("Added int field filter: fieldName={}, value={}, id={}",
                        fieldName, value, id);
}

/**
 * @brief 更新整数字段过滤条件
 * @param fieldName 字段名
 * @param oldValue 旧值 (nullptr表示新增)
 * @param newValue 新值
 * @param id 记录ID
 */
void FilterIndex::updateIntFieldFilter(const std::string &fieldName,
                                       int64_t *oldValue,
                                       int64_t newValue,
                                       uint64_t id)
{
    // 记录日志 (旧值或新值)
    if (oldValue != nullptr)
    {
        globalLogger->debug("Updated int field filter: fieldName={}, oldValue={}, newValue={}, id={}",
                            fieldName, *oldValue, newValue, id);
    }
    else
    {
        globalLogger->debug("Added int field filter: fieldName={}, oldValue=nullptr, newValue={}, id={}",
                            fieldName, newValue, id);
    }

    // 查找字段对应的map
    auto it = intFieldFilter.find(fieldName);
    if (it != intFieldFilter.end())
    {
        std::map<long, roaring_bitmap_t *> &valueMap = it->second;

        // 如果有旧值，从旧值的位图中移除ID
        auto oldBitmapItr = (oldValue != nullptr) ? valueMap.find(*oldValue) : valueMap.end();
        if (oldBitmapItr != valueMap.end())
        {
            roaring_bitmap_t *oldBitmap = oldBitmapItr->second;
            roaring_bitmap_remove(oldBitmap, id);
        }

        // 将ID添加到新值的位图中
        // 如果新值对应的位图不存在，则创建新的位图
        auto newBitmapItr = valueMap.find(newValue);
        if (newBitmapItr == valueMap.end())
        {
            roaring_bitmap_t *newBitmap = roaring_bitmap_create();
            valueMap[newValue] = newBitmap;
            newBitmapItr = valueMap.find(newValue);
        }
        roaring_bitmap_t *newBitmap = newBitmapItr->second;
        roaring_bitmap_add(newBitmap, id);
    }
    else
    {
        // 字段不存在，直接添加新过滤条件
        addIntFieldFilter(fieldName, newValue, id);
    }
}

/**
 * @brief 获取满足整数字段过滤条件的recordID位图
 * @param fieldName 字段名
 * @param op 过滤操作符
 * @param value 过滤值
 * @param resultBitmap 结果位图 (输出)
 */
void FilterIndex::getIntFieldFilterBitmap(const std::string &fieldName,
                                          Operation op,
                                          int64_t value,
                                          roaring_bitmap_t *resultBitmap)
{
    // 查找字段对应的map
    auto it = intFieldFilter.find(fieldName);
    if (it != intFieldFilter.end())
    {
        std::map<long, roaring_bitmap_t *> &valueMap = it->second;

        if (op == Operation::EQUAL)
        {
            // 等于操作：获取值对应的位图并进行并集
            auto bitmapItr = valueMap.find(value);
            if (bitmapItr != valueMap.end())
            {
                globalLogger->debug("Retrieved EQUAL bitmap for filter: fieldName={}, value={}",
                                    fieldName, value);
                // 将找到的位图与结果位图进行并集操作
                roaring_bitmap_or_inplace(resultBitmap, bitmapItr->second);
            }
        }
        else if (op == Operation::NOT_EQUAL)
        {
            // 不等于操作：获取所有不等于value的位图，并进行并集运算
            for (const auto &pair : valueMap)
            {
                if (pair.first != value)
                {
                    // 将不等于value的位图与结果位图进行并集操作
                    roaring_bitmap_or_inplace(resultBitmap, pair.second);
                }
            }
            globalLogger->debug("Retrieved NOT_EQUAL bitmap for filter: fieldName={}, value={}",
                                fieldName, value);
        }
        // TODO: 实现其他操作符
    }
}

/**
 * @brief 序列化整数字段过滤器
 * @return 序列化后的字符串
 */
std::string FilterIndex::serializeIntFieldFilter()
{
    std::ostringstream oss;

    // 将intFieldFilter序列化为字符串
    for (const auto &fieldEntry : intFieldFilter)
    {
        const std::string &fieldName = fieldEntry.first;
        const std::map<int64_t, roaring_bitmap_t *> &valueMap = fieldEntry.second;

        for (const auto &valueEntry : valueMap)
        {
            int64_t value = valueEntry.first;
            const roaring_bitmap_t *bitmap = valueEntry.second;

            // 将 位图 序列化为字节数组
            uint32_t bitmapSize = roaring_bitmap_portable_size_in_bytes(bitmap);
            char *serializedBitmap = new char[bitmapSize];
            roaring_bitmap_portable_serialize(bitmap, serializedBitmap);

            // 将字段名、值和位图字节数组写入输出流
            oss << fieldName << "|" << value << "|";
            oss.write(serializedBitmap, bitmapSize);
            oss << std::endl; // 每行一个条目

            delete[] serializedBitmap; // 释放内存
        }
    }
    
    return oss.str();
}

/**
 * @brief 反序列化整数字段过滤器
 * @param serializedData 待反序列化的字符串
 */
void FilterIndex::deserializeIntFieldFilter(const std::string &serializedData)
{
    std::istringstream iss(serializedData);
    std::string line;

    // 逐行读取并反序列化每个条目
    while (std::getline(iss, line))
    {
        std::istringstream lineStream(line);

        // 从输入流中读取字段名、值和位图字节数组
        std::string fieldName;
        std::getline(lineStream, fieldName, '|');
        std::string valueStr;
        std::getline(lineStream, valueStr, '|');
        int64_t value = std::stoll(valueStr);
        // 读取序列化后的位图字节数组
        std::string serializedBitmap(std::istreambuf_iterator<char>(lineStream), {});

        // 反序列化位图
        roaring_bitmap_t *bitmap = roaring_bitmap_portable_deserialize(serializedBitmap.data());

        // 将位图添加到intFieldFilter中
        intFieldFilter[fieldName][value] = bitmap;
    }
}

/**
 * @brief 保存索引到存储
 * @param scalarStorage 标量数据存储
 * @param key 存储键
 */
void FilterIndex::saveIndex(ScalarStorage &scalarStorage, const std::string &key)
{
    // 序列化索引数据
    std::string serializedData = serializeIntFieldFilter();
    // 存储序列化后的数据
    scalarStorage.put(key, serializedData);
}

/**
 * @brief 从存储加载索引
 * @param scalarStorage 标量数据存储
 * @param key 存储键
 */
void FilterIndex::loadIndex(ScalarStorage &scalarStorage, const std::string &key)
{
    // 从存储获取数据
    std::string serializedData = scalarStorage.get(key);
    // 反序列化数据并加载到索引
    deserializeIntFieldFilter(serializedData);
}