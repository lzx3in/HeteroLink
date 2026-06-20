use chrono::{DateTime, Local};
use serde::{Deserialize, Serialize, Serializer};

/// 告警级别
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
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
#[derive(Debug, Clone, Serialize, Deserialize)]
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
#[derive(Debug, Clone, Serialize)]
pub struct AlarmRecord {
    pub device_id: String,
    pub channel_id: i32,
    pub level: AlarmLevel,
    pub value: f32,
    pub message: String,
    #[serde(serialize_with = "serialize_datetime")]
    pub timestamp: DateTime<Local>,
    pub acknowledged: bool,
}

fn serialize_datetime<S>(dt: &DateTime<Local>, serializer: S) -> Result<S::Ok, S::Error>
where
    S: Serializer,
{
    serializer.serialize_str(&dt.format("%Y-%m-%d %H:%M:%S").to_string())
}
