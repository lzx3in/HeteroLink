# HeteroLink

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3+-green.svg)](https://github.com/espressif/esp-idf)
[![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20ESP32‑S3%20%7C%20ESP32‑C6-orange.svg)](docs/hardware/COMPATIBILITY.md)

**基于 ESP32 的通用型高速调试协处理器平台**

HeteroLink 为单板计算机、微控制器和嵌入式系统提供强大的调试、监控和数据采集能力。通过高速 SPI+DMA、自定义 UART 协议和云端 MQTT 连接，实现 kHz 级遥测、μs 级响应和零代码即插即用的调试体验。

![System Architecture](docs/images/architecture.svg)

---

## 🚀 核心特性

| 通道 | 协议 | 功能 | 性能 |
|------|------|------|------|
| **近端** | UART 自定义二进制 | 高频遥测、指令下发 | kHz 级数据上传，μs 级响应 |
| **远端** | MQTT over WiFi | 设备状态上云、远程控制、告警推送 | 集群管理 |
| **板间** | SPI+DMA+ 共享内存 | 高速双向数据传输 | 10+ MB/s |
| **点测** | GPIO+ADC | 数字逻辑探测 + 模拟信号采集 | 零代码即插即用 |

---

## 📦 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                    上位机 (Qt/C++ 跨平台)                    │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ 近端通道      │  │ 远端通道      │  │ 数据操作/告警    │   │
│  │ UART 二进制   │  │ MQTT over WiFi│  │ 日志记录        │   │
│  │ kHz 级遥测    │  │ 云端 Broker   │  │ 集群管理        │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            ↕ (近端 UART / 远端 WiFi)
┌─────────────────────────────────────────────────────────────┐
│              下位机 (ESP32 协处理器子板)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ SPI+DMA      │  │ GPIO 数字探测  │  │ ADC 模拟采集     │   │
│  │ 共享内存      │  │ 双模点测      │  │ 零代码即插即用   │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            ↕ (SPI+DMA+ 共享内存)
┌─────────────────────────────────────────────────────────────┐
│                    目标主板 (任意单板)                        │
│                    拓展调试能力                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 🛠️ 快速开始

### 1. 环境准备

```bash
# 安装 ESP-IDF (v5.3+)
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32 esp32s3 esp32c6
. export.sh

# 或使用 EIM (推荐)
cd ~/Code/HeteroLink/scripts
./install-esp-idf.sh
```

### 2. 编译固件

```bash
cd firmware/esp32/subboard

# 设置目标芯片
idf.py set-target esp32c6

# 配置 WiFi 和 MQTT
idf.py menuconfig

# 编译
idf.py build
```

### 3. 烧录测试

```bash
# 烧录到设备
idf.py -p /dev/ttyUSB0 flash

# 监视串口输出
idf.py -p /dev/ttyUSB0 monitor
```

**📖 详细指南**:
- [安装指南](docs/getting-started/installation.md)
- [快速上手](docs/getting-started/quick-start.md)
- [问题排查](docs/getting-started/troubleshooting.md)

---

## 📁 项目结构

```
HeteroLink/
├── 📄 根文件
│   ├── README.md                # 本文件
│   ├── LICENSE                  # Apache-2.0 许可证
│   ├── CONTRIBUTING.md          # 贡献指南
│   ├── CODE_OF_CONDUCT.md       # 行为准则
│   ├── SECURITY.md              # 安全策略
│   ├── CHANGELOG.md             # 版本历史
│   ├── AUTHORS.md               # 贡献者名单
│   └── MAINTAINERS.md           # 维护者信息
│
├── 📚 文档 (docs/)
│   ├── README.md                # 文档导航
│   ├── getting-started/         # 快速开始
│   │   ├── installation.md      # 安装指南
│   │   ├── quick-start.md       # 快速上手
│   │   └── troubleshooting.md   # 问题排查
│   ├── hardware/                # 硬件文档
│   │   ├── subboard/            # 子板设计
│   │   └── compatibility.md     # 芯片兼容性
│   ├── firmware/                # 固件文档
│   │   ├── architecture.md      # 架构设计
│   │   └── configuration.md     # 配置指南
│   └── software/                # 上位机文档
│
├── 💾 固件 (firmware/)
│   └── esp32/subboard/          # ESP32 固件
│       ├── main/                # 主程序
│       └── components/          # 自定义组件 (计划中)
│
├── 🔧 硬件 (hardware/)
│   ├── subboard/                # 子板设计
│   │   ├── kicad/               # KiCad 项目
│   │   ├── gerbers/             # Gerber 文件
│   │   └── bom/                 # BOM 表
│   └── adapter/                 # 转接板 (计划中)
│
├── 🖥️ 上位机 (software/)
│   ├── host/                    # Qt 应用 (计划中)
│   ├── cli/                     # 命令行工具 (计划中)
│   └── python/                  # Python 库 (计划中)
│
├── 📦 示例 (examples/)
│   ├── basic-telemetry/         # 基础遥测
│   ├── mqtt-control/            # MQTT 控制
│   ├── spi-master/              # SPI 主站
│   ├── motor-control/           # 电机控制
│   └── data-logger/             # 数据记录器
│
├── 🧪 测试 (tests/)
│   ├── hardware/                # 硬件测试
│   ├── firmware/                # 固件测试
│   └── integration/             # 集成测试
│
├── 🛠️ 脚本 (scripts/)
│   ├── install-esp-idf.sh       # ESP-IDF 安装
│   └── ...
│
└── .github/                     # GitHub 配置
    ├── workflows/               # CI/CD
    └── ISSUE_TEMPLATE/          # Issue 模板
```

---

## 🎯 应用场景

- **电机参数在线整定** - 实时采集电流、电压、转速
- **实时示波器** - kHz 级采样，可视化波形
- **自动测试流程** - 脚本化测试序列
- **多设备集群管理** - 同时监控数十个节点
- **故障告警推送** - 异常即时通知

---

## 🤝 参与贡献

我们欢迎各种形式的贡献！

- 🐛 [报告问题](https://github.com/HeteroLink/HeteroLink/issues)
- 💡 [功能建议](https://github.com/HeteroLink/HeteroLink/discussions)
- 🔧 [提交 PR](https://github.com/HeteroLink/HeteroLink/pulls)
- 📖 改进文档

开始贡献前，请阅读：
- [贡献指南](CONTRIBUTING.md)
- [行为准则](CODE_OF_CONDUCT.md)
- [维护者信息](MAINTAINERS.md)
- [安全策略](SECURITY.md)

---

## 📄 许可证

本项目采用 [Apache License 2.0](LICENSE) 开源。

---

## 🙏 致谢

- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif 物联网开发框架
- [EMQX](https://github.com/emqx/emqx) - 开源 MQTT Broker
- [Qt](https://www.qt.io/) - 跨平台应用框架
- 所有 [贡献者](AUTHORS.md)

---

## 📬 联系方式

- 🌐 官网：https://heterolink.dev (待上线)
- 📖 文档：[查看文档](docs/README.md)
- 💬 讨论区：[GitHub Discussions](https://github.com/HeteroLink/HeteroLink/discussions)
- 📧 安全：security@heterolink.dev (待设置)

---

<div align="center">

**Made with ❤️ by the HeteroLink Team**

[⭐ Star this repo](https://github.com/HeteroLink/HeteroLink/stargazers) | 
[🍴 Fork](https://github.com/HeteroLink/HeteroLink/fork) | 
[📥 Releases](https://github.com/HeteroLink/HeteroLink/releases) | 
[📖 Docs](docs/README.md)

---

📦 **v0.1.0** - 2026-03-21 - [查看变更日志](CHANGELOG.md)

</div>
