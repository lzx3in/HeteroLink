use std::sync::Arc;
use tokio::sync::{mpsc, Mutex as TokioMutex};

use crate::domain::config::MqttConfig;
use crate::protocol::{MqttChannel, MqttEvent};

/// MQTT 连接管理服务
///
/// 仅负责 MQTT 连接生命周期管理（连接/断开/状态查询），
/// 不涉及命令发送（由 `CommandService` 负责）。
#[derive(Clone)]
pub struct MqttService {
    mqtt_channel: Arc<TokioMutex<MqttChannel>>,
    mqtt_event_tx: mpsc::Sender<MqttEvent>,
}

impl MqttService {
    pub fn new(
        mqtt_channel: Arc<TokioMutex<MqttChannel>>,
        mqtt_event_tx: mpsc::Sender<MqttEvent>,
    ) -> Self {
        Self {
            mqtt_channel,
            mqtt_event_tx,
        }
    }

    /// 连接 MQTT
    pub async fn connect(&self, config: MqttConfig) {
        let mut mqtt_ch = self.mqtt_channel.lock().await;
        mqtt_ch.update_config(config);
        mqtt_ch.connect(self.mqtt_event_tx.clone()).await;
    }

    /// 断开 MQTT
    pub async fn disconnect(&self) {
        self.mqtt_channel.lock().await.disconnect().await;
    }

    /// 查询 MQTT 连接状态
    pub async fn is_connected(&self) -> bool {
        self.mqtt_channel.lock().await.is_connected().await
    }
}
