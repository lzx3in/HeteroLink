/**
 * HeteroLink Host - 测试辅助工具
 * 
 * 提供测试数据生成、断言辅助、Mock 工具等
 */

#pragma once

#include <QVector>
#include <QString>
#include <QDateTime>
#include <random>
#include <cstring>

#include "protocol/Protocol.h"
#include "core/DataProcessor.h"

namespace HeteroLink {
namespace TestHelpers {

/**
 * @brief 生成随机遥测数据
 * @param channelCount 通道数量
 * @param minValue 最小值
 * @param maxValue 最大值
 * @return 随机遥测数据
 */
inline TelemetryData generateRandomTelemetry(
    int channelCount = 4,
    float minValue = 0.0f,
    float maxValue = 100.0f
) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(minValue, maxValue);
    
    TelemetryData data;
    data.timestamp = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());
    data.channels.reserve(channelCount);
    
    for (int i = 0; i < channelCount; ++i) {
        data.channels.push_back(dist(gen));
    }
    
    return data;
}

/**
 * @brief 生成带时间戳序列的遥测数据
 * @param baseTimestamp 基准时间戳
 * @param channelValues 各通道值
 * @return 遥测数据
 */
inline TelemetryData createTelemetry(
    uint32_t baseTimestamp,
    const std::vector<float>& channelValues
) {
    TelemetryData data;
    data.timestamp = baseTimestamp;
    data.channels = channelValues;
    return data;
}

/**
 * @brief 创建已知 CRC 的测试帧（用于验证 CRC 计算）
 * @param deviceId 设备 ID
 * @param command 命令字
 * @param payload 载荷
 * @return 完整的协议帧
 */
inline Frame createTestFrame(
    uint8_t deviceId,
    uint8_t command,
    const std::vector<uint8_t>& payload = {}
) {
    Frame frame;
    frame.deviceId = deviceId;
    frame.command = command;
    frame.payload = payload;
    frame.length = static_cast<uint16_t>(payload.size());
    return frame;
}

/**
 * @brief 创建心跳帧（快捷方式）
 * @param deviceId 设备 ID
 * @return 心跳帧
 */
inline Frame createHeartbeatFrame(uint8_t deviceId) {
    return createTestFrame(deviceId, static_cast<uint8_t>(Command::CMD_HEARTBEAT));
}

/**
 * @brief 创建遥测帧（快捷方式）
 * @param deviceId 设备 ID
 * @param telemetry 遥测数据
 * @return 编码后的字节流
 */
inline std::vector<uint8_t> createTelemetryFrame(
    uint8_t deviceId,
    const TelemetryData& telemetry
) {
    Frame frame;
    frame.deviceId = deviceId;
    frame.command = static_cast<uint8_t>(Command::CMD_TELEMETRY);
    
    // 序列化遥测数据
    frame.payload.reserve(4 + telemetry.channels.size() * 4);
    
    // 时间戳（小端）
    frame.payload.push_back(telemetry.timestamp & 0xFF);
    frame.payload.push_back((telemetry.timestamp >> 8) & 0xFF);
    frame.payload.push_back((telemetry.timestamp >> 16) & 0xFF);
    frame.payload.push_back((telemetry.timestamp >> 24) & 0xFF);
    
    // 通道数据（小端 float）
    for (float ch : telemetry.channels) {
        uint32_t bits;
        std::memcpy(&bits, &ch, sizeof(float));
        frame.payload.push_back(bits & 0xFF);
        frame.payload.push_back((bits >> 8) & 0xFF);
        frame.payload.push_back((bits >> 16) & 0xFF);
        frame.payload.push_back((bits >> 24) & 0xFF);
    }
    
    return Protocol::encode(frame);
}

/**
 * @brief 验证两个浮点数向量是否近似相等
 * @param a 向量 a
 * @param b 向量 b
 * @param epsilon 容差
 * @return 是否相等
 */
inline bool vectorsApproxEqual(
    const std::vector<float>& a,
    const std::vector<float>& b,
    float epsilon = 0.001f
) {
    if (a.size() != b.size()) {
        return false;
    }
    
    for (size_t i = 0; i < a.size(); ++i) {
        if (qAbs(a[i] - b[i]) > epsilon) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 生成模拟设备数据（用于压力测试）
 * @param processor 数据处理器
 * @param deviceId 设备 ID
 * @param sampleCount 样本数量
 * @param channelCount 通道数量
 */
inline void generateSimulatedData(
    DataProcessor& processor,
    const QString& deviceId,
    int sampleCount = 100,
    int channelCount = 4
) {
    uint32_t baseTime = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());
    
    for (int i = 0; i < sampleCount; ++i) {
        TelemetryData data;
        data.timestamp = baseTime + i * 100;  // 每 100ms 一个样本
        data.channels.reserve(channelCount);
        
        // 生成正弦波模拟数据
        for (int ch = 0; ch < channelCount; ++ch) {
            float value = 50.0f + 25.0f * std::sin((i + ch * 10) * 0.1f);
            data.channels.push_back(value);
        }
        
        processor.addData(deviceId, data);
    }
}

/**
 * @brief 打印帧内容（用于调试）
 * @param frame 协议帧
 * @return 可读字符串
 */
inline QString frameToString(const Frame& frame) {
    QString result;
    result += QString("Frame[deviceId=%1, command=0x%2, length=%3, crc=0x%4]")
        .arg(frame.deviceId)
        .arg(frame.command, 2, 16, QChar('0'))
        .arg(frame.length)
        .arg(frame.crc, 4, 16, QChar('0'));
    
    if (!frame.payload.empty()) {
        result += " payload=[";
        QStringList hexBytes;
        for (uint8_t byte : frame.payload) {
            hexBytes << QString("%1").arg(byte, 2, 16, QChar('0'));
        }
        result += hexBytes.join(" ");
        result += "]";
    }
    
    return result;
}

/**
 * @brief 打印字节流（用于调试）
 * @param data 字节流
 * @param maxBytes 最大打印字节数
 * @return 可读字符串
 */
inline QString bytesToHex(const std::vector<uint8_t>& data, int maxBytes = 32) {
    QString result;
    int printLen = qMin(static_cast<int>(data.size()), maxBytes);
    
    for (int i = 0; i < printLen; ++i) {
        result += QString("%1 ").arg(data[i], 2, 16, QChar('0'));
    }
    
    if (data.size() > static_cast<size_t>(maxBytes)) {
        result += QString("... (%1 more bytes)").arg(data.size() - maxBytes);
    }
    
    return result.trimmed();
}

} // namespace TestHelpers
} // namespace HeteroLink
