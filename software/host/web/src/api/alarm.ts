import api from './index'
import type { ApiResponse, AlarmRecord, AlarmConfigRequest } from '@/types'

export function fetchAlarms(deviceId: string) {
  return api.get<ApiResponse<AlarmRecord[]>>(`/alarms/${deviceId}`)
}

export function configureAlarm(deviceId: string, config: AlarmConfigRequest) {
  return api.put<ApiResponse<void>>(`/alarms/${deviceId}/config`, config)
}

export function acknowledgeAlarm(deviceId: string, channelId: number) {
  return api.post<ApiResponse<void>>(`/alarms/${deviceId}/acknowledge/${channelId}`)
}

export function clearAlarms(deviceId: string) {
  return api.post<ApiResponse<void>>(`/alarms/${deviceId}/clear`)
}
