use axum::extract::State;
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::dto::{ApiResponse, StartRecordingRequest};

/// POST /api/recording/start - 开始录制
pub async fn start(
    State(state): State<AppState>,
    Json(req): Json<StartRecordingRequest>,
) -> Json<ApiResponse<()>> {
    match state.recording_service.start(&req.device_id, req.path.as_deref()).await {
        Ok(_) => {
            info!("Recording started for device {}", req.device_id);
            Json(ApiResponse::ok_message("录制已开始"))
        }
        Err(e) => Json(ApiResponse::error(&format!("开始录制失败: {}", e))),
    }
}

/// POST /api/recording/stop - 停止录制
pub async fn stop(
    State(state): State<AppState>,
) -> Json<ApiResponse<()>> {
    state.recording_service.stop().await;
    info!("Recording stopped via API");
    Json(ApiResponse::ok_message("录制已停止"))
}

/// GET /api/recording/status - 录制状态
pub async fn status(
    State(state): State<AppState>,
) -> Json<ApiResponse<serde_json::Value>> {
    let recording = state.recording_service.is_recording().await;
    Json(ApiResponse::ok(serde_json::json!({ "recording": recording })))
}
