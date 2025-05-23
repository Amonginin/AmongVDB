# 数据日志持久化功能测试 (v0.1.2)

## 功能概述

本测试套件验证向量数据库的数据日志持久化功能，包括：

1. **WAL日志系统**：Write-Ahead Logging机制
2. **数据恢复功能**：系统重启后的数据一致性恢复
3. **持久化管理**：Persistence类的完整功能测试

## 测试内容

### 单元测试
- `test_persistence_unit.cpp` - Persistence类基础功能测试
- `test_wal_operations.cpp` - WAL日志读写操作测试

### 集成测试
- `test_persistence_integration.cpp` - 完整数据流和恢复功能测试

### 端到端测试
- `test_persistence_e2e.sh` - HTTP接口持久化测试脚本

### 性能测试
- `test_persistence_performance.cpp` - 大数据量和性能基准测试

### 异常测试
- `test_persistence_errors.cpp` - 错误处理和异常情况测试

## 目录结构

```
test/v0.1.2/
├── README.md                      # 测试说明文档
├── unit/                          # 单元测试
│   ├── test_persistence_unit.cpp
│   └── test_wal_operations.cpp
├── integration/                   # 集成测试
│   └── test_persistence_integration.cpp
├── e2e/                          # 端到端测试
│   └── test_persistence_e2e.sh
├── performance/                   # 性能测试
│   └── test_persistence_performance.cpp
├── errors/                       # 异常测试
│   └── test_persistence_errors.cpp
├── common/                       # 公共测试工具
│   ├── test_utils.h
│   └── test_utils.cpp
├── CMakeLists.txt                # CMake构建文件
├── Makefile                      # Make构建文件
└── run_all_tests.sh             # 执行所有测试的脚本
```

## 快速开始

1. 编译测试程序：
```bash
cd test/v0.1.2
make all
```

2. 运行所有测试：
```bash
./run_all_tests.sh
```

3. 运行特定测试：
```bash
./unit_tests          # 单元测试
./integration_tests   # 集成测试
./performance_tests   # 性能测试
```

## 验收标准

- 所有单元测试通过率：100%
- 数据恢复一致性：无数据丢失
- 性能基准：10000条日志 < 5秒
- 错误处理：优雅处理异常，无崩溃

## 测试数据

测试过程中产生的临时文件将存储在 `/tmp/vdb_test_v0.1.2/` 目录下，测试完成后会自动清理。 