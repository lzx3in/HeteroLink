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
│                   驱动层 (Driver)                            │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ ESP-IDF HAL  │  │ 自定义驱动    │  │ 外设驱动        │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
....

== 📦 组件列表

[cols="1,2,1",options="header"]
|===
|*组件* |*功能* |*状态*

|`main`
|主应用程序，WiFi/MQTT 管理，主循环任务
|✅ 完成

|`spi_dma`
|SPI+DMA 高速数据传输驱动
|✅ 完成

|`uart_protocol`
|UART 自定义二进制协议栈
|✅ 完成

|`adc_gpio_probe`
|ADC/GPIO 双模点测功能
|✅ 完成

|`protocol_examples_common`
|ESP-IDF 通用组件（WiFi 连接）
|✅ 使用官方组件

|`espressif/mqtt`
|ESP-MQTT 客户端库
|✅ 使用官方组件
|===

== 🚀 快速链接

=== 开发流程

. link:../../getting-started/installation.adoc[安装 ESP-IDF]
. link:../../getting-started/quick-start.adoc[编译固件]
. link:../../getting-started/quick-start.adoc#烧录固件[烧录测试]
. link:troubleshooting.adoc[故障排查]

=== 常用命令

[source,bash]
----
# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash

# 监视
idf.py -p /dev/ttyUSB0 monitor

# 测试
idf.py test

# 清理
idf.py fullclean
----

=== 配置工具

[source,bash]
----
# menuconfig
idf.py menuconfig

# 查看配置
idf.py read-project-config

# 保存配置模板
idf.py save-defconfig
----

== 📊 通信通道

HeteroLink 支持三种通信通道：

[cols="1,1,1,1",options="header"]
|===
|*通道* |*协议* |*用途* |*性能*

|远端通道
|MQTT over WiFi
|设备状态上云、远程控制、告警推送
|集群管理

|近端通道
|UART 自定义二进制
|高频遥测、指令下发
|kHz 级数据上传

|板间通道
|SPI+DMA
|高速双向数据传输
|MHz 级时钟

|点测通道
|GPIO+ADC
|数字逻辑探测 + 模拟信号采集
|零代码即插即用
|===

== 🔗 相关资源

* link:../../README.adoc[项目主页]
* link:../../CHANGELOG.md[更新日志]
* link:../../CONTRIBUTING.md[贡献指南]
* https://docs.espressif.com/projects/esp-idf/en/latest/[ESP-IDF 官方文档]

---

*最后更新*: 2026-03-21
