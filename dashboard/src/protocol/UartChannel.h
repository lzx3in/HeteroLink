/**
 * HeteroLink Host - UART 通信通道
 * 
 * @file UartChannel.h
 * @brief 串口通信管理
 */

#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVector>
#include <QTimer>
#include <memory>

#include "protocol/Protocol.h"

namespace HeteroLink {

/**
 * @brief UART 通道配置
 */
struct UartConfig {
    QString portName;
    qint32 baudRate = 921600;
    QSerialPort::DataBits dataBits = QSerialPort::Data8;
    QSerialPort::Parity parity = QSerialPort::NoParity;
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
    
    UartConfig() = default;
};

/**
 * @brief UART 通信通道类
 * 
 * 负责串口通信、数据收发、协议解析
 */
class UartChannel : public QObject
{
    Q_OBJECT
    
public:
    explicit UartChannel(QObject *parent = nullptr);
    virtual ~UartChannel();
    
    /**
     * @brief 获取可用串口列表
     * @return 串口信息列表
     */
    static QVector<QSerialPortInfo> availablePorts();
    
    /**
     * @brief 连接串口
     * @param config 配置
     * @return 是否成功
     */
    virtual bool connect(const UartConfig& config);
    
    /**
     * @brief 断开连接
     */
    virtual void disconnect();
    
    /**
     * @brief 检查连接状态
     * @return 是否已连接
     */
    virtual bool isConnected() const;
    
    /**
     * @brief 发送帧
     * @param frame 帧
     * @return 是否成功
     */
    virtual bool sendFrame(const Frame& frame);
    
    /**
     * @brief 发送心跳
     * @param deviceId 设备 ID
     */
    virtual void sendHeartbeat(uint8_t deviceId);
    
    /**
     * @brief 发送控制命令
     * @param deviceId 设备 ID
     * @param cmdType 命令类型
     * @param payload 命令参数
     */
    virtual void sendControlCommand(uint8_t deviceId, uint8_t cmdType,
                           const std::vector<uint8_t>& payload);
    
    /**
     * @brief 设置设备 ID
     * @param deviceId 设备 ID
     */
    virtual void setDeviceId(uint8_t deviceId);
    
signals:
    /**
     * @brief 连接状态变化
     * @param connected 是否已连接
     */
    void connectionChanged(bool connected);
    
    /**
     * @brief 收到遥测数据
     * @param deviceId 设备 ID
     * @param data 遥测数据
     */
    void telemetryReceived(uint8_t deviceId, const TelemetryData& data);
    
    /**
     * @brief 收到错误
     * @param deviceId 设备 ID
     * @param error 错误码
     */
    void errorReceived(uint8_t deviceId, ErrorCode error);
    
    /**
     * @brief 发生错误
     * @param error 错误描述
     */
    void errorOccurred(const QString& error);
    
private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    
private:
    std::unique_ptr<QSerialPort> serialPort_;
    UartConfig config_;
    uint8_t deviceId_;
    QByteArray readBuffer_;
    bool connected_;
    
    bool findFrameStart(const QByteArray& buffer, int& pos);
    bool parseFrame(const QByteArray& buffer, Frame& frame, int& consumed);
};

} // namespace HeteroLink
