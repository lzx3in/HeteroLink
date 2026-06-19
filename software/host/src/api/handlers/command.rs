use axum::extract::{Path, State};
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::broadcast::WsMessage;
use crate::api::dto::{ApiResponse, StartCommandRequest, GpioCommandRequest};
use crate::protocol::MqttChannel;
use crate::protocol::MqttEvent;

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

    send_command(&state, &device_id, &cmd, &format!("start ({}Hz)", req.sample_rate)).await;
    Json(ApiResponse::ok_message("命令已发送"))
}

/// POST /api/command/:device_id/stop - 停止采样
pub async fn stop_sampling(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    let cmd = serde_json::json!({"cmd": "stop"}).to_string();
    send_command(&state, &device_id, &cmd, "stop").await;
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

    send_command(
        &state,
        &device_id,
        &cmd,
        &format!("set_gpio ch{}={}", req.channel, gpio_val),
    )
    .await;
    Json(ApiResponse::ok_message("命令已发送"))
}

async fn send_command(state: &AppState, device_id: &str, cmd: &str, desc: &str) {
    if let Some(ref sim_tx) = state.simulation_tx {
        let response = MqttChannel::simulate_response(cmd);
        let _ = sim_tx
            .send(MqttEvent::ResponseReceived {
                device_id: device_id.to_string(),
                response,
            })
            .await;
        let _ = state.ws_broadcast.send(WsMessage::Log {
            message: format!("[模拟] -> {} -> {}", desc, device_id),
            timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
        });
    } else {
        let mqtt_ch = state.mqtt_channel.lock().await;
        match mqtt_ch.publish_command(device_id, cmd).await {
            Ok(_) => {
                info!("Command sent: {} -> {}", desc, device_id);
                let _ = state.ws_broadcast.send(WsMessage::Log {
                    message: format!("-> 发送命令: {} -> {}", desc, device_id),
                    timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                });
            }
            Err(e) => {
                let _ = state.ws_broadcast.send(WsMessage::Log {
                    message: format!("发送失败: {}", e),
                    timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
                });
            }
        }
    }
}
