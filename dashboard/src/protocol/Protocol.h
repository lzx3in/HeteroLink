/**
 * HeteroLink Host - 协议定义
 * 
 * @file Protocol.h
 * @brief UART 二进制协议定义和编解码
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace HeteroLink {

// 帧头魔数
constexpr uint16_t FRAME_HEADER = 0xAA55;

// 命令字定义
enum class Command : uint8_t {
    CMD_HEARTBEAT    = 0x01,  // 心跳包
    CMD_CONFIG_REQ   = 0x02,  // 请求配置
    CMD_CONFIG_RSP   = 0x03,  // 配置响应
    CMD_TELEMETRY    = 0x10,  // 遥测数据
    CMD_CONTROL      = 0x20,  // 控制命令
    CMD_LOG_UPLOAD   = 0x30,  // 日志上传
    CMD_ERROR        = 0xFF   // 错误报告
};

// 错误码
enum class ErrorCode : uint8_t {
    ERR_NONE         = 0x00,
    ERR_INVALID_CMD  = 0x01,
    ERR_INVALID_LEN  = 0x02,
    ERR_CRC_ERROR    = 0x03,
    ERR_TIMEOUT      = 0x04,
    ERR_DEVICE_BUSY  = 0x05,
    ERR_UNKNOWN      = 0xFF
};

/**
 * @brief 协议帧结构
 */
struct Frame {
    uint16_t header;      // 帧头 0xAA55
    uint8_t deviceId;     // 设备 ID
    uint8_t command;      // 命令字
    uint16_t length;      // 数据长度
    std::vector<uint8_t> payload;  // 数据载荷
    uint16_t crc;         // CRC16 校验和
    
    Frame() : header(FRAME_HEADER), deviceId(0), command(0), length(0), crc(0) {}
};

/**
 * @brief 遥测数据结构
 */
struct TelemetryData {
    uint32_t timestamp;   // 时间戳 (ms)
    std::vector<float> channels;  // 多通道数据
    
    TelemetryData() : timestamp(0) {}
};

/**
 * @brief 协议编解码器
 */
class Protocol {
public:
    /**
     * @brief 编码帧
     * @param frame 输入帧
     * @return 编码后的字节流
     */
    static std::vector<uint8_t> encode(const Frame& frame);
    
    /**
     * @brief 解码帧
     * @param data 输入字节流
     * @param frame 输出帧
     * @return 是否成功
     */
    static bool decode(const std::vector<uint8_t>& data, Frame& frame);
    
    /**
     * @brief 计算 CRC16
     * @param data 数据
     * @param length 长度
     * @return CRC 值
     */
    static uint16_t crc16(const uint8_t* data, size_t length);
    
    /**
     * @brief 验证帧校验和
     * @param frame 帧
     * @return 是否有效
     */
    static bool verifyCrc(const Frame& frame);
    
    /**
     * @brief 解析遥测数据
     * @param payload 数据载荷
     * @return 遥测数据
     */
    static TelemetryData parseTelemetry(const std::vector<uint8_t>& payload);
    
    /**
     * @brief 创建心跳帧
     * @param deviceId 设备 ID
     * @return 帧
     */
    static Frame createHeartbeat(uint8_t deviceId);
    
    /**
     * @brief 创建控制命令帧
     * @param deviceId 设备 ID
     * @param cmdType 命令类型
     * @param payload 命令参数
     * @return 帧
     */
    static Frame createControlCommand(uint8_t deviceId, uint8_t cmdType, 
                                     const std::vector<uint8_t>& payload);
};

} // namespace HeteroLink
