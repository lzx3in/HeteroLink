use axum::extract::State;
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::dto::ApiResponse;
use crate::storage::config::AppConfig;

/// GET /api/config - 获取当前配置
pub async fn get_config(
    State(state): State<AppState>,
) -> Json<ApiResponse<AppConfig>> {
    Json(ApiResponse::ok(state.config_service.get_config().await))
}

/// PUT /api/config - 保存配置
pub async fn save_config(
    State(state): State<AppState>,
    Json(new_config): Json<AppConfig>,
) -> Json<ApiResponse<()>> {
    match state.config_service.save_config(new_config).await {
        Ok(_) => {
            info!("Config saved via API");
            Json(ApiResponse::ok_message("配置已保存"))
        }
        Err(e) => Json(ApiResponse::error(&format!("保存配置失败: {}", e))),
    }
}

/// POST /api/config/load - 从文件加载配置
pub async fn load_config(
    State(state): State<AppState>,
) -> Json<ApiResponse<AppConfig>> {
    match state.config_service.load_config().await {
        Ok(config) => {
            info!("Config loaded via API");
            Json(ApiResponse::ok(config))
        }
        Err(e) => Json(ApiResponse::error(&format!("加载配置失败: {}", e))),
    }
}

/// POST /api/config/reset - 重置默认配置
pub async fn reset_config(
    State(state): State<AppState>,
) -> Json<ApiResponse<AppConfig>> {
    info!("Config reset via API");
    Json(ApiResponse::ok(state.config_service.reset_config().await))
}
