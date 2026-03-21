# 贡献指南

感谢你考虑为 HeteroLink 做出贡献！🎉

## 📖 目录

- [行为准则](#行为准则)
- [我能贡献什么？](#我能贡献什么)
- [开始贡献](#开始贡献)
- [开发流程](#开发流程)
- [代码规范](#代码规范)
- [提交信息规范](#提交信息规范)
- [Pull Request](#pull-request)

---

## 行为准则

本项目采用 [贡献者公约](CODE_OF_CONDUCT.md)。参与即表示你同意遵守其条款。

---

## 我能贡献什么？

### 🐛 报告 Bug

1. 搜索 [现有 Issues](https://github.com/HeteroLink/HeteroLink/issues) 确认问题未被报告
2. 使用 [Bug Report 模板](.github/ISSUE_TEMPLATE/bug_report.md) 创建新 Issue
3. 提供尽可能多的信息：
   - ESP32 型号
   - ESP-IDF 版本
   - 复现步骤
   - 日志输出

### 💡 功能建议

1. 搜索 [现有 Discussions](https://github.com/HeteroLink/HeteroLink/discussions)
2. 创建新的 Discussion 描述你的想法
3. 说明使用场景和预期效果

### 🔧 代码贡献

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'feat: Add AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 📖 文档改进

- 修正拼写错误
- 补充缺失的说明
- 添加示例代码
- 翻译文档

---

## 开始贡献

### 1. 设置开发环境

```bash
# Fork 并克隆
git clone https://github.com/YOUR_USERNAME/HeteroLink.git
cd HeteroLink

# 添加上游远程仓库
git remote add upstream https://github.com/HeteroLink/HeteroLink.git

# 安装 ESP-IDF
./scripts/install-esp-idf.sh
```

### 2. 编译测试

```bash
cd software/esp32/subboard
idf.py set-target esp32c6
idf.py build
```

---

## 开发流程

### 分支策略

- `main` - 主分支，始终可编译
- `develop` - 开发分支，新功能合并至此
- `feature/*` - 特性分支
- `bugfix/*` - 修复分支
- `release/*` - 发布分支

### 工作流

```bash
# 同步上游
git checkout main
git pull upstream main

# 创建特性分支
git checkout -b feature/your-feature

# 开发并提交
git add .
git commit -m "feat: your feature description"

# 推送
git push origin feature/your-feature
```

---

## 代码规范

### C/C++ (固件)

遵循 [ESP-IDF 代码规范](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/index.html#coding-style)

```c
// 函数命名：小写 + 下划线
esp_err_t mqtt_app_start(void);

// 变量命名：小写 + 下划线
static const char *TAG = "heterolink";

// 宏命名：大写 + 下划线
#define HETEROLINK_VERSION_MAJOR 0
```

### 格式化

```bash
# 使用 clang-format
clang-format -i software/esp32/subboard/main/*.c
```

---

## 提交信息规范

采用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

### Type

- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式 (不影响功能)
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具/配置

### 示例

```
feat(mqtt): 添加 MQTT5 远端通道支持

- 实现 WiFi 自动连接
- 实现 MQTT5 客户端
- 添加 Last Will 遗嘱消息
- 支持 broker.emqx.io 公共 Broker

Closes #123

BREAKING CHANGE: 需要配置 WiFi SSID 和密码
```

---

## Pull Request

### PR 检查清单

- [ ] 代码通过编译
- [ ] 添加/更新测试
- [ ] 更新文档
- [ ] 遵循代码规范
- [ ] 提交信息规范

### 审核流程

1. CI 自动检查 (编译、测试)
2. 维护者代码审核
3. 讨论并修改
4. 合并到 `develop` 分支

---

## 🙏 感谢

每一位贡献者都值得感谢！你的贡献会让 HeteroLink 变得更好。

<div align="center">

**Happy Coding! 🚀**

</div>
