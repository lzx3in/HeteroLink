/**
 * HeteroLink Host - 设备管理器
 * 
 * @file DeviceManager.h
 * @brief 设备发现、连接、状态管理
 */

#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QTimer>
#include <memory>

#include "protocol/UartChannel.h"
#include "protocol/MqttChannel.h"

namespace HeteroLink {

/**
 * @brief 设备信息
 */
struct DeviceInfo {
    QString id;
    QString name;
    bool connected = false;
    bool online = false;  // MQTT 在线状态
    QString connectionType;  // "UART" 或 "MQTT"
    QString port;  // 串口号（UART 模式）
    int baudRate = 921600;  // 波特率（UART 模式）
    quint64 lastSeen = 0;  // 最后通信时间戳
    QVariantMap metadata;  // 设备元数据
    
    DeviceInfo() = default;
};

/**
 * @brief 设备管理器类
 * 
 * 管理多个设备的连接、状态同步、数据分发
 */
class DeviceManager : public QObject
{
    Q_OBJECT
    
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();
    
    /**
     * @brief 获取所有设备
     * @return 设备列表
     */
    QMap<QString, DeviceInfo> getDevices() const;
    
    /**
     * @brief 获取指定设备
     * @param deviceId 设备 ID
     * @return 设备信息
     */
    DeviceInfo getDevice(const QString& deviceId) const;
    
    /**
     * @brief 添加设备
     * @param deviceInfo 设备信息
     * @return 是否成功
     */
    bool addDevice(const DeviceInfo& deviceInfo);
    
    /**
     * @brief 更新设备信息
     * @param deviceInfo 设备信息
     * @return 是否成功
     */
    bool updateDevice(const DeviceInfo& deviceInfo);
    
    /**
     * @brief 移除设备
     * @param deviceId 设备 ID
     * @return 是否成功
     */
    bool removeDevice(const QString& deviceId);
    
    /**
     * @brief 连接设备（UART 模式）
     * @param deviceId 设备 ID
     * @param config UART 配置
     * @return 是否成功
     */
    bool connectDevice(const QString& deviceId, const UartConfig& config);
    
    /**
     * @brief 断开设备
     * @param deviceId 设备 ID
     */
    void disconnectDevice(const QString& deviceId);
    
    /**
     * @brief 获取可用串口列表
     * @return 串口信息
     */
    QVector<QSerialPortInfo> getAvailablePorts() const;
    
    /**
     * @brief 发送心跳
     * @param deviceId 设备 ID
     */
    void sendHeartbeat(const QString& deviceId);
    
    /**
     * @brief 发送控制命令
     * @param deviceId 设备 ID
     * @param cmdType 命令类型
     * @param payload 命令参数
     */
    void sendControlCommand(const QString& deviceId, uint8_t cmdType,
                           const std::vector<uint8_t>& payload);
    
    /**
     * @brief 设置 MQTT 通道
     * @param mqttChannel MQTT 通道
     */
    void setMqttChannel(std::shared_ptr<MqttChannel> mqttChannel);
    
    /**
     * @brief 连接设备（MQTT 模式）
     * @param deviceId 设备 ID
     * @param brokerHost Broker 地址
     * @param brokerPort Broker 端口
     * @return 是否成功
     */
    bool connectDeviceMqtt(const QString& deviceId, const QString& brokerHost, quint16 brokerPort);
    
    /**
     * @brief 断开设备（MQTT 模式）
     * @param deviceId 设备 ID
     */
    void disconnectDeviceMqtt(const QString& deviceId);
    
    /**
     * @brief 发送控制命令（MQTT 模式）
     * @param deviceId 设备 ID
     * @param command 命令内容（JSON 格式）
     */
    void sendControlCommandMqtt(const QString& deviceId, const QString& command);
    
signals:
    /**
     * @brief 设备列表变化
     * @param devices 设备列表
     */
    void devicesChanged(const QMap<QString, DeviceInfo>& devices);
    
    /**
     * @brief 设备状态变化
     * @param deviceId 设备 ID
     * @param connected 是否已连接
     * @param online 是否在线
     */
    void deviceStatusChanged(const QString& deviceId, bool connected, bool online);
    
    /**
     * @brief 收到遥测数据
     * @param deviceId 设备 ID
     * @param data 遥测数据
     */
    void telemetryReceived(const QString& deviceId, const TelemetryData& data);
    
    /**
     * @brief 设备错误
     * @param deviceId 设备 ID
     * @param error 错误描述
     */
    void deviceError(const QString& deviceId, const QString& error);
    
private slots:
    void onUartTelemetry(uint8_t deviceId, const TelemetryData& data);
    void onUartError(uint8_t deviceId, ErrorCode error);
    void onUartConnectionChanged(bool connected);
    void onMqttMessage(const QString& topic, const QByteArray& payload);
    
private:
    QMap<QString, DeviceInfo> devices_;
    QMap<QString, std::shared_ptr<UartChannel>> uartChannels_;
    std::shared_ptr<MqttChannel> mqttChannel_;
    QTimer heartbeatTimer_;
    
    void parseMqttMessage(const QString& topic, const QByteArray& payload);
    void updateDeviceStatus(const QString& deviceId, bool connected, bool online);
};

} // namespace HeteroLink
