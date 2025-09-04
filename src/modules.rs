//! 模块系统：包含定时控制模块（TimingControl）

use anyhow::Result;
use std::sync::Arc;
use tokio::{select, sync::mpsc, time::Duration};

use crate::types::{Config, IoEvent, OutMsg, Timing};

pub mod timing {
    use super::*;
    use std::time::Instant;

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

    #[derive(Debug, Default, Clone, Copy)]
    struct DegradeCtrl {
        // 是否已进入降级（黄闪）
        in_fail_flash: bool,
        // 最近一次 CAN 故障/恢复时间
        last_can_fault: Option<Instant>,
        last_can_ok: Option<Instant>,
        // 最近一次灯故障时间
        last_lamp_fault: Option<Instant>,
    }

    pub(super) fn select_timing(cfg: &Config) -> Timing {
        if let Some(ref name) = cfg.strategy.active_profile {
            if let Some(p) = cfg.strategy.profiles.iter().find(|p| &p.name == name) {
                crate::log::info(&format!(
                    "采用策略: profile='{}' (g={}ms,y={}ms,ar={}ms)",
                    name,
                    p.timing.g().as_millis(),
                    p.timing.y().as_millis(),
                    p.timing.ar().as_millis()
                ));
                return p.timing.clone();
            } else {
                crate::log::warn(&format!("未找到指定的 profile='{}'，回退到默认 timing", name));
            }
        }
        cfg.timing.clone()
    }

    /// 定时控制模块：实现固定配时的信号控制
    pub struct TimingController;

    impl TimingController {
        /// 运行定时控制状态机（不包含心跳任务，心跳由主控制器负责）
        pub async fn run(cfg: Arc<Config>, tx_can: mpsc::Sender<OutMsg>, mut rx_evt: mpsc::Receiver<IoEvent>) -> Result<()> {
            let mut current_state = TrafficState::AllRed;
            let mut metrics = StateMetrics::default();
            let mut degrade = DegradeCtrl::default();
            let start_time = Instant::now();

            // 选择时序（支持命名策略，默认回退原 timing）
            let t = select_timing(&cfg);
            crate::log::info(&format!(
                "定时控制模块启动：NS绿{}ms，黄{}ms，全红{}ms",
                t.g().as_millis(),
                t.y().as_millis(),
                t.ar().as_millis()
            ));

            // 初始全红
            transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed);
            all_red(&cfg, &tx_can).await?;
            sleepy_or_events(t.ar(), &mut rx_evt, &mut metrics, &mut degrade).await;
            if degrade.in_fail_flash {
                fail_flash_loop(&cfg, &tx_can, &mut rx_evt, &mut metrics, &mut degrade).await?;
                // 恢复后，重新全红稳态
                transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed);
            }

            loop {
                // NS 通行
                transition_to_state(&mut current_state, &mut metrics, TrafficState::NsGreen);
                set_ns(&cfg, &tx_can, 2).await?; // NS置绿 (state=2)
                sleepy_or_events(t.g(), &mut rx_evt, &mut metrics, &mut degrade).await;
                if degrade.in_fail_flash { fail_flash_loop(&cfg, &tx_can, &mut rx_evt, &mut metrics, &mut degrade).await?; transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed); continue; }

                transition_to_state(&mut current_state, &mut metrics, TrafficState::NsYellow);
                set_ns(&cfg, &tx_can, 1).await?; // NS黄
                sleepy_or_events(t.y(), &mut rx_evt, &mut metrics, &mut degrade).await;
                if degrade.in_fail_flash { fail_flash_loop(&cfg, &tx_can, &mut rx_evt, &mut metrics, &mut degrade).await?; transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed); continue; }

                transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed);
                all_red(&cfg, &tx_can).await?;
                sleepy_or_events(t.ar(), &mut rx_evt, &mut metrics, &mut degrade).await;
                if degrade.in_fail_flash { fail_flash_loop(&cfg, &tx_can, &mut rx_evt, &mut metrics, &mut degrade).await?; transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed); continue; }

                // EW 通行
                transition_to_state(&mut current_state, &mut metrics, TrafficState::EwGreen);
                set_ew(&cfg, &tx_can, 2).await?; // EW置绿
                sleepy_or_events(t.g(), &mut rx_evt, &mut metrics, &mut degrade).await;
                if degrade.in_fail_flash { fail_flash_loop(&cfg, &tx_can, &mut rx_evt, &mut metrics, &mut degrade).await?; transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed); continue; }

                transition_to_state(&mut current_state, &mut metrics, TrafficState::EwYellow);
                set_ew(&cfg, &tx_can, 1).await?; // EW黄
                sleepy_or_events(t.y(), &mut rx_evt, &mut metrics, &mut degrade).await;
                if degrade.in_fail_flash { fail_flash_loop(&cfg, &tx_can, &mut rx_evt, &mut metrics, &mut degrade).await?; transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed); continue; }

                transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed);
                all_red(&cfg, &tx_can).await?;
                sleepy_or_events(t.ar(), &mut rx_evt, &mut metrics, &mut degrade).await;
                if degrade.in_fail_flash { fail_flash_loop(&cfg, &tx_can, &mut rx_evt, &mut metrics, &mut degrade).await?; transition_to_state(&mut current_state, &mut metrics, TrafficState::AllRed); continue; }

                // 定期输出性能统计
                if metrics.state_changes % 20 == 0 {
                    log_performance_metrics(&metrics, start_time);
                }
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

    async fn sleepy_or_events(d: Duration, rx_evt: &mut mpsc::Receiver<IoEvent>, metrics: &mut StateMetrics, degrade: &mut DegradeCtrl) {
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
                                if state != 0 { // 非0为故障
                                    degrade.in_fail_flash = true;
                                    degrade.last_can_fault = Some(Instant::now());
                                    crate::log::warn("触发降级：进入黄闪");
                                    break;
                                } else {
                                    degrade.last_can_ok = Some(Instant::now());
                                }
                            },
                            IoEvent::LampFault{flag, ch, kind} => {
                                metrics.lamp_faults += 1;
                                crate::log::warn(&format!("灯故障事件: flag={}, ch={}, kind={}", flag, ch, kind));
                                degrade.in_fail_flash = true;
                                degrade.last_lamp_fault = Some(Instant::now());
                                crate::log::warn("触发降级：进入黄闪");
                                break;
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

    async fn fail_flash_loop(cfg: &Config, tx: &mpsc::Sender<OutMsg>, rx_evt: &mut mpsc::Receiver<IoEvent>, metrics: &mut StateMetrics, degrade: &mut DegradeCtrl) -> Result<()> {
        // 进入黄闪模式
        crate::log::warn("降级：发送 FailFlash 指令，进入黄闪模式");
        tx.send(OutMsg::FailFlash).await?;
        let mut reassert = tokio::time::interval(Duration::from_secs(5));
        // 稳定判据：收到 CAN 恢复（state==0）且 2 秒内无新的灯故障
        let stable_window = Duration::from_secs(2);
        use std::time::Instant as StdInstant; // 与 tokio::time::Instant 区分
        loop {
            select! {
                _ = reassert.tick() => {
                    // 周期性重申黄闪，防止意外退出
                    tx.send(OutMsg::FailFlash).await.ok();
                }
                maybe = rx_evt.recv() => {
                    if let Some(ev) = maybe {
                        metrics.events_processed += 1;
                        match ev {
                            IoEvent::CanFault{state, where_} => {
                                if state != 0 {
                                    degrade.last_can_fault = Some(StdInstant::now());
                                    crate::log::warn(&format!("黄闪期间：CAN仍故障 state={}, where={}", state, where_));
                                } else {
                                    degrade.last_can_ok = Some(StdInstant::now());
                                    crate::log::info("黄闪期间：检测到CAN恢复");
                                }
                            }
                            IoEvent::LampFault{flag, ch, kind} => {
                                degrade.last_lamp_fault = Some(StdInstant::now());
                                crate::log::warn(&format!("黄闪期间：灯故障持续 flag={}, ch={}, kind={}", flag, ch, kind));
                            }
                            IoEvent::VoltageMv{mv} => {
                                crate::log::debug(&format!("电压: {}mV", mv));
                            }
                            other => {
                                crate::log::debug(&format!("黄闪期间事件: {:?}", other));
                            }
                        }
                    } else {
                        // 事件通道关闭，直接退出（交由上层重启）
                        break;
                    }
                }
            }

            // 检查稳定判据
            let can_ok = degrade.last_can_ok.is_some() && degrade.last_can_fault.map_or(true, |t_fault| {
                // 上一次故障早于上一次恢复，且恢复已持续 stable_window
                let t_ok = degrade.last_can_ok.unwrap();
                t_ok > t_fault && t_ok.elapsed() >= stable_window
            });
            let lamp_ok = degrade.last_lamp_fault.map_or(true, |t| t.elapsed() >= stable_window);

            if can_ok && lamp_ok {
                crate::log::info("条件满足：退出黄闪，准备恢复正常控制");
                tx.send(OutMsg::Resume).await?;
                // 恢复后先全红稳态
                all_red(cfg, tx).await?;
                // 使用所选时序的全红时间
                let t = select_timing(cfg);
                tokio::time::sleep(t.ar()).await;
                degrade.in_fail_flash = false;
                break;
            }
        }
        Ok(())
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
        for ch in [
            cfg.mapping.ns_r, cfg.mapping.ns_y, cfg.mapping.ns_g,
            cfg.mapping.ew_r, cfg.mapping.ew_y, cfg.mapping.ew_g
        ] {
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
}


    #[cfg(test)]
    mod tests {
        use super::*;
        use crate::types::{CanCfg, Ids, SerialCfg, Mapping, Strategy, NamedTiming, LogCfg};
        use super::timing::select_timing;

        fn base_cfg() -> Config {
            Config {
                can: CanCfg { iface: "vcan0".into(), bitrate: 500_000 },
                ids: Ids { down: 0x100, up: 0x101 },
                serial: SerialCfg { panel: "/dev/ttyS0".into(), baud: 115200 },
                timing: Timing { g: 1000, y: 300, ar: 500 },
                mapping: Mapping { ns_r:1, ns_y:2, ns_g:3, ew_r:4, ew_y:5, ew_g:6 },
                strategy: Strategy::default(),
                log: LogCfg { file: "/tmp/sigctl.test.log".into(), max_bytes: 1_000_000 },
            }
        }

        #[test]
        fn test_select_timing_fallback() {
            let cfg = base_cfg();
            let t = select_timing(&cfg);
            assert_eq!(t.g(), cfg.timing.g());
            assert_eq!(t.y(), cfg.timing.y());
            assert_eq!(t.ar(), cfg.timing.ar());
        }

        #[test]
        fn test_select_timing_profile_match() {
            let mut cfg = base_cfg();
            cfg.strategy.active_profile = Some("p1".into());
            cfg.strategy.profiles.push(NamedTiming{ name: "p1".into(), timing: Timing{ g: 2222, y: 444, ar: 666 }});
            let t = select_timing(&cfg);
            assert_eq!(t.g().as_millis(), 2222);
            assert_eq!(t.y().as_millis(), 444);
            assert_eq!(t.ar().as_millis(), 666);
        }
    }