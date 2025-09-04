use crate::types::{Config, Dn, IoEvent, OutMsg, TAIL, Up};
#[cfg(target_os = "linux")]
use anyhow::Result;
#[cfg(target_os = "linux")]
use socketcan::{CanFrame, CanSocket, Socket, EmbeddedFrame};
use std::sync::Arc;
use tokio::sync::mpsc;
use tokio::task;

/// 启动 CAN I/O 子系统：返回 (发送句柄, 事件接收端)
pub fn spawn(cfg: Arc<Config>) -> (mpsc::Sender<OutMsg>, mpsc::Receiver<IoEvent>) {
    let (tx_out, rx_out) = mpsc::channel::<OutMsg>(128);
    let (tx_evt, rx_evt) = mpsc::channel::<IoEvent>(128);

    #[cfg(target_os = "linux")]
    {
        // Linux 实现：使用真实的 CAN 接口
        spawn_linux_can(cfg, rx_out, tx_evt);
    }

    #[cfg(not(target_os = "linux"))]
    {
        // 非 Linux 实现：模拟 CAN 接口
        spawn_mock_can(cfg, rx_out, tx_evt);
    }

    (tx_out, rx_evt)
}

#[cfg(target_os = "linux")]
fn spawn_linux_can(
    cfg: Arc<Config>,
    mut rx_out: mpsc::Receiver<OutMsg>,
    tx_evt: mpsc::Sender<IoEvent>,
) {
    // 发送线程（阻塞式）
    {
        let cfg = cfg.clone();
        let tx_evt = tx_evt.clone();
        task::spawn_blocking(move || -> Result<()> {
            let sock = CanSocket::open(&cfg.can.iface)?;
            crate::log::info(format!("CAN opened on {}", &cfg.can.iface));
            while let Some(msg) = rx_out.blocking_recv() {
                let payload = pack_dn(&msg);
                let frame = CanFrame::new(cfg.ids.down, &payload)?;
                if let Err(e) = sock.write_frame(&frame) {
                    let _ = tx_evt.blocking_send(IoEvent::Raw(vec![0xEE, 0xEE])); // 写错误标记
                    crate::log::error(format!("CAN write error: {}", e));
                }
            }
            Ok(())
        });
    }

    // 接收线程（阻塞式）
    {
        let cfg = cfg.clone();
        task::spawn_blocking(move || -> Result<()> {
            let sock = CanSocket::open(&cfg.can.iface)?;
            loop {
                match sock.read_frame() {
                    Ok(f) => {
                        if f.id() != cfg.ids.up { continue; }
                        let data = f.data().to_vec();
                        if let Some(evt) = parse_up(&data) {
                            let _ = tx_evt.blocking_send(evt);
                        } else {
                            let _ = tx_evt.blocking_send(IoEvent::Raw(data));
                        }
                    }
                    Err(e) => {
                        crate::log::warn(format!("CAN read error: {}", e));
                    }
                }
            }
        });
    }
}

#[cfg(not(target_os = "linux"))]
fn spawn_mock_can(
    cfg: Arc<Config>,
    mut rx_out: mpsc::Receiver<OutMsg>,
    tx_evt: mpsc::Sender<IoEvent>,
) {
    // 模拟发送线程
    {
        let cfg = cfg.clone();
        let _tx_evt = tx_evt.clone();
        task::spawn(async move {
            crate::log::info(format!("Mock CAN opened on {} (simulated)", &cfg.can.iface));
            while let Some(msg) = rx_out.recv().await {
                let payload = pack_dn(&msg);
                crate::log::debug(format!("Mock CAN send: {:?} -> {:02X?}", msg, payload));
                // 模拟发送成功，不产生错误事件
            }
        });
    }

    // 模拟接收线程 - 定期发送多种模拟事件
    {
        task::spawn(async move {
            use std::time::Duration;
            
            // 启动时发送版本信息
            tokio::time::sleep(Duration::from_millis(500)).await;
            let version_event = IoEvent::Version { 
                board: 1, y: 24, m: 1, d: 15, v1: 1, v2: 0 
            };
            let _ = tx_evt.send(version_event).await;
            crate::log::debug("Mock CAN: sent version info");
            
            // 定期发送驱动板状态（每5秒）
            let mut board_interval = tokio::time::interval(Duration::from_secs(5));
            let mut lamp_interval = tokio::time::interval(Duration::from_secs(3));
            let mut voltage_interval = tokio::time::interval(Duration::from_secs(10));
            // 新增：周期性注入故障与恢复，用于本地联调降级策略
            let mut can_fault_interval = tokio::time::interval(Duration::from_secs(15));
            let mut lamp_fault_interval = tokio::time::interval(Duration::from_secs(25));
            let mut can_fault_active = false;
            
            let mut board_state = 2u8; // 正常状态
            let mut lamp_state_counter = 0u16;
            
            loop {
                tokio::select! {
                    _ = board_interval.tick() => {
                        let board_event = IoEvent::BoardStatus { board: 1, state: board_state };
                        if tx_evt.send(board_event).await.is_err() { break; }
                        crate::log::debug(format!("Mock CAN: sent board status {}", board_state));
                    }
                    
                    _ = lamp_interval.tick() => {
                        // 模拟灯组状态变化（简单的循环模式）
                        lamp_state_counter = (lamp_state_counter + 1) % 4;
                        let (yg_bits, r_bits) = match lamp_state_counter {
                            0 => (0x0001, 0x0010), // NS绿，EW红
                            1 => (0x0002, 0x0010), // NS黄，EW红
                            2 => (0x0000, 0x0011), // 全红
                            3 => (0x0008, 0x0004), // EW绿，NS红
                            _ => (0x0000, 0x0015), // 默认全红
                        };
                        
                        let lamp_event = IoEvent::LampStatus { 
                            board: 1, yg_bits, r_bits 
                        };
                        if tx_evt.send(lamp_event).await.is_err() { break; }
                        crate::log::debug(format!("Mock CAN: sent lamp status YG:{:04X} R:{:04X}", yg_bits, r_bits));
                    }
                    
                    _ = voltage_interval.tick() => {
                        // 模拟电压信息（220V ± 10V）
                        use std::time::{SystemTime, UNIX_EPOCH};
                        let seed = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs() as u16;
                        let voltage_mv = 22000 + (seed % 2000) - 1000;
                        let voltage_event = IoEvent::VoltageMv { mv: voltage_mv };
                        if tx_evt.send(voltage_event).await.is_err() { break; }
                        crate::log::debug(format!("Mock CAN: sent voltage {}mV", voltage_mv));
                    }

                    // 新增：模拟 CAN 故障 -> 恢复
                    _ = can_fault_interval.tick() => {
                        if !can_fault_active {
                            can_fault_active = true;
                            let _ = tx_evt.send(IoEvent::CanFault { state: 1, where_: 1 }).await; // 故障
                            crate::log::warn("Mock CAN: inject CanFault state=1");
                            // 3秒后恢复
                            let tx_evt2 = tx_evt.clone();
                            task::spawn(async move {
                                tokio::time::sleep(Duration::from_secs(3)).await;
                                let _ = tx_evt2.send(IoEvent::CanFault { state: 0, where_: 1 }).await;
                                crate::log::info("Mock CAN: inject CanFault state=0 (recovered)");
                            });
                        }
                    }

                    // 新增：模拟灯故障一次性事件
                    _ = lamp_fault_interval.tick() => {
                        let _ = tx_evt.send(IoEvent::LampFault { flag: 1, ch: 2, kind: 1 }).await;
                        crate::log::warn("Mock CAN: inject LampFault ch=2 kind=1");
                    }
                }
            }
        });
    }
}

fn pack_dn(m: &OutMsg) -> Vec<u8> {
    match *m {
        OutMsg::Heartbeat => vec![Dn::Heartbeat as u8, 0xAB, TAIL],
        OutMsg::SetLamp { ch, state } => vec![Dn::Lamp as u8, ch, state, TAIL],
        OutMsg::Scheme { id, bitmap, green_s } => {
            let hi = ((bitmap >> 8) & 0xFF) as u8;
            let lo = (bitmap & 0xFF) as u8;
            vec![Dn::Scheme as u8, id, hi, lo, green_s, TAIL]
        }
        OutMsg::FailFlash => vec![Dn::FailFlash as u8, 0xAD, TAIL],
        OutMsg::Resume => vec![Dn::Resume as u8, 0xAE, TAIL],
        OutMsg::DefaultGreen { ns, ew } => vec![Dn::DefaultG as u8, ns, ew, TAIL],
        OutMsg::FailMode { mode } => vec![Dn::FailMode as u8, mode, TAIL],
    }
}

fn parse_up(d: &[u8]) -> Option<IoEvent> {
    if d.len() < 2 || *d.last()? != TAIL { return None; }
    match d[0] {
        x if x == Up::BoardSt as u8 => {
            if d.len() >= 4 { Some(IoEvent::BoardStatus { board: d[1], state: d[2] }) } else { None }
        }
        x if x == Up::LampSt as u8 => {
            if d.len() >= 6 {
                let yg = u16::from_be_bytes([d[2], d[3]]);
                let r  = u16::from_be_bytes([d[4], d[5]]);
                Some(IoEvent::LampStatus{ board: d[1], yg_bits: yg, r_bits: r })
            } else { None }
        }
        x if x == Up::LampFault as u8 => {
            if d.len() >= 5 { Some(IoEvent::LampFault{ flag: d[1], ch: d[2], kind: d[3] }) } else { None }
        }
        x if x == Up::CanFault as u8 => {
            if d.len() >= 4 { Some(IoEvent::CanFault{ state: d[1], where_: d[2] }) } else { None }
        }
        x if x == Up::Voltage as u8 => {
            if d.len() >= 5 {
                let mv = u16::from_be_bytes([d[2], d[3]]);
                Some(IoEvent::VoltageMv{ mv })
            } else { None }
        }
        x if x == Up::Version as u8 => {
            if d.len() >= 8 {
                Some(IoEvent::Version{ board:d[1], y:d[2], m:d[3], d:d[4], v1:d[5], v2:d[6] })
            } else { None }
        }
        _ => None
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::{OutMsg, IoEvent};

    #[test]
    fn test_pack_heartbeat() {
        let msg = OutMsg::Heartbeat;
        let payload = pack_dn(&msg);
        assert_eq!(payload, vec![0xAB, 0xAB, 0xED]);
    }

    #[test]
    fn test_pack_set_lamp() {
        let msg = OutMsg::SetLamp { ch: 1, state: 2 }; // 通道1置绿
        let payload = pack_dn(&msg);
        assert_eq!(payload, vec![0xAA, 0x01, 0x02, 0xED]);
    }

    #[test]
    fn test_pack_scheme() {
        let msg = OutMsg::Scheme { id: 1, bitmap: 0x0105, green_s: 20 };
        let payload = pack_dn(&msg);
        assert_eq!(payload, vec![0xA0, 0x01, 0x01, 0x05, 0x14, 0xED]);
    }

    #[test]
    fn test_pack_fail_flash() {
        let msg = OutMsg::FailFlash;
        let payload = pack_dn(&msg);
        assert_eq!(payload, vec![0xAD, 0xAD, 0xED]);
    }

    #[test]
    fn test_pack_resume() {
        let msg = OutMsg::Resume;
        let payload = pack_dn(&msg);
        assert_eq!(payload, vec![0xAE, 0xAE, 0xED]);
    }

    #[test]
    fn test_pack_default_green() {
        let msg = OutMsg::DefaultGreen { ns: 20, ew: 25 };
        let payload = pack_dn(&msg);
        assert_eq!(payload, vec![0xAF, 0x14, 0x19, 0xED]);
    }

    #[test]
    fn test_pack_fail_mode() {
        let msg = OutMsg::FailMode { mode: 0x02 };
        let payload = pack_dn(&msg);
        assert_eq!(payload, vec![0xA7, 0x02, 0xED]);
    }

    #[test]
    fn test_parse_board_status() {
        let data = vec![0xB1, 0x01, 0x02, 0xED];
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::BoardStatus { board, state } => {
                assert_eq!(board, 1);
                assert_eq!(state, 2);
            }
            _ => panic!("解析错误：期望BoardStatus"),
        }
    }

    #[test]
    fn test_parse_lamp_status() {
        let data = vec![0xB2, 0x01, 0x00, 0x05, 0x00, 0x10, 0xED];
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::LampStatus { board, yg_bits, r_bits } => {
                assert_eq!(board, 1);
                assert_eq!(yg_bits, 0x0005);
                assert_eq!(r_bits, 0x0010);
            }
            _ => panic!("解析错误：期望LampStatus"),
        }
    }

    #[test]
    fn test_parse_lamp_fault() {
        let data = vec![0xB3, 0x01, 0x03, 0x02, 0xED];
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::LampFault { flag, ch, kind } => {
                assert_eq!(flag, 1);
                assert_eq!(ch, 3);
                assert_eq!(kind, 2);
            }
            _ => panic!("解析错误：期望LampFault"),
        }
    }

    #[test]
    fn test_parse_can_fault() {
        let data = vec![0xB4, 0x01, 0x00, 0xED];
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::CanFault { state, where_ } => {
                assert_eq!(state, 1);
                assert_eq!(where_, 0);
            }
            _ => panic!("解析错误：期望CanFault"),
        }
    }

    #[test]
    fn test_parse_voltage() {
        let data = vec![0xB6, 0x01, 0x55, 0xF0, 0xED]; // 22000mV
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::VoltageMv { mv } => {
                assert_eq!(mv, 22000);
            }
            _ => panic!("解析错误：期望VoltageMv"),
        }
    }

    #[test]
    fn test_parse_version() {
        let data = vec![0xB8, 0x01, 0x18, 0x01, 0x0F, 0x01, 0x00, 0xED]; // 2024-01-15 v1.0
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::Version { board, y, m, d, v1, v2 } => {
                assert_eq!(board, 1);
                assert_eq!(y, 24);
                assert_eq!(m, 1);
                assert_eq!(d, 15);
                assert_eq!(v1, 1);
                assert_eq!(v2, 0);
            }
            _ => panic!("解析错误：期望Version"),
        }
    }

    #[test]
    fn test_parse_invalid_data() {
        // 测试无效数据
        assert!(parse_up(&vec![]).is_none());
        assert!(parse_up(&vec![0xB1]).is_none());
        assert!(parse_up(&vec![0xB1, 0x01]).is_none()); // 缺少尾字节
        assert!(parse_up(&vec![0xB1, 0x01, 0x02, 0xEE]).is_none()); // 错误的尾字节
        assert!(parse_up(&vec![0xFF, 0x01, 0x02, 0xED]).is_none()); // 未知命令
    }

    #[test]
    fn test_protocol_roundtrip() {
        // 测试编码-解码往返
        let original_msg = OutMsg::SetLamp { ch: 5, state: 1 };
        let encoded = pack_dn(&original_msg);
        
        // 验证编码结果
        assert_eq!(encoded.len(), 4);
        assert_eq!(encoded[0], 0xAA);
        assert_eq!(encoded[3], 0xED);
        
        // 模拟响应：灯组状态上报
        let response_data = vec![0xB2, 0x01, 0x00, 0x20, 0x00, 0x00, 0xED]; // 通道5黄灯
        let parsed_event = parse_up(&response_data).unwrap();
        
        match parsed_event {
            IoEvent::LampStatus { board: _, yg_bits, r_bits: _ } => {
                assert_eq!(yg_bits & 0x0020, 0x0020); // 检查通道5黄灯位
            }
            _ => panic!("期望LampStatus响应"),
        }
    }
}