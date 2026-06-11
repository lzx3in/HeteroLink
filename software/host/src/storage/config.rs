use anyhow::Result;
use serde::{Deserialize, Serialize};
use std::path::PathBuf;
use tracing::info;

/// UART 配置
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UartConfigData {
    pub port_name: String,
    pub baud_rate: u32,
}

impl Default for UartConfigData {
    fn default() -> Self {
        Self { port_name: String::new(), baud_rate: 921600 }
    }
}

/// MQTT 配置
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MqttConfigData {
    pub broker_host: String,
    pub broker_port: u16,
    pub username: String,
    pub password: String,
    pub client_id: String,
    pub use_tls: bool,
}

impl Default for MqttConfigData {
    fn default() -> Self {
        Self {
            broker_host: "broker.emqx.io".to_string(),
            broker_port: 1883,
            username: String::new(),
            password: String::new(),
            client_id: "heterolink-host".to_string(),
            use_tls: false,
        }
    }
}

/// 数据配置
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DataConfig {
    pub buffer_size: usize,
    pub auto_export: bool,
    pub export_path: String,
}

impl Default for DataConfig {
    fn default() -> Self {
        Self { buffer_size: 10000, auto_export: false, export_path: String::new() }
    }
}

/// 告警配置
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AlarmConfigData {
    pub enabled: bool,
    pub lower_limit: f32,
    pub upper_limit: f32,
    pub level: String,
}

impl Default for AlarmConfigData {
    fn default() -> Self {
        Self {
            enabled: true,
            lower_limit: -1000.0,
            upper_limit: 1000.0,
            level: "warning".to_string(),
        }
    }
}

/// UI 配置
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UiConfig {
    pub theme: String,
    pub language: String,
}

impl Default for UiConfig {
    fn default() -> Self {
        Self { theme: "dark".to_string(), language: "zh-CN".to_string() }
    }
}

/// 应用配置
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct AppConfig {
    pub uart: UartConfigData,
    pub mqtt: MqttConfigData,
    pub data: DataConfig,
    pub alarm: AlarmConfigData,
    pub ui: UiConfig,
}

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

    pub fn load(&mut self, path: Option<&str>) -> Result<()> {
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

    pub fn save(&self, path: Option<&str>) -> Result<()> {
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
