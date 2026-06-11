pub mod bridge;
pub use bridge::*;

use crate::core::{DeviceInfo, ChannelStats, AlarmRecord};
use crate::protocol::TelemetryData;
use crate::storage::{DataLogger, ConfigManager};
use crate::protocol::{MqttChannel, MqttEvent};
use crate::core::{DeviceManager, DataProcessor, AlarmSystem, DeviceEvent};
use std::sync::Arc;
use tokio::sync::{mpsc, Mutex as TokioMutex};
use slint::{ModelRc, VecModel, SharedString, Model};

pub fn setup_callbacks(
    _ui: &AppUI,
    _device_manager: Arc<TokioMutex<DeviceManager>>,
    _data_processor: Arc<DataProcessor>,
    _alarm_system: Arc<AlarmSystem>,
    _config_manager: Arc<TokioMutex<ConfigManager>>,
    _data_logger: Arc<TokioMutex<DataLogger>>,
    _mqtt_channel: Arc<TokioMutex<MqttChannel>>,
    _device_event_tx: mpsc::Sender<DeviceEvent>,
    _mqtt_event_tx: mpsc::Sender<MqttEvent>,
) {
    // TODO: wire callbacks
}

pub fn update_device_list(ui: &AppUI, devices: &[DeviceInfo]) {
    let items: Vec<DeviceItem> = devices.iter().map(|d| DeviceItem {
        id: d.id.clone().into(),
        name: d.name.clone().into(),
        connected: d.connected,
        online: d.online,
        connection_type: d.connection_type.clone().into(),
        port: d.port.clone().into(),
    }).collect();
    ui.set_devices(ModelRc::new(VecModel::from(items)));
}

pub fn update_telemetry_list(ui: &AppUI, data: &[TelemetryData]) {
    let items: Vec<TelemetrySnapshot> = data.iter().map(|d| TelemetrySnapshot {
        timestamp: d.timestamp as i32,
        channel_count: d.channels.len() as i32,
        ch0: d.channels.get(0).copied().unwrap_or(0.0),
        ch1: d.channels.get(1).copied().unwrap_or(0.0),
        ch2: d.channels.get(2).copied().unwrap_or(0.0),
        ch3: d.channels.get(3).copied().unwrap_or(0.0),
    }).collect();
    ui.set_telemetry_data(ModelRc::new(VecModel::from(items)));
}

pub fn update_stats_list(ui: &AppUI, stats: &[(i32, ChannelStats)]) {
    let items: Vec<StatsItem> = stats.iter().map(|(ch, s)| StatsItem {
        channel_id: *ch,
        min: s.min,
        max: s.max,
        avg: s.avg,
        rms: s.rms,
        sample_count: s.sample_count as i32,
    }).collect();
    ui.set_channel_stats(ModelRc::new(VecModel::from(items)));
}

pub fn update_alarm_list(ui: &AppUI, alarms: &[AlarmRecord]) {
    let items: Vec<AlarmItem> = alarms.iter().map(|a| AlarmItem {
        device_id: a.device_id.clone().into(),
        channel_id: a.channel_id,
        level: a.level.to_string().into(),
        value: a.value,
        message: a.message.clone().into(),
        timestamp: a.timestamp.format("%H:%M:%S").to_string().into(),
        acknowledged: a.acknowledged,
    }).collect();
    ui.set_alarm_records(ModelRc::new(VecModel::from(items)));
}

pub fn add_log_message(ui: &AppUI, msg: &str) {
    let mut logs: Vec<SharedString> = ui.get_log_messages().iter().collect();
    logs.push(msg.into());
    if logs.len() > 100 { logs.drain(0..logs.len() - 100); }
    ui.set_log_messages(ModelRc::new(VecModel::from(logs)));
}
