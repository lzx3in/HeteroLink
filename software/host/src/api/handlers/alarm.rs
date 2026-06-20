use axum::extract::{Path, State};
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::dto::{ApiResponse, AlarmConfigRequest, AlarmRecordDto};
use crate::services::alarm_service::AlarmConfigInput;

/// GET /api/alarms/:device_id - 获取告警记录
pub async fn list_alarms(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<Vec<AlarmRecordDto>>> {
    let alarms = state.alarm_service.list_alarms(&device_id).await;
    let dtos: Vec<AlarmRecordDto> = alarms.iter().map(AlarmRecordDto::from).collect();
    Json(ApiResponse::ok(dtos))
}

/// PUT /api/alarms/:device_id/config - 配置告警
pub async fn configure_alarm(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<AlarmConfigRequest>,
) -> Json<ApiResponse<()>> {
    let input = AlarmConfigInput {
        channel_id: req.channel_id,
        lower_limit: req.lower_limit,
        upper_limit: req.upper_limit,
        lower_enabled: req.lower_enabled,
        upper_enabled: req.upper_enabled,
        enabled: req.enabled,
    };
    state.alarm_service.configure_alarm(&device_id, &input).await;
    info!("Alarm configured via API for device {} channel {}", device_id, req.channel_id);
    Json(ApiResponse::ok_message("告警配置已保存"))
}

/// POST /api/alarms/:device_id/acknowledge/:channel_id - 确认告警
pub async fn acknowledge(
    State(state): State<AppState>,
    Path((device_id, channel_id)): Path<(String, i32)>,
) -> Json<ApiResponse<()>> {
    state.alarm_service.acknowledge(&device_id, channel_id).await;
    info!("Alarm acknowledged via API for device {} channel {}", device_id, channel_id);
    Json(ApiResponse::ok_message("告警已确认"))
}

/// POST /api/alarms/:device_id/clear - 清除告警
pub async fn clear_alarms(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    state.alarm_service.clear_alarms(&device_id).await;
    info!("Alarms cleared via API for device {}", device_id);
    Json(ApiResponse::ok_message("告警已清除"))
}
