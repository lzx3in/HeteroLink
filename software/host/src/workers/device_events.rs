use tokio::sync::mpsc;
use tracing::error;

use crate::domain::{DeviceInfo, DeviceEvent};
use crate::events::DomainEvent;
use crate::infra::event_bus::EventBus;
use crate::services::device_service::DeviceService;
use crate::services::telemetry_service::TelemetryService;

/// 启动设备事件处理后台任务
pub fn spawn_device_event_handler(
    device_service: DeviceService,
    telemetry_service: TelemetryService,
    event_bus: EventBus,
    mut rx: mpsc::Receiver<DeviceEvent>,
) {
    tokio::spawn(async move {
        while let Some(event) = rx.recv().await {
            match event {
                DeviceEvent::DevicesChanged(devices) => {
                    let device_list: Vec<DeviceInfo> =
                        devices.into_values().collect();
                    let connected = device_list.iter().filter(|d| d.connected).count();
                    let online = device_list.iter().filter(|d| d.online).count();
                    event_bus.emit(DomainEvent::DeviceListChanged {
                        devices: device_list,
                        connected_count: connected,
                        online_count: online,
                    });
                    event_bus.emit_log(format!("{} 已连接, {} 在线", connected, online));
                }
                DeviceEvent::TelemetryReceived { device_id, data } => {
                    telemetry_service.process_telemetry(&device_id, data).await;
                }
                DeviceEvent::DeviceStatusChanged {
                    device_id,
                    connected,
                    online,
                } => {
                    let status = if online {
                        "在线"
                    } else if connected {
                        "已连接"
                    } else {
                        "离线"
                    };
                    event_bus.emit_log(format!("设备 {} {}", device_id, status));
                    event_bus.emit(DomainEvent::DeviceStatusChanged {
                        device_id,
                        connected,
                        online,
                    });
                    device_service.broadcast_device_list().await;
                }
                DeviceEvent::DeviceError {
                    device_id,
                    error: err_msg,
                } => {
                    error!("Device {} error: {}", device_id, err_msg);
                    event_bus.emit_log(format!("设备 {} 错误: {}", device_id, err_msg));
                    event_bus.emit(DomainEvent::Error {
                        message: format!("设备 {} 错误: {}", device_id, err_msg),
                    });
                }
            }
        }
    });
}
