use std::sync::Arc;
use tokio::sync::{mpsc, Mutex as TokioMutex};

use crate::infra::event_bus::EventBus;
use crate::protocol::{MqttChannel, MqttEvent};

/// 命令调度服务
///
/// 统一处理设备命令发送，支持真实 MQTT 和模拟两种模式。
/// 仅负责命令发送，不涉及 MQTT 连接管理（由 `MqttService` 负责）。
#[derive(Clone)]
pub struct CommandService {
    mqtt_channel: Arc<TokioMutex<MqttChannel>>,
    event_bus: EventBus,
    #[cfg(feature = "simulation")]
    simulation_tx: Option<mpsc::Sender<MqttEvent>>,
}

impl CommandService {
    #[cfg(feature = "simulation")]
    pub fn new(
        mqtt_channel: Arc<TokioMutex<MqttChannel>>,
        _mqtt_event_tx: mpsc::Sender<MqttEvent>,
        simulation_tx: Option<mpsc::Sender<MqttEvent>>,
        event_bus: EventBus,
    ) -> Self {
        Self {
            mqtt_channel,
            event_bus,
            simulation_tx,
        }
    }

    #[cfg(not(feature = "simulation"))]
    pub fn new(
        mqtt_channel: Arc<TokioMutex<MqttChannel>>,
        _mqtt_event_tx: mpsc::Sender<MqttEvent>,
        _simulation_tx: Option<mpsc::Sender<MqttEvent>>,
        event_bus: EventBus,
    ) -> Self {
        Self {
            mqtt_channel,
            event_bus,
        }
    }

    /// 发送命令到设备（自动区分模拟/真实模式）
    pub async fn send_command(&self, device_id: &str, cmd: &str, desc: &str) {
        #[cfg(feature = "simulation")]
        if let Some(ref sim_tx) = self.simulation_tx {
            let response = crate::sim::mock_responses::simulate_response(cmd);
            let _ = sim_tx
                .send(MqttEvent::ResponseReceived {
                    device_id: device_id.to_string(),
                    response,
                })
                .await;
            self.event_bus
                .emit_log(format!("[模拟] -> {} -> {}", desc, device_id));
            return;
        }

        let mqtt_ch = self.mqtt_channel.lock().await;
        match mqtt_ch.publish_command(device_id, cmd).await {
            Ok(_) => {
                self.event_bus
                    .emit_log(format!("-> 发送命令: {} -> {}", desc, device_id));
            }
            Err(e) => {
                self.event_bus.emit_log(format!("发送失败: {}", e));
            }
        }
    }
}
