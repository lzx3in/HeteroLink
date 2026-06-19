<template>
  <div class="status-bar">
    <div class="status-left">
      <span class="status-item" :class="{ connected: wsConnected }">
        <el-icon><Connection /></el-icon>
        WS {{ wsConnected ? '已连接' : '断开' }}
      </span>
      <el-divider direction="vertical" />
      <span class="status-item">
        设备 {{ deviceStore.connectedCount }}/{{ deviceStore.devices.length }} 在线
      </span>
    </div>
    <div class="status-right">
      <span v-if="appStore.statusMessage" class="status-message">
        {{ appStore.statusMessage }}
      </span>
      <span class="status-time">{{ currentTime }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { Connection } from '@element-plus/icons-vue'
import { useDeviceStore } from '@/stores/deviceStore'
import { useAppStore } from '@/stores/appStore'

defineProps<{ wsConnected: boolean }>()

const deviceStore = useDeviceStore()
const appStore = useAppStore()

const currentTime = ref('')
let timer: ReturnType<typeof setInterval>

function updateTime() {
  currentTime.value = new Date().toLocaleTimeString('zh-CN', { hour12: false })
}

onMounted(() => {
  updateTime()
  timer = setInterval(updateTime, 1000)
})

onUnmounted(() => clearInterval(timer))
</script>

<style scoped lang="scss">
.status-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 28px;
  padding: 0 12px;
  background: var(--bg-statusbar, #007acc);
  color: #fff;
  font-size: 12px;
}

.status-left,
.status-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 4px;
  opacity: 0.85;

  &.connected {
    opacity: 1;
  }
}

.status-message {
  opacity: 0.9;
  max-width: 300px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.status-time {
  font-family: 'Courier New', monospace;
  opacity: 0.9;
}
</style>
