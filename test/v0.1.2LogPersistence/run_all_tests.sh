#!/bin/bash

# run_all_tests.sh
# 数据日志持久化功能完整测试执行脚本

set -e  # 遇到错误时停止执行

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 打印函数
print_header() {
    echo -e "\n${PURPLE}=================================="
    echo -e "$1"
    echo -e "==================================${NC}\n"
}

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

print_step() {
    echo -e "\n${CYAN}🔹 $1${NC}"
}

# 全局变量
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
START_TIME=$(date +%s)

# 测试结果记录
test_result() {
    local test_name=$1
    local result=$2
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ $result -eq 0 ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
        print_success "$test_name 通过"
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
        print_error "$test_name 失败"
    fi
}

# 检查依赖
check_dependencies() {
    print_step "检查系统依赖"
    
    local missing_deps=()
    
    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        missing_deps+=("g++")
    fi
    
    # 检查make
    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi
    
    # 检查curl (用于端到端测试)
    if ! command -v curl &> /dev/null; then
        missing_deps+=("curl")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "缺少以下依赖: ${missing_deps[*]}"
        print_info "请安装缺少的依赖后重试"
        exit 1
    fi
    
    print_success "所有系统依赖已满足"
}

# 清理环境
cleanup_environment() {
    print_step "清理测试环境"
    make clean
    rm -rf /tmp/vdb_test_v0.1.2
    print_success "环境清理完成"
}

# 设置环境
setup_environment() {
    print_step "设置测试环境"
    make setup
    print_success "测试环境设置完成"
}

# 编译测试
compile_tests() {
    print_step "编译所有测试程序"
    
    if make all; then
        print_success "所有测试程序编译成功"
        return 0
    else
        print_error "测试程序编译失败"
        return 1
    fi
}

# 运行单元测试
run_unit_tests() {
    print_step "运行单元测试"
    
    if make run-unit; then
        test_result "单元测试" 0
    else
        test_result "单元测试" 1
    fi
}

# 运行集成测试
run_integration_tests() {
    print_step "运行集成测试"
    
    if make run-integration; then
        test_result "集成测试" 0
    else
        test_result "集成测试" 1
    fi
}

# 运行性能测试
run_performance_tests() {
    print_step "运行性能测试"
    
    if [ -f "performance_tests" ]; then
        if make run-performance; then
            test_result "性能测试" 0
        else
            test_result "性能测试" 1
        fi
    else
        print_warning "性能测试程序不存在，跳过"
    fi
}

# 运行错误测试
run_error_tests() {
    print_step "运行错误处理测试"
    
    if [ -f "error_tests" ]; then
        if make run-errors; then
            test_result "错误处理测试" 0
        else
            test_result "错误处理测试" 1
        fi
    else
        print_warning "错误测试程序不存在，跳过"
    fi
}

# 运行端到端测试
run_e2e_tests() {
    print_step "运行端到端测试"
    
    # 检查是否存在服务器二进制文件
    if [ ! -f "../../vdb_server" ]; then
        print_warning "服务器二进制文件不存在，跳过端到端测试"
        print_info "请先编译服务器程序: cd ../.. && make"
        return
    fi
    
    if make run-e2e; then
        test_result "端到端测试" 0
    else
        test_result "端到端测试" 1
    fi
}

# 代码质量检查
run_code_quality_checks() {
    print_step "代码质量检查"
    
    # 静态分析
    if command -v cppcheck &> /dev/null; then
        print_info "运行静态代码分析..."
        if make analyze; then
            print_success "静态分析通过"
        else
            print_warning "静态分析发现问题"
        fi
    else
        print_warning "cppcheck 未安装，跳过静态分析"
    fi
    
    # 代码格式检查
    if command -v clang-format &> /dev/null; then
        print_info "检查代码格式..."
        # 这里只是显示信息，不强制格式化
        print_info "如需格式化代码，请运行: make format"
    else
        print_warning "clang-format 未安装，跳过格式检查"
    fi
}

# 内存检查
run_memory_checks() {
    print_step "内存泄漏检查"
    
    if command -v valgrind &> /dev/null; then
        print_info "运行内存泄漏检查 (仅检查单元测试)..."
        if make memcheck; then
            print_success "内存检查通过"
        else
            print_warning "内存检查发现问题"
        fi
    else
        print_warning "valgrind 未安装，跳过内存检查"
        print_info "安装命令: sudo apt-get install valgrind"
    fi
}

# 生成测试报告
generate_test_report() {
    print_step "生成测试报告"
    
    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))
    local success_rate=0
    
    if [ $TOTAL_TESTS -gt 0 ]; then
        success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    
    # 创建报告文件
    local report_file="/tmp/vdb_test_v0.1.2/test_report_$(date +%Y%m%d_%H%M%S).txt"
    mkdir -p "$(dirname "$report_file")"
    
    cat > "$report_file" << EOF
数据日志持久化功能测试报告
===============================

测试时间: $(date)
测试版本: v0.1.2
测试环境: $(uname -a)

测试结果统计:
- 总测试数: $TOTAL_TESTS
- 通过测试: $PASSED_TESTS
- 失败测试: $FAILED_TESTS
- 成功率: $success_rate%
- 总耗时: ${duration}秒

详细结果:
$(if [ $FAILED_TESTS -eq 0 ]; then echo "🎉 所有测试通过！"; else echo "⚠️  有测试失败，请检查详细日志"; fi)

测试环境:
- 编译器: $(g++ --version | head -n1)
- 操作系统: $(cat /etc/os-release | grep PRETTY_NAME | cut -d'"' -f2)
- 内核版本: $(uname -r)

建议:
$(if [ $success_rate -lt 100 ]; then echo "- 修复失败的测试用例"; fi)
$(if ! command -v valgrind &> /dev/null; then echo "- 安装 valgrind 进行内存检查"; fi)
$(if ! command -v cppcheck &> /dev/null; then echo "- 安装 cppcheck 进行静态分析"; fi)
EOF

    print_info "测试报告已生成: $report_file"
    
    # 显示简要报告
    print_header "测试结果总结"
    echo -e "${CYAN}总测试数:${NC} $TOTAL_TESTS"
    echo -e "${GREEN}通过测试:${NC} $PASSED_TESTS"
    echo -e "${RED}失败测试:${NC} $FAILED_TESTS"
    echo -e "${YELLOW}成功率:${NC} $success_rate%"
    echo -e "${BLUE}总耗时:${NC} ${duration}秒"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        print_success "🎉 所有测试通过！数据日志持久化功能工作正常。"
    else
        print_warning "⚠️  有 $FAILED_TESTS 个测试失败，请检查详细日志并修复问题。"
    fi
}

# 使用说明
show_usage() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --unit-only        只运行单元测试"
    echo "  --integration-only 只运行集成测试"
    echo "  --performance-only 只运行性能测试"
    echo "  --e2e-only         只运行端到端测试"
    echo "  --no-e2e          跳过端到端测试"
    echo "  --no-memcheck     跳过内存检查"
    echo "  --no-quality      跳过代码质量检查"
    echo "  --quick           快速测试（跳过可选检查）"
    echo "  --help            显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0                # 运行所有测试"
    echo "  $0 --unit-only    # 只运行单元测试"
    echo "  $0 --quick        # 快速测试模式"
}

# 主函数
main() {
    # 解析命令行参数
    local unit_only=false
    local integration_only=false
    local performance_only=false
    local e2e_only=false
    local no_e2e=false
    local no_memcheck=false
    local no_quality=false
    local quick_mode=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --unit-only)
                unit_only=true
                shift
                ;;
            --integration-only)
                integration_only=true
                shift
                ;;
            --performance-only)
                performance_only=true
                shift
                ;;
            --e2e-only)
                e2e_only=true
                shift
                ;;
            --no-e2e)
                no_e2e=true
                shift
                ;;
            --no-memcheck)
                no_memcheck=true
                shift
                ;;
            --no-quality)
                no_quality=true
                shift
                ;;
            --quick)
                quick_mode=true
                no_memcheck=true
                no_quality=true
                shift
                ;;
            --help)
                show_usage
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    print_header "数据日志持久化功能测试套件 v0.1.2"
    
    # 检查依赖
    check_dependencies
    
    # 清理和设置环境
    cleanup_environment
    setup_environment
    
    # 编译测试
    if ! compile_tests; then
        print_error "编译失败，无法继续测试"
        exit 1
    fi
    
    # 根据参数运行相应测试
    if $unit_only; then
        run_unit_tests
    elif $integration_only; then
        run_integration_tests
    elif $performance_only; then
        run_performance_tests
    elif $e2e_only; then
        run_e2e_tests
    else
        # 运行所有测试
        run_unit_tests
        run_integration_tests
        run_performance_tests
        run_error_tests
        
        if ! $no_e2e; then
            run_e2e_tests
        fi
        
        if ! $quick_mode; then
            if ! $no_quality; then
                run_code_quality_checks
            fi
            
            if ! $no_memcheck; then
                run_memory_checks
            fi
        fi
    fi
    
    # 生成测试报告
    generate_test_report
    
    # 返回适当的退出码
    if [ $FAILED_TESTS -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# 执行主函数
main "$@" 