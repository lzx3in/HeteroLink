use std::sync::Arc;

use crate::domain::{AlarmConfig, AlarmLevel, AlarmRecord};
use crate::core::AlarmSystem;
use crate::events::DomainEvent;
use crate::infra::event_bus::EventBus;

/// 告警管理服务
#[derive(Clone)]
pub struct AlarmService {
    alarm_system: Arc<AlarmSystem>,
    event_bus: EventBus,
}

/// 告警配置输入（从 API 层传入，等价于原 AlarmConfigRequest 的领域表示）
pub struct AlarmConfigInput {
    pub channel_id: i32,
    pub lower_limit: f32,
    pub upper_limit: f32,
    pub lower_enabled: bool,
    pub upper_enabled: bool,
    pub enabled: bool,
}

impl AlarmService {
    pub fn new(alarm_system: Arc<AlarmSystem>, event_bus: EventBus) -> Self {
        Self {
            alarm_system,
            event_bus,
        }
    }

    pub async fn list_alarms(&self, device_id: &str) -> Vec<AlarmRecord> {
        self.alarm_system.get_alarm_records(device_id).await
    }

    pub async fn configure_alarm(&self, device_id: &str, input: &AlarmConfigInput) {
        let config = AlarmConfig {
            channel_id: input.channel_id,
            lower_limit: input.lower_limit,
            upper_limit: input.upper_limit,
            lower_enabled: input.lower_enabled,
            upper_enabled: input.upper_enabled,
            level: AlarmLevel::Warning,
            enabled: input.enabled,
        };
        self.alarm_system.configure_alarm(device_id, config).await;
        self.event_bus.emit_log(format!(
            "告警配置已应用 -> {} CH{} (下限: {}, 上限: {})",
            device_id, input.channel_id, input.lower_limit, input.upper_limit
        ));
    }

    pub async fn acknowledge(&self, device_id: &str, channel_id: i32) {
        self.alarm_system
            .acknowledge_alarm(device_id, channel_id)
            .await;
        self.event_bus
            .emit_log(format!("已确认告警: {} 通道 {}", device_id, channel_id));

        let alarms = self.alarm_system.get_alarm_records(device_id).await;
        self.event_bus.emit(DomainEvent::AlarmsChanged {
            device_id: device_id.to_string(),
            alarms,
        });
    }

    pub async fn clear_alarms(&self, device_id: &str) {
        self.alarm_system.clear_records(device_id).await;
        self.event_bus
            .emit_log(format!("告警记录已清除: {}", device_id));
        self.event_bus.emit(DomainEvent::AlarmsChanged {
            device_id: device_id.to_string(),
            alarms: vec![],
        });
    }
}
