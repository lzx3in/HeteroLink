import api from './index'

export function exportCsv(deviceId: string) {
  return api.post(`/export/${deviceId}/csv`, null, { responseType: 'blob' })
}

export function exportJson(deviceId: string) {
  return api.post(`/export/${deviceId}/json`, null, { responseType: 'blob' })
}

export function downloadFile(blob: Blob, filename: string) {
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = filename
  a.click()
  URL.revokeObjectURL(url)
}
