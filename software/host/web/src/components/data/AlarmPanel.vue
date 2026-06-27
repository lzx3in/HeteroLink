<template>
  <div class="alarm-panel">
    <div v-if="!selectedId" class="empty-hint">请选择设备</div>
    <template v-else>
      <div class="alarm-actions">
        <el-button size="small" type="danger" @click="handleClear" :disabled="alarms.length === 0">
          清除全部
        </el-button>
      </div>
      <el-scrollbar class="alarm-list">
        <div v-if="alarms.length === 0" class="empty-hint">暂无告警</div>
        <div v-for="(alarm, idx) in alarms" :key="idx" class="alarm-item" :class="levelClass(alarm)">
          <div class="alarm-bar"></div>
          <div class="alarm-content">
            <div class="alarm-header">
              <el-tag :type="tagType(alarm)" size="small" effect="dark">
                {{ alarm.level }}
              </el-tag>
              <span class="alarm-channel">CH{{ alarm.channel_id }}</span>
              <span class="alarm-value mono-data">{{ alarm.value?.toFixed(2) }}</span>
            </div>
            <div class="alarm-footer">
              <span class="alarm-time">{{ formatTime(alarm.timestamp) }}</span>
              <el-button
                v-if="!alarm.acknowledged"
                size="small"
                text
                type="primary"
                @click="handleAck(alarm)"
              >
                确认
              </el-button>
              <el-tag v-else size="small" type="info">已确认</el-tag>
            </div>
          </div>
        </div>
      </el-scrollbar>
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useDeviceStore } from '@/stores/deviceStore'
import { useAlarmStore } from '@/stores/alarmStore'
import type { AlarmRecord } from '@/types'

const deviceStore = useDeviceStore()
const alarmStore = useAlarmStore()

const selectedId = computed(() => deviceStore.selectedDeviceId)

const alarms = computed(() => {
  const id = selectedId.value
  if (!id) return []
  return alarmStore.alarmsByDevice.get(id) || []
})

function levelClass(alarm: AlarmRecord): string {
  return `level-${alarm.level?.toLowerCase() || 'info'}`
}

function tagType(alarm: AlarmRecord): 'success' | 'warning' | 'danger' | 'info' | undefined {
  switch (alarm.level?.toLowerCase()) {
    case 'critical': return 'danger'
    case 'warning': return 'warning'
    case 'info': return 'info'
    default: return undefined
  }
}

function formatTime(ts: string): string {
  try {
    return new Date(ts).toLocaleString('zh-CN', { hour12: false })
  } catch {
    return ts
  }
}

function handleAck(alarm: AlarmRecord) {
  if (selectedId.value) {
    alarmStore.acknowledgeAlarm(selectedId.value, alarm.channel_id)
  }
}

function handleClear() {
  if (selectedId.value) {
    alarmStore.clearAlarms(selectedId.value)
  }
}
</script>

<style scoped lang="scss">
.alarm-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.alarm-actions {
  padding: 8px 12px;
  border-bottom: 1px solid var(--border-color);
}

.alarm-list {
  flex: 1;
}

.empty-hint {
  text-align: center;
  color: var(--el-text-color-secondary);
  padding: 40px 0;
}

.alarm-item {
  display: flex;
  padding: 8px 12px;
  border-bottom: 1px solid var(--border-color-lighter);

  &.level-critical .alarm-bar { background: var(--alarm-critical); }
  &.level-warning .alarm-bar { background: var(--alarm-warning); }
  &.level-info .alarm-bar { background: var(--alarm-info); }
}

.alarm-bar {
  width: 3px;
  border-radius: 2px;
  margin-right: 10px;
  flex-shrink: 0;
}

.alarm-content {
  flex: 1;
  min-width: 0;
}

.alarm-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 4px;
}

.alarm-channel {
  font-size: 12px;
  font-weight: 600;
}

.alarm-value {
  font-size: 12px;
}

.alarm-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.alarm-time {
  font-size: 11px;
  color: var(--el-text-color-secondary);
}
</style>
