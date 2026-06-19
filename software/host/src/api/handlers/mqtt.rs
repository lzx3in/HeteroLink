use axum::extract::State;
use axum::Json;
use tracing::{info, warn};

use crate::api::AppState;
use crate::api::broadcast::WsMessage;
use crate::api::dto::{ApiResponse, MqttConnectRequest};
use crate::protocol::MqttConfig;

/// POST /api/mqtt/connect - 连接 MQTT
pub async fn connect(
    State(state): State<AppState>,
    Json(req): Json<MqttConnectRequest>,
) -> Json<ApiResponse<()>> {
    if req.broker_host.is_empty() {
        return Json(ApiResponse::error("请设置 Broker 地址"));
    }

    info!("MQTT connect request: {}:{}", req.broker_host, req.broker_port);

    let _ = state.ws_broadcast.send(WsMessage::Log {
        message: format!("正在连接 MQTT: {}:{}...", req.broker_host, req.broker_port),
        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
    });

    let config = MqttConfig {
        broker_host: req.broker_host,
        broker_port: req.broker_port,
        username: req.username.filter(|s| !s.is_empty()),
        password: req.password.filter(|s| !s.is_empty()),
        client_id: req.client_id.unwrap_or_else(|| {
            format!("heterolink-host-{}", &uuid::Uuid::new_v4().to_string()[..8])
        }),
        use_tls: req.use_tls,
    };

    let mut mqtt_ch = state.mqtt_channel.lock().await;
    mqtt_ch.update_config(config);
    match mqtt_ch.connect(state.mqtt_event_tx.clone()).await {
        Ok(_) => {
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: "MQTT 连接中...".to_string(),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            Json(ApiResponse::ok_message("MQTT 连接中"))
        }
        Err(e) => {
            warn!("MQTT connect failed: {}", e);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("MQTT 连接失败: {}", e),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            Json(ApiResponse::error(&format!("MQTT 连接失败: {}", e)))
        }
    }
}

/// POST /api/mqtt/disconnect - 断开 MQTT
pub async fn disconnect(
    State(state): State<AppState>,
) -> Json<ApiResponse<()>> {
    match state.mqtt_channel.lock().await.disconnect().await {
        Ok(_) => {
            info!("MQTT disconnected via API");
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: "MQTT 已断开".to_string(),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            Json(ApiResponse::ok_message("MQTT 已断开"))
        }
        Err(e) => Json(ApiResponse::error(&format!("MQTT 断开失败: {}", e))),
    }
}

/// GET /api/mqtt/status - MQTT 状态
pub async fn status(
    State(state): State<AppState>,
) -> Json<ApiResponse<serde_json::Value>> {
    let connected = state.mqtt_channel.lock().await.is_connected().await;
    Json(ApiResponse::ok(serde_json::json!({ "connected": connected })))
}
