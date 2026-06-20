use std::sync::Arc;
use tokio::sync::Mutex as TokioMutex;

use crate::domain::error::HeteroLinkError;
use crate::events::DomainEvent;
use crate::infra::event_bus::EventBus;
use crate::storage::DataLogger;

/// 录制控制服务
#[derive(Clone)]
pub struct RecordingService {
    data_logger: Arc<TokioMutex<DataLogger>>,
    event_bus: EventBus,
}

impl RecordingService {
    pub fn new(data_logger: Arc<TokioMutex<DataLogger>>, event_bus: EventBus) -> Self {
        Self {
            data_logger,
            event_bus,
        }
    }

    pub async fn start(&self, device_id: &str, path: Option<&str>) -> Result<(), HeteroLinkError> {
        let base_path = path.unwrap_or("./logs");
        self.data_logger
            .lock()
            .await
            .start_recording(base_path, device_id)?;
        self.event_bus
            .emit_log(format!("开始记录: {}", device_id));
        self.event_bus.emit(DomainEvent::RecordingChanged { recording: true });
        Ok(())
    }

    pub async fn stop(&self) {
        self.data_logger.lock().await.stop_recording();
        self.event_bus.emit_log("停止记录");
        self.event_bus.emit(DomainEvent::RecordingChanged { recording: false });
    }

    pub async fn is_recording(&self) -> bool {
        self.data_logger.lock().await.is_recording()
    }
}
