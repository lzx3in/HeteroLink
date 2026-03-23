/**
 * HeteroLink Host - Mock 库单元测试
 * 
 * 测试 Mock 类本身的正确性，确保 Mock 行为符合预期
 * 
 * 包含：
 * - MockUartChannel 测试
 * - MockMqttChannel 测试
 * - MockDeviceManager 测试
 */

#include <QTest>
#include <QDebug>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>

#include "mocks/MockUartChannel.h"
#include "mocks/MockMqttChannel.h"
#include "mocks/MockDeviceManager.h"
#include "protocol/Protocol.h"

using namespace HeteroLink;

/**
 * @brief Mock 库综合测试
 */
class TestMocks : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting Mock library tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "Mock library tests finished";
    }
    
    // ========================================================================
    // MockUartChannel 测试
    // ========================================================================
    
    void testMockUartChannel_Creation() {
        MockUartChannel channel;
        QVERIFY(!channel.isConnected());
        QCOMPARE(channel.getSentFrames().size(), 0);
    }
    
    void testMockUartChannel_AvailablePorts() {
        // 测试 availablePorts 返回系统真实串口列表
        QVector<QSerialPortInfo> ports = MockUartChannel::availablePorts();
        // 验证能正常返回（数量取决于系统）
        QVERIFY(ports.size() >= 0);
    }
    
    void testMockUartChannel_Connect() {
        MockUartChannel channel;
        UartConfig config;
        config.portName = "COM_MOCK";  // 模拟端口名
        config.baudRate = 921600;
        
        bool result = channel.connect(config);
        QVERIFY(result);
        QVERIFY(channel.isConnected());
        QCOMPARE(channel.getConfig().portName, QString("COM_MOCK"));
    }
    
    void testMockUartChannel_Connect_Failure() {
        MockUartChannel channel;
        channel.setShouldFail(true);
        UartConfig config;
        
        bool result = channel.connect(config);
        QVERIFY(!result);
        QVERIFY(!channel.isConnected());
    }
    
    void testMockUartChannel_SendFrame() {
        MockUartChannel channel;
        UartConfig config;
        channel.connect(config);
        
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x10;
        frame.payload = {0xAA, 0xBB};
        
        bool result = channel.sendFrame(frame);
        QVERIFY(result);
        QCOMPARE(channel.getSentFrames().size(), 1);
    }
    
    void testMockUartChannel_SendFrame_NotConnected() {
        MockUartChannel channel;
        Frame frame;
        frame.deviceId = 0x01;
        
        bool result = channel.sendFrame(frame);
        QVERIFY(!result);
        QCOMPARE(channel.getSentFrames().size(), 0);
    }
    
    void testMockUartChannel_SendHeartbeat() {
        MockUartChannel channel;
        UartConfig config;
        channel.connect(config);
        
        channel.sendHeartbeat(0x05);
        
        QCOMPARE(channel.getSentFrames().size(), 1);
        QCOMPARE(channel.getSentFrames()[0].deviceId, 0x05u);
        QCOMPARE(channel.getSentFrames()[0].command, static_cast<uint8_t>(Command::CMD_HEARTBEAT));
    }
    
    void testMockUartChannel_SendControlCommand() {
        MockUartChannel channel;
        UartConfig config;
        channel.connect(config);
        
        std::vector<uint8_t> params = {0xAA, 0xBB};
        channel.sendControlCommand(0x03, 0x01, params);
        
        QCOMPARE(channel.getSentFrames().size(), 1);
        Frame frame = channel.getSentFrames()[0];
        QCOMPARE(frame.deviceId, 0x03u);
        QCOMPARE(frame.command, static_cast<uint8_t>(Command::CMD_CONTROL));
        QCOMPARE(frame.payload.size(), 3u);
        QCOMPARE(frame.payload[0], 0x01u);
    }
    
    void testMockUartChannel_SetDeviceId() {
        MockUartChannel channel;
        channel.setDeviceId(0x07);
        QCOMPARE(channel.getDeviceId(), 0x07u);
    }
    
    void testMockUartChannel_SimulateTelemetry() {
        MockUartChannel channel;
        UartConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(telemetryReceived(uint8_t,TelemetryData)));
        
        TelemetryData telemetry;
        telemetry.timestamp = 1000;
        telemetry.channels = {1.5f, 2.5f};
        
        channel.simulateTelemetry(0x01, telemetry);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toUInt(), 0x01u);
    }
    
    void testMockUartChannel_SimulateError() {
        MockUartChannel channel;
        UartConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(errorReceived(uint8_t,ErrorCode)));
        channel.simulateError(0x01, ErrorCode::ERR_CRC_ERROR);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toInt(), static_cast<int>(ErrorCode::ERR_CRC_ERROR));
    }
    
    void testMockUartChannel_SimulateDisconnect() {
        MockUartChannel channel;
        UartConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(connectionChanged(bool)));
        channel.simulateDisconnect();
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toBool(), false);
    }
    
    void testMockUartChannel_ClearSentHistory() {
        MockUartChannel channel;
        UartConfig config;
        channel.connect(config);
        
        Frame frame;
        frame.deviceId = 0x01;
        channel.sendFrame(frame);
        channel.sendFrame(frame);
        
        channel.clearSentHistory();
        QCOMPARE(channel.getSentFrames().size(), 0);
    }
    
    // ========================================================================
    // MockMqttChannel 测试
    // ========================================================================
    
    void testMockMqttChannel_Creation() {
        MockMqttChannel channel;
        QVERIFY(!channel.isConnected());
        QCOMPARE(channel.getPublishedMessages().size(), 0);
        QCOMPARE(channel.getSubscriptions().size(), 0);
    }
    
    void testMockMqttChannel_Connect() {
        MockMqttChannel channel;
        MqttConfig config;
        config.brokerHost = "localhost";
        
        bool result = channel.connect(config);
        QVERIFY(result);
        QVERIFY(channel.isConnected());
    }
    
    void testMockMqttChannel_Connect_Failure() {
        MockMqttChannel channel;
        channel.setShouldFail(true);
        MqttConfig config;
        
        bool result = channel.connect(config);
        QVERIFY(!result);
    }
    
    void testMockMqttChannel_Disconnect_ClearsSubscriptions() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        channel.subscribe("test/topic");
        
        channel.disconnect();
        QCOMPARE(channel.getSubscriptions().size(), 0);
    }
    
    void testMockMqttChannel_Subscribe() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        bool result = channel.subscribe("heterolink/+/telemetry");
        QVERIFY(result);
        QCOMPARE(channel.getSubscriptions().size(), 1);
    }
    
    void testMockMqttChannel_Publish() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        bool result = channel.publish("test/topic", QByteArray("Hello"), 1, false);
        QVERIFY(result);
        QCOMPARE(channel.getPublishedMessages().size(), 1);
    }
    
    void testMockMqttChannel_Publish_NotConnected() {
        MockMqttChannel channel;
        bool result = channel.publish("test/topic", QByteArray("Test"), 1, false);
        QVERIFY(!result);
    }
    
    void testMockMqttChannel_PublishDeviceStatus() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        channel.publishDeviceStatus("device_001", true);
        
        QCOMPARE(channel.getPublishedMessages().size(), 1);
        QCOMPARE(channel.getPublishedMessages()[0].topic, QString("heterolink/subboard/device_001/status"));
        QCOMPARE(channel.getPublishedMessages()[0].payload, QByteArray("online"));
    }
    
    void testMockMqttChannel_PublishTelemetry() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        QJsonObject obj;
        obj["value"] = 42.5;
        QJsonDocument doc(obj);
        
        channel.publishTelemetry("device_001", QString::fromUtf8(doc.toJson()));
        
        QCOMPARE(channel.getPublishedMessages().size(), 1);
    }
    
    void testMockMqttChannel_PublishCommand() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        channel.publishCommand("device_001", "{\"cmd\":\"reset\"}");
        
        QCOMPARE(channel.getPublishedMessages().size(), 1);
        QCOMPARE(channel.getPublishedMessages()[0].topic, QString("heterolink/subboard/device_001/command"));
    }
    
    void testMockMqttChannel_SubscribeDeviceCommands() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribeDeviceCommands("device_001");
        
        QCOMPARE(channel.getSubscriptions().size(), 1);
        QCOMPARE(channel.getSubscriptions()[0].topic, QString("heterolink/subboard/device_001/command"));
    }
    
    void testMockMqttChannel_SubscribeAllDeviceStatus() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribeAllDeviceStatus();
        
        QCOMPARE(channel.getSubscriptions().size(), 1);
        QCOMPARE(channel.getSubscriptions()[0].topic, QString("heterolink/subboard/+/status"));
    }
    
    void testMockMqttChannel_SubscribeAllDeviceTelemetry() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        channel.subscribeAllDeviceTelemetry();
        
        QCOMPARE(channel.getSubscriptions().size(), 1);
        QCOMPARE(channel.getSubscriptions()[0].topic, QString("heterolink/subboard/+/telemetry"));
    }
    
    void testMockMqttChannel_SimulateMessage() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        // 订阅一个通配符 topic
        channel.subscribe("test/#");
        
        QSignalSpy spy(&channel, SIGNAL(messageReceived(QString,QByteArray)));
        channel.simulateMessage("test/topic", QByteArray("Test"));
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testMockMqttChannel_SimulateDeviceStatus() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(deviceStatusReceived(QString,bool)));
        channel.simulateDeviceStatus("device_001", true);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toBool(), true);
    }
    
    void testMockMqttChannel_SimulateTelemetry() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(telemetryReceived(QString,QString)));
        channel.simulateTelemetry("device_001", "{\"value\":42}");
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testMockMqttChannel_SimulateDeviceCommand() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(deviceCommandReceived(QString,QString)));
        channel.simulateDeviceCommand("device_001", "{\"cmd\":\"start\"}");
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toString(), QString("{\"cmd\":\"start\"}"));
    }
    
    void testMockMqttChannel_TopicMatching_PlusWildcard() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        channel.subscribe("heterolink/+/status");
        
        QSignalSpy spy(&channel, SIGNAL(messageReceived(QString,QByteArray)));
        
        channel.simulateMessage("heterolink/device_001/status", QByteArray("online"));
        QCOMPARE(spy.count(), 1);
        
        channel.simulateMessage("heterolink/device_002/status", QByteArray("offline"));
        QCOMPARE(spy.count(), 2);
        
        // 不应该匹配
        channel.simulateMessage("heterolink/device_001/telemetry", QByteArray("{}"));
        QCOMPARE(spy.count(), 2);
    }
    
    void testMockMqttChannel_TopicMatching_HashWildcard() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        channel.subscribe("heterolink/#");
        
        QSignalSpy spy(&channel, SIGNAL(messageReceived(QString,QByteArray)));
        
        channel.simulateMessage("heterolink/device_001/status", QByteArray("online"));
        QCOMPARE(spy.count(), 1);
        
        channel.simulateMessage("heterolink/device_001/telemetry", QByteArray("{}"));
        QCOMPARE(spy.count(), 2);
        
        channel.simulateMessage("heterolink/anything/deep/nested", QByteArray("test"));
        QCOMPARE(spy.count(), 3);
    }
    
    void testMockMqttChannel_ClearPublishHistory() {
        MockMqttChannel channel;
        MqttConfig config;
        channel.connect(config);
        
        channel.publish("topic1", QByteArray("Msg1"));
        channel.publish("topic2", QByteArray("Msg2"));
        
        channel.clearPublishHistory();
        QCOMPARE(channel.getPublishedMessages().size(), 0);
    }
    
    // ========================================================================
    // MockDeviceManager 测试
    // ========================================================================
    
    void testMockDeviceManager_Creation() {
        MockDeviceManager manager;
        QVERIFY(manager.getDevices().isEmpty());
    }
    
    void testMockDeviceManager_GetMockChannels() {
        MockDeviceManager manager;
        
        MockMqttChannel* mqttChannel = manager.getMockMqttChannel();
        QVERIFY(mqttChannel != nullptr);
    }
    
    void testMockDeviceManager_AddMockDevice_Uart() {
        MockDeviceManager manager;
        
        bool result = manager.addMockDevice("device_001", "UART");
        
        QVERIFY(result);
        QCOMPARE(manager.getDevices().size(), 1);
        QCOMPARE(manager.getDevice("device_001").connectionType, QString("UART"));
    }
    
    void testMockDeviceManager_AddMockDevice_Mqtt() {
        MockDeviceManager manager;
        
        bool result = manager.addMockDevice("device_002", "MQTT");
        
        QVERIFY(result);
        QCOMPARE(manager.getDevices().size(), 1);
        QCOMPARE(manager.getDevice("device_002").connectionType, QString("MQTT"));
    }
    
    void testMockDeviceManager_SimulateTelemetry_Uart() {
        MockDeviceManager manager;
        manager.addMockDevice("1", "UART");
        
        QSignalSpy spy(&manager, SIGNAL(telemetryReceived(QString,TelemetryData)));
        
        // 连接通道
        MockUartChannel* channel = manager.getMockUartChannel("1");
        channel->connect(UartConfig());
        
        TelemetryData telemetry;
        telemetry.timestamp = 1000;
        telemetry.channels = {1.5f, 2.5f};
        
        manager.simulateTelemetry("1", telemetry);
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testMockDeviceManager_SimulateTelemetry_Mqtt() {
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "MQTT");
        
        QSignalSpy spy(&manager, SIGNAL(telemetryReceived(QString,TelemetryData)));
        
        // 连接 MQTT 通道
        MockMqttChannel* mqttChannel = manager.getMockMqttChannel();
        mqttChannel->connect(MqttConfig());
        
        TelemetryData telemetry;
        telemetry.timestamp = 1000;
        telemetry.channels = {1.5f};
        
        manager.simulateTelemetry("device_001", telemetry);
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testMockDeviceManager_SimulateDeviceConnect() {
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "UART");
        
        QSignalSpy spy(&manager, SIGNAL(deviceStatusChanged(QString,bool,bool)));
        manager.simulateDeviceConnect("device_001");
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toBool(), true);
        QCOMPARE(spy[0][2].toBool(), true);
    }
    
    void testMockDeviceManager_SimulateDeviceDisconnect() {
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "UART");
        
        QSignalSpy spy(&manager, SIGNAL(deviceStatusChanged(QString,bool,bool)));
        manager.simulateDeviceDisconnect("device_001");
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toBool(), false);
        QCOMPARE(spy[0][2].toBool(), false);
    }
    
    void testMockDeviceManager_GetSentHeartbeats() {
        MockDeviceManager manager;
        manager.addMockDevice("1", "UART");
        
        MockUartChannel* channel = manager.getMockUartChannel("1");
        channel->connect(UartConfig());
        channel->sendHeartbeat(1);
        channel->sendHeartbeat(1);
        
        QVector<Frame> heartbeats = manager.getSentHeartbeats("1");
        QCOMPARE(heartbeats.size(), 2);
    }
    
    void testMockDeviceManager_GetSentCommands() {
        MockDeviceManager manager;
        manager.addMockDevice("1", "UART");
        
        MockUartChannel* channel = manager.getMockUartChannel("1");
        channel->connect(UartConfig());
        channel->sendControlCommand(1, 0x01, {0xAA});
        channel->sendControlCommand(1, 0x02, {0xBB});
        
        QVector<Frame> commands = manager.getSentCommands("1");
        QCOMPARE(commands.size(), 2);
    }
    
    void testMockDeviceManager_GetPublishedMqttMessages() {
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "MQTT");
        
        MockMqttChannel* mqttChannel = manager.getMockMqttChannel();
        mqttChannel->connect(MqttConfig());
        mqttChannel->publishDeviceStatus("device_001", true);
        
        QVector<MockMqttMessage> messages = manager.getPublishedMqttMessages();
        QCOMPARE(messages.size(), 1);
    }
    
    void testMockDeviceManager_ClearMockDevices() {
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "UART");
        
        manager.clearMockDevices();
        
        MockMqttChannel* mqttChannel = manager.getMockMqttChannel();
        QVERIFY(mqttChannel == nullptr);
    }
    // ========================================================================
    
    void testIntegration_DeviceStatusChange_SignalEmitted() {
        // 集成测试：设备状态变化触发信号
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "UART");
        
        QSignalSpy spy(&manager, SIGNAL(deviceStatusChanged(QString,bool,bool)));
        
        // 模拟设备连接
        manager.simulateDeviceConnect("device_001");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toString(), QString("device_001"));
        QCOMPARE(spy[0][1].toBool(), true);  // connected
        QCOMPARE(spy[0][2].toBool(), true);  // online
        
        // 模拟设备断开
        manager.simulateDeviceDisconnect("device_001");
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy[1][1].toBool(), false);  // connected
        QCOMPARE(spy[1][2].toBool(), false);  // online
    }
    
    void testIntegration_MultiDevice_Telemetry() {
        // 集成测试：多设备同时发送遥测
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "UART");
        manager.addMockDevice("device_002", "UART");
        manager.addMockDevice("device_003", "MQTT");
        
        // 连接通道
        MockUartChannel* uartChannel1 = manager.getMockUartChannel("device_001");
        uartChannel1->connect(UartConfig());
        MockUartChannel* uartChannel2 = manager.getMockUartChannel("device_002");
        uartChannel2->connect(UartConfig());
        MockMqttChannel* mqttChannel = manager.getMockMqttChannel();
        mqttChannel->connect(MqttConfig());
        
        QSignalSpy spy(&manager, SIGNAL(telemetryReceived(QString,TelemetryData)));
        
        // UART 设备发送遥测
        TelemetryData uartData;
        uartData.timestamp = 1000;
        uartData.channels = {1.5f, 2.5f};
        manager.simulateTelemetry("device_001", uartData);
        manager.simulateTelemetry("device_002", uartData);
        
        // MQTT 设备发送遥测
        TelemetryData mqttData;
        mqttData.timestamp = 2000;
        mqttData.channels = {3.5f};
        manager.simulateTelemetry("device_003", mqttData);
        
        QCOMPARE(spy.count(), 3);
    }
    
    void testIntegration_HeartbeatSequence() {
        // 集成测试：心跳序列验证
        MockDeviceManager manager;
        manager.addMockDevice("1", "UART");
        
        MockUartChannel* channel = manager.getMockUartChannel("1");
        channel->connect(UartConfig());
        
        // 发送 5 次心跳
        for (int i = 0; i < 5; ++i) {
            channel->sendHeartbeat(1);
        }
        
        QVector<Frame> heartbeats = manager.getSentHeartbeats("1");
        QCOMPARE(heartbeats.size(), 5);
        
        // 验证所有心跳帧格式正确
        for (const auto& frame : heartbeats) {
            QCOMPARE(frame.deviceId, 1u);
            QCOMPARE(frame.command, static_cast<uint8_t>(Command::CMD_HEARTBEAT));
            QVERIFY(frame.payload.empty());
        }
    }
    
    void testIntegration_ControlCommandSequence() {
        // 集成测试：控制命令序列验证
        MockDeviceManager manager;
        manager.addMockDevice("1", "UART");
        
        MockUartChannel* channel = manager.getMockUartChannel("1");
        channel->connect(UartConfig());
        
        // 发送多个控制命令
        std::vector<uint8_t> params1 = {0x01, 0x02};
        std::vector<uint8_t> params2 = {0x03, 0x04, 0x05};
        channel->sendControlCommand(1, 0x10, params1);
        channel->sendControlCommand(1, 0x20, params2);
        
        QVector<Frame> commands = manager.getSentCommands("1");
        QCOMPARE(commands.size(), 2);
        
        // 验证第一个命令
        QCOMPARE(commands[0].deviceId, 1u);
        QCOMPARE(commands[0].command, static_cast<uint8_t>(Command::CMD_CONTROL));
        QCOMPARE(commands[0].payload.size(), 3u);  // cmdType + params
        QCOMPARE(commands[0].payload[0], 0x10u);   // cmdType
        
        // 验证第二个命令
        QCOMPARE(commands[1].payload[0], 0x20u);
        QCOMPARE(commands[1].payload.size(), 4u);
    }
    
    void testIntegration_Mqtt_PublishSequence() {
        // 集成测试：MQTT 发布序列验证
        MockDeviceManager manager;
        manager.addMockDevice("device_001", "MQTT");
        
        MockMqttChannel* mqttChannel = manager.getMockMqttChannel();
        mqttChannel->connect(MqttConfig());
        
        // 发布设备状态
        mqttChannel->publishDeviceStatus("device_001", true);
        
        // 验证发布了一条消息
        QCOMPARE(manager.getPublishedMqttMessages().size(), 1);
        QCOMPARE(manager.getPublishedMqttMessages()[0].topic, QString("heterolink/subboard/device_001/status"));
    }
};

QTEST_APPLESS_MAIN(TestMocks)
#include "test_mocks.moc"
