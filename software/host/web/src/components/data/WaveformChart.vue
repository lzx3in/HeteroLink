<template>
  <div class="waveform-chart">
    <!-- 控制栏 -->
    <div class="chart-controls">
      <div class="channel-toggles">
        <button
          v-for="(ch, i) in channelList"
          :key="i"
          class="ch-btn"
          :class="{ active: channelVisible[i] }"
          :style="{ borderColor: channelColors[i], color: channelVisible[i] ? '#fff' : channelColors[i], background: channelVisible[i] ? channelColors[i] + '33' : 'transparent' }"
          @click="channelVisible[i] = !channelVisible[i]"
        >
          {{ ch.name }}
        </button>
      </div>
      <div class="chart-actions">
        <button class="action-btn" :class="{ paused: isPaused }" @click="isPaused = !isPaused">
          {{ isPaused ? '▶ 继续' : '⏸ 暂停' }}
        </button>
        <button class="action-btn" @click="clearData">清空</button>
        <button class="action-btn" @click="resetZoom">重置缩放</button>
      </div>
    </div>

    <!-- 图表 -->
    <div class="chart-wrapper">
      <v-chart
        ref="chartRef"
        :option="chartOption"
        :autoresize="true"
        style="width: 100%; height: 100%"
        @datazoom="onDataZoom"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, shallowRef } from 'vue'
import VChart from 'vue-echarts'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart } from 'echarts/charts'
import {
  GridComponent,
  TooltipComponent,
  LegendComponent,
  DataZoomComponent,
  MarkLineComponent,
} from 'echarts/components'
import { useDeviceStore } from '@/stores/deviceStore'
import { useTelemetryStore } from '@/stores/telemetryStore'

use([CanvasRenderer, LineChart, GridComponent, TooltipComponent, LegendComponent, DataZoomComponent, MarkLineComponent])

const deviceStore = useDeviceStore()
const telemetryStore = useTelemetryStore()

const chartRef = shallowRef()

const channelColors = ['#4fc3f7', '#81c784', '#ffb74d', '#e57373']
const channelList = [
  { name: 'CH1' },
  { name: 'CH2' },
  { name: 'CH3' },
  { name: 'CH4' },
]

// 通道显示/隐藏
const channelVisible = ref([true, true, true, true])

// 暂停/继续
const isPaused = ref(false)
const pausedData = ref<{ timestamps: string[]; channels: number[][] } | null>(null)

// 告警阈值（本地配置，用于在图表上显示阈值线）
const thresholds = ref<{ upper: number | null; lower: number | null }[]>([
  { upper: null, lower: null },
  { upper: null, lower: null },
  { upper: null, lower: null },
  { upper: null, lower: null },
])

// DataZoom state
const zoomStart = ref(0)
const zoomEnd = ref(100)

function clearData() {
  const id = deviceStore.selectedDeviceId
  if (id) {
    telemetryStore.dataByDevice.set(id, [])
  }
}

function resetZoom() {
  zoomStart.value = 0
  zoomEnd.value = 100
}

function onDataZoom(params: any) {
  if (params.batch) {
    zoomStart.value = params.batch[0].start
    zoomEnd.value = params.batch[0].end
  } else {
    zoomStart.value = params.start ?? zoomStart.value
    zoomEnd.value = params.end ?? zoomEnd.value
  }
}

// 监听暂停状态变化，保存当前数据快照
watch(isPaused, (paused) => {
  if (paused) {
    // 保存当前显示数据快照
    const id = deviceStore.selectedDeviceId
    if (id) {
      const data = telemetryStore.dataByDevice.get(id) || []
      const timestamps = data.map((d) => formatTimestamp(d.timestamp))
      const channels = [0, 1, 2, 3].map((i) => data.map((d) => d.channels[i] ?? null))
      pausedData.value = { timestamps, channels }
    }
  } else {
    pausedData.value = null
  }
})

function formatTimestamp(ts: number): string {
  try {
    const date = new Date(ts)
    return date.toLocaleTimeString('zh-CN', { hour12: false })
  } catch {
    return ''
  }
}

const chartOption = computed(() => {
  const id = deviceStore.selectedDeviceId
  const rawData = id ? (telemetryStore.dataByDevice.get(id) || []) : []

  let timestamps: string[]
  let channelData: (number | null)[][]

  if (isPaused.value && pausedData.value) {
    timestamps = pausedData.value.timestamps
    channelData = pausedData.value.channels
  } else {
    timestamps = rawData.map((d) => formatTimestamp(d.timestamp))
    channelData = [0, 1, 2, 3].map((i) => rawData.map((d) => d.channels[i] ?? null))
  }

  const series = channelList.map((ch, i) => {
    const markLineData: any[] = []
    const th = thresholds.value[i]
    if (th.upper !== null) {
      markLineData.push({
        yAxis: th.upper,
        label: { formatter: `${ch.name} 上限: ${th.upper}`, position: 'insideEndTop', fontSize: 10, color: '#e57373' },
        lineStyle: { color: '#e57373', type: 'dashed', width: 1 },
      })
    }
    if (th.lower !== null) {
      markLineData.push({
        yAxis: th.lower,
        label: { formatter: `${ch.name} 下限: ${th.lower}`, position: 'insideEndBottom', fontSize: 10, color: '#ffb74d' },
        lineStyle: { color: '#ffb74d', type: 'dashed', width: 1 },
      })
    }

    return {
      name: ch.name,
      type: 'line' as const,
      smooth: true,
      showSymbol: false,
      lineStyle: { width: 1.5 },
      itemStyle: { color: channelColors[i] },
      data: channelVisible.value[i] ? channelData[i] : [],
      markLine: markLineData.length > 0 ? {
        silent: true,
        symbol: 'none',
        data: markLineData,
      } : undefined,
    }
  })

  return {
    backgroundColor: 'transparent',
    tooltip: {
      trigger: 'axis' as const,
      confine: true,
      axisPointer: { type: 'cross' as const, crossStyle: { color: '#666' } },
    },
    legend: {
      show: false, // 用自定义 channel-toggles 替代
    },
    grid: { left: 55, right: 16, top: 12, bottom: 52 },
    xAxis: {
      type: 'category' as const,
      data: timestamps,
      axisLabel: { color: '#999', fontSize: 10, showMaxLabel: true },
      axisLine: { lineStyle: { color: '#444' } },
      splitLine: { show: false },
    },
    yAxis: {
      type: 'value' as const,
      axisLabel: { color: '#999', fontSize: 10 },
      splitLine: { lineStyle: { color: '#2a2a2a' } },
      scale: true,
    },
    dataZoom: [
      {
        type: 'inside' as const,
        start: zoomStart.value,
        end: zoomEnd.value,
        minValueSpan: 10,
      },
      {
        type: 'slider' as const,
        start: zoomStart.value,
        end: zoomEnd.value,
        height: 20,
        bottom: 4,
        borderColor: '#444',
        fillerColor: 'rgba(79,195,247,0.15)',
        handleStyle: { color: '#4fc3f7' },
        textStyle: { color: '#999', fontSize: 10 },
        dataBackground: {
          lineStyle: { color: '#555' },
          areaStyle: { color: 'rgba(79,195,247,0.05)' },
        },
      },
    ],
    series,
    animation: false,
  }
})
</script>

<style scoped lang="scss">
.waveform-chart {
  display: flex;
  flex-direction: column;
  width: 100%;
  height: 100%;
  min-height: 200px;
}

.chart-controls {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 8px;
  border-bottom: 1px solid var(--border-color);
  flex-shrink: 0;
  gap: 8px;
}

.channel-toggles {
  display: flex;
  gap: 4px;
}

.ch-btn {
  padding: 2px 10px;
  border: 1px solid;
  border-radius: 3px;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s;
  background: transparent;

  &.active {
    font-weight: 700;
  }

  &:not(.active) {
    opacity: 0.5;
  }

  &:hover {
    opacity: 1;
  }
}

.chart-actions {
  display: flex;
  gap: 4px;
}

.action-btn {
  padding: 2px 8px;
  border: 1px solid #555;
  border-radius: 3px;
  font-size: 11px;
  cursor: pointer;
  color: #ccc;
  background: transparent;
  transition: all 0.15s;

  &:hover {
    background: rgba(255, 255, 255, 0.08);
  }

  &.paused {
    border-color: #4fc3f7;
    color: #4fc3f7;
  }
}

.chart-wrapper {
  flex: 1;
  min-height: 0;
}
</style>
