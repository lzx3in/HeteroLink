use axum::extract::State;
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::dto::{ApiResponse, UpdateStatusDto};

/// GET /api/update/status - 获取当前更新状态
pub async fn status(
    State(state): State<AppState>,
) -> Json<ApiResponse<UpdateStatusDto>> {
    let current = state.updater_service.current_version().await;
    let update_status = state.updater_service.status().await;
    Json(ApiResponse::ok(UpdateStatusDto::from((current.as_str(), &update_status))))
}

/// POST /api/update/check - 检查更新
pub async fn check(
    State(state): State<AppState>,
) -> Json<ApiResponse<UpdateStatusDto>> {
    info!("Update check requested via API");
    let current = state.updater_service.current_version().await;
    let update_status = state.updater_service.check_for_update().await;
    Json(ApiResponse::ok(UpdateStatusDto::from((current.as_str(), &update_status))))
}

/// POST /api/update/apply - 下载并应用更新
pub async fn apply(
    State(state): State<AppState>,
) -> Json<ApiResponse<UpdateStatusDto>> {
    info!("Update apply requested via API");
    match state.updater_service.apply_update().await {
        Ok(version) => {
            let current = state.updater_service.current_version().await;
            let update_status = state.updater_service.status().await;
            let mut dto = UpdateStatusDto::from((current.as_str(), &update_status));
            dto.new_version = Some(version);
            Json(ApiResponse::ok(dto))
        }
        Err(msg) => Json(ApiResponse::error(&msg)),
    }
}
