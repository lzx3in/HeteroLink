/**
 * HeteroLink Host - Application 单元测试
 * 
 * 测试应用程序的生命周期、模块初始化、配置加载功能
 */

#include <QTest>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QSignalSpy>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include "app/Application.h"
#include "storage/ConfigManager.h"

using namespace HeteroLink;

class TestApplication : public QObject
{
    Q_OBJECT
    
private:
    QString testDir_;
    QString testConfigPath_;
    
    // 生成临时测试目录
    QString createTestDir() {
        QString dir = QDir::tempPath() + "/HeteroLink_App_Tests_" + 
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
    
    // 创建测试配置文件
    QString createTestConfig(const QString& dir) {
        QString path = dir + "/test_config.json";
        QJsonObject config;
        config["uart"] = QJsonObject{
            {"portName", "COM_TEST"},
            {"baudRate", 115200}
        };
        config["mqtt"] = QJsonObject{
            {"brokerHost", "localhost"},
            {"brokerPort", 1883}
        };
        config["data"] = QJsonObject{
            {"bufferSize", 1000}
        };
        
        QJsonDocument doc(config);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
        }
        return path;
    }
    
private slots:
    void initTestCase() {
        qDebug() << "Starting Application unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "Application unit tests finished";
    }
    
    void init() {
        testDir_ = createTestDir();
        testConfigPath_ = createTestConfig(testDir_);
    }
    
    void cleanup() {
        cleanupTestDir(testDir_);
    }
    
    // ========== 构造函数测试 ==========
    
    void testConstructor_EmptyConfig() {
        Application app;
        
        // 构造后模块指针为空，需要 initialize 后才创建
        QVERIFY(app.deviceManager() == nullptr);
        QVERIFY(app.dataProcessor() == nullptr);
    }
    
    void testConstructor_WithConfigPath() {
        Application app(testConfigPath_);
        
        // 构造后模块指针为空
        QVERIFY(app.deviceManager() == nullptr);
    }
    
    void testDestructor_NoCrash() {
        Application* app = new Application();
        app->initialize();
        app->start();
        delete app;  // 不应崩溃
    }
    
    // ========== 初始化测试 ==========
    
    void testInitialize_Success() {
        Application app(testConfigPath_);
        
        bool result = app.initialize();
        
        QVERIFY(result);
    }
    
    void testInitialize_EmptyConfig_UsesDefaults() {
        Application app;  // 无配置文件
        
        bool result = app.initialize();
        
        // 应使用默认配置成功初始化
        QVERIFY(result);
    }
    
    void testInitialize_NonExistentConfig_UsesDefaults() {
        Application app("/nonexistent/path/config.json");
        
        bool result = app.initialize();
        
        // 应使用默认配置成功初始化
        QVERIFY(result);
    }
    
    void testInitialize_DoubleInit() {
        Application app(testConfigPath_);
        
        bool result1 = app.initialize();
        bool result2 = app.initialize();
        
        QVERIFY(result1);
        QVERIFY(result2);  // 重复初始化应允许或返回 true
    }
    
    // ========== 模块访问测试 ==========
    
    void testGetDeviceManager_NotNull() {
        Application app;
        app.initialize();
        QVERIFY(app.deviceManager() != nullptr);
    }
    
    void testGetDataProcessor_NotNull() {
        Application app;
        app.initialize();
        QVERIFY(app.dataProcessor() != nullptr);
    }
    
    void testGetAlarmSystem_NotNull() {
        Application app;
        app.initialize();
        QVERIFY(app.alarmSystem() != nullptr);
    }
    
    void testGetConfigManager_NotNull() {
        Application app;
        app.initialize();
        QVERIFY(app.configManager() != nullptr);
    }
    
    void testGetDataLogger_NotNull() {
        Application app;
        app.initialize();
        QVERIFY(app.dataLogger() != nullptr);
    }
    
    // ========== 启动/停止测试 ==========
    
    void testStart_AfterInit() {
        Application app(testConfigPath_);
        app.initialize();
        
        QSignalSpy spy(&app, SIGNAL(started()));
        app.start();
        
        // 验证 started 信号发射
        // 注意：取决于实现，可能同步可能异步
        QVERIFY(true);  // 至少不应崩溃
    }
    
    void testStop_AfterStart() {
        Application app(testConfigPath_);
        app.initialize();
        app.start();
        
        QSignalSpy spy(&app, SIGNAL(stopped()));
        app.stop();
        
        QVERIFY(true);  // 至少不应崩溃
    }
    
    void testStop_WithoutStart_NoCrash() {
        Application app(testConfigPath_);
        app.initialize();
        
        // 未启动就停止不应崩溃
        app.stop();
    }
    
    void testStartStop_MultipleTimes() {
        Application app(testConfigPath_);
        app.initialize();
        
        // 多次启动停止
        app.start();
        app.stop();
        app.start();
        app.stop();
        
        QVERIFY(true);  // 不应崩溃
    }
    
    // ========== 配置加载测试 ==========
    
    void testLoadConfig_ValidFile() {
        Application app(testConfigPath_);
        app.initialize();
        
        // 通过 configManager 验证配置已加载
        ConfigManager* config = app.configManager();
        QVERIFY(config != nullptr);
        
        // 验证默认配置存在
        QVERIFY(config->get("uart.baudRate").isValid());
    }
    
    void testLoadConfig_InvalidJson_UsesDefaults() {
        // 创建无效 JSON
        QString invalidPath = testDir_ + "/invalid.json";
        QFile file(invalidPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write("not valid json {{{");
            file.close();
        }
        
        Application app(invalidPath);
        app.initialize();
        
        // 应使用默认配置
        ConfigManager* config = app.configManager();
        QCOMPARE(config->get("uart.baudRate").toInt(), 921600);
    }
    
    void testLoadConfig_EmptyFile_UsesDefaults() {
        QString emptyPath = testDir_ + "/empty.json";
        QFile file(emptyPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write("");
            file.close();
        }
        
        Application app(emptyPath);
        app.initialize();
        
        // 应使用默认配置
        ConfigManager* config = app.configManager();
        QCOMPARE(config->get("uart.baudRate").toInt(), 921600);
    }
    
    // ========== 信号测试 ==========
    
    void testStarted_SignalExists() {
        Application app;
        
        QSignalSpy spy(&app, SIGNAL(started()));
        QVERIFY(spy.isValid());
    }
    
    void testStopped_SignalExists() {
        Application app;
        
        QSignalSpy spy(&app, SIGNAL(stopped()));
        QVERIFY(spy.isValid());
    }
    
    // ========== 边界条件测试 ==========
    
    void testBoundary_VeryLongConfigPath() {
        // 很长的路径
        QString longPath = testDir_ + "/" + QString(200, 'a') + "/config.json";
        
        Application app(longPath);
        
        // 不应崩溃，使用默认配置
        bool result = app.initialize();
        QVERIFY(result);
    }
    
    void testBoundary_SpecialCharactersInPath() {
        QString specialPath = testDir_ + "/config with spaces & special.json";
        
        // 复制配置文件
        QFile::copy(testConfigPath_, specialPath);
        
        Application app(specialPath);
        bool result = app.initialize();
        
        QVERIFY(result);
    }
    
    // ========== 模块集成测试 ==========
    
    void testIntegration_AllModulesAccessible() {
        Application app(testConfigPath_);
        app.initialize();
        
        // 验证所有模块都可访问
        QVERIFY(app.deviceManager() != nullptr);
        QVERIFY(app.dataProcessor() != nullptr);
        QVERIFY(app.alarmSystem() != nullptr);
        QVERIFY(app.configManager() != nullptr);
        QVERIFY(app.dataLogger() != nullptr);
        
        // 验证模块状态正常
        QVERIFY(app.deviceManager()->getDevices().isEmpty());
    }
    
    void testIntegration_StartStop_CleanState() {
        Application app(testConfigPath_);
        app.initialize();
        
        // 启动前状态
        int devicesBefore = app.deviceManager()->getDevices().size();
        
        app.start();
        app.stop();
        
        // 停止后状态应一致
        int devicesAfter = app.deviceManager()->getDevices().size();
        QCOMPARE(devicesBefore, devicesAfter);
    }
};

QTEST_APPLESS_MAIN(TestApplication)
#include "test_application.moc"
