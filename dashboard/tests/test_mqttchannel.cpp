/**
 * HeteroLink Host - MQTT 通道单元测试（使用 Mock）
 * 
 * 测试 MqttChannel 类的连接、订阅、发布功能
 * 使用 MockMqttChannel 无需真实 Broker
 */

#include <QTest>
#include <QDebug>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>

#include "mocks/MockMqttChannel.h"

using namespace HeteroLink;

class TestMqttChannel : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting MqttChannel unit tests (Mock)";
    }
    
    void cleanupTestCase() {
        qDebug() << "MqttChannel unit tests finished";
    }
    
    // ========== 连接测试 ==========
    
    void testConnect_Success() {
        MockMqttChannel channel;
        
        MqttConfig config;
        config.brokerHost = "localhost";
        config.brokerPort = 1883;
        config.clientId = "test-client";
        
        bool result = channel.connect(config);
        
        QVERIFY(result);
        QVERIFY(channel.isConnected());
        QCOMPARE(channel.getConfig().brokerHost, QString("localhost"));
        QCOMPARE(channel.getConfig().clientId, QString("test-client"));
    }
    
    void testConnect_Failure() {
        MockMqttChannel channel;
        channel.setShouldFail(true);
        
        MqttConfig config;
        config.brokerHost = "localhost";
        
        bool result = channel.connect(config);
        
        QVERIFY(!result);
        QVERIFY(!channel.isConnected());
    }
    
    void testDisconnect() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.disconnect();
        
        QVERIFY(!channel.isConnected());
        QCOMPARE(channel.getSubscriptions().size(), 0);  // 订阅应被清除
    }
    
    void testConnectionChanged_Signal() {
        MockMqttChannel channel;
        QSignalSpy spy(&channel, SIGNAL(connectionChanged(bool)));
        
        MqttConfig config;
        channel.connect(config);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toBool(), true);
        
        channel.disconnect();
        
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy[1][0].toBool(), false);
    }
    
    // ========== 订阅测试 ==========
    
    void testSubscribe_Basic() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        bool result = channel.subscribe("heterolink/subboard/+/telemetry");
        
        QVERIFY(result);
        QCOMPARE(channel.getSubscriptions().size(), 1);
        QCOMPARE(channel.getSubscriptions()[0].topic, QString("heterolink/subboard/+/telemetry"));
    }
    
    void testSubscribe_MultipleTopics() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribe("heterolink/subboard/+/status");
        channel.subscribe("heterolink/subboard/+/telemetry");
        channel.subscribe("heterolink/subboard/+/command");
        
        QCOMPARE(channel.getSubscriptions().size(), 3);
    }
    
    void testUnsubscribe() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribe("heterolink/subboard/+/status");
        QCOMPARE(channel.getSubscriptions().size(), 1);
        
        channel.unsubscribe("heterolink/subboard/+/status");
        
        // 订阅应标记为非活跃
        bool found = false;
        for (const auto& sub : channel.getSubscriptions()) {
            if (sub.topic == "heterolink/subboard/+/status" && !sub.active) {
                found = true;
            }
        }
        QVERIFY(found);
    }
    
    void testSubscribeDeviceCommands() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribeDeviceCommands("device_001");
        
        QCOMPARE(channel.getSubscriptions().size(), 1);
        QCOMPARE(channel.getSubscriptions()[0].topic, QString("heterolink/subboard/device_001/command"));
    }
    
    void testSubscribeAllDeviceStatus() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribeAllDeviceStatus();
        
        QCOMPARE(channel.getSubscriptions().size(), 1);
        QCOMPARE(channel.getSubscriptions()[0].topic, QString("heterolink/subboard/+/status"));
    }
    
    void testSubscribeAllDeviceTelemetry() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribeAllDeviceTelemetry();
        
        QCOMPARE(channel.getSubscriptions().size(), 1);
        QCOMPARE(channel.getSubscriptions()[0].topic, QString("heterolink/subboard/+/telemetry"));
    }
    
    // ========== 发布测试 ==========
    
    void testPublish_Basic() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        bool result = channel.publish("test/topic", QByteArray("Hello"), 1, false);
        
        QVERIFY(result);
        QCOMPARE(channel.getPublishedMessages().size(), 1);
        QCOMPARE(channel.getPublishedMessages()[0].topic, QString("test/topic"));
        QCOMPARE(channel.getPublishedMessages()[0].payload, QByteArray("Hello"));
        QCOMPARE(channel.getPublishedMessages()[0].qos, 1);
        QCOMPARE(channel.getPublishedMessages()[0].retain, false);
    }
    
    void testPublish_Retain() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.publish("test/topic", QByteArray("World"), 1, true);
        
        QCOMPARE(channel.getPublishedMessages().size(), 1);
        QCOMPARE(channel.getPublishedMessages()[0].retain, true);
    }
    
    void testPublish_NotConnected() {
        MockMqttChannel channel;
        
        bool result = channel.publish("test/topic", QByteArray("Test"), 1, false);
        
        QVERIFY(!result);  // 未连接应失败
    }
    
    void testPublish_Failure() {
        MockMqttChannel channel;
        channel.setShouldFail(true);
        
        MqttConfig config;
        channel.connect(config);
        
        bool result = channel.publish("test/topic", QByteArray("Test"), 1, false);
        
        QVERIFY(!result);
    }
    
    void testPublishDeviceStatus() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.publishDeviceStatus("device_001", true);
        
        QCOMPARE(channel.getPublishedMessages().size(), 1);
        QCOMPARE(channel.getPublishedMessages()[0].topic, QString("heterolink/subboard/device_001/status"));
        QCOMPARE(channel.getPublishedMessages()[0].payload, QByteArray("online"));
    }
    
    void testPublishTelemetry() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        QJsonObject obj;
        obj["timestamp"] = 1000;
        obj["value"] = 42.5;
        QJsonDocument doc(obj);
        
        channel.publishTelemetry("device_001", QString::fromUtf8(doc.toJson()));
        
        QCOMPARE(channel.getPublishedMessages().size(), 1);
        QCOMPARE(channel.getPublishedMessages()[0].topic, QString("heterolink/subboard/device_001/telemetry"));
    }
    
    void testPublishCommand() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.publishCommand("device_001", "{\"cmd\":\"reset\"}");
        
        QCOMPARE(channel.getPublishedMessages().size(), 1);
        QCOMPARE(channel.getPublishedMessages()[0].topic, QString("heterolink/subboard/device_001/command"));
    }
    
    // ========== 模拟接收测试 ==========
    
    void testSimulateMessage() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        // Subscribe to the topic first
        channel.subscribe("test/topic");
        
        QSignalSpy spy(&channel, SIGNAL(messageReceived(QString,QByteArray)));
        
        channel.simulateMessage("test/topic", QByteArray("Test Message"));
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toString(), QString("test/topic"));
        QCOMPARE(spy[0][1].toByteArray(), QByteArray("Test Message"));
    }
    
    void testSimulateDeviceStatus() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(deviceStatusReceived(QString,bool)));
        
        channel.simulateDeviceStatus("device_001", true);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toString(), QString("device_001"));
        QCOMPARE(spy[0][1].toBool(), true);
    }
    
    void testSimulateTelemetry() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(telemetryReceived(QString,QString)));
        
        QJsonObject obj;
        obj["value"] = 42.5;
        QJsonDocument doc(obj);
        
        channel.simulateTelemetry("device_001", QString::fromUtf8(doc.toJson()));
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toString(), QString("device_001"));
    }
    
    void testSimulateDeviceCommand() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(deviceCommandReceived(QString,QString)));
        
        channel.simulateDeviceCommand("device_001", "{\"cmd\":\"start\"}");
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toString(), QString("device_001"));
        QCOMPARE(spy[0][1].toString(), QString("{\"cmd\":\"start\"}"));
    }
    
    // ========== Topic 匹配测试 ==========
    
    void testTopicMatching_PlusWildcard() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribe("heterolink/subboard/+/status");
        
        QSignalSpy spy(&channel, SIGNAL(messageReceived(QString,QByteArray)));
        
        // 这些应该匹配
        channel.simulateMessage("heterolink/subboard/device_001/status", QByteArray("online"));
        QCOMPARE(spy.count(), 1);
        
        channel.simulateMessage("heterolink/subboard/device_002/status", QByteArray("offline"));
        QCOMPARE(spy.count(), 2);
        
        // 这个不应该匹配（层级不匹配）
        channel.simulateMessage("heterolink/subboard/device_001/telemetry", QByteArray("{}"));
        QCOMPARE(spy.count(), 2);  // 仍然是 2
    }
    
    void testTopicMatching_HashWildcard() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribe("heterolink/#");
        
        QSignalSpy spy(&channel, SIGNAL(messageReceived(QString,QByteArray)));
        
        // 这些都应该匹配
        channel.simulateMessage("heterolink/subboard/device_001/status", QByteArray("online"));
        QCOMPARE(spy.count(), 1);
        
        channel.simulateMessage("heterolink/subboard/device_001/telemetry", QByteArray("{}"));
        QCOMPARE(spy.count(), 2);
        
        channel.simulateMessage("heterolink/anything/deep/nested", QByteArray("test"));
        QCOMPARE(spy.count(), 3);
    }
    
    // ========== 历史清除测试 ==========
    
    void testClearPublishHistory() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        channel.publish("test/topic1", QByteArray("Msg1"));
        channel.publish("test/topic2", QByteArray("Msg2"));
        QCOMPARE(channel.getPublishedMessages().size(), 2);
        
        channel.clearPublishHistory();
        QCOMPARE(channel.getPublishedMessages().size(), 0);
    }
    
    // ========== 模拟断开/重连测试 ==========
    
    void testSimulateDisconnect() {
        MockMqttChannel channel;
        
        MqttConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(connectionChanged(bool)));
        
        channel.simulateDisconnect();
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toBool(), false);
        QVERIFY(!channel.isConnected());
    }
    
    void testSimulateConnect() {
        MockMqttChannel channel;
        
        channel.simulateConnect();
        
        QVERIFY(channel.isConnected());
    }
};

QTEST_APPLESS_MAIN(TestMqttChannel)
#include "test_mqttchannel.moc"
