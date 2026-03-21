# HeteroLink 固件配置指南

编译和配置 ESP32 固件。

---

## 🛠️ 配置方法

### 方法一：menuconfig (推荐)

```bash
cd firmware/esp32/subboard

# 打开配置菜单
idf.py menuconfig
```

### 方法二：修改 sdkconfig.defaults

```bash
# 编辑默认配置
vim sdkconfig.defaults
```

### 方法三：直接编辑 sdkconfig

⚠️ **不推荐**: 手动编辑 sdkconfig 可能导致配置冲突

---

## 📋 配置项详解

### WiFi 配置

```
HeteroLink Configuration →
    WiFi Configuration →
        (string) WiFi SSID
            默认值：""
            说明：WiFi 网络名称
            
        (string) WiFi Password
            默认值：""
            说明：WiFi 密码
            
        (bool) WiFi Auto Reconnect
            默认值：Y
            说明：断线自动重连
```

### MQTT 配置

```
HeteroLink Configuration →
    MQTT Configuration →
        (string) MQTT Broker URI
            默认值："mqtt://broker.emqx.io"
            说明：MQTT Broker 地址
            
        (int) MQTT Broker Port
            默认值：1883
            说明：MQTT Broker 端口
            
        (string) MQTT Client ID
            默认值："heterolink_%CHIPID%"
            说明：客户端 ID (%CHIPID% 自动替换为芯片 ID)
            
        (string) MQTT Username (optional)
            默认值：""
            说明：用户名 (如需要认证)
            
        (string) MQTT Password (optional)
            默认值：""
            说明：密码 (如需要认证)
```

### 设备配置

```
HeteroLink Configuration →
    Device Configuration →
        (string) Device ID
            默认值："subboard_001"
            说明：设备唯一标识
            
        (int) Data Upload Interval (ms)
            默认值：1000
            说明：数据上报间隔 (毫秒)
            
        (bool) Enable UART Protocol
            默认值：Y
            说明：启用 UART 近端通信
            
        (bool) Enable SPI DMA
            默认值：Y
            说明：启用 SPI+DMA 高速传输
```

### 调试配置

```
HeteroLink Configuration →
    Debug Configuration →
        (choice) Log Level
            选项：None / Error / Warning / Info / Debug / Verbose
            默认值：Info
            说明：日志级别
            
        (bool) Enable Debug Output
            默认值：Y
            说明：启用调试输出
```

---

## 📝 sdkconfig.defaults 示例

### 基础配置

```ini
# WiFi 配置
CONFIG_HETERO_WIFI_SSID="MyHomeWiFi"
CONFIG_HETERO_WIFI_PASSWORD="MyPassword123"
CONFIG_HETERO_WIFI_AUTO_RECONNECT=y

# MQTT 配置
CONFIG_HETERO_MQTT_BROKER_URI="mqtt://broker.emqx.io"
CONFIG_HETERO_MQTT_BROKER_PORT=1883
CONFIG_HETERO_MQTT_CLIENT_ID="heterolink_%CHIPID%"

# 设备配置
CONFIG_HETERO_DEVICE_ID="subboard_001"
CONFIG_HETERO_DATA_UPLOAD_INTERVAL=1000
CONFIG_HETERO_ENABLE_UART_PROTOCOL=y
CONFIG_HETERO_ENABLE_SPI_DMA=y

# 调试配置
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_HETERO_ENABLE_DEBUG_OUTPUT=y
```

### 生产环境配置

```ini
# WiFi 配置
CONFIG_HETERO_WIFI_SSID="ProductionWiFi"
CONFIG_HETERO_WIFI_PASSWORD="SecurePassword"
CONFIG_HETERO_WIFI_AUTO_RECONNECT=y

# MQTT 配置 (使用 TLS)
CONFIG_HETERO_MQTT_BROKER_URI="mqtts://mqtt.production.com"
CONFIG_HETERO_MQTT_BROKER_PORT=8883
CONFIG_HETERO_MQTT_USE_TLS=y
CONFIG_HETERO_MQTT_TLS_AUTH=y

# 设备配置
CONFIG_HETERO_DEVICE_ID="prod_subboard_001"
CONFIG_HETERO_DATA_UPLOAD_INTERVAL=5000

# 调试配置 (关闭调试输出)
CONFIG_LOG_DEFAULT_LEVEL_WARN=y
CONFIG_HETERO_ENABLE_DEBUG_OUTPUT=n
```

### 开发环境配置

```ini
# WiFi 配置
CONFIG_HETERO_WIFI_SSID="DevWiFi"
CONFIG_HETERO_WIFI_PASSWORD="DevPassword"

# MQTT 配置 (本地 Broker)
CONFIG_HETERO_MQTT_BROKER_URI="mqtt://192.168.1.100"
CONFIG_HETERO_MQTT_BROKER_PORT=1883

# 设备配置
CONFIG_HETERO_DEVICE_ID="dev_subboard_001"
CONFIG_HETERO_DATA_UPLOAD_INTERVAL=100

# 调试配置 (详细日志)
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_HETERO_ENABLE_DEBUG_OUTPUT=y
CONFIG_HETERO_ENABLE_UART_LOG=y
```

---

## 🔧 编译选项

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

### 编译命令

```bash
# 完整编译
idf.py build

# 仅编译特定组件
idf.py build mqtt_client

# 清理并重新编译
idf.py fullclean
idf.py build

# 编译并烧录
idf.py flash

# 编译、烧录并监视
idf.py flash monitor
```

---

## 📊 编译优化

### 优化代码大小

```ini
# sdkconfig.defaults
CONFIG_COMPILER_OPTIMIZATION_SIZE=y
CONFIG_LOG_DEFAULT_LEVEL_WARN=y
```

### 优化性能

```ini
# sdkconfig.defaults
CONFIG_COMPILER_OPTIMIZATION_PERF=y
CONFIG_FREERTOS_HZ=1000
```

### 启用 PSRAM (如适用)

```ini
# sdkconfig.defaults
CONFIG_SPIRAM_SUPPORT=y
CONFIG_SPIRAM_USE_CAPS_ALLOC=y
```

---

## 🧪 测试配置

### 创建测试配置

```bash
# 复制默认配置
cp sdkconfig.defaults sdkconfig.defaults.test

# 编辑测试配置
vim sdkconfig.defaults.test
```

### 测试配置示例

```ini
# 测试模式
CONFIG_HETERO_TEST_MODE=y
CONFIG_HETERO_ENABLE_ALL_FEATURES=y
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_HETERO_ENABLE_DEBUG_OUTPUT=y
```

---

## 🔄 配置版本控制

### 最佳实践

1. **提交 sdkconfig.defaults**
   - 包含合理的默认值
   - 便于团队协作

2. **不提交 sdkconfig**
   - 已添加到 .gitignore
   - 避免配置冲突

3. **使用配置模板**
   ```bash
   # 创建配置模板
   cp sdkconfig.defaults sdkconfig.defaults.template
   
   # 团队共享模板
   git add sdkconfig.defaults.template
   ```

---

## 🆘 常见问题

### 1. 配置不生效

**问题**: 修改配置后编译仍使用旧配置

**解决**:
```bash
# 清理配置
idf.py fullclean

# 重新配置
idf.py menuconfig

# 重新编译
idf.py build
```

### 2. 配置冲突

**问题**: menuconfig 提示配置冲突

**解决**:
```bash
# 重置配置
rm sdkconfig
idf.py menuconfig
```

### 3. 找不到配置项

**问题**: menuconfig 中找不到某个配置项

**原因**: 配置项依赖其他配置

**解决**:
- 检查配置项的依赖关系
- 先启用父级配置

---

## 📚 参考资源

- [ESP-IDF 配置系统](https://docs.espressif.com/projects/esp-idf/en/v5.3/api-guides/kconfig.html)
- [Kconfig 语法](https://docs.espressif.com/projects/esp-idf/en/v5.3/api-guides/kconfig.html#kconfig-syntax)
- [HeteroLink 架构](architecture.md)

---

**最后更新**: 2026-03-21
