<template>
  <div class="log-panel">
    <div class="log-actions">
      <el-button size="small" text @click="logStore.clearLogs()">
        <el-icon><Delete /></el-icon> 清空
      </el-button>
    </div>
    <el-scrollbar ref="scrollRef" class="log-scroll">
      <div v-if="logStore.messages.length === 0" class="empty-hint">暂无日志</div>
      <div v-for="(msg, idx) in logStore.messages" :key="idx" class="log-line" :class="'log-' + msg.level">
        <span class="log-time">{{ msg.time }}</span>
        <span class="log-level">{{ msg.level.toUpperCase() }}</span>
        <span class="log-text">{{ msg.text }}</span>
      </div>
    </el-scrollbar>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick } from 'vue'
import { Delete } from '@element-plus/icons-vue'
import { useLogStore } from '@/stores/logStore'

const logStore = useLogStore()
const scrollRef = ref()

watch(
  () => logStore.messages.length,
  () => {
    nextTick(() => {
      const el = scrollRef.value?.wrapRef
      if (el) el.scrollTop = el.scrollHeight
    })
  },
)
</script>

<style scoped lang="scss">
.log-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.log-actions {
  display: flex;
  justify-content: flex-end;
  padding: 4px 8px;
  border-bottom: 1px solid var(--border-color);
}

.log-scroll {
  flex: 1;
}

.empty-hint {
  text-align: center;
  color: var(--el-text-color-secondary);
  padding: 40px 0;
}

.log-line {
  display: flex;
  gap: 8px;
  padding: 2px 12px;
  font-family: 'Courier New', monospace;
  font-size: 11px;
  line-height: 1.6;

  &.log-error .log-level { color: #f44336; }
  &.log-warn .log-level { color: #ff9800; }
  &.log-info .log-level { color: #4fc3f7; }
}

.log-time {
  color: var(--el-text-color-secondary);
  flex-shrink: 0;
}

.log-level {
  width: 40px;
  text-align: right;
  flex-shrink: 0;
  font-weight: 600;
}

.log-text {
  color: var(--el-text-color-regular);
  word-break: break-all;
}
</style>
