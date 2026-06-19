import api from './index'
import type { ApiResponse, DeviceInfo, TelemetryData, ChannelStats } from '@/types'

export function fetchDevices() {
  return api.get<ApiResponse<DeviceInfo[]>>('/devices')
}

export function addDevice(id: string) {
  return api.post<ApiResponse<DeviceInfo>>('/devices', { id })
}

export function removeDevice(id: string) {
  return api.delete<ApiResponse<void>>(`/devices/${id}`)
}

export function disconnectDevice(id: string) {
  return api.post<ApiResponse<void>>(`/devices/${id}/disconnect`)
}

export function fetchTelemetry(id: string) {
  return api.get<ApiResponse<TelemetryData[]>>(`/devices/${id}/telemetry`)
}

export function fetchStats(id: string) {
  return api.get<ApiResponse<ChannelStats[]>>(`/devices/${id}/stats`)
}
