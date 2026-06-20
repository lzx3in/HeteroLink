use tokio::sync::mpsc;
use tracing::info;

use crate::core::DeviceManager;
use crate::domain::{DeviceEvent, DeviceInfo, TelemetryData};
use crate::infra::event_bus::EventBus;
use crate::protocol::{MqttChannel, MqttEvent};

use std::sync::Arc;
use tokio::sync::Mutex as TokioMutex;

/// 启动模拟遥测数据生成后台任务
pub fn spawn_simulation_generator(
    device_event_tx: mpsc::Sender<DeviceEvent>,
    sim_device_id: String,
) {
    tokio::spawn(async move {
        // 等待设备初始化完成
        tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
        info!("Simulation telemetry generator started");

        let mut interval = tokio::time::interval(tokio::time::Duration::from_millis(100));
        let mut t: f64 = 0.0;
        let dt: f64 = 0.1; // 100ms step

        loop {
            interval.tick().await;
            t += dt;

            let channels = vec![
                // CH1: 1 Hz sine wave, amplitude 1000, centered at 2000
                (2000.0 + 1000.0 * (2.0 * std::f64::consts::PI * 1.0 * t).sin()) as f32,
                // CH2: 2.5 Hz sine wave with phase offset, amplitude 500, centered at 1500
                (1500.0 + 500.0 * (2.0 * std::f64::consts::PI * 2.5 * t + 0.785).sin()) as f32,
                // CH3: Triangle wave, period 4s, amplitude 800, centered at 1800
                {
                    let phase = (t % 4.0) / 4.0;
                    let tri = if phase < 0.5 {
                        4.0 * phase - 1.0
                    } else {
                        3.0 - 4.0 * phase
                    };
                    (1800.0 + 800.0 * tri) as f32
                },
                // CH4: Sawtooth with noise, amplitude 600, centered at 2200
                {
                    let phase = (t % 3.0) / 3.0;
                    let noise = (fastrand::f64() - 0.5) * 100.0;
                    (2200.0 + 600.0 * (2.0 * phase - 1.0) + noise) as f32
                },
            ];

            let ts = (t * 1000.0) as u32;
            let telemetry = TelemetryData {
                timestamp: ts,
                channels,
            };

            let _ = device_event_tx
                .send(DeviceEvent::TelemetryReceived {
                    device_id: sim_device_id.clone(),
                    data: telemetry,
                })
                .await;
        }
    });
}

/// 模拟模式启动：注册模拟设备 + 触发 MQTT Connected 事件
pub async fn setup_simulation(
    device_manager: &Arc<TokioMutex<DeviceManager>>,
    mqtt_channel: &Arc<TokioMutex<MqttChannel>>,
    mqtt_event_tx: &mpsc::Sender<MqttEvent>,
    event_bus: &EventBus,
    sim_device_id: &str,
) {
    // 启用 MQTT 模拟模式
    mqtt_channel.lock().await.set_simulation_mode(true);

    // 注册模拟设备
    let mut device = DeviceInfo::new(sim_device_id.to_string(), "Simulator".to_string());
    device.connection_type = "Simulation".to_string();
    device.online = true;
    device.connected = true;
    let _ = device_manager.lock().await.add_device(device);

    // 触发 MQTT 连接和设备上线事件
    let _ = mqtt_event_tx.send(MqttEvent::Connected).await;
    let _ = mqtt_event_tx
        .send(MqttEvent::DeviceStatusReceived {
            device_id: sim_device_id.to_string(),
            online: true,
        })
        .await;

    info!("Simulation device '{}' created", sim_device_id);
    event_bus.emit_log("[模拟] 模拟设备已就绪");
}
