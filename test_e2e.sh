#!/bin/bash

# 信控程序端到端测试脚本
# 用于验证程序的基本功能和稳定性

set -e

echo "=== 信控程序端到端测试 ==="
echo "测试时间: $(date)"
echo

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试结果统计
TEST_PASSED=0
TEST_FAILED=0

# 辅助函数
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

test_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TEST_PASSED++))
}

test_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TEST_FAILED++))
}

# 清理函数
cleanup() {
    if [ ! -z "$SIGCTL_PID" ]; then
        log_info "停止信控程序 (PID: $SIGCTL_PID)"
        kill $SIGCTL_PID 2>/dev/null || true
        wait $SIGCTL_PID 2>/dev/null || true
    fi
    
    # 清理测试文件
    rm -f test_output.log test_config.yaml
}

trap cleanup EXIT

# 1. 编译测试
log_info "1. 编译测试"
if cargo build --release; then
    test_pass "程序编译成功"
else
    test_fail "程序编译失败"
    exit 1
fi
echo

# 2. 单元测试
log_info "2. 单元测试"
if cargo test; then
    test_pass "单元测试通过"
else
    test_fail "单元测试失败"
fi
echo

# 3. 配置文件测试
log_info "3. 配置文件测试"
cat > test_config.yaml << EOF
can:
  interface: "can0"
  bitrate: 250000

ids:
  dn: 0x100
  up: 0x180

serial:
  device: "/dev/ttyS2"
  baudrate: 115200

timing:
  green_ms: 15000
  yellow_ms: 3000
  allred_ms: 1000

mapping:
  ns_r: 1
  ns_y: 2
  ns_g: 3
  ew_r: 4
  ew_y: 5
  ew_g: 6

log:
  file: "test_output.log"
  max_size_mb: 10
EOF

if [ -f "test_config.yaml" ]; then
    test_pass "测试配置文件创建成功"
else
    test_fail "测试配置文件创建失败"
fi
echo

# 4. 程序启动测试
log_info "4. 程序启动测试"
cp test_config.yaml config.yaml

# 启动程序并获取PID
./target/release/sigctl > test_output.log 2>&1 &
SIGCTL_PID=$!

# 等待程序启动
sleep 3

# 检查程序是否还在运行
if kill -0 $SIGCTL_PID 2>/dev/null; then
    test_pass "程序启动成功 (PID: $SIGCTL_PID)"
else
    test_fail "程序启动失败"
    if [ -f "test_output.log" ]; then
        log_error "程序输出:"
        cat test_output.log
    fi
fi
echo

# 5. 日志输出测试
log_info "5. 日志输出测试"
sleep 5  # 等待一些日志输出

if [ -f "test_output.log" ] && [ -s "test_output.log" ]; then
    test_pass "日志文件生成成功"
    
    # 检查关键日志内容
    if grep -q "控制核心启动" test_output.log; then
        test_pass "控制核心启动日志正常"
    else
        test_fail "缺少控制核心启动日志"
    fi
    
    if grep -q "状态转换" test_output.log; then
        test_pass "状态转换日志正常"
    else
        test_fail "缺少状态转换日志"
    fi
    
    if grep -q "驱动板状态" test_output.log; then
        test_pass "CAN模拟事件日志正常"
    else
        test_fail "缺少CAN模拟事件日志"
    fi
else
    test_fail "日志文件未生成或为空"
fi
echo

# 6. 状态机周期测试
log_info "6. 状态机周期测试 (运行30秒)"
sleep 30

# 检查状态转换是否正常
if [ -f "test_output.log" ]; then
    ns_green_count=$(grep -c "NsGreen" test_output.log || echo "0")
    ew_green_count=$(grep -c "EwGreen" test_output.log || echo "0")
    all_red_count=$(grep -c "AllRed" test_output.log || echo "0")
    
    log_info "状态统计: NS绿灯=${ns_green_count}, EW绿灯=${ew_green_count}, 全红=${all_red_count}"
    
    if [ "$ns_green_count" -gt 0 ] && [ "$ew_green_count" -gt 0 ] && [ "$all_red_count" -gt 0 ]; then
        test_pass "状态机周期运行正常"
    else
        test_fail "状态机周期运行异常"
    fi
else
    test_fail "无法检查状态机周期"
fi
echo

# 7. 性能统计测试
log_info "7. 性能统计测试"
if grep -q "性能统计" test_output.log; then
    test_pass "性能统计输出正常"
    # 显示最新的性能统计
    log_info "最新性能统计:"
    grep "性能统计" test_output.log | tail -1
else
    test_fail "缺少性能统计输出"
fi
echo

# 8. 内存泄漏检测 (简单检查)
log_info "8. 内存使用检测"
if command -v ps >/dev/null 2>&1; then
    if kill -0 $SIGCTL_PID 2>/dev/null; then
        memory_kb=$(ps -o rss= -p $SIGCTL_PID 2>/dev/null || echo "0")
        if [ "$memory_kb" -gt 0 ] && [ "$memory_kb" -lt 102400 ]; then  # 100MB限制
            test_pass "内存使用正常 (${memory_kb}KB)"
        else
            test_warn "内存使用较高 (${memory_kb}KB)"
        fi
    else
        test_fail "程序已退出，无法检测内存使用"
    fi
else
    test_warn "无法检测内存使用 (ps命令不可用)"
fi
echo

# 9. 优雅停止测试
log_info "9. 优雅停止测试"
if kill -0 $SIGCTL_PID 2>/dev/null; then
    kill -TERM $SIGCTL_PID
    sleep 3
    
    if kill -0 $SIGCTL_PID 2>/dev/null; then
        log_warn "程序未响应TERM信号，使用KILL信号"
        kill -KILL $SIGCTL_PID
        test_fail "程序未能优雅停止"
    else
        test_pass "程序优雅停止成功"
    fi
    SIGCTL_PID=""  # 清空PID，避免cleanup重复处理
else
    test_fail "程序已意外退出"
fi
echo

# 10. 日志分析
log_info "10. 日志分析"
if [ -f "test_output.log" ]; then
    error_count=$(grep -c "ERROR" test_output.log || echo "0")
    warn_count=$(grep -c "WARN" test_output.log || echo "0")
    info_count=$(grep -c "INFO" test_output.log || echo "0")
    
    log_info "日志统计: ERROR=${error_count}, WARN=${warn_count}, INFO=${info_count}"
    
    if [ "$error_count" -eq 0 ]; then
        test_pass "无错误日志"
    else
        test_fail "发现 ${error_count} 条错误日志"
        log_error "错误日志:"
        grep "ERROR" test_output.log | head -5
    fi
    
    if [ "$warn_count" -lt 10 ]; then
        test_pass "警告日志数量正常 (${warn_count})"
    else
        test_warn "警告日志较多 (${warn_count})"
    fi
fi
echo

# 测试结果汇总
echo "=== 测试结果汇总 ==="
echo -e "通过: ${GREEN}${TEST_PASSED}${NC}"
echo -e "失败: ${RED}${TEST_FAILED}${NC}"
echo -e "总计: $((TEST_PASSED + TEST_FAILED))"
echo

if [ $TEST_FAILED -eq 0 ]; then
    echo -e "${GREEN}所有测试通过！${NC}"
    exit 0
else
    echo -e "${RED}有 ${TEST_FAILED} 个测试失败${NC}"
    exit 1
fi