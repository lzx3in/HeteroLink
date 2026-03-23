/**
 * HeteroLink Host - 数据处理器单元测试
 * 
 * 测试 DataProcessor 类的数据缓存、统计、导出功能
 */

#include <QTest>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSignalSpy>

#include "core/DataProcessor.h"

using namespace HeteroLink;

class TestDataProcessor : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting DataProcessor unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "DataProcessor unit tests finished";
    }
    
    void cleanup() {
        // 清理测试文件
        QDir dir(QDir::tempPath());
        dir.remove("test_export.csv");
        dir.remove("test_export.json");
    }
    
    // ========== 基本操作测试 ==========
    
    void testAddData_SinglePoint() {
        DataProcessor processor;
        
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {1.5f, 2.5f, 3.5f};
        
        processor.addData("device1", data);
        
        QVector<TelemetryData> retrieved = processor.getData("device1");
        QCOMPARE(retrieved.size(), 1);
        QCOMPARE(retrieved[0].timestamp, 1000u);
        QCOMPARE(retrieved[0].channels.size(), 3u);
    }
    
    void testAddData_MultiplePoints() {
        DataProcessor processor;
        
        // 添加 10 个数据点
        for (int i = 0; i < 10; ++i) {
            TelemetryData data;
            data.timestamp = 1000 + i * 100;
            data.channels = {static_cast<float>(i)};
            processor.addData("device1", data);
        }
        
        QVector<TelemetryData> retrieved = processor.getData("device1");
        QCOMPARE(retrieved.size(), 10);
        QCOMPARE(retrieved[0].timestamp, 1000u);
        QCOMPARE(retrieved[9].timestamp, 1900u);
    }
    
    void testAddData_MultipleDevices() {
        DataProcessor processor;
        
        TelemetryData data1, data2;
        data1.timestamp = 1000;
        data1.channels = {1.0f};
        data2.timestamp = 2000;
        data2.channels = {2.0f};
        
        processor.addData("device1", data1);
        processor.addData("device2", data2);
        
        QVector<TelemetryData> dev1Data = processor.getData("device1");
        QVector<TelemetryData> dev2Data = processor.getData("device2");
        
        QCOMPARE(dev1Data.size(), 1);
        QCOMPARE(dev2Data.size(), 1);
        QCOMPARE(dev1Data[0].timestamp, 1000u);
        QCOMPARE(dev2Data[0].timestamp, 2000u);
    }
    
    // ========== 缓冲区管理测试 ==========
    
    void testBufferSize_LimitEnforced() {
        DataProcessor processor;
        processor.setBufferSize("device1", 5);
        
        // 添加 10 个数据点（超过缓冲区限制）
        for (int i = 0; i < 10; ++i) {
            TelemetryData data;
            data.timestamp = 1000 + i * 100;
            data.channels = {static_cast<float>(i)};
            processor.addData("device1", data);
        }
        
        QVector<TelemetryData> retrieved = processor.getData("device1");
        QCOMPARE(retrieved.size(), 5);  // 应只保留最后 5 个
        QCOMPARE(retrieved[0].timestamp, 1500u);  // 从第 6 个开始
        QCOMPARE(retrieved[4].timestamp, 1900u);
    }
    
    void testBufferSize_PerDevice() {
        DataProcessor processor;
        processor.setBufferSize("device1", 5);
        processor.setBufferSize("device2", 10);
        
        // 两个设备都添加 15 个数据点
        for (int i = 0; i < 15; ++i) {
            TelemetryData data1, data2;
            data1.timestamp = 1000 + i * 100;
            data1.channels = {static_cast<float>(i)};
            data2.timestamp = 2000 + i * 100;
            data2.channels = {static_cast<float>(i * 2)};
            processor.addData("device1", data1);
            processor.addData("device2", data2);
        }
        
        QCOMPARE(processor.getData("device1").size(), 5);
        QCOMPARE(processor.getData("device2").size(), 10);
    }
    
    void testGetLatestData() {
        DataProcessor processor;
        
        // 添加 20 个数据点 (时间戳：1000, 1100, ..., 2900)
        for (int i = 0; i < 20; ++i) {
            TelemetryData data;
            data.timestamp = 1000 + i * 100;
            data.channels = {static_cast<float>(i)};
            processor.addData("device1", data);
        }
        
        // 获取最新 5 个 (应该是最后 5 个：1500, 1600, 1700, 1800, 1900 的索引是 5,6,7,8,9...不对)
        // 实际 buffer 是队列，添加顺序是 0-19，最新 5 个是索引 15-19，时间戳 2500-2900
        QVector<TelemetryData> latest = processor.getLatestData("device1", 5);
        QCOMPARE(latest.size(), 5);
        QCOMPARE(latest[0].timestamp, 2500u);  // 最新 5 个从 2500 开始 (i=15)
        QCOMPARE(latest[4].timestamp, 2900u);  // 最后一个是 2900 (i=19)
        
        // 获取最新 100 个（超过总数）
        QVector<TelemetryData> latestAll = processor.getLatestData("device1", 100);
        QCOMPARE(latestAll.size(), 20);  // 应返回全部
    }
    
    // ========== 统计数据测试 ==========
    
    void testStats_SingleChannel() {
        DataProcessor processor;
        
        // 添加已知数据
        for (int i = 1; i <= 10; ++i) {
            TelemetryData data;
            data.timestamp = i * 100;
            data.channels = {static_cast<float>(i)};  // 1.0, 2.0, ..., 10.0
            processor.addData("device1", data);
        }
        
        QMap<int, ChannelStats> stats = processor.getStats("device1");
        
        QVERIFY(stats.contains(0));  // 通道 0
        ChannelStats ch0Stats = stats[0];
        
        QCOMPARE(ch0Stats.sampleCount, 10u);
        QCOMPARE(ch0Stats.min, 1.0f);
        QCOMPARE(ch0Stats.max, 10.0f);
        QCOMPARE(ch0Stats.avg, 5.5f);  // (1+2+...+10)/10 = 5.5
    }
    
    void testStats_MultipleChannels() {
        DataProcessor processor;
        
        // 添加 3 通道数据
        for (int i = 0; i < 5; ++i) {
            TelemetryData data;
            data.timestamp = i * 100;
            data.channels = {
                static_cast<float>(i + 1),      // 通道 0: 1,2,3,4,5
                static_cast<float>((i + 1) * 2), // 通道 1: 2,4,6,8,10
                static_cast<float>((i + 1) * 10) // 通道 2: 10,20,30,40,50
            };
            processor.addData("device1", data);
        }
        
        QMap<int, ChannelStats> stats = processor.getStats("device1");
        
        QCOMPARE(stats.size(), 3);
        
        // 通道 0: avg = 3.0
        QVERIFY(qAbs(stats[0].avg - 3.0f) < 0.001f);
        QCOMPARE(stats[0].min, 1.0f);
        QCOMPARE(stats[0].max, 5.0f);
        
        // 通道 1: avg = 6.0
        QVERIFY(qAbs(stats[1].avg - 6.0f) < 0.001f);
        QCOMPARE(stats[1].min, 2.0f);
        QCOMPARE(stats[1].max, 10.0f);
        
        // 通道 2: avg = 30.0
        QVERIFY(qAbs(stats[2].avg - 30.0f) < 0.001f);
        QCOMPARE(stats[2].min, 10.0f);
        QCOMPARE(stats[2].max, 50.0f);
    }
    
    void testStats_EmptyData() {
        DataProcessor processor;
        
        QMap<int, ChannelStats> stats = processor.getStats("nonexistent");
        
        QVERIFY(stats.isEmpty());
    }
    
    void testStats_AfterClear() {
        DataProcessor processor;
        
        // 添加数据
        for (int i = 0; i < 5; ++i) {
            TelemetryData data;
            data.timestamp = i * 100;
            data.channels = {static_cast<float>(i)};
            processor.addData("device1", data);
        }
        
        // 清除数据
        processor.clearData("device1");
        
        QMap<int, ChannelStats> stats = processor.getStats("device1");
        QVERIFY(stats.isEmpty());
    }
    
    // ========== 清除操作测试 ==========
    
    void testClearData_SingleDevice() {
        DataProcessor processor;
        
        processor.addData("device1", TelemetryData{});
        processor.addData("device2", TelemetryData{});
        
        processor.clearData("device1");
        
        QVERIFY(processor.getData("device1").isEmpty());
        QCOMPARE(processor.getData("device2").size(), 1);
    }
    
    void testClearAll() {
        DataProcessor processor;
        
        processor.addData("device1", TelemetryData{});
        processor.addData("device2", TelemetryData{});
        processor.addData("device3", TelemetryData{});
        
        processor.clearAll();
        
        QVERIFY(processor.getData("device1").isEmpty());
        QVERIFY(processor.getData("device2").isEmpty());
        QVERIFY(processor.getData("device3").isEmpty());
    }
    
    // ========== 数据导出测试 ==========
    
    void testExportToCsv_Basic() {
        DataProcessor processor;
        
        // 添加测试数据
        for (int i = 0; i < 5; ++i) {
            TelemetryData data;
            data.timestamp = 1000 + i * 100;
            data.channels = {static_cast<float>(i + 1), static_cast<float>((i + 1) * 2)};
            processor.addData("device1", data);
        }
        
        QString testFile = QDir::tempPath() + "/test_export.csv";
        bool result = processor.exportToCsv("device1", testFile);
        
        QVERIFY(result);
        QVERIFY(QFile::exists(testFile));
        
        // 验证文件内容
        QFile file(testFile);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = file.readAll();
        file.close();
        
        // 验证 CSV 格式
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        QVERIFY(lines.size() >= 6);  // 1 行 header + 5 行数据
        
        // 验证 header (实际格式：timestamp,channel0,channel1,...)
        QVERIFY(lines[0].contains("timestamp"));
        QVERIFY(lines[0].contains("channel"));
        
        // 清理
        QFile::remove(testFile);
    }
    
    void testExportToCsv_NonExistentDevice() {
        DataProcessor processor;
        
        QString testFile = QDir::tempPath() + "/test_export.csv";
        bool result = processor.exportToCsv("nonexistent", testFile);
        
        QVERIFY(!result);  // 应失败
    }
    
    void testExportToJson_Basic() {
        DataProcessor processor;
        
        // 添加测试数据
        for (int i = 0; i < 3; ++i) {
            TelemetryData data;
            data.timestamp = 1000 + i * 100;
            data.channels = {static_cast<float>(i + 1)};
            processor.addData("device1", data);
        }
        
        QString testFile = QDir::tempPath() + "/test_export.json";
        bool result = processor.exportToJson("device1", testFile);
        
        QVERIFY(result);
        QVERIFY(QFile::exists(testFile));
        
        // 验证 JSON 格式
        QFile file(testFile);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        file.close();
        
        QVERIFY(error.error == QJsonParseError::NoError);
        QVERIFY(doc.isArray());  // JSON 导出的是数组
        
        // 清理
        QFile::remove(testFile);
    }
    
    void testExportToJson_NonExistentDevice() {
        DataProcessor processor;
        
        QString testFile = QDir::tempPath() + "/test_export.json";
        bool result = processor.exportToJson("nonexistent", testFile);
        
        QVERIFY(!result);
    }
    
    // ========== 信号测试 ==========
    
    void testDataUpdated_Signal() {
        DataProcessor processor;
        
        QSignalSpy spy(&processor, SIGNAL(dataUpdated(QString)));
        
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {1.0f};
        processor.addData("device1", data);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toString(), QString("device1"));
    }
    
    void testStatsUpdated_Signal() {
        DataProcessor processor;
        
        QSignalSpy spy(&processor, SIGNAL(statsUpdated(QString)));
        
        // 添加多个数据点以触发统计更新
        for (int i = 0; i < 5; ++i) {
            TelemetryData data;
            data.timestamp = i * 100;
            data.channels = {static_cast<float>(i)};
            processor.addData("device1", data);
        }
        
        // 统计应在每次添加时更新
        QVERIFY(spy.count() >= 1);
    }
};

QTEST_APPLESS_MAIN(TestDataProcessor)
#include "test_dataprocessor.moc"
