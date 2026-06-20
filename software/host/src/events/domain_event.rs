use crate::domain::{TelemetryData, DeviceInfo, AlarmRecord, ChannelStats};

/// 领域事件 — EventBus 的传输载荷
///
/// 与传输协议无关，不感知 WebSocket/HTTP/SSE。
/// 所有字段使用 `domain::*` 类型，DTO 转换推迟到 API 边界。
#[derive(Debug, Clone)]
pub enum DomainEvent {
    /// 遥测数据推送
    Telemetry {
        device_id: String,
        data: TelemetryData,
    },
    /// 设备列表变更
    DeviceListChanged {
        devices: Vec<DeviceInfo>,
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
        stats: Vec<(i32, ChannelStats)>,
    },
    /// 告警触发
    AlarmTriggered {
        alarm: AlarmRecord,
    },
    /// 告警列表变更
    AlarmsChanged {
        device_id: String,
        alarms: Vec<AlarmRecord>,
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
    /// 录制状态变更
    RecordingChanged {
        recording: bool,
    },
    /// 日志消息
    Log {
        message: String,
        timestamp: String,
    },
    /// 错误
    Error {
        message: String,
    },
    /// 发现新版本
    UpdateAvailable {
        version: String,
        body: String,
    },
}
