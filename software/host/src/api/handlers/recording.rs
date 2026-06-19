use axum::extract::State;
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::broadcast::WsMessage;
use crate::api::dto::{ApiResponse, StartRecordingRequest};

/// POST /api/recording/start - 开始录制
pub async fn start(
    State(state): State<AppState>,
    Json(req): Json<StartRecordingRequest>,
) -> Json<ApiResponse<()>> {
    let base_path = req.path.unwrap_or_else(|| "./logs".to_string());

    match state
        .data_logger
        .lock()
        .await
        .start_recording(&base_path, &req.device_id)
    {
        Ok(_) => {
            info!("Recording started for device {}", req.device_id);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("开始记录: {}", req.device_id),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            let _ = state
                .ws_broadcast
                .send(WsMessage::RecordingChanged { recording: true });
            Json(ApiResponse::ok_message("录制已开始"))
        }
        Err(e) => Json(ApiResponse::error(&format!("开始录制失败: {}", e))),
    }
}

/// POST /api/recording/stop - 停止录制
pub async fn stop(
    State(state): State<AppState>,
) -> Json<ApiResponse<()>> {
    state.data_logger.lock().await.stop_recording();
    info!("Recording stopped via API");
    let _ = state.ws_broadcast.send(WsMessage::Log {
        message: "停止记录".to_string(),
        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
    });
    let _ = state
        .ws_broadcast
        .send(WsMessage::RecordingChanged { recording: false });
    Json(ApiResponse::ok_message("录制已停止"))
}

/// GET /api/recording/status - 录制状态
pub async fn status(
    State(state): State<AppState>,
) -> Json<ApiResponse<serde_json::Value>> {
    let recording = state.data_logger.lock().await.is_recording();
    Json(ApiResponse::ok(serde_json::json!({ "recording": recording })))
}
