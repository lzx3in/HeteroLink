# HeteroLink 上位机 Windows 构建指南

**最后更新**: 2026-03-23  
**状态**: ✅ 完成 - 主程序、测试均可构建并通过

---

## 📋 前置条件

### 必需软件

1. **Visual Studio 2022/2026**
   - 安装 "使用 C++ 的桌面开发" 工作负载
   - 确保包含 Windows 10/11 SDK

2. **Qt 6.8+**
   - 推荐：Qt 6.10.2 MSVC 2022 64-bit
   - 安装路径示例：`C:\Qt\6.10.2\msvc2022_64`

3. **CMake 3.21+**
   - 下载地址：https://cmake.org/download/

---

## 🚀 快速构建流程

### 步骤 1: 构建 QtMqtt 子模块

```powershell
cd E:\Code\HeteroLink\3rdparty\qtmqtt
mkdir build
cd build

cmake .. -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64" `
  -DQT_HOST_PATH="C:/Qt/6.10.2/msvc2022_64"

cmake --build . --config RelWithDebInfo
```

### 步骤 2: 构建上位机主程序

```powershell
cd E:\Code\HeteroLink\software\host

# 配置（使用 CMake Preset）
cmake --preset windows-msvc-2026

# 构建 Release 版本
cmake --build build\windows-msvc-2026 --config Release
```

### 步骤 3: 运行程序

**方式 A: 一键部署后运行（推荐）**

部署后所有 Qt DLL 会复制到程序目录，之后可以直接双击运行：

```powershell
# 一次性部署（复制 Qt DLL）
cd E:\Code\HeteroLink\software\host\build\windows-msvc-2026\Release

# 复制 Qt 核心 DLL
Copy-Item "C:\Qt\6.10.2\msvc2022_64\bin\Qt6Core.dll" .
Copy-Item "C:\Qt\6.10.2\msvc2022_64\bin\Qt6Gui.dll" .
Copy-Item "C:\Qt\6.10.2\msvc2022_64\bin\Qt6Widgets.dll" .
Copy-Item "C:\Qt\6.10.2\msvc2022_64\bin\Qt6SerialPort.dll" .
Copy-Item "E:\Code\HeteroLink\3rdparty\qtmqtt\build\bin\Qt6Mqtt.dll" .

# 直接运行
.\heterolink-host.exe
```

**方式 B: 临时添加 PATH（不复制文件）**

```powershell
$env:PATH = "C:\Qt\6.10.2\msvc2022_64\bin;E:\Code\HeteroLink\3rdparty\qtmqtt\build\bin;" + $env:PATH
cd E:\Code\HeteroLink\software\host\build\windows-msvc-2026\Release
.\heterolink-host.exe
```

---

## 📁 构建产物

```
build\windows-msvc-2026\
├── Release\
│   ├── heterolink-host.exe    # 主程序
│   └── *.dll                  # Qt 运行时（部署后）
├── tests\Release\
│   ├── test_protocol.exe      # 协议测试
│   └── test_dataprocessor.exe # 数据处理器测试
└── examples\Release\
    └── mqtt_test.exe          # MQTT 测试（需要修复）
```

---

## ✅ 构建状态

| 组件 | 状态 | 备注 |
|------|------|------|
| 主程序 (heterolink-host.exe) | ✅ 成功 | 289KB |
| 测试 (test_protocol.exe) | ✅ 通过 | 1.73 秒 |
| 测试 (test_dataprocessor.exe) | ✅ 通过 | 0.29 秒 |
| MQTT 示例 (mqtt_test.exe) | ⏸️ 已禁用 | 需要时可启用 |

### MQTT API 已修复

代码已更新以匹配 Qt 6.10 的 qtmqtt API：
- ✅ `errorChanged` 信号已正确使用
- ✅ 订阅通过 `client_->subscribe()` 管理
- ✅ `messageReceived` 信号参数已适配

---

## 🔧 环境变量设置

### 方法 1: 临时设置（当前会话）

```powershell
$env:PATH = "C:\Qt\6.10.2\msvc2022_64\bin;" + $env:PATH
$env:Qt6_DIR = "C:\Qt\6.10.2\msvc2022_64\lib\cmake\Qt6"
```

### 方法 2: 永久设置

1. 打开"系统属性" → "高级" → "环境变量"
2. 在"系统变量"中添加/编辑：
   - `PATH`: 添加 `C:\Qt\6.10.2\msvc2022_64\bin`
   - 新建 `Qt6_DIR`: `C:\Qt\6.10.2\msvc2022_64\lib\cmake\Qt6`

---

## 🧪 运行测试

```powershell
cd build\windows-msvc-2026

# 运行所有测试
ctest -C Release --output-on-failure

# 运行单个测试
.\tests\Release\test_protocol.exe
.\tests\Release\test_dataprocessor.exe
```

---

## 📝 CMake Presets 说明

| Preset 名称 | 编译器 | 类型 |
|------------|--------|------|
| `windows-msvc-2026` | Visual Studio 2026 | Release |
| `windows-msvc-2026-debug` | Visual Studio 2026 | Debug |

---

## 🛠️ 故障排查

### CMake 找不到 Qt6

```powershell
# 检查 Qt 安装
Test-Path "C:\Qt\6.10.2\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake"

# 设置环境变量
$env:Qt6_DIR = "C:\Qt\6.10.2\msvc2022_64\lib\cmake\Qt6"
```

### 链接错误 - 找不到 Qt 库

确保在 Visual Studio Developer Command Prompt 中构建，或者：

```powershell
# 设置 VC 环境变量
& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### 运行时提示缺少 DLL

```powershell
# 使用 windeployqt 部署
cd build\windows-msvc-2026\Release
windeployqt heterolink-host.exe
```

---

## 📚 相关文档

- [主 README](README.md) - 功能概述
- [协议规范](../../docs/software/protocol-spec.md) - 通信协议
- [MQTT 指南](../../docs/software/mqtt-guide.md) - MQTT 配置
- [主机设计](../../docs/software/host-design.adoc) - 架构设计

---

## ✅ 构建检查清单

- [x] Visual Studio 2022/2026 已安装
- [x] Qt 6.8+ MSVC 2022 64-bit 已安装
- [x] CMake 3.21+ 已安装并添加到 PATH
- [x] QtMqtt 子模块已构建
- [x] 主程序构建成功 (289KB)
- [x] 测试通过 (2/2 tests passed)
- [ ] 程序运行验证 (需要硬件或 MQTT Broker)

---

**备注**: 本指南基于 Windows 11 24H2 + VS 2026 + Qt 6.10.2 环境编写。
