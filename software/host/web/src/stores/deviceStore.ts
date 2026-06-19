import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { DeviceInfo } from '@/types'
import { fetchDevices as apiFetchDevices, addDevice as apiAddDevice, removeDevice as apiRemoveDevice, disconnectDevice as apiDisconnectDevice } from '@/api/device'

export const useDeviceStore = defineStore('device', () => {
  const devices = ref<DeviceInfo[]>([])
  const selectedDeviceId = ref<string | null>(null)

  const connectedCount = computed(() => devices.value.filter(d => d.connected).length)

  async function fetchDevices() {
    try {
      const res = await apiFetchDevices()
      if (res.data.success && res.data.data) {
        devices.value = res.data.data
      }
    } catch (e) {
      console.error('Failed to fetch devices', e)
    }
  }

  async function addDevice(id: string) {
    try {
      await apiAddDevice(id)
      await fetchDevices()
    } catch (e) {
      console.error('Failed to add device', e)
    }
  }

  async function removeDevice(deviceId: string) {
    try {
      await apiRemoveDevice(deviceId)
      if (selectedDeviceId.value === deviceId) {
        selectedDeviceId.value = null
      }
      await fetchDevices()
    } catch (e) {
      console.error('Failed to remove device', e)
    }
  }

  async function disconnectDevice(deviceId: string) {
    try {
      await apiDisconnectDevice(deviceId)
      await fetchDevices()
    } catch (e) {
      console.error('Failed to disconnect device', e)
    }
  }

  function selectDevice(deviceId: string) {
    selectedDeviceId.value = deviceId
  }

  function updateFromWs(deviceList: DeviceInfo[]) {
    devices.value = deviceList
  }

  return {
    devices,
    selectedDeviceId,
    connectedCount,
    fetchDevices,
    addDevice,
    removeDevice,
    disconnectDevice,
    selectDevice,
    updateFromWs,
  }
})
