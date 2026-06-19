pub mod routes;
pub mod handlers;
pub mod ws;
pub mod broadcast;
pub mod dto;

use crate::core::{DeviceManager, DataProcessor, AlarmSystem, DeviceEvent};
use crate::protocol::{MqttChannel, MqttEvent};
use crate::storage::{ConfigManager, DataLogger};
use std::sync::Arc;
use tokio::sync::{mpsc, Mutex as TokioMutex};

use broadcast::WsMessage;

/// 全局共享状态，注入到所有 axum handler
#[derive(Clone)]
pub struct AppState {
    pub device_manager: Arc<TokioMutex<DeviceManager>>,
    pub data_processor: Arc<DataProcessor>,
    pub alarm_system: Arc<AlarmSystem>,
    pub config_manager: Arc<TokioMutex<ConfigManager>>,
    pub data_logger: Arc<TokioMutex<DataLogger>>,
    pub mqtt_channel: Arc<TokioMutex<MqttChannel>>,
    pub device_event_tx: mpsc::Sender<DeviceEvent>,
    pub mqtt_event_tx: mpsc::Sender<MqttEvent>,
    pub ws_broadcast: tokio::sync::broadcast::Sender<WsMessage>,
    pub simulation_tx: Option<mpsc::Sender<MqttEvent>>,
    pub simulation_mode: bool,
}
