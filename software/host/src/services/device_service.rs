use std::sync::Arc;
use tokio::sync::Mutex as TokioMutex;

use crate::domain::DeviceInfo;
use crate::domain::error::HeteroLinkError;
use crate::core::DeviceManager;
use crate::events::DomainEvent;
use crate::infra::event_bus::EventBus;

/// 设备管理服务
#[derive(Clone)]
pub struct DeviceService {
    device_manager: Arc<TokioMutex<DeviceManager>>,
    event_bus: EventBus,
}

impl DeviceService {
    pub fn new(device_manager: Arc<TokioMutex<DeviceManager>>, event_bus: EventBus) -> Self {
        Self {
            device_manager,
            event_bus,
        }
    }

    pub async fn list_devices(&self) -> Vec<DeviceInfo> {
        let dm = self.device_manager.lock().await;
        dm.get_devices().values().cloned().collect()
    }

    pub async fn add_device(&self, name: &str) -> Result<DeviceInfo, HeteroLinkError> {
        let device_id = format!("device_{}", chrono::Local::now().timestamp());
        let device = DeviceInfo::new(device_id.clone(), name.to_string());

        self.device_manager.lock().await.add_device(device.clone())?;
        self.event_bus
            .emit_log(format!("已添加设备: {} ({})", name, device_id));
        self.broadcast_device_list().await;
        Ok(device)
    }

    pub async fn remove_device(&self, device_id: &str) -> Result<(), HeteroLinkError> {
        self.device_manager.lock().await.remove_device(device_id)?;
        self.event_bus.emit_log(format!("已移除设备: {}", device_id));
        self.broadcast_device_list().await;
        Ok(())
    }

    pub async fn disconnect_device(&self, device_id: &str) {
        self.device_manager
            .lock()
            .await
            .disconnect_device(device_id);
        self.event_bus.emit_log(format!("已断开设备: {}", device_id));
        self.broadcast_device_list().await;
    }

    /// 确保设备存在并更新在线状态（单次锁获取、原子操作）
    pub async fn update_device_online(&self, device_id: &str, online: bool, conn_type: &str) {
        let mut dm = self.device_manager.lock().await;
        dm.ensure_device(device_id, &format!("MQTT-{}", device_id), conn_type);
        dm.set_online(device_id, online);

        let device_list: Vec<DeviceInfo> =
            dm.get_devices().values().cloned().collect();
        let connected = device_list.iter().filter(|d| d.connected).count();
        let online_count = device_list.iter().filter(|d| d.online).count();
        drop(dm);

        self.event_bus.emit(DomainEvent::DeviceListChanged {
            devices: device_list,
            connected_count: connected,
            online_count,
        });
    }

    /// 广播完整设备列表
    pub async fn broadcast_device_list(&self) {
        let dm = self.device_manager.lock().await;
        let device_list: Vec<DeviceInfo> =
            dm.get_devices().values().cloned().collect();
        let connected = device_list.iter().filter(|d| d.connected).count();
        let online = device_list.iter().filter(|d| d.online).count();
        drop(dm);

        self.event_bus.emit(DomainEvent::DeviceListChanged {
            devices: device_list,
            connected_count: connected,
            online_count: online,
        });
    }
}
