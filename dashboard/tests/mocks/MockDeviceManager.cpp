/**
 * HeteroLink Host - Mock 设备管理器实现
 * 
 * 注意：由于基类 DeviceManager 的槽函数 (onUartTelemetry 等) 是私有的，
 * MockDeviceManager 使用 Lambda 直接转发信号，而不是连接到私有槽。
 */

#include "MockDeviceManager.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
    
    // 使用 shared_ptr 包装，但不自动删除（因为 mockMqttChannel_ 由我们管理）
    setMqttChannel(std::shared_ptr<MqttChannel>(mockMqttChannel_, [](QObject*){
        // 空删除器，由 MockDeviceManager 管理生命周期
    }));
    
    // 连接 MQTT 信号到 DeviceManager 的处理逻辑
    connect(mockMqttChannel_, &MockMqttChannel::deviceStatusReceived,
            this, [this](const QString& deviceId, bool online) {
        emit deviceStatusChanged(deviceId, true, online);
    });
    
    // MQTT 遥测数据：解析 JSON 并转换为 TelemetryData 后发射信号
    connect(mockMqttChannel_, &MockMqttChannel::telemetryReceived,
            this, [this](const QString& deviceId, const QString& data) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            TelemetryData telemetry;
            if (obj.contains("timestamp")) {
                telemetry.timestamp = obj["timestamp"].toVariant().toUInt();
            }
            if (obj.contains("channels")) {
                QJsonArray channels = obj["channels"].toArray();
                for (const auto& ch : channels) {
                    telemetry.channels.push_back(ch.toDouble());
                }
            }
            emit telemetryReceived(deviceId, telemetry);
        }
    });
}

void MockDeviceManager::updateDeviceStatusPublic(const QString& deviceId, bool connected, bool online)
{
    // 通过 addDevice 更新设备状态（会覆盖现有设备）
    DeviceInfo info = getDevice(deviceId);
    if (!info.id.isEmpty()) {
        info.connected = connected;
        info.online = online;
        info.lastSeen = connected ? QDateTime::currentMSecsSinceEpoch() : 0;
        // 移除后重新添加以更新状态
        removeDevice(deviceId);
        addDevice(info);
    }
}

MockUartChannel* MockDeviceManager::getMockUartChannel(const QString& deviceId)
{
    if (!mockUartChannels_.contains(deviceId)) {
        auto* mockChannel = new MockUartChannel(this);
        mockUartChannels_[deviceId] = mockChannel;
        
        // 连接信号 - 使用 Lambda 直接转发，避免访问私有槽
        connect(mockChannel, &MockUartChannel::telemetryReceived,
                this, [this, deviceId](uint8_t /*devId*/, const TelemetryData& data) {
            // 更新最后通信时间
            DeviceInfo info = getDevice(deviceId);
            if (!info.id.isEmpty()) {
                info.lastSeen = QDateTime::currentMSecsSinceEpoch();
                // 注意：无法直接修改 devices_ map，需要通过公共方法
            }
            emit telemetryReceived(deviceId, data);
        });
        
        connect(mockChannel, &MockUartChannel::errorReceived,
                this, [this, deviceId](uint8_t /*devId*/, ErrorCode error) {
            QString errorMsg = QString("Device %1 error code: %2")
                                  .arg(deviceId).arg(static_cast<int>(error));
            emit deviceError(deviceId, errorMsg);
        });
        
        connect(mockChannel, &MockUartChannel::connectionChanged,
                this, [this, deviceId](bool connected) {
            // 更新设备连接状态
            // 注意：getDevice() 返回的是值拷贝，无法直接修改
            // online 状态与 connected 同步
            emit deviceStatusChanged(deviceId, connected, connected);
        });
        
        connect(mockChannel, &MockUartChannel::errorOccurred,
                this, [this, deviceId](const QString& error) {
            emit deviceError(deviceId, error);
        });
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
    info.lastSeen = 0;
    
    if (connectionType == "UART") {
        getMockUartChannel(deviceId);
    }
    
    return addDevice(info);
}

void MockDeviceManager::simulateTelemetry(const QString& deviceId, const TelemetryData& telemetry)
{
    if (mockUartChannels_.contains(deviceId)) {
        // UART 模式：直接发射信号
        mockUartChannels_[deviceId]->simulateTelemetry(0x01, telemetry);
    } else if (mockMqttChannel_) {
        // MQTT 模式：通过 MQTT 通道模拟
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
    // 通过基类的公共方法更新状态
    if (getDevices().contains(deviceId)) {
        DeviceInfo info = getDevice(deviceId);
        info.connected = connected;
        info.online = online;
        info.lastSeen = connected ? QDateTime::currentMSecsSinceEpoch() : 0;
        // 注意：无法直接调用 updateDeviceStatus（私有），直接发射信号
        emit deviceStatusChanged(deviceId, connected, online);
    }
}

void MockDeviceManager::simulateDeviceDisconnect(const QString& deviceId)
{
    if (mockUartChannels_.contains(deviceId)) {
        mockUartChannels_[deviceId]->simulateDisconnect();
    }
    // 注意：connectionChanged 信号会自动转发到 deviceStatusChanged
}

void MockDeviceManager::simulateDeviceConnect(const QString& deviceId)
{
    if (mockUartChannels_.contains(deviceId)) {
        mockUartChannels_[deviceId]->simulateConnect();
    }
    // 注意：connectionChanged 信号会自动转发到 deviceStatusChanged
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
