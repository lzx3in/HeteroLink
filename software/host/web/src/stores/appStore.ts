import { defineStore } from 'pinia'
import { ref } from 'vue'
import { startRecording as apiStartRecording, stopRecording as apiStopRecording } from '@/api/recording'
import { sendStartCommand as apiSendStartCommand, sendStopCommand as apiSendStopCommand, sendGpioCommand as apiSendGpioCommand } from '@/api/command'
import { exportCsv as apiExportCsv, exportJson as apiExportJson, downloadFile } from '@/api/export'

export const useAppStore = defineStore('app', () => {
  const mqttConnected = ref(false)
  const recording = ref(false)
  const recordingDeviceId = ref<string | null>(null)
  const statusMessage = ref('就绪')
  const lastCommandResponse = ref<any>(null)

  async function startRecording(deviceId: string) {
    try {
      await apiStartRecording(deviceId)
      recording.value = true
      recordingDeviceId.value = deviceId
    } catch (e) {
      console.error('Failed to start recording', e)
    }
  }

  async function stopRecording() {
    try {
      await apiStopRecording()
      recording.value = false
      recordingDeviceId.value = null
    } catch (e) {
      console.error('Failed to stop recording', e)
    }
  }

  async function exportCsv(deviceId: string) {
    try {
      const res = await apiExportCsv(deviceId)
      downloadFile(res.data, `export_${deviceId}_${Date.now()}.csv`)
    } catch (e) {
      console.error('Failed to export CSV', e)
    }
  }

  async function exportJson(deviceId: string) {
    try {
      const res = await apiExportJson(deviceId)
      downloadFile(res.data, `export_${deviceId}_${Date.now()}.json`)
    } catch (e) {
      console.error('Failed to export JSON', e)
    }
  }

  async function sendStartCommand(deviceId: string, sampleRate: number, channels: number[]) {
    try {
      await apiSendStartCommand(deviceId, sampleRate, channels)
    } catch (e) {
      console.error('Failed to send start command', e)
    }
  }

  async function sendStopCommand(deviceId: string) {
    try {
      await apiSendStopCommand(deviceId)
    } catch (e) {
      console.error('Failed to send stop command', e)
    }
  }

  async function sendGpioCommand(deviceId: string, pin: number, value: boolean) {
    try {
      await apiSendGpioCommand(deviceId, pin, value)
    } catch (e) {
      console.error('Failed to send GPIO command', e)
    }
  }

  return {
    mqttConnected,
    recording,
    recordingDeviceId,
    statusMessage,
    lastCommandResponse,
    startRecording,
    stopRecording,
    exportCsv,
    exportJson,
    sendStartCommand,
    sendStopCommand,
    sendGpioCommand,
  }
})
