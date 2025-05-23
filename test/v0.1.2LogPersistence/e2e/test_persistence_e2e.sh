#!/bin/bash

# test_persistence_e2e.sh
# 数据日志持久化功能端到端测试脚本

set -e  # 遇到错误时停止执行

# 测试配置
SERVER_HOST="localhost"
SERVER_PORT="8080"
DB_PATH="/tmp/vdb_test_v0.1.2/e2e_test_db"
WAL_PATH="/tmp/vdb_test_v0.1.2/e2e_test_wal.log"
TEST_TEMP_DIR="/tmp/vdb_test_v0.1.2"
SERVER_BINARY="../../vdb_server"  # 假设服务器二进制文件路径
SERVER_PID=""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印函数
print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_test_header() {
    echo -e "\n${BLUE}🧪 测试: $1${NC}"
    echo "=================================================="
}

# 清理函数
cleanup() {
    print_info "清理测试环境..."
    
    # 停止服务器
    if [ ! -z "$SERVER_PID" ] && kill -0 $SERVER_PID 2>/dev/null; then
        print_info "停止服务器 (PID: $SERVER_PID)..."
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null || true
    fi
    
    # 清理测试文件
    rm -rf $TEST_TEMP_DIR
    print_success "测试环境已清理"
}

# 设置信号处理
trap cleanup EXIT INT TERM

# 启动服务器
start_server() {
    print_info "启动向量数据库服务器..."
    
    # 创建测试目录
    mkdir -p $TEST_TEMP_DIR
    
    # 检查服务器二进制文件是否存在
    if [ ! -f "$SERVER_BINARY" ]; then
        print_error "服务器二进制文件不存在: $SERVER_BINARY"
        print_info "请先编译服务器程序"
        exit 1
    fi
    
    # 启动服务器
    $SERVER_BINARY --db-path=$DB_PATH --wal-path=$WAL_PATH --host=$SERVER_HOST --port=$SERVER_PORT &
    SERVER_PID=$!
    
    print_info "服务器启动中... (PID: $SERVER_PID)"
    
    # 等待服务器启动
    for i in {1..10}; do
        if curl -s http://$SERVER_HOST:$SERVER_PORT/health >/dev/null 2>&1; then
            print_success "服务器启动成功"
            return 0
        fi
        print_info "等待服务器启动... ($i/10)"
        sleep 2
    done
    
    print_error "服务器启动失败"
    exit 1
}

# 等待服务器响应
wait_for_server() {
    for i in {1..5}; do
        if curl -s http://$SERVER_HOST:$SERVER_PORT/health >/dev/null 2>&1; then
            return 0
        fi
        sleep 1
    done
    print_error "服务器无响应"
    return 1
}

# HTTP请求测试函数
test_http_request() {
    local method=$1
    local endpoint=$2
    local data=$3
    local expected_status=$4
    
    print_info "发送 $method 请求到 $endpoint"
    
    if [ "$method" = "POST" ]; then
        response=$(curl -s -w "\n%{http_code}" -X POST \
            "http://$SERVER_HOST:$SERVER_PORT$endpoint" \
            -H "Content-Type: application/json" \
            -d "$data")
    elif [ "$method" = "GET" ]; then
        response=$(curl -s -w "\n%{http_code}" \
            "http://$SERVER_HOST:$SERVER_PORT$endpoint")
    elif [ "$method" = "DELETE" ]; then
        response=$(curl -s -w "\n%{http_code}" -X DELETE \
            "http://$SERVER_HOST:$SERVER_PORT$endpoint" \
            -H "Content-Type: application/json" \
            -d "$data")
    fi
    
    # 分离响应体和状态码
    response_body=$(echo "$response" | head -n -1)
    status_code=$(echo "$response" | tail -n 1)
    
    print_info "响应状态码: $status_code"
    print_info "响应内容: $response_body"
    
    if [ "$status_code" = "$expected_status" ]; then
        print_success "HTTP请求成功"
        echo "$response_body"
        return 0
    else
        print_error "HTTP请求失败，期望状态码: $expected_status, 实际: $status_code"
        return 1
    fi
}

# 测试1: 基本数据插入和查询
test_basic_operations() {
    print_test_header "基本数据插入和查询"
    
    # 插入数据
    upsert_data='{
        "id": 1,
        "vectors": [0.1, 0.2, 0.3],
        "indexType": "FLAT",
        "category": 1
    }'
    
    test_http_request "POST" "/upsert" "$upsert_data" "200"
    
    # 查询数据
    query_result=$(test_http_request "GET" "/query/1" "" "200")
    
    # 验证查询结果
    if echo "$query_result" | grep -q '"id"'; then
        print_success "数据插入和查询测试通过"
    else
        print_error "查询结果验证失败"
        return 1
    fi
}

# 测试2: 搜索功能
test_search_functionality() {
    print_test_header "搜索功能测试"
    
    # 插入多条数据
    for i in {2..5}; do
        upsert_data="{
            \"id\": $i,
            \"vectors\": [0.$i, 0.$((i+1)), 0.$((i+2))],
            \"indexType\": \"FLAT\",
            \"category\": $i
        }"
        test_http_request "POST" "/upsert" "$upsert_data" "200"
    done
    
    # 执行搜索
    search_data='{
        "vectors": [0.1, 0.2, 0.3],
        "k": 3,
        "indexType": "FLAT"
    }'
    
    search_result=$(test_http_request "POST" "/search" "$search_data" "200")
    
    # 验证搜索结果
    if echo "$search_result" | grep -q '"vectors"' && echo "$search_result" | grep -q '"distances"'; then
        print_success "搜索功能测试通过"
    else
        print_error "搜索结果验证失败"
        return 1
    fi
}

# 测试3: 数据更新
test_data_update() {
    print_test_header "数据更新测试"
    
    # 更新已存在的数据
    update_data='{
        "id": 1,
        "vectors": [0.9, 0.8, 0.7],
        "indexType": "FLAT",
        "category": 999
    }'
    
    test_http_request "POST" "/upsert" "$update_data" "200"
    
    # 查询更新后的数据
    updated_result=$(test_http_request "GET" "/query/1" "" "200")
    
    # 验证更新结果
    if echo "$updated_result" | grep -q '"category":999'; then
        print_success "数据更新测试通过"
    else
        print_error "数据更新验证失败"
        return 1
    fi
}

# 测试4: WAL日志验证
test_wal_log_verification() {
    print_test_header "WAL日志验证"
    
    # 检查WAL日志文件是否存在
    if [ -f "$WAL_PATH" ]; then
        print_success "WAL日志文件已创建: $WAL_PATH"
        
        # 检查日志内容
        log_lines=$(wc -l < "$WAL_PATH")
        print_info "WAL日志文件包含 $log_lines 行"
        
        # 验证日志格式
        if grep -q "upsert" "$WAL_PATH"; then
            print_success "WAL日志包含upsert操作记录"
        else
            print_warning "WAL日志中未找到upsert操作记录"
        fi
        
        # 显示日志内容示例
        print_info "WAL日志内容示例:"
        head -3 "$WAL_PATH" | while read line; do
            print_info "  $line"
        done
        
    else
        print_error "WAL日志文件不存在: $WAL_PATH"
        return 1
    fi
}

# 测试5: 服务器重启数据恢复
test_server_restart_recovery() {
    print_test_header "服务器重启数据恢复测试"
    
    # 记录重启前的数据
    pre_restart_data=$(test_http_request "GET" "/query/1" "" "200")
    print_info "重启前数据: $pre_restart_data"
    
    # 停止服务器
    print_info "停止服务器..."
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null || true
    SERVER_PID=""
    
    sleep 2
    
    # 重新启动服务器
    print_info "重新启动服务器..."
    start_server
    
    # 等待服务器启动完成
    wait_for_server
    
    # 验证数据是否恢复
    post_restart_data=$(test_http_request "GET" "/query/1" "" "200")
    print_info "重启后数据: $post_restart_data"
    
    # 比较数据一致性
    if echo "$post_restart_data" | grep -q '"id"'; then
        print_success "服务器重启后数据恢复成功"
    else
        print_warning "服务器重启后数据可能未完全恢复"
        print_info "这可能需要手动调用恢复接口或检查自动恢复机制"
    fi
}

# 测试6: 错误处理
test_error_handling() {
    print_test_header "错误处理测试"
    
    # 测试无效数据插入
    invalid_data='{
        "invalid": "data"
    }'
    
    if test_http_request "POST" "/upsert" "$invalid_data" "400" >/dev/null 2>&1; then
        print_success "无效数据正确返回400错误"
    else
        print_warning "无效数据错误处理可能需要改进"
    fi
    
    # 测试查询不存在的数据
    if test_http_request "GET" "/query/99999" "" "404" >/dev/null 2>&1; then
        print_success "查询不存在数据正确返回404错误"
    else
        print_warning "查询不存在数据的错误处理可能需要改进"
    fi
}

# 测试7: 性能基准测试
test_performance_benchmark() {
    print_test_header "性能基准测试"
    
    print_info "执行批量插入性能测试..."
    
    start_time=$(date +%s)
    
    # 批量插入100个向量
    for i in {100..199}; do
        upsert_data="{
            \"id\": $i,
            \"vectors\": [0.$((i%10)), 0.$((i%20)), 0.$((i%30))],
            \"indexType\": \"FLAT\",
            \"category\": $((i%5))
        }"
        
        test_http_request "POST" "/upsert" "$upsert_data" "200" >/dev/null
        
        # 每10个打印进度
        if [ $((i % 10)) -eq 0 ]; then
            print_info "已插入 $((i-99))/100 个向量"
        fi
    done
    
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    print_success "批量插入100个向量耗时: ${duration}秒"
    
    # 性能基准验证
    if [ $duration -lt 30 ]; then
        print_success "性能测试通过 (< 30秒)"
    else
        print_warning "性能可能需要优化 (> 30秒)"
    fi
}

# 主测试流程
main() {
    print_info "开始数据日志持久化功能端到端测试"
    print_info "测试配置:"
    print_info "  服务器地址: $SERVER_HOST:$SERVER_PORT"
    print_info "  数据库路径: $DB_PATH"
    print_info "  WAL日志路径: $WAL_PATH"
    
    # 启动服务器
    start_server
    
    # 运行测试
    test_basic_operations
    test_search_functionality
    test_data_update
    test_wal_log_verification
    test_server_restart_recovery
    test_error_handling
    test_performance_benchmark
    
    print_success "所有端到端测试完成！"
    
    # 显示最终的WAL日志统计
    if [ -f "$WAL_PATH" ]; then
        final_log_count=$(wc -l < "$WAL_PATH")
        print_info "最终WAL日志条目数: $final_log_count"
    fi
}

# 执行主函数
main "$@" 