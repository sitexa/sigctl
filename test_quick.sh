#!/bin/bash

# 快速测试脚本 - 用于日常开发调试
# 运行基本的编译、测试和短时间功能验证

set -e

echo "=== 信控程序快速测试 ==="
echo "测试时间: $(date)"
echo

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# 清理函数
cleanup() {
    if [ ! -z "$SIGCTL_PID" ]; then
        log_info "停止程序..."
        kill $SIGCTL_PID 2>/dev/null || true
        wait $SIGCTL_PID 2>/dev/null || true
    fi
    rm -f quick_test.log
}

trap cleanup EXIT

# 1. 快速编译
log_info "1. 编译程序..."
if cargo build; then
    echo -e "${GREEN}✓${NC} 编译成功"
else
    echo -e "${RED}✗${NC} 编译失败"
    exit 1
fi

# 2. 运行单元测试
log_info "2. 运行单元测试..."
if cargo test --quiet; then
    echo -e "${GREEN}✓${NC} 单元测试通过"
else
    echo -e "${RED}✗${NC} 单元测试失败"
    exit 1
fi

# 3. 启动程序进行功能测试
log_info "3. 启动程序测试 (10秒)..."
./target/debug/sigctl > quick_test.log 2>&1 &
SIGCTL_PID=$!

# 等待启动
sleep 2

# 检查程序状态
if kill -0 $SIGCTL_PID 2>/dev/null; then
    echo -e "${GREEN}✓${NC} 程序启动成功"
else
    echo -e "${RED}✗${NC} 程序启动失败"
    if [ -f "quick_test.log" ]; then
        log_error "程序输出:"
        cat quick_test.log
    fi
    exit 1
fi

# 运行8秒观察状态转换
sleep 8

# 检查日志内容
if [ -f "quick_test.log" ]; then
    if grep -q "控制核心启动" quick_test.log; then
        echo -e "${GREEN}✓${NC} 控制核心正常启动"
    else
        echo -e "${RED}✗${NC} 控制核心启动异常"
    fi
    
    if grep -q "状态转换" quick_test.log; then
        echo -e "${GREEN}✓${NC} 状态机正常运行"
    else
        echo -e "${YELLOW}!${NC} 状态机可能未完成完整周期"
    fi
    
    if grep -q "驱动板状态" quick_test.log; then
        echo -e "${GREEN}✓${NC} CAN模拟正常工作"
    else
        echo -e "${YELLOW}!${NC} CAN模拟可能未启动"
    fi
    
    # 检查错误
    error_count=$(grep -c "ERROR" quick_test.log || echo "0")
    if [ "$error_count" -eq 0 ]; then
        echo -e "${GREEN}✓${NC} 无错误日志"
    else
        echo -e "${RED}✗${NC} 发现 ${error_count} 条错误"
        grep "ERROR" quick_test.log | head -3
    fi
fi

# 停止程序
log_info "4. 停止程序..."
kill $SIGCTL_PID 2>/dev/null || true
wait $SIGCTL_PID 2>/dev/null || true
SIGCTL_PID=""

echo
echo -e "${GREEN}快速测试完成！${NC}"
echo "如需完整测试，请运行: ./test_e2e.sh"
echo "查看详细日志: cat quick_test.log"