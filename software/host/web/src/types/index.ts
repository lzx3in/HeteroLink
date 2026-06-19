// --- 设备信息 (对齐 DeviceInfoDto) ---
export interface DeviceInfo {
  id: string
  name: string
  connected: boolean
  online: boolean
  connection_type: string
  port: string
  last_seen: number
}

// --- 遥测数据 (对齐 TelemetryDto) ---
export interface TelemetryDto {
  timestamp: number
  channels: number[]
}

// --- 通道统计 (对齐 ChannelStatsDto) ---
export interface ChannelStatsDto {
  channel_id: number
  min: number
  max: number
  avg: number
  rms: number
  sample_count: number
}

// --- 告警记录 (对齐 AlarmRecordDto) ---
export interface AlarmRecord {
  device_id: string
  channel_id: number
  level: string
  value: number
  message: string
  timestamp: string
  acknowledged: boolean
}

// --- 告警配置请求 (对齐 AlarmConfigRequest) ---
export interface AlarmConfigRequest {
  channel_id: number
  lower_limit: number
  upper_limit: number
  lower_enabled: boolean
  upper_enabled: boolean
  enabled: boolean
}

// --- MQTT 连接请求 (对齐 MqttConnectRequest) ---
export interface MqttConnectRequest {
  broker_host: string
  broker_port: number
  username?: string
  password?: string
  client_id?: string
  use_tls?: boolean
}

// --- MQTT 前端表单配置 (UI 使用) ---
export interface MqttFormConfig {
  broker_url: string
  client_id: string
  username: string
  password: string
  topic_prefix: string
  keep_alive: number
}

// --- 应用配置 ---
export interface AppConfig {
  recording_dir: string
  max_file_size: number
  display_buffer_size: number
  waveform_points: number
  mqtt?: MqttFormConfig
}

// --- WebSocket 消息 (对齐 broadcast::WsMessage) ---
export type WsMessage =
  | { type: 'Telemetry'; data: { device_id: string; timestamp: number; channels: number[] } }
  | { type: 'DeviceListChanged'; data: { devices: DeviceInfo[]; connected_count: number; online_count: number } }
  | { type: 'DeviceStatusChanged'; data: { device_id: string; connected: boolean; online: boolean } }
  | { type: 'StatsUpdated'; data: { device_id: string; stats: ChannelStatsDto[] } }
  | { type: 'AlarmTriggered'; data: { alarm: AlarmRecord } }
  | { type: 'AlarmsChanged'; data: { device_id: string; alarms: AlarmRecord[] } }
  | { type: 'MqttStatusChanged'; data: { connected: boolean } }
  | { type: 'CommandResponse'; data: { device_id: string; response: string } }
  | { type: 'Log'; data: { message: string; timestamp: string } }
  | { type: 'RecordingChanged'; data: { recording: boolean } }
  | { type: 'Error'; data: { message: string } }

// --- API 响应 (对齐 ApiResponse<T>) ---
export interface ApiResponse<T> {
  success: boolean
  data: T | null
  message: string | null
}
