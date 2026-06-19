use axum::extract::{Path, State};
use axum::response::{IntoResponse, Response};
use axum::body::Body;
use axum::http::{header, StatusCode};
use tracing::{info, error};

use crate::api::AppState;
use crate::api::broadcast::WsMessage;

/// POST /api/export/:device_id/csv - 导出 CSV
pub async fn export_csv(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> impl IntoResponse {
    let filename = format!(
        "export_{}_{}.csv",
        device_id,
        chrono::Local::now().format("%Y%m%d_%H%M%S")
    );
    let tmp_path = format!("/tmp/{}", filename);

    match state.data_processor.export_to_csv(&device_id, &tmp_path).await {
        Ok(_) => {
            info!("CSV exported: {}", tmp_path);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("已导出: {}", filename),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });

            match tokio::fs::read(&tmp_path).await {
                Ok(data) => {
                    let _ = tokio::fs::remove_file(&tmp_path).await;
                    Response::builder()
                        .status(StatusCode::OK)
                        .header(header::CONTENT_TYPE, "text/csv; charset=utf-8")
                        .header(
                            header::CONTENT_DISPOSITION,
                            format!("attachment; filename=\"{}\"", filename),
                        )
                        .body(Body::from(data))
                        .unwrap()
                }
                Err(e) => {
                    error!("Failed to read export file: {}", e);
                    Response::builder()
                        .status(StatusCode::INTERNAL_SERVER_ERROR)
                        .body(Body::from(format!("导出失败: {}", e)))
                        .unwrap()
                }
            }
        }
        Err(e) => {
            error!("CSV export failed: {}", e);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("导出失败: {}", e),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
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
    let filename = format!(
        "export_{}_{}.json",
        device_id,
        chrono::Local::now().format("%Y%m%d_%H%M%S")
    );
    let tmp_path = format!("/tmp/{}", filename);

    match state.data_processor.export_to_json(&device_id, &tmp_path).await {
        Ok(_) => {
            info!("JSON exported: {}", tmp_path);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("已导出: {}", filename),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });

            match tokio::fs::read(&tmp_path).await {
                Ok(data) => {
                    let _ = tokio::fs::remove_file(&tmp_path).await;
                    Response::builder()
                        .status(StatusCode::OK)
                        .header(header::CONTENT_TYPE, "application/json; charset=utf-8")
                        .header(
                            header::CONTENT_DISPOSITION,
                            format!("attachment; filename=\"{}\"", filename),
                        )
                        .body(Body::from(data))
                        .unwrap()
                }
                Err(e) => {
                    error!("Failed to read export file: {}", e);
                    Response::builder()
                        .status(StatusCode::INTERNAL_SERVER_ERROR)
                        .body(Body::from(format!("导出失败: {}", e)))
                        .unwrap()
                }
            }
        }
        Err(e) => {
            error!("JSON export failed: {}", e);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("导出失败: {}", e),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            Response::builder()
                .status(StatusCode::INTERNAL_SERVER_ERROR)
                .body(Body::from(format!("导出失败: {}", e)))
                .unwrap()
        }
    }
}
