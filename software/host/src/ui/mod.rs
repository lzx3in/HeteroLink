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
    ui: &AppUI,
    device_manager: Arc<TokioMutex<DeviceManager>>,
    data_processor: Arc<DataProcessor>,
    alarm_system: Arc<AlarmSystem>,
    config_manager: Arc<TokioMutex<ConfigManager>>,
    data_logger: Arc<TokioMutex<DataLogger>>,
    mqtt_channel: Arc<TokioMutex<MqttChannel>>,
    device_event_tx: mpsc::Sender<DeviceEvent>,
    mqtt_event_tx: mpsc::Sender<MqttEvent>,
) {
    // Connect UART
    {
        let ui_weak = ui.as_weak();
        let dm = device_manager.clone();
        let tx = device_event_tx.clone();
        ui.on_connect_uart(move || {
            let ui_weak = ui_weak.clone();
            let dm = dm.clone();
            let tx = tx.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    let port_idx = ui.get_uart_port_index() as usize;
                    let baud_idx = ui.get_uart_baud_index() as usize;
                    let ports = ui.get_available_ports();
                    let port = ports.iter().nth(port_idx).map(|s| s.to_string()).unwrap_or_default();
                    let baud_rates = [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600];
                    let baud = baud_rates.get(baud_idx).copied().unwrap_or(9600);
                    add_log_message(&ui, &format!("连接 UART: {} @ {}", port, baud));
                    
                    let device_id = format!("uart_{}", port.replace("/dev/", ""));
                    let config = crate::protocol::UartConfig {
                        port_name: port.clone(),
                        baud_rate: baud,
                        data_bits: 8,
                        stop_bits: 1,
                        parity: 0,
                    };
                    
                    // 先添加设备（如果不存在）
                    let mut dm_lock = dm.lock().await;
                    if dm_lock.get_device(&device_id).await.is_none() {
                        let mut device = crate::core::DeviceInfo::new(device_id.clone(), format!("UART {}", port));
                        device.port = port.clone();
                        let _ = dm_lock.add_device(device).await;
                    }
                    
                    // 连接 UART
                    match dm_lock.connect_device_uart(&device_id, config, tx).await {
                        Ok(_) => add_log_message(&ui, &format!("已连接: {}", device_id)),
                        Err(e) => add_log_message(&ui, &format!("连接失败: {}", e)),
                    }
                }
            }).unwrap();
        });
    }

    // Disconnect device
    {
        let ui_weak = ui.as_weak();
        let dm = device_manager.clone();
        ui.on_disconnect_device(move |idx| {
            let ui_weak = ui_weak.clone();
            let dm = dm.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    let devices = dm.lock().await.get_devices().await;
                    if let Some(device) = devices.values().nth(idx as usize) {
                        let device_id = device.id.clone();
                        if let Err(e) = dm.lock().await.disconnect_device(&device_id).await {
                            add_log_message(&ui, &format!("断开失败: {}", e));
                        } else {
                            add_log_message(&ui, &format!("已断开: {}", device_id));
                        }
                    }
                }
            }).unwrap();
        });
    }

    // Connect MQTT
    {
        let ui_weak = ui.as_weak();
        let mqtt = mqtt_channel.clone();
        let tx = mqtt_event_tx.clone();
        ui.on_connect_mqtt(move || {
            let ui_weak = ui_weak.clone();
            let mqtt = mqtt.clone();
            let tx = tx.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    // Read config from UI
                    let host = ui.get_mqtt_host().to_string();
                    let port = ui.get_mqtt_port() as u16;
                    let username = ui.get_mqtt_username().to_string();
                    let password = ui.get_mqtt_password().to_string();
                    let client_id = ui.get_mqtt_client_id().to_string();
                    let use_tls = ui.get_mqtt_tls();

                    if host.is_empty() {
                        add_log_message(&ui, "错误: 请设置 Broker 地址");
                        return;
                    }

                    add_log_message(&ui, &format!("正在连接 MQTT: {}:{}...", host, port));

                    // Update MQTT config
                    let config = crate::protocol::MqttConfig {
                        broker_host: host,
                        broker_port: port,
                        username: if username.is_empty() { None } else { Some(username) },
                        password: if password.is_empty() { None } else { Some(password) },
                        client_id: if client_id.is_empty() {
                            format!("heterolink-host-{}", &uuid::Uuid::new_v4().to_string()[..8])
                        } else {
                            client_id
                        },
                        use_tls,
                    };

                    let mut mqtt_ch = mqtt.lock().await;
                    mqtt_ch.update_config(config);
                    match mqtt_ch.connect(tx).await {
                        Ok(_) => add_log_message(&ui, "MQTT 连接中..."),
                        Err(e) => add_log_message(&ui, &format!("MQTT 连接失败: {}", e)),
                    }
                }
            }).unwrap();
        });
    }

    // Disconnect MQTT
    {
        let ui_weak = ui.as_weak();
        let mqtt = mqtt_channel.clone();
        ui.on_disconnect_mqtt(move || {
            let ui_weak = ui_weak.clone();
            let mqtt = mqtt.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    if let Err(e) = mqtt.lock().await.disconnect().await {
                        add_log_message(&ui, &format!("MQTT 断开失败: {}", e));
                    } else {
                        add_log_message(&ui, "MQTT 已断开");
                    }
                }
            }).unwrap();
        });
    }

    // Start recording
    {
        let ui_weak = ui.as_weak();
        let logger = data_logger.clone();
        ui.on_start_recording(move || {
            let ui_weak = ui_weak.clone();
            let logger = logger.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    if let Err(e) = logger.lock().await.start_recording("./logs", "device_0") {
                        add_log_message(&ui, &format!("开始记录失败: {}", e));
                    } else {
                        ui.set_recording(true);
                        add_log_message(&ui, "开始记录数据");
                    }
                }
            }).unwrap();
        });
    }

    // Stop recording
    {
        let ui_weak = ui.as_weak();
        let logger = data_logger.clone();
        ui.on_stop_recording(move || {
            let ui_weak = ui_weak.clone();
            let logger = logger.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    logger.lock().await.stop_recording();
                    ui.set_recording(false);
                    add_log_message(&ui, "停止记录");
                }
            }).unwrap();
        });
    }

    // Export CSV
    {
        let ui_weak = ui.as_weak();
        let dp = data_processor.clone();
        ui.on_export_csv(move || {
            let ui_weak = ui_weak.clone();
            let dp = dp.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    let devices = ui.get_devices();
                    if let Some(device) = devices.iter().next() {
                        let device_id = device.id.to_string();
                        let filename = format!("export_{}_{}.csv", device_id, chrono::Local::now().format("%Y%m%d_%H%M%S"));
                        match dp.export_to_csv(&device_id, &filename).await {
                            Ok(_) => add_log_message(&ui, &format!("已导出: {}", filename)),
                            Err(e) => add_log_message(&ui, &format!("导出失败: {}", e)),
                        }
                    }
                }
            }).unwrap();
        });
    }

    // Export JSON
    {
        let ui_weak = ui.as_weak();
        let dp = data_processor.clone();
        ui.on_export_json(move || {
            let ui_weak = ui_weak.clone();
            let dp = dp.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    let devices = ui.get_devices();
                    if let Some(device) = devices.iter().next() {
                        let device_id = device.id.to_string();
                        let filename = format!("export_{}_{}.json", device_id, chrono::Local::now().format("%Y%m%d_%H%M%S"));
                        match dp.export_to_json(&device_id, &filename).await {
                            Ok(_) => add_log_message(&ui, &format!("已导出: {}", filename)),
                            Err(e) => add_log_message(&ui, &format!("导出失败: {}", e)),
                        }
                    }
                }
            }).unwrap();
        });
    }

    // Save config
    {
        let ui_weak = ui.as_weak();
        let cfg = config_manager.clone();
        ui.on_save_config(move || {
            let ui_weak = ui_weak.clone();
            let cfg = cfg.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    if let Err(e) = cfg.lock().await.save(None) {
                        add_log_message(&ui, &format!("保存配置失败: {}", e));
                    } else {
                        add_log_message(&ui, "配置已保存");
                    }
                }
            }).unwrap();
        });
    }

    // Load config
    {
        let ui_weak = ui.as_weak();
        let cfg = config_manager.clone();
        ui.on_load_config(move || {
            let ui_weak = ui_weak.clone();
            let cfg = cfg.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    if let Err(e) = cfg.lock().await.load(None) {
                        add_log_message(&ui, &format!("加载配置失败: {}", e));
                    } else {
                        add_log_message(&ui, "配置已加载");
                    }
                }
            }).unwrap();
        });
    }

    // Reset config
    {
        let ui_weak = ui.as_weak();
        let cfg = config_manager.clone();
        ui.on_reset_config(move || {
            let ui_weak = ui_weak.clone();
            let cfg = cfg.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    cfg.lock().await.reset();
                    add_log_message(&ui, "配置已重置");
                }
            }).unwrap();
        });
    }

    // Acknowledge alarm
    {
        let ui_weak = ui.as_weak();
        let alarms = alarm_system.clone();
        ui.on_acknowledge_alarm(move |idx| {
            let ui_weak = ui_weak.clone();
            let alarms = alarms.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    let records = alarms.get_all_alarm_records().await;
                    if let Some(alarm) = records.get(idx as usize) {
                        alarms.acknowledge_alarm(&alarm.device_id, alarm.channel_id).await;
                        add_log_message(&ui, &format!("已确认告警: {} 通道 {}", alarm.device_id, alarm.channel_id));
                    }
                }
            }).unwrap();
        });
    }

    // Clear alarms
    {
        let ui_weak = ui.as_weak();
        let alarms = alarm_system.clone();
        ui.on_clear_alarms(move || {
            let ui_weak = ui_weak.clone();
            let alarms = alarms.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    alarms.clear_records("device_0").await;
                    add_log_message(&ui, "告警记录已清除");
                }
            }).unwrap();
        });
    }

    // Add device
    {
        let ui_weak = ui.as_weak();
        let dm = device_manager.clone();
        ui.on_add_device(move |name| {
            let ui_weak = ui_weak.clone();
            let dm = dm.clone();
            let name = name.to_string();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    let device_id = format!("device_{}", chrono::Local::now().timestamp());
                    let device = DeviceInfo::new(device_id.clone(), name.clone());
                    if let Err(e) = dm.lock().await.add_device(device).await {
                        add_log_message(&ui, &format!("添加设备失败: {}", e));
                    } else {
                        add_log_message(&ui, &format!("已添加设备: {} ({})", name, device_id));
                    }
                }
            }).unwrap();
        });
    }

    // Remove device
    {
        let ui_weak = ui.as_weak();
        let dm = device_manager.clone();
        ui.on_remove_device(move |idx| {
            let ui_weak = ui_weak.clone();
            let dm = dm.clone();
            slint::spawn_local(async move {
                if let Some(ui) = ui_weak.upgrade() {
                    let devices = dm.lock().await.get_devices().await;
                    if let Some(device) = devices.values().nth(idx as usize) {
                        let device_id = device.id.clone();
                        if let Err(e) = dm.lock().await.remove_device(&device_id).await {
                            add_log_message(&ui, &format!("移除设备失败: {}", e));
                        } else {
                            add_log_message(&ui, &format!("已移除设备: {}", device_id));
                        }
                    }
                }
            }).unwrap();
        });
    }

    // Select device
    {
        let ui_weak = ui.as_weak();
        ui.on_select_device(move |idx| {
            if let Some(ui) = ui_weak.upgrade() {
                ui.set_selected_device_index(idx);
                let devices = ui.get_devices();
                if let Some(device) = devices.iter().nth(idx as usize) {
                    add_log_message(&ui, &format!("选中设备: {}", device.name));
                }
            }
        });
    }
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
