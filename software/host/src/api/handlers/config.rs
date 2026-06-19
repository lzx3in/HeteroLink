use axum::extract::State;
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::broadcast::WsMessage;
use crate::api::dto::ApiResponse;
use crate::storage::config::AppConfig;

/// GET /api/config - 获取当前配置
pub async fn get_config(
    State(state): State<AppState>,
) -> Json<ApiResponse<AppConfig>> {
    let config = state.config_manager.lock().await;
    Json(ApiResponse::ok(config.get().clone()))
}

/// PUT /api/config - 保存配置
pub async fn save_config(
    State(state): State<AppState>,
    Json(new_config): Json<AppConfig>,
) -> Json<ApiResponse<()>> {
    let mut config = state.config_manager.lock().await;
    *config.get_mut() = new_config;
    match config.save(None) {
        Ok(_) => {
            info!("Config saved via API");
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: "配置已保存".to_string(),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            Json(ApiResponse::ok_message("配置已保存"))
        }
        Err(e) => Json(ApiResponse::error(&format!("保存配置失败: {}", e))),
    }
}

/// POST /api/config/load - 从文件加载配置
pub async fn load_config(
    State(state): State<AppState>,
) -> Json<ApiResponse<AppConfig>> {
    let mut config = state.config_manager.lock().await;
    match config.load(None) {
        Ok(_) => {
            info!("Config loaded via API");
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: "配置已加载".to_string(),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            Json(ApiResponse::ok(config.get().clone()))
        }
        Err(e) => Json(ApiResponse {
            success: false,
            data: None,
            message: Some(format!("加载配置失败: {}", e)),
        }),
    }
}

/// POST /api/config/reset - 重置默认配置
pub async fn reset_config(
    State(state): State<AppState>,
) -> Json<ApiResponse<AppConfig>> {
    let mut config = state.config_manager.lock().await;
    config.reset();
    info!("Config reset via API");
    let _ = state.ws_broadcast.send(WsMessage::Log {
        message: "配置已重置".to_string(),
        timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
    });
    Json(ApiResponse::ok(config.get().clone()))
}
