# HeteroLink 固件文档

ESP32 固件开发文档和 API 参考。

---

## 📚 文档导航

### 核心文档

- [架构设计](architecture.md) - 系统架构和组件设计
- [配置指南](configuration.md) - 编译和配置选项
- [API 参考](api-reference.md) - 接口文档

### 协议规范

- [MQTT 协议](mqtt-protocol.md) - 远端通信协议
- [UART 协议](uart-protocol.md) - 近端通信协议

### 开发指南

- [快速开始](../getting-started/quick-start.md) - 入门教程
- [示例项目](../examples/README.md) - 代码示例
- [问题排查](../getting-started/troubleshooting.md) - 常见问题

---

## 🔧 固件功能

### 已实现

- ✅ WiFi 连接管理
- ✅ MQTT5 远端通道
- ✅ 设备状态上报
- ✅ Last Will 遗嘱消息
- ✅ 自动重连机制

### 开发中

- 🟡 UART 近端通道 (自定义二进制协议)
- 🟡 SPI+DMA 高速数据传输
- 🟡 GPIO/ADC 双模点测
- 🟡 OTA 固件升级

### 计划中

- 🔴 安全启动
- 🔴 Flash 加密
- 🔴 MQTT over TLS
- 🔴 本地数据存储

---

## 📦 组件列表

| 组件 | 状态 | 描述 |
|------|------|------|
| heterolink_core | 🟡 开发中 | 核心库 |
| mqtt_client | ✅ 已完成 | MQTT 客户端 |
| uart_protocol | 🔴 计划中 | UART 协议栈 |
| spi_dma | 🔴 计划中 | SPI+DMA 驱动 |
| adc_gpio_probe | 🔴 计划中 | ADC/GPIO 探测 |

---

## 🎯 开发环境

### 要求

- ESP-IDF v5.3+
- ESP32 / ESP32-S3 / ESP32-C3 / ESP32-C6
- C/C++ 开发经验

### 工具

```bash
# 安装 ESP-IDF
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32 esp32s3 esp32c6
. export.sh

# 验证安装
idf.py --version
```

---

## 📖 快速开始

```bash
# 克隆项目
git clone https://github.com/HeteroLink/HeteroLink.git
cd HeteroLink/firmware/esp32/subboard

# 设置目标芯片
idf.py set-target esp32c6

# 配置项目
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash monitor
```

详细教程：[快速开始指南](../getting-started/quick-start.md)

---

## 🤝 贡献固件

### 开发流程

1. Fork 项目
2. 创建功能分支
3. 开发和测试
4. 提交 PR

### 代码规范

- 遵循 ESP-IDF 编码规范
- 添加必要的注释
- 编写单元测试
- 更新文档

### 提交 PR

在 PR 描述中说明：
- 功能描述
- 测试情况
- 相关 Issue

---

## 📞 技术支持

- 📖 [配置指南](configuration.md)
- ❓ [问题排查](../getting-started/troubleshooting.md)
- 💬 [社区讨论](https://github.com/HeteroLink/HeteroLink/discussions)

---

**最后更新**: 2026-03-21
