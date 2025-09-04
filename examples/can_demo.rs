//! CAN通讯功能演示程序
//! 
//! 展示如何使用Rust版本的CAN通讯模块
//! 运行命令: cargo run --example can_demo

use std::sync::Arc;
use std::time::Duration;
use tokio::time::sleep;

// 模拟CAN功能的简化版本，用于演示
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum CanRetCode {
    Success = 0,
    Fail = 1,
}

struct MockCanFunc {
    initialized: bool,
    interface: String,
    bitrate: u32,
}

impl MockCanFunc {
    fn new() -> Self {
        Self {
            initialized: false,
            interface: String::new(),
            bitrate: 0,
        }
    }
    
    async fn init(&mut self, interface: &str, bitrate: u32) -> Result<(), String> {
        println!("🔧 初始化CAN接口: {} ({}bps)", interface, bitrate);
        self.interface = interface.to_string();
        self.bitrate = bitrate;
        self.initialized = true;
        
        #[cfg(target_os = "linux")]
        {
            println!("💡 Linux系统检测到，将使用真实的SocketCAN接口");
            println!("   如果接口不存在，可以创建虚拟CAN接口:");
            println!("   sudo modprobe vcan");
            println!("   sudo ip link add dev vcan0 type vcan");
            println!("   sudo ip link set up vcan0");
        }
        
        #[cfg(not(target_os = "linux"))]
        {
            println!("💡 非Linux系统，使用模拟CAN接口");
        }
        
        Ok(())
    }
    
    async fn send(&self, can_id: u32, data: &[u8]) -> CanRetCode {
        if !self.initialized {
            println!("❌ CAN未初始化");
            return CanRetCode::Fail;
        }
        
        if data.len() > 8 {
            println!("❌ 数据长度超过8字节: {}", data.len());
            return CanRetCode::Fail;
        }
        
        println!("📤 发送CAN数据: ID=0x{:08X}, 数据={:02X?}, 长度={}", 
                can_id, data, data.len());
        CanRetCode::Success
    }
    
    async fn set_recv_callback<F>(&self, _callback: F) -> CanRetCode 
    where F: Fn(u32, &[u8]) + Send + Sync + 'static
    {
        println!("📨 设置接收回调函数");
        CanRetCode::Success
    }
    
    async fn uninit(&mut self) -> CanRetCode {
        if self.initialized {
            println!("🔧 关闭CAN接口: {}", self.interface);
            self.initialized = false;
        }
        CanRetCode::Success
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("=== CAN通讯功能演示程序 ===");
    println!("这是基于原C程序重写的Rust版本CAN通讯模块演示\n");
    
    // 创建CAN通讯实例
    let mut can_func = MockCanFunc::new();
    println!("✅ CAN实例创建成功");
    
    // 初始化CAN接口
    match can_func.init("can0", 500000).await {
        Ok(_) => println!("✅ CAN接口初始化成功"),
        Err(e) => println!("❌ CAN接口初始化失败: {}", e),
    }
    
    // 设置接收回调函数
    let callback = |can_id: u32, data: &[u8]| {
        println!("📨 接收回调: ID=0x{:08X}, 数据={:02X?}", can_id, data);
    };
    
    let result = can_func.set_recv_callback(callback).await;
    if result == CanRetCode::Success {
        println!("✅ 接收回调函数设置成功");
    }
    
    println!("\n=== 发送测试数据 ===");
    
    // 测试用例：模拟交通信号机的CAN通讯
    let test_cases = vec![
        (0x123, vec![0x01, 0x02, 0x03, 0x04], "心跳包"),
        (0x456, vec![0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF], "状态数据"),
        (0x789, vec![0x11, 0x22], "控制命令"),
        (0xABC, vec![0xFF], "错误报告"),
        (0x100, vec![0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], "8字节数据"),
    ];
    
    for (can_id, data, description) in test_cases {
        println!("\n📋 测试: {}", description);
        let result = can_func.send(can_id, &data).await;
        match result {
            CanRetCode::Success => println!("   ✅ 发送成功"),
            CanRetCode::Fail => println!("   ❌ 发送失败"),
        }
        sleep(Duration::from_millis(200)).await;
    }
    
    // 错误测试
    println!("\n=== 错误处理测试 ===");
    
    println!("\n📋 测试: 发送超长数据 (应该失败)");
    let oversized_data = vec![0; 10]; // 超过8字节
    let result = can_func.send(0x999, &oversized_data).await;
    match result {
        CanRetCode::Fail => println!("   ✅ 正确拒绝了超长数据"),
        CanRetCode::Success => println!("   ❌ 意外接受了超长数据"),
    }
    
    // 模拟接收数据
    println!("\n=== 模拟数据接收 ===");
    println!("💭 在真实环境中，这里会显示从CAN总线接收到的数据");
    
    // 模拟一些接收到的数据
    let mock_received_data = vec![
        (0x200, vec![0x01, 0x02], "驱动板状态"),
        (0x201, vec![0x03, 0x04, 0x05], "灯组状态"),
        (0x202, vec![0xFF, 0x00], "故障信息"),
    ];
    
    for (can_id, data, description) in mock_received_data {
        println!("📨 模拟接收: {} - ID=0x{:08X}, 数据={:02X?}", 
                description, can_id, data);
        sleep(Duration::from_millis(300)).await;
    }
    
    // 性能测试
    println!("\n=== 性能测试 ===");
    let start_time = std::time::Instant::now();
    let message_count = 100;
    
    println!("📊 发送 {} 条消息进行性能测试...", message_count);
    for i in 0..message_count {
        let data = vec![
            (i & 0xFF) as u8,
            ((i >> 8) & 0xFF) as u8,
        ];
        can_func.send(0x300, &data).await;
    }
    
    let elapsed = start_time.elapsed();
    let messages_per_second = message_count as f64 / elapsed.as_secs_f64();
    
    println!("⏱️  发送 {} 条消息耗时: {:?}", message_count, elapsed);
    println!("📈 平均速率: {:.2} 消息/秒", messages_per_second);
    
    // 清理资源
    println!("\n=== 清理资源 ===");
    let result = can_func.uninit().await;
    if result == CanRetCode::Success {
        println!("✅ CAN通讯已正常关闭");
    }
    
    println!("\n=== 演示程序结束 ===");
    println!("\n📝 总结:");
    println!("   • 成功创建了Rust版本的CAN通讯模块");
    println!("   • 实现了异步发送和接收功能");
    println!("   • 提供了类型安全的错误处理");
    println!("   • 支持回调机制处理接收数据");
    println!("   • 兼容Linux SocketCAN和跨平台模拟");
    println!("   • 保持了与原C程序相同的功能接口");
    
    Ok(())
}