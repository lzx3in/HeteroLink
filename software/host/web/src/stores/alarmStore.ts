import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { AlarmRecord, AlarmConfigRequest } from '@/types'
import { fetchAlarms as apiFetchAlarms, configureAlarm as apiConfigureAlarm, acknowledgeAlarm as apiAcknowledgeAlarm, clearAlarms as apiClearAlarms } from '@/api/alarm'

export const useAlarmStore = defineStore('alarm', () => {
  const alarmsByDevice = ref<Map<string, AlarmRecord[]>>(new Map())

  const allAlarms = computed(() => {
    const all: AlarmRecord[] = []
    alarmsByDevice.value.forEach(records => all.push(...records))
    return all.sort((a, b) => (b.timestamp || '').localeCompare(a.timestamp || ''))
  })

  async function fetchAlarms(deviceId: string) {
    try {
      const res = await apiFetchAlarms(deviceId)
      if (res.data.success && res.data.data) {
        alarmsByDevice.value.set(deviceId, res.data.data)
      }
    } catch (e) {
      console.error('Failed to fetch alarms', e)
    }
  }

  async function configureAlarm(deviceId: string, config: AlarmConfigRequest) {
    try {
      await apiConfigureAlarm(deviceId, config)
    } catch (e) {
      console.error('Failed to configure alarm', e)
    }
  }

  async function acknowledgeAlarm(deviceId: string, channelId: number) {
    try {
      await apiAcknowledgeAlarm(deviceId, channelId)
      await fetchAlarms(deviceId)
    } catch (e) {
      console.error('Failed to acknowledge alarm', e)
    }
  }

  async function clearAlarms(deviceId: string) {
    try {
      await apiClearAlarms(deviceId)
      alarmsByDevice.value.set(deviceId, [])
    } catch (e) {
      console.error('Failed to clear alarms', e)
    }
  }

  function onAlarmTriggered(deviceId: string, alarm: AlarmRecord) {
    if (!alarmsByDevice.value.has(deviceId)) {
      alarmsByDevice.value.set(deviceId, [])
    }
    alarmsByDevice.value.get(deviceId)!.push(alarm)
  }

  function updateAlarms(deviceId: string, alarms: AlarmRecord[]) {
    alarmsByDevice.value.set(deviceId, alarms)
  }

  return {
    alarmsByDevice,
    allAlarms,
    fetchAlarms,
    configureAlarm,
    acknowledgeAlarm,
    clearAlarms,
    onAlarmTriggered,
    updateAlarms,
  }
})
