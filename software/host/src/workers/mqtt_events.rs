use tokio::sync::mpsc;
use tracing::info;

use crate::domain::TelemetryData;
use crate::events::DomainEvent;
use crate::infra::event_bus::EventBus;
use crate::protocol::MqttEvent;
use crate::services::device_service::DeviceService;
use crate::services::telemetry_service::TelemetryService;

/// 启动 MQTT 事件处理后台任务
pub fn spawn_mqtt_event_handler(
    device_service: DeviceService,
    telemetry_service: TelemetryService,
    event_bus: EventBus,
    mut rx: mpsc::Receiver<MqttEvent>,
) {
    tokio::spawn(async move {
        while let Some(event) = rx.recv().await {
            match event {
                MqttEvent::Connected => {
                    event_bus.emit(DomainEvent::MqttStatusChanged { connected: true });
                    event_bus.emit_log("MQTT 已连接");
                }
                MqttEvent::Disconnected => {
                    event_bus.emit(DomainEvent::MqttStatusChanged { connected: false });
                    event_bus.emit_log("MQTT 已断开");
                }
                MqttEvent::DeviceStatusReceived { device_id, online } => {
                    device_service
                        .update_device_online(&device_id, online, "MQTT")
                        .await;
                    let status = if online { "上线" } else { "下线" };
                    event_bus.emit_log(format!("设备 {} {}", device_id, status));
                }
                MqttEvent::TelemetryReceived { device_id, data } => {
                    info!("Telemetry from {}: {} bytes", device_id, data.len());
                    if let Some(telemetry) = parse_mqtt_telemetry(&data) {
                        telemetry_service.process_telemetry(&device_id, telemetry).await;
                    }
                }
                MqttEvent::CommandReceived { device_id, command } => {
                    event_bus.emit_log(format!("命令: {} -> {}", device_id, command));
                }
                MqttEvent::ResponseReceived {
                    device_id,
                    response,
                } => {
                    let display = format_command_response(&device_id, &response);
                    event_bus.emit(DomainEvent::CommandResponse {
                        device_id: device_id.clone(),
                        response: display.clone(),
                    });
                    event_bus.emit_log(format!("<- {}", display));
                }
                MqttEvent::MessageReceived { .. } => {}
                MqttEvent::Error(msg) => {
                    event_bus.emit_log(format!("MQTT 错误: {}", msg));
                    event_bus.emit(DomainEvent::Error { message: msg });
                }
            }
        }
    });
}

/// 解析 MQTT 遥测 JSON 为 TelemetryData
fn parse_mqtt_telemetry(data: &str) -> Option<TelemetryData> {
    let json: serde_json::Value = serde_json::from_str(data).ok()?;

    let mut channels = Vec::new();
    if let Some(ch_map) = json.get("channels").and_then(|c| c.as_object()) {
        for i in 1..=8 {
            let key = format!("ch{}", i);
            if let Some(val) = ch_map.get(&key).and_then(|v| v.as_f64()) {
                channels.push(val as f32);
            }
        }
    }
    if channels.is_empty() {
        return None;
    }

    let ts = json.get("ts").and_then(|v| v.as_u64()).unwrap_or(0) as u32;
    Some(TelemetryData {
        timestamp: ts,
        channels,
    })
}

/// 格式化命令响应显示文本
fn format_command_response(device_id: &str, response: &str) -> String {
    if let Ok(json) = serde_json::from_str::<serde_json::Value>(response) {
        let status = json.get("status").and_then(|v| v.as_str()).unwrap_or("?");
        let cmd = json.get("cmd").and_then(|v| v.as_str()).unwrap_or("?");
        let msg = json
            .get("message")
            .and_then(|v| v.as_str())
            .unwrap_or("");
        format!("{} [{}] {}: {}", device_id, status, cmd, msg)
    } else {
        format!("{}: {}", device_id, response)
    }
}
