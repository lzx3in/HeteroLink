<template>
  <div class="alarm-config-panel">
    <div v-if="!selectedId" class="empty-hint">请选择设备</div>
    <template v-else>
      <div class="channel-config" v-for="ch in 4" :key="ch">
        <div class="channel-header">
          <span class="channel-label" :class="'ch' + ch">CH{{ ch }}</span>
          <el-switch v-model="channelEnabled[ch - 1]" size="small" />
        </div>

        <template v-if="channelEnabled[ch - 1]">
          <el-form size="small" label-width="60px" class="ch-form">
            <el-form-item label="下限">
              <el-input-number v-model="lowerLimits[ch - 1]" :precision="2" :step="0.1" />
            </el-form-item>
            <el-form-item label="上限">
              <el-input-number v-model="upperLimits[ch - 1]" :precision="2" :step="0.1" />
            </el-form-item>
          </el-form>
        </template>
      </div>

      <div class="config-actions">
        <el-button type="primary" size="small" @click="handleSave" :loading="loading">
          保存告警配置
        </el-button>
      </div>
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { useDeviceStore } from '@/stores/deviceStore'
import { useAlarmStore } from '@/stores/alarmStore'
import type { AlarmConfigRequest } from '@/types'

const deviceStore = useDeviceStore()
const alarmStore = useAlarmStore()
const loading = ref(false)

const selectedId = computed(() => deviceStore.selectedDeviceId)

const channelEnabled = ref([true, true, true, true])
const lowerLimits = ref([0, 0, 0, 0])
const upperLimits = ref([100, 100, 100, 100])

watch(selectedId, async (id) => {
  if (id) {
    await alarmStore.fetchAlarms(id)
  }
}, { immediate: true })

async function handleSave() {
  if (!selectedId.value) return
  loading.value = true
  try {
    // 逐通道发送告警配置（后端每次接受单个 AlarmConfigRequest）
    for (let i = 0; i < 4; i++) {
      const config: AlarmConfigRequest = {
        channel_id: i + 1,
        lower_limit: lowerLimits.value[i],
        upper_limit: upperLimits.value[i],
        lower_enabled: channelEnabled.value[i],
        upper_enabled: channelEnabled.value[i],
        enabled: channelEnabled.value[i],
      }
      await alarmStore.configureAlarm(selectedId.value, config)
    }
  } finally {
    loading.value = false
  }
}
</script>

<style scoped lang="scss">
.alarm-config-panel {
  padding: 12px;
}

.empty-hint {
  text-align: center;
  color: var(--el-text-color-secondary);
  padding: 40px 0;
}

.channel-config {
  margin-bottom: 12px;
  padding: 8px;
  background: var(--bg-card);
  border-radius: 4px;
}

.channel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 6px;
}

.channel-label {
  font-weight: 600;
  font-size: 13px;

  &.ch1 { color: var(--ch1-color); }
  &.ch2 { color: var(--ch2-color); }
  &.ch3 { color: var(--ch3-color); }
  &.ch4 { color: var(--ch4-color); }
}

.ch-form {
  padding-left: 8px;
}

.config-actions {
  padding-top: 8px;
  text-align: center;
}
</style>
