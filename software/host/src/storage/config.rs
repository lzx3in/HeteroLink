use std::path::PathBuf;
use tracing::info;

use crate::domain::error::HeteroLinkError;

pub use crate::domain::config::AppConfig;

/// 配置管理器
pub struct ConfigManager {
    config: AppConfig,
    config_path: PathBuf,
}

impl ConfigManager {
    pub fn new() -> Self {
        Self {
            config: AppConfig::default(),
            config_path: Self::default_config_path(),
        }
    }

    fn default_config_path() -> PathBuf {
        dirs::config_dir()
            .unwrap_or_else(|| PathBuf::from("."))
            .join("heterolink")
            .join("config.toml")
    }

    pub fn load(&mut self, path: Option<&str>) -> Result<(), HeteroLinkError> {
        let config_path = path.map(PathBuf::from).unwrap_or_else(|| self.config_path.clone());

        if !config_path.exists() {
            info!("Config file not found, using defaults: {:?}", config_path);
            self.config = AppConfig::default();
            return Ok(());
        }

        let content = std::fs::read_to_string(&config_path)?;
        self.config = toml::from_str(&content)?;
        self.config_path = config_path.clone();
        info!("Config loaded: {:?}", config_path);
        Ok(())
    }

    pub fn save(&self, path: Option<&str>) -> Result<(), HeteroLinkError> {
        let config_path = path.map(PathBuf::from).unwrap_or_else(|| self.config_path.clone());

        if let Some(parent) = config_path.parent() {
            std::fs::create_dir_all(parent)?;
        }

        let content = toml::to_string_pretty(&self.config)?;
        std::fs::write(&config_path, content)?;
        info!("Config saved: {:?}", config_path);
        Ok(())
    }

    pub fn get(&self) -> &AppConfig {
        &self.config
    }

    pub fn get_mut(&mut self) -> &mut AppConfig {
        &mut self.config
    }

    pub fn reset(&mut self) {
        self.config = AppConfig::default();
        info!("Config reset to defaults");
    }
}
