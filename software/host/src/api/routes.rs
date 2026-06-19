use axum::routing::{delete, get, post, put};
use axum::Router;
use tower_http::cors::CorsLayer;
use tower_http::services::ServeDir;

use super::handlers;
use super::ws;
use super::AppState;

pub fn create_router(state: AppState) -> Router {
    let api_routes = Router::new()
        // 设备管理
        .route("/devices", get(handlers::device::list_devices))
        .route("/devices", post(handlers::device::add_device))
        .route("/devices/{device_id}", delete(handlers::device::remove_device))
        .route(
            "/devices/{device_id}/disconnect",
            post(handlers::device::disconnect_device),
        )
        .route(
            "/devices/{device_id}/telemetry",
            get(handlers::device::get_telemetry),
        )
        .route(
            "/devices/{device_id}/stats",
            get(handlers::device::get_stats),
        )
        // MQTT 控制
        .route("/mqtt/connect", post(handlers::mqtt::connect))
        .route("/mqtt/disconnect", post(handlers::mqtt::disconnect))
        .route("/mqtt/status", get(handlers::mqtt::status))
        // 配置管理
        .route("/config", get(handlers::config::get_config))
        .route("/config", put(handlers::config::save_config))
        .route("/config/load", post(handlers::config::load_config))
        .route("/config/reset", post(handlers::config::reset_config))
        // 告警管理
        .route("/alarms/{device_id}", get(handlers::alarm::list_alarms))
        .route(
            "/alarms/{device_id}/config",
            put(handlers::alarm::configure_alarm),
        )
        .route(
            "/alarms/{device_id}/acknowledge/{channel_id}",
            post(handlers::alarm::acknowledge),
        )
        .route(
            "/alarms/{device_id}/clear",
            post(handlers::alarm::clear_alarms),
        )
        // 数据导出
        .route(
            "/export/{device_id}/csv",
            post(handlers::export::export_csv),
        )
        .route(
            "/export/{device_id}/json",
            post(handlers::export::export_json),
        )
        // 录制控制
        .route("/recording/start", post(handlers::recording::start))
        .route("/recording/stop", post(handlers::recording::stop))
        .route("/recording/status", get(handlers::recording::status))
        // 设备命令
        .route(
            "/command/{device_id}/start",
            post(handlers::command::start_sampling),
        )
        .route(
            "/command/{device_id}/stop",
            post(handlers::command::stop_sampling),
        )
        .route(
            "/command/{device_id}/gpio",
            post(handlers::command::set_gpio),
        );

    Router::new()
        .nest("/api", api_routes)
        .route("/ws", get(ws::ws_handler))
        // 静态文件服务 - 前端构建产物
        .fallback_service(ServeDir::new("web/dist"))
        .layer(CorsLayer::permissive())
        .with_state(state)
}
