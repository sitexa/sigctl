use serde::Deserialize;
use std::time::Duration;

pub const TAIL: u8 = 0xED;

#[derive(Deserialize, Clone, Debug)]
pub struct Config {
    pub can: CanCfg,
    pub ids: Ids,
    pub serial: SerialCfg,
    pub timing: Timing,
    pub mapping: Mapping,
    pub log: LogCfg,
}

#[derive(Deserialize, Clone, Debug)]
pub struct CanCfg { pub iface: String, pub bitrate: u32 }

#[derive(Deserialize, Clone, Debug)]
pub struct Ids { pub down: u32, pub up: u32 }

#[derive(Deserialize, Clone, Debug)]
pub struct SerialCfg { pub panel: String, pub baud: u32 }

#[derive(Deserialize, Clone, Debug)]
pub struct Timing { pub g: u64, pub y: u64, pub ar: u64 }
impl Timing {
    pub fn g(&self)->Duration { Duration::from_millis(self.g) }
    pub fn y(&self)->Duration { Duration::from_millis(self.y) }
    pub fn ar(&self)->Duration { Duration::from_millis(self.ar) }
}

#[derive(Deserialize, Clone, Debug)]
pub struct Mapping {
    pub ns_r: u8, pub ns_y: u8, pub ns_g: u8,
    pub ew_r: u8, pub ew_y: u8, pub ew_g: u8,
}

#[derive(Deserialize, Clone, Debug)]
pub struct LogCfg { pub file: String, pub max_bytes: u64 }

/// 下行命令（主 -> 灯）0xA*
#[repr(u8)]
#[derive(Copy, Clone, Debug)]
pub enum Dn {
    Scheme    = 0xA0,
    Lamp      = 0xAA,
    Heartbeat = 0xAB,
    ResetDrv  = 0xAC,
    FailFlash = 0xAD,
    Resume    = 0xAE,
    DefaultG  = 0xAF,
    FailMode  = 0xA7,
}

/// 上行命令（灯 -> 主）0xB*
#[repr(u8)]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Up {
    BoardSt   = 0xB1,
    LampSt    = 0xB2,
    LampFault = 0xB3,
    CanFault  = 0xB4,
    Voltage   = 0xB6,
    Version   = 0xB8,
}

/// 业务层向 CAN I/O 发送的消息
#[derive(Clone, Debug)]
pub enum OutMsg {
    Heartbeat,
    SetLamp { ch: u8, state: u8 },                     // 0红/1黄/2绿/3灭
    Scheme  { id: u8, bitmap: u16, green_s: u8 },      // 两字节通道位图，高字节在前
    FailFlash,
    Resume,
    DefaultGreen { ns: u8, ew: u8 },
    FailMode { mode: u8 },
}

/// CAN I/O 上报给业务层的事件
#[derive(Clone, Debug)]
pub enum IoEvent {
    BoardStatus { board: u8, state: u8 },
    LampStatus  { board: u8, yg_bits: u16, r_bits: u16 },
    LampFault   { flag: u8, ch: u8, kind: u8 },
    CanFault    { state: u8, where_: u8 },
    VoltageMv   { mv: u16 },
    Version     { board: u8, y:u8, m:u8, d:u8, v1:u8, v2:u8 },
    Raw(Vec<u8>), // 未识别帧，便于调试
}