import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { AppConfig, MqttConfig } from '@/types'
import { fetchConfig as apiFetchConfig, saveConfig as apiSaveConfig, loadConfig as apiLoadConfig, resetConfig as apiResetConfig } from '@/api/config'
import { connectMqtt as apiConnectMqtt, disconnectMqtt as apiDisconnectMqtt } from '@/api/mqtt'

export const useConfigStore = defineStore('config', () => {
  const config = ref<AppConfig | null>(null)
  const mqttConfig = ref<MqttConfig>({
    broker_url: 'mqtt://localhost:1883',
    client_id: 'heterolink-host',
    username: '',
    password: '',
    topic_prefix: 'heterolink',
    keep_alive: 60,
  })
  const mqttConnected = ref(false)

  async function fetchConfig() {
    try {
      const res = await apiFetchConfig()
      if (res.data.success && res.data.data) {
        config.value = res.data.data
        if (res.data.data.mqtt) {
          mqttConfig.value = { ...mqttConfig.value, ...res.data.data.mqtt }
        }
      }
    } catch (e) {
      console.error('Failed to fetch config', e)
    }
  }

  async function saveConfig(cfg: AppConfig) {
    try {
      await apiSaveConfig(cfg)
      config.value = cfg
    } catch (e) {
      console.error('Failed to save config', e)
    }
  }

  async function loadConfig() {
    try {
      const res = await apiLoadConfig()
      if (res.data.success && res.data.data) {
        config.value = res.data.data
        if (res.data.data.mqtt) {
          mqttConfig.value = { ...mqttConfig.value, ...res.data.data.mqtt }
        }
      }
    } catch (e) {
      console.error('Failed to load config', e)
    }
  }

  async function resetConfig() {
    try {
      const res = await apiResetConfig()
      if (res.data.success && res.data.data) {
        config.value = res.data.data
        if (res.data.data.mqtt) {
          mqttConfig.value = { ...mqttConfig.value, ...res.data.data.mqtt }
        }
      }
    } catch (e) {
      console.error('Failed to reset config', e)
    }
  }

  async function connectMqtt(mqtt: MqttConfig) {
    try {
      await apiConnectMqtt(mqtt)
      mqttConfig.value = mqtt
    } catch (e) {
      console.error('Failed to connect MQTT', e)
    }
  }

  async function disconnectMqtt() {
    try {
      await apiDisconnectMqtt()
    } catch (e) {
      console.error('Failed to disconnect MQTT', e)
    }
  }

  return {
    config,
    mqttConfig,
    mqttConnected,
    fetchConfig,
    saveConfig,
    loadConfig,
    resetConfig,
    connectMqtt,
    disconnectMqtt,
  }
})
