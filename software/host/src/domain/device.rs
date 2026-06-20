use serde::Serialize;
use std::collections::HashMap;

use crate::domain::TelemetryData;

/// 设备信息
#[derive(Debug, Clone, Serialize)]
pub struct DeviceInfo {
    pub id: String,
    pub name: String,
    pub connected: bool,
    pub online: bool,
    pub connection_type: String,
    pub port: String,
    pub last_seen: u64,
    pub metadata: HashMap<String, String>,
}

impl DeviceInfo {
    pub fn new(id: String, name: String) -> Self {
        Self {
            id,
            name,
            connected: false,
            online: false,
            connection_type: String::new(),
            port: String::new(),
            last_seen: 0,
            metadata: HashMap::new(),
        }
    }
}

/// 设备管理器事件
#[derive(Debug)]
#[allow(dead_code)]
pub enum DeviceEvent {
    DevicesChanged(HashMap<String, DeviceInfo>),
    DeviceStatusChanged {
        device_id: String,
        connected: bool,
        online: bool,
    },
    TelemetryReceived {
        device_id: String,
        data: TelemetryData,
    },
    DeviceError {
        device_id: String,
        error: String,
    },
}
