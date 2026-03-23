# HeteroLink Host - 构建指南

**最后更新**: 2026-03-23  
**状态**: ✅ 完成  
**构建系统**: CMake + vcpkg 清单模式（唯一支持方式）  
**CMake**: 3.21+ | **Qt**: 6.8+ via vcpkg

---

## ⚠️ 重要说明

**本项目仅支持 vcpkg 清单模式构建**，不使用系统安装的 Qt 或其他依赖。

**为什么只用 vcpkg？**
- ✅ 构建可复现 - 依赖版本锁定
- ✅ 跨平台一致 - Windows/Linux/macOS 使用相同依赖
- ✅ 自动化管理 - 无需手动安装 Qt
- ✅ 避免冲突 - 不依赖系统 Qt，避免版本不兼容

---

## 📋 前置条件

### 1. 安装 vcpkg

```bash
# 克隆 vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Windows
.\bootstrap-vcpkg.bat

# Linux/macOS
./bootstrap-vcpkg.sh
```

### 2. 设置环境变量

**Windows (PowerShell)**:
```powershell
# 临时设置（当前会话）
$env:VCPKG_ROOT = "C:\vcpkg"  # 修改为你的路径

# 永久设置：添加到系统环境变量
# 或使用 setx
setx VCPKG_ROOT "C:\vcpkg"
```

**Linux/macOS (Bash)**:
```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
export VCPKG_ROOT="$HOME/vcpkg"
export PATH="$VCPKG_ROOT:$PATH"
```

### 3. 验证安装

```bash
vcpkg --version
# 应输出版本号
```

---

## 🚀 快速构建

### Windows (PowerShell)

```powershell
cd E:\Code\HeteroLink\software\host

# 配置（自动安装依赖）
cmake --preset win-msvc2026-release

# 构建
cmake --build build/win-msvc2026-release --config Release

# 运行
.\build\win-msvc2026-release\Release\heterolink-host.exe
```

### Linux

```bash
cd /path/to/HeteroLink/software/host

# 配置
cmake --preset lnx-gcc-release

# 构建
cmake --build build/lnx-gcc-release -j$(nproc)

# 运行
./build/lnx-gcc-release/heterolink-host
```

### macOS

```bash
cd /path/to/HeteroLink/software/host

# 配置
cmake --preset mac-clang-debug

# 构建
cmake --build build/mac-clang-debug -j$(sysctl -n hw.ncpu)

# 运行
./build/mac-clang-debug/heterolink-host
```

---

## 📦 依赖管理

### vcpkg 清单模式

项目使用 **vcpkg manifest mode**，依赖在 `vcpkg.json` 中声明：

```json
{
  "name": "heterolink-host",
  "version": "0.1.0",
  "dependencies": [
    { "name": "qtbase", "features": ["gui", "widgets", "serialport"] },
    "qtmqtt",
    "qtcharts",
    "fmt",
    "spdlog"
  ]
}
```

### 依赖安装位置

依赖自动安装到构建目录，不污染系统：

```
build/<preset>/vcpkg_installed/
├── x64-windows/      # 或 x64-linux, arm64-osx 等
├── vcpkg/
└── vcpkg-manifest-install.log
```

### 依赖列表

| 依赖 | 用途 |
|------|------|
| `qtbase` (gui, widgets, serialport) | Qt 核心 + GUI + 串口 |
| `qtmqtt` | MQTT 远端通信 |
| `qtcharts` | 数据可视化 |
| `fmt` | 格式化库 |
| `spdlog` | 日志库 |

---

## 🔧 常见问题

### Q1: CMake 找不到 vcpkg 工具链

**错误**: `Could NOT find vcpkg`

**解决**:
```powershell
# 确保 VCPKG_ROOT 已设置
echo $env:VCPKG_ROOT

# 或显式指定工具链
cmake --preset win-msvc2026-release `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

### Q2: vcpkg 依赖下载失败

**错误**: 网络超时或下载失败

**解决**:
```bash
# 检查网络连接
# 如有代理，设置环境变量

# Windows (PowerShell)
$env:HTTP_PROXY = "http://127.0.0.1:10808"
$env:HTTPS_PROXY = "http://127.0.0.1:10808"

# Linux/macOS (Bash)
export HTTP_PROXY="http://127.0.0.1:10808"
export HTTPS_PROXY="http://127.0.0.1:10808"

# 重新配置
cmake --preset win-msvc2026-release
```

### Q3: Qt 构建时间过长

**说明**: Qt 从源码编译需要 30-60 分钟，这是正常的。

**建议**:
- 首次构建后，依赖会缓存到 `vcpkg_installed` 目录
- 清理构建时保留 `vcpkg_installed` 可避免重复下载
- 或使用 vcpkg 二进制缓存

### Q4: 运行时提示缺少 DLL (Windows)

**错误**: `无法启动程序，因为计算机中丢失 Qt6Core.dll`

**解决**:
```powershell
# 方式 1: 使用 windeployqt 部署
cd build\win-msvc2026-release\Release
windeployqt heterolink-host.exe

# 方式 2: 从 vcpkg_installed 复制 DLL
$vcpkgBin = "build\win-msvc2026-release\vcpkg_installed\x64-windows\bin"
Copy-Item "$vcpkgBin\Qt6*.dll" .
Copy-Item "$vcpkgBin\*.dll" .
```

### Q5: 清理重建

```powershell
# Windows
Remove-Item -Recurse -Force build

# Linux/macOS
rm -rf build

# 重新配置（依赖会重新安装）
cmake --preset win-msvc2026-release
```

**保留依赖缓存**（避免重复下载）:
```powershell
# 只删除构建产物，保留 vcpkg_installed
Remove-Item -Recurse -Force build\win-msvc2026-release\CMakeFiles
Remove-Item -Recurse -Force build\win-msvc2026-release\*.exe
# 保留 build\win-msvc2026-release\vcpkg_installed\
```

---

## 🧪 运行测试

```bash
# Windows
cd build\win-msvc2026-release
ctest -C Release --output-on-failure

# Linux/macOS
cd build/lnx-gcc-release
ctest --output-on-failure
```

**运行单个测试**:
```bash
# Windows
.\tests\Release\test_protocol.exe
.\tests\Release\test_dataprocessor.exe

# Linux/macOS
./tests/test_protocol
./tests/test_dataprocessor
```

---

## 📁 构建产物

### Windows

```
build\win-msvc2026-release\
├── Release\
│   ├── heterolink-host.exe    # 主程序 (~289KB)
│   └── *.dll                  # 部署后复制
├── vcpkg_installed\
│   └── x64-windows\
│       ├── bin\               # Qt DLL
│       └── lib\               # Qt 库
└── tests\Release\
    ├── test_protocol.exe
    └── test_dataprocessor.exe
```

### Linux/macOS

```
build/lnx-gcc-release/
├── heterolink-host          # 主程序
├── vcpkg_installed/
│   └── x64-linux/           # 或 arm64-osx
├── tests/
│   ├── test_protocol
│   └── test_dataprocessor
└── compile_commands.json    # IDE 使用
```

---

## 📊 CMake Presets

### Configure Presets

| Preset | 平台 | 编译器 | 类型 |
|--------|------|--------|------|
| `win-msvc2026-release` | Windows | MSVC 2026 | Release |
| `win-msvc2026-debug` | Windows | MSVC 2026 | Debug |
| `lnx-gcc-release` | Linux | GCC | Release |
| `lnx-gcc-debug` | Linux | GCC | Debug |
| `mac-clang-debug` | macOS | Clang | Debug |

### Workflow Presets

| Preset | 描述 |
|--------|------|
| `workflow-win-msvc2026` | Windows MSVC 完整流程 |
| `workflow-lnx-debug` | Linux Debug 完整流程 |
| `workflow-lnx-release` | Linux Release 完整流程 |

---

## ❌ 不支持的构建方式

以下方式**不被支持**，请勿使用：

- ❌ 系统 Qt 安装（Qt Online Installer）
- ❌ Homebrew Qt（macOS）
- ❌ 包管理器 Qt（apt, pacman 等）
- ❌ 手动指定 `CMAKE_PREFIX_PATH` 到系统 Qt
- ❌ vcpkg 经典模式（非清单模式）

**原因**: 这些方式会导致依赖版本不一致，构建不可复现，且增加维护成本。

---

## 📚 相关文档

- [README.md](README.md) - 功能概述
- [CMakePresets.json](CMakePresets.json) - CMake 预设配置
- [vcpkg.json](vcpkg.json) - 依赖清单
- [../../docs/software/protocol-spec.md](../../docs/software/protocol-spec.md) - 协议规范

---

## 🎯 下一步

1. ✅ 编译成功
2. ⏳ 连接硬件或 MQTT Broker 测试
3. ⏳ 开发新功能

---

**构建愉快！** 🚀
