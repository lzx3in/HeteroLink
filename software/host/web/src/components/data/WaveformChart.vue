<template>
  <div class="waveform-chart" ref="chartContainer">
    <v-chart
      ref="chartRef"
      :option="chartOption"
      :autoresize="true"
      style="width: 100%; height: 100%"
    />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import VChart from 'vue-echarts'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart } from 'echarts/charts'
import {
  GridComponent,
  TooltipComponent,
  LegendComponent,
  DataZoomComponent,
} from 'echarts/components'
import { useDeviceStore } from '@/stores/deviceStore'
import { useTelemetryStore } from '@/stores/telemetryStore'

use([CanvasRenderer, LineChart, GridComponent, TooltipComponent, LegendComponent, DataZoomComponent])

const deviceStore = useDeviceStore()
const telemetryStore = useTelemetryStore()

const channelColors = ['#4fc3f7', '#81c784', '#ffb74d', '#e57373']
const channelNames = ['CH1', 'CH2', 'CH3', 'CH4']

const chartOption = computed(() => {
  const id = deviceStore.selectedDeviceId
  const data = id ? (telemetryStore.dataByDevice.get(id) || []) : []
  const timestamps = data.map((d) => {
    // timestamp 是 u32 毫秒时间戳，直接格式化为时间
    try {
      const date = new Date(d.timestamp)
      return date.toLocaleTimeString('zh-CN', { hour12: false })
    } catch { return '' }
  })

  const series = channelNames.map((name, i) => ({
    name,
    type: 'line' as const,
    smooth: true,
    showSymbol: false,
    lineStyle: { width: 1.5 },
    itemStyle: { color: channelColors[i] },
    data: data.map((d) => d.channels[i] ?? null),
  }))

  return {
    backgroundColor: 'transparent',
    tooltip: { trigger: 'axis' as const },
    legend: {
      data: channelNames,
      textStyle: { color: '#ccc', fontSize: 11 },
      top: 4,
      right: 8,
    },
    grid: { left: 50, right: 16, top: 36, bottom: 32 },
    xAxis: {
      type: 'category' as const,
      data: timestamps,
      axisLabel: { color: '#999', fontSize: 10 },
      axisLine: { lineStyle: { color: '#444' } },
    },
    yAxis: {
      type: 'value' as const,
      axisLabel: { color: '#999', fontSize: 10 },
      splitLine: { lineStyle: { color: '#333' } },
    },
    series,
    animation: false,
  }
})
</script>

<style scoped lang="scss">
.waveform-chart {
  width: 100%;
  height: 100%;
  min-height: 200px;
}
</style>
