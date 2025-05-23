#include "filter_index.h"
#include "logger.h"

FilterIndex::FilterIndex()
{
}

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

void FilterIndex::updateIntFieldFilter(const std::string &fieldName,
                                       int64_t *oldValue,
                                       int64_t newValue,
                                       uint64_t id)
{
    // 记录日志
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

    // 查找字段对应的值映射
    auto it = intFieldFilter.find(fieldName);
    if (it != intFieldFilter.end())
    {
        std::map<long, roaring_bitmap_t *> &valueMap = it->second;

        // 如果存在旧值，从旧值的位图中删除ID
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
        // 如果字段不存在，直接添加新的过滤条件
        addIntFieldFilter(fieldName, newValue, id);
    }
}

void FilterIndex::getIntFieldFilterBitmap(const std::string &fieldName,
                                    Operation op,
                                    int64_t value,
                                    roaring_bitmap_t *resultBitmap)
{
    // 查找字段对应的值映射
    auto it = intFieldFilter.find(fieldName);
    if (it != intFieldFilter.end())
    {
        std::map<long, roaring_bitmap_t *> &valueMap = it->second;
        
        if (op == Operation::EQUAL)
        {
            // 等于操作：直接获取值对应的位图
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
        // TODO: 实现其他操作符（大于、小于等）
    }
}
