# HeteroLink Host - vcpkg 清单模式构建指南

## 前置要求

1. **vcpkg** 已安装并配置环境变量
   ```powershell
   # 验证 vcpkg 安装
   vcpkg --version
   ```

2. **CMake** >= 3.21

3. **编译器**:
   - Windows: Visual Studio 2019+ 或 MinGW
   - Linux: GCC 9+ 或 Clang 10+
   - macOS: Xcode Command Line Tools

## 快速开始

### Windows (PowerShell)

```powershell
cd E:\Code\HeteroLink\software\host

# 配置 (使用 vcpkg 清单模式)
cmake --preset windows-msvc-2026

# 构建
cmake --build --preset windows-msvc-2026-build

# 运行
.\build\windows-msvc-2026\Release\heterolink-host.exe
```

### Linux

```bash
cd /path/to/HeteroLink/software/host

# 配置
cmake --preset linux-debug

# 构建
cmake --build --preset linux-debug-build

# 运行
./build/linux-debug/heterolink-host
```

## vcpkg 依赖

项目使用 **清单模式 (manifest mode)**，依赖在 `vcpkg.json` 中声明：

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

首次配置时，vcpkg 会自动安装所有依赖到 `build/<preset>/vcpkg_installed` 目录。

## 清理重建

```powershell
# 删除构建目录
Remove-Item -Recurse -Force build

# 重新配置和构建
cmake --preset windows-msvc-2026
cmake --build --preset windows-msvc-2026-build
```

## 注意事项

- **不向后兼容**: 本项目仅支持 Qt6，不再支持 Qt5
- **清单模式**: 无需手动运行 `vcpkg install`，CMake 配置时自动处理
- **工具链**: CMakePresets.json 已配置 vcpkg 工具链文件
- **MQTT 必需**: Qt6 MQTT 模块是必需的编译依赖

## 故障排除

### vcpkg 未找到

确保 `VCPKG_ROOT` 环境变量已设置：

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"  # 修改为你的 vcpkg 路径
```

### Qt 模块缺失

清理后重新配置，确保 vcpkg 完成所有依赖安装：

```powershell
Remove-Item -Recurse -Force build
cmake --preset windows-msvc-2026
```

### 编译错误

检查编译器版本是否满足 CMake 3.21+ 要求。
