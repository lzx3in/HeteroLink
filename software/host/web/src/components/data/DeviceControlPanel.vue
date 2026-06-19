<template>
  <div class="device-control-panel">
    <div v-if="!selectedId" class="empty-hint">请选择设备</div>
    <template v-else>
      <el-form label-width="80px" size="small">
        <el-form-item label="设备 ID">
          <span class="mono-data">{{ selectedId }}</span>
        </el-form-item>

        <el-divider>采样控制</el-divider>

        <el-form-item label="采样率">
          <el-input-number v-model="sampleRate" :min="1" :max="10000" :step="100" />
          <span class="unit">Hz</span>
        </el-form-item>

        <el-form-item label="通道">
          <el-checkbox-group v-model="selectedChannels">
            <el-checkbox :value="1">CH1</el-checkbox>
            <el-checkbox :value="2">CH2</el-checkbox>
            <el-checkbox :value="3">CH3</el-checkbox>
            <el-checkbox :value="4">CH4</el-checkbox>
          </el-checkbox-group>
        </el-form-item>

        <el-form-item>
          <el-button-group>
            <el-button type="success" @click="handleStart">
              <el-icon><CaretRight /></el-icon> 开始采样
            </el-button>
            <el-button type="danger" @click="handleStop">
              <el-icon><VideoPause /></el-icon> 停止采样
            </el-button>
          </el-button-group>
        </el-form-item>

        <el-divider>GPIO 控制</el-divider>

        <el-form-item label="GPIO 引脚">
          <el-input-number v-model="gpioPin" :min="0" :max="39" />
        </el-form-item>

        <el-form-item>
          <el-button-group>
            <el-button type="primary" @click="handleGpio(true)">HIGH</el-button>
            <el-button @click="handleGpio(false)">LOW</el-button>
          </el-button-group>
        </el-form-item>

        <el-divider>命令响应</el-divider>

        <div v-if="lastResponse" class="command-response">
          <pre class="mono-data">{{ JSON.stringify(lastResponse, null, 2) }}</pre>
        </div>
        <div v-else class="empty-hint">暂无响应</div>
      </el-form>
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { CaretRight, VideoPause } from '@element-plus/icons-vue'
import { useDeviceStore } from '@/stores/deviceStore'
import { useAppStore } from '@/stores/appStore'

const deviceStore = useDeviceStore()
const appStore = useAppStore()

const selectedId = computed(() => deviceStore.selectedDeviceId)
const lastResponse = computed(() => appStore.lastCommandResponse)

const sampleRate = ref(1000)
const selectedChannels = ref<number[]>([1, 2, 3, 4])
const gpioPin = ref(2)

function handleStart() {
  if (!selectedId.value) return
  appStore.sendStartCommand(selectedId.value, sampleRate.value, selectedChannels.value)
}

function handleStop() {
  if (!selectedId.value) return
  appStore.sendStopCommand(selectedId.value)
}

function handleGpio(value: boolean) {
  if (!selectedId.value) return
  appStore.sendGpioCommand(selectedId.value, gpioPin.value, value)
}
</script>

<style scoped lang="scss">
.device-control-panel {
  height: 100%;
  overflow: auto;
  padding: 12px;
}

.empty-hint {
  text-align: center;
  color: var(--el-text-color-secondary);
  padding: 40px 0;
  font-size: 13px;
}

.unit {
  margin-left: 8px;
  color: var(--el-text-color-secondary);
  font-size: 12px;
}

.command-response {
  padding: 8px 12px;
  background: var(--bg-card);
  border-radius: 4px;
  max-height: 200px;
  overflow: auto;

  pre {
    margin: 0;
    font-size: 11px;
    white-space: pre-wrap;
  }
}
</style>
