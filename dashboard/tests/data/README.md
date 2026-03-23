# Test Data Fixtures

本目录包含测试用的样例数据文件。

## 文件列表

### 协议帧样例

- `frames_valid.bin` - 有效的协议帧（二进制）
- `frames_invalid.bin` - 无效的协议帧（用于错误测试）
- `telemetry_samples.json` - 遥测数据样例（JSON 格式）

### 配置样例

- `config_valid.json` - 有效的配置文件
- `config_invalid.json` - 无效的配置文件（用于验证测试）

## 使用方式

在测试中加载这些数据文件：

```cpp
QFile file(":/data/telemetry_samples.json");
// 或
QFile file(QFINDTESTDATA("data/telemetry_samples.json"));
```

## 生成新样例

使用 `scripts/generate_test_data.py` 生成新的测试数据。
