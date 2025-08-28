use crate::types::{Config, Dn, IoEvent, OutMsg, TAIL, Up};
#[cfg(target_os = "linux")]
use anyhow::Result;
#[cfg(target_os = "linux")]
use socketcan::{CANFrame, CANSocket};
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
            let sock = CANSocket::open(&cfg.can.iface)?;
            crate::log::info(format!("CAN opened on {}", &cfg.can.iface));
            while let Some(msg) = rx_out.blocking_recv() {
                let payload = pack_dn(&msg);
                let frame = CANFrame::new(cfg.ids.down, &payload, false, false)
                    .map_err(|e| anyhow::anyhow!("frame build: {}", e))?;
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
            let sock = CANSocket::open(&cfg.can.iface)?;
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

    // 模拟接收线程 - 定期发送一些模拟事件
    {
        task::spawn(async move {
            let mut interval = tokio::time::interval(std::time::Duration::from_secs(5));
            loop {
                interval.tick().await;
                // 发送模拟的心跳事件
                let mock_event = IoEvent::BoardStatus { board: 1, state: 0x01 };
                if tx_evt.send(mock_event).await.is_err() {
                    break;
                }
                crate::log::debug("Mock CAN: sent simulated board status");
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