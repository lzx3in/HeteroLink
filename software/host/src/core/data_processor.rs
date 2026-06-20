use crate::domain::{TelemetryData, ChannelStats};
use std::collections::{HashMap, VecDeque};
use std::time::{SystemTime, UNIX_EPOCH};
use tokio::sync::RwLock;
use tracing::info;

/// 设备数据（内部结构）
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
///
/// 使用 `RwLock` 替代 `Mutex`，允许并发读取（高频统计查询场景下更优）。
/// 外层已由 `Arc<DataProcessor>` 共享，内部无需再套 Arc。
pub struct DataProcessor {
    device_data: RwLock<HashMap<String, DeviceData>>,
    default_buffer_size: usize,
}

impl DataProcessor {
    pub fn new() -> Self {
        Self {
            device_data: RwLock::new(HashMap::new()),
            default_buffer_size: 10000,
        }
    }

    pub async fn set_buffer_size(&self, device_id: &str, size: usize) {
        let mut device_data = self.device_data.write().await;
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
        let mut device_data = self.device_data.write().await;
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
        let device_data = self.device_data.read().await;
        device_data
            .get(device_id)
            .map(|d| d.buffer.iter().cloned().collect())
            .unwrap_or_default()
    }

    pub async fn get_latest_data(&self, device_id: &str, count: usize) -> Vec<TelemetryData> {
        let device_data = self.device_data.read().await;
        device_data
            .get(device_id)
            .map(|d| {
                let start = d.buffer.len().saturating_sub(count);
                d.buffer.iter().skip(start).cloned().collect()
            })
            .unwrap_or_default()
    }

    pub async fn get_stats(&self, device_id: &str) -> HashMap<i32, ChannelStats> {
        let device_data = self.device_data.read().await;
        device_data
            .get(device_id)
            .map(|d| d.channel_stats.clone())
            .unwrap_or_default()
    }

    pub async fn clear_data(&self, device_id: &str) {
        let mut device_data = self.device_data.write().await;
        if let Some(data) = device_data.get_mut(device_id) {
            data.buffer.clear();
            data.channel_stats.clear();
        }
    }

    pub async fn clear_all(&self) {
        self.device_data.write().await.clear();
        info!("All data cleared");
    }

    fn update_stats(dev_data: &mut DeviceData, data: &TelemetryData) {
        for (i, &value) in data.channels.iter().enumerate() {
            let i = i as i32;
            let stats = dev_data
                .channel_stats
                .entry(i)
                .or_default();

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
            stats.rms =
                (stats.rms * stats.rms * (count - 1.0) / count + value * value / count).sqrt();
        }
    }
}
