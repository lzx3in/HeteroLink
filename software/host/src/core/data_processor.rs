use crate::protocol::TelemetryData;
use std::collections::{HashMap, VecDeque};
use std::sync::Arc;
use std::time::{SystemTime, UNIX_EPOCH};
use tokio::sync::Mutex;
use tracing::info;

/// 通道统计数据
#[derive(Debug, Clone, Default)]
pub struct ChannelStats {
    pub min: f32,
    pub max: f32,
    pub avg: f32,
    pub rms: f32,
    pub sample_count: u32,
}

/// 设备数据
struct DeviceData {
    buffer: VecDeque<TelemetryData>,
    channel_stats: HashMap<i32, ChannelStats>,
    last_update: u64,
    max_buffer_size: usize,
}

impl DeviceData {
    fn new(buffer_size: usize) -> Self {
        Self {
            buffer: VecDeque::new(),
            channel_stats: HashMap::new(),
            last_update: 0,
            max_buffer_size: buffer_size,
        }
    }
}

/// 数据处理器
pub struct DataProcessor {
    device_data: Arc<Mutex<HashMap<String, DeviceData>>>,
    default_buffer_size: usize,
}

impl DataProcessor {
    pub fn new() -> Self {
        Self {
            device_data: Arc::new(Mutex::new(HashMap::new())),
            default_buffer_size: 10000,
        }
    }

    pub async fn set_buffer_size(&self, device_id: &str, size: usize) {
        let mut device_data = self.device_data.lock().await;
        if let Some(data) = device_data.get_mut(device_id) {
            data.max_buffer_size = size;
            while data.buffer.len() > size {
                data.buffer.pop_front();
            }
        } else {
            device_data.insert(device_id.to_string(), DeviceData::new(size));
        }
        info!("Buffer size set for device {}: {}", device_id, size);
    }

    pub async fn add_data(&self, device_id: &str, data: TelemetryData) {
        let mut device_data = self.device_data.lock().await;
        let dev_data = device_data
            .entry(device_id.to_string())
            .or_insert_with(|| DeviceData::new(self.default_buffer_size));

        dev_data.buffer.push_back(data.clone());
        while dev_data.buffer.len() > dev_data.max_buffer_size {
            dev_data.buffer.pop_front();
        }

        dev_data.last_update = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_millis() as u64;

        Self::update_stats(dev_data, &data);
    }

    pub async fn get_data(&self, device_id: &str) -> Vec<TelemetryData> {
        let device_data = self.device_data.lock().await;
        if let Some(data) = device_data.get(device_id) {
            data.buffer.iter().cloned().collect()
        } else {
            Vec::new()
        }
    }

    pub async fn get_latest_data(&self, device_id: &str, count: usize) -> Vec<TelemetryData> {
        let device_data = self.device_data.lock().await;
        if let Some(data) = device_data.get(device_id) {
            let start = data.buffer.len().saturating_sub(count);
            data.buffer.iter().skip(start).cloned().collect()
        } else {
            Vec::new()
        }
    }

    pub async fn get_stats(&self, device_id: &str) -> HashMap<i32, ChannelStats> {
        let device_data = self.device_data.lock().await;
        if let Some(data) = device_data.get(device_id) {
            data.channel_stats.clone()
        } else {
            HashMap::new()
        }
    }

    pub async fn clear_data(&self, device_id: &str) {
        let mut device_data = self.device_data.lock().await;
        if let Some(data) = device_data.get_mut(device_id) {
            data.buffer.clear();
            data.channel_stats.clear();
        }
    }

    pub async fn clear_all(&self) {
        self.device_data.lock().await.clear();
        info!("All data cleared");
    }

    pub async fn export_to_csv(&self, device_id: &str, file_path: &str) -> anyhow::Result<()> {
        let device_data = self.device_data.lock().await;
        let data = device_data.get(device_id)
            .ok_or_else(|| anyhow::anyhow!("No data for device: {}", device_id))?;

        let mut writer = csv::Writer::from_path(file_path)?;

        if let Some(first) = data.buffer.front() {
            let channel_count = first.channels.len();
            let mut header = vec!["timestamp".to_string()];
            for i in 0..channel_count {
                header.push(format!("channel{}", i));
            }
            writer.write_record(&header)?;

            for item in &data.buffer {
                let mut record = vec![item.timestamp.to_string()];
                for ch in &item.channels {
                    record.push(ch.to_string());
                }
                writer.write_record(&record)?;
            }
        }

        writer.flush()?;
        info!("Data exported to CSV: {}", file_path);
        Ok(())
    }

    pub async fn export_to_json(&self, device_id: &str, file_path: &str) -> anyhow::Result<()> {
        let device_data = self.device_data.lock().await;
        let data = device_data.get(device_id)
            .ok_or_else(|| anyhow::anyhow!("No data for device: {}", device_id))?;

        let json = serde_json::to_string_pretty(&data.buffer.iter().cloned().collect::<Vec<_>>())?;
        std::fs::write(file_path, json)?;
        info!("Data exported to JSON: {}", file_path);
        Ok(())
    }

    fn update_stats(dev_data: &mut DeviceData, data: &TelemetryData) {
        for (i, &value) in data.channels.iter().enumerate() {
            let i = i as i32;
            let stats = dev_data.channel_stats.entry(i).or_insert_with(ChannelStats::default);

            if stats.sample_count == 0 || value < stats.min {
                stats.min = value;
            }
            if stats.sample_count == 0 || value > stats.max {
                stats.max = value;
            }

            let old_avg = stats.avg;
            stats.sample_count += 1;
            stats.avg = old_avg + (value - old_avg) / stats.sample_count as f32;

            let count = stats.sample_count as f32;
            stats.rms = (stats.rms * stats.rms * (count - 1.0) / count + value * value / count).sqrt();
        }
    }
}
