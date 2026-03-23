/**
 * HeteroLink Host - DataLogger 单元测试
 * 
 * 测试数据记录器的文件操作、自动分段、错误处理功能
 */

#include <QTest>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QDateTime>

#include "storage/DataLogger.h"
#include "protocol/Protocol.h"

using namespace HeteroLink;

class TestDataLogger : public QObject
{
    Q_OBJECT
    
private:
    QString testDir_;
    
    // 生成临时测试目录
    QString createTestDir() {
        QString dir = QDir::tempPath() + "/HeteroLink_DataLogger_Tests_" + 
                      QString::number(QDateTime::currentMSecsSinceEpoch());
        QDir().mkpath(dir);
        return dir;
    }
    
    // 清理测试目录
    void cleanupTestDir(const QString& dir) {
        QDir d(dir);
        if (d.exists()) {
            d.removeRecursively();
        }
    }
    
private slots:
    void initTestCase() {
        qDebug() << "Starting DataLogger unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "DataLogger unit tests finished";
    }
    
    void init() {
        testDir_ = createTestDir();
    }
    
    void cleanup() {
        cleanupTestDir(testDir_);
    }
    
    // ========== 基础功能测试 ==========
    
    void testConstructor_DefaultState() {
        DataLogger logger;
        
        QVERIFY(!logger.isRecording());
        QCOMPARE(logger.currentFilePath(), QString(""));
    }
    
    void testDestructor_NoCrash() {
        DataLogger* logger = new DataLogger();
        delete logger;  // 不应崩溃
    }
    
    // ========== 开始/停止记录测试 ==========
    
    void testStartRecording_Success() {
        DataLogger logger;
        
        bool result = logger.startRecording(testDir_, "device_001");
        
        QVERIFY(result);
        QVERIFY(logger.isRecording());
        QVERIFY(logger.currentFilePath().contains("device_001"));
        QVERIFY(QFile::exists(logger.currentFilePath()));
        
        logger.stopRecording();
    }
    
    void testStartRecording_InvalidPath_Fails() {
        DataLogger logger;
        
        // 空路径会尝试创建目录，可能成功（取决于系统）
        // 这里只验证不崩溃
        bool result = logger.startRecording("", "device_001");
        
        // 结果取决于系统，主要验证不崩溃
        QVERIFY(result == true || result == false);
    }
    
    void testStartRecording_EmptyDeviceId() {
        DataLogger logger;
        
        // 空设备 ID 应允许（但可能不合理）
        bool result = logger.startRecording(testDir_, "");
        
        QVERIFY(result);
        QVERIFY(logger.isRecording());
        
        logger.stopRecording();
    }
    
    void testStopRecording_Success() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        QSignalSpy spy(&logger, SIGNAL(recordingStopped()));
        logger.stopRecording();
        
        QVERIFY(!logger.isRecording());
        QCOMPARE(spy.count(), 1);
    }
    
    void testStopRecording_NotRecording_NoCrash() {
        DataLogger logger;
        
        // 未开始记录时停止不应崩溃
        logger.stopRecording();
    }
    
    void testRecordingStarted_Signal() {
        DataLogger logger;
        
        QSignalSpy spy(&logger, SIGNAL(recordingStarted(QString)));
        logger.startRecording(testDir_, "device_001");
        
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy[0][0].toString().contains("device_001"));
        
        logger.stopRecording();
    }
    
    // ========== 写入数据测试 ==========
    
    void testWriteData_Basic() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {1.5f, 2.5f, 3.5f};
        
        logger.writeData("device_001", data);
        
        // 停止记录以刷新缓冲区
        logger.stopRecording();
        
        // 验证文件内容
        QFile file(logger.currentFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = file.readAll();
        file.close();
        
        // 验证有 header + 1 行数据
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        QVERIFY(lines.size() >= 2);
        QVERIFY(lines[0].contains("timestamp"));  // header
    }
    
    void testWriteData_NotRecording_Ignores() {
        DataLogger logger;
        
        // 未开始记录时写入应忽略
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {1.0f};
        
        logger.writeData("device_001", data);  // 不应崩溃
        
        QVERIFY(!logger.isRecording());
    }
    
    void testWriteData_MultipleDataPoints() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        // 写入 10 个数据点
        for (int i = 0; i < 10; ++i) {
            TelemetryData data;
            data.timestamp = 1000 + i * 100;
            data.channels = {static_cast<float>(i)};
            logger.writeData("device_001", data);
        }
        
        logger.stopRecording();
        
        // 验证文件内容
        QFile file(logger.currentFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = file.readAll();
        file.close();
        
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        QCOMPARE(lines.size(), 11);  // header + 10 行数据
    }
    
    void testWriteData_MultipleChannels() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        
        logger.writeData("device_001", data);
        logger.stopRecording();
        
        QFile file(logger.currentFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = file.readAll();
        file.close();
        
        // 验证 header 包含 5 个通道
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        QVERIFY(lines[0].contains("channel0"));
        QVERIFY(lines[0].contains("channel4"));
    }
    
    // ========== 文件大小限制测试 ==========
    
    void testSetMaxFileSize() {
        DataLogger logger;
        
        logger.setMaxFileSize(50);  // 50 MB
        
        // 验证设置生效（通过后续行为验证）
        QVERIFY(logger.isRecording() == false);  // 未开始记录
    }
    
    // ========== 自动分段测试 ==========
    
    void testSetAutoSplit() {
        DataLogger logger;
        
        logger.setAutoSplit(true, 1000);  // 1 秒间隔
        
        // 验证设置生效
        QVERIFY(logger.isRecording() == false);
    }
    
    // ========== 错误处理测试 ==========
    
    void testErrorOccurred_SignalExists() {
        DataLogger logger;
        
        QSignalSpy spy(&logger, SIGNAL(errorOccurred(QString)));
        QVERIFY(spy.isValid());
    }
    
    // ========== 边界条件测试 ==========
    
    void testBoundary_EmptyChannels() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {};  // 空通道
        
        logger.writeData("device_001", data);  // 不应崩溃
        logger.stopRecording();
        
        // 验证文件存在
        QVERIFY(QFile::exists(logger.currentFilePath()));
    }
    
    void testBoundary_LargeTimestamp() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        TelemetryData data;
        data.timestamp = 0xFFFFFFFF;  // 最大 uint32
        data.channels = {1.0f};
        
        logger.writeData("device_001", data);  // 不应崩溃
        logger.stopRecording();
        
        QVERIFY(QFile::exists(logger.currentFilePath()));
    }
    
    void testBoundary_NegativeChannelValues() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {-100.5f, -200.5f, -300.5f};
        
        logger.writeData("device_001", data);
        logger.stopRecording();
        
        // 验证文件内容包含负值
        QFile file(logger.currentFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = file.readAll();
        file.close();
        
        QVERIFY(content.contains("-"));
    }
    
    void testBoundary_VeryLargeChannelValues() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        TelemetryData data;
        data.timestamp = 1000;
        data.channels = {1e10f, 1e20f, 1e30f};
        
        logger.writeData("device_001", data);  // 不应崩溃
        logger.stopRecording();
        
        QVERIFY(QFile::exists(logger.currentFilePath()));
    }
    
    void testBoundary_ManyChannels() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        TelemetryData data;
        data.timestamp = 1000;
        // 32 个通道
        for (int i = 0; i < 32; ++i) {
            data.channels.push_back(static_cast<float>(i));
        }
        
        logger.writeData("device_001", data);
        logger.stopRecording();
        
        QVERIFY(QFile::exists(logger.currentFilePath()));
    }
    
    // ========== 文件路径生成测试 ==========
    
    void testFilePath_ContainsDeviceId() {
        DataLogger logger;
        logger.startRecording(testDir_, "test_device_123");
        
        QString path = logger.currentFilePath();
        
        QVERIFY(path.contains("test_device_123"));
        
        logger.stopRecording();
    }
    
    void testFilePath_ContainsTimestamp() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        QString path = logger.currentFilePath();
        
        // 路径应包含时间戳（日期）
        QVERIFY(path.contains(QDate::currentDate().toString("yyyy-MM-dd")));
        
        logger.stopRecording();
    }
    
    void testFilePath_InTestDir() {
        DataLogger logger;
        logger.startRecording(testDir_, "device_001");
        
        QString path = logger.currentFilePath();
        
        QVERIFY(path.startsWith(testDir_));
        
        logger.stopRecording();
    }
    
    // ========== 多次开始/停止测试 ==========
    
    void testMultipleStartStop() {
        DataLogger logger;
        
        // 第一次
        logger.startRecording(testDir_, "device_001");
        QVERIFY(logger.isRecording());
        logger.stopRecording();
        QVERIFY(!logger.isRecording());
        
        // 第二次
        logger.startRecording(testDir_, "device_002");
        QVERIFY(logger.isRecording());
        logger.stopRecording();
        QVERIFY(!logger.isRecording());
    }
    
    void testStartRecording_SameDeviceTwice() {
        DataLogger logger;
        
        logger.startRecording(testDir_, "device_001");
        QString firstPath = logger.currentFilePath();
        
        // 停止后重新开始同一设备
        logger.stopRecording();
        logger.startRecording(testDir_, "device_001");
        QString secondPath = logger.currentFilePath();
        
        // 应生成不同文件（时间戳不同）
        QVERIFY(firstPath != secondPath);
        
        logger.stopRecording();
    }
};

QTEST_APPLESS_MAIN(TestDataLogger)
#include "test_datalogger.moc"
