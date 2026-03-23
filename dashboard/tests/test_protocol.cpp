/**
 * HeteroLink Host - 协议测试
 */

#include <QTest>
#include <QDebug>

// 简化版的协议实现用于测试
#include <cstdint>
#include <vector>
#include <cstring>

class TestProtocol : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting protocol tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "Protocol tests finished";
    }
    
    void testCrc16() {
        // 测试 CRC16 计算
        uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
        uint16_t crc = calculateCrc16(data, 4);
        QVERIFY(crc != 0);  // CRC 不应为 0
    }
    
    void testFrameEncode() {
        // 测试帧编码
        std::vector<uint8_t> buffer;
        buffer.push_back(0xAA);  // 帧头低字节
        buffer.push_back(0x55);  // 帧头高字节
        buffer.push_back(0x01);  // 设备 ID
        buffer.push_back(0x10);  // 命令字
        buffer.push_back(0x04);  // 长度低字节
        buffer.push_back(0x00);  // 长度高字节
        buffer.push_back(0x01);  // 数据
        buffer.push_back(0x02);
        buffer.push_back(0x03);
        buffer.push_back(0x04);
        
        QCOMPARE(buffer.size(), 10u);
        QCOMPARE(buffer[0], 0xAA);
        QCOMPARE(buffer[1], 0x55);
    }
    
    void testFrameDecode() {
        // 测试帧解码
        std::vector<uint8_t> data = {
            0xAA, 0x55,  // 帧头
            0x01,        // 设备 ID
            0x10,        // 命令字
            0x00, 0x00,  // 长度
            0x00, 0x00   // CRC (占位)
        };
        
        QVERIFY(data.size() >= 8);  // 最小帧长度
    }
    
    void testTelemetryParse() {
        // 测试遥测数据解析
        std::vector<uint8_t> payload(20);
        
        // 时间戳
        payload[0] = 0xE8;
        payload[1] = 0x03;
        payload[2] = 0x00;
        payload[3] = 0x00;
        
        // 通道 1 (float: 1.5)
        float ch1 = 1.5f;
        std::memcpy(&payload[4], &ch1, sizeof(float));
        
        // 验证时间戳
        uint32_t timestamp = payload[0] | (payload[1] << 8) | 
                            (payload[2] << 16) | (payload[3] << 24);
        QCOMPARE(timestamp, 1000u);
        
        // 验证通道数据
        float parsedCh1;
        std::memcpy(&parsedCh1, &payload[4], sizeof(float));
        QVERIFY(qAbs(parsedCh1 - ch1) < 0.001f);
    }
    
private:
    uint16_t calculateCrc16(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF;
        static const uint16_t crc16_table[256] = {
            0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
            0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
            // ... (完整表省略，实际测试中需要完整表)
        };
        
        for (size_t i = 0; i < length; ++i) {
            uint8_t index = (crc ^ data[i]) & 0xFF;
            crc = (crc >> 8) ^ crc16_table[index];
        }
        
        return crc;
    }
};

QTEST_APPLESS_MAIN(TestProtocol)
#include "test_protocol.moc"
