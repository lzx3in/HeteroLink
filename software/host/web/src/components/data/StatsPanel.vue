<template>
  <div class="stats-panel">
    <div v-if="!selectedId" class="empty-hint">请选择设备</div>
    <template v-else>
      <div class="stats-grid">
        <div v-for="stat in stats" :key="stat.channel_id" class="stat-card" :class="'ch' + stat.channel_id">
          <div class="stat-header">
            <span class="stat-label">CH{{ stat.channel_id }}</span>
          </div>
          <div class="stat-values">
            <div class="stat-row">
              <span class="stat-key">平均</span>
              <span class="stat-val current">{{ fmt(stat.avg) }}</span>
            </div>
            <div class="stat-row">
              <span class="stat-key">最小</span>
              <span class="stat-val">{{ fmt(stat.min) }}</span>
            </div>
            <div class="stat-row">
              <span class="stat-key">最大</span>
              <span class="stat-val">{{ fmt(stat.max) }}</span>
            </div>
            <div class="stat-row">
              <span class="stat-key">RMS</span>
              <span class="stat-val">{{ fmt(stat.rms) }}</span>
            </div>
            <div class="stat-row">
              <span class="stat-key">采样数</span>
              <span class="stat-val">{{ stat.sample_count }}</span>
            </div>
          </div>
        </div>
      </div>
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useDeviceStore } from '@/stores/deviceStore'
import { useTelemetryStore } from '@/stores/telemetryStore'

const deviceStore = useDeviceStore()
const telemetryStore = useTelemetryStore()

const selectedId = computed(() => deviceStore.selectedDeviceId)

const stats = computed(() => {
  const id = selectedId.value
  if (!id) return []
  return telemetryStore.statsByDevice.get(id) || []
})

function fmt(v: number | undefined): string {
  if (v === undefined || v === null) return '--'
  return v.toFixed(2)
}
</script>

<style scoped lang="scss">
.stats-panel {
  height: 100%;
  overflow: auto;
  padding: 12px;
}

.empty-hint {
  text-align: center;
  color: var(--el-text-color-secondary);
  padding: 40px 0;
}

.stats-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.stat-card {
  padding: 10px;
  border-radius: 6px;
  background: var(--bg-card);
  border-left: 3px solid var(--el-border-color);

  &.ch1 { border-left-color: var(--ch1-color); }
  &.ch2 { border-left-color: var(--ch2-color); }
  &.ch3 { border-left-color: var(--ch3-color); }
  &.ch4 { border-left-color: var(--ch4-color); }
}

.stat-header {
  margin-bottom: 6px;
}

.stat-label {
  font-weight: 600;
  font-size: 13px;
}

.stat-values {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.stat-row {
  display: flex;
  justify-content: space-between;
  font-size: 12px;
}

.stat-key {
  color: var(--el-text-color-secondary);
}

.stat-val {
  font-family: 'Courier New', monospace;
  font-weight: 500;

  &.current {
    font-weight: 700;
    color: var(--el-color-primary);
  }
}
</style>
