use crate::protocol::TelemetryData;
use chrono::{DateTime, Local};
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::Mutex;
use tracing::{info, warn};

/// 告警级别
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AlarmLevel {
    Info,
    Warning,
    Critical,
}

impl std::fmt::Display for AlarmLevel {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            AlarmLevel::Info => write!(f, "INFO"),
            AlarmLevel::Warning => write!(f, "WARNING"),
            AlarmLevel::Critical => write!(f, "CRITICAL"),
        }
    }
}

/// 告警配置
#[derive(Debug, Clone)]
pub struct AlarmConfig {
    pub channel_id: i32,
    pub lower_limit: f32,
    pub upper_limit: f32,
    pub lower_enabled: bool,
    pub upper_enabled: bool,
    pub level: AlarmLevel,
    pub enabled: bool,
}

impl Default for AlarmConfig {
    fn default() -> Self {
        Self {
            channel_id: 0,
            lower_limit: 0.0,
            upper_limit: 0.0,
            lower_enabled: false,
            upper_enabled: false,
            level: AlarmLevel::Warning,
            enabled: true,
        }
    }
}

/// 告警记录
#[derive(Debug, Clone)]
pub struct AlarmRecord {
    pub device_id: String,
    pub channel_id: i32,
    pub level: AlarmLevel,
    pub value: f32,
    pub message: String,
    pub timestamp: DateTime<Local>,
    pub acknowledged: bool,
}

/// 告警系统
pub struct AlarmSystem {
    alarm_configs: Arc<Mutex<HashMap<String, Vec<AlarmConfig>>>>,
    alarm_records: Arc<Mutex<HashMap<String, Vec<AlarmRecord>>>>,
    active_alarms: Arc<Mutex<HashMap<String, HashMap<i32, bool>>>>,
}

impl AlarmSystem {
    pub fn new() -> Self {
        Self {
            alarm_configs: Arc::new(Mutex::new(HashMap::new())),
            alarm_records: Arc::new(Mutex::new(HashMap::new())),
            active_alarms: Arc::new(Mutex::new(HashMap::new())),
        }
    }

    pub async fn configure_alarm(&self, device_id: &str, config: AlarmConfig) {
        let mut configs = self.alarm_configs.lock().await;
        let device_configs = configs.entry(device_id.to_string()).or_insert_with(Vec::new);

        if let Some(existing) = device_configs.iter_mut().find(|c| c.channel_id == config.channel_id) {
            *existing = config;
        } else {
            device_configs.push(config);
        }

        info!("Alarm configured for device {} channel {}", device_id, device_configs.last().unwrap().channel_id);
    }

    pub async fn get_alarms(&self, device_id: &str) -> Vec<AlarmConfig> {
        let configs = self.alarm_configs.lock().await;
        configs.get(device_id).cloned().unwrap_or_default()
    }

    pub async fn set_alarm_enabled(&self, device_id: &str, channel_id: i32, enabled: bool) {
        let mut configs = self.alarm_configs.lock().await;
        if let Some(device_configs) = configs.get_mut(device_id) {
            if let Some(config) = device_configs.iter_mut().find(|c| c.channel_id == channel_id) {
                config.enabled = enabled;
                info!("Alarm {} for device {} channel {}",
                    if enabled { "enabled" } else { "disabled" }, device_id, channel_id);
            }
        }
    }

    pub async fn check_data(&self, device_id: &str, data: &TelemetryData) {
        let configs = self.alarm_configs.lock().await;
        let Some(device_configs) = configs.get(device_id) else { return };

        for config in device_configs {
            if !config.enabled || config.channel_id as usize >= data.channels.len() {
                continue;
            }

            let value = data.channels[config.channel_id as usize];

            if config.lower_enabled && value < config.lower_limit {
                let msg = format!("Channel {} value {} below lower limit {}",
                    config.channel_id, value, config.lower_limit);
                self.trigger_alarm(device_id, config.channel_id, config.level, value, &msg).await;
            } else if config.upper_enabled && value > config.upper_limit {
                let msg = format!("Channel {} value {} above upper limit {}",
                    config.channel_id, value, config.upper_limit);
                self.trigger_alarm(device_id, config.channel_id, config.level, value, &msg).await;
            } else {
                let mut active = self.active_alarms.lock().await;
                if let Some(device_active) = active.get_mut(device_id) {
                    if device_active.get(&config.channel_id) == Some(&true) {
                        device_active.insert(config.channel_id, false);
                        info!("Alarm cleared for device {} channel {}", device_id, config.channel_id);
                    }
                }
            }
        }
    }

    pub async fn get_alarm_records(&self, device_id: &str) -> Vec<AlarmRecord> {
        let records = self.alarm_records.lock().await;
        records.get(device_id).cloned().unwrap_or_default()
    }

    pub async fn get_all_alarm_records(&self) -> Vec<AlarmRecord> {
        let records = self.alarm_records.lock().await;
        records.values().flat_map(|v| v.clone()).collect()
    }

    pub async fn acknowledge_alarm(&self, device_id: &str, channel_id: i32) {
        let mut records = self.alarm_records.lock().await;
        if let Some(device_records) = records.get_mut(device_id) {
            if let Some(record) = device_records.iter_mut()
                .find(|r| r.channel_id == channel_id && !r.acknowledged) {
                record.acknowledged = true;
                info!("Alarm acknowledged for device {} channel {}", device_id, channel_id);
            }
        }
    }

    pub async fn clear_records(&self, device_id: &str) {
        self.alarm_records.lock().await.remove(device_id);
        info!("Alarm records cleared for device: {}", device_id);
    }

    async fn trigger_alarm(&self, device_id: &str, channel_id: i32, level: AlarmLevel, value: f32, message: &str) {
        let mut active = self.active_alarms.lock().await;
        let device_active = active.entry(device_id.to_string()).or_insert_with(HashMap::new);

        if device_active.get(&channel_id) == Some(&true) {
            return;
        }
        device_active.insert(channel_id, true);
        drop(active);

        let record = AlarmRecord {
            device_id: device_id.to_string(),
            channel_id,
            level,
            value,
            message: message.to_string(),
            timestamp: Local::now(),
            acknowledged: false,
        };

        let mut records = self.alarm_records.lock().await;
        records.entry(device_id.to_string()).or_insert_with(Vec::new).push(record);

        warn!("Alarm triggered: {}", message);
    }
}
