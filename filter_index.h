#pragma once

#include "roaring/roaring.h"
#include "scalar_storage.h"
#include <set>
#include <memory>
#include <vector>
#include <string>
#include <map>

/**
 * @brief 过滤条件索引类
 *
 * 该类用于管理和查询基于字段值的过滤条件索引。
 * 使用RoaringBitmap作为底层存储结构，提供高效的位图操作。
 */
class FilterIndex
{
public:
    /**
     * @brief 过滤操作符枚举
     */
    enum class Operation
    {
        EQUAL,     ///< 等于
        NOT_EQUAL, ///< 不等于
        // GREATER_THAN,       ///< 大于
        // LESS_THAN,          ///< 小于
        // GREATER_THAN_OR_EQUAL, ///< 大于等于
        // TODO: 其他操作符
    };

    FilterIndex();

    /**
     * @brief 添加整数字段的过滤条件，并记录recordID
     * @param fieldName 字段名称
     * @param value 字段值
     * @param id 记录ID
     */
    void addIntFieldFilter(const std::string &fieldName,
                           int64_t value,
                           uint64_t id);

    /**
     * @brief 更新整数字段的过滤条件，并更新recordID
     * @param fieldName 字段名称
     * @param oldValue 旧的字段值（如果为nullptr则表示新增）
     * @param newValue 新的字段值
     * @param id 记录ID
     */
    void updateIntFieldFilter(const std::string &fieldName,
                              int64_t *oldValue,
                              int64_t newValue,
                              uint64_t id);

    /**
     * @brief 获取满足过滤条件的recordID位图
     * @param fieldName 字段名称
     * @param op 过滤操作符
     * @param value 过滤值
     * @param resultBitmap 结果位图（输出参数）
     */
    void getIntFieldFilterBitmap(const std::string &fieldName,
                                 Operation op,
                                 int64_t value,
                                 roaring_bitmap_t *resultBitmap);

    /**
     * @brief 序列化整数型字段过滤器
     * @return 返回序列化后的字符串
     *
     * 将当前整数型字段过滤器的状态序列化为字符串，以便存储或传输。
     */
    std::string serializeIntFieldFilter();

    /**
     * @brief 反序列化整数型字段过滤器
     * @param serializedData 待反序列化的字符串数据
     *
     * 从序列化的字符串数据中恢复整数型字段过滤器的状态。
     */
    void deserializeIntFieldFilter(const std::string &serializedData);

    /**
     * @brief 保存索引
     * @param scalarStorage 标量数据存储对象
     * @param key 保存索引时使用的键
     *
     * 将当前的过滤器索引保存到指定的ScalarStorage中，使用给定的键标识。
     */
    void saveIndex(ScalarStorage &scalarStorage,
                   const std::string &key);

    /**
     * @brief 加载索引
     * @param scalarStorage 标量数据存储对象
     * @param key 加载索引时使用的键
     *
     * 从指定的ScalarStorage中加载之前保存的过滤器索引，使用给定的键标识。
     */
    void loadIndex(ScalarStorage &scalarStorage,
                   const std::string &key);
    // TODO: 其他类型字段过滤器

private:
    /**
     * @brief 整数字段过滤索引
     *
     * 第一层map的key是字段名
     * 第二层map的key是字段值
     * 最内层是存储记录ID的RoaringBitmap
     */
    std::map<std::string, std::map<int64_t, roaring_bitmap_t *>> intFieldFilter;
    // TODO: 其他类型字段过滤索引
};
