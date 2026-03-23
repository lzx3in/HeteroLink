/**
 * HeteroLink Host - Mock 设备管理器实现
 */

#include "MockDeviceManager.h"

namespace HeteroLink {

MockDeviceManager::MockDeviceManager(QObject *parent)
    : DeviceManager(parent)
{
    setupMockChannels();
}

MockDeviceManager::~MockDeviceManager()
{
    clearMockDevices();
}

void MockDeviceManager::setupMockChannels()
{
    // 创建 Mock MQTT 通道
    mockMqttChannel_ = new MockMqttChannel(this);
    setMqttChannel(std::shared_ptr<MqttChannel>(mockMqttChannel_, [](QObject*){}));
}

MockUartChannel* MockDeviceManager::getMockUartChannel(const QString& deviceId)
{
    if (!mockUartChannels_.contains(deviceId)) {
        auto* mockChannel = new MockUartChannel(this);
        mockUartChannels_[deviceId] = mockChannel;
        
        // 连接信号
        connect(mockChannel, &MockUartChannel::telemetryReceived,
                this, &MockDeviceManager::onUartTelemetry);
        connect(mockChannel, &MockUartChannel::errorReceived,
                this, &MockDeviceManager::onUartError);
        connect(mockChannel, &MockUartChannel::connectionChanged,
                this, &MockDeviceManager::onUartConnectionChanged);
    }
    
    return mockUartChannels_[deviceId];
}

MockMqttChannel* MockDeviceManager::getMockMqttChannel()
{
    return mockMqttChannel_;
}

bool MockDeviceManager::addMockDevice(const QString& deviceId, const QString& connectionType)
{
    DeviceInfo info;
    info.id = deviceId;
    info.name = QString("Mock Device %1").arg(deviceId);
    info.connectionType = connectionType;
    info.connected = false;
    info.online = false;
    
    if (connectionType == "UART") {
        getMockUartChannel(deviceId);
    }
    
    return addDevice(info);
}

void MockDeviceManager::simulateTelemetry(const QString& deviceId, const TelemetryData& telemetry)
{
    if (mockUartChannels_.contains(deviceId)) {
        mockUartChannels_[deviceId]->simulateTelemetry(0x01, telemetry);
    } else if (mockMqttChannel_) {
        QJsonDocument doc;
        QJsonObject obj;
        obj["timestamp"] = static_cast<qint64>(telemetry.timestamp);
        QJsonArray channels;
        for (float ch : telemetry.channels) {
            channels.append(ch);
        }
        obj["channels"] = channels;
        doc.setObject(obj);
        
        mockMqttChannel_->simulateTelemetry(deviceId, QString::fromUtf8(doc.toJson()));
    }
}

void MockDeviceManager::simulateDeviceStatus(const QString& deviceId, bool connected, bool online)
{
    updateDeviceStatus(deviceId, connected, online);
    emit deviceStatusChanged(deviceId, connected, online);
}

void MockDeviceManager::simulateDeviceDisconnect(const QString& deviceId)
{
    if (mockUartChannels_.contains(deviceId)) {
        mockUartChannels_[deviceId]->simulateDisconnect();
    }
    simulateDeviceStatus(deviceId, false, false);
}

void MockDeviceManager::simulateDeviceConnect(const QString& deviceId)
{
    if (mockUartChannels_.contains(deviceId)) {
        mockUartChannels_[deviceId]->simulateConnect();
    }
    simulateDeviceStatus(deviceId, true, true);
}

QVector<Frame> MockDeviceManager::getSentHeartbeats(const QString& deviceId) const
{
    QVector<Frame> heartbeats;
    
    if (mockUartChannels_.contains(deviceId)) {
        for (const auto& frame : mockUartChannels_[deviceId]->getSentFrames()) {
            if (frame.command == static_cast<uint8_t>(Command::CMD_HEARTBEAT)) {
                heartbeats.append(frame);
            }
        }
    }
    
    return heartbeats;
}

QVector<Frame> MockDeviceManager::getSentCommands(const QString& deviceId) const
{
    QVector<Frame> commands;
    
    if (mockUartChannels_.contains(deviceId)) {
        for (const auto& frame : mockUartChannels_[deviceId]->getSentFrames()) {
            if (frame.command == static_cast<uint8_t>(Command::CMD_CONTROL)) {
                commands.append(frame);
            }
        }
    }
    
    return commands;
}

QVector<MockMqttMessage> MockDeviceManager::getPublishedMqttMessages() const
{
    if (mockMqttChannel_) {
        return mockMqttChannel_->getPublishedMessages();
    }
    return {};
}

void MockDeviceManager::clearMockDevices()
{
    for (auto* channel : mockUartChannels_) {
        channel->deleteLater();
    }
    mockUartChannels_.clear();
    
    if (mockMqttChannel_) {
        mockMqttChannel_->deleteLater();
        mockMqttChannel_ = nullptr;
    }
}

} // namespace HeteroLink
