/**
 * HeteroLink Host - Mock MQTT 通道实现
 */

#include "MockMqttChannel.h"
#include <QTimer>
#include <QRegularExpression>

namespace HeteroLink {

MockMqttChannel::MockMqttChannel(QObject *parent)
    : MqttChannel(parent)
{
}

MockMqttChannel::~MockMqttChannel()
{
    disconnect();
}

bool MockMqttChannel::connect(const MqttConfig& config)
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

void MockMqttChannel::disconnect()
{
    connected_ = false;
    subscriptions_.clear();
    emit connectionChanged(false);
}

bool MockMqttChannel::isConnected() const
{
    return connected_;
}

bool MockMqttChannel::subscribe(const QString& topic)
{
    if (!connected_) {
        return false;
    }
    
    if (shouldFail_) {
        return false;
    }
    
    MockSubscription sub;
    sub.topic = topic;
    sub.active = true;
    subscriptions_.append(sub);
    
    return true;
}

void MockMqttChannel::unsubscribe(const QString& topic)
{
    for (auto& sub : subscriptions_) {
        if (sub.topic == topic) {
            sub.active = false;
        }
    }
}

bool MockMqttChannel::publish(const QString& topic, const QByteArray& payload,
                              int qos, bool retain)
{
    if (!connected_) {
        return false;
    }
    
    if (shouldFail_) {
        emit errorOccurred("Mock publish failed");
        return false;
    }
    
    MockMqttMessage msg;
    msg.topic = topic;
    msg.payload = payload;
    msg.qos = qos;
    msg.retain = retain;
    msg.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    publishedMessages_.append(msg);
    
    // 检查是否有订阅匹配此 topic，如果有则模拟接收
    processSubscription(topic, payload);
    
    return true;
}

void MockMqttChannel::publishDeviceStatus(const QString& deviceId, bool online)
{
    QString topic = QString("heterolink/subboard/%1/status").arg(deviceId);
    QByteArray payload = online ? "online" : "offline";
    publish(topic, payload, 1, true);
}

void MockMqttChannel::publishTelemetry(const QString& deviceId, const QString& data)
{
    QString topic = QString("heterolink/subboard/%1/telemetry").arg(deviceId);
    publish(topic, data.toUtf8(), 1, false);
}

void MockMqttChannel::publishCommand(const QString& deviceId, const QString& command)
{
    QString topic = QString("heterolink/subboard/%1/command").arg(deviceId);
    publish(topic, command.toUtf8(), 1, false);
}

void MockMqttChannel::subscribeDeviceCommands(const QString& deviceId)
{
    QString topic = QString("heterolink/subboard/%1/command").arg(deviceId);
    subscribe(topic);
}

void MockMqttChannel::subscribeAllDeviceStatus()
{
    subscribe("heterolink/subboard/+/status");
}

void MockMqttChannel::subscribeAllDeviceTelemetry()
{
    subscribe("heterolink/subboard/+/telemetry");
}

void MockMqttChannel::simulateMessage(const QString& topic, const QByteArray& payload)
{
    if (!connected_) {
        return;
    }
    
    emit messageReceived(topic, payload);
    processSubscription(topic, payload);
}

void MockMqttChannel::simulateDeviceStatus(const QString& deviceId, bool online)
{
    if (!connected_) {
        return;
    }
    
    QString topic = QString("heterolink/subboard/%1/status").arg(deviceId);
    simulateMessage(topic, online ? "online" : "offline");
    emit deviceStatusReceived(deviceId, online);
}

void MockMqttChannel::simulateTelemetry(const QString& deviceId, const QString& data)
{
    if (!connected_) {
        return;
    }
    
    QString topic = QString("heterolink/subboard/%1/telemetry").arg(deviceId);
    simulateMessage(topic, data.toUtf8());
    emit telemetryReceived(deviceId, data);
}

void MockMqttChannel::simulateDeviceCommand(const QString& deviceId, const QString& command)
{
    if (!connected_) {
        return;
    }
    
    QString topic = QString("heterolink/subboard/%1/command").arg(deviceId);
    simulateMessage(topic, command.toUtf8());
    emit deviceCommandReceived(deviceId, command);
}

void MockMqttChannel::simulateDisconnect()
{
    connected_ = false;
    emit connectionChanged(false);
    emit errorOccurred("Mock broker disconnected");
}

void MockMqttChannel::simulateConnect()
{
    connected_ = true;
    emit connectionChanged(true);
}

QVector<MockMqttMessage> MockMqttChannel::getPublishedMessages() const
{
    return publishedMessages_;
}

QVector<MockSubscription> MockMqttChannel::getSubscriptions() const
{
    return subscriptions_;
}

void MockMqttChannel::clearPublishHistory()
{
    publishedMessages_.clear();
}

void MockMqttChannel::processSubscription(const QString& topic, const QByteArray& payload)
{
    // 简单的 topic 匹配（支持 + 通配符）
    for (const auto& sub : subscriptions_) {
        if (!sub.active) continue;
        
        // 将 MQTT topic 转换为正则表达式
        QString pattern = sub.topic;
        pattern.replace("+", "[^/]+");  // + 匹配单个层级
        pattern.replace("#", ".*");     // # 匹配剩余所有层级
        pattern = "^" + pattern + "$";
        
        QRegularExpression regex(pattern);
        if (regex.match(topic).hasMatch()) {
            // 模拟延迟后发射信号
            if (simulatedDelayMs_ > 0) {
                QTimer::singleShot(simulatedDelayMs_, this, [this, topic, payload]() {
                    emit messageReceived(topic, payload);
                });
            } else {
                emit messageReceived(topic, payload);
            }
        }
    }
}

} // namespace HeteroLink
