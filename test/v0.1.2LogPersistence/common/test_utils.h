/**
 * @file test_utils.h
 * @brief 测试工具类头文件
 * @details 提供数据日志持久化功能测试的公共工具函数和宏定义
 */

#pragma once

#include <string>
#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <fstream>
#include <functional>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "../../persistence.h"
#include "../../vector_database.h"
#include "../../index_factory.h"
#include "../../logger.h"

namespace test_utils {

/**
 * @brief 测试断言宏，增强错误信息
 */
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "❌ 测试失败: " << message << std::endl; \
            std::cerr << "   文件: " << __FILE__ << ", 行号: " << __LINE__ << std::endl; \
            assert(false); \
        } else { \
            std::cout << "✅ 测试通过: " << message << std::endl; \
        } \
    } while(0)

/**
 * @brief 测试用例开始宏
 */
#define TEST_CASE_BEGIN(name) \
    std::cout << "\n🧪 开始测试: " << name << std::endl; \
    auto test_start_time = std::chrono::high_resolution_clock::now();

/**
 * @brief 测试用例结束宏
 */
#define TEST_CASE_END(name) \
    auto test_end_time = std::chrono::high_resolution_clock::now(); \
    auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(test_end_time - test_start_time); \
    std::cout << "✅ 测试完成: " << name << " (耗时: " << test_duration.count() << "ms)" << std::endl;

/**
 * @brief 测试套件类
 */
class TestSuite {
public:
    TestSuite(const std::string& name);
    ~TestSuite();
    
    void run_test(const std::string& test_name, std::function<void()> test_func);
    void print_summary();
    
private:
    std::string suite_name_;
    int total_tests_;
    int passed_tests_;
    int failed_tests_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief 测试环境管理类
 */
class TestEnvironment {
public:
    static std::string get_test_temp_dir();
    static void setup_test_environment();
    static void cleanup_test_environment();
    static std::string create_temp_file(const std::string& prefix = "test");
    static void remove_temp_file(const std::string& filepath);
};

/**
 * @brief 测试数据生成器
 */
class TestDataGenerator {
public:
    static rapidjson::Document create_test_vector_data(uint64_t id, 
                                                      const std::vector<float>& vectors,
                                                      const std::string& index_type = "FLAT",
                                                      int category = -1);
    
    static std::vector<float> generate_random_vector(int dimensions);
    
    static rapidjson::Document create_upsert_data(uint64_t id, int dimensions = 3);
    static rapidjson::Document create_delete_data(uint64_t id, const std::string& index_type = "FLAT");
    static rapidjson::Document create_query_data(uint64_t id);
};

/**
 * @brief WAL日志验证器
 */
class WALLogValidator {
public:
    static bool validate_wal_format(const std::string& log_line);
    static bool validate_wal_file(const std::string& filepath);
    static int count_log_entries(const std::string& filepath);
    static std::vector<std::string> parse_log_operations(const std::string& filepath);
};

/**
 * @brief 性能测试工具
 */
class PerformanceTimer {
public:
    PerformanceTimer();
    void start();
    void stop();
    double get_elapsed_ms() const;
    double get_elapsed_seconds() const;
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    bool is_running_;
};

/**
 * @brief 索引工厂初始化助手
 */
class IndexFactoryHelper {
public:
    static void init_all_indexes(int vector_dim = 3, int max_elements = 1000);
    static void cleanup_indexes();
};

} // namespace test_utils 