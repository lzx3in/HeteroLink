# HeteroLink 示例项目

通过实际示例学习 HeteroLink 的使用。

---

## 📦 示例列表

### 入门示例

| 示例 | 难度 | 描述 | 时间 |
|------|------|------|------|
| [基础遥测](basic-telemetry/) | ⭐ 初级 | 采集并上传传感器数据 | 15 分钟 |
| [MQTT 控制](mqtt-control/) | ⭐ 初级 | 通过 MQTT 控制 GPIO | 20 分钟 |

### 进阶示例

| 示例 | 难度 | 描述 | 时间 |
|------|------|------|------|
| [SPI 主站](spi-master/) | ⭐⭐ 中级 | 高速 SPI+DMA 数据传输 | 45 分钟 |
| [电机控制](motor-control/) | ⭐⭐⭐ 高级 | 电机参数在线整定 | 1 小时 |
| [数据记录器](data-logger/) | ⭐⭐ 中级 | 本地数据存储和回放 | 30 分钟 |

---

## 🎯 按功能分类

### 数据采集

- [基础遥测](basic-telemetry/) - ADC 采集 + MQTT 上传
- [数据记录器](data-logger/) - 本地 Flash 存储

### 远程控制

- [MQTT 控制](mqtt-control/) - GPIO 远程控制
- [电机控制](motor-control/) - 电机参数整定

### 高速通信

- [SPI 主站](spi-master/) - SPI+DMA 高速传输

---

## 🚀 开始使用

### 1. 克隆项目

```bash
git clone https://github.com/HeteroLink/HeteroLink.git
cd HeteroLink/examples
```

### 2. 选择示例

```bash
cd basic-telemetry  # 或其他示例目录
```

### 3. 编译烧录

```bash
# 设置目标芯片
idf.py set-target esp32c6

# 配置项目
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash

# 监视输出
idf.py -p /dev/ttyUSB0 monitor
```

---

## 📖 示例结构

每个示例包含：

```
example-name/
├── README.md           # 示例说明
├── main/
│   ├── app_main.c      # 主程序
│   ├── CMakeLists.txt  # 构建配置
│   └── idf_component.yml # 依赖
├── sdkconfig.defaults  # 默认配置
└── CMakeLists.txt      # 项目配置
```

---

## 💡 学习建议

### 新手路径

1. 从 [基础遥测](basic-telemetry/) 开始
2. 理解 MQTT 通信机制
3. 尝试修改参数和代码
4. 进阶到更复杂的示例

### 实践练习

- 修改采样频率
- 添加新的传感器
- 自定义 MQTT Topic
- 实现本地数据处理

---

## 🤝 贡献示例

欢迎提交你自己的示例！

### 要求

- 代码清晰、有注释
- 包含完整的 README
- 通过编译和测试
- 遵循项目代码规范

### 提交流程

1. Fork 项目
2. 创建示例目录
3. 编写代码和文档
4. 提交 Pull Request

---

## 📞 需要帮助？

- 📖 [安装指南](../getting-started/installation.md)
- ❓ [问题排查](../getting-started/troubleshooting.md)
- 💬 [社区讨论](https://github.com/HeteroLink/HeteroLink/discussions)

---

**准备好开始了吗？** → [基础遥测示例](basic-telemetry/)
