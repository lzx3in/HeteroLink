use axum::extract::{Path, State};
use axum::Json;
use tracing::info;

use crate::api::AppState;
use crate::api::broadcast::WsMessage;
use crate::api::dto::{
    ApiResponse, AddDeviceRequest, DeviceInfoDto, TelemetryDto, ChannelStatsDto,
};
use crate::core::DeviceInfo;

/// GET /api/devices - 获取设备列表
pub async fn list_devices(
    State(state): State<AppState>,
) -> Json<ApiResponse<Vec<DeviceInfoDto>>> {
    let devices = state.device_manager.lock().await.get_devices().await;
    let device_list: Vec<DeviceInfoDto> = devices.values().map(DeviceInfoDto::from).collect();
    Json(ApiResponse::ok(device_list))
}

/// POST /api/devices - 添加设备
pub async fn add_device(
    State(state): State<AppState>,
    Json(req): Json<AddDeviceRequest>,
) -> Json<ApiResponse<DeviceInfoDto>> {
    let device_id = format!("device_{}", chrono::Local::now().timestamp());
    let device = DeviceInfo::new(device_id.clone(), req.name.clone());

    match state.device_manager.lock().await.add_device(device.clone()).await {
        Ok(_) => {
            info!("Device added via API: {} ({})", req.name, device_id);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("已添加设备: {} ({})", req.name, device_id),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            // Broadcast device list update
            broadcast_device_list(&state).await;
            Json(ApiResponse::ok(DeviceInfoDto::from(device)))
        }
        Err(e) => {
            Json(ApiResponse {
                success: false,
                data: None,
                message: Some(format!("添加设备失败: {}", e)),
            })
        }
    }
}

/// DELETE /api/devices/:device_id - 移除设备
pub async fn remove_device(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    match state.device_manager.lock().await.remove_device(&device_id).await {
        Ok(_) => {
            info!("Device removed via API: {}", device_id);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("已移除设备: {}", device_id),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            broadcast_device_list(&state).await;
            Json(ApiResponse::ok_message("设备已移除"))
        }
        Err(e) => Json(ApiResponse::error(&format!("移除设备失败: {}", e))),
    }
}

/// POST /api/devices/:device_id/disconnect - 断开设备
pub async fn disconnect_device(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<()>> {
    match state.device_manager.lock().await.disconnect_device(&device_id).await {
        Ok(_) => {
            info!("Device disconnected via API: {}", device_id);
            let _ = state.ws_broadcast.send(WsMessage::Log {
                message: format!("已断开设备: {}", device_id),
                timestamp: chrono::Local::now().format("%H:%M:%S").to_string(),
            });
            broadcast_device_list(&state).await;
            Json(ApiResponse::ok_message("设备已断开"))
        }
        Err(e) => Json(ApiResponse::error(&format!("断开设备失败: {}", e))),
    }
}

/// GET /api/devices/:device_id/telemetry - 获取最新遥测数据
pub async fn get_telemetry(
    State(state): State<AppState>,
    Path(device_id): Path<String>,
) -> Json<ApiResponse<Vec<TelemetryDto>>> {
    let data = state.data_processor.get_latest_data(&device_id, 50).await;
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
    let stats = state.data_processor.get_stats(&device_id).await;
    let dtos: Vec<ChannelStatsDto> = stats.into_iter().map(ChannelStatsDto::from).collect();
    Json(ApiResponse::ok(dtos))
}

/// 广播设备列表更新
pub async fn broadcast_device_list(state: &AppState) {
    let devices = state.device_manager.lock().await.get_devices().await;
    let device_list: Vec<DeviceInfoDto> = devices.values().map(DeviceInfoDto::from).collect();
    let connected = device_list.iter().filter(|d| d.connected).count();
    let online = device_list.iter().filter(|d| d.online).count();
    let _ = state.ws_broadcast.send(WsMessage::DeviceListChanged {
        devices: device_list,
        connected_count: connected,
        online_count: online,
    });
}
