import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { AppConfig, MqttFormConfig, MqttConnectRequest } from '@/types'
import { fetchConfig as apiFetchConfig, saveConfig as apiSaveConfig, loadConfig as apiLoadConfig, resetConfig as apiResetConfig } from '@/api/config'
import { connectMqtt as apiConnectMqtt, disconnectMqtt as apiDisconnectMqtt } from '@/api/mqtt'

export const useConfigStore = defineStore('config', () => {
  const config = ref<AppConfig | null>(null)
  const mqttConfig = ref<MqttFormConfig>({
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

  /** 将 MqttFormConfig 的 broker_url 解析为 MqttConnectRequest 并发送 */
  async function connectMqtt(form: MqttFormConfig) {
    try {
      // 解析 broker_url -> broker_host + broker_port
      let host = form.broker_url
      let port = 1883
      try {
        const url = new URL(form.broker_url)
        host = url.hostname
        port = url.port ? parseInt(url.port) : (url.protocol === 'mqtts:' ? 8883 : 1883)
      } catch {
        // 如果不是合法 URL，尝试 host:port 格式
        const parts = form.broker_url.split(':')
        if (parts.length >= 2) {
          const lastPart = parts[parts.length - 1]
          const parsed = parseInt(lastPart)
          if (!isNaN(parsed)) {
            port = parsed
            host = parts.slice(0, -1).join(':')
          }
        }
      }

      const req: MqttConnectRequest = {
        broker_host: host,
        broker_port: port,
        username: form.username || undefined,
        password: form.password || undefined,
        client_id: form.client_id || undefined,
        use_tls: form.broker_url.startsWith('mqtts://'),
      }

      await apiConnectMqtt(req)
      mqttConfig.value = form
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
