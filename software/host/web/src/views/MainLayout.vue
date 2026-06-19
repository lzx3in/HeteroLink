<template>
  <div class="main-layout">
    <AppToolbar />
    <div class="layout-body">
      <!-- 左侧: 设备列表 -->
      <div class="panel-left">
        <DeviceListPanel />
      </div>

      <!-- 中央: 数据区 -->
      <div class="panel-center">
        <el-tabs v-model="centerTab" class="center-tabs">
          <el-tab-pane label="实时数据" name="data">
            <RealtimeDataTable />
          </el-tab-pane>
          <el-tab-pane label="波形图" name="waveform">
            <WaveformChart />
          </el-tab-pane>
          <el-tab-pane label="统计" name="stats">
            <StatsPanel />
          </el-tab-pane>
          <el-tab-pane label="告警" name="alarm">
            <AlarmPanel />
          </el-tab-pane>
          <el-tab-pane label="日志" name="log">
            <LogPanel />
          </el-tab-pane>
          <el-tab-pane label="设备控制" name="control">
            <DeviceControlPanel />
          </el-tab-pane>
        </el-tabs>
      </div>

      <!-- 右侧: 配置面板 -->
      <div class="panel-right">
        <el-tabs v-model="rightTab" class="right-tabs">
          <el-tab-pane label="MQTT" name="mqtt">
            <MqttConfigPanel />
          </el-tab-pane>
          <el-tab-pane label="告警配置" name="alarm-config">
            <AlarmConfigPanel />
          </el-tab-pane>
          <el-tab-pane label="通用" name="general">
            <GeneralConfigPanel />
          </el-tab-pane>
        </el-tabs>
      </div>
    </div>
    <StatusBar :ws-connected="wsConnected" />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import AppToolbar from '@/components/toolbar/AppToolbar.vue'
import StatusBar from '@/components/status/StatusBar.vue'
import DeviceListPanel from '@/components/device/DeviceListPanel.vue'
import RealtimeDataTable from '@/components/data/RealtimeDataTable.vue'
import WaveformChart from '@/components/data/WaveformChart.vue'
import StatsPanel from '@/components/data/StatsPanel.vue'
import AlarmPanel from '@/components/data/AlarmPanel.vue'
import LogPanel from '@/components/data/LogPanel.vue'
import DeviceControlPanel from '@/components/data/DeviceControlPanel.vue'
import MqttConfigPanel from '@/components/config/MqttConfigPanel.vue'
import AlarmConfigPanel from '@/components/config/AlarmConfigPanel.vue'
import GeneralConfigPanel from '@/components/config/GeneralConfigPanel.vue'
import { useWebSocket } from '@/composables/useWebSocket'
import { useDeviceStore } from '@/stores/deviceStore'
import { useConfigStore } from '@/stores/configStore'

const centerTab = ref('data')
const rightTab = ref('mqtt')

const { connected: wsConnected } = useWebSocket()

const deviceStore = useDeviceStore()
const configStore = useConfigStore()

onMounted(async () => {
  await deviceStore.fetchDevices()
  await configStore.fetchConfig()
})
</script>

<style scoped lang="scss">
.main-layout {
  display: flex;
  flex-direction: column;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  background: var(--bg-main);
}

.layout-body {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.panel-left {
  width: 280px;
  flex-shrink: 0;
  border-right: 1px solid var(--border-color);
  overflow: hidden;
}

.panel-center {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  display: flex;
  flex-direction: column;

  :deep(.center-tabs) {
    display: flex;
    flex-direction: column;
    height: 100%;

    .el-tabs__content {
      flex: 1;
      overflow: hidden;

      .el-tab-pane {
        height: 100%;
      }
    }
  }
}

.panel-right {
  width: 320px;
  flex-shrink: 0;
  border-left: 1px solid var(--border-color);
  overflow: auto;

  :deep(.right-tabs) {
    .el-tabs__content {
      .el-tab-pane {
        min-height: 300px;
      }
    }
  }
}
</style>
