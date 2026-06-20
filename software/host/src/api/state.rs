use crate::infra::event_bus::EventBus;
use crate::services::{
    alarm_service::AlarmService,
    command_service::CommandService,
    config_service::ConfigService,
    device_service::DeviceService,
    export_service::ExportService,
    mqtt_service::MqttService,
    recording_service::RecordingService,
    telemetry_service::TelemetryService,
};

/// 全局共享状态，注入到所有 axum handler
///
/// 仅持有 service 层引用，不再直接暴露底层组件。
#[derive(Clone)]
pub struct AppState {
    pub device_service: DeviceService,
    pub telemetry_service: TelemetryService,
    pub alarm_service: AlarmService,
    pub command_service: CommandService,
    pub mqtt_service: MqttService,
    pub config_service: ConfigService,
    pub recording_service: RecordingService,
    pub export_service: ExportService,
    pub event_bus: EventBus,
}
