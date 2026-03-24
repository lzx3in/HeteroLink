/**
 * HeteroLink Host - 设备管理器实现
 */

#include "core/DeviceManager.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QSerialPortInfo>

namespace HeteroLink {

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    connect(&heartbeatTimer_, &QTimer::timeout, this, [this]() {
        // 定期发送心跳
        for (auto it = devices_.begin(); it != devices_.end(); ++it) {
            if (it.value().connected && it.value().connectionType == "UART") {
                sendHeartbeat(it.key());
            }
        }
    });
    heartbeatTimer_.start(5000);  // 5 秒心跳
}

DeviceManager::~DeviceManager()
{
    // 断开所有设备
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        disconnectDevice(it.key());
    }
}

QMap<QString, DeviceInfo> DeviceManager::getDevices() const
{
    return devices_;
}

DeviceInfo DeviceManager::getDevice(const QString& deviceId) const
{
    return devices_.value(deviceId);
}

bool DeviceManager::addDevice(const DeviceInfo& deviceInfo)
{
    if (devices_.contains(deviceInfo.id)) {
        LOG_WARNING("Device already exists: " + deviceInfo.id.toStdString());
        return false;
    }
    
    devices_[deviceInfo.id] = deviceInfo;
    LOG_INFO("Device added: " + deviceInfo.id.toStdString());
    emit devicesChanged(devices_);
    return true;
}

bool DeviceManager::updateDevice(const DeviceInfo& deviceInfo)
{
    if (!devices_.contains(deviceInfo.id)) {
        LOG_WARNING("Device not found: " + deviceInfo.id.toStdString());
        return false;
    }
    
    devices_[deviceInfo.id] = deviceInfo;
    LOG_INFO("Device updated: " + deviceInfo.id.toStdString());
    emit devicesChanged(devices_);
    return true;
}

bool DeviceManager::removeDevice(const QString& deviceId)
{
    if (!devices_.contains(deviceId)) {
        return false;
    }
    
    disconnectDevice(deviceId);
    devices_.remove(deviceId);
    LOG_INFO("Device removed: " + deviceId.toStdString());
    emit devicesChanged(devices_);
    return true;
}

bool DeviceManager::connectDevice(const QString& deviceId, const UartConfig& config)
{
    if (!devices_.contains(deviceId)) {
        LOG_ERROR("Device not found: " + deviceId.toStdString());
        return false;
    }
    
    // 创建 UART 通道
    auto uartChannel = std::make_shared<UartChannel>();
    
    // 连接信号
    connect(uartChannel.get(), &UartChannel::telemetryReceived,
            this, &DeviceManager::onUartTelemetry);
    connect(uartChannel.get(), &UartChannel::errorReceived,
            this, &DeviceManager::onUartError);
    connect(uartChannel.get(), &UartChannel::connectionChanged,
            this, &DeviceManager::onUartConnectionChanged);
    
    // 设置设备 ID
    bool ok = uartChannel->connect(config);
    if (!ok) {
        LOG_ERROR("Failed to connect UART for device: " + deviceId.toStdString());
        return false;
    }
    
    uartChannels_[deviceId] = uartChannel;
    updateDeviceStatus(deviceId, true, devices_[deviceId].online);
    
    LOG_INFO("Device connected via UART: " + deviceId.toStdString());
    return true;
}

void DeviceManager::disconnectDevice(const QString& deviceId)
{
    if (uartChannels_.contains(deviceId)) {
        uartChannels_[deviceId]->disconnect();
        uartChannels_.remove(deviceId);
        updateDeviceStatus(deviceId, false, devices_[deviceId].online);
        LOG_INFO("Device disconnected: " + deviceId.toStdString());
    }
}

QVector<QSerialPortInfo> DeviceManager::getAvailablePorts() const
{
    return UartChannel::availablePorts();
}

void DeviceManager::sendHeartbeat(const QString& deviceId)
{
    if (uartChannels_.contains(deviceId)) {
        // 从设备 ID 提取数字部分
        bool ok;
        uint8_t devId = deviceId.toInt(&ok);
        if (ok) {
            uartChannels_[deviceId]->sendHeartbeat(devId);
        }
    }
}

void DeviceManager::sendControlCommand(const QString& deviceId, uint8_t cmdType,
                                       const std::vector<uint8_t>& payload)
{
    if (uartChannels_.contains(deviceId)) {
        bool ok;
        uint8_t devId = deviceId.toInt(&ok);
        if (ok) {
            uartChannels_[deviceId]->sendControlCommand(devId, cmdType, payload);
        }
    }
}

void DeviceManager::setMqttChannel(std::shared_ptr<MqttChannel> mqttChannel)
{
    mqttChannel_ = mqttChannel;
    if (mqttChannel_) {
        connect(mqttChannel_.get(), &MqttChannel::messageReceived,
                this, &DeviceManager::onMqttMessage);
        connect(mqttChannel_.get(), &MqttChannel::connectionChanged,
                this, [this](bool connected) {
            LOG_INFO("MQTT connection changed: " + std::to_string(connected));
            // 更新所有 MQTT 设备的连接状态
            for (auto it = devices_.begin(); it != devices_.end(); ++it) {
                if (it.value().connectionType == "MQTT") {
                    updateDeviceStatus(it.key(), connected, connected);
                }
            }
        });
        connect(mqttChannel_.get(), &MqttChannel::deviceStatusReceived,
                this, [this](const QString& deviceId, bool online) {
            LOG_INFO("Device status received: " + deviceId.toStdString() + 
                     " online=" + std::to_string(online));
            if (devices_.contains(deviceId)) {
                updateDeviceStatus(deviceId, devices_[deviceId].connected, online);
            } else {
                // 自动添加新发现的设备
                DeviceInfo info;
                info.id = deviceId;
                info.name = "MQTT Device " + deviceId;
                info.connectionType = "MQTT";
                info.online = online;
                info.connected = mqttChannel_ && mqttChannel_->isConnected();
                info.lastSeen = QDateTime::currentMSecsSinceEpoch();
                devices_[deviceId] = info;
                emit devicesChanged(devices_);
            }
        });
        connect(mqttChannel_.get(), &MqttChannel::telemetryReceived,
                this, [this](const QString& deviceId, const QString& data) {
            // TODO: 解析 JSON 遥测数据
            LOG_DEBUG("Telemetry received from " + deviceId.toStdString() + ": " + data.toStdString());
        });
        connect(mqttChannel_.get(), &MqttChannel::deviceCommandReceived,
                this, [this](const QString& deviceId, const QString& command) {
            LOG_INFO("Command received for device " + deviceId.toStdString() + ": " + command.toStdString());
            // TODO: 处理设备命令
        });
    }
}

bool DeviceManager::connectDeviceMqtt(const QString& deviceId, const QString& brokerHost, quint16 brokerPort)
{
    if (!mqttChannel_) {
        LOG_ERROR("MQTT channel not configured");
        return false;
    }
    
    if (devices_.contains(deviceId)) {
        LOG_WARNING("Device already exists: " + deviceId.toStdString());
        return false;
    }
    
    // 添加设备
    DeviceInfo info;
    info.id = deviceId;
    info.name = "MQTT Device " + deviceId;
    info.connectionType = "MQTT";
    info.port = brokerHost + ":" + QString::number(brokerPort);
    info.connected = mqttChannel_->isConnected();
    info.online = false;
    info.lastSeen = QDateTime::currentMSecsSinceEpoch();
    devices_[deviceId] = info;
    
    // 订阅该设备的命令和状态
    mqttChannel_->subscribeDeviceCommands(deviceId);
    mqttChannel_->subscribe("heterolink/subboard/" + deviceId + "/status");
    
    LOG_INFO("MQTT device added: " + deviceId.toStdString());
    emit devicesChanged(devices_);
    return true;
}

void DeviceManager::disconnectDeviceMqtt(const QString& deviceId)
{
    if (!mqttChannel_ || !devices_.contains(deviceId)) {
        return;
    }
    
    if (devices_[deviceId].connectionType != "MQTT") {
        return;
    }
    
    // 取消订阅
    mqttChannel_->unsubscribe("heterolink/subboard/" + deviceId + "/command");
    mqttChannel_->unsubscribe("heterolink/subboard/" + deviceId + "/status");
    
    devices_.remove(deviceId);
    LOG_INFO("MQTT device removed: " + deviceId.toStdString());
    emit devicesChanged(devices_);
}

void DeviceManager::sendControlCommandMqtt(const QString& deviceId, const QString& command)
{
    if (!mqttChannel_) {
        LOG_ERROR("MQTT channel not configured");
        return;
    }
    
    mqttChannel_->publishCommand(deviceId, command);
    LOG_INFO("MQTT command sent to " + deviceId.toStdString() + ": " + command.toStdString());
}

void DeviceManager::onUartTelemetry(uint8_t deviceId, const TelemetryData& data)
{
    QString devId = QString::number(deviceId);
    if (devices_.contains(devId)) {
        devices_[devId].lastSeen = QDateTime::currentMSecsSinceEpoch();
        emit telemetryReceived(devId, data);
    }
}

void DeviceManager::onUartError(uint8_t deviceId, ErrorCode error)
{
    QString devId = QString::number(deviceId);
    QString errorMsg = QString("Device %1 error code: %2")
                          .arg(devId).arg(static_cast<int>(error));
    emit deviceError(devId, errorMsg);
}

void DeviceManager::onUartConnectionChanged(bool connected)
{
    Q_UNUSED(connected)
    // 更新所有 UART 设备的状态
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it.value().connectionType == "UART") {
            updateDeviceStatus(it.key(), connected, it.value().online);
        }
    }
}

void DeviceManager::onMqttMessage(const QString& topic, const QByteArray& payload)
{
    parseMqttMessage(topic, payload);
}

void DeviceManager::parseMqttMessage(const QString& topic, const QByteArray& payload)
{
    // 解析 MQTT 消息格式：heterolink/subboard/<deviceId>/status
    QStringList parts = topic.split('/');
    if (parts.size() < 4) {
        return;
    }
    
    QString deviceId = parts[2];
    QString messageType = parts[3];
    
    if (messageType == "status") {
        bool online = (payload == "online");
        updateDeviceStatus(deviceId, devices_[deviceId].connected, online);
        LOG_INFO("Device status via MQTT: " + deviceId.toStdString() + 
                 " online=" + (online ? "true" : "false"));
    }
}

void DeviceManager::updateDeviceStatus(const QString& deviceId, bool connected, bool online)
{
    if (devices_.contains(deviceId)) {
        devices_[deviceId].connected = connected;
        devices_[deviceId].online = online;
        emit deviceStatusChanged(deviceId, connected, online);
    }
}

} // namespace HeteroLink
