use serde::Serialize;
use crate::api::dto::{DeviceInfoDto, ChannelStatsDto, AlarmRecordDto};
use crate::events::DomainEvent;

/// WebSocket 广播消息类型（API 线格式）
///
/// 仅在 `api` 层使用，与领域事件 (`DomainEvent`) 通过 `from_domain` 桥接。
#[derive(Debug, Clone, Serialize)]
#[serde(tag = "type", content = "data")]
pub enum WsMessage {
    /// 遥测数据推送
    Telemetry {
        device_id: String,
        timestamp: u32,
        channels: Vec<f32>,
    },
    /// 设备列表变更
    DeviceListChanged {
        devices: Vec<DeviceInfoDto>,
        connected_count: usize,
        online_count: usize,
    },
    /// 设备状态变更
    DeviceStatusChanged {
        device_id: String,
        connected: bool,
        online: bool,
    },
    /// 统计数据更新
    StatsUpdated {
        device_id: String,
        stats: Vec<ChannelStatsDto>,
    },
    /// 告警触发
    AlarmTriggered {
        alarm: AlarmRecordDto,
    },
    /// 告警列表变更
    AlarmsChanged {
        device_id: String,
        alarms: Vec<AlarmRecordDto>,
    },
    /// MQTT 状态变更
    MqttStatusChanged {
        connected: bool,
    },
    /// 命令响应
    CommandResponse {
        device_id: String,
        response: String,
    },
    /// 日志消息
    Log {
        message: String,
        timestamp: String,
    },
    /// 录制状态变更
    RecordingChanged {
        recording: bool,
    },
    /// 错误
    Error {
        message: String,
    },
}

impl WsMessage {
    /// 从领域事件转换为 API 线格式
    pub fn from_domain(event: &DomainEvent) -> Option<Self> {
        Some(match event {
            DomainEvent::Telemetry { device_id, data } => WsMessage::Telemetry {
                device_id: device_id.clone(),
                timestamp: data.timestamp,
                channels: data.channels.clone(),
            },
            DomainEvent::DeviceListChanged { devices, connected_count, online_count } => {
                WsMessage::DeviceListChanged {
                    devices: devices.iter().map(DeviceInfoDto::from).collect(),
                    connected_count: *connected_count,
                    online_count: *online_count,
                }
            }
            DomainEvent::DeviceStatusChanged { device_id, connected, online } => {
                WsMessage::DeviceStatusChanged {
                    device_id: device_id.clone(),
                    connected: *connected,
                    online: *online,
                }
            }
            DomainEvent::StatsUpdated { device_id, stats } => WsMessage::StatsUpdated {
                device_id: device_id.clone(),
                stats: stats.iter().cloned().map(ChannelStatsDto::from).collect(),
            },
            DomainEvent::AlarmTriggered { alarm } => WsMessage::AlarmTriggered {
                alarm: AlarmRecordDto::from(alarm),
            },
            DomainEvent::AlarmsChanged { device_id, alarms } => WsMessage::AlarmsChanged {
                device_id: device_id.clone(),
                alarms: alarms.iter().map(AlarmRecordDto::from).collect(),
            },
            DomainEvent::MqttStatusChanged { connected } => WsMessage::MqttStatusChanged {
                connected: *connected,
            },
            DomainEvent::CommandResponse { device_id, response } => WsMessage::CommandResponse {
                device_id: device_id.clone(),
                response: response.clone(),
            },
            DomainEvent::Log { message, timestamp } => WsMessage::Log {
                message: message.clone(),
                timestamp: timestamp.clone(),
            },
            DomainEvent::RecordingChanged { recording } => WsMessage::RecordingChanged {
                recording: *recording,
            },
            DomainEvent::Error { message } => WsMessage::Error {
                message: message.clone(),
            },
        })
    }
}
