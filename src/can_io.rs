//! 统一的CAN通讯模块
//! 
//! 整合了io_can.rs和can_func.rs的功能，提供统一的、可配置的CAN接口
//! 支持Linux SocketCAN和跨平台模拟模式

use crate::types::{Config, Dn, IoEvent, OutMsg, TAIL, Up};
use anyhow::{Result, Context};
use std::sync::Arc;
use tokio::sync::{mpsc, Mutex};
use tokio::task::{self, JoinHandle};
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

/// CAN通讯配置参数（扩展自原Config）
#[derive(Debug, Clone)]
pub struct CanConfig {
    pub interface_name: String,
    pub bitrate: u32,
    pub restart_ms: u32,
    pub down_id: u32,
    pub up_id: u32,
}

impl From<&Config> for CanConfig {
    fn from(cfg: &Config) -> Self {
        Self {
            interface_name: cfg.can.iface.clone(),
            bitrate: cfg.can.bitrate,
            restart_ms: 200, // 默认值
            down_id: cfg.ids.down,
            up_id: cfg.ids.up,
        }
    }
}

/// 统一CAN通讯功能结构体
pub struct UnifiedCan {
    config: CanConfig,
    #[cfg(target_os = "linux")]
    send_socket: Option<CanSocket>,
    #[cfg(target_os = "linux")]
    recv_socket: Option<CanSocket>,
    send_handle: Option<JoinHandle<()>>,
    recv_handle: Option<JoinHandle<()>>,
    callback: Option<CanRecvCallback>,
    tx_channel: Option<mpsc::Sender<CanFrameData>>,
    shutdown_tx: Option<mpsc::Sender<()>>,
}

impl UnifiedCan {
    /// 创建新的统一CAN实例
    pub fn new(config: CanConfig) -> Self {
        Self {
            config,
            #[cfg(target_os = "linux")]
            send_socket: None,
            #[cfg(target_os = "linux")]
            recv_socket: None,
            send_handle: None,
            recv_handle: None,
            callback: None,
            tx_channel: None,
            shutdown_tx: None,
        }
    }

    /// 初始化CAN通讯
    pub async fn init(&mut self) -> Result<()> {
        #[cfg(target_os = "linux")]
        {
            self.configure_can_interface().await?;
            self.init_linux_sockets().await?;
        }
        
        #[cfg(not(target_os = "linux"))]
        {
            crate::log::info(format!("Mock CAN initialized on {} (simulated)", &self.config.interface_name));
        }
        
        Ok(())
    }

    #[cfg(target_os = "linux")]
    async fn configure_can_interface(&self) -> Result<()> {
        use tokio::process::Command;
        
        // 配置CAN接口
        let commands = [
            format!("ip link set {} down", self.config.interface_name),
            format!("ip link set {} type can bitrate {} restart-ms {}", 
                   self.config.interface_name, self.config.bitrate, self.config.restart_ms),
            format!("ip link set {} up", self.config.interface_name),
        ];
        
        for cmd in &commands {
            let output = Command::new("sh")
                .arg("-c")
                .arg(cmd)
                .output()
                .await
                .context(format!("Failed to execute: {}", cmd))?;
                
            if !output.status.success() {
                let stderr = String::from_utf8_lossy(&output.stderr);
                crate::log::warn(format!("Command '{}' failed: {}", cmd, stderr));
            }
        }
        
        Ok(())
    }

    #[cfg(target_os = "linux")]
    async fn init_linux_sockets(&mut self) -> Result<()> {
        // 创建发送和接收socket
        let send_socket = CanSocket::open(&self.config.interface_name)
            .context("Failed to open CAN socket for sending")?;
        let recv_socket = CanSocket::open(&self.config.interface_name)
            .context("Failed to open CAN socket for receiving")?;
            
        self.send_socket = Some(send_socket);
        self.recv_socket = Some(recv_socket);
        
        crate::log::info(format!("CAN sockets opened on {}", &self.config.interface_name));
        Ok(())
    }

    /// 设置接收回调函数
    pub async fn set_recv_callback(&mut self, callback: CanRecvCallback) -> CanRetCode {
        self.callback = Some(callback);
        CanRetCode::Success
    }

    /// 发送CAN数据
    pub async fn send(&self, can_id: u32, data: &[u8]) -> CanRetCode {
        if data.len() > 8 {
            crate::log::error(format!("CAN data too long: {} bytes", data.len()));
            return CanRetCode::Fail;
        }

        #[cfg(target_os = "linux")]
        {
            if let Some(ref socket) = self.send_socket {
                match CanFrame::new(can_id, data) {
                    Ok(frame) => {
                        if let Err(e) = socket.write_frame(&frame) {
                            crate::log::error(format!("CAN write error: {}", e));
                            return CanRetCode::Fail;
                        }
                    }
                    Err(e) => {
                        crate::log::error(format!("CAN frame creation error: {}", e));
                        return CanRetCode::Fail;
                    }
                }
            } else {
                crate::log::error("CAN socket not initialized");
                return CanRetCode::Fail;
            }
        }

        #[cfg(not(target_os = "linux"))]
        {
            crate::log::debug(format!("Mock CAN send: ID={:X}, Data={:02X?}", can_id, data));
        }

        CanRetCode::Success
    }

    /// 启动接收任务
    pub async fn start_recv_task(&mut self) -> Result<()> {
        let callback = self.callback.clone();
        let config = self.config.clone();
        
        #[cfg(target_os = "linux")]
        {
            if let Some(socket) = self.recv_socket.take() {
                let handle = task::spawn_blocking(move || {
                    loop {
                        match socket.read_frame() {
                            Ok(frame) => {
                                if frame.id() == config.up_id {
                                    let data = frame.data().to_vec();
                                    if let Some(ref cb) = callback {
                                        cb(frame.id(), &data);
                                    }
                                }
                            }
                            Err(e) => {
                                crate::log::warn(format!("CAN read error: {}", e));
                            }
                        }
                    }
                });
                self.recv_handle = Some(handle);
            }
        }
        
        #[cfg(not(target_os = "linux"))]
        {
            let handle = task::spawn(async move {
                let mut interval = tokio::time::interval(Duration::from_millis(1000));
                let mut counter = 0u8;
                
                loop {
                    interval.tick().await;
                    
                    // 模拟各种CAN事件
                    let mock_events = [
                        vec![0xB1, 0x01, 0x01, 0xED], // 板卡状态
                        vec![0xB2, 0x00, 0x01, 0x00, 0x02, 0x01, 0xED], // 灯组状态
                        vec![0xB3, 0x00, 0x01, 0x01, 0xED], // 灯故障
                        vec![0xB6, 0x00, 0xDC, 0xED], // 电压220V (0x00DC = 220)
                        if counter % 10 == 0 { vec![0xB4, 0x01, 0x00, 0xED] } else { vec![0xB4, 0x00, 0x00, 0xED] }, // CAN故障
                        vec![0xB8, 0x01, 0x23, 0x12, 0x15, 0x01, 0x00, 0xED], // 版本信息
                    ];
                    
                    for event_data in &mock_events {
                        if let Some(ref cb) = callback {
                            cb(config.up_id, event_data);
                        }
                    }
                    
                    counter = counter.wrapping_add(1);
                }
            });
            self.recv_handle = Some(handle);
        }
        
        Ok(())
    }

    /// 停止CAN通讯
    pub async fn uninit(&mut self) -> CanRetCode {
        // 停止接收任务
        if let Some(handle) = self.recv_handle.take() {
            handle.abort();
        }
        
        // 停止发送任务
        if let Some(handle) = self.send_handle.take() {
            handle.abort();
        }
        
        #[cfg(target_os = "linux")]
        {
            self.send_socket = None;
            self.recv_socket = None;
        }
        
        self.callback = None;
        
        crate::log::info("CAN communication stopped");
        CanRetCode::Success
    }
}

/// 启动 CAN I/O 子系统：返回 (发送句柄, 事件接收端)
/// 保持与原io_can.rs兼容的接口
pub fn spawn(cfg: Arc<Config>) -> (mpsc::Sender<OutMsg>, mpsc::Receiver<IoEvent>) {
    let (tx_out, rx_out) = mpsc::channel::<OutMsg>(128);
    let (tx_evt, rx_evt) = mpsc::channel::<IoEvent>(128);

    let can_config = CanConfig::from(cfg.as_ref());
    let mut unified_can = UnifiedCan::new(can_config);
    
    // 启动发送任务
    {
        let cfg = cfg.clone();
        let tx_evt = tx_evt.clone();
        let mut rx_out = rx_out;
        
        task::spawn(async move {
            // 初始化CAN
            if let Err(e) = unified_can.init().await {
                crate::log::error(format!("CAN initialization failed: {}", e));
                return;
            }
            
            // 处理发送消息
            while let Some(msg) = rx_out.recv().await {
                let payload = pack_dn(&msg);
                if unified_can.send(cfg.ids.down, &payload).await != CanRetCode::Success {
                    let _ = tx_evt.send(IoEvent::Raw(vec![0xEE, 0xEE])).await; // 写错误标记
                }
            }
        });
    }
    
    // 启动接收任务
    {
        let cfg = cfg.clone();
        let tx_evt = tx_evt.clone();
        
        task::spawn(async move {
            let can_config = CanConfig::from(cfg.as_ref());
            let mut recv_can = UnifiedCan::new(can_config);
            
            if let Err(e) = recv_can.init().await {
                crate::log::error(format!("CAN receiver initialization failed: {}", e));
                return;
            }
            
            // 设置接收回调
            let tx_evt_cb = tx_evt.clone();
            let callback: CanRecvCallback = Arc::new(move |_can_id, data| {
                if let Some(evt) = parse_up(data) {
                    let _ = tx_evt_cb.try_send(evt);
                } else {
                    let _ = tx_evt_cb.try_send(IoEvent::Raw(data.to_vec()));
                }
            });
            
            let _ = recv_can.set_recv_callback(callback).await;
            
            if let Err(e) = recv_can.start_recv_task().await {
                crate::log::error(format!("Failed to start CAN receive task: {}", e));
            }
        });
    }

    (tx_out, rx_evt)
}

/// 创建CAN功能实例（兼容can_func.rs接口）
pub fn can_create() -> UnifiedCan {
    let config = CanConfig {
        interface_name: "can0".to_string(),
        bitrate: 500000,
        restart_ms: 200,
        down_id: 0x123,
        up_id: 0x456,
    };
    UnifiedCan::new(config)
}

// 以下函数保持与原io_can.rs的兼容性

fn pack_dn(m: &OutMsg) -> Vec<u8> {
    match m {
        OutMsg::Heartbeat => vec![0xAA, 0x55, 0xED],
        OutMsg::SetLamp { ch, state } => vec![0xA0, *ch, *state, 0xED],
        OutMsg::Scheme { id, bitmap, green_s } => {
            let mut result = vec![0xA1, *id];
            result.extend_from_slice(&bitmap.to_be_bytes());
            result.push(*green_s);
            result.push(0xED);
            result
        },
        OutMsg::FailFlash => vec![0xA2, 0x01, 0xED],
        OutMsg::Resume => vec![0xA2, 0x00, 0xED],
        OutMsg::DefaultGreen { ns, ew } => vec![0xA3, *ns, *ew, 0xED],
        OutMsg::FailMode { mode } => vec![0xA4, *mode, 0xED],
    }
}

fn parse_up(d: &[u8]) -> Option<IoEvent> {
    if d.len() < 3 || d[d.len() - 1] != TAIL {
        return None;
    }
    
    match d[0] {
        0xB1 => Some(IoEvent::BoardStatus { board: d[1], state: d.get(2).copied().unwrap_or(0) }),
        0xB2 if d.len() >= 6 => {
            let yg_bits = ((d[1] as u16) << 8) | (d[2] as u16);
            let r_bits = ((d[3] as u16) << 8) | (d[4] as u16);
            Some(IoEvent::LampStatus { board: d[5], yg_bits, r_bits })
        },
        0xB3 if d.len() >= 4 => Some(IoEvent::LampFault {
            flag: d[1],
            ch: d[2],
            kind: d[3],
        }),
        0xB4 if d.len() >= 4 => Some(IoEvent::CanFault {
            state: d[1],
            where_: d[2],
        }),
        0xB6 if d.len() >= 3 => {
            let mv = ((d[1] as u16) << 8) | (d[2] as u16);
            Some(IoEvent::VoltageMv { mv })
        },
        0xB8 if d.len() >= 7 => Some(IoEvent::Version {
            board: d[1],
            y: d[2],
            m: d[3],
            d: d[4],
            v1: d[5],
            v2: d[6],
        }),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::{OutMsg, IoEvent};

    #[test]
    fn test_pack_heartbeat() {
        let msg = OutMsg::Heartbeat;
        let packed = pack_dn(&msg);
        assert_eq!(packed, vec![0xAA, 0x55, 0xED]);
    }

    #[test]
    fn test_pack_set_lamp() {
        let msg = OutMsg::SetLamp { ch: 0x01, state: 0x02 };
        let packed = pack_dn(&msg);
        assert_eq!(packed, vec![0xA0, 0x01, 0x02, 0xED]);
    }

    #[test]
    fn test_parse_board_status() {
        let data = vec![0xB1, 0x01, 0x01, 0xED];
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::BoardStatus { board, state } => {
                assert_eq!(board, 1);
                assert_eq!(state, 1);
            },
            _ => panic!("解析错误：期望BoardStatus"),
        }
    }

    #[test]
    fn test_parse_version() {
        let data = vec![0xB8, 0x01, 0x23, 0x12, 0x15, 0x01, 0x00, 0xED];
        let event = parse_up(&data).unwrap();
        match event {
            IoEvent::Version { board, y, m, d, v1, v2 } => {
                assert_eq!(board, 1);
                assert_eq!(y, 0x23);
                assert_eq!(m, 0x12);
                assert_eq!(d, 0x15);
                assert_eq!(v1, 1);
                assert_eq!(v2, 0);
            },
            _ => panic!("解析错误：期望Version"),
        }
    }

    #[tokio::test]
    async fn test_unified_can_creation() {
        let config = CanConfig {
            interface_name: "can0".to_string(),
            bitrate: 500000,
            restart_ms: 200,
            down_id: 0x123,
            up_id: 0x456,
        };
        let can = UnifiedCan::new(config);
        // 基本创建测试
        assert!(can.send_handle.is_none());
        assert!(can.recv_handle.is_none());
    }

    #[tokio::test]
    async fn test_can_send_oversized_data() {
        let config = CanConfig {
            interface_name: "can0".to_string(),
            bitrate: 500000,
            restart_ms: 200,
            down_id: 0x123,
            up_id: 0x456,
        };
        let can = UnifiedCan::new(config);
        let oversized_data = vec![0u8; 16]; // 超过8字节
        let result = can.send(0x123, &oversized_data).await;
        assert_eq!(result, CanRetCode::Fail);
    }
}