# Makefile for Persistence Testing v0.1.2
# 数据日志持久化功能测试构建文件

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -I../../include -I../..
LIBS = -lrocksdb -lfaiss -lpthread -lspdlog -lstdc++fs

# 目录设置
SRC_DIR = ../..
COMMON_DIR = common
UNIT_DIR = unit
INTEGRATION_DIR = integration
PERFORMANCE_DIR = performance
ERRORS_DIR = errors

# 源文件
COMMON_SRCS = $(COMMON_DIR)/test_utils.cpp
MAIN_SRCS = $(SRC_DIR)/persistence.cpp \
           $(SRC_DIR)/vector_database.cpp \
           $(SRC_DIR)/scalar_storage.cpp \
           $(SRC_DIR)/index_factory.cpp \
           $(SRC_DIR)/faiss_index.cpp \
           $(SRC_DIR)/hnswlib_index.cpp \
           $(SRC_DIR)/filter_index.cpp \
           $(SRC_DIR)/logger.cpp

# 目标文件
UNIT_TARGET = unit_tests
INTEGRATION_TARGET = integration_tests
PERFORMANCE_TARGET = performance_tests
ERRORS_TARGET = error_tests

# 默认目标
.PHONY: all clean unit integration performance errors help setup

all: setup unit integration performance errors

# 设置测试环境
setup:
	@echo "🔧 设置测试环境..."
	@mkdir -p /tmp/vdb_test_v0.1.2
	@echo "✅ 测试环境设置完成"

# 单元测试
unit: $(UNIT_TARGET)

$(UNIT_TARGET): $(UNIT_DIR)/test_persistence_unit.cpp $(COMMON_SRCS) $(MAIN_SRCS)
	@echo "🔨 编译单元测试..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ 单元测试编译完成: $(UNIT_TARGET)"

# 集成测试
integration: $(INTEGRATION_TARGET)

$(INTEGRATION_TARGET): $(INTEGRATION_DIR)/test_persistence_integration.cpp $(COMMON_SRCS) $(MAIN_SRCS)
	@echo "🔨 编译集成测试..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ 集成测试编译完成: $(INTEGRATION_TARGET)"

# 性能测试
performance: $(PERFORMANCE_TARGET)

$(PERFORMANCE_TARGET): $(PERFORMANCE_DIR)/test_persistence_performance.cpp $(COMMON_SRCS) $(MAIN_SRCS)
	@echo "🔨 编译性能测试..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ 性能测试编译完成: $(PERFORMANCE_TARGET)"

# 错误测试
errors: $(ERRORS_TARGET)

$(ERRORS_TARGET): $(ERRORS_DIR)/test_persistence_errors.cpp $(COMMON_SRCS) $(MAIN_SRCS)
	@echo "🔨 编译错误测试..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ 错误测试编译完成: $(ERRORS_TARGET)"

# 运行所有测试
run-all: all
	@echo "🚀 运行所有测试..."
	@echo "\n📋 1. 单元测试"
	@./$(UNIT_TARGET) || echo "❌ 单元测试失败"
	@echo "\n📋 2. 集成测试"
	@./$(INTEGRATION_TARGET) || echo "❌ 集成测试失败"
	@echo "\n📋 3. 性能测试"
	@./$(PERFORMANCE_TARGET) || echo "❌ 性能测试失败"
	@echo "\n📋 4. 错误测试"
	@./$(ERRORS_TARGET) || echo "❌ 错误测试失败"
	@echo "\n🎉 所有测试运行完成！"

# 运行单独测试
run-unit: $(UNIT_TARGET)
	@echo "🧪 运行单元测试..."
	@./$(UNIT_TARGET)

run-integration: $(INTEGRATION_TARGET)
	@echo "🧪 运行集成测试..."
	@./$(INTEGRATION_TARGET)

run-performance: $(PERFORMANCE_TARGET)
	@echo "🧪 运行性能测试..."
	@./$(PERFORMANCE_TARGET)

run-errors: $(ERRORS_TARGET)
	@echo "🧪 运行错误测试..."
	@./$(ERRORS_TARGET)

# 运行端到端测试
run-e2e:
	@echo "🧪 运行端到端测试..."
	@chmod +x e2e/test_persistence_e2e.sh
	@cd e2e && ./test_persistence_e2e.sh

# 清理
clean:
	@echo "🧹 清理构建文件..."
	@rm -f $(UNIT_TARGET) $(INTEGRATION_TARGET) $(PERFORMANCE_TARGET) $(ERRORS_TARGET)
	@rm -rf /tmp/vdb_test_v0.1.2
	@echo "✅ 清理完成"

# 安装依赖
install-deps:
	@echo "📦 检查依赖..."
	@echo "请确保已安装以下依赖:"
	@echo "  - RocksDB"
	@echo "  - Faiss"
	@echo "  - spdlog"
	@echo "  - rapidjson"
	@echo ""
	@echo "Ubuntu/Debian 安装命令:"
	@echo "  sudo apt-get install librocksdb-dev libfaiss-dev libspdlog-dev rapidjson-dev"
	@echo ""
	@echo "CentOS/RHEL 安装命令:"
	@echo "  sudo yum install rocksdb-devel faiss-devel spdlog-devel rapidjson-devel"

# 代码格式化
format:
	@echo "🎨 格式化代码..."
	@find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "✅ 代码格式化完成"

# 静态分析
analyze:
	@echo "🔍 运行静态分析..."
	@cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem \
		$(UNIT_DIR)/ $(INTEGRATION_DIR)/ $(PERFORMANCE_DIR)/ $(ERRORS_DIR)/ $(COMMON_DIR)/
	@echo "✅ 静态分析完成"

# 内存检查
memcheck: $(UNIT_TARGET)
	@echo "🔍 运行内存检查..."
	@valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
		--track-origins=yes ./$(UNIT_TARGET)

# 性能分析
profile: $(PERFORMANCE_TARGET)
	@echo "📊 运行性能分析..."
	@perf record -g ./$(PERFORMANCE_TARGET)
	@perf report

# 覆盖率测试
coverage:
	@echo "📈 生成覆盖率报告..."
	@$(MAKE) CXXFLAGS="$(CXXFLAGS) --coverage" all
	@./$(UNIT_TARGET)
	@./$(INTEGRATION_TARGET)
	@gcov *.gcno
	@lcov --capture --directory . --output-file coverage.info
	@genhtml coverage.info --output-directory coverage_html
	@echo "✅ 覆盖率报告生成完成，查看: coverage_html/index.html"

# 调试构建
debug:
	@echo "🐛 构建调试版本..."
	@$(MAKE) CXXFLAGS="$(CXXFLAGS) -DDEBUG -O0" all

# 发布构建
release:
	@echo "🚀 构建发布版本..."
	@$(MAKE) CXXFLAGS="$(CXXFLAGS) -DNDEBUG -O3" all

# 帮助信息
help:
	@echo "🆘 Persistence Testing v0.1.2 构建帮助"
	@echo "==============================================="
	@echo ""
	@echo "主要目标:"
	@echo "  all           - 编译所有测试"
	@echo "  unit          - 编译单元测试"
	@echo "  integration   - 编译集成测试"
	@echo "  performance   - 编译性能测试"
	@echo "  errors        - 编译错误测试"
	@echo ""
	@echo "运行测试:"
	@echo "  run-all       - 运行所有测试"
	@echo "  run-unit      - 运行单元测试"
	@echo "  run-integration - 运行集成测试"
	@echo "  run-performance - 运行性能测试"
	@echo "  run-errors    - 运行错误测试"
	@echo "  run-e2e       - 运行端到端测试"
	@echo ""
	@echo "工具命令:"
	@echo "  clean         - 清理构建文件"
	@echo "  setup         - 设置测试环境"
	@echo "  install-deps  - 显示依赖安装信息"
	@echo "  format        - 格式化代码"
	@echo "  analyze       - 静态代码分析"
	@echo "  memcheck      - 内存泄漏检查"
	@echo "  coverage      - 生成覆盖率报告"
	@echo "  debug         - 构建调试版本"
	@echo "  release       - 构建发布版本"
	@echo ""
	@echo "示例:"
	@echo "  make all && make run-all    # 编译并运行所有测试"
	@echo "  make unit && make run-unit  # 编译并运行单元测试"
	@echo "  make run-e2e               # 运行端到端测试" 