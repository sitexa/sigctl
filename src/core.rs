use crate::types::{Config, IoEvent, OutMsg};
use anyhow::Result;
use std::sync::Arc;
use std::time::Instant;
use tokio::{select, sync::mpsc, time::{sleep, Duration}};

#[derive(Debug, Clone, PartialEq)]
enum TrafficState {
    AllRed,
    NsGreen,
    NsYellow,
    EwGreen,
    EwYellow,
}

#[derive(Debug, Default)]
struct StateMetrics {
    state_changes: u64,
    events_processed: u64,
    can_faults: u64,
    lamp_faults: u64,
    last_state_change: Option<Instant>,
}

/// 控制核心：固定配时 -> 输出意图（由 io_can 打包成 0xAA/0xA0）
pub async fn run(cfg: Arc<Config>, tx_can: mpsc::Sender<OutMsg>, mut rx_evt: mpsc::Receiver<IoEvent>) -> Result<()> {
    let mut current_state = TrafficState::AllRed;
    let mut metrics = StateMetrics::default();
    let start_time = Instant::now();
    
    crate::log::info(&format!("控制核心启动，配置：NS绿{}ms，黄{}ms，全红{}ms", 
        cfg.timing.g().as_millis(), cfg.timing.y().as_millis(), cfg.timing.ar().as_millis()));
    // 周期心跳任务
    let hb_tx = tx_can.clone();
    tokio::spawn(async move {
        let mut interval = tokio::time::interval(Duration::from_millis(100));
        loop {
            interval.tick().await;
            let _ = hb_tx.send(OutMsg::Heartbeat).await;
        }
    });

    // 初始全红
    transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed);
    all_red(&cfg, &tx_can).await?;
    sleepy_or_events(cfg.timing.ar(), &mut rx_evt, &mut metrics).await;

    loop {
        // NS 通行
        transition_to_state(&mut current_state, &mut metrics, TrafficState::NsGreen);
        set_ns(&cfg, &tx_can, 2).await?; // NS置绿 (state=2)
        sleepy_or_events(cfg.timing.g(), &mut rx_evt, &mut metrics).await;

        transition_to_state(&mut current_state, &mut metrics, TrafficState::NsYellow);
        set_ns(&cfg, &tx_can, 1).await?; // NS黄
        sleepy_or_events(cfg.timing.y(), &mut rx_evt, &mut metrics).await;

        transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed);
        all_red(&cfg, &tx_can).await?;
        sleepy_or_events(cfg.timing.ar(), &mut rx_evt, &mut metrics).await;

        // EW 通行
        transition_to_state(&mut current_state, &mut metrics, TrafficState::EwGreen);
        set_ew(&cfg, &tx_can, 2).await?; // EW置绿
        sleepy_or_events(cfg.timing.g(), &mut rx_evt, &mut metrics).await;

        transition_to_state(&mut current_state, &mut metrics, TrafficState::EwYellow);
        set_ew(&cfg, &tx_can, 1).await?; // EW黄
        sleepy_or_events(cfg.timing.y(), &mut rx_evt, &mut metrics).await;

        transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed);
        all_red(&cfg, &tx_can).await?;
        sleepy_or_events(cfg.timing.ar(), &mut rx_evt, &mut metrics).await;
        
        // 定期输出性能统计
        if metrics.state_changes % 20 == 0 {
            log_performance_metrics(&metrics, start_time);
        }
    }
}

fn transition_to_state(current: &mut TrafficState, metrics: &mut StateMetrics, new_state: TrafficState) {
    if *current != new_state {
        crate::log::info(&format!("状态转换: {:?} -> {:?}", current, new_state));
        *current = new_state;
        metrics.state_changes += 1;
        metrics.last_state_change = Some(Instant::now());
    }
}

fn log_performance_metrics(metrics: &StateMetrics, start_time: Instant) {
    let uptime = start_time.elapsed();
    let avg_cycle_time = if metrics.state_changes > 0 {
        uptime.as_secs_f64() / (metrics.state_changes as f64 / 5.0) // 每个完整周期5个状态
    } else {
        0.0
    };
    
    crate::log::info(&format!(
        "性能统计 - 运行时间: {:.1}s, 状态变更: {}, 事件处理: {}, CAN故障: {}, 灯故障: {}, 平均周期: {:.1}s",
        uptime.as_secs_f64(),
        metrics.state_changes,
        metrics.events_processed,
        metrics.can_faults,
        metrics.lamp_faults,
        avg_cycle_time
    ));
}

async fn sleepy_or_events(d: Duration, rx_evt: &mut mpsc::Receiver<IoEvent>, metrics: &mut StateMetrics) {
    let t = tokio::time::sleep(d);
    tokio::pin!(t);
    loop {
        select! {
            _ = &mut t => break,
            maybe = rx_evt.recv() => {
                if let Some(ev) = maybe {
                    metrics.events_processed += 1;
                    match ev {
                        IoEvent::CanFault{state, where_} => {
                            metrics.can_faults += 1;
                            crate::log::warn(&format!("CAN故障事件: state={}, where={}", state, where_));
                        },
                        IoEvent::LampFault{flag, ch, kind} => {
                            metrics.lamp_faults += 1;
                            crate::log::warn(&format!("灯故障事件: flag={}, ch={}, kind={}", flag, ch, kind));
                        },
                        IoEvent::BoardStatus{board, state} => {
                            crate::log::debug(&format!("驱动板状态: board={}, state={}", board, state));
                        },
                        IoEvent::LampStatus{board, yg_bits, r_bits} => {
                            crate::log::debug(&format!("灯状态: board={}, yg_bits=0x{:04X}, r_bits=0x{:04X}", board, yg_bits, r_bits));
                        },
                        IoEvent::VoltageMv{mv} => {
                            crate::log::debug(&format!("电压: {}mV", mv));
                            if mv < 20000 || mv > 26000 {
                                crate::log::warn(&format!("电压异常: {}mV (正常范围: 20000-26000mV)", mv));
                            }
                        },
                        IoEvent::Version{board, y, m, d, v1, v2} => {
                            crate::log::info(&format!("驱动板版本: board={}, 20{}-{:02}-{:02} v{}.{}", board, y, m, d, v1, v2));
                        },
                        _ => {
                            crate::log::debug(&format!("其他事件: {:?}", ev));
                        }
                    }
                } else {
                    break;
                }
            }
        }
    }
}

async fn set_ns(cfg: &Config, tx: &mpsc::Sender<OutMsg>, state: u8) -> Result<()> {
    // NS: set R off if green/yellow else on; set EW red on
    // 采用点控 0xAA：按配置的通道号写入
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ew_r, state: 0 }).await?; // EW红 -> 红
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ns_r, state: if state==2 || state==1 {3} else {0} }).await?; // NS红 -> 灭/红
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ns_y, state: if state==1 {1} else {3} }).await?; // NS黄
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ns_g, state: if state==2 {2} else {3} }).await?; // NS绿
    // 另一向全红
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ew_y, state: 3 }).await?;
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ew_g, state: 3 }).await?;
    Ok(())
}

async fn set_ew(cfg: &Config, tx: &mpsc::Sender<OutMsg>, state: u8) -> Result<()> {
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ns_r, state: 0 }).await?; // NS红 -> 红
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ew_r, state: if state==2 || state==1 {3} else {0} }).await?; // EW红
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ew_y, state: if state==1 {1} else {3} }).await?; // EW黄
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ew_g, state: if state==2 {2} else {3} }).await?; // EW绿
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ns_y, state: 3 }).await?;
    tx.send(OutMsg::SetLamp{ ch: cfg.mapping.ns_g, state: 3 }).await?;
    Ok(())
}

async fn all_red(cfg: &Config, tx: &mpsc::Sender<OutMsg>) -> Result<()> {
    for ch in [cfg.mapping.ns_r, cfg.mapping.ns_y, cfg.mapping.ns_g,
               cfg.mapping.ew_r, cfg.mapping.ew_y, cfg.mapping.ew_g] {
        // 红灯置红，其余置灭：为简化，全部置红，再把非红通道灭
        // 红灯：state=0；黄/绿：state=3(灭)
        if ch == cfg.mapping.ns_r || ch == cfg.mapping.ew_r {
            tx.send(OutMsg::SetLamp{ ch, state: 0 }).await?;
        } else {
            tx.send(OutMsg::SetLamp{ ch, state: 3 }).await?;
        }
    }
    Ok(())
}