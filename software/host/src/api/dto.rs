use serde::{Deserialize, Serialize};
use crate::domain::{DeviceInfo, ChannelStats, AlarmRecord, AlarmLevel};

// --- 设备 ---

#[derive(Debug, Clone, Serialize)]
pub struct DeviceInfoDto {
    pub id: String,
    pub name: String,
    pub connected: bool,
    pub online: bool,
    pub connection_type: String,
    pub port: String,
    pub last_seen: u64,
}

impl From<&DeviceInfo> for DeviceInfoDto {
    fn from(d: &DeviceInfo) -> Self {
        Self {
            id: d.id.clone(),
            name: d.name.clone(),
            connected: d.connected,
            online: d.online,
            connection_type: d.connection_type.clone(),
            port: d.port.clone(),
            last_seen: d.last_seen,
        }
    }
}

impl From<DeviceInfo> for DeviceInfoDto {
    fn from(d: DeviceInfo) -> Self {
        Self::from(&d)
    }
}

#[derive(Debug, Deserialize)]
pub struct AddDeviceRequest {
    pub name: String,
}

// --- 遥测 ---

#[derive(Debug, Clone, Serialize)]
pub struct TelemetryDto {
    pub timestamp: u32,
    pub channels: Vec<f32>,
}

// --- 统计 ---

#[derive(Debug, Clone, Serialize)]
pub struct ChannelStatsDto {
    pub channel_id: i32,
    pub min: f32,
    pub max: f32,
    pub avg: f32,
    pub rms: f32,
    pub sample_count: u32,
}

impl From<(i32, ChannelStats)> for ChannelStatsDto {
    fn from((ch, s): (i32, ChannelStats)) -> Self {
        Self {
            channel_id: ch,
            min: s.min,
            max: s.max,
            avg: s.avg,
            rms: s.rms,
            sample_count: s.sample_count,
        }
    }
}

// --- 告警 ---

#[derive(Debug, Clone, Serialize)]
pub struct AlarmRecordDto {
    pub device_id: String,
    pub channel_id: i32,
    pub level: String,
    pub value: f32,
    pub message: String,
    pub timestamp: String,
    pub acknowledged: bool,
}

impl From<&AlarmRecord> for AlarmRecordDto {
    fn from(a: &AlarmRecord) -> Self {
        Self {
            device_id: a.device_id.clone(),
            channel_id: a.channel_id,
            level: match a.level {
                AlarmLevel::Info => "INFO".to_string(),
                AlarmLevel::Warning => "WARNING".to_string(),
                AlarmLevel::Critical => "CRITICAL".to_string(),
            },
            value: a.value,
            message: a.message.clone(),
            timestamp: a.timestamp.format("%H:%M:%S").to_string(),
            acknowledged: a.acknowledged,
        }
    }
}

impl From<AlarmRecord> for AlarmRecordDto {
    fn from(a: AlarmRecord) -> Self {
        Self::from(&a)
    }
}

#[derive(Debug, Deserialize)]
pub struct AlarmConfigRequest {
    pub channel_id: i32,
    pub lower_limit: f32,
    pub upper_limit: f32,
    pub lower_enabled: bool,
    pub upper_enabled: bool,
    pub enabled: bool,
}

// --- MQTT ---

#[derive(Debug, Deserialize)]
pub struct MqttConnectRequest {
    pub broker_host: String,
    pub broker_port: u16,
    pub username: Option<String>,
    pub password: Option<String>,
    pub client_id: Option<String>,
    #[serde(default)]
    pub use_tls: bool,
}

// --- 命令 ---

#[derive(Debug, Deserialize)]
pub struct StartCommandRequest {
    pub sample_rate: u32,
}

#[derive(Debug, Deserialize)]
pub struct GpioCommandRequest {
    pub channel: i32,
    pub value: bool,
}

// --- 录制 ---

#[derive(Debug, Deserialize)]
pub struct StartRecordingRequest {
    pub device_id: String,
    pub path: Option<String>,
}

// --- 更新 ---

#[derive(Debug, Clone, Serialize)]
pub struct UpdateStatusDto {
    pub status: String,
    pub current_version: String,
    pub new_version: Option<String>,
    pub body: Option<String>,
    pub progress: Option<f32>,
    pub message: Option<String>,
}

impl From<(&str, &crate::services::updater_service::UpdateStatus)> for UpdateStatusDto {
    fn from((current, s): (&str, &crate::services::updater_service::UpdateStatus)) -> Self {
        use crate::services::updater_service::UpdateStatus;
        match s {
            UpdateStatus::Idle => Self {
                status: "idle".into(), current_version: current.into(),
                new_version: None, body: None, progress: None, message: None,
            },
            UpdateStatus::Checking => Self {
                status: "checking".into(), current_version: current.into(),
                new_version: None, body: None, progress: None, message: None,
            },
            UpdateStatus::Available { version, body, .. } => Self {
                status: "available".into(), current_version: current.into(),
                new_version: Some(version.clone()), body: Some(body.clone()),
                progress: None, message: None,
            },
            UpdateStatus::Downloading { progress } => Self {
                status: "downloading".into(), current_version: current.into(),
                new_version: None, body: None, progress: Some(*progress), message: None,
            },
            UpdateStatus::Ready { version } => Self {
                status: "ready".into(), current_version: current.into(),
                new_version: Some(version.clone()), body: None, progress: None,
                message: Some("更新已就绪，请重启服务".into()),
            },
            UpdateStatus::UpToDate => Self {
                status: "up_to_date".into(), current_version: current.into(),
                new_version: None, body: None, progress: None, message: None,
            },
            UpdateStatus::Error(msg) => Self {
                status: "error".into(), current_version: current.into(),
                new_version: None, body: None, progress: None,
                message: Some(msg.clone()),
            },
        }
    }
}

// --- 通用响应 ---

#[derive(Debug, Serialize)]
pub struct ApiResponse<T: Serialize> {
    pub success: bool,
    pub data: Option<T>,
    pub message: Option<String>,
}

impl<T: Serialize> ApiResponse<T> {
    pub fn ok(data: T) -> Self {
        Self {
            success: true,
            data: Some(data),
            message: None,
        }
    }

    pub fn error(msg: &str) -> Self {
        Self {
            success: false,
            data: None,
            message: Some(msg.to_string()),
        }
    }
}

impl ApiResponse<()> {
    pub fn ok_message(msg: &str) -> Self {
        Self {
            success: true,
            data: None,
            message: Some(msg.to_string()),
        }
    }
}
