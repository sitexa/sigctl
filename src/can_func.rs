//! 交通信号机CAN通讯功能实现 (Rust版本)
//! 
//! 基于原C程序重写，提供类型安全的异步CAN通讯接口
//! 支持Linux SocketCAN和跨平台模拟模式

use anyhow::{Result, Context};
use std::sync::Arc;
use tokio::sync::{mpsc, Mutex};
use tokio::task::JoinHandle;
use std::collections::HashMap;
use std::time::Duration;

#[cfg(target_os = "linux")]
use socketcan::{CanFrame, CanSocket, Socket, EmbeddedFrame};

/// CAN通讯返回代码
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CanRetCode {
    Success = 0,
    Fail = 1,
}

/// CAN数据接收回调函数类型
pub type CanRecvCallback = Arc<dyn Fn(u32, &[u8]) + Send + Sync>;

/// CAN帧数据结构
#[derive(Debug, Clone)]
pub struct CanFrameData {
    pub can_id: u32,
    pub data: Vec<u8>,
}

/// CAN通讯配置参数
#[derive(Debug, Clone)]
pub struct CanConfig {
    pub interface_name: String,
    pub bitrate: u32,
    pub restart_ms: u32,
}

impl Default for CanConfig {
    fn default() -> Self {
        Self {
            interface_name: "can0".to_string(),
            bitrate: 500000,
            restart_ms: 200,
        }
    }
}

/// CAN通讯功能参数
struct CanParam {
    #[cfg(target_os = "linux")]
    socket: Option<CanSocket>,
    recv_handle: Option<JoinHandle<()>>,
    callback: Option<CanRecvCallback>,
    config: CanConfig,
}

/// CAN通讯功能主结构体
pub struct CanFunc {
    param: Arc<Mutex<CanParam>>,
    tx_channel: Option<mpsc::Sender<CanFrameData>>,
    shutdown_tx: Option<mpsc::Sender<()>>,
}

impl CanFunc {
    /// 创建新的CAN通讯功能实例
    pub fn new() -> Self {
        let param = CanParam {
            #[cfg(target_os = "linux")]
            socket: None,
            recv_handle: None,
            callback: None,
            config: CanConfig::default(),
        };
        
        Self {
            param: Arc::new(Mutex::new(param)),
            tx_channel: None,
            shutdown_tx: None,
        }
    }

    /// CAN通讯初始化
    /// 
    /// # 参数
    /// * `interface_name` - CAN接口名称 (如 "can0")
    /// * `bitrate` - 波特率
    /// 
    /// # 返回值
    /// * `Ok(())` - 初始化成功
    /// * `Err(anyhow::Error)` - 初始化失败
    pub async fn init(&mut self, interface_name: &str, bitrate: u32) -> Result<()> {
        let mut param = self.param.lock().await;
        
        param.config.interface_name = interface_name.to_string();
        param.config.bitrate = bitrate;
        
        #[cfg(target_os = "linux")]
        {
            // 配置CAN接口
            self.configure_can_interface(&param.config).await
                .context("Failed to configure CAN interface")?;
            
            // 创建CAN套接字
            let socket = CanSocket::open(&param.config.interface_name)
                .context("Failed to open CAN socket")?;
            
            param.socket = Some(socket);
            
            crate::log::info(format!("CAN initialized on {} with bitrate {}", 
                interface_name, bitrate));
        }
        
        #[cfg(not(target_os = "linux"))]
        {
            crate::log::info(format!("Mock CAN initialized on {} with bitrate {}", 
                interface_name, bitrate));
        }
        
        // 释放锁，避免与 start_tasks 的可变借用冲突
        drop(param);
        
        // 启动发送和接收任务
        self.start_tasks().await?;
        
        Ok(())
    }

    /// 配置CAN接口 (仅Linux)
    #[cfg(target_os = "linux")]
    async fn configure_can_interface(&self, config: &CanConfig) -> Result<()> {
        use tokio::process::Command;
        
        // 关闭接口
        let output = Command::new("ip")
            .args(["link", "set", &config.interface_name, "down"])
            .output()
            .await
            .context("Failed to execute ip link set down")?;
        
        if !output.status.success() {
            crate::log::warn(format!("Failed to bring down interface {}: {}", 
                config.interface_name, String::from_utf8_lossy(&output.stderr)));
        }
        
        // 设置CAN参数
        let output = Command::new("ip")
            .args([
                "link", "set", &config.interface_name, "type", "can",
                "bitrate", &config.bitrate.to_string(),
                "restart-ms", &config.restart_ms.to_string()
            ])
            .output()
            .await
            .context("Failed to configure CAN interface")?;
        
        if !output.status.success() {
            return Err(anyhow::anyhow!("Failed to configure CAN interface: {}", 
                String::from_utf8_lossy(&output.stderr)));
        }
        
        // 启动接口
        let output = Command::new("ip")
            .args(["link", "set", &config.interface_name, "up"])
            .output()
            .await
            .context("Failed to bring up CAN interface")?;
        
        if !output.status.success() {
            return Err(anyhow::anyhow!("Failed to bring up CAN interface: {}", 
                String::from_utf8_lossy(&output.stderr)));
        }
        
        Ok(())
    }

    /// 启动发送和接收任务
    async fn start_tasks(&mut self) -> Result<()> {
        let (tx_send, mut rx_send) = mpsc::channel::<CanFrameData>(128);
        let (shutdown_tx, mut shutdown_rx) = mpsc::channel::<()>(1);
        
        self.tx_channel = Some(tx_send);
        self.shutdown_tx = Some(shutdown_tx);
        
        let param_clone = self.param.clone();
        
        // 发送任务
        let send_handle = {
            let param = param_clone.clone();
            tokio::spawn(async move {
                #[cfg(target_os = "linux")]
                {
                    while let Some(frame_data) = rx_send.recv().await {
                        let param_guard = param.lock().await;
                        if let Some(ref socket) = param_guard.socket {
                            match CanFrame::new(frame_data.can_id, &frame_data.data) {
                                Ok(frame) => {
                                    if let Err(e) = socket.write_frame(&frame) {
                                        crate::log::error(format!("CAN send error: {}", e));
                                    }
                                }
                                Err(e) => {
                                    crate::log::error(format!("Failed to create CAN frame: {}", e));
                                }
                            }
                        }
                    }
                }
                
                #[cfg(not(target_os = "linux"))]
                {
                    while let Some(frame_data) = rx_send.recv().await {
                        crate::log::debug(format!("Mock CAN send: ID={:08X}, Data={:02X?}", 
                            frame_data.can_id, frame_data.data));
                    }
                }
            })
        };
        
        // 接收任务
        let recv_handle = {
            let param = param_clone.clone();
            tokio::spawn(async move {
                #[cfg(target_os = "linux")]
                {
                    loop {
                        tokio::select! {
                            _ = shutdown_rx.recv() => {
                                crate::log::info("CAN receive task shutting down");
                                break;
                            }
                            _ = async {
                                let param_guard = param.lock().await;
                                if let Some(ref socket) = param_guard.socket {
                                    match socket.read_frame() {
                                        Ok(frame) => {
                                            let can_id = frame.id();
                                            let data = frame.data();
                                            
                                            // 检查错误帧
                                            if can_id & 0x20000000 != 0 { // CAN_ERR_FLAG
                                                crate::log::warn(format!("CAN receive error: {:08X}", can_id));
                                                return;
                                            }
                                            
                                            // 调用回调函数
                                            if let Some(ref callback) = param_guard.callback {
                                                callback(can_id, data);
                                            }
                                        }
                                        Err(e) => {
                                            crate::log::warn(format!("CAN read error: {}", e));
                                            tokio::time::sleep(Duration::from_millis(100)).await;
                                        }
                                    }
                                }
                            } => {}
                        }
                    }
                }
                
                #[cfg(not(target_os = "linux"))]
                {
                    let mut interval = tokio::time::interval(Duration::from_secs(2));
                    loop {
                        tokio::select! {
                            _ = shutdown_rx.recv() => {
                                crate::log::info("Mock CAN receive task shutting down");
                                break;
                            }
                            _ = interval.tick() => {
                                let param_guard = param.lock().await;
                                if let Some(ref callback) = param_guard.callback {
                                    // 模拟接收数据
                                    let mock_data = vec![0x01, 0x02, 0x03, 0x04];
                                    callback(0x123, &mock_data);
                                }
                            }
                        }
                    }
                }
            })
        };
        
        // 保存接收任务句柄
        {
            let mut param_guard = self.param.lock().await;
            param_guard.recv_handle = Some(recv_handle);
        }
        
        Ok(())
    }

    /// 设置数据接收回调函数
    pub async fn set_recv_callback(&self, callback: CanRecvCallback) -> CanRetCode {
        let mut param = self.param.lock().await;
        param.callback = Some(callback);
        CanRetCode::Success
    }

    /// 发送CAN数据
    /// 
    /// # 参数
    /// * `can_id` - CAN标识符
    /// * `data` - 数据缓冲区 (最大8字节)
    /// 
    /// # 返回值
    /// * `CanRetCode::Success` - 发送成功
    /// * `CanRetCode::Fail` - 发送失败
    pub async fn send(&self, can_id: u32, data: &[u8]) -> CanRetCode {
        if data.len() > 8 {
            crate::log::error("CAN data length exceeds 8 bytes");
            return CanRetCode::Fail;
        }
        
        if let Some(ref tx) = self.tx_channel {
            let frame_data = CanFrameData {
                can_id,
                data: data.to_vec(),
            };
            
            match tx.send(frame_data).await {
                Ok(_) => CanRetCode::Success,
                Err(e) => {
                    crate::log::error(format!("Failed to send CAN frame: {}", e));
                    CanRetCode::Fail
                }
            }
        } else {
            crate::log::error("CAN not initialized");
            CanRetCode::Fail
        }
    }

    /// CAN通讯逆初始化
    pub async fn uninit(&mut self) -> CanRetCode {
        // 发送关闭信号
        if let Some(ref shutdown_tx) = self.shutdown_tx {
            let _ = shutdown_tx.send(()).await;
        }
        
        let mut param = self.param.lock().await;
        
        // 等待接收任务结束
        if let Some(handle) = param.recv_handle.take() {
            handle.abort();
        }
        
        // 清理资源
        #[cfg(target_os = "linux")]
        {
            param.socket = None;
        }
        
        param.callback = None;
        self.tx_channel = None;
        self.shutdown_tx = None;
        
        crate::log::info("CAN uninitialized");
        CanRetCode::Success
    }
}

/// 实现Drop trait以确保资源清理
impl Drop for CanFunc {
    fn drop(&mut self) {
        // 注意: 在Drop中不能使用async，所以这里只能做同步清理
        if let Some(ref shutdown_tx) = self.shutdown_tx {
            let _ = shutdown_tx.try_send(());
        }
    }
}

/// 便利函数：创建CAN通讯功能实例
pub fn can_create() -> CanFunc {
    CanFunc::new()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicU32, Ordering};
    use std::time::Duration;

    #[tokio::test]
    async fn test_can_create() {
        let can_func = can_create();
        assert!(can_func.tx_channel.is_none());
    }

    #[tokio::test]
    async fn test_can_init_mock() {
        let mut can_func = can_create();
        let result = can_func.init("can0", 500000).await;
        
        #[cfg(not(target_os = "linux"))]
        assert!(result.is_ok());
        
        let _ = can_func.uninit().await;
    }

    #[tokio::test]
    async fn test_can_send() {
        let mut can_func = can_create();
        let _ = can_func.init("can0", 500000).await;
        
        let data = vec![0x01, 0x02, 0x03, 0x04];
        let result = can_func.send(0x123, &data).await;
        
        #[cfg(not(target_os = "linux"))]
        assert_eq!(result, CanRetCode::Success);
        
        let _ = can_func.uninit().await;
    }

    #[tokio::test]
    async fn test_can_callback() {
        let mut can_func = can_create();
        let _ = can_func.init("can0", 500000).await;
        
        let received_id = Arc::new(AtomicU32::new(0));
        let received_id_clone = received_id.clone();
        
        let callback = Arc::new(move |can_id: u32, data: &[u8]| {
            received_id_clone.store(can_id, Ordering::Relaxed);
            println!("Received CAN ID: {:08X}, Data: {:02X?}", can_id, data);
        });
        
        let result = can_func.set_recv_callback(callback).await;
        assert_eq!(result, CanRetCode::Success);
        
        // 在模拟模式下等待一段时间以接收模拟数据
        #[cfg(not(target_os = "linux"))]
        {
            tokio::time::sleep(Duration::from_secs(3)).await;
            assert_ne!(received_id.load(Ordering::Relaxed), 0);
        }
        
        let _ = can_func.uninit().await;
    }

    #[tokio::test]
    async fn test_can_send_oversized_data() {
        let mut can_func = can_create();
        let _ = can_func.init("can0", 500000).await;
        
        let oversized_data = vec![0; 10]; // 超过8字节
        let result = can_func.send(0x123, &oversized_data).await;
        assert_eq!(result, CanRetCode::Fail);
        
        let _ = can_func.uninit().await;
    }
}