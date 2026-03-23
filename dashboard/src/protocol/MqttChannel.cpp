/**
 * HeteroLink Host - MQTT 通信通道实现
 */

#include "protocol/MqttChannel.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

namespace HeteroLink {

MqttChannel::MqttChannel(QObject *parent)
    : QObject(parent)
    , connected_(false)
    , reconnectAttempts_(0)
{
#ifdef HAS_QT_MQTT
    setupClient();
#endif
}

MqttChannel::~MqttChannel()
{
    disconnect();
}

#ifdef HAS_QT_MQTT
void MqttChannel::setupClient()
{
    client_ = std::make_unique<QMqttClient>(this);
    
    // 配置客户端
    client_->setHostname(config_.brokerHost);
    client_->setPort(config_.brokerPort);
    
    // 使用 Qt6 connect 语法
    QObject::connect(client_.get(), &QMqttClient::connected, [this]() {
        connected_ = true;
        reconnectAttempts_ = 0;  // 重置重连计数
        LOG_INFO("MQTT connected to " + config_.brokerHost.toStdString() + ":" + 
                 std::to_string(config_.brokerPort));
        emit connectionChanged(true);
        
        // 连接成功后重新订阅 - 通过 client 重新订阅
        for (auto it = subscriptions_.begin(); it != subscriptions_.end(); ++it) {
            const QString& topic = it.key();
            client_->subscribe(topic);
        }
    });
    
    QObject::connect(client_.get(), &QMqttClient::disconnected, [this]() {
        connected_ = false;
        LOG_INFO("MQTT disconnected");
        emit connectionChanged(false);
        
        // 尝试自动重连
        attemptReconnect();
    });
    
    // QtMqtt 使用 errorChanged 信号
    QObject::connect(client_.get(), &QMqttClient::errorChanged, [this](QMqttClient::ClientError error) {
        QString errorMsg;
        switch (error) {
            case QMqttClient::ClientError::InvalidProtocolVersion:
                errorMsg = "MQTT error: Invalid protocol version";
                break;
            case QMqttClient::ClientError::IdRejected:
                errorMsg = "MQTT error: Client ID rejected";
                break;
            case QMqttClient::ClientError::ServerUnavailable:
                errorMsg = "MQTT error: Server unavailable";
                break;
            case QMqttClient::ClientError::BadUsernameOrPassword:
                errorMsg = "MQTT error: Bad username or password";
                break;
            case QMqttClient::ClientError::NotAuthorized:
                errorMsg = "MQTT error: Not authorized";
                break;
            default:
                errorMsg = "MQTT error: " + QString::number(error);
                break;
        }
        LOG_ERROR(errorMsg.toStdString());
        emit errorOccurred(errorMsg);
    });
    
    // messageReceived 通过 subscription 触发，这里监听所有消息
    QObject::connect(client_.get(), &QMqttClient::messageReceived, [this](const QByteArray& message, const QMqttTopicName& topic) {
        QString topicName = topic.name();
        LOG_DEBUG("MQTT message received on topic: " + topicName.toStdString());
        
        // 解析消息类型
        if (topicName.contains("/status")) {
            QString deviceId = parseDeviceIdFromTopic(topicName);
            bool online = (message == "online");
            emit deviceStatusReceived(deviceId, online);
        } else if (topicName.contains("/command")) {
            QString deviceId = parseDeviceIdFromTopic(topicName);
            emit deviceCommandReceived(deviceId, QString::fromUtf8(message));
        } else if (topicName.contains("/telemetry")) {
            QString deviceId = parseDeviceIdFromTopic(topicName);
            emit telemetryReceived(deviceId, QString::fromUtf8(message));
        }
        
        emit messageReceived(topicName, message);
    });
}

QString MqttChannel::parseDeviceIdFromTopic(const QString& topic)
{
    // 解析格式：heterolink/subboard/{deviceId}/status|command|telemetry
    QStringList parts = topic.split('/');
    if (parts.size() >= 3) {
        return parts[2];
    }
    return QString();
}

void MqttChannel::attemptReconnect()
{
    if (reconnectAttempts_ >= MAX_RECONNECT_ATTEMPTS) {
        LOG_ERROR("Max reconnection attempts reached");
        QString errorMsg = "MQTT: Max reconnection attempts (" + 
                          QString::number(MAX_RECONNECT_ATTEMPTS) + ") reached";
        emit errorOccurred(errorMsg);
        return;
    }
    
    reconnectAttempts_++;
    int delayMs = qMin(1000 * (1 << reconnectAttempts_), 30000);  // 指数退避，最大 30 秒
    
    LOG_INFO("MQTT attempting reconnection " + QString::number(reconnectAttempts_) + 
             " in " + QString::number(delayMs) + "ms");
    
    QTimer::singleShot(delayMs, this, [this]() {
        if (config_.brokerHost.isEmpty()) {
            return;
        }
        client_->setHostname(config_.brokerHost);
        client_->setPort(config_.brokerPort);
        client_->connectToHost();
    });
}
#endif

bool MqttChannel::connect(const MqttConfig& config)
{
    config_ = config;
    reconnectAttempts_ = 0;
    
#ifdef HAS_QT_MQTT
    if (client_) {
        // 生成客户端 ID（如果未指定）
        QString clientId = config.clientId;
        if (clientId.isEmpty()) {
            clientId = "heterolink_host_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        }
        
        client_->setHostname(config.brokerHost);
        client_->setPort(config.brokerPort);
        client_->setClientId(clientId);
        
        if (!config.username.isEmpty()) {
            client_->setUsername(config.username);
            client_->setPassword(config.password);
        }
        
        // 设置 Last Will
        if (!config.willTopic.isEmpty()) {
            client_->setWillTopic(config.willTopic);
            client_->setWillMessage(config.willMessage.toUtf8());
            client_->setWillQoS(1);
            client_->setWillRetain(true);
        }
        
        // 设置 MQTT 版本为 5.0
        client_->setProtocolVersion(QMqttClient::MQTT_5_0);
        
        // TLS 配置
        if (config.useTls) {
            // TODO: 配置 TLS
            LOG_INFO("TLS enabled (configuration pending)");
        }
        
        LOG_INFO("MQTT connecting to " + config.brokerHost.toStdString() + ":" + 
                 std::to_string(config.brokerPort));
        client_->connectToHost();
        return true;
    }
#else
    LOG_INFO("MQTT support not compiled (HAS_QT_MQTT not defined)");
#endif
    
    // 模拟连接成功（用于编译测试）
    connected_ = true;
    emit connectionChanged(true);
    return true;
}

void MqttChannel::disconnect()
{
#ifdef HAS_QT_MQTT
    if (client_) {
        client_->disconnectFromHost();
    }
    subscriptions_.clear();
#endif
    
    connected_ = false;
    LOG_INFO("MQTT disconnected");
    emit connectionChanged(false);
}

bool MqttChannel::isConnected() const
{
    return connected_;
}

bool MqttChannel::subscribe(const QString& topic)
{
#ifdef HAS_QT_MQTT
    if (!client_ || !connected_) {
        return false;
    }
    
    auto subscription = client_->subscribe(topic, 1);
    if (subscription) {
        subscriptions_[topic] = subscription;
        LOG_INFO("MQTT subscribed: " + topic.toStdString());
        return true;
    }
#else
    Q_UNUSED(topic)
#endif
    
    return false;
}

void MqttChannel::unsubscribe(const QString& topic)
{
#ifdef HAS_QT_MQTT
    if (subscriptions_.contains(topic)) {
        auto sub = subscriptions_.take(topic);
        if (sub) {
            sub->unsubscribe();
        }
        LOG_INFO("MQTT unsubscribed: " + topic.toStdString());
    }
#else
    Q_UNUSED(topic)
#endif
}

bool MqttChannel::publish(const QString& topic, const QByteArray& payload,
                          int qos, bool retain)
{
#ifdef HAS_QT_MQTT
    if (!client_ || !connected_) {
        return false;
    }
    
    auto result = client_->publish(topic, payload, qos, retain);
    if (result == -1) {
        LOG_ERROR("MQTT publish failed: " + topic.toStdString());
        return false;
    }
    return true;
#else
    Q_UNUSED(topic)
    Q_UNUSED(payload)
    Q_UNUSED(qos)
    Q_UNUSED(retain)
    return false;
#endif
}

void MqttChannel::publishDeviceStatus(const QString& deviceId, bool online)
{
    QString topic = QString("heterolink/subboard/%1/status").arg(deviceId);
    QString message = online ? "online" : "offline";
    publish(topic, message.toUtf8(), 1, true);
}

void MqttChannel::publishTelemetry(const QString& deviceId, const QString& data)
{
    QString topic = QString("heterolink/subboard/%1/telemetry").arg(deviceId);
    publish(topic, data.toUtf8());
}

void MqttChannel::publishCommand(const QString& deviceId, const QString& command)
{
    QString topic = QString("heterolink/subboard/%1/command").arg(deviceId);
    publish(topic, command.toUtf8(), 1, false);
}

void MqttChannel::subscribeDeviceCommands(const QString& deviceId)
{
    QString topic = QString("heterolink/subboard/%1/command").arg(deviceId);
    subscribe(topic);
}

void MqttChannel::subscribeAllDeviceStatus()
{
    subscribe("heterolink/subboard/+/status");
}

void MqttChannel::subscribeAllDeviceTelemetry()
{
    subscribe("heterolink/subboard/+/telemetry");
}

} // namespace HeteroLink
