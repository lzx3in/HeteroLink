# HeteroLink 上位机构建指南

**支持平台**: Windows 10/11, Linux (Ubuntu/Debian/Arch), macOS 10.15+

---

## 🚀 快速构建

### 使用 CMake Presets（推荐）

CMakePresets.json 已配置好各平台工具链，开箱即用。

#### Linux

```bash
cd software/host

# Debug 版本
cmake --preset linux-debug
cmake --build build/linux-debug -j$(nproc)

# Release 版本
cmake --preset linux-release
cmake --build build/linux-release -j$(nproc)

# 一键工作流
cmake --workflow --preset linux-debug-workflow
```

#### Windows (PowerShell)

```powershell
cd software/host

# Visual Studio 2022 Release
cmake --preset windows-msvc-2022
cmake --build build/windows-msvc-2022 --config Release

# Visual Studio 2022 Debug
cmake --preset windows-msvc-2022-debug
cmake --build build/windows-msvc-2022-debug --config Debug

# MinGW Release
cmake --preset windows-mingw
cmake --build build/windows-mingw

# 一键工作流
cmake --workflow --preset windows-msvc-2022-workflow
cmake --workflow --preset windows-mingw-workflow
```

#### macOS

```bash
cd software/host

# Debug 版本
cmake --preset macos-clang
cmake --build build/macos-clang -j$(sysctl -n hw.ncpu)
```

---

### 运行测试

```bash
# Linux/macOS
cd build/linux-debug
ctest --output-on-failure

# Windows (PowerShell)
cd build\windows-msvc-2022
ctest -C Release --output-on-failure
```

### 运行程序

#### Linux

```bash
# 本地桌面
./build/linux-debug/heterolink-host

# 无头模式（服务器）
QT_QPA_PLATFORM=offscreen ./build/linux-debug/heterolink-host
```

#### Windows

```powershell
# 直接运行
.\build\windows-msvc-2022\Release\heterolink-host.exe

# 或使用 start
start .\build\windows-msvc-2022\Release\heterolink-host.exe
```

#### macOS

```bash
./build/macos-clang/heterolink-host
```

---

## 🛠️ 手动构建（不使用 Presets）

### Linux

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

### Windows (Visual Studio Developer Command Prompt)

```cmd
cd software/host
mkdir build && cd build

cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

---

## 📦 依赖项

### Windows

#### 方式 1: 使用 Qt 在线安装程序（推荐）

1. 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
2. 安装 Qt 6.8+，选择组件：
   - ✅ Qt 6.x → MSVC 2022 64-bit
   - ✅ Qt Serial Port
   - ✅ Qt Charts (可选)
   - ✅ Qt Tools → Debugging Tools for Windows

3. 设置环境变量（PowerShell）：

```powershell
# 添加到 $PROFILE 或手动设置
$env:PATH = "C:\Qt\6.8\msvc2022_64\bin;" + $env:PATH
$env:Qt6_DIR = "C:\Qt\6.8\msvc2022_64\lib\cmake\Qt6"
```

#### 方式 2: 使用 vcpkg

```powershell
# 安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# 安装 Qt 和相关库
.\vcpkg\vcpkg install qt6-base:x64-windows qt6-serialport:x64-windows

# 使用 vcpkg 工具链
cmake --preset windows-msvc-2022 `
  -DCMAKE_TOOLCHAIN_FILE=[vcpkg 路径]/scripts/buildsystems/vcpkg.cmake
```

#### 方式 3: 使用 MSYS2 (MinGW)

```bash
# 安装 MSYS2 后，在 MSYS2 UCRT64 终端执行
pacman -S mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-serialport mingw-w64-ucrt-x86_64-qt6-charts mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc

# 构建
cd /c/path/to/HeteroLink/software/host
cmake --preset windows-mingw
cmake --build build/windows-mingw
```

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

### Windows 相关

#### Q1: CMake 找不到 Visual Studio

**错误**: `Could NOT find VisualStudio`

**解决**:
1. 安装 [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/)
2. 确保勾选 "C++ 生成工具" 和 "Windows 10/11 SDK"
3. 使用 "Developer Command Prompt for VS 2022" 运行命令

#### Q2: CMake 找不到 Qt6

**错误**: `Could NOT find Qt6`

**解决**:

```powershell
# 方法 1: 设置 Qt6_DIR
$env:Qt6_DIR = "C:\Qt\6.8\msvc2022_64\lib\cmake\Qt6"

# 方法 2: 添加到 PATH
$env:PATH = "C:\Qt\6.8\msvc2022_64\bin;" + $env:PATH

# 方法 3: 显式指定 CMAKE_PREFIX_PATH
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.8\msvc2022_64"
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
cd build\windows-msvc-2022\Release
windeployqt heterolink-host.exe

# 方法 2: 将 Qt bin 目录添加到 PATH
$env:PATH = "C:\Qt\6.8\msvc2022_64\bin;" + $env:PATH
```

#### Q5: 串口无法打开（权限问题）

**解决**:
1. 以管理员身份运行程序
2. 检查设备管理器中 COM 端口号
3. 确保没有其他程序占用该串口

### Linux 相关

#### Q1: CMake 找不到 Qt6

```bash
# 查找 Qt6 路径
pkg-config --modversion Qt6Core

# 显式指定 Qt6 路径
cmake --preset linux-debug -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
```

#### Q2: 编译时出现 ESP-IDF 工具链错误

**错误示例**: `as: unrecognized option '--64'`

**原因**: ESP-IDF 的 export.sh 修改了 PATH，导致 CMake 使用了 ESP32 工具链。

**解决方法**:
1. 使用 CMake Presets（已配置系统编译器）
2. 或退出 ESP-IDF 环境后重新构建
3. 或显式指定 `CMAKE_CXX_COMPILER=g++`

#### Q3: 运行时提示找不到 xcb 平台插件

**原因**: WSL 或无图形界面环境无法创建 GUI 窗口。

**解决方法**:
- 在本地 Linux 桌面运行
- 或使用 offscreen 模式：`QT_QPA_PLATFORM=offscreen ./heterolink-host`
- 或使用 X11 转发：`export DISPLAY=:0`

#### Q4: 串口权限不足（Linux）

```bash
# 将用户添加到 dialout 组
sudo usermod -a -G dialout $USER

# 重新登录或重启
```

### macOS 相关

#### Q1: CMake 找不到 Qt6 (Homebrew)

```bash
# 添加 Homebrew Qt 到 PATH
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"

# 设置 Qt6_DIR
export Qt6_DIR="/opt/homebrew/opt/qt@6/lib/cmake/Qt6"
```

---

## 📁 构建产物

### Linux

```
build/linux-debug/
├── heterolink-host          # 主程序（Debug 版）
├── tests/
│   ├── test_protocol        # 协议测试
│   └── test_dataprocessor   # 数据处理器测试
├── compile_commands.json    # 编译命令（IDE 使用）
└── CMakeCache.txt           # CMake 缓存
```

### Windows

```
build\windows-msvc-2022\
├── Release\
│   ├── heterolink-host.exe
│   └── *.dll               # Qt 运行时
├── Debug\
│   └── heterolink-host.exe
└── tests\
    └── Release\
        ├── test_protocol.exe
        └── test_dataprocessor.exe
```

---

## 🎯 下一步

1. ✅ 编译成功
2. ⏳ 集成 Qt Charts（可选，用于波形显示）
3. ⏳ 实现 MQTT 通道（需要安装 MQTT 库）
4. ⏳ 实际硬件联调

---

## 📋 预设列表

| 预设名称 | 平台 | 编译器 | 类型 |
|---------|------|--------|------|
| `linux-debug` | Linux | GCC | Debug |
| `linux-release` | Linux | GCC | Release |
| `windows-msvc-2022` | Windows | MSVC 2022 | Release |
| `windows-msvc-2022-debug` | Windows | MSVC 2022 | Debug |
| `windows-msvc-2019` | Windows | MSVC 2019 | Release |
| `windows-mingw` | Windows | MinGW GCC | Release |
| `windows-mingw-debug` | Windows | MinGW GCC | Debug |
| `macos-clang` | macOS | Clang | Debug |

---

**最后更新**: 2026-03-21  
**状态**: ✅ 所有平台预设已配置
