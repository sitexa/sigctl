use once_cell::sync::Lazy;
use std::{fs::{OpenOptions, rename, create_dir_all}, io::Write, path::Path, sync::Mutex, time::{SystemTime, UNIX_EPOCH}};

static LOGGER: Lazy<Mutex<Logger>> = Lazy::new(|| Mutex::new(Logger::new("", 1_048_576)));

pub struct Logger { path: String, max: u64 }
impl Logger {
    pub fn new(path: &str, max: u64) -> Self { Self{ path: path.to_string(), max } }
    fn rotate_if_needed(&self) {
        if let Ok(meta) = std::fs::metadata(&self.path) {
            if meta.len() > self.max {
                let bak = format!("{}.bak", &self.path);
                let _ = rename(&self.path, bak);
            }
        }
    }
    fn write_line(&self, level: &str, msg: &str) {
        let ts = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs();
        let line = format!("[{}][{}] {}\n", ts, level, msg);
        // 确保日志目录存在
        if let Some(parent) = Path::new(&self.path).parent() {
            let _ = create_dir_all(parent);
        }
        self.rotate_if_needed();
        if let Ok(mut f) = OpenOptions::new().create(true).append(true).open(&self.path) {
            let _ = f.write_all(line.as_bytes());
        }
    }
}

pub fn init(path: &str, max_bytes: u64) {
    let mut lg = LOGGER.lock().unwrap();
    lg.path = path.to_string();
    lg.max = max_bytes;
    lg.rotate_if_needed();
    // 在初始化时不调用 info 函数，避免循环依赖
    // info("logger initialized");
}

pub fn info(msg: impl AsRef<str>) { LOGGER.lock().unwrap().write_line("INFO", msg.as_ref()); }
pub fn warn(msg: impl AsRef<str>) { LOGGER.lock().unwrap().write_line("WARN", msg.as_ref()); }
pub fn error(msg: impl AsRef<str>) { LOGGER.lock().unwrap().write_line("ERROR", msg.as_ref()); }
pub fn debug(msg: impl AsRef<str>) { LOGGER.lock().unwrap().write_line("DEBUG", msg.as_ref()); }