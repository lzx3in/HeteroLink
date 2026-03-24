/**
 * HeteroLink Host - MQTT 通信通道
 * 
 * @file MqttChannel.h
 * @brief MQTT 通信管理
 */

#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <memory>

#ifdef HAS_QT_MQTT
#include <QMqttClient>
#include <QMqttSubscription>
#endif

namespace HeteroLink {

/**
 * @brief MQTT 通道配置
 */
struct MqttConfig {
    QString brokerHost;
    quint16 brokerPort = 1883;
    QString username;
    QString password;
    QString clientId;
    bool useTls = false;
    QString willTopic;
    QString willMessage;
    
    MqttConfig() : brokerHost("localhost"), brokerPort(1883) {}
};

/**
 * @brief MQTT 通信通道类
 * 
 * 负责 MQTT 连接、订阅、发布
 */
class MqttChannel : public QObject
{
    Q_OBJECT
    
public:
    explicit MqttChannel(QObject *parent = nullptr);
    virtual ~MqttChannel();
    
    /**
     * @brief 连接到 Broker
     * @param config 配置
     * @return 是否成功
     */
    virtual bool connect(const MqttConfig& config);
    
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
     * @brief 获取当前配置
     * @return MQTT 配置
     */
    MqttConfig getConfig() const { return config_; }
    
    /**
     * @brief 订阅 Topic
     * @param topic Topic 名称
     * @return 是否成功
     */
    virtual bool subscribe(const QString& topic);
    
    /**
     * @brief 取消订阅
     * @param topic Topic 名称
     */
    virtual void unsubscribe(const QString& topic);
    
    /**
     * @brief 发布消息
     * @param topic Topic 名称
     * @param payload 消息内容
     * @param qos QoS 级别
     * @param retain 是否保留
     * @return 是否成功
     */
    virtual bool publish(const QString& topic, const QByteArray& payload,
                int qos = 1, bool retain = false);
    
    /**
     * @brief 发布设备状态
     * @param deviceId 设备 ID
     * @param online 是否在线
     */
    virtual void publishDeviceStatus(const QString& deviceId, bool online);
    
    /**
     * @brief 发布遥测数据
     * @param deviceId 设备 ID
     * @param data JSON 格式数据
     */
    virtual void publishTelemetry(const QString& deviceId, const QString& data);
    
    /**
     * @brief 发布命令到设备
     * @param deviceId 设备 ID
     * @param command 命令内容
     */
    virtual void publishCommand(const QString& deviceId, const QString& command);
    
    /**
     * @brief 订阅设备命令
     * @param deviceId 设备 ID
     */
    virtual void subscribeDeviceCommands(const QString& deviceId);
    
    /**
     * @brief 订阅所有设备状态
     */
    virtual void subscribeAllDeviceStatus();
    
    /**
     * @brief 订阅所有设备遥测
     */
    virtual void subscribeAllDeviceTelemetry();
    
signals:
    /**
     * @brief 连接状态变化
     * @param connected 是否已连接
     */
    void connectionChanged(bool connected);
    
    /**
     * @brief 收到消息
     * @param topic Topic
     * @param payload 消息内容
     */
    void messageReceived(const QString& topic, const QByteArray& payload);
    
    /**
     * @brief 发生错误
     * @param error 错误描述
     */
    void errorOccurred(const QString& error);
    
    /**
     * @brief 收到设备状态
     * @param deviceId 设备 ID
     * @param online 是否在线
     */
    void deviceStatusReceived(const QString& deviceId, bool online);
    
    /**
     * @brief 收到设备命令
     * @param deviceId 设备 ID
     * @param command 命令内容
     */
    void deviceCommandReceived(const QString& deviceId, const QString& command);
    
    /**
     * @brief 收到遥测数据
     * @param deviceId 设备 ID
     * @param data JSON 数据
     */
    void telemetryReceived(const QString& deviceId, const QString& data);
    
private:
#ifdef HAS_QT_MQTT
    std::unique_ptr<QMqttClient> client_;
    QMap<QString, QMqttSubscription*> subscriptions_;
#endif
    MqttConfig config_;
    bool connected_;
    int reconnectAttempts_;
    static const int MAX_RECONNECT_ATTEMPTS = 5;
    
#ifdef HAS_QT_MQTT
    void setupClient();
    void attemptReconnect();
    QString parseDeviceIdFromTopic(const QString& topic);
#endif
};

} // namespace HeteroLink
