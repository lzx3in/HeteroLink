use axum::extract::{Path, State};
use axum::Json;

use crate::api::AppState;
use crate::api::dto::{ApiResponse, StartCommandRequest, GpioCommandRequest};

/// POST /api/command/:device_id/start - 开始采样
pub async fn start_sampling(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<StartCommandRequest>,
) -> Json<ApiResponse<()>> {
    let cmd = serde_json::json!({
        "cmd": "start",
        "params": { "sample_rate": req.sample_rate }
    })
    .to_string();

    state.command_service.send_command(&device_id, &cmd, &format!("start ({}Hz)", req.sample_rate)).await;
    Json(ApiResponse::ok_message("命令已发送"))
}

/// POST /api/command/:device_id/stop - 停止采样
pub async fn stop_sampling(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    let cmd = serde_json::json!({"cmd": "stop"}).to_string();
    state.command_service.send_command(&device_id, &cmd, "stop").await;
    Json(ApiResponse::ok_message("命令已发送"))
}

/// POST /api/command/:device_id/gpio - 设置 GPIO
pub async fn set_gpio(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
    Json(req): Json<GpioCommandRequest>,
) -> Json<ApiResponse<()>> {
    let gpio_val: i32 = if req.value { 1 } else { 0 };
    let cmd = serde_json::json!({
        "cmd": "set_gpio",
        "channel": req.channel,
        "value": gpio_val
    })
    .to_string();

    state.command_service.send_command(
        &device_id,
        &cmd,
        &format!("set_gpio ch{}={}", req.channel, gpio_val),
    ).await;
    Json(ApiResponse::ok_message("命令已发送"))
}
