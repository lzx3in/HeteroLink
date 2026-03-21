# HeteroLink 上位机构建指南

## 🚀 快速构建

### 使用 CMake Presets（推荐）

CMakePresets.json 已配置好工具链，避免 ESP-IDF 环境干扰。

```bash
cd software/host

# Debug 版本
cmake --preset linux-debug
cmake --build build/linux-debug -j$(nproc)

# Release 版本
cmake --preset linux-release
cmake --build build/linux-release -j$(nproc)
```

### 运行测试

```bash
cd build/linux-debug
ctest --output-on-failure
```

### 运行程序

需要图形界面环境：

```bash
# 本地 Linux 桌面
./build/linux-debug/heterolink-host

# 或使用 Qt 平台插件
QT_QPA_PLATFORM=offscreen ./build/linux-debug/heterolink-host  # 无头模式
```

---

## 🛠️ 手动构建（不使用 Presets）

如果在 ESP-IDF 环境中构建，需显式指定系统编译器：

```bash
cd software/host
rm -rf build
mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_CXX_FLAGS="-m64"

cmake --build . -j$(nproc)
```

---

## 📦 依赖项

### Ubuntu/Debian

```bash
# Qt6 核心
sudo apt install qt6-base-dev libqt6serialport6-dev

# 可选：Qt Charts（数据可视化）
sudo apt install libqt6charts6-dev

# MQTT 支持（二选一）
# 选项 A: Qt MQTT（如果可用）
sudo apt install libqt6mqtt6-dev

# 选项 B: Paho MQTT C++
sudo apt install libpaho-mqttpp-dev
```

### Arch Linux

```bash
sudo pacman -S qt6-base qt6-serialport qt6-charts
```

### macOS

```bash
brew install qt6 paho-mqtt-cpp
```

---

## 🔧 常见问题

### Q1: CMake 找不到 Qt6

```bash
# 查找 Qt6 路径
pkg-config --modversion Qt6Core

# 显式指定 Qt6 路径
cmake --preset linux-debug -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
```

### Q2: 编译时出现 ESP-IDF 工具链错误

**错误示例**: `as: unrecognized option '--64'`

**原因**: ESP-IDF 的 export.sh 修改了 PATH，导致 CMake 使用了 ESP32 工具链。

**解决方法**:
1. 使用 CMake Presets（已配置系统编译器）
2. 或退出 ESP-IDF 环境后重新构建
3. 或显式指定 `CMAKE_CXX_COMPILER=g++`

### Q3: 运行时提示找不到 xcb 平台插件

**原因**: WSL 或无图形界面环境无法创建 GUI 窗口。

**解决方法**:
- 在本地 Linux 桌面运行
- 或使用 offscreen 模式：`QT_QPA_PLATFORM=offscreen ./heterolink-host`
- 或使用 X11 转发：`export DISPLAY=:0`

### Q4: 串口权限不足（Linux）

```bash
# 将用户添加到 dialout 组
sudo usermod -a -G dialout $USER

# 重新登录或重启
```

---

## 📁 构建产物

```
build/linux-debug/
├── heterolink-host          # 主程序（Debug 版）
├── tests/
│   ├── test_protocol        # 协议测试
│   └── test_dataprocessor   # 数据处理器测试
├── compile_commands.json    # 编译命令（IDE 使用）
└── CMakeCache.txt           # CMake 缓存
```

---

## 🎯 下一步

1. ✅ 编译成功
2. ⏳ 集成 Qt Charts（可选，用于波形显示）
3. ⏳ 实现 MQTT 通道（需要安装 MQTT 库）
4. ⏳ 实际硬件联调

---

**最后更新**: 2026-03-21  
**状态**: ✅ 编译通过，测试通过
