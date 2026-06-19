use anyhow::Result;
use std::sync::Arc;
use tokio::sync::{mpsc, Mutex as TokioMutex};
use tracing::{info, warn, error};

mod protocol;
mod core;
mod storage;
mod api;

use crate::core::{DeviceManager, DataProcessor, AlarmSystem, DeviceInfo, DeviceEvent};
use crate::protocol::{MqttChannel, MqttConfig, MqttEvent};
use crate::storage::{ConfigManager, DataLogger};
use crate::api::{AppState, routes, broadcast::WsMessage};
use crate::api::dto::{DeviceInfoDto, ChannelStatsDto};

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_max_level(tracing::Level::INFO)
        .init();

    let simulation_mode = std::env::args().any(|a| a == "--simulate" || a == "-s");
    let bind_addr = std::env::args()
        .find(|a| a.starts_with("--bind="))
        .map(|a| a.trim_start_matches("--bind=").to_string())
        .unwrap_or_else(|| "0.0.0.0:3000".to_string());

    info!(
        "Starting HeteroLink Host Web Server...{}",
        if simulation_mode { " [SIMULATION MODE]" } else { "" }
    );

    // 创建核心模块实例
    let device_manager = Arc::new(TokioMutex::new(DeviceManager::new()));
    let data_processor = Arc::new(DataProcessor::new());
    let alarm_system = Arc::new(AlarmSystem::new());
    let config_manager = Arc::new(TokioMutex::new(ConfigManager::new()));
    let data_logger = Arc::new(TokioMutex::new(DataLogger::new()));

    // 加载配置
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
            username: if cfg.mqtt.username.is_empty() {
                None
            } else {
                Some(cfg.mqtt.username.clone())
            },
            password: if cfg.mqtt.password.is_empty() {
                None
            } else {
                Some(cfg.mqtt.password.clone())
            },
            client_id: cfg.mqtt.client_id.clone(),
            use_tls: cfg.mqtt.use_tls,
        }
    };
    let mqtt_channel = Arc::new(TokioMutex::new(MqttChannel::new(mqtt_config)));

    // 启用模拟模式
    if simulation_mode {
        mqtt_channel.lock().await.set_simulation_mode(true);
    }

    // Simulation event channel
    let (sim_tx, mut sim_rx) = mpsc::channel::<MqttEvent>(50);
    let simulation_tx: Option<mpsc::Sender<MqttEvent>> =
        if simulation_mode { Some(sim_tx) } else { None };

    // 创建事件通道
    let (device_event_tx, device_event_rx) = mpsc::channel::<DeviceEvent>(200);
    let (mqtt_event_tx, mqtt_event_rx) = mpsc::channel::<MqttEvent>(200);
    let (ws_broadcast_tx, _) = tokio::sync::broadcast::channel::<WsMessage>(1024);

    // 构建 AppState
    let state = AppState {
        device_manager: device_manager.clone(),
        data_processor: data_processor.clone(),
        alarm_system: alarm_system.clone(),
        config_manager: config_manager.clone(),
        data_logger: data_logger.clone(),
        mqtt_channel: mqtt_channel.clone(),
        device_event_tx: device_event_tx.clone(),
        mqtt_event_tx: mqtt_event_tx.clone(),
        ws_broadcast: ws_broadcast_tx.clone(),
        simulation_tx: simulation_tx.clone(),
        simulation_mode,
    };

    // 转发模拟事件
    if simulation_mode {
        let mqtt_tx_sim = mqtt_event_tx.clone();
        tokio::spawn(async move {
            while let Some(evt) = sim_rx.recv().await {
                let _ = mqtt_tx_sim.send(evt).await;
            }
        });
    }

    // 启动设备事件处理任务
    spawn_device_event_handler(state.clone(), device_event_rx);

    // 启动 MQTT 事件处理任务
    spawn_mqtt_event_handler(state.clone(), mqtt_event_rx);

    // 心跳任务
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

    // 自动连接 MQTT + 模拟设备
    {
        let state = state.clone();
        tokio::spawn(async move {
            tokio::time::sleep(tokio::time::Duration::from_millis(500)).await;

            if state.simulation_mode {
                let sim_id = "sim_device_001".to_string();
                let mut device = DeviceInfo::new(sim_id.clone(), "Simulator".to_string());
                device.connection_type = "Simulation".to_string();
                device.online = true;
                device.connected = true;
                let _ = state.device_manager.lock().await.add_device(device).await;

                let _ = state.mqtt_event_tx.send(MqttEvent::Connected).await;
                let _ = state
                    .mqtt_event_tx
                    .send(MqttEvent::DeviceStatusReceived {
                        device_id: sim_id.clone(),
                        online: true,
                    })
                    .await;

                info!("Simulation device '{}' created", sim_id);
                let _ = state.ws_broadcast.send(WsMessage::Log {
                    message: "[模拟] 模拟设备已就绪".to_string(),
                    timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                });
            }

            info!("Auto-connecting to MQTT broker...");
            let mut mqtt = state.mqtt_channel.lock().await;
            match mqtt.connect(state.mqtt_event_tx.clone()).await {
                Ok(_) => info!("MQTT connection initiated"),
                Err(e) => warn!("MQTT auto-connect failed: {}", e),
            }
        });
    }

    // 构建并启动 axum 服务器
    let router = routes::create_router(state);
    let listener = tokio::net::TcpListener::bind(&bind_addr).await?;
    info!("Server listening on http://{}", bind_addr);
    axum::serve(listener, router).await?;

    info!("HeteroLink Host exiting");
    Ok(())
}

/// 设备事件处理任务
fn spawn_device_event_handler(state: AppState, mut rx: mpsc::Receiver<DeviceEvent>) {
    tokio::spawn(async move {
        while let Some(event) = rx.recv().await {
            match event {
                DeviceEvent::DevicesChanged(devices) => {
                    let device_list: Vec<DeviceInfoDto> =
                        devices.values().map(DeviceInfoDto::from).collect();
                    let connected = device_list.iter().filter(|d| d.connected).count();
                    let online = device_list.iter().filter(|d| d.online).count();
                    let _ = state.ws_broadcast.send(WsMessage::DeviceListChanged {
                        devices: device_list,
                        connected_count: connected,
                        online_count: online,
                    });
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: format!("{} 已连接, {} 在线", connected, online),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                }
                DeviceEvent::TelemetryReceived { device_id, data } => {
                    // 处理数据
                    state.data_processor.add_data(&device_id, data.clone()).await;
                    state.alarm_system.check_data(&device_id, &data).await;
                    state
                        .data_logger
                        .lock()
                        .await
                        .write_data(&device_id, &data);

                    // 广播遥测数据
                    let _ = state.ws_broadcast.send(WsMessage::Telemetry {
                        device_id: device_id.clone(),
                        timestamp: data.timestamp,
                        channels: data.channels.clone(),
                    });

                    // 广播更新的统计数据
                    let stats = state.data_processor.get_stats(&device_id).await;
                    let stats_dto: Vec<ChannelStatsDto> =
                        stats.into_iter().map(ChannelStatsDto::from).collect();
                    let _ = state.ws_broadcast.send(WsMessage::StatsUpdated {
                        device_id,
                        stats: stats_dto,
                    });
                }
                DeviceEvent::DeviceStatusChanged {
                    device_id,
                    connected,
                    online,
                } => {
                    let status = if online {
                        "在线"
                    } else if connected {
                        "已连接"
                    } else {
                        "离线"
                    };
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: format!("设备 {} {}", device_id, status),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                    let _ = state.ws_broadcast.send(WsMessage::DeviceStatusChanged {
                        device_id,
                        connected,
                        online,
                    });
                    // Also broadcast full device list
                    api::handlers::device::broadcast_device_list(&state).await;
                }
                DeviceEvent::DeviceError {
                    device_id,
                    error: err_msg,
                } => {
                    error!("Device {} error: {}", device_id, err_msg);
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: format!("设备 {} 错误: {}", device_id, err_msg),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                    let _ = state.ws_broadcast.send(WsMessage::Error {
                        message: format!("设备 {} 错误: {}", device_id, err_msg),
                    });
                }
            }
        }
    });
}

/// MQTT 事件处理任务
fn spawn_mqtt_event_handler(state: AppState, mut rx: mpsc::Receiver<MqttEvent>) {
    tokio::spawn(async move {
        while let Some(event) = rx.recv().await {
            match event {
                MqttEvent::Connected => {
                    let _ = state
                        .ws_broadcast
                        .send(WsMessage::MqttStatusChanged { connected: true });
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: "MQTT 已连接".to_string(),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                }
                MqttEvent::Disconnected => {
                    let _ = state
                        .ws_broadcast
                        .send(WsMessage::MqttStatusChanged { connected: false });
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: "MQTT 已断开".to_string(),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                }
                MqttEvent::DeviceStatusReceived { device_id, online } => {
                    // 确保设备存在
                    {
                        let dm = state.device_manager.lock().await;
                        let devices = dm.get_devices().await;
                        if !devices.contains_key(&device_id) {
                            let mut device =
                                DeviceInfo::new(device_id.clone(), format!("MQTT-{}", device_id));
                            device.connection_type = "MQTT".to_string();
                            let _ = dm.add_device(device).await;
                        }
                    }
                    // 更新设备在线状态
                    {
                        let dm = state.device_manager.lock().await;
                        let devices = dm.get_devices().await;
                        if let Some(device) = devices.get(&device_id).cloned() {
                            let mut updated = device;
                            updated.online = online;
                            updated.connected = online;
                            let _ = dm.remove_device(&device_id).await;
                            let _ = dm.add_device(updated).await;
                        }
                    }
                    let status = if online { "上线" } else { "下线" };
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: format!("设备 {} {}", device_id, status),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                    api::handlers::device::broadcast_device_list(&state).await;
                }
                MqttEvent::TelemetryReceived { device_id, data } => {
                    info!("Telemetry from {}: {} bytes", device_id, data.len());
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
                        if channels.is_empty() {
                            continue;
                        }

                        let ts = json
                            .get("ts")
                            .and_then(|v| v.as_u64())
                            .unwrap_or(0) as u32;
                        let telemetry = protocol::TelemetryData {
                            timestamp: ts,
                            channels,
                        };

                        state
                            .data_processor
                            .add_data(&device_id, telemetry.clone())
                            .await;
                        state
                            .alarm_system
                            .check_data(&device_id, &telemetry)
                            .await;
                        state
                            .data_logger
                            .lock()
                            .await
                            .write_data(&device_id, &telemetry);

                        let _ = state.ws_broadcast.send(WsMessage::Telemetry {
                            device_id: device_id.clone(),
                            timestamp: telemetry.timestamp,
                            channels: telemetry.channels.clone(),
                        });

                        let stats = state.data_processor.get_stats(&device_id).await;
                        let stats_dto: Vec<ChannelStatsDto> =
                            stats.into_iter().map(ChannelStatsDto::from).collect();
                        let _ = state.ws_broadcast.send(WsMessage::StatsUpdated {
                            device_id,
                            stats: stats_dto,
                        });
                    }
                }
                MqttEvent::CommandReceived { device_id, command } => {
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: format!("命令: {} -> {}", device_id, command),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                }
                MqttEvent::ResponseReceived {
                    device_id,
                    response,
                } => {
                    let display =
                        if let Ok(json) = serde_json::from_str::<serde_json::Value>(&response) {
                            let status =
                                json.get("status").and_then(|v| v.as_str()).unwrap_or("?");
                            let cmd = json.get("cmd").and_then(|v| v.as_str()).unwrap_or("?");
                            let msg = json
                                .get("message")
                                .and_then(|v| v.as_str())
                                .unwrap_or("");
                            format!("{} [{}] {}: {}", device_id, status, cmd, msg)
                        } else {
                            format!("{}: {}", device_id, response)
                        };
                    let _ = state.ws_broadcast.send(WsMessage::CommandResponse {
                        device_id,
                        response: display.clone(),
                    });
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: format!("<- {}", display),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                }
                MqttEvent::MessageReceived { .. } => {}
                MqttEvent::Error(msg) => {
                    let _ = state.ws_broadcast.send(WsMessage::Log {
                        message: format!("MQTT 错误: {}", msg),
                        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                    });
                    let _ = state.ws_broadcast.send(WsMessage::Error { message: msg });
                }
            }
        }
    });
}
