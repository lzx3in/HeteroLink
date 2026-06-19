// 设备信息
export interface DeviceInfo {
  id: string
  connected: boolean
  alarm_active: boolean
  last_seen: string
}

// 遥测数据
export interface TelemetryData {
  device_id: string
  timestamp: string
  ch1: number
  ch2: number
  ch3: number
  ch4: number
}

// 通道统计
export interface ChannelStats {
  name: string
  current: number
  min: number
  max: number
  mean: number
}

// 告警记录
export interface AlarmRecord {
  device_id: string
  channel: number
  level: string
  value: number
  message: string
  timestamp: string
  acknowledged: boolean
}

// 告警配置
export interface AlarmConfig {
  channel: number
  warning_threshold: number
  critical_threshold: number
  enabled: boolean
}

// MQTT 配置
export interface MqttConfig {
  broker_url: string
  client_id: string
  username: string
  password: string
  topic_prefix: string
  keep_alive: number
}

// 应用配置
export interface AppConfig {
  recording_dir: string
  max_file_size: number
  display_buffer_size: number
  waveform_points: number
  mqtt?: MqttConfig
}

// WebSocket 消息
export type WsMessage =
  | { type: 'Telemetry'; data: TelemetryData }
  | { type: 'DeviceListChanged'; data: { devices: DeviceInfo[]; stats?: Record<string, ChannelStats[]> } }
  | { type: 'StatsUpdated'; data: { device_id: string; stats: ChannelStats[] } }
  | { type: 'AlarmTriggered'; data: { device_id: string; alarm: AlarmRecord } }
  | { type: 'AlarmsChanged'; data: { device_id: string } }
  | { type: 'MqttStatusChanged'; data: { connected: boolean; config: MqttConfig | null } }
  | { type: 'CommandResponse'; data: { device_id: string; message: string } }
  | { type: 'Log'; data: { level: string; message: string } }
  | { type: 'RecordingChanged'; data: { recording: boolean; device_id: string | null } }
  | { type: 'Error'; data: { message: string } }

// API 响应
export interface ApiResponse<T> {
  success: boolean
  data: T | null
  message: string | null
}
