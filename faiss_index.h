#pragma once

#include <vector>
#include "faiss/Index.h"
#include "faiss/impl/IDSelector.h"
#include "faiss/utils/utils.h"
#include "roaring/roaring.h"

/**
 * @brief 基于 Roaring Bitmap 的 FAISS ID 选择器
 * 该结构体继承自 faiss::IDSelector，用于通过 Roaring Bitmap 判断某个ID是否在集合中。
 */
struct RoaringBitmapIDSelector : faiss::IDSelector
{
    /**
     * @brief 构造函数
     * @param bitmap 指向 roaring_bitmap_t 的指针，表示ID集合
     */
    RoaringBitmapIDSelector(const roaring_bitmap_t *bitmap);
    /**
     * @brief 析构函数
     */
    ~RoaringBitmapIDSelector();
    /**
     * @brief 判断给定ID是否在 Roaring Bitmap 中
     * @param id 待判断的ID
     * @return 如果ID存在于 bitmap 中返回 true，否则返回 false
     */
    bool is_member(int64_t id) const final;
    
    /**
     * @brief 指向 Roaring Bitmap 的指针，存储ID集合
     */
    const roaring_bitmap_t *bitmap;
};

/**
 * @brief FAISS 索引管理类
 *
 * 该类用于管理 FAISS 索引对象，支持向量的插入、查询和删除操作。
 */
class FaissIndex
{
public:
    /**
     * @brief 构造函数，接收一个指向FAISS索引对象的指针
     * @param index 指向 FAISS 索引对象的指针
     */
    FaissIndex(faiss::Index *index);

    /**
     * @brief 向索引中插入单个向量及其标签
     * @param data 向量数据（float类型数组）
     * @param label 向量对应的标签（ID）
     */
    void insertVectors(const std::vector<float> &data, uint64_t label);

    /**
     * @brief 查询与输入向量最近邻的k个向量
     * @param query 查询向量数据（可包含多个查询向量）
     * @param k 每个查询返回的最近邻数量
     * @param bitmap 可选参数，指定ID过滤的 Roaring Bitmap
     * @return 返回一个 pair，第一个为匹配向量的ID数组，第二个为对应的距离数组
     */
    std::pair<std::vector<long>, std::vector<float>> searchVectors(
        const std::vector<float> &query, int k, const roaring_bitmap_t *bitmap = nullptr);

    /**
     * @brief 从索引中删除指定ID的向量
     * @param ids 要删除的向量ID列表
     */
    void removeVectors(const std::vector<long> &ids);

    /**
     * @brief 保存索引到文件
     * @param filePath 保存路径
     */
    void saveIndex(const std::string &filePath);

    /**
     * @brief 从文件加载索引
     * @param filePath 加载路径
     */
    void loadIndex(const std::string &filePath);

private:
    /**
     * @brief 指向FAISS索引对象的指针
     */
    faiss::Index *index;
};
