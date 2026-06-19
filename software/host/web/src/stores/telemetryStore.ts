import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { TelemetryData, ChannelStats } from '@/types'
import { fetchTelemetry as apiFetchTelemetry, fetchStats as apiFetchStats } from '@/api/device'

const MAX_BUFFER = 200

export const useTelemetryStore = defineStore('telemetry', () => {
  const dataByDevice = ref<Map<string, TelemetryData[]>>(new Map())
  const statsByDevice = ref<Map<string, ChannelStats[]>>(new Map())

  function pushTelemetry(deviceId: string, data: TelemetryData) {
    if (!dataByDevice.value.has(deviceId)) {
      dataByDevice.value.set(deviceId, [])
    }
    const buf = dataByDevice.value.get(deviceId)!
    buf.push(data)
    if (buf.length > MAX_BUFFER) {
      buf.splice(0, buf.length - MAX_BUFFER)
    }
  }

  function updateStats(deviceId: string, stats: ChannelStats[]) {
    statsByDevice.value.set(deviceId, stats)
  }

  async function fetchTelemetry(deviceId: string) {
    try {
      const res = await apiFetchTelemetry(deviceId)
      if (res.data.success && res.data.data) {
        dataByDevice.value.set(deviceId, res.data.data)
      }
    } catch (e) {
      console.error('Failed to fetch telemetry', e)
    }
  }

  async function fetchStats(deviceId: string) {
    try {
      const res = await apiFetchStats(deviceId)
      if (res.data.success && res.data.data) {
        statsByDevice.value.set(deviceId, res.data.data)
      }
    } catch (e) {
      console.error('Failed to fetch stats', e)
    }
  }

  return {
    dataByDevice,
    statsByDevice,
    pushTelemetry,
    updateStats,
    fetchTelemetry,
    fetchStats,
  }
})
