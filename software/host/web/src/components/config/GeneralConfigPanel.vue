<template>
  <div class="general-config-panel">
    <el-form :model="form" label-width="90px" size="small">
      <el-divider>数据记录</el-divider>

      <el-form-item label="记录目录">
        <el-input v-model="form.recording_dir" placeholder="./recordings" />
      </el-form-item>

      <el-form-item label="最大文件大小">
        <el-input-number v-model="form.max_file_size" :min="1" :max="1000" />
        <span class="unit">MB</span>
      </el-form-item>

      <el-divider>显示</el-divider>

      <el-form-item label="数据缓冲">
        <el-input-number v-model="form.display_buffer_size" :min="50" :max="1000" :step="50" />
        <span class="unit">条</span>
      </el-form-item>

      <el-form-item label="波形点数">
        <el-input-number v-model="form.waveform_points" :min="50" :max="500" :step="50" />
      </el-form-item>

      <el-divider>操作</el-divider>

      <el-form-item>
        <el-button-group>
          <el-button type="primary" @click="handleSave" :loading="saving">
            保存配置
          </el-button>
          <el-button @click="handleLoad">
            重新加载
          </el-button>
          <el-button type="warning" @click="handleReset">
            恢复默认
          </el-button>
        </el-button-group>
      </el-form-item>
    </el-form>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useConfigStore } from '@/stores/configStore'
import { ElMessage } from 'element-plus'
import type { AppConfig } from '@/types'

const configStore = useConfigStore()
const saving = ref(false)

const defaultConfig: AppConfig = {
  recording_dir: './recordings',
  max_file_size: 100,
  display_buffer_size: 200,
  waveform_points: 200,
}

const form = ref<AppConfig>({ ...defaultConfig })

onMounted(async () => {
  await configStore.fetchConfig()
  if (configStore.config) {
    form.value = { ...form.value, ...configStore.config }
  }
})

async function handleSave() {
  saving.value = true
  try {
    await configStore.saveConfig(form.value)
    ElMessage.success('配置已保存')
  } catch {
    ElMessage.error('保存失败')
  } finally {
    saving.value = false
  }
}

async function handleLoad() {
  try {
    await configStore.loadConfig()
    if (configStore.config) {
      form.value = { ...form.value, ...configStore.config }
    }
    ElMessage.success('配置已加载')
  } catch {
    ElMessage.error('加载失败')
  }
}

async function handleReset() {
  try {
    await configStore.resetConfig()
    if (configStore.config) {
      form.value = { ...form.value, ...configStore.config }
    }
    ElMessage.success('已恢复默认')
  } catch {
    ElMessage.error('重置失败')
  }
}
</script>

<style scoped lang="scss">
.general-config-panel {
  padding: 12px;
}

.unit {
  margin-left: 8px;
  color: var(--el-text-color-secondary);
  font-size: 12px;
}
</style>
