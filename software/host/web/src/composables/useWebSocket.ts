import { ref, onMounted, onUnmounted } from 'vue'
import { useDeviceStore } from '@/stores/deviceStore'
import { useTelemetryStore } from '@/stores/telemetryStore'
import { useAlarmStore } from '@/stores/alarmStore'
import { useConfigStore } from '@/stores/configStore'
import { useLogStore } from '@/stores/logStore'
import { useAppStore } from '@/stores/appStore'
import type { WsMessage } from '@/types'

export function useWebSocket() {
  const connected = ref(false)
  let ws: WebSocket | null = null
  let reconnectAttempts = 0
  const maxReconnectAttempts = 10
  let reconnectTimer: ReturnType<typeof setTimeout> | null = null

  const deviceStore = useDeviceStore()
  const telemetryStore = useTelemetryStore()
  const alarmStore = useAlarmStore()
  const configStore = useConfigStore()
  const logStore = useLogStore()
  const appStore = useAppStore()

  function getWsUrl(): string {
    const loc = window.location
    const proto = loc.protocol === 'https:' ? 'wss:' : 'ws:'
    return `${proto}//${loc.host}/ws`
  }

  function handleMessage(msg: WsMessage) {
    switch (msg.type) {
      case 'Telemetry':
        telemetryStore.pushTelemetry(msg.data.device_id, msg.data)
        break
      case 'DeviceListChanged':
        deviceStore.updateFromWs(msg.data.devices)
        if (msg.data.stats) {
          for (const [id, stats] of Object.entries(msg.data.stats)) {
            telemetryStore.updateStats(id, stats as any)
          }
        }
        break
      case 'StatsUpdated':
        telemetryStore.updateStats(msg.data.device_id, msg.data.stats)
        break
      case 'AlarmTriggered':
        alarmStore.onAlarmTriggered(msg.data.device_id, msg.data.alarm)
        logStore.addLog('warn', `告警触发: ${msg.data.device_id} CH${msg.data.alarm.channel}`)
        break
      case 'AlarmsChanged':
        if (msg.data.device_id) {
          alarmStore.fetchAlarms(msg.data.device_id)
        }
        break
      case 'MqttStatusChanged':
        appStore.mqttConnected = msg.data.connected
        configStore.mqttConfig = msg.data.config || configStore.mqttConfig
        logStore.addLog(msg.data.connected ? 'info' : 'warn',
          msg.data.connected ? 'MQTT 已连接' : 'MQTT 已断开')
        break
      case 'CommandResponse':
        appStore.lastCommandResponse = msg.data
        logStore.addLog('info', `命令响应: ${msg.data.device_id}: ${msg.data.message}`)
        break
      case 'Log':
        logStore.addLog(msg.data.level || 'info', msg.data.message)
        break
      case 'RecordingChanged':
        appStore.recording = msg.data.recording
        appStore.recordingDeviceId = msg.data.device_id || null
        logStore.addLog('info', msg.data.recording ? '录制已开始' : '录制已停止')
        break
      case 'Error':
        logStore.addLog('error', msg.data.message)
        break
    }
  }

  function connect() {
    if (ws?.readyState === WebSocket.OPEN) return

    try {
      ws = new WebSocket(getWsUrl())

      ws.onopen = () => {
        connected.value = true
        reconnectAttempts = 0
        logStore.addLog('info', 'WebSocket 已连接')
      }

      ws.onmessage = (event) => {
        try {
          const msg: WsMessage = JSON.parse(event.data)
          handleMessage(msg)
        } catch (e) {
          console.error('[WS] Failed to parse message:', e)
        }
      }

      ws.onclose = () => {
        connected.value = false
        ws = null
        scheduleReconnect()
      }

      ws.onerror = (e) => {
        console.error('[WS] Error:', e)
        ws?.close()
      }
    } catch (e) {
      console.error('[WS] Connection failed:', e)
      scheduleReconnect()
    }
  }

  function scheduleReconnect() {
    if (reconnectAttempts >= maxReconnectAttempts) {
      logStore.addLog('error', 'WebSocket 重连次数已达上限')
      return
    }
    const delay = Math.min(1000 * Math.pow(2, reconnectAttempts), 30000)
    reconnectAttempts++
    reconnectTimer = setTimeout(() => connect(), delay)
  }

  function disconnect() {
    if (reconnectTimer) {
      clearTimeout(reconnectTimer)
      reconnectTimer = null
    }
    reconnectAttempts = maxReconnectAttempts // prevent reconnect
    ws?.close()
    ws = null
    connected.value = false
  }

  onMounted(() => connect())
  onUnmounted(() => disconnect())

  return { connected, connect, disconnect }
}
