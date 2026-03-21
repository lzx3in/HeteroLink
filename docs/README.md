# HeteroLink 文档导航

欢迎使用 HeteroLink 文档！

---

## 🚀 快速开始

**第一次使用？** 从这里开始：

- [📦 安装指南](getting-started/installation.md) - 环境准备
- [⚡ 快速上手](getting-started/quick-start.md) - 10 分钟入门
- [❓ 问题排查](getting-started/troubleshooting.md) - 常见问题

---

## 📚 文档分类

### 🔧 硬件文档

了解 HeteroLink 硬件设计和兼容性：

- [硬件概览](hardware/index.md) - 系统架构
- [子板设计](hardware/subboard/overview.md) - 协处理器子板
- [芯片兼容性](hardware/compatibility.md) - 支持的 ESP32 型号
- [组装指南](hardware/subboard/assembly.md) - 硬件组装

### 💾 固件文档

ESP32 固件开发和配置：

- [固件概览](firmware/index.md) - 固件架构
- [架构设计](firmware/architecture.md) - 系统设计
- [配置指南](firmware/configuration.md) - 编译配置
- [API 参考](firmware/api-reference.md) - 接口文档
- [MQTT 协议](firmware/mqtt-protocol.md) - 远端通信
- [UART 协议](firmware/uart-protocol.md) - 近端通信

### 🖥️ 上位机文档

Qt 上位机和工具链：

- [上位机概览](software/index.md) - 软件架构
- [Qt 应用](software/host-app.md) - 桌面应用
- [命令行工具](software/cli-tools.md) - CLI 工具
- [Python 库](software/python-lib.md) - Python SDK

### 📖 示例项目

学习和测试用的示例代码：

- [示例索引](examples/README.md) - 所有示例
- [基础遥测](examples/basic-telemetry/) - 数据采集
- [MQTT 控制](examples/mqtt-control/) - 远程控制
- [SPI 主站](examples/spi-master/) - 高速传输
- [电机控制](examples/motor-control/) - 电机参数整定
- [数据记录器](examples/data-logger/) - 日志记录

### 🛠️ 开发指南

参与项目开发：

- [贡献指南](development/contributing.md) - 如何贡献
- [代码规范](development/coding-style.md) - 编码标准
- [测试指南](development/testing.md) - 测试方法
- [发布流程](development/release.md) - 版本发布

---

## 📊 按角色查看

### 新手用户
1. [安装指南](getting-started/installation.md)
2. [快速上手](getting-started/quick-start.md)
3. [基础示例](examples/basic-telemetry/)

### 硬件开发者
1. [硬件概览](hardware/index.md)
2. [子板设计](hardware/subboard/overview.md)
3. [芯片兼容性](hardware/compatibility.md)

### 固件开发者
1. [固件架构](firmware/architecture.md)
2. [配置指南](firmware/configuration.md)
3. [API 参考](firmware/api-reference.md)

### 应用开发者
1. [上位机概览](software/index.md)
2. [MQTT 协议](firmware/mqtt-protocol.md)
3. [示例项目](examples/README.md)

### 贡献者
1. [贡献指南](development/contributing.md)
2. [代码规范](development/coding-style.md)
3. [测试指南](development/testing.md)

---

## 🔗 外部资源

- **ESP-IDF 文档**: https://docs.espressif.com/projects/esp-idf/
- **ESP32 技术参考**: https://www.espressif.com/en/products/socs/esp32
- **MQTT 协议**: https://mqtt.org/
- **GitHub 仓库**: https://github.com/HeteroLink/HeteroLink

---

## 📝 文档维护

### 更新文档

发现文档问题？欢迎改进：

1. 点击页面右上角的 "Edit this page"
2. 提交修改
3. 等待审核合并

### 文档规范

- 使用 Markdown 格式
- 代码示例添加语法高亮
- 图片放在 `images/` 目录
- 保持中英文术语一致

### 构建文档

```bash
# 安装依赖
pip install mkdocs mkdocs-material

# 本地预览
cd docs
mkdocs serve

# 构建静态站点
mkdocs build
```

---

## 📞 需要帮助？

- 🐛 [报告问题](https://github.com/HeteroLink/HeteroLink/issues)
- 💡 [功能建议](https://github.com/HeteroLink/HeteroLink/discussions)
- 💬 [社区讨论](https://github.com/HeteroLink/HeteroLink/discussions)

---

**最后更新**: 2026-03-21
