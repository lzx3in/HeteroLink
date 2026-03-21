# ESP32 芯片兼容性

HeteroLink 支持多种 ESP32 系列芯片。

---

## ✅ 支持的芯片

### ESP32-C6 (推荐)

**状态**: ✅ 完全支持 | **推荐度**: ⭐⭐⭐⭐⭐

| 特性 | 规格 |
|------|------|
| CPU | RISC-V 单核 160 MHz |
| Flash | 4MB (默认) |
| WiFi | WiFi 6 (802.11ax) |
| Bluetooth | BLE 5.0 |
| GPIO | 22 个 |
| ADC | 12-bit, 6 通道 |
| SPI | 2 路 |
| UART | 2 路 |
| I2C | 1 路 |
| 工作电压 | 3.0-3.6V |

**优势**:
- 最新 WiFi 6 支持
- 低功耗
- 成本低
- 推荐用于新项目

**开发板**:
- ESP32-C6-DevKitC-1
- Seeed XIAO ESP32C6
- 其他兼容开发板

---

### ESP32-S3

**状态**: ✅ 完全支持 | **推荐度**: ⭐⭐⭐⭐

| 特性 | 规格 |
|------|------|
| CPU | Xtensa 双核 240 MHz |
| Flash | 8MB (默认) |
| WiFi | WiFi 4 (802.11n) |
| Bluetooth | BLE 5.0 |
| GPIO | 45 个 |
| ADC | 12-bit, 20 通道 |
| PSRAM | 可选 2/4/8MB |
| AI 加速 | ✅ 向量指令 |

**优势**:
- 高性能双核
- 更多 GPIO
- AI 加速能力
- 适合图像处理

**开发板**:
- ESP32-S3-DevKitC-1
- ESP32-S3-WROOM-1

---

### ESP32-C3

**状态**: ✅ 完全支持 | **推荐度**: ⭐⭐⭐

| 特性 | 规格 |
|------|------|
| CPU | RISC-V 单核 160 MHz |
| Flash | 4MB (默认) |
| WiFi | WiFi 4 (802.11n) |
| Bluetooth | BLE 5.0 |
| GPIO | 17 个 |
| ADC | 12-bit, 6 通道 |
| 工作电压 | 3.0-3.6V |

**优势**:
- 低成本
- 低功耗
- 兼容 ESP32-C6 代码

**开发板**:
- ESP32-C3-DevKitC-02
- ESP32-C3-WROOM-02

---

### ESP32 (经典款)

**状态**: ✅ 支持 | **推荐度**: ⭐⭐

| 特性 | 规格 |
|------|------|
| CPU | Xtensa 双核 240 MHz |
| Flash | 4MB (默认) |
| WiFi | WiFi 4 (802.11n) |
| Bluetooth | 经典 + BLE |
| GPIO | 34 个 |
| ADC | 12-bit, 18 通道 |
| DAC | 8-bit, 2 通道 |

**优势**:
- 成熟稳定
- 生态丰富
- 双核性能

**注意**:
- 功耗较高
- 逐渐被新款替代

**开发板**:
- ESP32-DevKitC
- ESP32-WROOM-32

---

## 📊 对比表格

| 特性 | ESP32-C6 | ESP32-S3 | ESP32-C3 | ESP32 |
|------|----------|----------|----------|-------|
| **CPU** | RISC-V 160MHz | Xtensa 240MHz 双核 | RISC-V 160MHz | Xtensa 240MHz 双核 |
| **WiFi** | WiFi 6 | WiFi 4 | WiFi 4 | WiFi 4 |
| **蓝牙** | BLE 5.0 | BLE 5.0 | BLE 5.0 | 经典+BLE |
| **GPIO** | 22 | 45 | 17 | 34 |
| **ADC** | 6 通道 | 20 通道 | 6 通道 | 18 通道 |
| **PSRAM** | ❌ | ✅ | ❌ | ✅ |
| **USB** | ❌ | ✅ (OTG) | ❌ | ❌ |
| **AI 加速** | ❌ | ✅ | ❌ | ❌ |
| **成本** | $ | $$ | $ | $$ |
| **功耗** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ |
| **推荐度** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |

---

## 🔧 编译配置

### 设置目标芯片

```bash
# ESP32-C6 (推荐)
idf.py set-target esp32c6

# ESP32-S3
idf.py set-target esp32s3

# ESP32-C3
idf.py set-target esp32c3

# ESP32
idf.py set-target esp32
```

### 菜单配置

```bash
idf.py menuconfig

# 导航到:
# HeteroLink Configuration →
#   Target Chip Selection
```

---

## 📝 代码兼容性

### 通用代码

以下代码在所有芯片上通用：

```c
// WiFi 连接
#include "esp_wifi.h"

// MQTT 客户端
#include "esp_mqtt.h"

// GPIO 控制
#include "driver/gpio.h"

// ADC 采集
#include "driver/adc.h"
```

### 芯片特定代码

某些功能需要条件编译：

```c
#ifdef CONFIG_IDF_TARGET_ESP32S3
    // ESP32-S3 特定代码
    // 例如：PSRAM 配置
    esp_psram_init();
#endif

#ifdef CONFIG_IDF_TARGET_ESP32C6
    // ESP32-C6 特定代码
    // 例如：WiFi 6 配置
#endif
```

---

## 🎯 选择建议

### 选择 ESP32-C6 如果：

- ✅ 需要最新 WiFi 6
- ✅ 关注低功耗
- ✅ 成本敏感
- ✅ 新项目开发

### 选择 ESP32-S3 如果：

- ✅ 需要高性能
- ✅ 需要 PSRAM
- ✅ 涉及图像处理
- ✅ 需要 USB OTG

### 选择 ESP32-C3 如果：

- ✅ 预算有限
- ✅ 简单应用
- ✅ 低功耗需求

### 选择 ESP32 如果：

- ✅ 已有 ESP32 开发板
- ✅ 需要经典蓝牙
- ✅ 维护旧项目

---

## 📚 参考资源

- [ESP32-C6 技术参考](https://www.espressif.com/en/products/socs/esp32-c6)
- [ESP32-S3 技术参考](https://www.espressif.com/en/products/socs/esp32-s3)
- [ESP32-C3 技术参考](https://www.espressif.com/en/products/socs/esp32-c3)
- [ESP32 技术参考](https://www.espressif.com/en/products/socs/esp32)

---

**最后更新**: 2026-03-21
