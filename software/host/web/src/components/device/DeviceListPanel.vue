<template>
  <div class="device-list-panel">
    <div class="panel-header">
      <span class="panel-title">设备列表</span>
      <el-button size="small" text @click="showAddDialog = true">
        <el-icon><Plus /></el-icon>
      </el-button>
    </div>
    <el-scrollbar class="device-scroll">
      <div v-if="deviceStore.devices.length === 0" class="empty-hint">
        暂无设备，请添加或连接 MQTT
      </div>
      <div
        v-for="device in deviceStore.devices"
        :key="device.id"
        class="device-item"
        :class="{ selected: deviceStore.selectedDeviceId === device.id }"
        @click="deviceStore.selectDevice(device.id)"
      >
        <div class="device-info">
          <span class="status-dot" :class="statusClass(device)"></span>
          <span class="device-name">{{ device.name || device.id }}</span>
        </div>
        <div class="device-meta">
          <el-tag size="small" :type="device.connected ? 'success' : 'info'" effect="plain">
            {{ device.connected ? '在线' : '离线' }}
          </el-tag>
          <el-dropdown trigger="click" @command="(cmd: string) => handleCommand(cmd, device.id)">
            <el-button size="small" text @click.stop>
              <el-icon><MoreFilled /></el-icon>
            </el-button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="disconnect" :disabled="!device.connected">
                  断开连接
                </el-dropdown-item>
                <el-dropdown-item command="remove" divided>
                  <span style="color: var(--el-color-danger)">移除设备</span>
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </div>
    </el-scrollbar>

    <el-dialog v-model="showAddDialog" title="添加设备" width="320px" :append-to-body="true">
      <el-input v-model="newDeviceName" placeholder="输入设备名称" @keyup.enter="handleAdd" />
      <template #footer>
        <el-button @click="showAddDialog = false">取消</el-button>
        <el-button type="primary" :disabled="!newDeviceName.trim()" @click="handleAdd">
          添加
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { Plus, MoreFilled } from '@element-plus/icons-vue'
import { useDeviceStore } from '@/stores/deviceStore'
import type { DeviceInfo } from '@/types'

const deviceStore = useDeviceStore()
const showAddDialog = ref(false)
const newDeviceName = ref('')

function statusClass(device: DeviceInfo) {
  if (!device.connected) return 'offline'
  if (!device.online) return 'offline'
  return 'online'
}

function handleAdd() {
  const name = newDeviceName.value.trim()
  if (name) {
    deviceStore.addDevice(name)
    newDeviceName.value = ''
    showAddDialog.value = false
  }
}

function handleCommand(cmd: string, deviceId: string) {
  if (cmd === 'disconnect') {
    deviceStore.disconnectDevice(deviceId)
  } else if (cmd === 'remove') {
    deviceStore.removeDevice(deviceId)
  }
}
</script>

<style scoped lang="scss">
.device-list-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--bg-panel);
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 12px;
  border-bottom: 1px solid var(--border-color);

  .panel-title {
    font-size: 13px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }
}

.device-scroll {
  flex: 1;
}

.empty-hint {
  padding: 24px 16px;
  text-align: center;
  color: var(--el-text-color-secondary);
  font-size: 12px;
}

.device-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 12px;
  cursor: pointer;
  border-bottom: 1px solid var(--border-color-lighter);
  transition: background 0.15s;

  &:hover {
    background: var(--bg-hover);
  }

  &.selected {
    background: #094771;
  }
}

.device-info {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 0;
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  flex-shrink: 0;

  &.online { background: var(--status-online); }
  &.offline { background: var(--el-text-color-placeholder); }
  &.alarm { background: var(--status-alarm); animation: blink 0.8s infinite; }
}

@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}

.device-name {
  font-size: 13px;
  font-weight: 500;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.device-meta {
  display: flex;
  align-items: center;
  gap: 4px;
  flex-shrink: 0;
}
</style>
