use serde::{Deserialize, Serialize};

/// MQTT 配置（合并后的唯一类型，替代原 MqttConfig / MqttConfigData）
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MqttConfig {
    pub broker_host: String,
    pub broker_port: u16,
    pub username: String,
    pub password: String,
    pub client_id: String,
    pub use_tls: bool,
}

impl Default for MqttConfig {
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

impl MqttConfig {
    /// 获取用户名（空字符串视为 None）
    pub fn auth_username(&self) -> Option<String> {
        if self.username.is_empty() { None } else { Some(self.username.clone()) }
    }

    /// 获取密码（空字符串视为 None）
    pub fn auth_password(&self) -> Option<String> {
        if self.password.is_empty() { None } else { Some(self.password.clone()) }
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
    pub mqtt: MqttConfig,
    pub data: DataConfig,
    pub alarm: AlarmConfigData,
    pub ui: UiConfig,
}
