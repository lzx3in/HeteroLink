# HeteroLink 项目重构方案

**目标**: 将 HeteroLink 打造为成熟的开源项目

**当前状态**: 基础框架已建立，需要完善目录结构和文档体系

---

## 📁 建议的目录结构

```
HeteroLink/
├── 📄 项目根文件
│   ├── README.md                    # ✅ 已有，需补充徽章和更多语言版本
│   ├── LICENSE                      # ✅ 已有 (Apache-2.0)
│   ├── CONTRIBUTING.md              # ✅ 已有
│   ├── CODE_OF_CONDUCT.md           # ✅ 已有
│   ├── SECURITY.md                  # ❌ 待添加 (安全策略)
│   ├── CHANGELOG.md                 # ❌ 待添加 (版本历史)
│   ├── AUTHORS.md                   # ❌ 待添加 (贡献者名单)
│   └── MAINTAINERS.md               # ❌ 待添加 (维护者信息)
│
├── 📚 文档体系 (docs/)
│   ├── README.md                    # ❌ 文档导航索引
│   ├── getting-started/             # ❌ 快速开始系列
│   │   ├── index.md                 # 文档首页
│   │   ├── installation.md          # 安装指南
│   │   ├── quick-start.md           # ✅ 已有 (需迁移)
│   │   └── troubleshooting.md       # 问题排查
│   ├── hardware/                    # ❌ 硬件文档
│   │   ├── index.md                 # 硬件概览
│   │   ├── subboard/                # 子板文档
│   │   │   ├── overview.md          # 子板概览
│   │   │   ├── schematics.md        # 原理图说明
│   │   │   └── assembly.md          # 组装指南
│   │   ├── compatibility.md         # 芯片兼容性
│   │   └── ordering.md              # 订购指南 (如适用)
│   ├── firmware/                    # ❌ 固件文档
│   │   ├── index.md                 # 固件概览
│   │   ├── architecture.md          # 架构设计
│   │   ├── api-reference.md         # API 参考
│   │   ├── mqtt-protocol.md         # MQTT 协议规范
│   │   ├── uart-protocol.md         # UART 协议规范
│   │   └── configuration.md         # 配置指南
│   ├── software/                    # ❌ 上位机文档
│   │   ├── index.md                 # 上位机概览
│   │   ├── host-app.md              # Qt 上位机文档
│   │   └── cli-tools.md             # 命令行工具
│   ├── examples/                    # ❌ 示例文档
│   │   └── README.md                # 示例索引
│   ├── development/                 # ❌ 开发指南
│   │   ├── contributing.md          # 贡献指南 (扩展版)
│   │   ├── coding-style.md          # 代码规范
│   │   ├── testing.md               # 测试指南
│   │   └── release.md               # 发布流程
│   └── images/                      # ❌ 图片资源
│       ├── architecture.svg         # 架构图
│       ├── wiring-diagram.svg       # 接线图
│       └── screenshots/             # 截图
│
├── 🔧 硬件设计 (hardware/)
│   ├── README.md                    # ❌ 硬件目录说明
│   ├── subboard/                    # ❌ 子板设计
│   │   ├── kicad/                   # KiCad 项目
│   │   │   ├── subboard.kicad_pcb
│   │   │   ├── subboard.kicad_sch
│   │   │   └── subboard.kicad_prl
│   │   ├── gerbers/                 # Gerber 文件 (发布用)
│   │   ├── bom/                     # BOM 表
│   │   └── 3d-models/               # 3D 模型
│   └── adapter/                     # ❌ 转接板设计
│       └── kicad/
│
├── 💾 固件代码 (firmware/)
│   ├── README.md                    # ❌ 固件目录说明
│   ├── esp32/                       # ✅ 已有 (需移动)
│   │   └── subboard/                # 子板固件
│   │       ├── main/
│   │       │   ├── app_main.c       # ✅ 已有
│   │       │   ├── CMakeLists.txt   # ✅ 已有
│   │       │   ├── idf_component.yml # ✅ 已有
│   │       │   └── Kconfig.projbuild # ✅ 已有
│   │       ├── components/          # ❌ 自定义组件
│   │       │   ├── heterolink_core/ # 核心库
│   │       │   ├── mqtt_client/     # MQTT 客户端
│   │       │   ├── uart_protocol/   # UART 协议栈
│   │       │   ├── spi_dma/         # SPI+DMA 驱动
│   │       │   └── adc_gpio_probe/  # ADC/GPIO 探测
│   │       ├── tests/               # ❌ 固件测试
│   │       └── CMakeLists.txt
│   └── examples/                    # ❌ 固件示例
│       ├── basic_telemetry/         # 基础遥测
│       ├── mqtt_control/            # MQTT 控制
│       └── spi_master/              # SPI 主站
│
├── 🖥️ 上位机软件 (software/)
│   ├── README.md                    # ❌ 软件目录说明
│   ├── host/                        # ❌ Qt 上位机
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   ├── include/
│   │   ├── resources/
│   │   └── tests/
│   ├── cli/                         # ❌ 命令行工具
│   │   ├── Cargo.toml               # 如用 Rust
│   │   └── src/
│   └── python/                      # ❌ Python 库
│       ├── setup.py
│       └── heterolink/
│
├── 🧪 测试 (tests/)
│   ├── README.md                    # ❌ 测试说明
│   ├── hardware/                    # 硬件测试
│   ├── firmware/                    # 固件测试
│   ├── integration/                 # 集成测试
│   └── scripts/                     # 测试脚本
│
├── 📦 示例项目 (examples/)
│   ├── README.md                    # ❌ 示例索引
│   ├── motor-control/               # 电机控制示例
│   ├── data-logger/                 # 数据记录器
│   ├── oscilloscope/                # 示波器示例
│   └── multi-device/                # 多设备示例
│
├── 🛠️ 脚本工具 (scripts/)
│   ├── README.md                    # ❌ 脚本说明
│   ├── install-esp-idf.sh           # ✅ 已有
│   ├── flash.sh                     # ❌ 烧录脚本
│   ├── test-mqtt.sh                 # ❌ MQTT 测试
│   ├── generate-docs.sh             # ❌ 文档生成
│   └── release.sh                   # ❌ 发布脚本
│
├── 🔒 CI/CD (.github/)
│   ├── workflows/
│   │   ├── build.yml                # ✅ 已有
│   │   ├── test.yml                 # ❌ 测试工作流
│   │   ├── docs.yml                 # ❌ 文档部署
│   │   ├── release.yml              # ❌ 发布工作流
│   │   └── lint.yml                 # ❌ 代码检查
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md            # ✅ 已有
│   │   ├── feature_request.md       # ✅ 已有
│   │   └── hardware-review.md       # ❌ 硬件审查
│   ├── PULL_REQUEST_TEMPLATE.md     # ✅ 已有
│   └── dependabot.yml               # ❌ 依赖更新
│
├── 📝 配置根文件
│   ├── .gitignore                   # ✅ 已有
│   ├── .gitmodules                  # ❌ 子模块配置
│   ├── .clang-format                # ❌ 代码格式化
│   ├── .pre-commit-config.yaml      # ❌ Git 钩子
│   ├── PlatformIO.ini               # ❌ PlatformIO 支持
│   └── CMakePresets.json            # ❌ CMake 预设
│
└── 📌 其他
    ├── .vscode/                     # ✅ 已有
    ├── .idea/                       # ❌ CLion 配置
    └── licenses/                    # ✅ 已有 (第三方许可证)
```

---

## 🎯 优先级任务

### 🔴 P0 - 立即执行 (本周)

1. **重组 firmware/ 目录**
   - 将 `software/esp32/subboard/` 移动到 `firmware/esp32/subboard/`
   - 创建 `firmware/README.md`
   - 添加组件目录结构 (`components/`)

2. **完善文档导航**
   - 创建 `docs/README.md` 作为文档索引
   - 按新结构重组 `docs/` 子目录
   - 迁移 `QUICK_START.md` 到 `docs/getting-started/`

3. **创建示例项目**
   - 建立 `examples/` 目录
   - 添加 2-3 个基础示例
   - 每个示例包含独立 README

4. **添加关键根文件**
   - `SECURITY.md` - 安全策略
   - `CHANGELOG.md` - 版本历史
   - `MAINTAINERS.md` - 维护者信息

### 🟡 P1 - 短期 (2 周内)

5. **扩展 CI/CD**
   - 添加测试工作流 (`test.yml`)
   - 添加文档部署 (`docs.yml`)
   - 配置 Dependabot

6. **完善测试框架**
   - 创建 `tests/` 目录结构
   - 添加基础单元测试
   - 编写集成测试脚本

7. **硬件文档**
   - 创建 `hardware/README.md`
   - 添加芯片兼容性文档
   - 准备 KiCad 项目结构

### 🟢 P2 - 中期 (1 个月内)

8. **上位机项目**
   - 初始化 Qt 项目 (`software/host/`)
   - 创建基础 UI 框架
   - 实现 MQTT 连接

9. **开发者体验**
   - 添加 `.pre-commit-config.yaml`
   - 配置代码格式化 (`.clang-format`)
   - 添加 PlatformIO 支持

10. **文档站点**
    - 集成 MkDocs 或 Docusaurus
    - 配置 GitHub Pages 部署
    - 添加中文文档支持

---

## 📋 执行清单

### 第一阶段：结构重组

```bash
cd /home/lzx/Code/HeteroLink

# 1. 创建新目录结构
mkdir -p firmware/esp32
mkdir -p docs/{getting-started,hardware,firmware,software,examples,development,images}
mkdir -p examples/{basic-telemetry,mqtt-control,spi-master}
mkdir -p tests/{hardware,firmware,integration,scripts}
mkdir -p software/{host,cli,python}
mkdir -p hardware/{subboard,adapter}

# 2. 移动固件代码
mv software/esp32/subboard firmware/esp32/subboard
rmdir -p software/esp32 2>/dev/null || true

# 3. 迁移文档
mv docs/QUICK_START.md docs/getting-started/quick-start.md

# 4. 创建根文件
touch SECURITY.md CHANGELOG.md AUTHORS.md MAINTAINERS.md
```

### 第二阶段：内容填充

- [ ] 编写所有 `README.md` 文件
- [ ] 创建示例项目代码
- [ ] 完善 CI/CD 配置
- [ ] 添加测试用例

### 第三阶段：优化发布

- [ ] 配置文档站点
- [ ] 准备 v0.1.0 发布
- [ ] 创建 Release Notes
- [ ] 发布首个正式版本

---

## 📊 成熟度对比

| 维度 | 当前状态 | 目标状态 |
|------|----------|----------|
| 目录结构 | ⭐⭐☆☆☆ 基础框架 | ⭐⭐⭐⭐⭐ 完整清晰 |
| 文档体系 | ⭐⭐☆☆☆ 单篇文档 | ⭐⭐⭐⭐⭐ 分层完整 |
| 示例项目 | ❌ 无 | ⭐⭐⭐⭐⭐ 5+ 示例 |
| 测试覆盖 | ❌ 无 | ⭐⭐⭐☆☆ 基础测试 |
| CI/CD | ⭐⭐☆☆☆ 仅构建 | ⭐⭐⭐⭐⭐ 全流程 |
| 社区文件 | ⭐⭐⭐☆☆ 基础 | ⭐⭐⭐⭐⭐ 完整 |

---

## 💡 关键建议

1. **保持向后兼容**: 移动目录时，在旧位置添加重定向说明
2. **渐进式重构**: 分阶段执行，每阶段可独立发布
3. **文档先行**: 在实现功能前先写文档框架
4. **示例驱动**: 用实际示例展示项目能力
5. **自动化优先**: 尽可能用 CI/CD 替代手动流程

---

## 📅 下一步行动

1. **立即**: 讨论并确认此重构方案
2. **今天**: 执行第一阶段 (结构重组)
3. **本周**: 完成 P0 任务
4. **下周**: 开始 P1 任务
5. **月底**: 准备 v0.1.0 发布

---

<div align="center">

**让 HeteroLink 成为 ESP32 生态中的明星项目！ 🚀**

</div>
