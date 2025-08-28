use crate::types::Config;
use anyhow::Result;
use std::{io::{Read, Write}, sync::Arc, time::Duration};
use tokio::sync::mpsc;
use tokio::task;

/// 面板通信（COM5）：最小读写骨架（可按协议 0xC*/0xD* 扩展）
pub struct PanelHandle {
    pub tx: mpsc::Sender<Vec<u8>>,   // 发送帧（原始）
}

pub fn spawn(cfg: Arc<Config>) -> Result<PanelHandle> {
    let (tx, mut rx) = mpsc::channel::<Vec<u8>>(64);
    let dev = cfg.serial.panel.clone();
    let baud = cfg.serial.baud;
    task::spawn_blocking(move || -> Result<()> {
        let mut port = serialport::new(dev, baud)
            .timeout(Duration::from_millis(50))
            .open()?;
        crate::log::info("Panel serial opened");
        let mut buf = [0u8; 256];
        loop {
            // 读
            match port.read(&mut buf) {
                Ok(n) if n>0 => {
                    // TODO: 解析 0xD* 帧并回调上层
                    crate::log::info(format!("panel rx {} bytes", n));
                }
                _ => {}
            }
            // 写
            if let Ok(frame) = rx.try_recv() {
                if let Err(e) = port.write_all(&frame) {
                    crate::log::warn(format!("panel tx error: {}", e));
                }
            }
        }
    });
    Ok(PanelHandle{ tx })
}