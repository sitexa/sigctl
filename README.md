# 交通信号机控制系统

在嵌入式 Linux 主控板（EM928x）+ 灯控板（驱动板）上，实现可投入现场联调的最小可用系统（MVP）：双相位固定配时、CAN 通讯、失联安全降级、面板交互与运维自启；并以内部协议（0xA\*/0xB\*/0xC\*/0xD\*）为标准对接。

# sigctl (skeleton)

Quick start:
```bash
# On the device (ensure can0 is up: ip link set can0 up type can bitrate 250000 restart-ms 100)
cd sigctl
cargo build --release
sudo ./target/release/sigctl
```

# 设计方案

[设计方案](doc/PLAN.md)

# 配置要点（conf/config.yaml）

- can.iface: can0（确保系统已 ip link set can0 up type can bitrate 250000）
- ids.down/up: CAN 11-bit ID（默认 0x100/0x180；若现场已定，改成现场值）
- serial.panel: COM5 的设备名（示例 /dev/ttyS2，按主板实际映射改）
- timing: 绿/黄/全红（毫秒）
- mapping: 六路灯通道编号（按你的驱动板通道表改）
- log: 日志文件路径与最大字节数

# 快速运行（在设备上）

```
# 1) 先把 can0 拉起来（示例 250kbps）
sudo ip link set can0 up type can bitrate 250000 restart-ms 100

# 2) 解压并进入项目
unzip sigctl_skeleton_v1.zip -d ~
cd ~/sigctl

# 3) 构建 + 运行（或用 build.sh 复制到 /mnt/nandflash）
cargo build --release
sudo ./target/release/sigctl
```

# 做了什么（骨架逻辑）

- io_can：阻塞线程读写 can0；上行解析 0xB1/0xB2/0xB3/0xB4/0xB6/0xB8，下行封装 0xAB/0xAA/0xA0/0xAD/0xAE/0xAF/0xA7。
- core：固定配时状态机；每 100ms 发送心跳；按 mapping 以点控帧 0xAA 切灯色（先 MVP，后续可切换到 0xA0 按阶段批量下发）。
- ui_panel：已打开串口并留收/发通道；后续把 0xC*/0xD* 的帧结构补进去即可。
- log：简单文件日志，超过阈值自动 .bak 轮转。

# 建议下一步

1. 把 mapping 改成现场通道号，先做台架灯板点控联调：
    - candump can0 应能看到每 100ms 的 0xAB，以及 0xAA 切灯帧。
2. 在 ui_panel.rs 填入你们 0xC1/0xC2/0xC4/0xC6 的具体格式（我可以帮你补完整帧结构和打包/解析）。
3. 如需方案下发，在 core 中改用 OutMsg::Scheme{ id, bitmap, green_s } 生成 0xA0 帧即可。
4. 把 build.sh 的复制路径替换为 EM928x 的实际自启路径（例如 /mnt/nandflash/sigctl），再配合 userinfo.txt 完成一键自启。