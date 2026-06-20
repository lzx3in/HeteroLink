use axum::extract::{Path, State};
use axum::response::{IntoResponse, Response};
use axum::body::Body;
use axum::http::{header, StatusCode};
use tracing::error;

use crate::api::AppState;
use crate::infra::time_util::now_datetime_file;

/// POST /api/export/:device_id/csv - 导出 CSV
pub async fn export_csv(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> impl IntoResponse {
    let filename = format!("export_{}_{}.csv", device_id, now_datetime_file());

    match state.export_service.export_csv_bytes(&device_id).await {
        Ok(data) => Response::builder()
            .status(StatusCode::OK)
            .header(header::CONTENT_TYPE, "text/csv; charset=utf-8")
            .header(header::CONTENT_DISPOSITION, format!("attachment; filename=\"{}\"", filename))
            .body(Body::from(data))
            .unwrap(),
        Err(e) => {
            error!("CSV export failed: {}", e);
            Response::builder()
                .status(StatusCode::INTERNAL_SERVER_ERROR)
                .body(Body::from(format!("导出失败: {}", e)))
                .unwrap()
        }
    }
}

/// POST /api/export/:device_id/json - 导出 JSON
pub async fn export_json(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> impl IntoResponse {
    let filename = format!("export_{}_{}.json", device_id, now_datetime_file());

    match state.export_service.export_json_bytes(&device_id).await {
        Ok(data) => Response::builder()
            .status(StatusCode::OK)
            .header(header::CONTENT_TYPE, "application/json; charset=utf-8")
            .header(header::CONTENT_DISPOSITION, format!("attachment; filename=\"{}\"", filename))
            .body(Body::from(data))
            .unwrap(),
        Err(e) => {
            error!("JSON export failed: {}", e);
            Response::builder()
                .status(StatusCode::INTERNAL_SERVER_ERROR)
                .body(Body::from(format!("导出失败: {}", e)))
                .unwrap()
        }
    }
}
