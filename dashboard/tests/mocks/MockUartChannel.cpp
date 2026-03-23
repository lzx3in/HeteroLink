/**
 * HeteroLink Host - Mock UART 通道实现
 */

#include "MockUartChannel.h"
#include <QTimer>

namespace HeteroLink {

MockUartChannel::MockUartChannel(QObject *parent)
    : UartChannel(parent)
{
}

MockUartChannel::~MockUartChannel()
{
    disconnect();
}

QVector<QSerialPortInfo> MockUartChannel::availablePorts()
{
    // 返回模拟的串口列表
    // 注意：QSerialPortInfo 是只读的，我们返回真实串口供测试使用
    // 测试应该验证 ports.size() >= 0 而不是特定名称
    return QSerialPortInfo::availablePorts();
}

bool MockUartChannel::connect(const UartConfig& config)
{
    config_ = config;
    
    if (shouldFail_) {
        emit errorOccurred("Mock connection failed");
        return false;
    }
    
    connected_ = true;
    emit connectionChanged(true);
    return true;
}

void MockUartChannel::disconnect()
{
    connected_ = false;
    emit connectionChanged(false);
}

bool MockUartChannel::isConnected() const
{
    return connected_;
}

bool MockUartChannel::sendFrame(const Frame& frame)
{
    if (!connected_) {
        return false;
    }
    
    if (shouldFail_) {
        emit errorOccurred("Mock send failed");
        return false;
    }
    
    sentFrames_.append(frame);
    
    if (simulatedDelayMs_ > 0) {
        QTimer::singleShot(simulatedDelayMs_, this, [this]() {
            // 模拟发送完成
        });
    }
    
    return true;
}

void MockUartChannel::simulateReceive(const QByteArray& data)
{
    if (!connected_) {
        return;
    }
    
    receiveQueue_.enqueue(data);
    processReceiveQueue();
}

void MockUartChannel::simulateTelemetry(uint8_t deviceId, const TelemetryData& telemetry)
{
    if (!connected_) {
        return;
    }
    
    // 直接发射信号，模拟接收到遥测数据
    emit telemetryReceived(deviceId, telemetry);
}

void MockUartChannel::simulateError(uint8_t deviceId, ErrorCode error)
{
    if (!connected_) {
        return;
    }
    
    emit errorReceived(deviceId, error);
}

void MockUartChannel::simulateDisconnect()
{
    connected_ = false;
    emit connectionChanged(false);
    emit errorOccurred("Mock device disconnected");
}

void MockUartChannel::simulateConnect()
{
    connected_ = true;
    emit connectionChanged(true);
}

void MockUartChannel::clearSentHistory()
{
    sentFrames_.clear();
}

void MockUartChannel::sendHeartbeat(uint8_t deviceId)
{
    if (!connected_) {
        return;
    }
    
    Frame frame;
    frame.deviceId = deviceId;
    frame.command = static_cast<uint8_t>(Command::CMD_HEARTBEAT);
    frame.payload.clear();
    
    sendFrame(frame);
}

void MockUartChannel::sendControlCommand(uint8_t deviceId, uint8_t cmdType,
                                         const std::vector<uint8_t>& payload)
{
    if (!connected_) {
        return;
    }
    
    Frame frame;
    frame.deviceId = deviceId;
    frame.command = static_cast<uint8_t>(Command::CMD_CONTROL);
    
    // payload 格式：cmdType + 参数
    frame.payload.push_back(cmdType);
    for (uint8_t byte : payload) {
        frame.payload.push_back(byte);
    }
    
    sendFrame(frame);
}

void MockUartChannel::processReceiveQueue()
{
    // 处理接收队列（如果需要解析帧）
    while (!receiveQueue_.isEmpty()) {
        QByteArray data = receiveQueue_.dequeue();
        // 可以在这里添加帧解析逻辑
        Q_UNUSED(data)
    }
}

} // namespace HeteroLink
