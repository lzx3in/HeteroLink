<template>
  <div class="realtime-data-table">
    <el-table :data="tableData" stripe size="small" height="100%" :header-cell-style="headerStyle">
      <el-table-column prop="timestamp" label="时间" width="100">
        <template #default="{ row }">
          <span class="mono-data">{{ formatTime(row.timestamp) }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CH1" width="100">
        <template #default="{ row }">
          <span class="mono-data ch1">{{ formatValue(row.ch1) }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CH2" width="100">
        <template #default="{ row }">
          <span class="mono-data ch2">{{ formatValue(row.ch2) }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CH3" width="100">
        <template #default="{ row }">
          <span class="mono-data ch3">{{ formatValue(row.ch3) }}</span>
        </template>
      </el-table-column>
      <el-table-column label="CH4" width="100">
        <template #default="{ row }">
          <span class="mono-data ch4">{{ formatValue(row.ch4) }}</span>
        </template>
      </el-table-column>
    </el-table>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useDeviceStore } from '@/stores/deviceStore'
import { useTelemetryStore } from '@/stores/telemetryStore'

const deviceStore = useDeviceStore()
const telemetryStore = useTelemetryStore()

const headerStyle = { background: 'var(--bg-header)', fontSize: '12px' }

const tableData = computed(() => {
  const id = deviceStore.selectedDeviceId
  if (!id) return []
  return [...(telemetryStore.dataByDevice.get(id) || [])].reverse().slice(0, 50)
})

function formatTime(ts: string): string {
  try {
    return new Date(ts).toLocaleTimeString('zh-CN', { hour12: false })
  } catch {
    return ts
  }
}

function formatValue(v: number | undefined): string {
  if (v === undefined || v === null) return '--'
  return v.toFixed(2)
}
</script>

<style scoped lang="scss">
.realtime-data-table {
  height: 100%;
  overflow: hidden;
}

.mono-data {
  font-family: 'Courier New', monospace;
  font-size: 12px;
}

.ch1 { color: var(--ch1-color); }
.ch2 { color: var(--ch2-color); }
.ch3 { color: var(--ch3-color); }
.ch4 { color: var(--ch4-color); }
</style>
