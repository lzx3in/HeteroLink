use crate::protocol::TelemetryData;
use anyhow::Result;
use chrono::Local;
use std::fs::{self, File};
use std::io::Write;
use std::path::PathBuf;
use std::time::{SystemTime, UNIX_EPOCH};
use tracing::{info, warn, error};

/// 数据记录器
pub struct DataLogger {
    file: Option<File>,
    base_path: String,
    device_id: String,
    recording: bool,
    max_file_size: u64,
    auto_split: bool,
    split_interval_ms: u64,
    bytes_written: u64,
    last_split_time: u64,
}

impl DataLogger {
    pub fn new() -> Self {
        Self {
            file: None,
            base_path: String::new(),
            device_id: String::new(),
            recording: false,
            max_file_size: 100 * 1024 * 1024,
            auto_split: false,
            split_interval_ms: 3600000,
            bytes_written: 0,
            last_split_time: 0,
        }
    }

    pub fn start_recording(&mut self, base_path: &str, device_id: &str) -> Result<()> {
        if self.recording {
            warn!("Already recording");
            return Ok(());
        }

        self.base_path = base_path.to_string();
        self.device_id = device_id.to_string();

        fs::create_dir_all(base_path)?;

        self.open_file()?;
        self.recording = true;
        self.bytes_written = 0;
        self.last_split_time = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_millis() as u64;

        info!("Recording started: {:?}", self.current_file_path());
        Ok(())
    }

    pub fn stop_recording(&mut self) {
        if !self.recording {
            return;
        }
        self.close_file();
        self.recording = false;
        info!("Recording stopped");
    }

    pub fn is_recording(&self) -> bool {
        self.recording
    }

    pub fn current_file_path(&self) -> Option<PathBuf> {
        if self.recording {
            Some(PathBuf::from(&self.base_path).join(self.generate_filename()))
        } else {
            None
        }
    }

    pub fn set_max_file_size(&mut self, size_mb: u64) {
        self.max_file_size = size_mb * 1024 * 1024;
    }

    pub fn set_auto_split(&mut self, enabled: bool, interval_ms: u64) {
        self.auto_split = enabled;
        self.split_interval_ms = interval_ms;
    }

    pub fn write_data(&mut self, device_id: &str, data: &TelemetryData) {
        if !self.recording || device_id != self.device_id {
            return;
        }

        if let Some(ref mut file) = self.file {
            let mut line = data.timestamp.to_string();
            for ch in &data.channels {
                line.push_str(&format!(",{:.6}", ch));
            }
            line.push('\n');

            let bytes = line.as_bytes();
            if let Err(e) = file.write_all(bytes) {
                error!("Failed to write data: {}", e);
                return;
            }
            self.bytes_written += bytes.len() as u64;

            if self.auto_split {
                let now = SystemTime::now()
                    .duration_since(UNIX_EPOCH)
                    .unwrap()
                    .as_millis() as u64;

                if self.bytes_written >= self.max_file_size {
                    let _ = self.split_file();
                } else if now - self.last_split_time >= self.split_interval_ms {
                    let _ = self.split_file();
                }
            }
        }
    }

    fn open_file(&mut self) -> Result<()> {
        let path = PathBuf::from(&self.base_path).join(self.generate_filename());
        let mut file = File::create(&path)?;

        // 写入 CSV 表头
        let mut header = "timestamp".to_string();
        for i in 0..16 {
            header.push_str(&format!(",ch{}", i));
        }
        header.push('\n');
        file.write_all(header.as_bytes())?;

        self.file = Some(file);
        Ok(())
    }

    fn close_file(&mut self) {
        self.file.take();
    }

    fn generate_filename(&self) -> String {
        let now = Local::now();
        format!("heterolink_{}_{}.csv", self.device_id, now.format("%Y%m%d_%H%M%S"))
    }

    fn split_file(&mut self) -> Result<()> {
        let old_path = self.current_file_path();
        self.close_file();
        self.open_file()?;
        self.bytes_written = 0;
        self.last_split_time = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_millis() as u64;

        if let Some(path) = old_path {
            info!("File split: {:?} -> {:?}", path, self.current_file_path());
        }
        Ok(())
    }
}
