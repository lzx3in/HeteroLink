# HeteroLink ESP32 Subboard 固件

ESP32 协处理器子板固件，实现高速调试协处理器功能。

## 📦 功能特性

### 已实现 ✅

- **WiFi 连接管理**
  - 自动重连
  - 使用 `protocol_examples_common` 组件
  
- **MQTT5 远端通道**
  - 设备状态上云（online/offline）
  - Last Will 遗嘱消息
  - 远程控制命令订阅
  - 遥测数据发布
  - 告警推送

- **UART 自定义二进制协议**（近端通道）
  - 帧格式：`0xAA [CMD] [LEN] [PAYLOAD] [CRC16] 0x55`
  - 波特率：921600
  - 支持心跳、采集控制、遥测数据

- **ADC/GPIO 双模点测**
  - 8 通道可配置
  - ADC 模拟采集（12 位，多档衰减）
  - GPIO 数字探测（输入/输出）
  - 零代码即插即用

- **SPI+DMA 高速数据传输**（板间通道）
  - 全双工通信
  - DMA 零拷贝传输
  - 最高 10MHz 时钟（可配置）

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (Application)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ WiFi 管理     │  │ MQTT 客户端   │  │ 主循环任务      │   │
│  │              │  │              │  │ 数据采集/控制    │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            ↕
┌─────────────────────────────────────────────────────────────┐
│                   协议层 (Protocol)                          │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ UART 协议栈  │  │ SPI+DMA      │  │ ADC/GPIO 探测    │   │
│  │ 近端通信      │  │ 板间通信      │  │ 双模点测        │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## 📁 项目结构

```
firmware/esp32/subboard/
├── main/                      # 主应用
│   ├── app_main.c            # 程序入口 + 主逻辑
│   ├── CMakeLists.txt        # 构建配置
│   ├── idf_component.yml     # 依赖管理
│   └── Kconfig.projbuild     # menuconfig 配置
│
├── components/                # 自定义组件
│   ├── spi_dma/              # SPI+DMA 驱动
│   │   ├── include/spi_dma.h
│   │   ├── spi_dma.c
│   │   └── CMakeLists.txt
│   │
│   ├── uart_protocol/        # UART 协议栈
│   │   ├── include/uart_protocol.h
│   │   ├── uart_protocol.c
│   │   └── CMakeLists.txt
│   │
│   └── adc_gpio_probe/       # ADC/GPIO 探测
│       ├── include/adc_gpio_probe.h
│       ├── adc_gpio_probe.c
│       └── CMakeLists.txt
│
├── sdkconfig.defaults        # 默认配置
└── CMakeLists.txt            # 项目配置
```

## 🚀 快速开始

### 1. 环境准备

```bash
# 安装 ESP-IDF v5.x
git clone -b v5.3 https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32c6  # 根据芯片型号选择
. ./export.sh
```

### 2. 配置项目

```bash
cd firmware/esp32/subboard

# 方法 1: 使用默认配置
# 编辑 sdkconfig.defaults，修改 WiFi 和 MQTT 配置

# 方法 2: 使用 menuconfig
idf.py menuconfig
# HeteroLink Configuration → 配置 WiFi/MQTT/设备参数
```

### 3. 编译烧录

```bash
# 编译
idf.py build

# 烧录（替换 /dev/ttyUSB0 为你的串口）
idf.py -p /dev/ttyUSB0 flash

# 监视串口输出
idf.py -p /dev/ttyUSB0 monitor
```

### 4. 测试 MQTT 连接

```bash
# 使用 mosquitto 订阅设备状态
mosquitto_sub -h broker.emqx.io -t "heterolink/subboard/+/status" -v

# 订阅遥测数据
mosquitto_sub -h broker.emqx.io -t "heterolink/subboard/+/telemetry" -v

# 发送控制命令
mosquitto_pub -h broker.emqx.io \
  -t "heterolink/subboard/subboard_001/command" \
  -m '{"cmd": "start"}'
```

## 📡 通信协议

### MQTT Topic 规范

| Topic | 方向 | QoS | Retain | 说明 |
|-------|------|-----|--------|------|
| `heterolink/subboard/{id}/status` | 设备→云 | 1 | ✅ | 设备状态（online/offline） |
| `heterolink/subboard/{id}/telemetry` | 设备→云 | 0 | ❌ | 遥测数据 |
| `heterolink/subboard/{id}/command` | 云→设备 | 1 | ❌ | 控制命令 |
| `heterolink/subboard/{id}/alarm` | 设备→云 | 1 | ❌ | 告警通知 |

### UART 帧格式

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| 帧头 0xAA     | 命令字        | 数据长度 (LE)                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
~                         数据载荷                              ~
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| CRC16 (LE)    | 帧尾 0x55     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 命令字定义

| 命令字 | 名称 | 方向 | 说明 |
|--------|------|------|------|
| 0x01 | CMD_HEARTBEAT | 双向 | 心跳请求/响应 |
| 0x21 | CMD_START_ACQ | 主机→设备 | 开始采集 |
| 0x22 | CMD_STOP_ACQ | 主机→设备 | 停止采集 |
| 0x10 | CMD_TELEMETRY | 设备→主机 | 遥测数据 |
| 0xFF | CMD_ERROR | 设备→主机 | 错误报告 |

## ⚙️ 配置选项

通过 `idf.py menuconfig` 配置：

### WiFi Configuration
- `HETERO_WIFI_SSID`: WiFi 名称
- `HETERO_WIFI_PASSWORD`: WiFi 密码
- `HETERO_WIFI_AUTO_RECONNECT`: 自动重连

### MQTT Configuration
- `HETERO_MQTT_BROKER_URI`: MQTT Broker 地址
- `HETERO_MQTT_BROKER_PORT`: MQTT 端口
- `HETERO_MQTT_CLIENT_ID`: 客户端 ID
- `HETERO_MQTT_USERNAME`: 用户名（可选）
- `HETERO_MQTT_PASSWORD`: 密码（可选）

### Device Configuration
- `HETERO_DEVICE_ID`: 设备唯一标识
- `HETERO_DATA_UPLOAD_INTERVAL`: 数据上报间隔（ms）
- `HETERO_ENABLE_UART_PROTOCOL`: 启用 UART 协议
- `HETERO_ENABLE_SPI_DMA`: 启用 SPI+DMA
- `HETERO_ENABLE_ADC_GPIO_PROBE`: 启用 ADC/GPIO 探测

## 🧪 测试

### 单元测试

```bash
# 运行所有测试
idf.py test
```

### 功能测试

1. **WiFi 连接测试**
   ```bash
   idf.py monitor
   # 查看 "WiFi connected!" 日志
   ```

2. **MQTT 连接测试**
   ```bash
   # 订阅状态 topic
   mosquitto_sub -h broker.emqx.io -t "heterolink/subboard/+/status" -v
   # 应收到：heterolink/subboard/subboard_001/status {"state":"online",...}
   ```

3. **UART 协议测试**
   ```bash
   # 使用串口工具发送心跳帧
   echo -ne "\xAA\x01\x00\x00\xC0\xC1\x55" > /dev/ttyUSB0
   # 应收到心跳响应
   ```

## 📊 内存使用

典型内存占用（ESP32-C6）：

- **代码段**: ~500KB
- **数据段**: ~50KB
- **空闲堆**: ~150KB

## 🔧 故障排查

### WiFi 连接失败

1. 检查 SSID/密码是否正确
2. 确认 2.4GHz 网络（ESP32 不支持 5GHz）
3. 检查信号强度

### MQTT 连接失败

1. 确认 Broker 地址和端口
2. 检查防火墙规则
3. 查看 MQTT 日志：`CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y`

### UART 通信失败

1. 检查 TX/RX 引脚连接
2. 确认波特率匹配（默认 921600）
3. 验证 CRC16 校验

## 📚 相关文档

- [固件架构](../../docs/firmware/architecture.adoc)
- [协议规范](../../docs/software/protocol-spec.adoc)
- [MQTT 使用指南](../../docs/software/mqtt-guide.adoc)
- [快速开始](../../docs/getting-started/quick-start.adoc)

## 📝 待办事项

- [ ] 完善 MQTT 命令解析（JSON 解析器）
- [ ] 添加 OTA 升级支持
- [ ] 实现配置持久化（NVS）
- [ ] 添加 FreeRTOS 任务监控
- [ ] 完善错误处理和日志

## 📄 许可证

Apache-2.0

---

*最后更新*: 2026-03-21
