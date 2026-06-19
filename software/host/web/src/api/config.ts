import api from './index'
import type { ApiResponse, AppConfig } from '@/types'

export function fetchConfig() {
  return api.get<ApiResponse<AppConfig>>('/config')
}

export function saveConfig(config: AppConfig) {
  return api.put<ApiResponse<void>>('/config', config)
}

export function loadConfig() {
  return api.post<ApiResponse<AppConfig>>('/config/load')
}

export function resetConfig() {
  return api.post<ApiResponse<AppConfig>>('/config/reset')
}
