/**
 * HeteroLink Host - ConfigManager 单元测试
 * 
 * 测试配置管理器的加载、保存、读写、嵌套配置等功能
 */

#include <QTest>
#include <QSignalSpy>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

#include "storage/ConfigManager.h"

using namespace HeteroLink;

class TestConfigManager : public QObject
{
    Q_OBJECT
    
private:
    QString testConfigPath_;
    
    // 生成唯一的测试配置文件路径
    QString getTestConfigPath(const QString& testName) {
        QString tempDir = QDir::tempPath() + "/HeteroLinkTests";
        QDir().mkpath(tempDir);
        return tempDir + "/config_" + testName + "_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".json";
    }
    
    // 清理测试文件
    void cleanupTestFile() {
        if (!testConfigPath_.isEmpty() && QFile::exists(testConfigPath_)) {
            QFile::remove(testConfigPath_);
        }
    }
    
private slots:
    void initTestCase() {
        qDebug() << "Starting ConfigManager unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "ConfigManager unit tests finished";
        // 清理所有测试生成的临时文件
        QString tempDir = QDir::tempPath() + "/HeteroLinkTests";
        QDir dir(tempDir);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
    
    void init() {
        // 每个测试前生成新的测试路径
        testConfigPath_ = getTestConfigPath(QTest::currentTestFunction());
    }
    
    void cleanup() {
        // 每个测试后清理
        cleanupTestFile();
    }
    
    // ========== 构造和默认配置测试 ==========
    
    void testConstructor_DefaultState() {
        // 测试构造后的初始状态
        ConfigManager config;
        
        // 新创建的 ConfigManager 应该是空的（未加载配置）
        // get 应该返回默认值
        QCOMPARE(config.get("uart.baudRate").isValid(), false);
    }
    
    void testGetDefaultConfig_HasAllSections() {
        // 测试默认配置包含所有必需的节
        QVariantMap defaultConfig = ConfigManager::getDefaultConfig();
        
        QVERIFY(defaultConfig.contains("uart"));
        QVERIFY(defaultConfig.contains("mqtt"));
        QVERIFY(defaultConfig.contains("data"));
        QVERIFY(defaultConfig.contains("alarm"));
        QVERIFY(defaultConfig.contains("ui"));
        
        // 验证 UART 默认配置
        QVariantMap uart = defaultConfig["uart"].toMap();
        QCOMPARE(uart["baudRate"].toInt(), 921600);
        QCOMPARE(uart["portName"].toString(), QString(""));
        
        // 验证 MQTT 默认配置
        QVariantMap mqtt = defaultConfig["mqtt"].toMap();
        QCOMPARE(mqtt["brokerHost"].toString(), "localhost");
        QCOMPARE(mqtt["brokerPort"].toInt(), 1883);
        QCOMPARE(mqtt["useTls"].toBool(), false);
        
        // 验证数据配置
        QVariantMap data = defaultConfig["data"].toMap();
        QCOMPARE(data["bufferSize"].toInt(), 10000);
        QCOMPARE(data["autoExport"].toBool(), false);
        
        // 验证告警配置
        QVariantMap alarm = defaultConfig["alarm"].toMap();
        QCOMPARE(alarm["enabled"].toBool(), true);
        QCOMPARE(alarm["lowerLimit"].toInt(), -1000);
        QCOMPARE(alarm["upperLimit"].toInt(), 1000);
        
        // 验证 UI 配置
        QVariantMap ui = defaultConfig["ui"].toMap();
        QCOMPARE(ui["theme"].toString(), "dark");
        QCOMPARE(ui["language"].toString(), "zh-CN");
    }
    
    // ========== 加载配置测试 ==========
    
    void testLoad_NonExistentFile_UsesDefaults() {
        // 加载不存在的文件应使用默认配置并返回 true
        ConfigManager config;
        QString nonExistentPath = "/nonexistent/path/config_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".json";
        
        bool result = config.load(nonExistentPath);
        
        QVERIFY(result);  // 应该成功（使用默认值）
        
        // 验证加载了默认值
        QCOMPARE(config.get("uart.baudRate").toInt(), 921600);
        QCOMPARE(config.get("mqtt.brokerHost").toString(), "localhost");
    }
    
    void testLoad_ValidJsonFile() {
        // 创建有效的测试配置文件
        QVariantMap testConfig;
        testConfig["testKey"] = "testValue";
        testConfig["testNumber"] = 42;
        
        QJsonDocument doc(QJsonObject::fromVariantMap(testConfig));
        QFile file(testConfigPath_);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(doc.toJson());
        file.close();
        
        // 加载配置
        ConfigManager config;
        bool result = config.load(testConfigPath_);
        
        QVERIFY(result);
        QCOMPARE(config.get("testKey").toString(), QString("testValue"));
        QCOMPARE(config.get("testNumber").toInt(), 42);
    }
    
    void testLoad_InvalidJsonFile_UsesDefaults() {
        // 创建无效的 JSON 文件
        QFile file(testConfigPath_);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("this is not valid json {{{");
        file.close();
        
        // 加载应返回 false 但使用默认配置
        ConfigManager config;
        bool result = config.load(testConfigPath_);
        
        QVERIFY(!result);  // 解析失败
        
        // 但仍应能访问默认值
        QCOMPARE(config.get("uart.baudRate").toInt(), 921600);
    }
    
    void testLoad_NestedConfiguration() {
        // 测试加载嵌套配置
        QVariantMap testConfig;
        QVariantMap level1;
        QVariantMap level2;
        level2["deepKey"] = "deepValue";
        level1["level2"] = level2;
        level1["simpleKey"] = "simpleValue";
        testConfig["level1"] = level1;
        
        QJsonDocument doc(QJsonObject::fromVariantMap(testConfig));
        QFile file(testConfigPath_);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(doc.toJson());
        file.close();
        
        ConfigManager config;
        config.load(testConfigPath_);
        
        QCOMPARE(config.get("level1.simpleKey").toString(), QString("simpleValue"));
        QCOMPARE(config.get("level1.level2.deepKey").toString(), QString("deepValue"));
    }
    
    // ========== 保存配置测试 ==========
    
    void testSave_ToFile() {
        ConfigManager config;
        config.set("testKey", "testValue");
        config.set("nested.value", 123);
        
        bool result = config.save(testConfigPath_);
        
        QVERIFY(result);
        QVERIFY(QFile::exists(testConfigPath_));
        
        // 验证保存的内容
        QFile file(testConfigPath_);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        QVERIFY(error.error == QJsonParseError::NoError);
        
        QVariantMap loaded = doc.object().toVariantMap();
        QCOMPARE(loaded["testKey"].toString(), QString("testValue"));
        QCOMPARE(loaded["nested"].toMap()["value"].toInt(), 123);
    }
    
    void testSave_EmptyConfigPath_Fails() {
        ConfigManager config;
        // 未设置配置路径时保存应失败
        bool result = config.save("");
        
        QVERIFY(!result);
    }
    
    void testSaveAndLoad_RoundTrip() {
        // 保存后重新加载应得到相同数据
        ConfigManager config1;
        config1.set("key1", "value1");
        config1.set("key2", 42);
        config1.set("nested.deep.value", 3.14);
        config1.save(testConfigPath_);
        
        ConfigManager config2;
        config2.load(testConfigPath_);
        
        QCOMPARE(config2.get("key1").toString(), QString("value1"));
        QCOMPARE(config2.get("key2").toInt(), 42);
        QCOMPARE(config2.get("nested.deep.value").toDouble(), 3.14);
    }
    
    // ========== Get/Set 测试 ==========
    
    void testGet_SimpleKey() {
        ConfigManager config;
        config.set("simpleKey", "simpleValue");
        
        QCOMPARE(config.get("simpleKey").toString(), QString("simpleValue"));
    }
    
    void testGet_NestedKey() {
        ConfigManager config;
        config.set("level1.level2.level3", "deepValue");
        
        QCOMPARE(config.get("level1.level2.level3").toString(), QString("deepValue"));
    }
    
    void testGet_NonExistentKey_ReturnsDefault() {
        ConfigManager config;
        
        // 不存在的键应返回默认值
        QCOMPARE(config.get("nonExistent").isValid(), false);
        QCOMPARE(config.get("nonExistent", "defaultValue").toString(), QString("defaultValue"));
        QCOMPARE(config.get("nonExistent", 42).toInt(), 42);
    }
    
    void testGet_PartialNestedKey_ReturnsDefault() {
        ConfigManager config;
        config.set("level1.existing", "value");
        
        // 部分存在但不完整的嵌套键应返回默认值
        QCOMPARE(config.get("level1.nonExistent").isValid(), false);
        QCOMPARE(config.get("level1.level2.level3", "default").toString(), QString("default"));
    }
    
    void testSet_OverwriteExisting() {
        ConfigManager config;
        config.set("key", "first");
        QCOMPARE(config.get("key").toString(), QString("first"));
        
        config.set("key", "second");
        QCOMPARE(config.get("key").toString(), QString("second"));
    }
    
    void testSet_VariousTypes() {
        ConfigManager config;
        
        config.set("stringVal", "hello");
        config.set("intVal", 42);
        config.set("doubleVal", 3.14);
        config.set("boolVal", true);
        
        QCOMPARE(config.get("stringVal").toString(), QString("hello"));
        QCOMPARE(config.get("intVal").toInt(), 42);
        QCOMPARE(config.get("doubleVal").toDouble(), 3.14);
        QCOMPARE(config.get("boolVal").toBool(), true);
    }
    
    void testSet_NestedCreatesStructure() {
        ConfigManager config;
        
        // 设置深层嵌套键应自动创建中间结构
        config.set("a.b.c.d.e", "deep");
        
        QCOMPARE(config.get("a.b.c.d.e").toString(), QString("deep"));
        QVERIFY(config.get("a.b.c.d").toMap().contains("e"));
    }
    
    // ========== Reset 测试 ==========
    
    void testReset_RestoresDefaults() {
        ConfigManager config;
        
        // 修改一些配置
        config.set("uart.baudRate", 115200);
        config.set("mqtt.brokerHost", "custom-broker");
        config.set("customKey", "customValue");
        
        // 重置
        config.reset();
        
        // 应恢复默认值
        QCOMPARE(config.get("uart.baudRate").toInt(), 921600);
        QCOMPARE(config.get("mqtt.brokerHost").toString(), QString("localhost"));
        
        // 自定义键应被清除
        QCOMPARE(config.get("customKey").isValid(), false);
    }
    
    // ========== 信号测试 ==========
    
    void testConfigChanged_SignalEmitted() {
        ConfigManager config;
        
        QSignalSpy spy(&config, SIGNAL(configChanged(QString, QVariant)));
        
        config.set("testKey", "testValue");
        
        QCOMPARE(spy.count(), 1);
        
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments[0].toString(), QString("testKey"));
        QCOMPARE(arguments[1].toString(), QString("testValue"));
    }
    
    void testConfigChanged_NestedKey() {
        ConfigManager config;
        
        QSignalSpy spy(&config, SIGNAL(configChanged(QString, QVariant)));
        
        config.set("nested.key", "value");
        
        QCOMPARE(spy.count(), 1);
        
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments[0].toString(), QString("nested.key"));
        QCOMPARE(arguments[1].toString(), QString("value"));
    }
    
    void testConfigChanged_ResetEmitsEmpty() {
        ConfigManager config;
        
        QSignalSpy spy(&config, SIGNAL(configChanged(QString, QVariant)));
        
        config.reset();
        
        QCOMPARE(spy.count(), 1);
        
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments[0].toString(), QString(""));
        QVERIFY(!arguments[1].isValid());
    }
    
    // ========== 边界情况测试 ==========
    
    void testGet_EmptyKey() {
        ConfigManager config;
        config.set("", "emptyKeyValue");
        
        // 空键的处理
        QVariant value = config.get("");
        // 空键可能返回默认值或无效值，取决于实现
        QVERIFY(!value.isValid() || value.toString() == "emptyKeyValue");
    }
    
    void testSet_EmptyKey() {
        ConfigManager config;
        
        // 设置空键不应崩溃
        config.set("", "value");
        
        // 验证行为（可能忽略或特殊处理）
        QVERIFY(true);  // 主要是测试不崩溃
    }
    
    void testLoad_EmptyFilePath_UsesStandardLocation() {
        ConfigManager config;
        
        // 空路径应使用标准配置位置
        // 这个测试主要验证不崩溃且能正常工作
        bool result = config.load("");
        
        // 即使文件不存在也应成功（使用默认值）
        QVERIFY(result);
        
        // 验证默认值可用
        QCOMPARE(config.get("uart.baudRate").toInt(), 921600);
    }
    
    void testConcurrentGetSet() {
        // 测试快速连续的读写操作
        ConfigManager config;
        
        for (int i = 0; i < 100; ++i) {
            QString key = "key" + QString::number(i);
            config.set(key, i);
            QCOMPARE(config.get(key).toInt(), i);
        }
        
        // 验证所有值都正确
        for (int i = 0; i < 100; ++i) {
            QString key = "key" + QString::number(i);
            QCOMPARE(config.get(key).toInt(), i);
        }
    }
    
    // ========== 实际使用场景测试 ==========
    
    void testRealWorld_UartConfiguration() {
        ConfigManager config;
        
        // 模拟实际的 UART 配置场景
        config.set("uart.portName", "COM3");
        config.set("uart.baudRate", 115200);
        config.set("uart.dataBits", 8);
        config.set("uart.parity", "none");
        config.set("uart.stopBits", 1);
        
        QCOMPARE(config.get("uart.portName").toString(), QString("COM3"));
        QCOMPARE(config.get("uart.baudRate").toInt(), 115200);
        QCOMPARE(config.get("uart.dataBits").toInt(), 8);
        QCOMPARE(config.get("uart.parity").toString(), QString("none"));
        QCOMPARE(config.get("uart.stopBits").toInt(), 1);
        
        // 保存并重新加载
        config.save(testConfigPath_);
        
        ConfigManager config2;
        config2.load(testConfigPath_);
        
        QCOMPARE(config2.get("uart.portName").toString(), QString("COM3"));
        QCOMPARE(config2.get("uart.baudRate").toInt(), 115200);
    }
    
    void testRealWorld_MqttConfiguration() {
        ConfigManager config;
        
        // 模拟实际的 MQTT 配置场景
        config.set("mqtt.brokerHost", "mqtt.example.com");
        config.set("mqtt.brokerPort", 8883);
        config.set("mqtt.username", "user123");
        config.set("mqtt.password", "secret");
        config.set("mqtt.clientId", "heterolink-device-001");
        config.set("mqtt.useTls", true);
        
        QCOMPARE(config.get("mqtt.brokerHost").toString(), QString("mqtt.example.com"));
        QCOMPARE(config.get("mqtt.brokerPort").toInt(), 8883);
        QCOMPARE(config.get("mqtt.username").toString(), QString("user123"));
        QCOMPARE(config.get("mqtt.password").toString(), QString("secret"));
        QCOMPARE(config.get("mqtt.clientId").toString(), QString("heterolink-device-001"));
        QCOMPARE(config.get("mqtt.useTls").toBool(), true);
    }
    
    void testRealWorld_AlarmThresholds() {
        ConfigManager config;
        
        // 模拟实际的告警阈值配置
        config.set("alarm.enabled", true);
        config.set("alarm.lowerLimit", -500);
        config.set("alarm.upperLimit", 500);
        config.set("alarm.level", "critical");
        config.set("alarm.emailNotifications", true);
        
        QCOMPARE(config.get("alarm.enabled").toBool(), true);
        QCOMPARE(config.get("alarm.lowerLimit").toInt(), -500);
        QCOMPARE(config.get("alarm.upperLimit").toInt(), 500);
        QCOMPARE(config.get("alarm.level").toString(), QString("critical"));
        QCOMPARE(config.get("alarm.emailNotifications").toBool(), true);
    }
};

QTEST_APPLESS_MAIN(TestConfigManager)
#include "test_configmanager.moc"
