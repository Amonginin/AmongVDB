/**
 * @file test_persistence_unit.cpp
 * @brief Persistence类单元测试
 * @details 测试Persistence类的基础功能，包括ID管理、文件初始化等
 */

#include "../common/test_utils.h"
#include "../../persistence.h"
#include "../../logger.h"

using namespace test_utils;

/**
 * @brief 测试Persistence类构造函数和基础ID功能
 */
void test_persistence_constructor_and_id() {
    TEST_CASE_BEGIN("Persistence构造函数和ID管理");
    
    Persistence persistence;
    
    // 测试初始ID应该为1
    TEST_ASSERT(persistence.getID() == 1, "初始ID应该为1");
    
    // 测试ID递增功能
    uint64_t newId = persistence.increaseID();
    TEST_ASSERT(newId == 2, "第一次递增后ID应该为2");
    TEST_ASSERT(persistence.getID() == 2, "当前ID应该为2");
    
    // 再次测试ID递增
    newId = persistence.increaseID();
    TEST_ASSERT(newId == 3, "第二次递增后ID应该为3");
    TEST_ASSERT(persistence.getID() == 3, "当前ID应该为3");
    
    TEST_CASE_END("Persistence构造函数和ID管理");
}

/**
 * @brief 测试WAL文件初始化功能
 */
void test_wal_file_initialization() {
    TEST_CASE_BEGIN("WAL文件初始化");
    
    TestEnvironment::setup_test_environment();
    
    Persistence persistence;
    std::string validPath = TestEnvironment::create_temp_file("test_wal");
    
    // 测试正常文件路径初始化
    try {
        persistence.init(validPath);
        TEST_ASSERT(true, "正常路径初始化成功");
    } catch (const std::exception& e) {
        TEST_ASSERT(false, "正常路径不应该初始化失败: " + std::string(e.what()));
    }
    
    // 测试无效路径初始化
    Persistence persistence2;
    try {
        persistence2.init("/invalid/nonexistent/path/wal.log");
        TEST_ASSERT(false, "无效路径应该抛出异常");
    } catch (const std::runtime_error& e) {
        TEST_ASSERT(true, "无效路径正确抛出runtime_error异常");
    } catch (...) {
        TEST_ASSERT(false, "应该抛出runtime_error类型的异常");
    }
    
    TestEnvironment::cleanup_test_environment();
    
    TEST_CASE_END("WAL文件初始化");
}

/**
 * @brief 测试WAL日志写入功能
 */
void test_wal_log_writing() {
    TEST_CASE_BEGIN("WAL日志写入");
    
    TestEnvironment::setup_test_environment();
    
    Persistence persistence;
    std::string walPath = TestEnvironment::create_temp_file("test_write_wal");
    persistence.init(walPath);
    
    // 创建测试数据
    auto testData = TestDataGenerator::create_upsert_data(123, 3);
    
    // 测试写入upsert操作
    try {
        persistence.writeWALLog("upsert", testData, "v1.0");
        TEST_ASSERT(true, "upsert日志写入成功");
    } catch (const std::exception& e) {
        TEST_ASSERT(false, "upsert日志写入失败: " + std::string(e.what()));
    }
    
    // 测试写入delete操作
    auto deleteData = TestDataGenerator::create_delete_data(456, "FLAT");
    try {
        persistence.writeWALLog("delete", deleteData, "v1.0");
        TEST_ASSERT(true, "delete日志写入成功");
    } catch (const std::exception& e) {
        TEST_ASSERT(false, "delete日志写入失败: " + std::string(e.what()));
    }
    
    // 验证文件内容
    TEST_ASSERT(WALLogValidator::validate_wal_file(walPath), "WAL文件格式验证通过");
    
    int logCount = WALLogValidator::count_log_entries(walPath);
    TEST_ASSERT(logCount == 2, "WAL文件应该包含2条日志记录");
    
    auto operations = WALLogValidator::parse_log_operations(walPath);
    TEST_ASSERT(operations.size() == 2, "解析出的操作数量应该为2");
    TEST_ASSERT(operations[0] == "upsert", "第一个操作应该是upsert");
    TEST_ASSERT(operations[1] == "delete", "第二个操作应该是delete");
    
    TestEnvironment::cleanup_test_environment();
    
    TEST_CASE_END("WAL日志写入");
}

/**
 * @brief 测试WAL日志读取功能
 */
void test_wal_log_reading() {
    TEST_CASE_BEGIN("WAL日志读取");
    
    TestEnvironment::setup_test_environment();
    
    // 先写入测试数据
    Persistence writePersistence;
    std::string walPath = TestEnvironment::create_temp_file("test_read_wal");
    writePersistence.init(walPath);
    
    auto testData1 = TestDataGenerator::create_upsert_data(100, 3);
    auto testData2 = TestDataGenerator::create_delete_data(200, "HNSW");
    
    writePersistence.writeWALLog("upsert", testData1, "v1.0");
    writePersistence.writeWALLog("delete", testData2, "v1.0");
    
    // 测试读取功能
    Persistence readPersistence;
    readPersistence.init(walPath);
    
    std::string operationType1;
    rapidjson::Document readData1;
    
    // 读取第一条日志
    readPersistence.readNextWALLog(&operationType1, &readData1);
    TEST_ASSERT(operationType1 == "upsert", "第一条日志操作类型应该是upsert");
    TEST_ASSERT(readData1.HasMember("id"), "第一条日志应该包含id字段");
    TEST_ASSERT(readData1["id"].GetUint64() == 100, "第一条日志ID应该为100");
    
    std::string operationType2;
    rapidjson::Document readData2;
    
    // 读取第二条日志
    readPersistence.readNextWALLog(&operationType2, &readData2);
    TEST_ASSERT(operationType2 == "delete", "第二条日志操作类型应该是delete");
    TEST_ASSERT(readData2.HasMember("id"), "第二条日志应该包含id字段");
    TEST_ASSERT(readData2["id"].GetUint64() == 200, "第二条日志ID应该为200");
    
    // 测试读取完毕后的行为
    std::string operationType3;
    rapidjson::Document readData3;
    readPersistence.readNextWALLog(&operationType3, &readData3);
    TEST_ASSERT(operationType3.empty(), "读取完毕后操作类型应该为空");
    
    TestEnvironment::cleanup_test_environment();
    
    TEST_CASE_END("WAL日志读取");
}

/**
 * @brief 测试ID同步功能
 */
void test_id_synchronization() {
    TEST_CASE_BEGIN("ID同步功能");
    
    TestEnvironment::setup_test_environment();
    
    // 创建一个persistence实例并写入一些日志
    Persistence writePersistence;
    std::string walPath = TestEnvironment::create_temp_file("test_id_sync");
    writePersistence.init(walPath);
    
    // 写入多条日志，ID会递增
    for (int i = 1; i <= 5; i++) {
        auto testData = TestDataGenerator::create_upsert_data(i * 100, 3);
        writePersistence.writeWALLog("upsert", testData, "v1.0");
    }
    
    uint64_t finalWriteId = writePersistence.getID();
    TEST_ASSERT(finalWriteId == 6, "写入5条日志后ID应该为6");  // 初始为1，写入5条后为6
    
    // 创建新的persistence实例并读取日志
    Persistence readPersistence;
    readPersistence.init(walPath);
    
    // 读取所有日志
    std::string operationType;
    rapidjson::Document readData;
    int readCount = 0;
    
    while (true) {
        std::string prevOpType = operationType;
        readPersistence.readNextWALLog(&operationType, &readData);
        if (operationType.empty()) {
            break;
        }
        readCount++;
    }
    
    TEST_ASSERT(readCount == 5, "应该读取到5条日志");
    
    // 验证ID同步
    uint64_t syncedId = readPersistence.getID();
    TEST_ASSERT(syncedId == finalWriteId, "读取后的ID应该与写入后的ID同步");
    
    TestEnvironment::cleanup_test_environment();
    
    TEST_CASE_END("ID同步功能");
}

/**
 * @brief 主函数 - 运行所有单元测试
 */
int main() {
    // 初始化日志系统
    initGlobalLogger();
    setLogLevel(spdlog::level::info);
    
    TestSuite suite("Persistence类单元测试");
    
    // 运行所有测试
    suite.run_test("构造函数和ID管理", test_persistence_constructor_and_id);
    suite.run_test("WAL文件初始化", test_wal_file_initialization);
    suite.run_test("WAL日志写入", test_wal_log_writing);
    suite.run_test("WAL日志读取", test_wal_log_reading);
    suite.run_test("ID同步功能", test_id_synchronization);
    
    return 0;
} 