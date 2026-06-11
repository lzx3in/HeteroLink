use anyhow::Result;
use rumqttc::{AsyncClient, Event, Incoming, MqttOptions, QoS};
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::{mpsc, Mutex};
use tokio::time::sleep;
use tracing::{debug, error, info};

/// MQTT 通道配置
#[derive(Debug, Clone)]
pub struct MqttConfig {
    pub broker_host: String,
    pub broker_port: u16,
    pub username: Option<String>,
    pub password: Option<String>,
    pub client_id: String,
    pub use_tls: bool,
    pub will_topic: Option<String>,
    pub will_message: Option<String>,
}

impl Default for MqttConfig {
    fn default() -> Self {
        Self {
            broker_host: "localhost".to_string(),
            broker_port: 1883,
            username: None,
            password: None,
            client_id: format!("heterolink-host-{}", uuid::Uuid::new_v4().to_string()[..8].to_string()),
            use_tls: false,
            will_topic: None,
            will_message: None,
        }
    }
}

/// MQTT 通道事件
#[derive(Debug, Clone)]
pub enum MqttEvent {
    Connected,
    Disconnected,
    MessageReceived { topic: String, payload: Vec<u8> },
    Error(String),
    DeviceStatusReceived { device_id: String, online: bool },
    TelemetryReceived { device_id: String, data: String },
    CommandReceived { device_id: String, command: String },
}

/// MQTT 通道
pub struct MqttChannel {
    config: MqttConfig,
    client: Option<Arc<Mutex<AsyncClient>>>,
    connected: Arc<Mutex<bool>>,
    reconnect_attempts: Arc<Mutex<u32>>,
}

const MAX_RECONNECT_ATTEMPTS: u32 = 5;

impl MqttChannel {
    pub fn new(config: MqttConfig) -> Self {
        Self {
            config,
            client: None,
            connected: Arc::new(Mutex::new(false)),
            reconnect_attempts: Arc::new(Mutex::new(0)),
        }
    }

    pub async fn is_connected(&self) -> bool {
        *self.connected.lock().await
    }

    pub async fn connect(&mut self, event_tx: mpsc::Sender<MqttEvent>) -> Result<()> {
        let mut mqttoptions = MqttOptions::new(
            &self.config.client_id,
            &self.config.broker_host,
            self.config.broker_port,
        );

        if let (Some(user), Some(pass)) = (&self.config.username, &self.config.password) {
            mqttoptions.set_credentials(user, pass);
        }

        if let (Some(topic), Some(msg)) = (&self.config.will_topic, &self.config.will_message) {
            let will = rumqttc::LastWill::new(
                topic.clone(),
                msg.as_bytes().to_vec(),
                QoS::AtLeastOnce,
                true,
            );
            mqttoptions.set_last_will(will);
        }

        mqttoptions.set_keep_alive(Duration::from_secs(5));

        let (client, mut eventloop) = AsyncClient::new(mqttoptions, 10);
        let client = Arc::new(Mutex::new(client));
        self.client = Some(client.clone());

        let connected = self.connected.clone();
        let reconnect_attempts = self.reconnect_attempts.clone();

        tokio::spawn(async move {
            loop {
                match eventloop.poll().await {
                    Ok(Event::Incoming(Incoming::ConnAck(_))) => {
                        *connected.lock().await = true;
                        *reconnect_attempts.lock().await = 0;
                        info!("MQTT connected");
                        let _ = event_tx.send(MqttEvent::Connected).await;
                    }
                    Ok(Event::Incoming(Incoming::Publish(publish))) => {
                        let topic = publish.topic.clone();
                        let payload = publish.payload.to_vec();
                        debug!("MQTT message on topic: {}", topic);

                        // 解析消息类型
                        if topic.contains("/status") {
                            let device_id = Self::parse_device_id(&topic);
                            let online = payload == b"online";
                            let _ = event_tx.send(MqttEvent::DeviceStatusReceived {
                                device_id, online,
                            }).await;
                        } else if topic.contains("/telemetry") {
                            let device_id = Self::parse_device_id(&topic);
                            let data = String::from_utf8_lossy(&payload).to_string();
                            let _ = event_tx.send(MqttEvent::TelemetryReceived {
                                device_id, data,
                            }).await;
                        } else if topic.contains("/command") {
                            let device_id = Self::parse_device_id(&topic);
                            let command = String::from_utf8_lossy(&payload).to_string();
                            let _ = event_tx.send(MqttEvent::CommandReceived {
                                device_id, command,
                            }).await;
                        }

                        let _ = event_tx.send(MqttEvent::MessageReceived { topic, payload }).await;
                    }
                    Ok(Event::Incoming(Incoming::Disconnect)) => {
                        *connected.lock().await = false;
                        info!("MQTT disconnected");
                        let _ = event_tx.send(MqttEvent::Disconnected).await;
                    }
                    Ok(_) => {}
                    Err(e) => {
                        *connected.lock().await = false;
                        error!("MQTT error: {}", e);
                        let _ = event_tx.send(MqttEvent::Error(e.to_string())).await;

                        let attempts = *reconnect_attempts.lock().await;
                        if attempts >= MAX_RECONNECT_ATTEMPTS {
                            error!("Max reconnection attempts reached");
                            break;
                        }

                        *reconnect_attempts.lock().await = attempts + 1;
                        let delay = std::cmp::min(1000 * (1 << attempts), 30000);
                        info!("Reconnecting in {}ms (attempt {})", delay, attempts + 1);
                        sleep(Duration::from_millis(delay as u64)).await;
                    }
                }
            }
        });

        Ok(())
    }

    pub async fn disconnect(&mut self) -> Result<()> {
        if let Some(client) = &self.client {
            let client = client.lock().await;
            client.disconnect().await?;
        }
        *self.connected.lock().await = false;
        self.client = None;
        info!("MQTT disconnected");
        Ok(())
    }

    pub async fn publish(&self, topic: &str, payload: &[u8], qos: QoS, retain: bool) -> Result<()> {
        if let Some(client) = &self.client {
            let client = client.lock().await;
            client.publish(topic, qos, retain, payload).await?;
            Ok(())
        } else {
            Err(anyhow::anyhow!("MQTT client not connected"))
        }
    }

    pub async fn subscribe(&self, topic: &str, qos: QoS) -> Result<()> {
        if let Some(client) = &self.client {
            let client = client.lock().await;
            client.subscribe(topic, qos).await?;
            Ok(())
        } else {
            Err(anyhow::anyhow!("MQTT client not connected"))
        }
    }

    pub async fn publish_device_status(&self, device_id: &str, online: bool) -> Result<()> {
        let topic = format!("heterolink/subboard/{}/status", device_id);
        let msg = if online { "online" } else { "offline" };
        self.publish(&topic, msg.as_bytes(), QoS::AtLeastOnce, true).await
    }

    pub async fn publish_telemetry(&self, device_id: &str, data: &str) -> Result<()> {
        let topic = format!("heterolink/subboard/{}/telemetry", device_id);
        self.publish(&topic, data.as_bytes(), QoS::AtLeastOnce, false).await
    }

    pub async fn publish_command(&self, device_id: &str, command: &str) -> Result<()> {
        let topic = format!("heterolink/subboard/{}/command", device_id);
        self.publish(&topic, command.as_bytes(), QoS::AtLeastOnce, false).await
    }

    pub async fn subscribe_device_commands(&self, device_id: &str) -> Result<()> {
        let topic = format!("heterolink/subboard/{}/command", device_id);
        self.subscribe(&topic, QoS::AtLeastOnce).await
    }

    pub async fn subscribe_all_device_status(&self) -> Result<()> {
        self.subscribe("heterolink/subboard/+/status", QoS::AtLeastOnce).await
    }

    pub async fn subscribe_all_device_telemetry(&self) -> Result<()> {
        self.subscribe("heterolink/subboard/+/telemetry", QoS::AtLeastOnce).await
    }

    fn parse_device_id(topic: &str) -> String {
        topic.split('/').nth(2).unwrap_or("").to_string()
    }
}
