/**
 * @file test_utils.cpp
 * @brief 测试工具类实现文件
 */

#include "test_utils.h"
#include <random>
#include <sstream>
#include <functional>
#include <sys/stat.h>
#include <unistd.h>

// 文件系统兼容性处理
#if __cplusplus >= 201703L && __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    // 如果都不支持，使用传统方法
    #include <cstdlib>
    #include <sys/types.h>
    #include <dirent.h>
#endif

namespace test_utils {

// ================== TestSuite 实现 ==================

TestSuite::TestSuite(const std::string& name) 
    : suite_name_(name), total_tests_(0), passed_tests_(0), failed_tests_(0) {
    start_time_ = std::chrono::high_resolution_clock::now();
    std::cout << "\n🚀 开始测试套件: " << suite_name_ << std::endl;
    std::cout << "=====================================\n" << std::endl;
}

TestSuite::~TestSuite() {
    print_summary();
}

void TestSuite::run_test(const std::string& test_name, std::function<void()> test_func) {
    total_tests_++;
    
    try {
        TEST_CASE_BEGIN(test_name);
        test_func();
        TEST_CASE_END(test_name);
        passed_tests_++;
    } catch (const std::exception& e) {
        failed_tests_++;
        std::cerr << "❌ 测试失败: " << test_name << std::endl;
        std::cerr << "   异常信息: " << e.what() << std::endl;
    } catch (...) {
        failed_tests_++;
        std::cerr << "❌ 测试失败: " << test_name << std::endl;
        std::cerr << "   未知异常" << std::endl;
    }
}

void TestSuite::print_summary() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    
    std::cout << "\n=====================================\n";
    std::cout << "📊 测试套件总结: " << suite_name_ << std::endl;
    std::cout << "总测试数: " << total_tests_ << std::endl;
    std::cout << "通过测试: " << passed_tests_ << std::endl;
    std::cout << "失败测试: " << failed_tests_ << std::endl;
    std::cout << "成功率: " << (total_tests_ > 0 ? (passed_tests_ * 100.0 / total_tests_) : 0) << "%" << std::endl;
    std::cout << "总耗时: " << duration.count() << "ms" << std::endl;
    
    if (failed_tests_ == 0) {
        std::cout << "🎉 所有测试通过！" << std::endl;
    } else {
        std::cout << "⚠️  有测试失败，请检查！" << std::endl;
    }
    std::cout << "=====================================\n" << std::endl;
}

// ================== TestEnvironment 实现 ==================

std::string TestEnvironment::get_test_temp_dir() {
    return "/tmp/vdb_test_v0.1.2";
}

void TestEnvironment::setup_test_environment() {
    std::string temp_dir = get_test_temp_dir();
    
#if defined(fs)
    fs::create_directories(temp_dir);
#else
    // 使用传统方法创建目录
    std::string cmd = "mkdir -p " + temp_dir;
    system(cmd.c_str());
#endif
    
    std::cout << "📁 测试环境已创建: " << temp_dir << std::endl;
}

void TestEnvironment::cleanup_test_environment() {
    std::string temp_dir = get_test_temp_dir();
    
#if defined(fs)
    if (fs::exists(temp_dir)) {
        fs::remove_all(temp_dir);
    }
#else
    // 使用传统方法删除目录
    std::string cmd = "rm -rf " + temp_dir;
    system(cmd.c_str());
#endif
    
    std::cout << "🗑️  测试环境已清理: " << temp_dir << std::endl;
}

std::string TestEnvironment::create_temp_file(const std::string& prefix) {
    std::string temp_dir = get_test_temp_dir();
    static int counter = 0;
    std::string filename = prefix + "_" + std::to_string(++counter) + ".tmp";
    return temp_dir + "/" + filename;
}

void TestEnvironment::remove_temp_file(const std::string& filepath) {
#if defined(fs)
    if (fs::exists(filepath)) {
        fs::remove(filepath);
    }
#else
    // 使用传统方法删除文件
    unlink(filepath.c_str());
#endif
}

// ================== TestDataGenerator 实现 ==================

rapidjson::Document TestDataGenerator::create_test_vector_data(uint64_t id,
                                                              const std::vector<float>& vectors,
                                                              const std::string& index_type,
                                                              int category) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    // 添加ID
    doc.AddMember("id", id, allocator);
    
    // 添加向量数据
    rapidjson::Value vectorArray(rapidjson::kArrayType);
    for (float val : vectors) {
        vectorArray.PushBack(val, allocator);
    }
    doc.AddMember("vectors", vectorArray, allocator);
    
    // 添加索引类型
    rapidjson::Value indexTypeVal;
    indexTypeVal.SetString(index_type.c_str(), index_type.length(), allocator);
    doc.AddMember("indexType", indexTypeVal, allocator);
    
    // 如果指定了分类，添加分类字段
    if (category >= 0) {
        doc.AddMember("category", category, allocator);
    }
    
    return doc;
}

std::vector<float> TestDataGenerator::generate_random_vector(int dimensions) {
    std::vector<float> vector(dimensions);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    for (int i = 0; i < dimensions; i++) {
        vector[i] = dis(gen);
    }
    
    return vector;
}

rapidjson::Document TestDataGenerator::create_upsert_data(uint64_t id, int dimensions) {
    auto vectors = generate_random_vector(dimensions);
    return create_test_vector_data(id, vectors, "FLAT", id % 5);
}

rapidjson::Document TestDataGenerator::create_delete_data(uint64_t id, const std::string& index_type) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    doc.AddMember("id", id, allocator);
    
    rapidjson::Value indexTypeVal;
    indexTypeVal.SetString(index_type.c_str(), index_type.length(), allocator);
    doc.AddMember("indexType", indexTypeVal, allocator);
    
    return doc;
}

rapidjson::Document TestDataGenerator::create_query_data(uint64_t id) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    doc.AddMember("id", id, allocator);
    
    return doc;
}

// ================== WALLogValidator 实现 ==================

bool WALLogValidator::validate_wal_format(const std::string& log_line) {
    // WAL格式: logID|version|operationType|jsonData
    std::istringstream iss(log_line);
    std::string field;
    int field_count = 0;
    
    while (std::getline(iss, field, '|')) {
        field_count++;
    }
    
    return field_count >= 4;  // 至少4个字段
}

bool WALLogValidator::validate_wal_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && !validate_wal_format(line)) {
            return false;
        }
    }
    
    return true;
}

int WALLogValidator::count_log_entries(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return -1;
    }
    
    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            count++;
        }
    }
    
    return count;
}

std::vector<std::string> WALLogValidator::parse_log_operations(const std::string& filepath) {
    std::vector<std::string> operations;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return operations;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::istringstream iss(line);
            std::string logId, version, operation;
            
            std::getline(iss, logId, '|');
            std::getline(iss, version, '|');
            std::getline(iss, operation, '|');
            
            operations.push_back(operation);
        }
    }
    
    return operations;
}

// ================== PerformanceTimer 实现 ==================

PerformanceTimer::PerformanceTimer() : is_running_(false) {}

void PerformanceTimer::start() {
    start_time_ = std::chrono::high_resolution_clock::now();
    is_running_ = true;
}

void PerformanceTimer::stop() {
    end_time_ = std::chrono::high_resolution_clock::now();
    is_running_ = false;
}

double PerformanceTimer::get_elapsed_ms() const {
    auto end = is_running_ ? std::chrono::high_resolution_clock::now() : end_time_;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time_);
    return duration.count();
}

double PerformanceTimer::get_elapsed_seconds() const {
    return get_elapsed_ms() / 1000.0;
}

// ================== IndexFactoryHelper 实现 ==================

void IndexFactoryHelper::init_all_indexes(int vector_dim, int max_elements) {
    // 初始化FLAT索引
    getGlobalIndexFactory()->init(IndexFactory::IndexType::FLAT, vector_dim, max_elements);
    
    // 初始化HNSW索引
    getGlobalIndexFactory()->init(IndexFactory::IndexType::HNSW, vector_dim, max_elements);
    
    // 初始化过滤索引
    getGlobalIndexFactory()->init(IndexFactory::IndexType::FILTER, 1, max_elements);
    
    std::cout << "🔧 索引工厂已初始化 (维度: " << vector_dim << ", 最大元素: " << max_elements << ")" << std::endl;
}

void IndexFactoryHelper::cleanup_indexes() {
    // 注意：实际的清理可能需要根据IndexFactory的具体实现来完成
    std::cout << "🧹 索引工厂已清理" << std::endl;
}

} // namespace test_utils 