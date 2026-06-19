import api from './index'
import type { ApiResponse, AlarmRecord, AlarmConfig } from '@/types'

export function fetchAlarms(deviceId: string) {
  return api.get<ApiResponse<AlarmRecord[]>>(`/alarms/${deviceId}`)
}

export function configureAlarm(deviceId: string, config: AlarmConfig[]) {
  return api.put<ApiResponse<void>>(`/alarms/${deviceId}/config`, { channels: config })
}

export function acknowledgeAlarm(deviceId: string, channel: number) {
  return api.post<ApiResponse<void>>(`/alarms/${deviceId}/acknowledge/${channel}`)
}

export function clearAlarms(deviceId: string) {
  return api.post<ApiResponse<void>>(`/alarms/${deviceId}/clear`)
}
