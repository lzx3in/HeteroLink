use crate::protocol::TelemetryData;
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
    let dp_mqtt = data_processor.clone();
    let as_mqtt = alarm_system.clone();
    let dl_mqtt = data_logger.clone();
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
                    // Ensure device exists
                    let mut dm = dm_mqtt.lock().await;
                    let devices = dm.get_devices().await;
                    if !devices.contains_key(&device_id) {
                        let mut device = DeviceInfo::new(device_id.clone(), format!("MQTT-{}", device_id));
                        device.connection_type = "MQTT".to_string();
                        let _ = dm.add_device(device).await;
                    }
                    drop(dm);
                    // Update device online status
                    {
                        let mut dm = dm_mqtt.lock().await;
                        let devices = dm.get_devices().await;
                        if let Some(mut device) = devices.get(&device_id).cloned() {
                            device.online = online;
                            device.connected = online;
                            let _ = dm.remove_device(&device_id).await;
                            let _ = dm.add_device(device).await;
                        }
                    }
                    // Update UI device list
                    let devices = dm_mqtt.lock().await.get_devices().await;
                    let device_list: Vec<DeviceInfo> = devices.values().cloned().collect();
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::update_device_list(&ui, &device_list);
                            let status = if online { "上线" } else { "下线" };
                            ui::add_log_message(&ui, &format!("设备 {} {}", device_id, status));
                        }
                    }).ok();
                }
                MqttEvent::TelemetryReceived { device_id, data } => {
                    info!("Telemetry from {}: {} bytes", device_id, data.len());
                    // Parse telemetry JSON: {"ts":...,"channels":{"ch1":722.0,"ch2":463.0,...}}
                    if let Ok(json) = serde_json::from_str::<serde_json::Value>(&data) {
                        let mut channels = Vec::new();
                        if let Some(ch_map) = json.get("channels").and_then(|c| c.as_object()) {
                            for i in 1..=8 {
                                let key = format!("ch{}", i);
                                if let Some(val) = ch_map.get(&key).and_then(|v| v.as_f64()) {
                                    channels.push(val as f32);
                                }
                            }
                        }
                        if channels.is_empty() { continue; }

                        let ts = json.get("ts").and_then(|v| v.as_u64()).unwrap_or(0) as u32;
                        let telemetry = TelemetryData { timestamp: ts, channels };

                        // Add to data processor
                        dp_mqtt.add_data(&device_id, telemetry.clone()).await;

                        // Check alarms
                        as_mqtt.check_data(&device_id, &telemetry).await;

                        // Write to logger
                        dl_mqtt.lock().await.write_data(&device_id, &telemetry);

                        // Update UI with latest data
                        let latest = dp_mqtt.get_latest_data(&device_id, 50).await;
                        let stats = dp_mqtt.get_stats(&device_id).await;
                        let stats_vec: Vec<(i32, crate::core::ChannelStats)> = stats.into_iter().collect();
                        let alarms = as_mqtt.get_alarm_records(&device_id).await;
                        let uw = uw_mqtt.clone();
                        slint::invoke_from_event_loop(move || {
                            if let Some(ui) = uw.upgrade() {
                                ui::update_telemetry_list(&ui, &latest);
                                ui::update_stats_list(&ui, &stats_vec);
                                ui::update_alarm_list(&ui, &alarms);
                            }
                        }).ok();
                    }
                }
                MqttEvent::CommandReceived { device_id, command } => {
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::add_log_message(&ui, &format!("命令: {} -> {}", device_id, command));
                        }
                    }).ok();
                }
                MqttEvent::MessageReceived { .. } => {}
                MqttEvent::Error(msg) => {
                    let uw = uw_mqtt.clone();
                    slint::invoke_from_event_loop(move || {
                        if let Some(ui) = uw.upgrade() {
                            ui::add_log_message(&ui, &format!("MQTT 错误: {}", msg));
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
    
    // Auto-connect MQTT on startup
    {
        let mqtt_ch = mqtt_channel.clone();
        let tx = mqtt_event_tx.clone();
        tokio::spawn(async move {
            tokio::time::sleep(tokio::time::Duration::from_millis(500)).await;
            info!("Auto-connecting to MQTT broker...");
            let mut mqtt = mqtt_ch.lock().await;
            match mqtt.connect(tx).await {
                Ok(_) => {
                    info!("MQTT connection initiated");
                    let _ = slint::invoke_from_event_loop(move || {
                        // UI update will happen via MQTT events
                    });
                }
                Err(e) => {
                    warn!("MQTT auto-connect failed: {}", e);
                    let _ = slint::invoke_from_event_loop(move || {
                        // Error will be logged via MQTT events
                    });
                }
            }
        });
    }
    
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
