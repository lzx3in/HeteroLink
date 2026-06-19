import api from './index'
import type { ApiResponse, MqttConnectRequest } from '@/types'

export function connectMqtt(req: MqttConnectRequest) {
  return api.post<ApiResponse<void>>('/mqtt/connect', req)
}

export function disconnectMqtt() {
  return api.post<ApiResponse<void>>('/mqtt/disconnect')
}

export function fetchMqttStatus() {
  return api.get<ApiResponse<{ connected: boolean }>>('/mqtt/status')
}
