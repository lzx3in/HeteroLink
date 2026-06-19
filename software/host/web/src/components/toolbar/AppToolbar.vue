<template>
  <div class="app-toolbar">
    <div class="toolbar-left">
      <span class="app-title">HeteroLink</span>
      <el-divider direction="vertical" />
      <el-tag :type="mqttConnected ? 'success' : 'danger'" size="small">
        MQTT {{ mqttConnected ? '已连接' : '未连接' }}
      </el-tag>
      <el-tag v-if="recording" type="warning" size="small" effect="dark">
        <el-icon class="recording-dot"><VideoCamera /></el-icon>
        录制中 ({{ recordingDeviceId }})
      </el-tag>
    </div>
    <div class="toolbar-center">
      <el-button-group>
        <el-button
          :type="recording ? 'danger' : 'success'"
          size="small"
          @click="toggleRecording"
          :disabled="!canRecord"
        >
          <el-icon><VideoCamera v-if="!recording" /><VideoPause v-else /></el-icon>
          {{ recording ? '停止录制' : '开始录制' }}
        </el-button>
        <el-button size="small" @click="handleExportCsv" :disabled="!selectedDevice">
          <el-icon><Download /></el-icon> CSV
        </el-button>
        <el-button size="small" @click="handleExportJson" :disabled="!selectedDevice">
          <el-icon><Download /></el-icon> JSON
        </el-button>
      </el-button-group>
    </div>
    <div class="toolbar-right">
      <span v-if="statusMessage" class="status-msg">{{ statusMessage }}</span>
      <span class="device-count">{{ deviceCount }} 设备</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { VideoCamera, VideoPause, Download } from '@element-plus/icons-vue'
import { useAppStore } from '@/stores/appStore'
import { useDeviceStore } from '@/stores/deviceStore'

const appStore = useAppStore()
const deviceStore = useDeviceStore()

const mqttConnected = computed(() => appStore.mqttConnected)
const recording = computed(() => appStore.recording)
const recordingDeviceId = computed(() => appStore.recordingDeviceId)
const statusMessage = computed(() => appStore.statusMessage)
const selectedDevice = computed(() => deviceStore.selectedDeviceId)
const deviceCount = computed(() => deviceStore.devices.length)
const canRecord = computed(() => !!selectedDevice.value && mqttConnected.value)

function toggleRecording() {
  if (recording.value) {
    appStore.stopRecording()
  } else if (selectedDevice.value) {
    appStore.startRecording(selectedDevice.value)
  }
}

function handleExportCsv() {
  if (selectedDevice.value) appStore.exportCsv(selectedDevice.value)
}

function handleExportJson() {
  if (selectedDevice.value) appStore.exportJson(selectedDevice.value)
}
</script>

<style scoped lang="scss">
.app-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 42px;
  padding: 0 12px;
  background: var(--bg-toolbar);
  border-bottom: 1px solid var(--border-color);
}

.toolbar-left,
.toolbar-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

.toolbar-center {
  display: flex;
  align-items: center;
}

.app-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--el-color-primary);
  letter-spacing: 1px;
}

.recording-dot {
  margin-right: 4px;
  animation: blink 1s infinite;
}

@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}

.status-msg {
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.device-count {
  font-size: 12px;
  color: var(--el-text-color-regular);
}
</style>
