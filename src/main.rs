mod types;
mod ui_panel;
mod core;
mod log;
mod can_io;
mod modules;

use crate::log as logx;

use crate::types::Config;
use anyhow::Result;
use std::{fs, sync::Arc};

#[tokio::main]
async fn main() -> Result<()> {
    // 读取配置
    let cfg: Config = serde_yaml::from_str(&fs::read_to_string("conf/config.yaml")?)?;
    logx::init(&cfg.log.file, cfg.log.max_bytes);
    logx::info("sigctl starting");

    let cfg = Arc::new(cfg);

    // 启动 CAN I/O (使用统一CAN模块)
    let (tx_can, rx_evt) = can_io::spawn(cfg.clone());

    // 启动面板串口（可选）
    if let Err(e) = ui_panel::spawn(cfg.clone()) {
        logx::warn(format!("panel spawn failed: {}", e));
    }

    // 启动控制核心
    if let Err(e) = core::run(cfg.clone(), tx_can.clone(), rx_evt).await {
        logx::error(format!("core exited with error: {}", e));
    }

    Ok(())
}