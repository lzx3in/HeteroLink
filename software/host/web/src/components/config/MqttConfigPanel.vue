<template>
  <div class="mqtt-config-panel">
    <el-form :model="form" label-width="80px" size="small">
      <el-form-item label="Broker">
        <el-input v-model="form.broker_url" placeholder="mqtt://localhost:1883" />
      </el-form-item>

      <el-form-item label="客户端 ID">
        <el-input v-model="form.client_id" placeholder="heterolink-host" />
      </el-form-item>

      <el-form-item label="用户名">
        <el-input v-model="form.username" placeholder="可选" />
      </el-form-item>

      <el-form-item label="密码">
        <el-input v-model="form.password" type="password" show-password placeholder="可选" />
      </el-form-item>

      <el-form-item label="主题前缀">
        <el-input v-model="form.topic_prefix" placeholder="heterolink" />
      </el-form-item>

      <el-form-item label="Keep Alive">
        <el-input-number v-model="form.keep_alive" :min="10" :max="600" :step="10" />
        <span class="unit">秒</span>
      </el-form-item>

      <el-form-item>
        <el-button-group>
          <el-button
            v-if="!configStore.mqttConnected"
            type="success"
            @click="handleConnect"
            :loading="loading"
          >
            连接
          </el-button>
          <el-button
            v-else
            type="danger"
            @click="handleDisconnect"
            :loading="loading"
          >
            断开
          </el-button>
        </el-button-group>
      </el-form-item>
    </el-form>

    <div class="mqtt-status">
      <el-tag :type="configStore.mqttConnected ? 'success' : 'danger'" effect="plain">
        {{ configStore.mqttConnected ? '已连接' : '未连接' }}
      </el-tag>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useConfigStore } from '@/stores/configStore'
import type { MqttConfig } from '@/types'

const configStore = useConfigStore()
const loading = ref(false)

const defaultConfig: MqttConfig = {
  broker_url: 'mqtt://localhost:1883',
  client_id: 'heterolink-host',
  username: '',
  password: '',
  topic_prefix: 'heterolink',
  keep_alive: 60,
}

const form = ref<MqttConfig>({ ...defaultConfig })

onMounted(async () => {
  await configStore.fetchConfig()
  if (configStore.mqttConfig) {
    form.value = { ...form.value, ...configStore.mqttConfig }
  }
})

async function handleConnect() {
  loading.value = true
  try {
    await configStore.connectMqtt(form.value)
  } finally {
    loading.value = false
  }
}

async function handleDisconnect() {
  loading.value = true
  try {
    await configStore.disconnectMqtt()
  } finally {
    loading.value = false
  }
}
</script>

<style scoped lang="scss">
.mqtt-config-panel {
  padding: 12px;
}

.unit {
  margin-left: 8px;
  color: var(--el-text-color-secondary);
  font-size: 12px;
}

.mqtt-status {
  padding: 8px 0;
  text-align: center;
}
</style>
