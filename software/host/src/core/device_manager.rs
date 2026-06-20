use crate::domain::DeviceInfo;
use std::collections::HashMap;
use tracing::info;

/// 设备管理器
///
/// 注意：本结构体本身由外层 `TokioMutex<DeviceManager>` 保护，
/// 内部字段不再使用额外的 Arc/Mutex 包装。
pub struct DeviceManager {
    devices: HashMap<String, DeviceInfo>,
}

impl DeviceManager {
    pub fn new() -> Self {
        Self {
            devices: HashMap::new(),
        }
    }

    pub fn get_devices(&self) -> &HashMap<String, DeviceInfo> {
        &self.devices
    }

    pub fn get_device(&self, device_id: &str) -> Option<&DeviceInfo> {
        self.devices.get(device_id)
    }

    pub fn device_exists(&self, device_id: &str) -> bool {
        self.devices.contains_key(device_id)
    }

    pub fn add_device(&mut self, device: DeviceInfo) -> Result<(), crate::domain::error::HeteroLinkError> {
        if self.devices.contains_key(&device.id) {
            return Err(crate::domain::error::HeteroLinkError::DeviceAlreadyExists(device.id.clone()));
        }
        info!("Device added: {}", device.id);
        self.devices.insert(device.id.clone(), device);
        Ok(())
    }

    pub fn remove_device(&mut self, device_id: &str) -> Result<(), crate::domain::error::HeteroLinkError> {
        if self.devices.remove(device_id).is_none() {
            return Err(crate::domain::error::HeteroLinkError::DeviceNotFound(device_id.to_string()));
        }
        info!("Device removed: {}", device_id);
        Ok(())
    }

    pub fn disconnect_device(&mut self, device_id: &str) {
        if let Some(device) = self.devices.get_mut(device_id) {
            device.connected = false;
        }
        info!("Device disconnected: {}", device_id);
    }

    /// 确保设备存在，不存在则自动创建（原子操作）
    pub fn ensure_device(&mut self, device_id: &str, name: &str, conn_type: &str) {
        if !self.devices.contains_key(device_id) {
            let mut device = DeviceInfo::new(device_id.to_string(), name.to_string());
            device.connection_type = conn_type.to_string();
            self.devices.insert(device_id.to_string(), device);
            info!("Device auto-created: {} ({})", device_id, conn_type);
        }
    }

    /// 更新设备在线状态（需在已持有外层锁时调用）
    pub fn set_online(&mut self, device_id: &str, online: bool) {
        if let Some(device) = self.devices.get_mut(device_id) {
            device.online = online;
            device.connected = online;
        }
    }
}
