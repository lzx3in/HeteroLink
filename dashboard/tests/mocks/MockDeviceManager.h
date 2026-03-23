/**
 * HeteroLink Host - Mock 设备管理器
 * 
 * @file MockDeviceManager.h
 * @brief 用于单元测试的虚拟设备管理器，使用 Mock 通道
 */

#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QVector>

#include "core/DeviceManager.h"
#include "mocks/MockUartChannel.h"
#include "mocks/MockMqttChannel.h"

namespace HeteroLink {

/**
 * @brief 模拟设备管理器
 * 
 * 使用 MockUartChannel 和 MockMqttChannel 进行测试
 */
class MockDeviceManager : public DeviceManager
{
    Q_OBJECT
    
public:
    explicit MockDeviceManager(QObject *parent = nullptr);
    ~MockDeviceManager();
    
    /**
     * @brief 获取 Mock UART 通道
     * @param deviceId 设备 ID
     * @return Mock 通道指针
     */
    MockUartChannel* getMockUartChannel(const QString& deviceId);
    
    /**
     * @brief 获取 Mock MQTT 通道
     * @return Mock 通道指针
     */
    MockMqttChannel* getMockMqttChannel();
    
    /**
     * @brief 添加模拟设备
     * @param deviceId 设备 ID
     * @param connectionType 连接类型 ("UART" 或 "MQTT")
     * @return 是否成功
     */
    bool addMockDevice(const QString& deviceId, const QString& connectionType);
    
    /**
     * @brief 模拟设备遥测数据
     * @param deviceId 设备 ID
     * @param telemetry 遥测数据
     */
    void simulateTelemetry(const QString& deviceId, const TelemetryData& telemetry);
    
    /**
     * @brief 模拟设备状态变化
     * @param deviceId 设备 ID
     * @param connected 是否已连接
     * @param online 是否在线
     */
    void simulateDeviceStatus(const QString& deviceId, bool connected, bool online);
    
    /**
     * @brief 模拟设备断开
     * @param deviceId 设备 ID
     */
    void simulateDeviceDisconnect(const QString& deviceId);
    
    /**
     * @brief 模拟设备连接
     * @param deviceId 设备 ID
     */
    void simulateDeviceConnect(const QString& deviceId);
    
    /**
     * @brief 获取发送的心跳列表
     * @param deviceId 设备 ID
     * @return 心跳帧列表
     */
    QVector<Frame> getSentHeartbeats(const QString& deviceId) const;
    
    /**
     * @brief 获取发送的控制命令列表
     * @param deviceId 设备 ID
     * @return 命令帧列表
     */
    QVector<Frame> getSentCommands(const QString& deviceId) const;
    
    /**
     * @brief 获取发布的 MQTT 消息列表
     * @return MQTT 消息列表
     */
    QVector<MockMqttMessage> getPublishedMqttMessages() const;
    
    /**
     * @brief 清除所有模拟设备
     */
    void clearMockDevices();
    
    /**
     * @brief 更新设备状态（公共版本，用于测试）
     * @param deviceId 设备 ID
     * @param connected 是否已连接
     * @param online 是否在线
     * @note 基类的 updateDeviceStatus 是私有的，此方法提供测试访问
     */
    void updateDeviceStatusPublic(const QString& deviceId, bool connected, bool online);
    
private:
    QMap<QString, MockUartChannel*> mockUartChannels_;
    MockMqttChannel* mockMqttChannel_ = nullptr;
    
    void setupMockChannels();
};

} // namespace HeteroLink
