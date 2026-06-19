use axum::extract::{Path, State};
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::broadcast::WsMessage;
use crate::api::dto::{ApiResponse, AlarmConfigRequest, AlarmRecordDto};
use crate::core::{AlarmConfig, AlarmLevel};

/// GET /api/alarms/:device_id - 获取告警记录
pub async fn list_alarms(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<Vec<AlarmRecordDto>>> {
    let alarms = state.alarm_system.get_alarm_records(&device_id).await;
    let dtos: Vec<AlarmRecordDto> = alarms.iter().map(AlarmRecordDto::from).collect();
    Json(ApiResponse::ok(dtos))
}

/// PUT /api/alarms/:device_id/config - 配置告警
pub async fn configure_alarm(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<AlarmConfigRequest>,
) -> Json<ApiResponse<()>> {
    let config = AlarmConfig {
        channel_id: req.channel_id,
        lower_limit: req.lower_limit,
        upper_limit: req.upper_limit,
        lower_enabled: req.lower_enabled,
        upper_enabled: req.upper_enabled,
        level: AlarmLevel::Warning,
        enabled: req.enabled,
    };

    state.alarm_system.configure_alarm(&device_id, config).await;
    info!(
        "Alarm configured via API for device {} channel {}",
        device_id, req.channel_id
    );
    let _ = state.ws_broadcast.send(WsMessage::Log {
        message: format!(
            "告警配置已应用 -> {} CH{} (下限: {}, 上限: {})",
            device_id, req.channel_id, req.lower_limit, req.upper_limit
        ),
        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
    });
    Json(ApiResponse::ok_message("告警配置已保存"))
}

/// POST /api/alarms/:device_id/acknowledge/:channel_id - 确认告警
pub async fn acknowledge(
    State(state): State<AppState>,
    Path((device_id, channel_id)): Path<(String, i32)>,
) -> Json<ApiResponse<()>> {
    state
        .alarm_system
        .acknowledge_alarm(&device_id, channel_id)
        .await;
    info!(
        "Alarm acknowledged via API for device {} channel {}",
        device_id, channel_id
    );
    let _ = state.ws_broadcast.send(WsMessage::Log {
        message: format!("已确认告警: {} 通道 {}", device_id, channel_id),
        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
    });

    // Broadcast updated alarms
    let alarms = state.alarm_system.get_alarm_records(&device_id).await;
    let dtos: Vec<AlarmRecordDto> = alarms.iter().map(AlarmRecordDto::from).collect();
    let _ = state.ws_broadcast.send(WsMessage::AlarmsChanged {
        device_id,
        alarms: dtos,
    });
    Json(ApiResponse::ok_message("告警已确认"))
}

/// POST /api/alarms/:device_id/clear - 清除告警
pub async fn clear_alarms(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    state.alarm_system.clear_records(&device_id).await;
    info!("Alarms cleared via API for device {}", device_id);
    let _ = state.ws_broadcast.send(WsMessage::Log {
        message: format!("告警记录已清除: {}", device_id),
        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
    });
    let _ = state.ws_broadcast.send(WsMessage::AlarmsChanged {
        device_id: device_id.clone(),
        alarms: vec![],
    });
    Json(ApiResponse::ok_message("告警已清除"))
}
