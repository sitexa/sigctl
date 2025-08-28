use crate::types::{Config, IoEvent, OutMsg};
use anyhow::Result;
use std::sync::Arc;
use tokio::{select, sync::mpsc, time::{sleep, Duration}};

/// 控制核心：固定配时 -> 输出意图（由 io_can 打包成 0xAA/0xA0）
pub async fn run(cfg: Arc<Config>, tx_can: mpsc::Sender<OutMsg>, mut rx_evt: mpsc::Receiver<IoEvent>) -> Result<()> {
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
    all_red(&cfg, &tx_can).await?;
    sleep(cfg.timing.ar()).await;

    loop {
        // NS 通行
        set_ns(&cfg, &tx_can, 2).await?; // NS置绿 (state=2)
        sleepy_or_events(cfg.timing.g(), &mut rx_evt).await;

        set_ns(&cfg, &tx_can, 1).await?; // NS黄
        sleepy_or_events(cfg.timing.y(), &mut rx_evt).await;

        all_red(&cfg, &tx_can).await?;
        sleepy_or_events(cfg.timing.ar(), &mut rx_evt).await;

        // EW 通行
        set_ew(&cfg, &tx_can, 2).await?; // EW置绿
        sleepy_or_events(cfg.timing.g(), &mut rx_evt).await;

        set_ew(&cfg, &tx_can, 1).await?; // EW黄
        sleepy_or_events(cfg.timing.y(), &mut rx_evt).await;

        all_red(&cfg, &tx_can).await?;
        sleepy_or_events(cfg.timing.ar(), &mut rx_evt).await;
    }
}

async fn sleepy_or_events(d: Duration, rx_evt: &mut mpsc::Receiver<IoEvent>) {
    let t = tokio::time::sleep(d);
    tokio::pin!(t);
    loop {
        select! {
            _ = &mut t => break,
            maybe = rx_evt.recv() => {
                if let Some(ev) = maybe {
                    match ev {
                        IoEvent::CanFault{..} => crate::log::warn("CAN fault event"),
                        IoEvent::LampFault{..} => crate::log::warn("Lamp fault event"),
                        _ => {} // 这里可扩展处理
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