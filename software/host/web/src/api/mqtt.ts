import api from './index'
import type { ApiResponse, MqttConfig } from '@/types'

export function connectMqtt(config: MqttConfig) {
  return api.post<ApiResponse<void>>('/mqtt/connect', config)
}

export function disconnectMqtt() {
  return api.post<ApiResponse<void>>('/mqtt/disconnect')
}

export function fetchMqttStatus() {
  return api.get<ApiResponse<{ connected: boolean; config: MqttConfig | null }>>('/mqtt/status')
}
