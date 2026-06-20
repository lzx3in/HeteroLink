use std::sync::Arc;
use tokio::sync::Mutex as TokioMutex;

use crate::core::{AlarmSystem, DataProcessor};
use crate::domain::{ChannelStats, TelemetryData};
use crate::events::DomainEvent;
use crate::infra::event_bus::EventBus;
use crate::storage::DataLogger;

/// 遥测数据处理服务
///
/// 提供统一的遥测处理管道 `process_telemetry`，
/// 消除原先设备事件和 MQTT 事件中重复的 5 步处理逻辑。
#[derive(Clone)]
pub struct TelemetryService {
    data_processor: Arc<DataProcessor>,
    alarm_system: Arc<AlarmSystem>,
    data_logger: Arc<TokioMutex<DataLogger>>,
    event_bus: EventBus,
}

impl TelemetryService {
    pub fn new(
        data_processor: Arc<DataProcessor>,
        alarm_system: Arc<AlarmSystem>,
        data_logger: Arc<TokioMutex<DataLogger>>,
        event_bus: EventBus,
    ) -> Self {
        Self {
            data_processor,
            alarm_system,
            data_logger,
            event_bus,
        }
    }

    /// 统一遥测处理管道（核心方法）
    ///
    /// 1. 写入 DataProcessor 缓冲区
    /// 2. 检查告警
    /// 3. 写入 DataLogger（如正在录制）
    /// 4. 广播遥测数据
    /// 5. 广播触发的告警
    /// 6. 广播更新的通道统计
    pub async fn process_telemetry(&self, device_id: &str, data: TelemetryData) {
        // 1. 写入数据处理器
        self.data_processor.add_data(device_id, data.clone()).await;

        // 2. 告警检查
        let triggered = self.alarm_system.check_data(device_id, &data).await;

        // 3. 写入数据记录器
        self.data_logger
            .lock()
            .await
            .write_data(device_id, &data);

        // 4. 广播遥测数据
        self.event_bus.emit(DomainEvent::Telemetry {
            device_id: device_id.to_string(),
            data: data.clone(),
        });

        // 5. 广播触发的告警
        for alarm in &triggered {
            self.event_bus.emit(DomainEvent::AlarmTriggered {
                alarm: alarm.clone(),
            });
            self.event_bus.emit_log(format!(
                "[告警] {} CH{}: {}",
                alarm.device_id, alarm.channel_id, alarm.message
            ));
        }

        // 6. 广播更新的统计数据
        let stats = self.data_processor.get_stats(device_id).await;
        let stats_vec: Vec<(i32, ChannelStats)> = stats.into_iter().collect();
        self.event_bus.emit(DomainEvent::StatsUpdated {
            device_id: device_id.to_string(),
            stats: stats_vec,
        });
    }

    pub async fn get_latest(&self, device_id: &str, count: usize) -> Vec<TelemetryData> {
        self.data_processor.get_latest_data(device_id, count).await
    }

    pub async fn get_stats(&self, device_id: &str) -> Vec<(i32, ChannelStats)> {
        let stats = self.data_processor.get_stats(device_id).await;
        stats.into_iter().collect()
    }
}
