use axum::extract::{Path, State};
use axum::Json;

use crate::api::dto::{ApiResponse, AddDeviceRequest, DeviceInfoDto, TelemetryDto, ChannelStatsDto};

/// GET /api/devices - 获取设备列表
pub async fn list_devices(
    State(state): State<AppState>,
) -> Json<ApiResponse<Vec<DeviceInfoDto>>> {
    let devices = state.device_service.list_devices().await;
    let dtos: Vec<DeviceInfoDto> = devices.iter().map(DeviceInfoDto::from).collect();
    Json(ApiResponse::ok(dtos))
}

/// POST /api/devices - 添加设备
pub async fn add_device(
    State(state): State<AppState>,
    Json(req): Json<AddDeviceRequest>,
) -> Json<ApiResponse<DeviceInfoDto>> {
    match state.device_service.add_device(&req.name).await {
        Ok(device) => Json(ApiResponse::ok(DeviceInfoDto::from(&device))),
        Err(e) => Json(ApiResponse::error(&format!("添加设备失败: {}", e))),
    }
}

/// DELETE /api/devices/:device_id - 移除设备
pub async fn remove_device(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    match state.device_service.remove_device(&device_id).await {
        Ok(_) => Json(ApiResponse::ok_message("设备已移除")),
        Err(e) => Json(ApiResponse::error(&format!("移除设备失败: {}", e))),
    }
}

/// POST /api/devices/:device_id/disconnect - 断开设备
pub async fn disconnect_device(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    state.device_service.disconnect_device(&device_id).await;
    Json(ApiResponse::ok_message("设备已断开"))
}

/// GET /api/devices/:device_id/telemetry - 获取最新遥测数据
pub async fn get_telemetry(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<Vec<TelemetryDto>>> {
    let data = state.telemetry_service.get_latest(&device_id, 50).await;
    let dtos: Vec<TelemetryDto> = data
        .iter()
        .map(|d| TelemetryDto {
            timestamp: d.timestamp,
            channels: d.channels.clone(),
        })
        .collect();
    Json(ApiResponse::ok(dtos))
}

/// GET /api/devices/:device_id/stats - 获取通道统计
pub async fn get_stats(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<Vec<ChannelStatsDto>>> {
    let stats = state.telemetry_service.get_stats(&device_id).await;
    let dtos: Vec<ChannelStatsDto> = stats.into_iter().map(ChannelStatsDto::from).collect();
    Json(ApiResponse::ok(dtos))
}

// AppState type alias to reduce repetition
use crate::api::AppState;
