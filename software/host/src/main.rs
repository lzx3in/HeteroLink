use anyhow::Result;
use std::sync::Arc;
use tokio::sync::mpsc;
use tokio::sync::Mutex as TokioMutex;
use tracing::{info, warn, error};
use slint::ComponentHandle;

mod protocol;
mod core;
mod storage;
mod ui;

use crate::core::{DeviceManager, DataProcessor, AlarmSystem, DeviceInfo, DeviceEvent};
use crate::protocol::{MqttChannel, MqttConfig, MqttEvent};
use crate::storage::{ConfigManager, DataLogger};
use crate::ui::AppUI;

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_max_level(tracing::Level::INFO)
        .init();

    info!("Starting HeteroLink Host...");

    let device_manager = Arc::new(TokioMutex::new(DeviceManager::new()));
    let data_processor = Arc::new(DataProcessor::new());
    let alarm_system = Arc::new(AlarmSystem::new());
    let config_manager = Arc::new(TokioMutex::new(ConfigManager::new()));
    let data_logger = Arc::new(TokioMutex::new(DataLogger::new()));

    // Load config
    {
        let mut config = config_manager.lock().await;
        if let Err(e) = config.load(None) {
            warn!("Failed to load config: {}", e);
        }
    }

    let mqtt_config = {
        let config = config_manager.lock().await;
        let cfg = config.get();
        MqttConfig {
            broker_host: cfg.mqtt.broker_host.clone(),
            broker_port: cfg.mqtt.broker_port,
            username: if cfg.mqtt.username.is_empty() { None } else { Some(cfg.mqtt.username.clone()) },
            password: if cfg.mqtt.password.is_empty() { None } else { Some(cfg.mqtt.password.clone()) },
            client_id: cfg.mqtt.client_id.clone(),
            use_tls: cfg.mqtt.use_tls,
            will_topic: None,
            will_message: None,
        }
    };
    let mqtt_channel = Arc::new(TokioMutex::new(MqttChannel::new(mqtt_config)));

    let (device_event_tx, mut device_event_rx) = mpsc::channel::<DeviceEvent>(200);
    let (mqtt_event_tx, mut mqtt_event_rx) = mpsc::channel::<MqttEvent>(200);

    // Create UI (must be on main thread)
    let ui = AppUI::new()?;
    let ui_weak = ui.as_weak();

    // Setup callbacks on UI thread
    ui::setup_callbacks(
        &ui,
        device_manager.clone(),
        data_processor.clone(),
        alarm_system.clone(),
        config_manager.clone(),
        data_logger.clone(),
        mqtt_channel.clone(),
        device_event_tx.clone(),
        mqtt_event_tx.clone(),
    );

    // Update available ports
    if let Ok(ports) = serialport::available_ports() {
        let port_names: Vec<slint::SharedString> = ports.iter().map(|p| p.port_name.clone().into()).collect();
        ui.set_available_ports(slint::ModelRc::new(slint::VecModel::from(port_names)));
        info!("Available ports: {:?}", ports.iter().map(|p| &p.port_name).collect::<Vec<_>>());
    }

    // Device event handler (background tokio task, posts to UI thread)
    let dm_dev = device_manager.clone();
    let dp_dev = data_processor.clone();
    let as_dev = alarm_system.clone();
    let dl_dev = data_logger.clone();
    let uw_dev = ui_weak.clone();

    tokio::spawn(async move {
        while let Some(event) = device_event_rx.recv().await {
            match event {
                DeviceEvent::DevicesChanged(devices) => {
                    let device_list: Vec<DeviceInfo> = devices.values().cloned().collect();
                    let connected = device_list.iter().filter(|d| d.connected).count() as i32;
                    let online = device_list.iter().filter(|d| d.online).count() as i32;
                    let uw = uw_dev.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::update_device_list(&ui, &device_list);
                            ui.set_status_message(format!("{} 已连接, {} 在线", connected, online).into());
                            ui.set_connected_count(connected);
                            ui.set_online_count(online);
                        }
                    }).ok();
                }
                DeviceEvent::TelemetryReceived { device_id, data } => {
                    dp_dev.add_data(&device_id, data.clone()).await;
                    as_dev.check_data(&device_id, &data).await;
                    {
                        let mut logger = dl_dev.lock().await;
                        logger.write_data(&device_id, &data);
                    }
                    let latest = dp_dev.get_latest_data(&device_id, 50).await;
                    let stats = dp_dev.get_stats(&device_id).await;
                    let stats_vec: Vec<(i32, crate::core::ChannelStats)> = stats.into_iter().collect();
                    let alarms = as_dev.get_alarm_records(&device_id).await;
                    let uw = uw_dev.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::update_telemetry_list(&ui, &latest);
                            ui::update_stats_list(&ui, &stats_vec);
                            ui::update_alarm_list(&ui, &alarms);
                        }
                    }).ok();
                }
                DeviceEvent::DeviceStatusChanged { device_id, connected, online } => {
                    let devices = dm_dev.lock().await.get_devices().await;
                    let device_list: Vec<DeviceInfo> = devices.values().cloned().collect();
                    let status = if online { "在线" } else if connected { "已连接" } else { "离线" };
                    let msg = format!("设备 {} {}", device_id, status);
                    let uw = uw_dev.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::update_device_list(&ui, &device_list);
                            ui::add_log_message(&ui, &msg);
                        }
                    }).ok();
                }
                DeviceEvent::DeviceError { device_id, error: err_msg } => {
                    let msg = format!("设备 {} 错误: {}", device_id, err_msg);
                    let uw = uw_dev.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::add_log_message(&ui, &msg);
                        }
                    }).ok();
                    error!("Device {} error: {}", device_id, err_msg);
                }
            }
        }
    });

    // MQTT event handler
    let dm_mqtt = device_manager.clone();
    let uw_mqtt = ui_weak.clone();

    tokio::spawn(async move {
        while let Some(event) = mqtt_event_rx.recv().await {
            match event {
                MqttEvent::Connected => {
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui.set_mqtt_connected(true);
                            ui::add_log_message(&ui, "MQTT 已连接");
                        }
                    }).ok();
                }
                MqttEvent::Disconnected => {
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui.set_mqtt_connected(false);
                            ui::add_log_message(&ui, "MQTT 已断开");
                        }
                    }).ok();
                }
                MqttEvent::DeviceStatusReceived { device_id, online } => {
                    let devices = dm_mqtt.lock().await.get_devices().await;
                    if let Some(mut device) = devices.get(&device_id).cloned() {
                        device.online = online;
                        let _ = dm_mqtt.lock().await.add_device(device).await;
                    }
                    let msg = format!("MQTT 设备 {} {}", device_id, if online { "上线" } else { "下线" });
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::add_log_message(&ui, &msg);
                        }
                    }).ok();
                }
                MqttEvent::TelemetryReceived { device_id, data } => {
                    let msg = format!("MQTT 遥测: {} -> {}...", device_id, &data[..data.len().min(60)]);
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::add_log_message(&ui, &msg);
                        }
                    }).ok();
                }
                MqttEvent::CommandReceived { device_id, command } => {
                    let msg = format!("MQTT 命令: {} -> {}", device_id, command);
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::add_log_message(&ui, &msg);
                        }
                    }).ok();
                }
                MqttEvent::MessageReceived { .. } => {}
                MqttEvent::Error(msg) => {
                    let uw = uw_mqtt.clone();
                    let err_msg = format!("MQTT 错误: {}", msg);
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::add_log_message(&ui, &err_msg);
                        }
                    }).ok();
                }
            }
        }
    });

    // Heartbeat task
    let dm_hb = device_manager.clone();
    tokio::spawn(async move {
        let mut interval = tokio::time::interval(tokio::time::Duration::from_secs(5));
        loop {
            interval.tick().await;
            let devices = dm_hb.lock().await.get_devices().await;
            for device_id in devices.keys() {
                let _ = dm_hb.lock().await.send_heartbeat(device_id).await;
            }
        }
    });

    info!("UI created, window reference: {:?}", ui.as_weak().upgrade().is_some());
    
    // Explicitly show the window
    ui.show().map_err(|e| anyhow::anyhow!("Failed to show window: {}", e))?;
    info!("Window shown");
    
    // Run the event loop
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| slint::run_event_loop())) {
        Ok(Ok(())) => info!("Event loop exited normally"),
        Ok(Err(e)) => {
            error!("Event loop error: {}", e);
            return Err(anyhow::anyhow!("Event loop error: {}", e));
        }
        Err(panic) => {
            let msg = if let Some(s) = panic.downcast_ref::<&str>() { s.to_string() }
                      else if let Some(s) = panic.downcast_ref::<String>() { s.clone() }
                      else { "unknown panic".to_string() };
            error!("Event loop panicked: {}", msg);
            return Err(anyhow::anyhow!("Event loop panic: {}", msg));
        }
    }

    info!("HeteroLink Host exiting");
    Ok(())
}
