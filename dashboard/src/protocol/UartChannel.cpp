/**
 * HeteroLink Host - UART 通信通道实现
 */

#include "protocol/UartChannel.h"
#include "protocol/Protocol.h"
#include "utils/Logger.h"

namespace HeteroLink {

UartChannel::UartChannel(QObject *parent)
    : QObject(parent)
    , deviceId_(0)
    , connected_(false)
{
}

UartChannel::~UartChannel()
{
    disconnect();
}

QVector<QSerialPortInfo> UartChannel::availablePorts()
{
    QVector<QSerialPortInfo> ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : infos) {
        ports.append(info);
    }
    return ports;
}

bool UartChannel::connect(const UartConfig& config)
{
    if (connected_) {
        LOG_WARNING("Already connected");
        return false;
    }
    
    config_ = config;
    serialPort_ = std::make_unique<QSerialPort>();
    
    // 配置串口
    serialPort_->setPortName(config.portName);
    serialPort_->setBaudRate(config.baudRate);
    serialPort_->setDataBits(config.dataBits);
    serialPort_->setParity(config.parity);
    serialPort_->setStopBits(config.stopBits);
    serialPort_->setFlowControl(config.flowControl);
    
    // 连接信号
    QObject::connect(serialPort_.get(), &QSerialPort::readyRead,
            this, &UartChannel::onReadyRead);
    QObject::connect(serialPort_.get(), &QSerialPort::errorOccurred,
            this, &UartChannel::onErrorOccurred);
    
    // 打开串口
    if (!serialPort_->open(QIODevice::ReadWrite)) {
        LOG_ERROR("Failed to open serial port: " + config.portName);
        emit errorOccurred("Failed to open serial port: " + serialPort_->errorString());
        serialPort_.reset();
        return false;
    }
    
    connected_ = true;
    readBuffer_.clear();
    LOG_INFO("UART connected: " + config.portName + 
             " @ " + QString::number(config.baudRate));
    emit connectionChanged(true);
    
    return true;
}

void UartChannel::disconnect()
{
    if (!connected_) {
        return;
    }
    
    if (serialPort_) {
        serialPort_->close();
        serialPort_.reset();
    }
    
    connected_ = false;
    readBuffer_.clear();
    LOG_INFO("UART disconnected");
    emit connectionChanged(false);
}

bool UartChannel::isConnected() const
{
    return connected_;
}

bool UartChannel::sendFrame(const Frame& frame)
{
    if (!connected_ || !serialPort_) {
        return false;
    }
    
    std::vector<uint8_t> data = Protocol::encode(frame);
    qint64 written = serialPort_->write(reinterpret_cast<const char*>(data.data()), data.size());
    
    if (written != static_cast<qint64>(data.size())) {
        LOG_ERROR("Failed to write complete frame");
        return false;
    }
    
    serialPort_->flush();
    return true;
}

void UartChannel::sendHeartbeat(uint8_t deviceId)
{
    Frame frame = Protocol::createHeartbeat(deviceId);
    sendFrame(frame);
}

void UartChannel::sendControlCommand(uint8_t deviceId, uint8_t cmdType,
                                     const std::vector<uint8_t>& payload)
{
    Frame frame = Protocol::createControlCommand(deviceId, cmdType, payload);
    sendFrame(frame);
}

void UartChannel::setDeviceId(uint8_t deviceId)
{
    deviceId_ = deviceId;
}

void UartChannel::onReadyRead()
{
    if (!serialPort_) {
        return;
    }
    
    // 读取可用数据
    QByteArray data = serialPort_->readAll();
    readBuffer_.append(data);
    
    // 尝试解析帧
    while (readBuffer_.size() >= 8) {  // 最小帧长度
        Frame frame;
        int consumed = 0;
        
        if (parseFrame(readBuffer_, frame, consumed)) {
            // 成功解析一帧
            readBuffer_.remove(0, consumed);
            
            // 处理帧
            if (frame.command == static_cast<uint8_t>(Command::CMD_TELEMETRY)) {
                TelemetryData telemetry = Protocol::parseTelemetry(frame.payload);
                emit telemetryReceived(frame.deviceId, telemetry);
            } else if (frame.command == static_cast<uint8_t>(Command::CMD_ERROR)) {
                if (!frame.payload.empty()) {
                    ErrorCode error = static_cast<ErrorCode>(frame.payload[0]);
                    emit errorReceived(frame.deviceId, error);
                }
            }
        } else {
            // 解析失败，跳过帧头
            if (consumed > 0) {
                readBuffer_.remove(0, consumed);
            } else {
                // 找不到帧头，清空缓冲区
                readBuffer_.clear();
            }
            break;
        }
    }
}

void UartChannel::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }
    
    QString errorMsg = "UART error: " + serialPort_->errorString();
    LOG_ERROR(errorMsg);
    emit errorOccurred(errorMsg);
    
    if (error == QSerialPort::ResourceError || error == QSerialPort::OpenError) {
        disconnect();
    }
}

bool UartChannel::findFrameStart(const QByteArray& buffer, int& pos)
{
    // 查找帧头 0x5AA5 (小端)
    for (int i = 0; i < buffer.size() - 1; ++i) {
        if (static_cast<uint8_t>(buffer[i]) == 0xA5 && 
            static_cast<uint8_t>(buffer[i + 1]) == 0x5A) {
            pos = i;
            return true;
        }
    }
    pos = -1;
    return false;
}

bool UartChannel::parseFrame(const QByteArray& buffer, Frame& frame, int& consumed)
{
    // 查找帧头
    int startPos = 0;
    if (!findFrameStart(buffer, startPos)) {
        consumed = buffer.size();  // 跳过整个缓冲区
        return false;
    }
    
    // 跳过帧头前的无效数据
    if (startPos > 0) {
        consumed = startPos;
        return false;
    }
    
    // 检查最小长度
    if (buffer.size() < 8) {
        consumed = 0;
        return false;
    }
    
    // 解析帧头
    uint16_t header = static_cast<uint8_t>(buffer[0]) | 
                     (static_cast<uint8_t>(buffer[1]) << 8);
    if (header != FRAME_HEADER) {
        consumed = 1;  // 跳过第一个字节
        return false;
    }
    
    // 解析设备 ID 和命令
    frame.deviceId = static_cast<uint8_t>(buffer[2]);
    frame.command = static_cast<uint8_t>(buffer[3]);
    
    // 解析长度
    uint16_t len = static_cast<uint8_t>(buffer[4]) | 
                  (static_cast<uint8_t>(buffer[5]) << 8);
    frame.length = len;
    
    // 检查缓冲区是否有足够数据
    int frameSize = 6 + len + 2;  // header(2) + deviceId(1) + command(1) + length(2) + payload(N) + crc(2)
    if (buffer.size() < frameSize) {
        consumed = 0;
        return false;
    }
    
    // 解析 payload
    frame.payload.clear();
    for (uint16_t i = 0; i < len; ++i) {
        frame.payload.push_back(static_cast<uint8_t>(buffer[6 + i]));
    }
    
    // 解析并验证 CRC
    uint16_t receivedCrc = static_cast<uint8_t>(buffer[6 + len]) | 
                          (static_cast<uint8_t>(buffer[6 + len + 1]) << 8);
    frame.crc = receivedCrc;
    
    if (!Protocol::verifyCrc(frame)) {
        LOG_WARNING("CRC check failed");
        consumed = 1;  // 跳过第一个字节，重新同步
        return false;
    }
    
    frame.header = header;
    consumed = frameSize;
    return true;
}

} // namespace HeteroLink
