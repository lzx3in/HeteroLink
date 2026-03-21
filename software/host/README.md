# HeteroLink Host - 上位机软件

基于 Qt/C++ 开发的跨平台异构协处理器监控平台上位机。

## 功能特性

- **设备管理**: 支持多设备连接、状态监控
- **数据可视化**: 实时波形显示、数据表格
- **通信协议**: 
  - **UART 自定义二进制协议**（近端通信）
    - 高速双向数据传输
    - kHz 级遥测数据上传
    - μs 级指令响应
  - **MQTT over WiFi**（远端通信）
    - 设备集群管理
    - 云端连接
    - Last Will 遗嘱消息
    - 自动重连机制
    - TLS 加密支持（待实现）
- **告警系统**: 阈值监控、告警记录
- **数据记录**: CSV/JSON 格式导出
- **配置管理**: JSON 配置文件

## 系统要求

- **操作系统**: Windows 10+, macOS 10.15+, Linux
- **编译器**: GCC 9+, Clang 10+, MSVC 2019+
- **Qt 版本**: Qt 6.2+
- **CMake**: 3.20+

## 依赖项

### Qt 模块
- Qt6 Core
- Qt6 Gui
- Qt6 Widgets
- Qt6 SerialPort
- Qt6 Charts (可选，用于数据可视化)

### 可选依赖
- **Qt MQTT** (用于 MQTT 远端通信)
  - Qt6: `Qt6::Mqtt` 模块（Qt 6.2+ 自带）
  - Qt5: 需单独安装 [QtMQTT](https://github.com/qt/qtmqtt)
  
安装 Qt MQTT（Qt6）:
```bash
# Ubuntu/Debian
sudo apt install qt6-mqtt-dev

# macOS (Homebrew)
brew install qt@6  # 包含 MQTT 模块

# Windows (vcpkg)
vcpkg install qt6-mqtt
```

## 构建说明

### 1. 配置 Qt 环境

```bash
# Linux/macOS
export Qt6_DIR=/path/to/Qt/6.x/gcc_64/lib/cmake/Qt6

# Windows (PowerShell)
$env:Qt6_DIR = "C:\Qt\6.x\msvc2019_64\lib\cmake\Qt6"
```

### 2. 创建构建目录

```bash
mkdir build && cd build
```

### 3. 运行 CMake

```bash
# Linux/macOS
cmake .. -DCMAKE_BUILD_TYPE=Release

# Windows
cmake .. -G "Visual Studio 16 2019" -A x64
```

### 4. 编译

```bash
# Linux/macOS
cmake --build . -j$(nproc)

# Windows
cmake --build . --config Release
```

### 5. 运行

```bash
# Linux/macOS
./heterolink-host

# Windows
.\Release\heterolink-host.exe
```

## 命令行参数

```
-c, --config <file>    使用指定的配置文件
-v, --verbose          启用详细日志输出
--auto-connect         启动时自动连接设备
-h, --help             显示帮助信息
--version              显示版本信息
```

## 项目结构

```
software/host/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 本文件
├── src/
│   ├── main.cpp            # 程序入口
│   ├── app/
│   │   ├── Application.h   # 应用主类
│   │   └── Application.cpp
│   ├── core/
│   │   ├── DeviceManager.h # 设备管理器
│   │   ├── DeviceManager.cpp
│   │   ├── DataProcessor.h # 数据处理器
│   │   ├── DataProcessor.cpp
│   │   └── AlarmSystem.h   # 告警系统
│   │   └── AlarmSystem.cpp
│   ├── protocol/
│   │   ├── Protocol.h      # 协议定义
│   │   ├── Protocol.cpp
│   │   ├── UartChannel.h   # UART 通道
│   │   ├── UartChannel.cpp
│   │   ├── MqttChannel.h   # MQTT 通道
│   │   └── MqttChannel.cpp
│   ├── ui/
│   │   ├── MainWindow.h    # 主窗口
│   │   ├── MainWindow.cpp
│   │   ├── MainWindow.ui
│   │   ├── DevicePanel.h   # 设备面板
│   │   ├── DevicePanel.cpp
│   │   ├── DevicePanel.ui
│   │   ├── DataWidget.h    # 数据可视化
│   │   ├── DataWidget.cpp
│   │   ├── DataWidget.ui
│   │   ├── ConfigPanel.h   # 配置面板
│   │   ├── ConfigPanel.cpp
│   │   └── ConfigPanel.ui
│   ├── storage/
│   │   ├── ConfigManager.h # 配置管理
│   │   ├── ConfigManager.cpp
│   │   ├── DataLogger.h    # 数据记录
│   │   └── DataLogger.cpp
│   └── utils/
│       ├── Logger.h        # 日志工具
│       └── Logger.cpp
├── resources/
│   ├── resources.qrc       # Qt 资源文件
│   ├── styles/
│   │   └── dark.qss        # 深色主题样式
│   └── icons/              # 图标资源
└── tests/
    ├── CMakeLists.txt
    ├── test_protocol.cpp   # 协议测试
    └── test_dataprocessor.cpp  # 数据处理器测试
```

## 配置文件

配置文件默认位置：
- **Linux**: `~/.config/HeteroLink/config.json`
- **macOS**: `~/Library/Application Support/HeteroLink/config.json`
- **Windows**: `%APPDATA%\HeteroLink\config.json`

### 配置示例

```json
{
  "uart": {
    "portName": "COM3",
    "baudRate": 921600
  },
  "mqtt": {
    "brokerHost": "localhost",
    "brokerPort": 1883,
    "username": "",
    "password": "",
    "clientId": "heterolink-host"
  },
  "data": {
    "bufferSize": 10000,
    "autoExport": false,
    "exportPath": "~/heterolink_data"
  },
  "alarm": {
    "enabled": true,
    "lowerLimit": -1000,
    "upperLimit": 1000,
    "level": "warning"
  },
  "ui": {
    "theme": "dark",
    "language": "zh-CN"
  }
}
```

## 通信协议

详见 [协议规范文档](../../docs/software/protocol-spec.md)

## 开发指南

### 添加新功能

1. 在相应模块创建头文件和实现文件
2. 更新 `CMakeLists.txt` 添加源文件
3. 在 `Application` 类中注册新组件
4. 编写单元测试

### 代码风格

- 遵循 Qt 命名约定
- 使用智能指针管理资源
- 信号槽连接使用 lambda 表达式
- 所有日志使用 `LOG_*` 宏

### 测试

```bash
cd build
ctest --output-on-failure
```

## 故障排查

### 常见问题

**Q: 编译时找不到 Qt6**
- 确保 `Qt6_DIR` 环境变量正确设置
- 检查 Qt 安装路径是否包含 CMake 配置文件

**Q: 串口无法打开**
- 检查串口权限（Linux 需要添加到 `dialout` 组）
- 确认串口未被其他程序占用

**Q: MQTT 连接失败**
- 检查 Broker 地址和端口
- 确认防火墙允许 MQTT 流量（默认 1883）
- 检查是否安装了 Qt MQTT 模块
- 查看日志输出确认错误信息

**Q: 编译时提示 HAS_QT_MQTT 未定义**
- 确认已安装 Qt MQTT 模块
- 重新运行 CMake 配置
- 检查 CMake 输出中是否显示 "Qt MQTT module found"

## 许可证

MIT License

## 联系方式

- 项目主页：https://github.com/heterolink/heterolink
- 问题反馈：https://github.com/heterolink/heterolink/issues
