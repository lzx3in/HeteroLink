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
const channelKeys = ['ch1', 'ch2', 'ch3', 'ch4'] as const

const chartOption = computed(() => {
  const id = deviceStore.selectedDeviceId
  const data = id ? (telemetryStore.dataByDevice.get(id) || []) : []
  const timestamps = data.map((d) => {
    try { return new Date(d.timestamp).toLocaleTimeString('zh-CN', { hour12: false }) } catch { return '' }
  })

  const series = channelKeys.map((key, i) => ({
    name: channelNames[i],
    type: 'line' as const,
    smooth: true,
    showSymbol: false,
    lineStyle: { width: 1.5 },
    itemStyle: { color: channelColors[i] },
    data: data.map((d) => d[key] ?? null),
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
