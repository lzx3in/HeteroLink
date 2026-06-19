import { createPinia } from 'pinia'

const pinia = createPinia()
export default pinia

export { useDeviceStore } from './deviceStore'
export { useTelemetryStore } from './telemetryStore'
export { useAlarmStore } from './alarmStore'
export { useConfigStore } from './configStore'
export { useLogStore } from './logStore'
export { useAppStore } from './appStore'
