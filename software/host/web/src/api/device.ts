import api from './index'
import type { ApiResponse, DeviceInfo, TelemetryDto, ChannelStatsDto } from '@/types'

export function fetchDevices() {
  return api.get<ApiResponse<DeviceInfo[]>>('/devices')
}

export function addDevice(name: string) {
  return api.post<ApiResponse<DeviceInfo>>('/devices', { name })
}

export function removeDevice(id: string) {
  return api.delete<ApiResponse<void>>(`/devices/${id}`)
}

export function disconnectDevice(id: string) {
  return api.post<ApiResponse<void>>(`/devices/${id}/disconnect`)
}

export function fetchTelemetry(id: string) {
  return api.get<ApiResponse<TelemetryDto[]>>(`/devices/${id}/telemetry`)
}

export function fetchStats(id: string) {
  return api.get<ApiResponse<ChannelStatsDto[]>>(`/devices/${id}/stats`)
}
