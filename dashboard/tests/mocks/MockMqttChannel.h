/**
 * HeteroLink Host - Mock MQTT 通道
 * 
 * @file MockMqttChannel.h
 * @brief 用于单元测试的虚拟 MQTT 通道，无需真实 Broker
 */

#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonDocument>

#include "protocol/MqttChannel.h"

namespace HeteroLink {

/**
 * @brief 模拟 MQTT 订阅
 */
struct MockSubscription {
    QString topic;
    int qos = 1;
    bool active = true;
};

/**
 * @brief 模拟 MQTT 消息
 */
struct MockMqttMessage {
    QString topic;
    QByteArray payload;
    int qos = 1;
    bool retain = false;
    quint64 timestamp = 0;
};

/**
 * @brief 模拟 MQTT 通道
 * 
 * 完全在内存中模拟 MQTT 通信行为，用于单元测试
 */
class MockMqttChannel : public MqttChannel
{
    Q_OBJECT
    
public:
    explicit MockMqttChannel(QObject *parent = nullptr);
    ~MockMqttChannel();
    
    /**
     * @brief 连接到 Broker（模拟）
     * @param config 配置
     * @return 总是返回 true（除非设置 shouldFail_）
     */
    bool connect(const MqttConfig& config) override;
    
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
     * @return MQTT 配置
     */
    MqttConfig getConfig() const { return config_; }
    
    /**
     * @brief 订阅 Topic（模拟）
     * @param topic Topic 名称
     * @return 总是返回 true
     */
    bool subscribe(const QString& topic) override;
    
    /**
     * @brief 取消订阅（模拟）
     * @param topic Topic 名称
     */
    void unsubscribe(const QString& topic) override;
    
    /**
     * @brief 发布消息（模拟）
     * @param topic Topic 名称
     * @param payload 消息内容
     * @param qos QoS 级别
     * @param retain 是否保留
     * @return 总是返回 true
     */
    bool publish(const QString& topic, const QByteArray& payload,
                int qos = 1, bool retain = false) override;
    
    /**
     * @brief 发布设备状态（模拟）
     * @param deviceId 设备 ID
     * @param online 是否在线
     */
    void publishDeviceStatus(const QString& deviceId, bool online) override;
    
    /**
     * @brief 发布遥测数据（模拟）
     * @param deviceId 设备 ID
     * @param data JSON 格式数据
     */
    void publishTelemetry(const QString& deviceId, const QString& data) override;
    
    /**
     * @brief 发布命令到设备（模拟）
     * @param deviceId 设备 ID
     * @param command 命令内容
     */
    void publishCommand(const QString& deviceId, const QString& command) override;
    
    /**
     * @brief 订阅设备命令（模拟）
     * @param deviceId 设备 ID
     */
    void subscribeDeviceCommands(const QString& deviceId) override;
    
    /**
     * @brief 订阅所有设备状态（模拟）
     */
    void subscribeAllDeviceStatus() override;
    
    /**
     * @brief 订阅所有设备遥测（模拟）
     */
    void subscribeAllDeviceTelemetry() override;
    
    /**
     * @brief 模拟接收消息
     * @param topic Topic
     * @param payload 消息内容
     */
    void simulateMessage(const QString& topic, const QByteArray& payload);
    
    /**
     * @brief 模拟接收设备状态
     * @param deviceId 设备 ID
     * @param online 是否在线
     */
    void simulateDeviceStatus(const QString& deviceId, bool online);
    
    /**
     * @brief 模拟接收遥测数据
     * @param deviceId 设备 ID
     * @param data JSON 数据
     */
    void simulateTelemetry(const QString& deviceId, const QString& data);
    
    /**
     * @brief 模拟接收设备命令
     * @param deviceId 设备 ID
     * @param command 命令内容
     */
    void simulateDeviceCommand(const QString& deviceId, const QString& command);
    
    /**
     * @brief 模拟连接断开
     */
    void simulateDisconnect();
    
    /**
     * @brief 模拟连接建立
     */
    void simulateConnect();
    
    /**
     * @brief 获取已发布的消息列表
     * @return 已发布的消息
     */
    QVector<MockMqttMessage> getPublishedMessages() const { return publishedMessages_; }
    
    /**
     * @brief 获取已订阅的 Topic 列表
     * @return 订阅列表
     */
    QVector<MockSubscription> getSubscriptions() const { return subscriptions_; }
    
    /**
     * @brief 清除发布历史
     */
    void clearPublishHistory();
    
    /**
     * @brief 设置是否模拟错误
     * @param shouldFail 是否失败
     */
    void setShouldFail(bool shouldFail) { shouldFail_ = shouldFail; }
    
    /**
     * @brief 设置模拟延迟（毫秒）
     * @param delayMs 延迟时间
     */
    void setSimulatedDelay(int delayMs) { simulatedDelayMs_ = delayMs; }
    
private:
    MqttConfig config_;
    bool connected_ = false;
    QVector<MockSubscription> subscriptions_;
    QVector<MockMqttMessage> publishedMessages_;
    int simulatedDelayMs_ = 0;
    bool shouldFail_ = false;
    
    void processSubscription(const QString& topic, const QByteArray& payload);
};

} // namespace HeteroLink
