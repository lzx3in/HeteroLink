# MQTT 功能实现总结

## 实现概述

本次完善为 HeteroLink 上位机添加了完整的 MQTT 远端通信支持，实现了设备集群管理、云端连接和远程控制功能。

## 主要变更

### 1. MqttChannel 类增强

**文件**: `src/protocol/MqttChannel.h`, `src/protocol/MqttChannel.cpp`

#### 新增功能
- ✅ **自动重连机制**: 指数退避策略，最大 5 次重试，最大延迟 30 秒
- ✅ **MQTT 5.0 支持**: 使用 `QMqttClient::MQTT_5_0` 协议版本
- ✅ **自动生成 Client ID**: 如果未指定，自动生成唯一客户端 ID
- ✅ **Last Will 配置**: 支持遗嘱消息 Topic 和内容配置
- ✅ **详细错误处理**: 针对不同 MQTT 错误类型提供具体错误信息

#### 新增方法
```cpp
// 发布命令到设备
void publishCommand(const QString& deviceId, const QString& command);

// 订阅所有设备状态
void subscribeAllDeviceStatus();

// 订阅所有设备遥测
void subscribeAllDeviceTelemetry();

// 获取当前配置
MqttConfig getConfig() const;
```

#### 新增信号
```cpp
// 收到设备状态
void deviceStatusReceived(const QString& deviceId, bool online);

// 收到设备命令
void deviceCommandReceived(const QString& deviceId, const QString& command);

// 收到遥测数据
void telemetryReceived(const QString& deviceId, const QString& data);
```

### 2. DeviceManager 类增强

**文件**: `src/core/DeviceManager.h`, `src/core/DeviceManager.cpp`

#### 新增功能
- ✅ **MQTT 设备管理**: 支持通过 MQTT 连接和管理设备
- ✅ **设备自动发现**: 收到设备状态时自动添加新设备
- ✅ **设备状态同步**: 同步 MQTT 连接状态和设备在线状态

#### 新增方法
```cpp
// 连接设备（MQTT 模式）
bool connectDeviceMqtt(const QString& deviceId, const QString& brokerHost, quint16 brokerPort);

// 断开设备（MQTT 模式）
void disconnectDeviceMqtt(const QString& deviceId);

// 发送控制命令（MQTT 模式）
void sendControlCommandMqtt(const QString& deviceId, const QString& command);
```

#### 信号集成
- 连接 `MqttChannel::connectionChanged` 更新所有 MQTT 设备状态
- 连接 `MqttChannel::deviceStatusReceived` 处理设备上下线
- 连接 `MqttChannel::telemetryReceived` 处理遥测数据
- 连接 `MqttChannel::deviceCommandReceived` 处理远程命令

### 3. ConfigPanel 配置界面增强

**文件**: `src/ui/ConfigPanel.h`, `src/ui/ConfigPanel.cpp`

#### 新增配置项
- ✅ **客户端 ID**: 支持自定义或自动生成
- ✅ **TLS 加密**: 启用 TLS 加密连接（配置框架，具体实现待完成）
- ✅ **Last Will 开关**: 启用/禁用遗嘱消息
- ✅ **Will Topic**: 遗嘱消息 Topic 配置
- ✅ **Will Message**: 遗嘱消息内容配置

#### UI 改进
- 添加配置提示文本
- Will 配置输入框根据开关自动启用/禁用
- 密码输入框使用加密显示

### 4. CMake 构建配置

**文件**: `software/host/CMakeLists.txt`

#### Qt6 MQTT 支持
```cmake
find_package(Qt6 OPTIONAL_COMPONENTS Charts Mqtt)
if(TARGET Qt6::Mqtt)
    target_link_libraries(heterolink-host PRIVATE Qt6::Mqtt)
    target_compile_definitions(heterolink-host PRIVATE HAS_QT_MQTT)
endif()
```

#### Qt5 MQTT 支持（兼容）
```cmake
find_path(QT_MQTT_INCLUDE_DIR QMqttClient)
if(QT_MQTT_INCLUDE_DIR)
    target_include_directories(heterolink-host PRIVATE ${QT_MQTT_INCLUDE_DIR})
    target_compile_definitions(heterolink-host PRIVATE HAS_QT_MQTT)
endif()
```

### 5. 文档

#### 新增文档
- ✅ `docs/software/mqtt-guide.md` - MQTT 使用指南
  - 系统架构图
  - Topic 规范说明
  - 配置说明
  - 上位机和 ESP32 代码示例
  - 公共 Broker 推荐
  - 私有 Broker 部署指南
  - 调试技巧
  - 故障排查

#### 更新文档
- ✅ `software/host/README.md` - 添加 MQTT 功能说明和依赖安装指南
- ✅ `README.md` - 更新项目结构说明

### 6. 示例程序

**文件**: `examples/mqtt_test.cpp`, `examples/CMakeLists.txt`

独立的 MQTT 功能测试程序，用于验证：
- Broker 连接
- Topic 订阅
- 消息发布
- 消息接收

## Topic 规范

```
heterolink/subboard/{deviceId}/status      # 设备状态（online/offline）
heterolink/subboard/{deviceId}/telemetry   # 遥测数据（JSON 格式）
heterolink/subboard/{deviceId}/command     # 设备命令（JSON 格式）
```

### 通配符订阅
```
heterolink/subboard/+/status      # 所有设备状态
heterolink/subboard/+/telemetry   # 所有设备遥测
heterolink/subboard/#             # 所有消息
```

## 使用示例

### 1. 配置 MQTT 连接

```cpp
#include "protocol/MqttChannel.h"

using namespace HeteroLink;

auto mqttChannel = std::make_shared<MqttChannel>();

MqttConfig config;
config.brokerHost = "broker.emqx.io";
config.brokerPort = 1883;
config.clientId = "heterolink_host_001";
config.willTopic = "heterolink/subboard/status";
config.willMessage = "offline";

mqttChannel->connect(config);
```

### 2. 订阅设备消息

```cpp
// 订阅所有设备状态
mqttChannel->subscribeAllDeviceStatus();

// 订阅特定设备命令
mqttChannel->subscribeDeviceCommands("device_001");

// 监听信号
connect(mqttChannel.get(), &MqttChannel::deviceStatusReceived,
        [](const QString& deviceId, bool online) {
    qDebug() << "Device" << deviceId << "is" << (online ? "online" : "offline");
});
```

### 3. 发布消息

```cpp
// 发布设备状态
mqttChannel->publishDeviceStatus("device_001", true);

// 发布遥测数据
mqttChannel->publishTelemetry("device_001", R"({"temp":25.5})");

// 发布命令
mqttChannel->publishCommand("device_001", R"({"cmd":"restart"})");
```

### 4. 通过 DeviceManager 管理 MQTT 设备

```cpp
#include "core/DeviceManager.h"

auto deviceManager = std::make_unique<DeviceManager>();
auto mqttChannel = std::make_shared<MqttChannel>();

// 设置 MQTT 通道
deviceManager->setMqttChannel(mqttChannel);

// 连接 MQTT 设备
deviceManager->connectDeviceMqtt("device_001", "broker.emqx.io", 1883);

// 发送命令
deviceManager->sendControlCommandMqtt("device_001", R"({"cmd":"read","sensor":1})");
```

## 依赖安装

### Ubuntu/Debian
```bash
sudo apt install qt6-mqtt-dev
```

### macOS (Homebrew)
```bash
brew install qt@6
```

### Windows (vcpkg)
```bash
vcpkg install qt6-mqtt
```

## 编译说明

```bash
cd software/host
mkdir build && cd build

# 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . -j$(nproc)

# 运行测试
./examples/mqtt_test

# 运行主程序
./heterolink-host
```

## 测试步骤

### 1. 启动公共 Broker 测试
```bash
# 运行测试程序
./examples/mqtt_test
```

预期输出：
```
=== HeteroLink MQTT Test ===
Connecting to broker.emqx.io : 1883
[INFO] MQTT connecting to broker.emqx.io:1883
[INFO] MQTT connected
[EVENT] Connection changed: connected

=== Subscribing to topics ===
Subscribed to: heterolink/subboard/+/status
Subscribed to: heterolink/subboard/test_device/command

=== Publishing test messages ===
Published: heterolink/subboard/test_device/status = online
Published: heterolink/subboard/test_device/telemetry = {"temperature":25.5,"humidity":60}
```

### 2. 使用 MQTTX 验证

1. 下载并安装 [MQTTX](https://mqttx.app/)
2. 创建连接到 `broker.emqx.io:1883`
3. 订阅 `heterolink/subboard/#`
4. 观察测试程序发布的消息

### 3. 使用 mosquitto_sub 验证
```bash
mosquitto_sub -h broker.emqx.io -t "heterolink/subboard/#" -v
```

## 待完成功能

### 短期 (v0.2.0)
- [ ] TLS 加密连接实现
- [ ] MQTT 认证（用户名/密码）
- [ ] 遥测数据 JSON 解析
- [ ] 设备命令处理逻辑

### 中期 (v0.3.0)
- [ ] 客户端证书认证
- [ ] 消息队列管理
- [ ] 离线消息缓存
- [ ] 多 Broker 支持

### 长期 (v1.0.0)
- [ ] MQTT-SN 支持（低功耗设备）
- [ ] 桥接模式
- [ ] 集群管理 UI

## 技术亮点

1. **自动重连**: 指数退避算法，避免频繁重连冲击 Broker
2. **Last Will**: 设备异常断开时自动发布离线消息
3. **信号槽解耦**: MqttChannel 与业务逻辑完全解耦
4. **条件编译**: 支持有/无 Qt MQTT 模块两种编译模式
5. **详细日志**: 所有关键操作都有日志输出，便于调试

## 参考资料

- [Qt MQTT 文档](https://doc.qt.io/qt-6/qtmqtt-index.html)
- [MQTT 协议规范](https://mqtt.org/documentation)
- [EMQX 文档](https://www.emqx.com/zh/docs)
- [ESP-IDF MQTT 示例](https://github.com/espressif/esp-idf/tree/master/examples/protocols/mqtt)

---

**提交哈希**: `8235315`
**提交日期**: 2026-03-21
**作者**: HeteroLink Team
