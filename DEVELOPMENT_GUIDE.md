# 信控程序开发调试指南

本文档提供信控程序在macOS环境下的完整开发和调试指南。

## 目录

1. [环境准备](#环境准备)
2. [项目结构](#项目结构)
3. [编译和运行](#编译和运行)
4. [调试方法](#调试方法)
5. [测试指南](#测试指南)
6. [常见问题](#常见问题)
7. [部署指南](#部署指南)

## 环境准备

### 1. Rust工具链

```bash
# 安装Rust (如果尚未安装)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env

# 验证安装
rustc --version
cargo --version
```

### 2. 交叉编译支持 (可选)

如需为嵌入式Linux目标编译：

```bash
# 添加ARM64目标
rustup target add aarch64-unknown-linux-gnu

# 安装交叉编译工具链 (macOS)
brew install aarch64-elf-gcc
```

### 3. 开发工具

```bash
# 代码格式化和检查工具
rustup component add rustfmt clippy

# 可选：安装cargo-watch用于自动重编译
cargo install cargo-watch
```

## 项目结构

```
sigctl/
├── src/
│   ├── main.rs          # 程序入口
│   ├── types.rs         # 数据结构定义
│   ├── io_can.rs        # CAN通信模块
│   ├── core.rs          # 控制核心逻辑
│   ├── ui_panel.rs      # 面板通信模块
│   └── log.rs           # 日志模块
├── config.yaml          # 配置文件
├── Cargo.toml           # 项目配置
├── test_quick.sh        # 快速测试脚本
├── test_e2e.sh          # 端到端测试脚本
└── Development.md       # 架构说明文档
```

### 核心模块说明

- **io_can.rs**: CAN通信抽象层，Linux使用SocketCAN，macOS使用模拟接口
- **core.rs**: 状态机控制逻辑，实现固定配时信号控制
- **types.rs**: 系统配置和消息类型定义
- **ui_panel.rs**: 串口面板通信 (可选)

## 编译和运行

### 1. 基本编译

```bash
# 调试版本编译
cargo build

# 发布版本编译
cargo build --release

# 检查代码 (不生成二进制)
cargo check
```

### 2. 运行程序

```bash
# 直接运行 (使用config.yaml)
cargo run

# 或运行编译后的二进制
./target/debug/sigctl
```

### 3. 自动重编译 (开发时推荐)

```bash
# 监听文件变化，自动重编译和运行
cargo watch -x run

# 仅监听编译
cargo watch -x check
```

## 调试方法

### 1. 日志调试

程序使用分级日志系统：

```bash
# 查看实时日志
tail -f sigctl.log

# 过滤特定级别
grep "ERROR" sigctl.log
grep "状态转换" sigctl.log
```

**日志级别说明：**
- `ERROR`: 严重错误
- `WARN`: 警告信息 (如电压异常、故障事件)
- `INFO`: 重要信息 (如状态转换、性能统计)
- `DEBUG`: 详细调试信息 (如CAN事件、灯状态)

### 2. 状态机调试

程序会输出详细的状态转换日志：

```
[INFO] 状态转换: AllRed -> NsGreen
[INFO] 状态转换: NsGreen -> NsYellow
[INFO] 状态转换: NsYellow -> AllRed
[INFO] 状态转换: AllRed -> EwGreen
```

**性能统计输出：**
```
[INFO] 性能统计 - 运行时间: 120.5s, 状态变更: 20, 事件处理: 156, CAN故障: 0, 灯故障: 0, 平均周期: 24.1s
```

### 3. CAN通信调试

在macOS上，程序使用模拟CAN接口：

- 每5秒发送驱动板状态
- 每3秒发送灯组状态变化
- 每10秒发送电压信息
- 启动时发送版本信息

**查看CAN事件：**
```bash
grep "驱动板状态\|灯状态\|电压" sigctl.log
```

### 4. 配置调试

**验证配置文件：**
```bash
# 检查配置文件语法
python3 -c "import yaml; yaml.safe_load(open('config.yaml'))"

# 或使用在线YAML验证器
```

**常用配置修改：**
```yaml
# 调整时序 (毫秒)
timing:
  green_ms: 10000    # 绿灯时间
  yellow_ms: 3000    # 黄灯时间
  allred_ms: 1000    # 全红时间

# 调整日志级别 (通过环境变量)
# export RUST_LOG=debug
```

### 5. 内存和性能调试

```bash
# 使用Valgrind (需要安装)
valgrind --tool=memcheck ./target/debug/sigctl

# 查看程序资源使用
top -p $(pgrep sigctl)

# 或使用htop
htop -p $(pgrep sigctl)
```

## 测试指南

### 1. 单元测试

```bash
# 运行所有测试
cargo test

# 运行特定模块测试
cargo test io_can::tests

# 显示测试输出
cargo test -- --nocapture

# 运行单个测试
cargo test test_pack_heartbeat
```

### 2. 快速功能测试

```bash
# 运行快速测试 (10秒)
./test_quick.sh
```

快速测试包括：
- 编译验证
- 单元测试
- 程序启动测试
- 基本功能验证

### 3. 完整端到端测试

```bash
# 运行完整测试 (约2分钟)
./test_e2e.sh
```

端到端测试包括：
- 编译和单元测试
- 配置文件测试
- 长时间运行测试
- 性能和内存检查
- 日志分析

### 4. 协议测试

```bash
# 测试CAN协议编解码
cargo test io_can::tests::test_protocol_roundtrip

# 测试所有协议功能
cargo test io_can::tests
```

## 常见问题

### 1. 编译问题

**问题：** `error: linker 'cc' not found`
```bash
# macOS解决方案
xcode-select --install
```

**问题：** 依赖版本冲突
```bash
# 清理并重新构建
cargo clean
cargo build
```

### 2. 运行时问题

**问题：** 程序启动后立即退出
- 检查config.yaml文件是否存在且格式正确
- 查看错误日志：`./target/debug/sigctl 2>&1 | tee error.log`

**问题：** CAN模拟不工作
- 确认在非Linux系统上运行
- 检查日志中是否有"CAN模拟启动"信息

**问题：** 状态机不转换
- 检查时序配置是否合理
- 查看状态转换日志
- 确认没有阻塞性错误

### 3. 性能问题

**问题：** 内存使用过高
- 检查日志文件大小限制
- 监控事件处理频率
- 使用`cargo build --release`优化版本

**问题：** CPU使用率高
- 检查心跳频率设置 (默认100ms)
- 优化日志输出频率
- 检查是否有死循环

## 部署指南

### 1. 交叉编译 (macOS -> Linux)

```bash
# 为ARM64 Linux编译
cargo build --release --target aarch64-unknown-linux-gnu

# 编译结果位于
# target/aarch64-unknown-linux-gnu/release/sigctl
```

### 2. 部署到目标设备

```bash
# 使用SCP传输
scp target/aarch64-unknown-linux-gnu/release/sigctl user@target:/opt/sigctl/
scp config.yaml user@target:/opt/sigctl/

# 或使用NFS共享开发
# 在目标设备上挂载开发机的项目目录
```

### 3. 目标设备配置

```bash
# 在嵌入式Linux上
# 1. 配置CAN接口
sudo ip link set can0 type can bitrate 250000
sudo ip link set up can0

# 2. 设置自启动
sudo systemctl enable sigctl.service

# 3. 启动程序
sudo systemctl start sigctl.service
```

### 4. 远程调试

```bash
# SSH隧道调试
ssh -L 2222:localhost:22 user@target

# 远程日志监控
ssh user@target 'tail -f /opt/sigctl/sigctl.log'

# 远程性能监控
ssh user@target 'top -p $(pgrep sigctl)'
```

## 开发最佳实践

### 1. 代码质量

```bash
# 代码格式化
cargo fmt

# 代码检查
cargo clippy

# 文档生成
cargo doc --open
```

### 2. 版本控制

```bash
# 提交前检查
cargo test && cargo clippy && cargo fmt --check

# 标记版本
git tag v1.0.0
```

### 3. 持续集成

建议在CI/CD中运行：
- `cargo test` - 单元测试
- `cargo clippy` - 代码检查
- `./test_e2e.sh` - 端到端测试
- 交叉编译验证

### 4. 调试技巧

1. **使用条件编译**：
   ```rust
   #[cfg(debug_assertions)]
   println!("调试信息: {:?}", data);
   ```

2. **环境变量控制**：
   ```bash
   RUST_LOG=debug ./target/debug/sigctl
   ```

3. **分阶段测试**：
   - 先运行单元测试
   - 再运行快速测试
   - 最后运行完整测试

4. **日志分析**：
   ```bash
   # 统计错误
   grep -c "ERROR" sigctl.log
   
   # 分析性能
   grep "性能统计" sigctl.log | tail -5
   
   # 查看状态转换频率
   grep "状态转换" sigctl.log | wc -l
   ```

---

## 联系和支持

如遇到问题，请：
1. 查看本文档的常见问题部分
2. 运行测试脚本诊断问题
3. 收集相关日志信息
4. 提供复现步骤

**调试信息收集模板：**
```bash
# 系统信息
uname -a
rustc --version

# 编译信息
cargo build 2>&1 | tee build.log

# 运行日志
./target/debug/sigctl 2>&1 | tee run.log

# 测试结果
./test_quick.sh 2>&1 | tee test.log
```