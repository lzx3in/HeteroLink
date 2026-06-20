use std::sync::Arc;
use tokio::sync::Mutex as TokioMutex;

use crate::domain::error::HeteroLinkError;
use crate::infra::event_bus::EventBus;
use crate::storage::config::{AppConfig, ConfigManager};

/// 配置管理服务
#[derive(Clone)]
pub struct ConfigService {
    config_manager: Arc<TokioMutex<ConfigManager>>,
    event_bus: EventBus,
}

impl ConfigService {
    pub fn new(config_manager: Arc<TokioMutex<ConfigManager>>, event_bus: EventBus) -> Self {
        Self {
            config_manager,
            event_bus,
        }
    }

    pub async fn get_config(&self) -> AppConfig {
        let config = self.config_manager.lock().await;
        config.get().clone()
    }

    pub async fn save_config(&self, new_config: AppConfig) -> Result<(), HeteroLinkError> {
        let mut config = self.config_manager.lock().await;
        *config.get_mut() = new_config;
        config.save(None)?;
        self.event_bus.emit_log("配置已保存");
        Ok(())
    }

    pub async fn load_config(&self) -> Result<AppConfig, HeteroLinkError> {
        let mut config = self.config_manager.lock().await;
        config.load(None)?;
        let app_config = config.get().clone();
        self.event_bus.emit_log("配置已加载");
        Ok(app_config)
    }

    pub async fn reset_config(&self) -> AppConfig {
        let mut config = self.config_manager.lock().await;
        config.reset();
        self.event_bus.emit_log("配置已重置");
        config.get().clone()
    }
}
