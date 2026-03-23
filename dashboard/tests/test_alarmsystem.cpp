/**
 * HeteroLink Host - 告警系统单元测试
 * 
 * 测试 AlarmSystem 类的阈值监控、告警触发、记录管理功能
 */

#include <QTest>
#include <QDebug>
#include <QSignalSpy>

#include "core/AlarmSystem.h"
#include "protocol/Protocol.h"

using namespace HeteroLink;

class TestAlarmSystem : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting AlarmSystem unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "AlarmSystem unit tests finished";
    }
    
    // ========== 基础功能测试 ==========
    
    void testConstructor() {
        // 测试构造函数
        AlarmSystem system;
        
        // 初始应该没有告警配置
        QVERIFY(system.getAlarms("device001").isEmpty());
    }
    
    void testDestructor_NoCrash() {
        // 测试析构函数不会崩溃
        AlarmSystem* system = new AlarmSystem();
        // 添加一些配置再删除
        delete system;
    }
    
    void testInitialState_EmptyAlarms() {
        // 测试初始状态
        AlarmSystem system;
        
        QVERIFY(system.getAlarms("device001").isEmpty());
        QVERIFY(system.getAlarmRecords("device001").isEmpty());
        QVERIFY(system.getAllAlarmRecords().isEmpty());
    }
    
    // ========== 告警级别枚举测试 ==========
    
    void testAlarmLevel_Values() {
        // 测试告警级别枚举值
        QCOMPARE(static_cast<int>(AlarmLevel::INFO), 0);
        QCOMPARE(static_cast<int>(AlarmLevel::WARNING), 1);
        QCOMPARE(static_cast<int>(AlarmLevel::CRITICAL), 2);
    }
    
    void testAlarmLevel_Assignment() {
        // 测试告警级别赋值
        AlarmLevel level = AlarmLevel::WARNING;
        QCOMPARE(level, AlarmLevel::WARNING);
        
        level = AlarmLevel::CRITICAL;
        QCOMPARE(level, AlarmLevel::CRITICAL);
    }
    
    // ========== 告警配置测试 ==========
    
    void testAlarmConfig_DefaultValues() {
        // 测试告警配置默认值
        AlarmConfig config;
        
        QCOMPARE(config.channelId, 0);
        QCOMPARE(config.lowerLimit, 0.0f);
        QCOMPARE(config.upperLimit, 0.0f);
        QCOMPARE(config.lowerEnabled, false);
        QCOMPARE(config.upperEnabled, false);
        QCOMPARE(config.level, AlarmLevel::WARNING);
        QCOMPARE(config.enabled, true);
    }
    
    void testAlarmConfig_CustomValues() {
        // 测试告警配置自定义值
        AlarmConfig config;
        config.channelId = 2;
        config.lowerLimit = 10.0f;
        config.upperLimit = 90.0f;
        config.lowerEnabled = true;
        config.upperEnabled = true;
        config.level = AlarmLevel::CRITICAL;
        config.enabled = true;
        
        QCOMPARE(config.channelId, 2);
        QCOMPARE(config.lowerLimit, 10.0f);
        QCOMPARE(config.upperLimit, 90.0f);
        QCOMPARE(config.lowerEnabled, true);
        QCOMPARE(config.upperEnabled, true);
        QCOMPARE(config.level, AlarmLevel::CRITICAL);
    }
    
    void testConfigureAlarm_Success() {
        // 测试成功配置告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.lowerLimit = 10.0f;
        config.upperLimit = 90.0f;
        config.lowerEnabled = true;
        config.upperEnabled = true;
        
        system.configureAlarm("device001", config);
        
        QVector<AlarmConfig> alarms = system.getAlarms("device001");
        QCOMPARE(alarms.size(), 1);
        QCOMPARE(alarms[0].channelId, 0);
        QCOMPARE(alarms[0].lowerLimit, 10.0f);
        QCOMPARE(alarms[0].upperLimit, 90.0f);
    }
    
    void testConfigureAlarm_UpdateExisting() {
        // 测试更新已有告警配置
        AlarmSystem system;
        
        // 第一次配置
        AlarmConfig config1;
        config1.channelId = 0;
        config1.upperLimit = 80.0f;
        config1.upperEnabled = true;
        system.configureAlarm("device001", config1);
        
        // 更新配置
        AlarmConfig config2;
        config2.channelId = 0;  // 相同通道
        config2.upperLimit = 100.0f;
        config2.upperEnabled = true;
        system.configureAlarm("device001", config2);
        
        QVector<AlarmConfig> alarms = system.getAlarms("device001");
        QCOMPARE(alarms.size(), 1);  // 仍为 1 个（更新而非新增）
        QCOMPARE(alarms[0].upperLimit, 100.0f);  // 新值
    }
    
    void testConfigureAlarm_MultipleChannels() {
        // 测试多通道告警配置
        AlarmSystem system;
        
        for (int i = 0; i < 4; ++i) {
            AlarmConfig config;
            config.channelId = i;
            config.upperLimit = 100.0f;
            config.upperEnabled = true;
            system.configureAlarm("device001", config);
        }
        
        QVector<AlarmConfig> alarms = system.getAlarms("device001");
        QCOMPARE(alarms.size(), 4);
    }
    
    void testConfigureAlarm_MultipleDevices() {
        // 测试多设备告警配置
        AlarmSystem system;
        
        AlarmConfig config1, config2;
        config1.channelId = 0;
        config1.upperLimit = 80.0f;
        config1.upperEnabled = true;
        
        config2.channelId = 0;
        config2.upperLimit = 90.0f;
        config2.upperEnabled = true;
        
        system.configureAlarm("device001", config1);
        system.configureAlarm("device002", config2);
        
        QCOMPARE(system.getAlarms("device001").size(), 1);
        QCOMPARE(system.getAlarms("device002").size(), 1);
        QCOMPARE(system.getAlarms("device001")[0].upperLimit, 80.0f);
        QCOMPARE(system.getAlarms("device002")[0].upperLimit, 90.0f);
    }
    
    // ========== 启用/禁用告警测试 ==========
    
    void testSetAlarmEnabled_Success() {
        // 测试成功启用/禁用告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 100.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        // 禁用
        system.setAlarmEnabled("device001", 0, false);
        
        QVector<AlarmConfig> alarms = system.getAlarms("device001");
        QCOMPARE(alarms[0].enabled, false);
        
        // 重新启用
        system.setAlarmEnabled("device001", 0, true);
        alarms = system.getAlarms("device001");
        QCOMPARE(alarms[0].enabled, true);
    }
    
    void testSetAlarmEnabled_NonExistentDevice() {
        // 测试对不存在的设备启用告警
        AlarmSystem system;
        
        // 不应崩溃
        system.setAlarmEnabled("nonexistent", 0, false);
    }
    
    void testSetAlarmEnabled_NonExistentChannel() {
        // 测试对不存在的通道启用告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        system.configureAlarm("device001", config);
        
        // 对通道 1 操作（不存在）
        system.setAlarmEnabled("device001", 1, false);
        
        // 通道 0 应不受影响
        QVector<AlarmConfig> alarms = system.getAlarms("device001");
        QCOMPARE(alarms.size(), 1);
        QCOMPARE(alarms[0].enabled, true);
    }
    
    // ========== 数据检查与告警触发测试 ==========
    
    void testCheckData_NoConfig() {
        // 测试没有配置时检查数据
        AlarmSystem system;
        
        TelemetryData data;
        data.channels = {50.0f};
        
        // 不应崩溃
        system.checkData("device001", data);
    }
    
    void testCheckData_UpperLimit_Exceeded() {
        // 测试超过上限触发告警
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        config.level = AlarmLevel::WARNING;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};  // 超过上限
        system.checkData("device001", data);
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testCheckData_LowerLimit_Exceeded() {
        // 测试低于下限触发告警
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.lowerLimit = 20.0f;
        config.lowerEnabled = true;
        config.level = AlarmLevel::WARNING;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {10.0f};  // 低于下限
        system.checkData("device001", data);
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testCheckData_WithinLimits() {
        // 测试数据在正常范围内不触发告警
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.lowerLimit = 20.0f;
        config.upperLimit = 80.0f;
        config.lowerEnabled = true;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {50.0f};  // 在范围内
        system.checkData("device001", data);
        
        QCOMPARE(spy.count(), 0);  // 不应触发
    }
    
    void testCheckData_DisabledAlarm() {
        // 测试禁用的告警不触发
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        config.enabled = false;  // 禁用
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};  // 超过上限
        system.checkData("device001", data);
        
        QCOMPARE(spy.count(), 0);  // 不应触发
    }
    
    void testCheckData_NoRepeatTrigger() {
        // 测试告警不重复触发
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        
        // 多次检查
        system.checkData("device001", data);
        system.checkData("device001", data);
        system.checkData("device001", data);
        
        QCOMPARE(spy.count(), 1);  // 只触发一次
    }
    
    void testCheckData_AlarmRecovery() {
        // 测试告警恢复
        AlarmSystem system;
        
        QSignalSpy triggerSpy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        QSignalSpy clearSpy(&system, SIGNAL(alarmCleared(QString,int)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        // 触发告警
        TelemetryData highData;
        highData.channels = {90.0f};
        system.checkData("device001", highData);
        QCOMPARE(triggerSpy.count(), 1);
        
        // 恢复正常
        TelemetryData normalData;
        normalData.channels = {50.0f};
        system.checkData("device001", normalData);
        
        QCOMPARE(clearSpy.count(), 1);  // 应触发恢复信号
    }
    
    void testCheckData_MultipleChannels() {
        // 测试多通道检查
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        
        // 配置两个通道
        AlarmConfig config1;
        config1.channelId = 0;
        config1.upperLimit = 80.0f;
        config1.upperEnabled = true;
        system.configureAlarm("device001", config1);
        
        AlarmConfig config2;
        config2.channelId = 1;
        config2.upperLimit = 90.0f;
        config2.upperEnabled = true;
        system.configureAlarm("device001", config2);
        
        // 两个通道都超限
        TelemetryData data;
        data.channels = {85.0f, 95.0f};
        system.checkData("device001", data);
        
        QCOMPARE(spy.count(), 2);  // 两个告警
    }
    
    // ========== 告警记录测试 ==========
    
    void testGetAlarmRecords_Empty() {
        // 测试获取空记录
        AlarmSystem system;
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QVERIFY(records.isEmpty());
    }
    
    void testGetAlarmRecords_AfterTrigger() {
        // 测试触发后获取记录
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QCOMPARE(records[0].deviceId, QString("device001"));
        QCOMPARE(records[0].channelId, 0);
        QVERIFY(records[0].value > 80.0f);
    }
    
    void testGetAllAlarmRecords_MultipleDevices() {
        // 测试获取所有设备的记录
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        
        system.configureAlarm("device001", config);
        system.configureAlarm("device002", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        system.checkData("device002", data);
        
        QVector<AlarmRecord> allRecords = system.getAllAlarmRecords();
        QCOMPARE(allRecords.size(), 2);
    }
    
    void testAlarmRecord_Fields() {
        // 测试告警记录字段
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        config.level = AlarmLevel::CRITICAL;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        
        AlarmRecord record = records[0];
        QCOMPARE(record.deviceId, QString("device001"));
        QCOMPARE(record.channelId, 0);
        QCOMPARE(record.level, AlarmLevel::CRITICAL);
        QVERIFY(record.value > 80.0f);
        QVERIFY(!record.message.isEmpty());
        QVERIFY(record.timestamp.isValid());
        QCOMPARE(record.acknowledged, false);
    }
    
    // ========== 告警确认测试 ==========
    
    void testAcknowledgeAlarm_Success() {
        // 测试成功确认告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        // 确认告警
        system.acknowledgeAlarm("device001", 0);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QCOMPARE(records[0].acknowledged, true);
    }
    
    void testAcknowledgeAlarm_NonExistentDevice() {
        // 测试确认不存在设备的告警
        AlarmSystem system;
        
        // 不应崩溃
        system.acknowledgeAlarm("nonexistent", 0);
    }
    
    void testAcknowledgeAlarm_NonExistentChannel() {
        // 测试确认不存在通道的告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        // 确认通道 1（不存在）
        system.acknowledgeAlarm("device001", 1);
        
        // 通道 0 的告警应未确认
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QCOMPARE(records[0].acknowledged, false);
    }
    
    // ========== 清除记录测试 ==========
    
    void testClearRecords_Success() {
        // 测试成功清除记录
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        // 清除记录
        system.clearRecords("device001");
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QVERIFY(records.isEmpty());
    }
    
    void testClearRecords_NonExistentDevice() {
        // 测试清除不存在设备的记录
        AlarmSystem system;
        
        // 不应崩溃
        system.clearRecords("nonexistent");
    }
    
    void testClearRecords_Partial() {
        // 测试部分清除（只清除一个设备）
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        
        system.configureAlarm("device001", config);
        system.configureAlarm("device002", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        system.checkData("device002", data);
        
        // 只清除 device001
        system.clearRecords("device001");
        
        QVERIFY(system.getAlarmRecords("device001").isEmpty());
        QCOMPARE(system.getAlarmRecords("device002").size(), 1);
    }
    
    // ========== 信号测试 ==========
    
    void testAlarmTriggered_SignalExists() {
        // 测试告警触发信号存在
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        QVERIFY(spy.isValid());
    }
    
    void testAlarmCleared_SignalExists() {
        // 测试告警恢复信号存在
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmCleared(QString,int)));
        QVERIFY(spy.isValid());
    }
    
    void testAlarmTriggered_Signal_Data() {
        // 测试告警触发信号数据
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmTriggered(QString,AlarmRecord)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        config.level = AlarmLevel::CRITICAL;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        QCOMPARE(spy.count(), 1);
        
        // 验证信号参数
        QString deviceId = spy[0][0].toString();
        AlarmRecord record = spy[0][1].value<AlarmRecord>();
        
        QCOMPARE(deviceId, QString("device001"));
        QCOMPARE(record.channelId, 0);
        QCOMPARE(record.level, AlarmLevel::CRITICAL);
        QVERIFY(record.value > 80.0f);
    }
    
    void testAlarmCleared_Signal_Data() {
        // 测试告警恢复信号数据
        AlarmSystem system;
        
        QSignalSpy spy(&system, SIGNAL(alarmCleared(QString,int)));
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        // 触发告警
        TelemetryData highData;
        highData.channels = {90.0f};
        system.checkData("device001", highData);
        
        // 恢复正常
        TelemetryData normalData;
        normalData.channels = {50.0f};
        system.checkData("device001", normalData);
        
        QCOMPARE(spy.count(), 1);
        
        // 验证信号参数
        QString deviceId = spy[0][0].toString();
        int channelId = spy[0][1].toInt();
        
        QCOMPARE(deviceId, QString("device001"));
        QCOMPARE(channelId, 0);
    }
    
    // ========== 边界条件测试 ==========
    
    void testBoundary_LimitEquality() {
        // 测试上下限相等的情况
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.lowerLimit = 50.0f;
        config.upperLimit = 50.0f;
        config.lowerEnabled = true;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {50.0f};  // 等于限值
        system.checkData("device001", data);
        
        // 等于限值不应触发（< 和 > 而非 <= 和 >=）
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QVERIFY(records.isEmpty());
    }
    
    void testBoundary_ZeroLimits() {
        // 测试零限值
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.lowerLimit = 0.0f;
        config.upperLimit = 0.0f;
        config.lowerEnabled = true;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {0.0f};
        system.checkData("device001", data);
        
        // 等于 0 不应触发
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QVERIFY(records.isEmpty());
    }
    
    void testBoundary_NegativeLimits() {
        // 测试负限值
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.lowerLimit = -100.0f;
        config.upperLimit = -50.0f;
        config.lowerEnabled = true;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        // 低于下限（触发告警）
        TelemetryData lowData;
        lowData.channels = {-150.0f};
        system.checkData("device001", lowData);
        
        // 回到正常范围（恢复告警）
        TelemetryData normalData;
        normalData.channels = {-75.0f};  // 在 -100 和 -50 之间
        system.checkData("device001", normalData);
        
        // 高于上限（再次触发告警）
        TelemetryData highData;
        highData.channels = {-40.0f};
        system.checkData("device001", highData);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 2);  // 下限告警 + 上限告警
    }
    
    void testBoundary_VeryLargeLimits() {
        // 测试极大限值
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.lowerLimit = -1e10f;
        config.upperLimit = 1e10f;
        config.lowerEnabled = true;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {0.0f};
        system.checkData("device001", data);
        
        // 在范围内，不应触发
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QVERIFY(records.isEmpty());
    }
    
    void testBoundary_EmptyChannels() {
        // 测试空通道数据
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {};  // 空通道
        system.checkData("device001", data);
        
        // 不应崩溃，也不应触发
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QVERIFY(records.isEmpty());
    }
    
    void testBoundary_ChannelIndexOutOfBounds() {
        // 测试通道索引越界
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 5;  // 通道 5
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {50.0f, 60.0f};  // 只有 2 个通道
        system.checkData("device001", data);
        
        // 不应崩溃，也不应触发（通道不存在）
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QVERIFY(records.isEmpty());
    }
    
    // ========== 告警级别测试 ==========
    
    void testAlarmLevel_Info() {
        // 测试 INFO 级别告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        config.level = AlarmLevel::INFO;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QCOMPARE(records[0].level, AlarmLevel::INFO);
    }
    
    void testAlarmLevel_Warning() {
        // 测试 WARNING 级别告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        config.level = AlarmLevel::WARNING;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QCOMPARE(records[0].level, AlarmLevel::WARNING);
    }
    
    void testAlarmLevel_Critical() {
        // 测试 CRITICAL 级别告警
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 0;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        config.level = AlarmLevel::CRITICAL;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {90.0f};
        system.checkData("device001", data);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QCOMPARE(records[0].level, AlarmLevel::CRITICAL);
    }
    
    // ========== 消息格式测试 ==========
    
    void testAlarmMessage_LowerLimit() {
        // 测试下限告警消息格式
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 2;
        config.lowerLimit = 20.0f;
        config.lowerEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {10.0f, 20.0f, 15.0f};  // 通道 2 为 15.0f
        system.checkData("device001", data);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QVERIFY(records[0].message.contains("below"));
        QVERIFY(records[0].message.contains("2"));  // 通道 ID
        QVERIFY(records[0].message.contains("15"));  // 值
    }
    
    void testAlarmMessage_UpperLimit() {
        // 测试上限告警消息格式
        AlarmSystem system;
        
        AlarmConfig config;
        config.channelId = 1;
        config.upperLimit = 80.0f;
        config.upperEnabled = true;
        system.configureAlarm("device001", config);
        
        TelemetryData data;
        data.channels = {50.0f, 90.0f};  // 通道 1 为 90.0f
        system.checkData("device001", data);
        
        QVector<AlarmRecord> records = system.getAlarmRecords("device001");
        QCOMPARE(records.size(), 1);
        QVERIFY(records[0].message.contains("above"));
        QVERIFY(records[0].message.contains("1"));  // 通道 ID
        QVERIFY(records[0].message.contains("90"));  // 值
    }
};

QTEST_APPLESS_MAIN(TestAlarmSystem)
#include "test_alarmsystem.moc"
