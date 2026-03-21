# HeteroLink Host - 上位机应用

跨平台 Qt/C++ 应用，用于设备管理、数据采集和可视化。

## 🚀 快速开始

### 环境要求

- **Qt**: 6.5+
- **CMake**: 3.20+
- **编译器**: GCC 11+ / Clang 14+ / MSVC 2019+
- **操作系统**: Windows 10+ / Linux / macOS 10.15+

### 构建步骤

```bash
cd software/host

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
cmake --build .

# 运行
./heterolink-host
```

### 依赖安装

**Ubuntu/Debian**:
```bash
sudo apt-get install qt6-base-dev qt6-serialport-dev libqmqtt-dev
```

**Windows**:
使用 Qt Online Installer，选择以下组件：
- Qt 6.x MSVC 2019 64-bit
- Qt Serial Port
- Qt Charts (可选)

**macOS**:
```bash
brew install qt@6 cmake
```

## 📁 项目结构

```
host/
├── CMakeLists.txt           # 构建配置
├── src/                     # 源代码
│   ├── main.cpp             # 程序入口
│   ├── app/                 # 应用层
│   ├── core/                # 核心业务逻辑
│   ├── protocol/            # 通信协议
│   ├── ui/                  # 用户界面
│   ├── storage/             # 数据存储
│   └── utils/               # 工具类
├── resources/               # 资源文件
├── tests/                   # 单元测试
└── docs/                    # 开发文档
```

## 🛠️ 开发指南

详见 [设计文档](../../docs/software/host-design.md)

## 📄 许可证

Apache License 2.0
