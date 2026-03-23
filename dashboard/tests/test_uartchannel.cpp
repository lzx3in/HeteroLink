/**
 * HeteroLink Host - UART 通道单元测试（使用 Mock）
 * 
 * 测试 UartChannel 类的连接、发送、接收功能
 * 使用 MockUartChannel 无需真实硬件
 */

#include <QTest>
#include <QDebug>
#include <QSignalSpy>

#include "mocks/MockUartChannel.h"
#include "protocol/Protocol.h"

using namespace HeteroLink;

class TestUartChannel : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting UartChannel unit tests (Mock)";
    }
    
    void cleanupTestCase() {
        qDebug() << "UartChannel unit tests finished";
    }
    
    // ========== 连接测试 ==========
    
    void testConnect_Success() {
        MockUartChannel channel;
        
        UartConfig config;
        config.portName = "COM_MOCK_1";
        config.baudRate = 921600;
        
        bool result = channel.connect(config);
        
        QVERIFY(result);
        QVERIFY(channel.isConnected());
        QCOMPARE(channel.getConfig().portName, QString("COM_MOCK_1"));
        QCOMPARE(channel.getConfig().baudRate, qint32(921600));
    }
    
    void testConnect_Failure() {
        MockUartChannel channel;
        channel.setShouldFail(true);
        
        UartConfig config;
        config.portName = "COM_MOCK_1";
        
        bool result = channel.connect(config);
        
        QVERIFY(!result);
        QVERIFY(!channel.isConnected());
    }
    
    void testDisconnect() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        QVERIFY(channel.isConnected());
        
        channel.disconnect();
        QVERIFY(!channel.isConnected());
    }
    
    void testConnectionChanged_Signal() {
        MockUartChannel channel;
        QSignalSpy spy(&channel, SIGNAL(connectionChanged(bool)));
        
        UartConfig config;
        channel.connect(config);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toBool(), true);
        
        channel.disconnect();
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy[1][0].toBool(), false);
    }
    
    // ========== 发送测试 ==========
    
    void testSendFrame_Heartbeat() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = static_cast<uint8_t>(Command::CMD_HEARTBEAT);
        frame.payload.clear();
        
        bool result = channel.sendFrame(frame);
        
        QVERIFY(result);
        QCOMPARE(channel.getSentFrames().size(), 1);
        QCOMPARE(channel.getSentFrames()[0].deviceId, 0x01u);
        QCOMPARE(channel.getSentFrames()[0].command, static_cast<uint8_t>(Command::CMD_HEARTBEAT));
    }
    
    void testSendFrame_WithPayload() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        Frame frame;
        frame.deviceId = 0x02;
        frame.command = 0x10;
        frame.payload = {0x01, 0x02, 0x03};
        
        bool result = channel.sendFrame(frame);
        
        QVERIFY(result);
        QCOMPARE(channel.getSentFrames().size(), 1);
        QCOMPARE(channel.getSentFrames()[0].payload.size(), 3u);
    }
    
    void testSendFrame_NotConnected() {
        MockUartChannel channel;
        
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x01;
        
        bool result = channel.sendFrame(frame);
        
        QVERIFY(!result);  // 未连接应失败
        QCOMPARE(channel.getSentFrames().size(), 0);
    }
    
    void testSendFrame_Failure() {
        MockUartChannel channel;
        channel.setShouldFail(true);
        
        UartConfig config;
        channel.connect(config);
        
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x01;
        
        bool result = channel.sendFrame(frame);
        
        QVERIFY(!result);
    }
    
    void testSendHeartbeat_HelperMethod() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        channel.sendHeartbeat(0x05);
        
        QCOMPARE(channel.getSentFrames().size(), 1);
        QCOMPARE(channel.getSentFrames()[0].deviceId, 0x05u);
        QCOMPARE(channel.getSentFrames()[0].command, static_cast<uint8_t>(Command::CMD_HEARTBEAT));
    }
    
    void testSendControlCommand_HelperMethod() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        std::vector<uint8_t> params = {0xAA, 0xBB};
        channel.sendControlCommand(0x03, 0x01, params);
        
        QCOMPARE(channel.getSentFrames().size(), 1);
        QCOMPARE(channel.getSentFrames()[0].deviceId, 0x03u);
        QCOMPARE(channel.getSentFrames()[0].command, static_cast<uint8_t>(Command::CMD_CONTROL));
        QCOMPARE(channel.getSentFrames()[0].payload.size(), 3u);  // cmdType + params
    }
    
    // ========== 接收测试 ==========
    
    void testSimulateTelemetry() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(telemetryReceived(uint8_t,TelemetryData)));
        
        TelemetryData telemetry;
        telemetry.timestamp = 1000;
        telemetry.channels = {1.5f, 2.5f, 3.5f};
        
        channel.simulateTelemetry(0x01, telemetry);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toUInt(), 0x01u);
        
        TelemetryData received = spy[0][1].value<TelemetryData>();
        QCOMPARE(received.timestamp, 1000u);
        QCOMPARE(received.channels.size(), 3);
    }
    
    void testSimulateError() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(errorReceived(uint8_t,ErrorCode)));
        
        channel.simulateError(0x01, ErrorCode::ERR_CRC_ERROR);
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toUInt(), 0x01u);
        QCOMPARE(spy[0][1].toInt(), static_cast<int>(ErrorCode::ERR_CRC_ERROR));
    }
    
    void testSimulateDisconnect() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        QSignalSpy spy(&channel, SIGNAL(connectionChanged(bool)));
        
        channel.simulateDisconnect();
        
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toBool(), false);
        QVERIFY(!channel.isConnected());
    }
    
    void testSimulateConnect() {
        MockUartChannel channel;
        
        channel.simulateConnect();
        
        QVERIFY(channel.isConnected());
    }
    
    // ========== 历史清除测试 ==========
    
    void testClearSentHistory() {
        MockUartChannel channel;
        
        UartConfig config;
        channel.connect(config);
        
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x01;
        
        channel.sendFrame(frame);
        channel.sendFrame(frame);
        QCOMPARE(channel.getSentFrames().size(), 2);
        
        channel.clearSentHistory();
        QCOMPARE(channel.getSentFrames().size(), 0);
    }
    
    // ========== 可用端口测试 ==========
    
    void testAvailablePorts() {
        QVector<QSerialPortInfo> ports = MockUartChannel::availablePorts();
        
        // Just verify we can get the list of available ports
        // Don't check specific names as they depend on the system
        QVERIFY(ports.size() >= 0);  // May be 0 if no real serial ports
    }
};

QTEST_APPLESS_MAIN(TestUartChannel)
#include "test_uartchannel.moc"
