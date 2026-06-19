import { defineStore } from 'pinia'
import { ref } from 'vue'

export interface LogMessage {
  time: string
  level: string
  text: string
}

const MAX_MESSAGES = 100

export const useLogStore = defineStore('log', () => {
  const messages = ref<LogMessage[]>([])

  function addLog(level: string, text: string) {
    messages.value.push({
      time: new Date().toLocaleTimeString('zh-CN', { hour12: false }),
      level,
      text,
    })
    if (messages.value.length > MAX_MESSAGES) {
      messages.value.splice(0, messages.value.length - MAX_MESSAGES)
    }
  }

  function clearLogs() {
    messages.value = []
  }

  return { messages, addLog, clearLogs }
})
