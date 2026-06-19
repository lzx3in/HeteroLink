import api from './index'
import type { ApiResponse } from '@/types'

export function sendStartCommand(deviceId: string, sampleRate: number) {
  return api.post<ApiResponse<void>>(`/command/${deviceId}/start`, {
    sample_rate: sampleRate,
  })
}

export function sendStopCommand(deviceId: string) {
  return api.post<ApiResponse<void>>(`/command/${deviceId}/stop`)
}

export function sendGpioCommand(deviceId: string, channel: number, value: boolean) {
  return api.post<ApiResponse<void>>(`/command/${deviceId}/gpio`, {
    channel,
    value,
  })
}
