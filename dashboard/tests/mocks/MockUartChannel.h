/**
 * HeteroLink Host - Mock UART 通道
 * 
 * @file MockUartChannel.h
 * @brief 用于单元测试的虚拟 UART 通道，无需真实硬件
 */

#pragma once

#include <QObject>
#include <QVector>
#include <QQueue>
#include <QTimer>

#include "protocol/UartChannel.h"
#include "protocol/Protocol.h"

namespace HeteroLink {

/**
 * @brief 模拟 UART 通道
 * 
 * 完全在内存中模拟串口通信行为，用于单元测试
 */
class MockUartChannel : public UartChannel
{
    Q_OBJECT
    
public:
    explicit MockUartChannel(QObject *parent = nullptr);
    ~MockUartChannel();
    
    /**
     * @brief 获取可用串口列表（模拟）
     * @return 模拟的串口信息列表
     */
    static QVector<QSerialPortInfo> availablePorts();
    
    /**
     * @brief 连接串口（模拟）
     * @param config 配置
     * @return 总是返回 true
     */
    bool connect(const UartConfig& config) override;
    
    /**
     * @brief 断开连接（模拟）
     */
    void disconnect() override;
    
    /**
     * @brief 检查连接状态
     * @return 是否已连接
     */
    bool isConnected() const override;
    
    /**
     * @brief 获取当前配置
     * @return UART 配置
     */
    UartConfig getConfig() const { return config_; }
    
    /**
     * @brief 发送帧（模拟）
     * @param frame 帧
     * @return 总是返回 true
     */
    bool sendFrame(const Frame& frame) override;
    
    /**
     * @brief 模拟接收数据
     * @param data 接收到的原始数据
     */
    void simulateReceive(const QByteArray& data);
    
    /**
     * @brief 模拟接收遥测帧
     * @param deviceId 设备 ID
     * @param telemetry 遥测数据
     */
    void simulateTelemetry(uint8_t deviceId, const TelemetryData& telemetry);
    
    /**
     * @brief 模拟接收错误
     * @param deviceId 设备 ID
     * @param error 错误码
     */
    void simulateError(uint8_t deviceId, ErrorCode error);
    
    /**
     * @brief 模拟连接断开
     */
    void simulateDisconnect();
    
    /**
     * @brief 模拟连接建立
     */
    void simulateConnect();
    
    /**
     * @brief 获取发送的帧列表
     * @return 已发送的帧
     */
    QVector<Frame> getSentFrames() const { return sentFrames_; }
    
    /**
     * @brief 清除发送历史
     */
    void clearSentHistory();
    
    /**
     * @brief 设置模拟延迟（毫秒）
     * @param delayMs 延迟时间
     */
    void setSimulatedDelay(int delayMs) { simulatedDelayMs_ = delayMs; }
    
    /**
     * @brief 设置是否模拟错误
     * @param shouldFail 是否失败
     */
    void setShouldFail(bool shouldFail) { shouldFail_ = shouldFail; }
    
private:
    UartConfig config_;
    bool connected_ = false;
    QVector<Frame> sentFrames_;
    QQueue<QByteArray> receiveQueue_;
    int simulatedDelayMs_ = 0;
    bool shouldFail_ = false;
    
    void processReceiveQueue();
};

} // namespace HeteroLink
