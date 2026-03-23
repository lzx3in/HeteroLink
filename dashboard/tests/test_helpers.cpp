/**
 * HeteroLink Host - 测试辅助工具单元测试
 * 
 * 验证 TestHelpers 命名空间中的工具函数工作正常
 */

#include <QTest>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "TestHelpers.h"

using namespace HeteroLink;
using namespace HeteroLink::TestHelpers;

class TestHelpersUnit : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting TestHelpers unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "TestHelpers unit tests finished";
    }
    
    // ========== 数据生成器测试 ==========
    
    void testGenerateRandomTelemetry() {
        TelemetryData data = generateRandomTelemetry(4, 0.0f, 100.0f);
        
        QCOMPARE(data.channels.size(), 4);
        
        // 验证所有通道值在范围内
        for (float ch : data.channels) {
            QVERIFY(ch >= 0.0f);
            QVERIFY(ch <= 100.0f);
        }
    }
    
    void testGenerateRandomTelemetry_DifferentCounts() {
        // 测试不同通道数量
        QCOMPARE(generateRandomTelemetry(1).channels.size(), 1);
        QCOMPARE(generateRandomTelemetry(2).channels.size(), 2);
        QCOMPARE(generateRandomTelemetry(8).channels.size(), 8);
    }
    
    void testCreateTelemetry() {
        std::vector<float> values = {1.0f, 2.0f, 3.0f, 4.0f};
        TelemetryData data = createTelemetry(12345, values);
        
        QCOMPARE(data.timestamp, 12345u);
        QCOMPARE(data.channels.size(), 4u);
        QVERIFY(vectorsApproxEqual(data.channels, std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f}));
    }
    
    // ========== 帧创建测试 ==========
    
    void testCreateTestFrame() {
        Frame frame = createTestFrame(0x05, 0x10, {0x01, 0x02, 0x03});
        
        QCOMPARE(frame.deviceId, 0x05u);
        QCOMPARE(frame.command, 0x10u);
        QCOMPARE(frame.payload.size(), 3u);
        QCOMPARE(frame.length, 3u);
    }
    
    void testCreateHeartbeatFrame() {
        Frame frame = createHeartbeatFrame(0x03);
        
        QCOMPARE(frame.deviceId, 0x03u);
        QCOMPARE(frame.command, static_cast<uint8_t>(Command::CMD_HEARTBEAT));
        QVERIFY(frame.payload.empty());
    }
    
    void testCreateTelemetryFrame() {
        TelemetryData telemetry;
        telemetry.timestamp = 5000;
        telemetry.channels = {1.5f, 2.5f};
        
        std::vector<uint8_t> encoded = createTelemetryFrame(0x02, telemetry);
        
        // 验证编码后的帧可以正确解码
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(result);
        QCOMPARE(decoded.deviceId, 0x02u);
        QCOMPARE(decoded.command, static_cast<uint8_t>(Command::CMD_TELEMETRY));
        
        // 验证遥测数据
        TelemetryData parsed = Protocol::parseTelemetry(decoded.payload);
        QCOMPARE(parsed.timestamp, 5000u);
        QCOMPARE(parsed.channels.size(), 2);
        QVERIFY(vectorsApproxEqual(parsed.channels, telemetry.channels));
    }
    
    // ========== 断言辅助测试 ==========
    
    void testVectorsApproxEqual_Same() {
        std::vector<float> a = {1.0f, 2.0f, 3.0f};
        std::vector<float> b = {1.0f, 2.0f, 3.0f};
        
        QVERIFY(vectorsApproxEqual(a, b));
    }
    
    void testVectorsApproxEqual_Different() {
        std::vector<float> a = {1.0f, 2.0f, 3.0f};
        std::vector<float> b = {1.0f, 2.0f, 4.0f};
        
        QVERIFY(!vectorsApproxEqual(a, b));
    }
    
    void testVectorsApproxEqual_WithinEpsilon() {
        std::vector<float> a = {1.0f, 2.0f, 3.0f};
        std::vector<float> b = {1.0005f, 2.0005f, 3.0005f};
        
        // 差值 0.0005，默认 epsilon 0.001，应该相等
        QVERIFY(vectorsApproxEqual(a, b));
        
        // 更小的 epsilon，应该不相等
        QVERIFY(!vectorsApproxEqual(a, b, 0.0001f));
    }
    
    void testVectorsApproxEqual_DifferentSizes() {
        std::vector<float> a = {1.0f, 2.0f};
        std::vector<float> b = {1.0f, 2.0f, 3.0f};
        
        QVERIFY(!vectorsApproxEqual(a, b));
    }
    
    // ========== 模拟数据生成测试 ==========
    
    void testGenerateSimulatedData() {
        DataProcessor processor;
        
        generateSimulatedData(processor, "sim_device", 50, 4);
        
        QVector<TelemetryData> data = processor.getData("sim_device");
        QCOMPARE(data.size(), 50);
        
        // 验证每个数据点有 4 个通道
        for (const auto& point : data) {
            QCOMPARE(point.channels.size(), 4);
        }
        
        // 验证时间戳递增
        for (int i = 1; i < data.size(); ++i) {
            QVERIFY(data[i].timestamp > data[i-1].timestamp);
        }
    }
    
    // ========== 调试工具测试 ==========
    
    void testFrameToString() {
        Frame frame = createHeartbeatFrame(0x01);
        QString str = frameToString(frame);
        
        QVERIFY(str.contains("deviceId=1"));
        QVERIFY(str.contains("command=0x01"));
    }
    
    void testFrameToString_WithPayload() {
        Frame frame = createTestFrame(0x01, 0x10, {0xAA, 0xBB, 0xCC});
        QString str = frameToString(frame);
        
        QVERIFY(str.contains("payload="));
        QVERIFY(str.contains("aa"));
        QVERIFY(str.contains("bb"));
        QVERIFY(str.contains("cc"));
    }
    
    void testBytesToHex() {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        QString hex = bytesToHex(data);
        
        QCOMPARE(hex, QString("01 02 03 04"));
    }
    
    void testBytesToHex_Truncation() {
        // 生成 50 字节的测试数据
        std::vector<uint8_t> data(50);
        for (int i = 0; i < 50; ++i) {
            data[i] = static_cast<uint8_t>(i);
        }
        
        QString hex = bytesToHex(data, 10);
        
        // 应该只显示前 10 个字节
        QVERIFY(hex.contains("00"));
        QVERIFY(hex.contains("09"));
        QVERIFY(!hex.contains("0a"));  // 第 11 个字节不应该显示
        QVERIFY(hex.contains("more bytes"));
    }
    
    // ========== 测试数据加载测试 ==========
    
    void testLoadTelemetrySamples() {
        // 验证测试数据文件存在且可解析
        QString dataPath = QFINDTESTDATA("data/telemetry_samples.json");
        QVERIFY(!dataPath.isEmpty());
        
        QFile file(dataPath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(doc.isArray());
        
        QJsonArray samples = doc.array();
        QVERIFY(samples.size() > 0);
        
        // 验证第一个样本的格式
        QJsonObject first = samples[0].toObject();
        QVERIFY(first.contains("timestamp"));
        QVERIFY(first.contains("channels"));
    }
    
    void testLoadProtocolFrames() {
        QString dataPath = QFINDTESTDATA("data/protocol_frames.json");
        QVERIFY(!dataPath.isEmpty());
        
        QFile file(dataPath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(doc.isObject());
        
        QJsonObject root = doc.object();
        QVERIFY(root.contains("valid_frames"));
        QVERIFY(root.contains("invalid_frames"));
    }
};

QTEST_APPLESS_MAIN(TestHelpersUnit)
#include "test_helpers.moc"
