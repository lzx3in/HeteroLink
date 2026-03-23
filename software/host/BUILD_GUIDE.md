# HeteroLink Host - 跨平台构建指南

**最后更新**: 2026-03-23  
**状态**: ✅ 完成 - 所有平台预设已配置  
**CMake**: 3.21+ | **Qt**: 6.8+ | **vcpkg**: 清单模式

---

## 📋 目录

1. [快速开始](#-快速开始)
2. [平台特定指南](#-平台特定指南)
3. [依赖管理](#-依赖管理)
4. [构建产物](#-构建产物)
5. [运行测试](#-运行测试)
6. [故障排查](#-故障排查)

---

## 🚀 快速开始

### 使用 CMake Presets（推荐）

CMakePresets.json 已配置好各平台工具链，开箱即用。

#### Linux

```bash
cd software/host

# Debug 版本
cmake --preset lnx-gcc-debug
cmake --build build/lnx-gcc-debug -j$(nproc)

# Release 版本
cmake --preset lnx-gcc-release
cmake --build build/lnx-gcc-release -j$(nproc)

# 一键工作流
cmake --workflow --preset workflow-lnx-debug
```

#### Windows (PowerShell)

```powershell
cd software/host

# Visual Studio 2026 Release
cmake --preset win-msvc2026-release
cmake --build build/win-msvc2026-release --config Release

# Visual Studio 2026 Debug
cmake --preset win-msvc2026-debug
cmake --build build/win-msvc2026-debug --config Debug

# MinGW Release
cmake --preset win-gcc-release
cmake --build build/win-gcc-release

# 一键工作流
cmake --workflow --preset workflow-win-msvc2026
```

#### macOS

```bash
cd software/host

# Debug 版本
cmake --preset mac-clang-debug
cmake --build build/mac-clang-debug -j$(sysctl -n hw.ncpu)
```

---

## 🛠️ 平台特定指南

### Windows

#### 方式 1: Visual Studio + Qt 在线安装（推荐）

1. **安装 Visual Studio 2022/2026**
   - 工作负载：「使用 C++ 的桌面开发」
   - 组件：Windows 10/11 SDK、C++ 生成工具

2. **安装 Qt 6.8+**
   - 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
   - 选择组件：
     - ✅ Qt 6.x → MSVC 2022 64-bit
     - ✅ Qt Serial Port
     - ✅ Qt Charts (可选)
     - ✅ Qt Tools → Debugging Tools for Windows

3. **设置环境变量**（PowerShell）

```powershell
# 临时设置（当前会话）
$env:PATH = "C:\Qt\6.10\msvc2022_64\bin;" + $env:PATH
$env:Qt6_DIR = "C:\Qt\6.10\msvc2022_64\lib\cmake\Qt6"

# 永久设置：添加到系统环境变量
```

4. **构建**

```powershell
cmake --preset win-msvc2026-release
cmake --build build/win-msvc2026-release --config Release
```

#### 方式 2: vcpkg 清单模式（自动化依赖）

**前置要求**：
```powershell
# 安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# 设置环境变量
$env:VCPKG_ROOT = "C:\vcpkg"  # 修改为你的路径
```

**构建步骤**：
```powershell
cd E:\Code\HeteroLink\software\host

# 配置（自动安装依赖）
cmake --preset win-msvc2026-release

# 构建
cmake --build build/win-msvc2026-release --config Release

# 运行
.\build\win-msvc2026-release\Release\heterolink-host.exe
```

**vcpkg.json 依赖**：
```json
{
  "dependencies": [
    { "name": "qtbase", "features": ["gui", "widgets", "serialport"] },
    "qtmqtt",
    "qtcharts",
    "fmt",
    "spdlog"
  ]
}
```

#### 方式 3: MSYS2 (MinGW GCC)

```bash
# 安装 MSYS2 后，在 UCRT64 终端执行
pacman -S mingw-w64-ucrt-x86_64-qt6-base \
         mingw-w64-ucrt-x86_64-qt6-serialport \
         mingw-w64-ucrt-x86_64-qt6-charts \
         mingw-w64-ucrt-x86_64-cmake \
         mingw-w64-ucrt-x86_64-gcc

# 构建
cd /c/path/to/HeteroLink/software/host
cmake --preset win-gcc-release
cmake --build build/win-gcc-release
```

---

### Linux

#### Ubuntu/Debian

```bash
# 安装依赖
sudo apt install qt6-base-dev libqt6serialport6-dev libqt6charts6-dev

# MQTT 支持（二选一）
# 选项 A: Qt MQTT（如果可用）
sudo apt install libqt6mqtt6-dev

# 选项 B: Paho MQTT C++
sudo apt install libpaho-mqttpp-dev

# 构建
cd software/host
cmake --preset lnx-gcc-release
cmake --build build/lnx-gcc-release -j$(nproc)
```

#### Arch Linux

```bash
sudo pacman -S qt6-base qt6-serialport qt6-charts

# 构建
cd software/host
cmake --preset lnx-gcc-release
cmake --build build/lnx-gcc-release -j$(nproc)
```

#### 在 ESP-IDF 环境中构建

如果在 ESP-IDF 环境中构建，需显式指定系统编译器：

```bash
cd software/host
rm -rf build
mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_CXX_FLAGS="-m64"

cmake --build . -j$(nproc)
```

---

### macOS

```bash
# 安装依赖（Homebrew）
brew install qt@6

# 设置环境变量
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
export Qt6_DIR="/opt/homebrew/opt/qt@6/lib/cmake/Qt6"

# 构建
cd software/host
cmake --preset mac-clang-debug
cmake --build build/mac-clang-debug -j$(sysctl -n hw.ncpu)
```

---

## 📦 依赖管理

### 依赖清单

| 依赖 | 必需 | 用途 |
|------|------|------|
| Qt6 Core | ✅ | 核心框架 |
| Qt6 Gui | ✅ | GUI 渲染 |
| Qt6 Widgets | ✅ | UI 组件 |
| Qt6 SerialPort | ✅ | 串口通信 |
| Qt6 Charts | ❌ | 数据可视化（可选） |
| Qt6 Mqtt | ❌ | MQTT 远端通信（可选） |
| fmt | ✅ | 格式化库 |
| spdlog | ✅ | 日志库 |

### vcpkg 清单模式

项目使用 **vcpkg 清单模式 (manifest mode)**，依赖在 `vcpkg.json` 中声明。

**优势**：
- ✅ 无需手动运行 `vcpkg install`
- ✅ CMake 配置时自动安装依赖
- ✅ 依赖安装到 `build/<preset>/vcpkg_installed`，不污染系统
- ✅ 版本锁定，构建可复现

**清理重建**：
```powershell
# 删除构建目录
Remove-Item -Recurse -Force build

# 重新配置和构建
cmake --preset win-msvc2026-release
cmake --build build/win-msvc2026-release --config Release
```

---

## 📁 构建产物

### Linux

```
build/lnx-gcc-release/
├── heterolink-host          # 主程序
├── tests/
│   ├── test_protocol        # 协议测试
│   └── test_dataprocessor   # 数据处理器测试
├── compile_commands.json    # 编译命令（IDE 使用）
└── CMakeCache.txt           # CMake 缓存
```

### Windows

```
build\win-msvc2026-release\
├── Release\
│   ├── heterolink-host.exe  # 主程序 (约 289KB)
│   └── *.dll                # Qt 运行时（部署后）
├── Debug\
│   └── heterolink-host.exe
└── tests\Release\
    ├── test_protocol.exe
    └── test_dataprocessor.exe
```

### macOS

```
build/mac-clang-debug/
├── heterolink-host          # 主程序
└── tests/
    ├── test_protocol
    └── test_dataprocessor
```

---

## 🧪 运行测试

```bash
# Linux/macOS
cd build/lnx-gcc-release
ctest --output-on-failure

# Windows (PowerShell)
cd build\win-msvc2026-release
ctest -C Release --output-on-failure
```

**运行单个测试**：
```bash
# Linux
./tests/test_protocol
./tests/test_dataprocessor

# Windows
.\tests\Release\test_protocol.exe
.\tests\Release\test_dataprocessor.exe
```

---

## ▶️ 运行程序

### Linux

```bash
# 本地桌面
./build/lnx-gcc-release/heterolink-host

# 无头模式（服务器）
QT_QPA_PLATFORM=offscreen ./build/lnx-gcc-release/heterolink-host
```

### Windows

```powershell
# 方式 1: 直接运行
.\build\win-msvc2026-release\Release\heterolink-host.exe

# 方式 2: 部署后运行（复制 Qt DLL 后）
cd build\win-msvc2026-release\Release
windeployqt heterolink-host.exe
.\heterolink-host.exe
```

### macOS

```bash
./build/mac-clang-debug/heterolink-host
```

---

## 🔧 故障排查

### Windows 相关

#### Q1: CMake 找不到 Visual Studio

**错误**: `Could NOT find VisualStudio`

**解决**:
1. 安装 [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/)
2. 确保勾选「C++ 生成工具」和「Windows 10/11 SDK」
3. 使用「Developer Command Prompt for VS 2026」运行命令

#### Q2: CMake 找不到 Qt6

**错误**: `Could NOT find Qt6`

**解决**:
```powershell
# 方法 1: 设置 Qt6_DIR
$env:Qt6_DIR = "C:\Qt\6.10\msvc2022_64\lib\cmake\Qt6"

# 方法 2: 添加到 PATH
$env:PATH = "C:\Qt\6.10\msvc2022_64\bin;" + $env:PATH

# 方法 3: 显式指定 CMAKE_PREFIX_PATH
cmake --preset win-msvc2026-release -DCMAKE_PREFIX_PATH="C:\Qt\6.10\msvc2022_64"
```

#### Q3: 链接错误 - 找不到 Qt 库

**错误**: `LINK : fatal error LNK1181: cannot open input file 'Qt6Core.lib'`

**解决**:
1. 确保 Qt 安装时选择了 MSVC 版本（不是 MinGW）
2. 检查 Qt 路径是否正确
3. 在 Visual Studio Developer Command Prompt 中构建

#### Q4: 运行时提示缺少 DLL

**错误**: `无法启动程序，因为计算机中丢失 Qt6Core.dll`

**解决**:
```powershell
# 方法 1: 使用 windeployqt 部署
cd build\win-msvc2026-release\Release
windeployqt heterolink-host.exe

# 方法 2: 将 Qt bin 目录添加到 PATH
$env:PATH = "C:\Qt\6.10\msvc2022_64\bin;" + $env:PATH
```

#### Q5: 串口无法打开（权限问题）

**解决**:
1. 以管理员身份运行程序
2. 检查设备管理器中 COM 端口号
3. 确保没有其他程序占用该串口

---

### Linux 相关

#### Q1: CMake 找不到 Qt6

```bash
# 查找 Qt6 路径
pkg-config --modversion Qt6Core

# 显式指定 Qt6 路径
cmake --preset lnx-gcc-release -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
```

#### Q2: 编译时出现 ESP-IDF 工具链错误

**错误示例**: `as: unrecognized option '--64'`

**原因**: ESP-IDF 的 `export.sh` 修改了 PATH，导致 CMake 使用了 ESP32 工具链。

**解决**:
1. 使用 CMake Presets（已配置系统编译器）
2. 或退出 ESP-IDF 环境后重新构建
3. 或显式指定 `CMAKE_CXX_COMPILER=g++`

#### Q3: 运行时提示找不到 xcb 平台插件

**原因**: WSL 或无图形界面环境无法创建 GUI 窗口。

**解决**:
- 在本地 Linux 桌面运行
- 或使用 offscreen 模式：`QT_QPA_PLATFORM=offscreen ./heterolink-host`
- 或使用 X11 转发：`export DISPLAY=:0`

#### Q4: 串口权限不足

```bash
# 将用户添加到 dialout 组
sudo usermod -a -G dialout $USER

# 重新登录或重启
```

---

### macOS 相关

#### Q1: CMake 找不到 Qt6 (Homebrew)

```bash
# 添加 Homebrew Qt 到 PATH
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"

# 设置 Qt6_DIR
export Qt6_DIR="/opt/homebrew/opt/qt@6/lib/cmake/Qt6"
```

---

## 📊 CMake Presets 参考

### Configure Presets

| Preset | 平台 | 编译器 | 类型 |
|--------|------|--------|------|
| `lnx-gcc-release` | Linux | GCC | Release |
| `lnx-gcc-debug` | Linux | GCC | Debug |
| `win-msvc2026-release` | Windows | MSVC 2026 | Release |
| `win-msvc2026-debug` | Windows | MSVC 2026 | Debug |
| `win-msvc2019-release` | Windows | MSVC 2019 | Release |
| `win-gcc-release` | Windows | MinGW GCC | Release |
| `win-gcc-debug` | Windows | MinGW GCC | Debug |
| `mac-clang-debug` | macOS | Clang | Debug |

### Workflow Presets

| Preset | 描述 |
|--------|------|
| `workflow-lnx-debug` | Linux Debug 完整流程（配置→构建→测试） |
| `workflow-lnx-release` | Linux Release 完整流程（配置→构建） |
| `workflow-win-msvc2026` | Windows MSVC 2026 完整流程 |
| `workflow-win-gcc` | Windows MinGW 完整流程 |

---

## 🎯 下一步

1. ✅ 编译成功
2. ⏳ 集成 Qt Charts（可选，用于波形显示）
3. ⏳ 实现 MQTT 通道（需要安装 MQTT 库）
4. ⏳ 实际硬件联调

---

## 📚 相关文档

- [README.md](README.md) - 功能概述
- [DESIGN_SUMMARY.md](DESIGN_SUMMARY.md) - 设计总结
- [MQTT_IMPLEMENTATION.md](MQTT_IMPLEMENTATION.md) - MQTT 实现细节
- [../../docs/software/protocol-spec.md](../../docs/software/protocol-spec.md) - 协议规范
- [../../docs/software/mqtt-guide.md](../../docs/software/mqtt-guide.md) - MQTT 配置指南

---

**构建愉快！** 🚀
