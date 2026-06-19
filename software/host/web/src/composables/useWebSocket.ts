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
        telemetryStore.pushTelemetry(msg.data.device_id, {
          timestamp: msg.data.timestamp,
          channels: msg.data.channels,
        })
        break
      case 'DeviceListChanged':
        deviceStore.updateFromWs(msg.data.devices)
        break
      case 'DeviceStatusChanged':
        // 设备状态变更会通过后续的 DeviceListChanged 更新列表
        logStore.addLog('info',
          `设备 ${msg.data.device_id} ${msg.data.connected ? (msg.data.online ? '上线' : '已连接') : '离线'}`)
        break
      case 'StatsUpdated':
        telemetryStore.updateStats(msg.data.device_id, msg.data.stats)
        break
      case 'AlarmTriggered': {
        const alarm = msg.data.alarm
        alarmStore.onAlarmTriggered(alarm.device_id, alarm)
        logStore.addLog('warn', `告警触发: ${alarm.device_id} CH${alarm.channel_id}`)
        break
      }
      case 'AlarmsChanged':
        alarmStore.updateAlarms(msg.data.device_id, msg.data.alarms)
        break
      case 'MqttStatusChanged':
        appStore.mqttConnected = msg.data.connected
        configStore.mqttConnected = msg.data.connected
        logStore.addLog(msg.data.connected ? 'info' : 'warn',
          msg.data.connected ? 'MQTT 已连接' : 'MQTT 已断开')
        break
      case 'CommandResponse':
        appStore.lastCommandResponse = {
          device_id: msg.data.device_id,
          response: msg.data.response,
        }
        logStore.addLog('info', `命令响应: ${msg.data.device_id}: ${msg.data.response}`)
        break
      case 'Log':
        logStore.addLog('info', msg.data.message)
        break
      case 'RecordingChanged':
        appStore.recording = msg.data.recording
        if (!msg.data.recording) {
          appStore.recordingDeviceId = null
        }
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
