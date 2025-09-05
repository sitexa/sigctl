1) 项目简介与定位
- 目标：在嵌入式 Linux 主控板（如 EM928x）+ 灯控板上，落地最小可用（MVP）的信号控制系统：双相位固定配时、CAN 通讯、失联/故障安全降级（黄闪）、面板交互骨架、可持续运行与日志。
- 当前形态：完整的运行骨架与生产可用的 CAN 通讯层（Linux 上使用 SocketCAN，非 Linux 平台提供仿真）；控制核心采用固定配时状态机，已具备心跳、降级、恢复等关键流程。
- 面向对象：现场联调工程师、开发者（Rust/C），维护与运维人员。

2) 已实现功能清单
- 固定配时双相位控制（NS↔EW），包含绿/黄/全红的周期切换与稳态控制。
- CAN 通讯协议栈（0xA*/0xB*）：发送心跳、点控灯状态、方案与默认绿；接收驱动板状态、灯状态、故障、电压、版本等帧，支持组帧/解析与单元测试。
- 失联/故障安全降级：检测到 CAN 故障或灯故障时进入黄闪模式，周期重申黄闪指令；检测恢复后回归配时状态机。
- 周期心跳：100ms 下行心跳保持设备在线判定。
- 串口面板通信骨架：提供面板串口（COM5）读写通道，可在此之上扩展面板协议（0xC*/0xD*）。
- 跨平台运行：Linux 直连真实 CAN；非 Linux 平台使用仿真（便于本机开发与调试）。
- 日志系统：简单稳健的文件日志与滚动（大小阈值切换 .bak），路径/大小可配。
- 可选策略配置：通过命名 Profile 选择时序，未命中则回退默认时序。

3) 系统结构与文件组织
- 入口与进程编排
  - <mcfile name="main.rs" path="/Users/xnpeng/sumoptis/sigctl/src/main.rs"></mcfile>：加载配置、初始化日志、启动 CAN I/O、启动面板串口（可选）、启动控制核心。
- 类型与协议
  - <mcfile name="types.rs" path="/Users/xnpeng/sumoptis/sigctl/src/types.rs"></mcfile>：Config 配置结构、命令与事件枚举、时序/映射/策略等核心类型，以及协议 TAIL 常量（0xED）。
- CAN I/O 子系统
  - <mcfile name="io_can.rs" path="/Users/xnpeng/sumoptis/sigctl/src/io_can.rs"></mcfile>：Linux 使用 SocketCAN，非 Linux 使用 Mock；负责 OutMsg→帧编码（0xA*）与 Up 帧（0xB*）解析为 IoEvent。
- 控制核心与模块系统
  - <mcfile name="core.rs" path="/Users/xnpeng/sumoptis/sigctl/src/core.rs"></mcfile>：主控骨架（心跳任务 + 委托定时控制模块）。
  - <mcfile name="modules.rs" path="/Users/xnpeng/sumoptis/sigctl/src/modules.rs"></mcfile>：模块系统与定时控制模块 TimingController（状态机+降级逻辑+策略选择）。
- 面板串口
  - <mcfile name="ui_panel.rs" path="/Users/xnpeng/sumoptis/sigctl/src/ui_panel.rs"></mcfile>：面板串口（读/写）骨架，预留协议扩展点。
- 日志
  - <mcfile name="log.rs" path="/Users/xnpeng/sumoptis/sigctl/src/log.rs"></mcfile>：文件日志与按大小滚动。
- 配置与文档
  - <mcfile name="config.yaml" path="/Users/xnpeng/sumoptis/sigctl/conf/config.yaml"></mcfile>：系统配置。
  - <mcfile name="README.md" path="/Users/xnpeng/sumoptis/sigctl/README.md"></mcfile>、<mcfile name="DEVELOPMENT_GUIDE.md" path="/Users/xnpeng/sumoptis/sigctl/DEVELOPMENT_GUIDE.md"></mcfile>：设计与开发说明。
- 示例与测试
  - <mcfile name="can_demo.rs" path="/Users/xnpeng/sumoptis/sigctl/examples/can_demo.rs"></mcfile>：CAN 通讯示例程序（演示用）。
  - <mcfile name="test_quick.sh" path="/Users/xnpeng/sumoptis/sigctl/test_quick.sh"></mcfile>、<mcfile name="test_e2e.sh" path="/Users/xnpeng/sumoptis/sigctl/test_e2e.sh"></mcfile>：测试脚本（作为参考）。

4) 程序逻辑（启动到运行）
- 启动流程（入口）
  - 读取配置 conf/config.yaml。
  - 初始化日志（路径与大小阈值）。
  - 启动 CAN I/O 子系统，获得 tx_can（发送）与 rx_evt（事件）。
  - 尝试启动面板串口（失败仅警告，不退出）。
  - 启动控制核心（异步），进入主循环与状态机。
- CAN I/O（收发与协议）
  - Linux：打开配置指定的 CAN 接口（如 can0），按 11-bit 标识符 ids.down/ids.up 收发。
  - 非 Linux：模拟发送（打印/调试）与周期性模拟接收（版本、板状态、灯状态、电压、故障注入与恢复等），便于本机联调。
  - OutMsg→0xA*（pack_dn）：支持 Heartbeat、SetLamp、Scheme、FailFlash、Resume、DefaultGreen、FailMode。
  - 0xB*→IoEvent（parse_up）：支持 BoardSt、LampSt、LampFault、CanFault、Voltage、Version。
- 控制核心与定时控制
  - 主控固定每 100ms 发送 Heartbeat。
  - 定时控制模块 TimingController 执行状态机：
    - 周期序列：AllRed → NsGreen → NsYellow → AllRed → EwGreen → EwYellow → AllRed …
    - 每一状态通过点控 0xAA 写入灯通道：红/黄/绿/灭（0/1/2/3），按 mapping 映射到 6 路通道。
    - 过程中监听 IoEvent：记录统计、打印调试；检测 CanFault/LampFault 时进入降级。
  - 降级（FailFlash）：
    - 触发条件：收到 CanFault state!=0 或 LampFault。
    - 动作：发送 OutMsg::FailFlash，周期重申（确保现场黄闪）；监听恢复信号后退出，回到 AllRed 稳态，再继续配时。
  - 策略选择：
    - 支持通过 strategy.active_profile 指定命名时序（profiles 列表中匹配）；未匹配回退到 timing。
- 面板串口
  - 读写通道与日志，解析/协议留空；可按 0xC*/0xD* 协议扩展，不影响现有逻辑。
- 日志与监控
  - 写入配置路径，自动创建目录与按大小滚动；记录 INFO/WARN/ERROR/DEBUG。
  - 性能统计：周期性输出状态变更、处理事件数、故障计数、估算平均周期。

5) 配置说明（conf/config.yaml）
- can
  - iface：CAN 接口名（Linux 示例 can0）。
  - bitrate：总线速率（仅作记录；真正生效由系统 ip link 配置）。
- ids
  - down：主→灯的 CAN ID（11-bit）。
  - up：灯→主的 CAN ID（11-bit）。
- serial
  - panel：面板串口设备名（示例 /dev/ttyS2）。
  - baud：波特率。
- timing（默认时序）
  - g/y/ar：绿/黄/全红时长（毫秒）。
- mapping（通道映射）
  - ns_r/ns_y/ns_g/ew_r/ew_y/ew_g：六路灯通道编号。
- strategy（可选）
  - active_profile：激活的命名策略名。
  - profiles：[{name, timing}] 列表，按 name 精确匹配后使用对应 timing。
- log
  - file：日志文件路径。
  - max_bytes：滚动阈值（超过则 .bak 轮转）。

6) 构建与运行
- Linux（真实 CAN）
  - 系统准备：ip link 设置 CAN 接口（示例：can0，bitrate=250000）。
  - 配置 conf/config.yaml 中的 iface、ids 与 mapping 等。
  - 构建与运行：
    - cargo build --release
    - sudo ./target/release/sigctl
  - 观察日志文件（log.file 路径），确认“主控制器启动/定时控制模块启动/状态转换/心跳/灯状态”等日志正常。
- 非 Linux（本机开发）
  - 无需配置 CAN 驱动；程序自动启用 Mock CAN。
  - cargo run 即可，日志会看到“Mock CAN opened…”、“注入 CanFault/LampFault”、“Version/BoardStatus/LampStatus”等模拟事件，验证控制逻辑与降级流程。
- 示例程序
  - 运行 CAN 通讯示例：cargo run --example can_demo
  - 该示例仅演示 CAN 接口的初始化、发送、回调设置与简单的性能测试，便于理解与二次封装（不影响主程序）。

7) 使用方法（现场与台架）
- 修改 conf/config.yaml，确保以下项符合现场：
  - can.iface、ids.down/up、mapping 六路灯通道。
  - timing 或 strategy.active_profile/profiles。
  - serial.panel/baud（如需面板）。
  - log.file（建议位于持久化分区）。
- 启动前检查（Linux）：
  - CAN：ip link set can0 up type can bitrate 250000 restart-ms 100（示例）
  - 权限：运行程序需要访问 CAN 与串口（可能需要 root 或 udev 规则）。
- 启动程序并观察日志：
  - 首次应看到“定时控制模块启动：NS绿…ms，黄…ms，全红…ms”与“状态转换 … → …”。
  - 非 Linux 会看到“Mock CAN…”与版本/板状态/电压等周期事件。
- 故障与恢复：
  - 故障（CAN/灯）触发黄闪，日志中出现“触发降级：进入黄闪”、“降级：发送 FailFlash 指令”。
  - 恢复后回到 AllRed 再继续配时，日志提示恢复。

8) 测试方法与建议
- 单元测试（协议打包与解析）
  - cargo test
  - 覆盖 OutMsg 打包（Heartbeat/SetLamp/Scheme…）与 Up 帧解析（BoardSt/LampSt/LampFault/CanFault/Voltage/Version）。
- 本机联调（非 Linux）
  - 直接 cargo run，利用 Mock CAN 的周期事件与注入故障，验证状态机与降级流程；关注日志的状态转换、故障、恢复。
- Linux 台架/现场联调
  - 配置 CAN 并启动程序；使用真实灯控板连线。
  - 重点检查：心跳、方案/点控是否生效、失联自动黄闪、恢复回滚到 AllRed、灯状态与故障上报。
- 示例程序验证
  - cargo run --example can_demo，观察初始化、发送、回调设置与性能输出，快速确认开发环境的 CAN 基础接口行为（Linux 为 SocketCAN，其他平台为模拟）。
- 脚本参考
  - <mcfile name="test_quick.sh" path="/Users/xnpeng/sumoptis/sigctl/test_quick.sh"></mcfile>：快速编译、测试与短时运行参考（请以 conf/config.yaml 为准调整项）。
  - <mcfile name="test_e2e.sh" path="/Users/xnpeng/sumoptis/sigctl/test_e2e.sh"></mcfile>：端到端测试脚本（字段命名与当前配置结构可能存在差异，作为思路参考，建议以 conf/config.yaml 为准进行调整后使用）。

9) 常见问题与排查
- 无 CAN 收发（Linux）
  - 检查 can0 是否 up 且速率/终端匹配；确认 ids.down/up 与现场一致；查看日志是否存在 “CAN write/read error”。
- 非 Linux 无外设
  - 属于正常：系统在 Mock 模式运行，用于开发调试；通过日志观测模拟事件。
- 状态机不切换
  - 检查 timing 或 strategy.active_profile 对应的 profiles 是否配置正确；查看日志中是否有降级进入黄闪；确认事件循环未被大量错误淹没。
- 黄闪不恢复
  - 确认上游已恢复并有“CanFault state=0”等事件；现场可手动下发 Resume（若上层策略允许）。
- 日志文件无输出
  - 检查 log.file 路径与目录权限；log.max_bytes 是否过小导致频繁滚动。
- 面板串口无数据
  - 目前为骨架，读写通路正常情况下仅打印收发长度；扩展 0xC*/0xD* 协议时请在此模块内解析与上报。

10) 开发与扩展建议（保持复用与无缝集成）
- 新策略与日计划：建议在 <mcfile name="modules.rs" path="/Users/xnpeng/sumoptis/sigctl/src/modules.rs"></mcfile> 内新增独立模块并与主控解耦，通过统一接口对接，重用现有 CAN I/O 与日志；通过 <mcfile name="types.rs" path="/Users/xnpeng/sumoptis/sigctl/src/types.rs"></mcfile> 的 Strategy/profiles 进行配置入口扩展，保持“全局唯一、全局一致，不重复创建”。
- 面板协议扩展：在 <mcfile name="ui_panel.rs" path="/Users/xnpeng/sumoptis/sigctl/src/ui_panel.rs"></mcfile> 中完成 0xC*/0xD* 编解码与事件上报；不要改变现有主控与 CAN I/O 交互。
- 协议演进：如需新增 0xA*/0xB* 指令与事件，先在 <mcfile name="types.rs" path="/Users/xnpeng/sumoptis/sigctl/src/types.rs"></mcfile> 扩展枚举与 OutMsg/IoEvent，再在 <mcfile name="io_can.rs" path="/Users/xnpeng/sumoptis/sigctl/src/io_can.rs"></mcfile> 实现打包/解析与测试，确保兼容现有逻辑。

11) 快速开始（摘要）
- Linux 设备：
  - ip link set can0 up type can bitrate 250000 restart-ms 100
  - 编辑 <mcfile name="config.yaml" path="/Users/xnpeng/sumoptis/sigctl/conf/config.yaml"></mcfile>，设置 iface/ids/mapping/timing/log 等
  - cargo build --release
  - sudo ./target/release/sigctl
- 非 Linux（开发机）：
  - cargo run（自动启用 Mock CAN）
  - 观察日志中的版本、板状态、灯状态、故障注入等，核对状态机行为
- 示例：
  - cargo run --example can_demo（快速了解 CAN API 行为）

如需我把上述内容整理到一个项目内的使用手册文件中（例如 docs 或 README 的章节），告诉我你希望的文件名与放置位置，我可以在不破坏原有程序的前提下无缝集成。
        