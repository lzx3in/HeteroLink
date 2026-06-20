use anyhow::Result;
use std::sync::Arc;
use tokio::sync::{mpsc, Mutex as TokioMutex};
use tracing::{info, warn};

use crate::core::{AlarmSystem, DataProcessor, DeviceManager};
use crate::protocol::MqttChannel;
use crate::storage::{ConfigManager, DataLogger};
use crate::infra::event_bus::EventBus;
use crate::services::{
    alarm_service::AlarmService,
    command_service::CommandService,
    config_service::ConfigService,
    device_service::DeviceService,
    export_service::ExportService,
    mqtt_service::MqttService,
    recording_service::RecordingService,
    telemetry_service::TelemetryService,
    updater_service::UpdaterService,
};
use crate::api::{AppState, routes};
use crate::workers::{device_events, mqtt_events};

/// Application builder — handles DI assembly and server startup.
pub struct AppBuilder {
    simulation_mode: bool,
    bind_addr: String,
}

impl AppBuilder {
    /// Parse command-line arguments to construct the builder.
    pub fn from_args() -> Self {
        #[cfg(feature = "simulation")]
        let simulation_mode = std::env::args().any(|a| a == "--simulate" || a == "-s");
        #[cfg(not(feature = "simulation"))]
        let simulation_mode = false;

        let bind_addr = std::env::args()
            .find(|a| a.starts_with("--bind="))
            .map(|a| a.trim_start_matches("--bind=").to_string())
            .unwrap_or_else(|| "0.0.0.0:3000".to_string());

        Self {
            simulation_mode,
            bind_addr,
        }
    }

    /// Assemble all services and run the HTTP server.
    pub async fn build_and_run(self) -> Result<()> {
        let simulation_mode = self.simulation_mode;
        let bind_addr = self.bind_addr;

        info!(
            "Starting HeteroLink Host Web Server...{}",
            if simulation_mode {
                " [SIMULATION MODE]"
            } else {
                ""
            }
        );

        // ── 1. Core engines ──────────────────────────────────────────
        let device_manager = Arc::new(TokioMutex::new(DeviceManager::new()));
        let data_processor = Arc::new(DataProcessor::new());
        let alarm_system = Arc::new(AlarmSystem::new());
        let config_manager = Arc::new(TokioMutex::new(ConfigManager::new()));
        let data_logger = Arc::new(TokioMutex::new(DataLogger::new()));

        // Load configuration
        {
            let mut config = config_manager.lock().await;
            if let Err(e) = config.load(None) {
                warn!("Failed to load config: {}", e);
            }
        }

        let mqtt_config = {
            let config = config_manager.lock().await;
            config.get().mqtt.clone()
        };
        let mqtt_channel = Arc::new(TokioMutex::new(MqttChannel::new(mqtt_config)));

        // ── 2. Event channels & EventBus ─────────────────────────────
        #[allow(unused_variables)]
        let (device_event_tx, device_event_rx) = mpsc::channel(200);
        let (mqtt_event_tx, mqtt_event_rx) = mpsc::channel(200);
        let event_bus = EventBus::new(1024);

        #[cfg(feature = "simulation")]
        let (sim_tx, mut sim_rx) = mpsc::channel::<crate::protocol::MqttEvent>(50);
        #[cfg(feature = "simulation")]
        let simulation_tx: Option<mpsc::Sender<_>> =
            if simulation_mode { Some(sim_tx) } else { None };
        #[cfg(not(feature = "simulation"))]
        let simulation_tx: Option<mpsc::Sender<crate::protocol::MqttEvent>> = None;

        // ── 3. Services ──────────────────────────────────────────────
        let device_service = DeviceService::new(device_manager.clone(), event_bus.clone());
        let telemetry_service = TelemetryService::new(
            data_processor.clone(),
            alarm_system.clone(),
            data_logger.clone(),
            event_bus.clone(),
        );
        let alarm_service = AlarmService::new(alarm_system.clone(), event_bus.clone());
        let command_service = CommandService::new(
            mqtt_channel.clone(),
            mqtt_event_tx.clone(),
            simulation_tx,
            event_bus.clone(),
        );
        let mqtt_service = MqttService::new(mqtt_channel.clone(), mqtt_event_tx.clone());
        let config_service = ConfigService::new(config_manager.clone(), event_bus.clone());
        let recording_service = RecordingService::new(data_logger.clone(), event_bus.clone());
        let export_service = ExportService::new(data_processor.clone());
        let updater_service = UpdaterService::new(event_bus.clone());

        // ── 4. AppState ──────────────────────────────────────────────
        let state = AppState {
            device_service: device_service.clone(),
            telemetry_service: telemetry_service.clone(),
            alarm_service,
            command_service,
            mqtt_service,
            config_service,
            recording_service,
            export_service,
            updater_service: updater_service.clone(),
            event_bus: event_bus.clone(),
        };

        // ── 5. Background workers ────────────────────────────────────
        device_events::spawn_device_event_handler(
            device_service.clone(),
            telemetry_service.clone(),
            event_bus.clone(),
            device_event_rx,
        );
        mqtt_events::spawn_mqtt_event_handler(
            device_service.clone(),
            telemetry_service.clone(),
            event_bus.clone(),
            mqtt_event_rx,
        );

        // Simulation event forwarding
        #[cfg(feature = "simulation")]
        if simulation_mode {
            let mqtt_tx = mqtt_event_tx.clone();
            tokio::spawn(async move {
                while let Some(evt) = sim_rx.recv().await {
                    let _ = mqtt_tx.send(evt).await;
                }
            });
        }

        // ── 6. Auto-connect MQTT + simulation devices ────────────────
        {
            let mqtt_channel = mqtt_channel.clone();
            let mqtt_event_tx = mqtt_event_tx.clone();
            #[cfg(feature = "simulation")]
            let device_manager = device_manager.clone();
            #[cfg(feature = "simulation")]
            let event_bus = event_bus.clone();

            tokio::spawn(async move {
                tokio::time::sleep(tokio::time::Duration::from_millis(500)).await;

                #[cfg(feature = "simulation")]
                if simulation_mode {
                    crate::sim::simulation::setup_simulation(
                        &device_manager,
                        &mqtt_channel,
                        &mqtt_event_tx,
                        &event_bus,
                        "sim_device_001",
                    )
                    .await;
                    crate::sim::simulation::spawn_simulation_generator(
                        device_event_tx,
                        "sim_device_001".to_string(),
                    );
                }

                info!("Auto-connecting to MQTT broker...");
                let mut mqtt = mqtt_channel.lock().await;
                mqtt.connect(mqtt_event_tx).await;
                info!("MQTT connection initiated");

                // 启动后延迟检查更新（静默）
                tokio::time::sleep(tokio::time::Duration::from_secs(5)).await;
                let updater = updater_service.clone();
                tokio::spawn(async move {
                    let status = updater.check_for_update().await;
                    if let crate::services::updater_service::UpdateStatus::Available { ref version, ref body, .. } = status {
                        tracing::info!("Update available: v{}", version);
                        updater.event_bus().emit_update_available(version, body);
                    }
                });
            });
        }

        // ── 7. Start axum server ─────────────────────────────────────
        let router = routes::create_router(state);
        let listener = tokio::net::TcpListener::bind(&bind_addr).await?;
        info!("Server listening on http://{}", bind_addr);
        axum::serve(listener, router).await?;

        info!("HeteroLink Host exiting");
        Ok(())
    }
}
