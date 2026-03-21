/**
 * HeteroLink Host - 告警系统实现
 */

#include "core/AlarmSystem.h"
#include "utils/Logger.h"

namespace HeteroLink {

AlarmSystem::AlarmSystem(QObject *parent)
    : QObject(parent)
{
}

AlarmSystem::~AlarmSystem()
{
}

void AlarmSystem::configureAlarm(const QString& deviceId, const AlarmConfig& config)
{
    if (!alarmConfigs_.contains(deviceId)) {
        alarmConfigs_[deviceId] = QVector<AlarmConfig>();
    }
    
    // 查找是否已存在该通道的配置
    auto& configs = alarmConfigs_[deviceId];
    bool found = false;
    for (auto& cfg : configs) {
        if (cfg.channelId == config.channelId) {
            cfg = config;
            found = true;
            break;
        }
    }
    
    if (!found) {
        configs.append(config);
    }
    
    LOG_INFO("Alarm configured for device " + deviceId.toStdString() + 
             " channel " + std::to_string(config.channelId));
}

QVector<AlarmConfig> AlarmSystem::getAlarms(const QString& deviceId) const
{
    return alarmConfigs_.value(deviceId);
}

void AlarmSystem::setAlarmEnabled(const QString& deviceId, int channelId, bool enabled)
{
    if (!alarmConfigs_.contains(deviceId)) {
        return;
    }
    
    auto& configs = alarmConfigs_[deviceId];
    for (auto& cfg : configs) {
        if (cfg.channelId == channelId) {
            cfg.enabled = enabled;
            LOG_INFO("Alarm " + (enabled ? "enabled" : "disabled") + 
                     " for device " + deviceId.toStdString() + 
                     " channel " + std::to_string(channelId));
            break;
        }
    }
}

void AlarmSystem::checkData(const QString& deviceId, const TelemetryData& data)
{
    if (!alarmConfigs_.contains(deviceId)) {
        return;
    }
    
    const auto& configs = alarmConfigs_[deviceId];
    
    for (const auto& config : configs) {
        if (!config.enabled || config.channelId >= data.channels.size()) {
            continue;
        }
        
        float value = data.channels[config.channelId];
        
        // 检查下限
        if (config.lowerEnabled && value < config.lowerLimit) {
            QString msg = QString("Channel %1 value %2 below lower limit %3")
                             .arg(config.channelId).arg(value).arg(config.lowerLimit);
            triggerAlarm(deviceId, config.channelId, config.level, value, msg);
        }
        // 检查上限
        else if (config.upperEnabled && value > config.upperLimit) {
            QString msg = QString("Channel %1 value %2 above upper limit %3")
                             .arg(config.channelId).arg(value).arg(config.upperLimit);
            triggerAlarm(deviceId, config.channelId, config.level, value, msg);
        }
        // 检查恢复
        else if (activeAlarms_[deviceId][config.channelId]) {
            // 值回到正常范围，清除告警
            activeAlarms_[deviceId][config.channelId] = false;
            emit alarmCleared(deviceId, config.channelId);
            LOG_INFO("Alarm cleared for device " + deviceId.toStdString() + 
                     " channel " + std::to_string(config.channelId));
        }
    }
}

QVector<AlarmRecord> AlarmSystem::getAlarmRecords(const QString& deviceId) const
{
    return alarmRecords_.value(deviceId);
}

QVector<AlarmRecord> AlarmSystem::getAllAlarmRecords() const
{
    QVector<AlarmRecord> allRecords;
    for (auto it = alarmRecords_.begin(); it != alarmRecords_.end(); ++it) {
        allRecords.append(it.value());
    }
    return allRecords;
}

void AlarmSystem::acknowledgeAlarm(const QString& deviceId, int channelId)
{
    if (!alarmRecords_.contains(deviceId)) {
        return;
    }
    
    auto& records = alarmRecords_[deviceId];
    for (auto& record : records) {
        if (record.channelId == channelId && !record.acknowledged) {
            record.acknowledged = true;
            LOG_INFO("Alarm acknowledged for device " + deviceId.toStdString() + 
                     " channel " + std::to_string(channelId));
            break;
        }
    }
}

void AlarmSystem::clearRecords(const QString& deviceId)
{
    alarmRecords_.remove(deviceId);
    LOG_INFO("Alarm records cleared for device: " + deviceId.toStdString());
}

void AlarmSystem::triggerAlarm(const QString& deviceId, int channelId, AlarmLevel level,
                               float value, const QString& message)
{
    // 如果告警已激活，不再重复触发（避免刷屏）
    if (activeAlarms_[deviceId][channelId]) {
        return;
    }
    
    activeAlarms_[deviceId][channelId] = true;
    
    AlarmRecord record;
    record.deviceId = deviceId;
    record.channelId = channelId;
    record.level = level;
    record.value = value;
    record.message = message;
    record.timestamp = QDateTime::currentDateTime();
    record.acknowledged = false;
    
    if (!alarmRecords_.contains(deviceId)) {
        alarmRecords_[deviceId] = QVector<AlarmRecord>();
    }
    alarmRecords_[deviceId].append(record);
    
    LOG_WARNING("Alarm triggered: " + message.toStdString());
    emit alarmTriggered(deviceId, record);
}

} // namespace HeteroLink
