= HeteroLink 固件文档索引

ESP32 协处理器子板固件系统文档。

== 📚 文档导航

=== 核心文档

* link:architecture.adoc[固件架构] - 系统设计、组件结构、数据流
* link:configuration.adoc[配置指南] - menuconfig 选项、sdkconfig 配置
* link:../software/protocol-spec.adoc[协议规范] - UART 二进制协议、MQTT 协议

=== 快速开始

* link:../../getting-started/quick-start.adoc[快速开始] - 5 分钟上手指南
* link:../../getting-started/installation.adoc[安装指南] - ESP-IDF 环境搭建
* link:../../getting-started/troubleshooting.adoc[故障排查] - 常见问题解决

=== 组件文档

* link:../software/mqtt-guide.adoc[MQTT 使用指南] - MQTT 配置、Topic 规范、示例
* `components/spi_dma/` - SPI+DMA 驱动 API 文档
* `components/uart_protocol/` - UART 协议栈 API 文档
* `components/adc_gpio_probe/` - ADC/GPIO 探测 API 文档

=== 示例代码

* link:../examples/[示例项目] - 参考设计和测试代码

== 🏗️ 固件架构概览

=== 系统架构图

....
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
                            ↕
┌─────────────────────────────────────────────────────────────┐
│                   驱动层 (ESP-IDF HAL)                       │
│  WiFi  │  MQTT  │  UART  │  SPI  │  DMA  │  ADC  │  GPIO   │
└─────────────────────────────────────────────────────────────┘
....

=== 数据流

**远端通道**: ADC 采集 → JSON 封装 → MQTT Publish → WiFi → Broker → 上位机

**近端通道**: 上位机 → UART 接收 → 帧解析 → 命令处理 → 响应

**板间通道**: 目标主板 → SPI → DMA → 共享内存 → 高速传输

== 📦 组件列表（v0.1.0）

[cols="1,3,1",options="header"]
|===
|*组件* |*功能* |*状态*

|`main`
|主应用程序
- WiFi 连接管理（自动重连）
- MQTT5 客户端（状态/命令/遥测/告警）
- 主循环任务（10ms 周期）
- 组件初始化
|✅ 完成

|`spi_dma`
|SPI+DMA 高速数据传输驱动
- 全双工通信
- DMA 零拷贝传输
- 异步传输和回调
- 最高 10MHz 时钟
|✅ 完成

|`uart_protocol`
|UART 自定义二进制协议栈
- 帧格式：0xAA [CMD] [LEN] [PAYLOAD] [CRC16] 0x55
- 波特率：921600
- CRC16/MODBUS 校验
- 支持心跳、采集控制、遥测、错误报告
|✅ 完成

|`adc_gpio_probe`
|ADC/GPIO 双模点测功能
- 8 通道可配置（ADC 模拟/GPIO 数字）
- ADC 连续采样和单次读取
- ADC 值转电压（mV）
- 零代码即插即用
|✅ 完成

|`protocol_examples_common`
|ESP-IDF 官方 WiFi 连接组件
|✅ 使用官方

|`espressif/mqtt`
|ESP-MQTT 客户端库（MQTT 5.0）
|✅ 使用官方
|===

== 🚀 快速链接

=== 开发流程

. link:../../getting-started/quick-start.adoc[快速开始] - 5 分钟上手
. link:../../getting-started/installation.adoc[安装 ESP-IDF] - 环境搭建
. link:configuration.adoc[配置指南] - menuconfig 详解
. link:architecture.adoc[固件架构] - 系统设计
. link:troubleshooting.adoc[故障排查] - 问题解决

=== 常用命令

**编译烧录**:
[source,bash]
----
# 编译
idf.py build

# 烧录（替换为实际端口）
idf.py -p /dev/ttyUSB0 flash

# 监视串口
idf.py -p /dev/ttyUSB0 monitor

# 清理
idf.py fullclean
----

**配置工具**:
[source,bash]
----
# menuconfig
idf.py menuconfig

# 查看配置
idf.py read-project-config

# 保存配置模板
idf.py save-defconfig
----

**测试**:
[source,bash]
----
# 运行测试（待实现）
idf.py test
----

== 📊 通信通道（已实现）

HeteroLink 支持四种通信通道：

[cols="1,2,2,1",options="header"]
|===
|*通道* |*协议* |*用途* |*状态*

|远端通道
|MQTT over WiFi
|设备状态上云、远程控制、告警推送、集群管理
|✅ 完成

|近端通道
|UART 自定义二进制（921600 波特）
|高频遥测、指令下发、CRC16 校验
|✅ 完成

|板间通道
|SPI+DMA（最高 10MHz）
|高速双向数据传输、零拷贝、异步回调
|✅ 完成

|点测通道
|GPIO+ADC（8 通道可配置）
|数字逻辑探测 + 模拟信号采集、零代码即插即用
|✅ 完成
|===

== 🔗 相关资源

=== 项目文档
* link:../../README.adoc[项目主页]
* link:../../CHANGELOG.md[更新日志]
* link:../../CONTRIBUTING.md[贡献指南]

=== 外部资源
* https://docs.espressif.com/projects/esp-idf/en/latest/[ESP-IDF 官方文档]
* https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html[ESP-MQTT 组件]
* https://www.espressif.com/en/products/socs/esp32[ESP32 系列芯片]

---

**维护者**: HeteroLink Contributors  
**版本**: 0.1.0  
**最后更新**: 2026-03-21
