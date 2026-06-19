import api from './index'
import type { ApiResponse } from '@/types'

export interface RecordingStatus {
  recording: boolean
  device_id: string | null
  start_time: string | null
  samples_recorded: number
}

export function startRecording(deviceId: string, path?: string) {
  return api.post<ApiResponse<void>>('/recording/start', {
    device_id: deviceId,
    path,
  })
}

export function stopRecording() {
  return api.post<ApiResponse<void>>('/recording/stop')
}

export function fetchRecordingStatus() {
  return api.get<ApiResponse<RecordingStatus>>('/recording/status')
}
