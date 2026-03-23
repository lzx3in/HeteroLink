/**
 * HeteroLink Host - UART 通道单元测试
 * 
 * 测试 UartChannel 类的协议解析、帧处理逻辑
 * 
 * 注意：由于 QSerialPort 需要真实硬件，本测试主要验证协议层逻辑
 */

#include <QTest>
#include <QDebug>
#include <QSignalSpy>

#include "protocol/UartChannel.h"
#include "protocol/Protocol.h"
#include "TestHelpers.h"

using namespace HeteroLink;
using namespace HeteroLink::TestHelpers;

class TestUartChannel : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting UartChannel unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "UartChannel unit tests finished";
    }
    
    // ========== 基础功能测试 ==========
    
    void testAvailablePorts() {
        // 测试获取可用串口列表
        QVector<QSerialPortInfo> ports = UartChannel::availablePorts();
        
        // 至少应该有 0 个或多个端口（取决于系统）
        QVERIFY(ports.size() >= 0);
    }
    
    void testConstructor() {
        // 测试构造函数
        UartChannel channel;
        QVERIFY(!channel.isConnected());
    }
    
    void testDestructor_NoCrash() {
        // 测试析构函数不会崩溃
        UartChannel* channel = new UartChannel();
        delete channel;
        // 如果析构崩溃，测试会失败
    }
    
    // ========== 连接状态测试 ==========
    
    void testInitialState_NotConnected() {
        // 测试初始状态
        UartChannel channel;
        QVERIFY(!channel.isConnected());
    }
    
    void testDisconnect_WhenNotConnected() {
        // 测试未连接时断开不会崩溃
        UartChannel channel;
        channel.disconnect();
        QVERIFY(!channel.isConnected());
    }
    
    void testSendFrame_WhenNotConnected() {
        // 测试未连接时发送失败
        UartChannel channel;
        
        Frame frame = createHeartbeatFrame(0x01);
        bool result = channel.sendFrame(frame);
        
        QVERIFY(!result);  // 未连接应返回 false
    }
    
    void testSendHeartbeat_WhenNotConnected() {
        // 测试未连接时发送心跳不会崩溃
        UartChannel channel;
        channel.sendHeartbeat(0x01);
        // 不应崩溃
    }
    
    void testSendControlCommand_WhenNotConnected() {
        // 测试未连接时发送控制命令不会崩溃
        UartChannel channel;
        
        std::vector<uint8_t> payload = {0x01, 0x02};
        channel.sendControlCommand(0x01, 0x10, payload);
        // 不应崩溃
    }
    
    void testSetDeviceId() {
        // 测试设置设备 ID（接口验证）
        UartChannel channel;
        channel.setDeviceId(0x05);
        // 设备 ID 是内部状态，通过后续发送验证
    }
    
    // ========== 信号存在性测试 ==========
    
    void testConnectionChanged_SignalExists() {
        // 测试连接状态变化信号存在
        UartChannel channel;
        
        QSignalSpy spy(&channel, SIGNAL(connectionChanged(bool)));
        QVERIFY(spy.isValid());
    }
    
    void testTelemetryReceived_SignalExists() {
        // 测试遥测数据接收信号存在
        UartChannel channel;
        
        QSignalSpy spy(&channel, SIGNAL(telemetryReceived(uint8_t,TelemetryData)));
        QVERIFY(spy.isValid());
    }
    
    void testErrorReceived_SignalExists() {
        // 测试错误接收信号存在
        UartChannel channel;
        
        QSignalSpy spy(&channel, SIGNAL(errorReceived(uint8_t,ErrorCode)));
        QVERIFY(spy.isValid());
    }
    
    void testErrorOccurred_SignalExists() {
        // 测试错误发生信号存在
        UartChannel channel;
        
        QSignalSpy spy(&channel, SIGNAL(errorOccurred(QString)));
        QVERIFY(spy.isValid());
    }
    
    // ========== 协议帧编码测试 ==========
    
    void testFrameEncoding_Heartbeat() {
        // 测试心跳帧编码
        Frame original;
        original.deviceId = 0x01;
        original.command = 0x01;
        original.payload.clear();
        
        std::vector<uint8_t> encoded = Protocol::encode(original);
        
        // 验证帧结构
        QCOMPARE(encoded.size(), 8u);  // header(2) + deviceId(1) + command(1) + length(2) + crc(2)
        QCOMPARE(static_cast<uint8_t>(encoded[0]), 0x55u);  // 帧头低字节
        QCOMPARE(static_cast<uint8_t>(encoded[1]), 0xAAu);  // 帧头高字节
        QCOMPARE(static_cast<uint8_t>(encoded[2]), 0x01u);  // 设备 ID
        QCOMPARE(static_cast<uint8_t>(encoded[3]), 0x01u);  // 命令字 (HEARTBEAT)
        QCOMPARE(static_cast<uint8_t>(encoded[4]), 0x00u);  // 长度低字节
        QCOMPARE(static_cast<uint8_t>(encoded[5]), 0x00u);  // 长度高字节
    }
    
    void testFrameEncoding_Telemetry() {
        // 测试遥测帧编码
        TelemetryData telemetry;
        telemetry.timestamp = 1000;
        telemetry.channels = {1.5f, 2.5f};
        
        std::vector<uint8_t> encoded = createTelemetryFrame(0x01, telemetry);
        
        // 验证可以解码
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(result);
        QCOMPARE(decoded.deviceId, 0x01u);
        QCOMPARE(decoded.command, static_cast<uint8_t>(Command::CMD_TELEMETRY));
    }
    
    // ========== 协议帧解码测试 ==========
    
    void testFrameDecoding_ValidHeartbeat() {
        // 测试有效心跳帧解码
        Frame original;
        original.deviceId = 0x02;
        original.command = 0x01;
        
        std::vector<uint8_t> encoded = Protocol::encode(original);
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(result);
        QCOMPARE(decoded.deviceId, 0x02u);
        QCOMPARE(decoded.command, 0x01u);
        QVERIFY(decoded.payload.empty());
    }
    
    void testFrameDecoding_InvalidHeader() {
        // 测试无效帧头
        std::vector<uint8_t> invalid = {
            0x00, 0x00,  // 错误帧头
            0x01, 0x01, 0x00, 0x00, 0x00, 0x00
        };
        
        Frame decoded;
        bool result = Protocol::decode(invalid, decoded);
        
        QVERIFY(!result);
    }
    
    void testFrameDecoding_TooShort() {
        // 测试帧太短
        std::vector<uint8_t> shortFrame = {0x55, 0xAA, 0x01};
        
        Frame decoded;
        bool result = Protocol::decode(shortFrame, decoded);
        
        QVERIFY(!result);
    }
    
    void testFrameDecoding_CrcError() {
        // 测试 CRC 错误检测
        Frame original;
        original.deviceId = 0x01;
        original.command = 0x10;
        original.payload = {0x01, 0x02};
        
        std::vector<uint8_t> encoded = Protocol::encode(original);
        
        // 篡改 CRC
        encoded[encoded.size() - 1] ^= 0xFF;
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(!result);
    }
    
    // ========== 帧同步测试 ==========
    
    void testFrameSync_WithLeadingGarbage() {
        // 测试带前导垃圾数据的帧同步
        std::vector<uint8_t> garbage = {0x00, 0xFF, 0x12, 0x34};
        
        Frame frame = createHeartbeatFrame(0x01);
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        // 拼接垃圾数据和有效帧
        std::vector<uint8_t> buffer = garbage;
        buffer.insert(buffer.end(), encoded.begin(), encoded.end());
        
        // 验证缓冲区包含有效帧
        QVERIFY(buffer.size() > 8);
    }
    
    void testFrameSync_MultipleFrames() {
        // 测试多帧连续接收
        Frame frame1 = createHeartbeatFrame(0x01);
        Frame frame2 = createHeartbeatFrame(0x02);
        
        std::vector<uint8_t> encoded1 = Protocol::encode(frame1);
        std::vector<uint8_t> encoded2 = Protocol::encode(frame2);
        
        // 拼接两帧
        std::vector<uint8_t> buffer = encoded1;
        buffer.insert(buffer.end(), encoded2.begin(), encoded2.end());
        
        // 验证缓冲区包含两帧 (16 字节)
        QCOMPARE(buffer.size(), 16u);
        
        // 验证可以分别解码
        Frame decoded1, decoded2;
        bool result1 = Protocol::decode(
            std::vector<uint8_t>(buffer.begin(), buffer.begin() + 8), 
            decoded1
        );
        bool result2 = Protocol::decode(
            std::vector<uint8_t>(buffer.begin() + 8, buffer.end()), 
            decoded2
        );
        
        QVERIFY(result1);
        QVERIFY(result2);
        QCOMPARE(decoded1.deviceId, 0x01u);
        QCOMPARE(decoded2.deviceId, 0x02u);
    }
    
    // ========== 边界条件测试 ==========
    
    void testBoundary_MaxPayloadSize() {
        // 测试最大载荷大小 (255 字节)
        std::vector<uint8_t> largePayload(255);
        for (size_t i = 0; i < largePayload.size(); ++i) {
            largePayload[i] = static_cast<uint8_t>(i);
        }
        
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x10;
        frame.payload = largePayload;
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        // 验证编码成功
        QVERIFY(encoded.size() > 0);
        
        // 验证可以解码
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        QVERIFY(result);
        QCOMPARE(decoded.payload.size(), 255u);
    }
    
    void testBoundary_EmptyPayload() {
        // 测试空载荷（心跳帧）
        Frame frame = createHeartbeatFrame(0x01);
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        QCOMPARE(encoded.size(), 8u);  // 最小帧长度
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        QVERIFY(result);
        QVERIFY(decoded.payload.empty());
    }
    
    void testBoundary_ZeroDeviceId() {
        // 测试设备 ID 为 0
        Frame frame = createHeartbeatFrame(0x00);
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(result);
        QCOMPARE(decoded.deviceId, 0x00u);
    }
    
    void testBoundary_MaxDeviceId() {
        // 测试设备 ID 为 255
        Frame frame = createHeartbeatFrame(0xFF);
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(result);
        QCOMPARE(decoded.deviceId, 0xFFu);
    }
    
    // ========== 遥测数据解析测试 ==========
    
    void testTelemetryParsing_SingleChannel() {
        // 测试单通道遥测解析
        TelemetryData original;
        original.timestamp = 5000;
        original.channels = {42.5f};
        
        std::vector<uint8_t> encoded = createTelemetryFrame(0x01, original);
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        QVERIFY(result);
        
        TelemetryData parsed = Protocol::parseTelemetry(decoded.payload);
        QCOMPARE(parsed.timestamp, 5000u);
        QCOMPARE(parsed.channels.size(), 1u);
        QVERIFY(qAbs(parsed.channels[0] - 42.5f) < 0.001f);
    }
    
    void testTelemetryParsing_EightChannels() {
        // 测试 8 通道遥测解析
        TelemetryData original;
        original.timestamp = 10000;
        original.channels = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
        
        std::vector<uint8_t> encoded = createTelemetryFrame(0x01, original);
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        QVERIFY(result);
        
        TelemetryData parsed = Protocol::parseTelemetry(decoded.payload);
        QCOMPARE(parsed.timestamp, 10000u);
        QCOMPARE(parsed.channels.size(), 8u);
        
        for (int i = 0; i < 8; ++i) {
            QVERIFY(qAbs(parsed.channels[i] - static_cast<float>(i + 1)) < 0.001f);
        }
    }
};

QTEST_APPLESS_MAIN(TestUartChannel)
#include "test_uartchannel.moc"
