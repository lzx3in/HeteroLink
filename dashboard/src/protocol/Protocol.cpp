/**
 * HeteroLink Host - 协议编解码实现
 * 
 * @file Protocol.cpp
 */

#include "Protocol.h"
#include <cstring>
#include <stdexcept>

namespace HeteroLink {

// CRC16 查找表
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

std::vector<uint8_t> Protocol::encode(const Frame& frame)
{
    std::vector<uint8_t> buffer;
    
    // 计算总长度
    size_t totalLen = 8 + frame.payload.size();  // header(2) + deviceId(1) + command(1) + length(2) + payload(N) + crc(2)
    buffer.reserve(totalLen);
    
    // 帧头 (小端)
    buffer.push_back(frame.header & 0xFF);
    buffer.push_back((frame.header >> 8) & 0xFF);
    
    // 设备 ID
    buffer.push_back(frame.deviceId);
    
    // 命令字
    buffer.push_back(frame.command);
    
    // 数据长度 (小端)
    uint16_t len = static_cast<uint16_t>(frame.payload.size());
    buffer.push_back(len & 0xFF);
    buffer.push_back((len >> 8) & 0xFF);
    
    // 数据载荷
    for (uint8_t byte : frame.payload) {
        buffer.push_back(byte);
    }
    
    // 计算 CRC（从 deviceId 到 payload 末尾）
    uint16_t crc = crc16(&buffer[2], buffer.size() - 2);
    
    // CRC (小端)
    buffer.push_back(crc & 0xFF);
    buffer.push_back((crc >> 8) & 0xFF);
    
    return buffer;
}

bool Protocol::decode(const std::vector<uint8_t>& data, Frame& frame)
{
    // 最小帧长度：header(2) + deviceId(1) + command(1) + length(2) + crc(2) = 8
    if (data.size() < 8) {
        return false;
    }
    
    // 检查帧头
    uint16_t header = data[0] | (data[1] << 8);
    if (header != FRAME_HEADER) {
        return false;
    }
    frame.header = header;
    
    // 解析设备 ID
    frame.deviceId = data[2];
    
    // 解析命令字
    frame.command = data[3];
    
    // 解析数据长度
    uint16_t len = data[4] | (data[5] << 8);
    frame.length = len;
    
    // 检查数据完整性
    if (data.size() < 8 + len) {
        return false;
    }
    
    // 解析数据载荷
    frame.payload.assign(data.begin() + 6, data.begin() + 6 + len);
    
    // 解析 CRC
    uint16_t receivedCrc = data[6 + len] | (data[7 + len] << 8);
    frame.crc = receivedCrc;
    
    // 验证 CRC
    if (!verifyCrc(frame)) {
        return false;
    }
    
    return true;
}

uint16_t Protocol::crc16(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc16_table[index];
    }
    
    return crc;
}

bool Protocol::verifyCrc(const Frame& frame)
{
    // 重新计算 CRC
    std::vector<uint8_t> tempBuffer;
    tempBuffer.push_back(frame.deviceId);
    tempBuffer.push_back(frame.command);
    tempBuffer.push_back(frame.length & 0xFF);
    tempBuffer.push_back((frame.length >> 8) & 0xFF);
    for (uint8_t byte : frame.payload) {
        tempBuffer.push_back(byte);
    }
    
    uint16_t calculatedCrc = crc16(tempBuffer.data(), tempBuffer.size());
    
    return calculatedCrc == frame.crc;
}

TelemetryData Protocol::parseTelemetry(const std::vector<uint8_t>& payload)
{
    TelemetryData telemetry;
    
    // 最小长度：timestamp(4) + 至少 1 个通道 (4)
    if (payload.size() < 8) {
        return telemetry;
    }
    
    // 解析时间戳
    telemetry.timestamp = payload[0] | (payload[1] << 8) | 
                         (payload[2] << 16) | (payload[3] << 24);
    
    // 解析通道数据（每个通道 4 字节 float）
    size_t channelCount = (payload.size() - 4) / 4;
    telemetry.channels.reserve(channelCount);
    
    for (size_t i = 0; i < channelCount; ++i) {
        size_t offset = 4 + i * 4;
        uint32_t floatBits = payload[offset] | (payload[offset + 1] << 8) |
                            (payload[offset + 2] << 16) | (payload[offset + 3] << 24);
        float value;
        std::memcpy(&value, &floatBits, sizeof(float));
        telemetry.channels.push_back(value);
    }
    
    return telemetry;
}

Frame Protocol::createHeartbeat(uint8_t deviceId)
{
    Frame frame;
    frame.deviceId = deviceId;
    frame.command = static_cast<uint8_t>(Command::CMD_HEARTBEAT);
    frame.length = 0;
    frame.payload.clear();
    return frame;
}

Frame Protocol::createControlCommand(uint8_t deviceId, uint8_t cmdType,
                                    const std::vector<uint8_t>& payload)
{
    Frame frame;
    frame.deviceId = deviceId;
    frame.command = static_cast<uint8_t>(Command::CMD_CONTROL);
    frame.payload = payload;
    frame.payload.insert(frame.payload.begin(), cmdType);
    frame.length = static_cast<uint16_t>(frame.payload.size());
    return frame;
}

} // namespace HeteroLink
