use axum::extract::State;
use axum::Json;
use tracing::info;

use crate::api::dto::{ApiResponse, MqttConnectRequest};
use crate::domain::config::MqttConfig;

use crate::api::AppState;

/// POST /api/mqtt/connect - 连接 MQTT
pub async fn connect(
    State(state): State<AppState>,
    Json(req): Json<MqttConnectRequest>,
) -> Json<ApiResponse<()>> {
    if req.broker_host.is_empty() {
        return Json(ApiResponse::error("请设置 Broker 地址"));
    }

    info!("MQTT connect request: {}:{}", req.broker_host, req.broker_port);
    state
        .event_bus
        .emit_log(format!("正在连接 MQTT: {}:{}...", req.broker_host, req.broker_port));

    let config = MqttConfig {
        broker_host: req.broker_host,
        broker_port: req.broker_port,
        username: req.username.filter(|s| !s.is_empty()).unwrap_or_default(),
        password: req.password.filter(|s| !s.is_empty()).unwrap_or_default(),
        client_id: req.client_id.unwrap_or_else(|| {
            format!("heterolink-host-{}", &uuid::Uuid::new_v4().to_string()[..8])
        }),
        use_tls: req.use_tls,
    };

    state.mqtt_service.connect(config).await;
    state.event_bus.emit_log("MQTT 连接中...");
    Json(ApiResponse::ok_message("MQTT 连接中"))
}

/// POST /api/mqtt/disconnect - 断开 MQTT
pub async fn disconnect(
    State(state): State<AppState>,
) -> Json<ApiResponse<()>> {
    state.mqtt_service.disconnect().await;
    info!("MQTT disconnected via API");
    state.event_bus.emit_log("MQTT 已断开");
    Json(ApiResponse::ok_message("MQTT 已断开"))
}

/// GET /api/mqtt/status - MQTT 状态
pub async fn status(
    State(state): State<AppState>,
) -> Json<ApiResponse<serde_json::Value>> {
    let connected = state.mqtt_service.is_connected().await;
    Json(ApiResponse::ok(serde_json::json!({ "connected": connected })))
}
