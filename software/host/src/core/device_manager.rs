use crate::protocol::{MqttChannel, TelemetryData};
use anyhow::Result;
use serde::Serialize;
use std::collections::HashMap;
use std::sync::Arc;
use std::time::{SystemTime, UNIX_EPOCH};
use tokio::sync::{mpsc, Mutex};
use tracing::info;

/// 设备信息
#[derive(Debug, Clone, Serialize)]
pub struct DeviceInfo {
    pub id: String,
    pub name: String,
    pub connected: bool,
    pub online: bool,
    pub connection_type: String,
    pub port: String,
    pub last_seen: u64,
    pub metadata: HashMap<String, String>,
}

impl DeviceInfo {
    pub fn new(id: String, name: String) -> Self {
        Self {
            id,
            name,
            connected: false,
            online: false,
            connection_type: String::new(),
            port: String::new(),
            last_seen: 0,
            metadata: HashMap::new(),
        }
    }
}

/// 设备管理器事件
#[derive(Debug)]
pub enum DeviceEvent {
    DevicesChanged(HashMap<String, DeviceInfo>),
    DeviceStatusChanged { device_id: String, connected: bool, online: bool },
    TelemetryReceived { device_id: String, data: TelemetryData },
    DeviceError { device_id: String, error: String },
}

/// 设备管理器
pub struct DeviceManager {
    devices: Arc<Mutex<HashMap<String, DeviceInfo>>>,
    mqtt_channel: Option<Arc<Mutex<MqttChannel>>>,
    heartbeat_tx: Option<mpsc::Sender<String>>,
}

impl DeviceManager {
    pub fn new() -> Self {
        Self {
            devices: Arc::new(Mutex::new(HashMap::new())),
            mqtt_channel: None,
            heartbeat_tx: None,
        }
    }

    pub async fn get_devices(&self) -> HashMap<String, DeviceInfo> {
        self.devices.lock().await.clone()
    }

    pub async fn get_device(&self, device_id: &str) -> Option<DeviceInfo> {
        self.devices.lock().await.get(device_id).cloned()
    }

    pub async fn add_device(&self, device: DeviceInfo) -> Result<()> {
        let mut devices = self.devices.lock().await;
        if devices.contains_key(&device.id) {
            return Err(anyhow::anyhow!("Device already exists: {}", device.id));
        }
        info!("Device added: {}", device.id);
        devices.insert(device.id.clone(), device);
        Ok(())
    }

    pub async fn remove_device(&self, device_id: &str) -> Result<()> {
        let mut devices = self.devices.lock().await;
        if devices.remove(device_id).is_none() {
            return Err(anyhow::anyhow!("Device not found: {}", device_id));
        }
        info!("Device removed: {}", device_id);
        Ok(())
    }

    pub async fn disconnect_device(&self, device_id: &str) -> Result<()> {
        let mut devices = self.devices.lock().await;
        if let Some(device) = devices.get_mut(device_id) {
            device.connected = false;
        }

        info!("Device disconnected: {}", device_id);
        Ok(())
    }

    pub fn set_mqtt_channel(&mut self, channel: MqttChannel) {
        self.mqtt_channel = Some(Arc::new(Mutex::new(channel)));
    }

    pub async fn connect_device_mqtt(
        &self,
        device_id: &str,
        broker_host: &str,
        broker_port: u16,
        _event_tx: mpsc::Sender<DeviceEvent>,
    ) -> Result<()> {
        if self.mqtt_channel.is_none() {
            return Err(anyhow::anyhow!("MQTT channel not configured"));
        }

        let mut devices = self.devices.lock().await;
        if devices.contains_key(device_id) {
            return Err(anyhow::anyhow!("Device already exists: {}", device_id));
        }

        let mut device = DeviceInfo::new(device_id.to_string(), format!("MQTT Device {}", device_id));
        device.connection_type = "MQTT".to_string();
        device.port = format!("{}:{}", broker_host, broker_port);
        device.last_seen = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() as u64;

        let mqtt_connected = {
            if let Some(mqtt) = &self.mqtt_channel {
                mqtt.lock().await.is_connected().await
            } else {
                false
            }
        };
        device.connected = mqtt_connected;

        devices.insert(device_id.to_string(), device);
        drop(devices);

        // 订阅该设备的命令和状态
        if let Some(mqtt) = &self.mqtt_channel {
            let _mqtt = mqtt.lock().await;
        }

        info!("MQTT device added: {}", device_id);
        Ok(())
    }

    pub async fn send_heartbeat(&self, _device_id: &str) -> Result<()> {
        // 心跳通过 MQTT 通道处理，此处无需操作
        Ok(())
    }
}
