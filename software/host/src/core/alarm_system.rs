use crate::domain::{TelemetryData, AlarmLevel, AlarmConfig, AlarmRecord};
use chrono::Local;
use std::collections::HashMap;
use tokio::sync::RwLock;
use tracing::{info, warn};

/// 告警系统内部状态（三合一，由单个 RwLock 保护）
struct AlarmState {
    configs: HashMap<String, Vec<AlarmConfig>>,
    records: HashMap<String, Vec<AlarmRecord>>,
    active: HashMap<String, HashMap<i32, bool>>,
}

impl AlarmState {
    fn new() -> Self {
        Self {
            configs: HashMap::new(),
            records: HashMap::new(),
            active: HashMap::new(),
        }
    }
}

/// 告警系统
///
/// 将原先的三个独立 `Arc<Mutex<HashMap>>` 合并为单个 `RwLock<AlarmState>`，
/// 消除嵌套锁风险，读操作（get_alarms/get_records）使用 `.read()`，
/// 写操作（check_data/configure）使用 `.write()`。
pub struct AlarmSystem {
    state: RwLock<AlarmState>,
}

impl AlarmSystem {
    pub fn new() -> Self {
        Self {
            state: RwLock::new(AlarmState::new()),
        }
    }

    pub async fn configure_alarm(&self, device_id: &str, config: AlarmConfig) {
        let mut state = self.state.write().await;
        let device_configs = state
            .configs
            .entry(device_id.to_string())
            .or_insert_with(Vec::new);

        if let Some(existing) = device_configs
            .iter_mut()
            .find(|c| c.channel_id == config.channel_id)
        {
            *existing = config;
        } else {
            device_configs.push(config);
        }

        info!(
            "Alarm configured for device {} channel {}",
            device_id,
            device_configs.last().unwrap().channel_id
        );
    }

    pub async fn get_alarms(&self, device_id: &str) -> Vec<AlarmConfig> {
        let state = self.state.read().await;
        state
            .configs
            .get(device_id)
            .cloned()
            .unwrap_or_default()
    }

    pub async fn set_alarm_enabled(&self, device_id: &str, channel_id: i32, enabled: bool) {
        let mut state = self.state.write().await;
        if let Some(device_configs) = state.configs.get_mut(device_id) {
            if let Some(config) = device_configs
                .iter_mut()
                .find(|c| c.channel_id == channel_id)
            {
                config.enabled = enabled;
                info!(
                    "Alarm {} for device {} channel {}",
                    if enabled { "enabled" } else { "disabled" },
                    device_id,
                    channel_id
                );
            }
        }
    }

    pub async fn check_data(
        &self,
        device_id: &str,
        data: &TelemetryData,
    ) -> Vec<AlarmRecord> {
        let mut state = self.state.write().await;

        let Some(device_configs) = state.configs.get(device_id).cloned() else {
            return Vec::new();
        };

        let mut triggered = Vec::new();

        for config in &device_configs {
            if !config.enabled || config.channel_id as usize >= data.channels.len() {
                continue;
            }

            let value = data.channels[config.channel_id as usize];

            if config.lower_enabled && value < config.lower_limit {
                let msg = format!(
                    "Channel {} value {} below lower limit {}",
                    config.channel_id, value, config.lower_limit
                );
                if let Some(record) = Self::trigger_alarm_inner(
                    &mut state,
                    device_id,
                    config.channel_id,
                    config.level,
                    value,
                    &msg,
                ) {
                    triggered.push(record);
                }
            } else if config.upper_enabled && value > config.upper_limit {
                let msg = format!(
                    "Channel {} value {} above upper limit {}",
                    config.channel_id, value, config.upper_limit
                );
                if let Some(record) = Self::trigger_alarm_inner(
                    &mut state,
                    device_id,
                    config.channel_id,
                    config.level,
                    value,
                    &msg,
                ) {
                    triggered.push(record);
                }
            } else {
                // 恢复正常，清除激活状态
                if let Some(device_active) = state.active.get_mut(device_id) {
                    if device_active.get(&config.channel_id) == Some(&true) {
                        device_active.insert(config.channel_id, false);
                        info!(
                            "Alarm cleared for device {} channel {}",
                            device_id, config.channel_id
                        );
                    }
                }
            }
        }

        triggered
    }

    pub async fn get_alarm_records(&self, device_id: &str) -> Vec<AlarmRecord> {
        let state = self.state.read().await;
        state
            .records
            .get(device_id)
            .cloned()
            .unwrap_or_default()
    }

    pub async fn get_all_alarm_records(&self) -> Vec<AlarmRecord> {
        let state = self.state.read().await;
        state.records.values().flat_map(|v| v.clone()).collect()
    }

    pub async fn acknowledge_alarm(&self, device_id: &str, channel_id: i32) {
        let mut state = self.state.write().await;
        if let Some(device_records) = state.records.get_mut(device_id) {
            if let Some(record) = device_records
                .iter_mut()
                .find(|r| r.channel_id == channel_id && !r.acknowledged)
            {
                record.acknowledged = true;
                info!(
                    "Alarm acknowledged for device {} channel {}",
                    device_id, channel_id
                );
            }
        }
    }

    pub async fn clear_records(&self, device_id: &str) {
        self.state.write().await.records.remove(device_id);
        info!("Alarm records cleared for device: {}", device_id);
    }

    /// 内部触发告警（在已持有写锁时调用，避免嵌套锁）
    fn trigger_alarm_inner(
        state: &mut AlarmState,
        device_id: &str,
        channel_id: i32,
        level: AlarmLevel,
        value: f32,
        message: &str,
    ) -> Option<AlarmRecord> {
        let device_active = state
            .active
            .entry(device_id.to_string())
            .or_default();

        if device_active.get(&channel_id) == Some(&true) {
            return None;
        }
        device_active.insert(channel_id, true);

        let record = AlarmRecord {
            device_id: device_id.to_string(),
            channel_id,
            level,
            value,
            message: message.to_string(),
            timestamp: Local::now(),
            acknowledged: false,
        };

        state
            .records
            .entry(device_id.to_string())
            .or_default()
            .push(record.clone());

        warn!("Alarm triggered: {}", message);
        Some(record)
    }
}
