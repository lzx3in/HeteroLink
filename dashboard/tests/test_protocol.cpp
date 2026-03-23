/**
 * HeteroLink Host - 协议层单元测试
 * 
 * 测试 Protocol 类的编解码、CRC 校验、遥测解析功能
 */

#include <QTest>
#include <QDebug>

#include "protocol/Protocol.h"

using namespace HeteroLink;

class TestProtocol : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting Protocol unit tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "Protocol unit tests finished";
    }
    
    // ========== CRC16 测试 ==========
    
    void testCrc16_EmptyData() {
        // 空数据的 CRC
        uint16_t crc = Protocol::crc16(nullptr, 0);
        QCOMPARE(crc, 0xFFFFu);  // 空数据应返回初始值
    }
    
    void testCrc16_SingleByte() {
        // 单字节 CRC（0x00 的 CRC16 是 0x40BF）
        uint8_t data[] = {0x00};
        uint16_t crc = Protocol::crc16(data, 1);
        QCOMPARE(crc, 0x40BFu);
    }
    
    void testCrc16_KnownValue() {
        // 已知数据的 CRC（用于回归测试）
        uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
        uint16_t crc = Protocol::crc16(data, 4);
        QVERIFY(crc != 0);  // CRC 不应为 0
        QVERIFY(crc != 0xFFFF);  // 也不应是初始值
    }
    
    void testCrc16_Repeatability() {
        // 重复性：相同数据应产生相同 CRC
        uint8_t data[] = {0x12, 0x34, 0x56};
        uint16_t crc1 = Protocol::crc16(data, 3);
        uint16_t crc2 = Protocol::crc16(data, 3);
        QCOMPARE(crc1, crc2);
    }
    
    // ========== 帧编码测试 ==========
    
    void testEncode_HeartbeatFrame() {
        // 编码心跳帧（无 payload）
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = static_cast<uint8_t>(Command::CMD_HEARTBEAT);
        frame.payload.clear();
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        // 验证帧结构：header(2) + deviceId(1) + command(1) + length(2) + crc(2) = 8
        QCOMPARE(encoded.size(), 8u);
        QCOMPARE(encoded[0], 0x55u);  // 帧头低字节 (0xAA55 小端)
        QCOMPARE(encoded[1], 0xAAu);  // 帧头高字节
        QCOMPARE(encoded[2], 0x01u);  // 设备 ID
        QCOMPARE(encoded[3], 0x01u);  // 命令字 (CMD_HEARTBEAT)
        QCOMPARE(encoded[4], 0x00u);  // 长度低字节
        QCOMPARE(encoded[5], 0x00u);  // 长度高字节
        // CRC 在最后 2 字节
    }
    
    void testEncode_WithPayload() {
        // 编码带 payload 的帧
        Frame frame;
        frame.deviceId = 0x02;
        frame.command = 0x10;
        frame.payload = {0x01, 0x02, 0x03, 0x04};
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        // 总长度：8 + 4 = 12
        QCOMPARE(encoded.size(), 12u);
        QCOMPARE(encoded[2], 0x02u);  // 设备 ID
        QCOMPARE(encoded[3], 0x10u);  // 命令字
        QCOMPARE(encoded[4], 0x04u);  // 长度低字节
        QCOMPARE(encoded[5], 0x00u);  // 长度高字节
        QCOMPARE(encoded[6], 0x01u);  // payload[0]
        QCOMPARE(encoded[7], 0x02u);  // payload[1]
        QCOMPARE(encoded[8], 0x03u);  // payload[2]
        QCOMPARE(encoded[9], 0x04u);  // payload[3]
    }
    
    void testEncode_VerifyCrcIncluded() {
        // 验证编码包含 CRC
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x10;
        frame.payload = {0xAA, 0xBB};
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        
        // 提取接收到的 CRC
        uint16_t receivedCrc = encoded[8] | (encoded[9] << 8);
        QVERIFY(receivedCrc != 0);
    }
    
    // ========== 帧解码测试 ==========
    
    void testDecode_ValidHeartbeat() {
        // 解码有效的心跳帧
        std::vector<uint8_t> encoded = {
            0x55, 0xAA,  // 帧头
            0x01,        // 设备 ID
            0x01,        // 命令字 (HEARTBEAT)
            0x00, 0x00,  // 长度
            0x00, 0x00   // CRC (占位，实际会被验证)
        };
        
        // 先编码一个真正的帧来获取正确 CRC
        Frame original;
        original.deviceId = 0x01;
        original.command = 0x01;
        std::vector<uint8_t> properEncoded = Protocol::encode(original);
        
        Frame decoded;
        bool result = Protocol::decode(properEncoded, decoded);
        
        QVERIFY(result);
        QCOMPARE(decoded.header, FRAME_HEADER);
        QCOMPARE(decoded.deviceId, 0x01u);
        QCOMPARE(decoded.command, 0x01u);
        QCOMPARE(decoded.length, 0u);
        QVERIFY(decoded.payload.empty());
    }
    
    void testDecode_WithPayload() {
        // 解码带 payload 的帧
        Frame original;
        original.deviceId = 0x03;
        original.command = 0x10;
        original.payload = {0x11, 0x22, 0x33};
        
        std::vector<uint8_t> encoded = Protocol::encode(original);
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(result);
        QCOMPARE(decoded.deviceId, 0x03u);
        QCOMPARE(decoded.command, 0x10u);
        QCOMPARE(decoded.length, 3u);
        QCOMPARE(decoded.payload.size(), 3u);
        QCOMPARE(decoded.payload[0], 0x11u);
        QCOMPARE(decoded.payload[1], 0x22u);
        QCOMPARE(decoded.payload[2], 0x33u);
    }
    
    void testDecode_InvalidHeader() {
        // 无效帧头应失败
        std::vector<uint8_t> invalid = {
            0x00, 0x00,  // 错误帧头
            0x01, 0x01, 0x00, 0x00, 0x00, 0x00
        };
        
        Frame decoded;
        bool result = Protocol::decode(invalid, decoded);
        
        QVERIFY(!result);
    }
    
    void testDecode_TooShort() {
        // 帧太短应失败
        std::vector<uint8_t> shortFrame = {0x55, 0xAA, 0x01};
        
        Frame decoded;
        bool result = Protocol::decode(shortFrame, decoded);
        
        QVERIFY(!result);
    }
    
    void testDecode_CrcError() {
        // CRC 错误应失败
        Frame original;
        original.deviceId = 0x01;
        original.command = 0x10;
        original.payload = {0x01, 0x02};
        
        std::vector<uint8_t> encoded = Protocol::encode(original);
        
        // 篡改 CRC
        encoded[encoded.size() - 1] ^= 0xFF;
        
        Frame decoded;
        bool result = Protocol::decode(encoded, decoded);
        
        QVERIFY(!result);  // 应因 CRC 错误失败
    }
    
    // ========== CRC 验证测试 ==========
    
    void testVerifyCrc_ValidFrame() {
        // 有效帧应通过 CRC 验证
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x10;
        frame.payload = {0x01, 0x02, 0x03};
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        Frame decoded;
        Protocol::decode(encoded, decoded);
        
        bool valid = Protocol::verifyCrc(decoded);
        QVERIFY(valid);
    }
    
    void testVerifyCrc_TamperedPayload() {
        // 篡改 payload 后 CRC 应失败
        Frame frame;
        frame.deviceId = 0x01;
        frame.command = 0x10;
        frame.payload = {0x01, 0x02};
        
        std::vector<uint8_t> encoded = Protocol::encode(frame);
        Frame decoded;
        Protocol::decode(encoded, decoded);
        
        // 篡改 payload
        decoded.payload[0] = 0xFF;
        
        bool valid = Protocol::verifyCrc(decoded);
        QVERIFY(!valid);
    }
    
    // ========== 遥测数据解析测试 ==========
    
    void testParseTelemetry_Basic() {
        // 解析基本遥测数据（小端序 IEEE 754）
        std::vector<uint8_t> payload = {
            0xE8, 0x03, 0x00, 0x00,  // 时间戳 1000
            0x00, 0x00, 0x80, 0x3F,  // 1.0f (IEEE 754 小端：0x3F800000)
            0x00, 0x00, 0x40, 0x40,  // 3.0f (IEEE 754 小端：0x40400000)
        };
        
        TelemetryData telemetry = Protocol::parseTelemetry(payload);
        
        QCOMPARE(telemetry.timestamp, 1000u);
        QCOMPARE(telemetry.channels.size(), 2u);
        QVERIFY(qAbs(telemetry.channels[0] - 1.0f) < 0.001f);
        QVERIFY(qAbs(telemetry.channels[1] - 3.0f) < 0.001f);
    }
    
    void testParseTelemetry_Empty() {
        // 空 payload 应返回空遥测
        std::vector<uint8_t> emptyPayload;
        TelemetryData telemetry = Protocol::parseTelemetry(emptyPayload);
        
        QCOMPARE(telemetry.timestamp, 0u);
        QVERIFY(telemetry.channels.empty());
    }
    
    void testParseTelemetry_TooShort() {
        // payload 太短（少于 8 字节）应返回空遥测
        std::vector<uint8_t> shortPayload = {0x01, 0x02, 0x03};
        TelemetryData telemetry = Protocol::parseTelemetry(shortPayload);
        
        QCOMPARE(telemetry.timestamp, 0u);
        QVERIFY(telemetry.channels.empty());
    }
    
    void testParseTelemetry_MultipleChannels() {
        // 多通道遥测
        std::vector<uint8_t> payload(24);  // 4 字节时间戳 + 5 个通道 * 4 字节
        
        // 时间戳
        payload[0] = 0x10, payload[1] = 0x27, payload[2] = 0x00, payload[3] = 0x00;  // 10000
        
        // 5 个通道：1.0, 2.0, 3.0, 4.0, 5.0
        for (int i = 0; i < 5; ++i) {
            float val = static_cast<float>(i + 1);
            std::memcpy(&payload[4 + i * 4], &val, sizeof(float));
        }
        
        TelemetryData telemetry = Protocol::parseTelemetry(payload);
        
        QCOMPARE(telemetry.timestamp, 10000u);
        QCOMPARE(telemetry.channels.size(), 5u);
        for (int i = 0; i < 5; ++i) {
            QVERIFY(qAbs(telemetry.channels[i] - static_cast<float>(i + 1)) < 0.001f);
        }
    }
    
    // ========== 工厂方法测试 ==========
    
    void testCreateHeartbeat() {
        Frame frame = Protocol::createHeartbeat(0x05);
        
        QCOMPARE(frame.deviceId, 0x05u);
        QCOMPARE(frame.command, static_cast<uint8_t>(Command::CMD_HEARTBEAT));
        QVERIFY(frame.payload.empty());
    }
    
    void testCreateControlCommand() {
        std::vector<uint8_t> params = {0xAA, 0xBB};
        Frame frame = Protocol::createControlCommand(0x02, 0x01, params);
        
        QCOMPARE(frame.deviceId, 0x02u);
        QCOMPARE(frame.command, static_cast<uint8_t>(Command::CMD_CONTROL));
        QCOMPARE(frame.payload.size(), 3u);  // cmdType + params
        QCOMPARE(frame.payload[0], 0x01u);   // cmdType 在第一位
        QCOMPARE(frame.payload[1], 0xAAu);
        QCOMPARE(frame.payload[2], 0xBBu);
    }
};

QTEST_APPLESS_MAIN(TestProtocol)
#include "test_protocol.moc"
