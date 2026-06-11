use anyhow::Result;
use rumqttc::{AsyncClient, Event, Incoming, MqttOptions, QoS};
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::{mpsc, Mutex};
use tokio::time::sleep;
use tracing::{debug, error, info, warn};

#[derive(Debug, Clone)]
pub struct MqttConfig {
    pub broker_host: String,
    pub broker_port: u16,
    pub username: Option<String>,
    pub password: Option<String>,
    pub client_id: String,
    pub use_tls: bool,
}

impl Default for MqttConfig {
    fn default() -> Self {
        Self {
            broker_host: "broker.emqx.io".to_string(),
            broker_port: 1883,
            username: None,
            password: None,
            client_id: format!("heterolink-host-{}", &uuid::Uuid::new_v4().to_string()[..8]),
            use_tls: false,
        }
    }
}

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

pub struct MqttChannel {
    pub config: MqttConfig,
    client: Option<Arc<Mutex<AsyncClient>>>,
    connected: Arc<Mutex<bool>>,
    reconnect_attempts: Arc<Mutex<u32>>,
}

const MAX_RECONNECT_ATTEMPTS: u32 = 10;

/// Topics to auto-subscribe after connect
const SUBSCRIBE_TOPICS: &[&str] = &[
    "heterolink/subboard/+/status",
    "heterolink/subboard/+/telemetry",
    "heterolink/subboard/+/alarm",
];

impl MqttChannel {
    pub fn new(config: MqttConfig) -> Self {
        Self {
            config,
            client: None,
            connected: Arc::new(Mutex::new(false)),
            reconnect_attempts: Arc::new(Mutex::new(0)),
        }
    }

    pub fn update_config(&mut self, config: MqttConfig) {
        self.config = config;
    }

    pub async fn is_connected(&self) -> bool {
        *self.connected.lock().await
    }

    pub async fn connect(&mut self, event_tx: mpsc::Sender<MqttEvent>) -> Result<()> {
        // Disconnect existing
        if self.client.is_some() {
            let _ = self.disconnect().await;
        }

        let mut opts = MqttOptions::new(
            &self.config.client_id,
            &self.config.broker_host,
            self.config.broker_port,
        );

        if let (Some(user), Some(pass)) = (&self.config.username, &self.config.password) {
            if !user.is_empty() {
                opts.set_credentials(user, pass);
            }
        }

        opts.set_keep_alive(Duration::from_secs(10));
        opts.set_clean_session(true);

        let (client, mut eventloop) = AsyncClient::new(opts, 100);
        let client = Arc::new(Mutex::new(client));
        self.client = Some(client.clone());

        let connected = self.connected.clone();
        let reconnect_attempts = self.reconnect_attempts.clone();
        let broker_host = self.config.broker_host.clone();

        tokio::spawn(async move {
            loop {
                match eventloop.poll().await {
                    Ok(Event::Incoming(Incoming::ConnAck(ack))) => {
                        if ack.code == rumqttc::ConnectReturnCode::Success {
                            *connected.lock().await = true;
                            *reconnect_attempts.lock().await = 0;
                            info!("MQTT connected to {}", broker_host);
                            let _ = event_tx.send(MqttEvent::Connected).await;

                            // Auto-subscribe
                            let client = client.lock().await;
                            for topic in SUBSCRIBE_TOPICS {
                                match client.subscribe(*topic, QoS::AtLeastOnce).await {
                                    Ok(_) => info!("Subscribed: {}", topic),
                                    Err(e) => warn!("Subscribe failed for {}: {}", topic, e),
                                }
                            }
                        } else {
                            error!("MQTT ConnAck error: {:?}", ack.code);
                            let _ = event_tx.send(MqttEvent::Error(
                                format!("ConnAck: {:?}", ack.code)
                            )).await;
                        }
                    }
                    Ok(Event::Incoming(Incoming::Publish(publish))) => {
                        let topic = publish.topic.clone();
                        let payload = publish.payload.to_vec();
                        debug!("MQTT [{}]: {} bytes", topic, payload.len());

                        if topic.contains("/status") {
                            let device_id = Self::parse_device_id(&topic);
                            let online = String::from_utf8_lossy(&payload).contains("online");
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
                        info!("MQTT disconnected (server)");
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
                        let delay = std::cmp::min(1000u64 * (1 << attempts), 30000);
                        info!("Reconnecting in {}ms (attempt {})", delay, attempts + 1);
                        sleep(Duration::from_millis(delay)).await;
                    }
                }
            }
        });

        Ok(())
    }

    pub async fn disconnect(&mut self) -> Result<()> {
        if let Some(client) = &self.client {
            let client = client.lock().await;
            let _ = client.disconnect().await;
        }
        *self.connected.lock().await = false;
        self.client = None;
        *self.reconnect_attempts.lock().await = 0;
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

    pub async fn publish_command(&self, device_id: &str, command: &str) -> Result<()> {
        let topic = format!("heterolink/subboard/{}/command", device_id);
        self.publish(&topic, command.as_bytes(), QoS::AtLeastOnce, false).await
    }

    fn parse_device_id(topic: &str) -> String {
        topic.split('/').nth(2).unwrap_or("unknown").to_string()
    }
}
