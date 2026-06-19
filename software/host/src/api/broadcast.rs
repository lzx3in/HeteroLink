use serde::Serialize;
use crate::api::dto::{DeviceInfoDto, ChannelStatsDto, AlarmRecordDto};

/// WebSocket 广播消息类型
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
