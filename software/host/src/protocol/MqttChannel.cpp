/**
 * HeteroLink Host - MQTT 通信通道实现
 */

#include "protocol/MqttChannel.h"
#include "utils/Logger.h"

namespace HeteroLink {

MqttChannel::MqttChannel(QObject *parent)
    : QObject(parent)
    , connected_(false)
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
    
    connect(client_.get(), &QMqttClient::connected, this, [this]() {
        connected_ = true;
        LOG_INFO("MQTT connected");
        emit connectionChanged(true);
    });
    
    connect(client_.get(), &QMqttClient::disconnected, this, [this]() {
        connected_ = false;
        LOG_INFO("MQTT disconnected");
        emit connectionChanged(false);
    });
    
    connect(client_.get(), &QMqttClient::errorOccurred, this, [this](QMqttClient::ClientError error) {
        QString errorMsg = "MQTT error: " + QString::number(error);
        LOG_ERROR(errorMsg.toStdString());
        emit errorOccurred(errorMsg);
    });
    
    connect(client_.get(), &QMqttClient::messageReceived, this, [this](const QByteArray& message, const QMqttTopicName& topic) {
        emit messageReceived(topic.name(), message);
    });
}
#endif

bool MqttChannel::connect(const MqttConfig& config)
{
    config_ = config;
    
#ifdef HAS_QT_MQTT
    if (client_) {
        client_->setHostname(config.brokerHost);
        client_->setPort(config.brokerPort);
        client_->setClientId(config.clientId);
        
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
        
        // TLS 配置
        if (config.useTls) {
            // TODO: 配置 TLS
            LOG_INFO("TLS enabled (configuration pending)");
        }
        
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
#endif
    
    connected_ = false;
    subscriptions_.clear();
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

void MqttChannel::subscribeDeviceCommands(const QString& deviceId)
{
    QString topic = QString("heterolink/subboard/%1/command").arg(deviceId);
    subscribe(topic);
}

} // namespace HeteroLink
