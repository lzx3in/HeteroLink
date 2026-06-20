/// 领域错误类型 — 统一全项目的错误枚举
#[derive(Debug, thiserror::Error)]
#[allow(dead_code)]
pub enum HeteroLinkError {
    #[error("Device not found: {0}")]
    DeviceNotFound(String),

    #[error("Device already exists: {0}")]
    DeviceAlreadyExists(String),

    #[error("MQTT not connected")]
    MqttNotConnected,

    #[error("MQTT connection failed: {0}")]
    MqttConnectionFailed(String),

    #[error("MQTT error: {0}")]
    MqttError(String),

    #[error("Command failed: {0}")]
    CommandFailed(String),

    #[error("Configuration error: {0}")]
    ConfigError(String),

    #[error("Recording error: {0}")]
    RecordingError(String),

    #[error("Export error: {0}")]
    ExportError(String),

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Serialization error: {0}")]
    Serialization(#[from] serde_json::Error),
}

impl From<rumqttc::ClientError> for HeteroLinkError {
    fn from(e: rumqttc::ClientError) -> Self {
        HeteroLinkError::MqttError(e.to_string())
    }
}

impl From<csv::Error> for HeteroLinkError {
    fn from(e: csv::Error) -> Self {
        HeteroLinkError::ExportError(e.to_string())
    }
}

impl From<toml::de::Error> for HeteroLinkError {
    fn from(e: toml::de::Error) -> Self {
        HeteroLinkError::ConfigError(e.to_string())
    }
}

impl From<toml::ser::Error> for HeteroLinkError {
    fn from(e: toml::ser::Error) -> Self {
        HeteroLinkError::ConfigError(e.to_string())
    }
}
