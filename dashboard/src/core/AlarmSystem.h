/**
 * HeteroLink Host - 告警系统
 * 
 * @file AlarmSystem.h
 * @brief 阈值监控、告警触发
 */

#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <QVector>

#include "protocol/Protocol.h"

namespace HeteroLink {

/**
 * @brief 告警级别
 */
enum class AlarmLevel {
    INFO,
    WARNING,
    CRITICAL
};

/**
 * @brief 告警配置
 */
struct AlarmConfig {
    int channelId = 0;
    float lowerLimit = 0;
    float upperLimit = 0;
    bool lowerEnabled = false;
    bool upperEnabled = false;
    AlarmLevel level = AlarmLevel::WARNING;
    bool enabled = true;
    
    AlarmConfig() = default;
};

/**
 * @brief 告警记录
 */
struct AlarmRecord {
    QString deviceId;
    int channelId;
    AlarmLevel level;
    float value;
    QString message;
    QDateTime timestamp;
    bool acknowledged = false;
    
    AlarmRecord() = default;
};

/**
 * @brief 告警系统类
 * 
 * 监控数据阈值，触发告警
 */
class AlarmSystem : public QObject
{
    Q_OBJECT
    
public:
    explicit AlarmSystem(QObject *parent = nullptr);
    ~AlarmSystem();
    
    /**
     * @brief 配置告警
     * @param deviceId 设备 ID
     * @param config 告警配置
     */
    void configureAlarm(const QString& deviceId, const AlarmConfig& config);
    
    /**
     * @brief 获取告警配置
     * @param deviceId 设备 ID
     * @return 告警配置列表
     */
    QVector<AlarmConfig> getAlarms(const QString& deviceId) const;
    
    /**
     * @brief 启用/禁用告警
     * @param deviceId 设备 ID
     * @param channelId 通道 ID
     * @param enabled 是否启用
     */
    void setAlarmEnabled(const QString& deviceId, int channelId, bool enabled);
    
    /**
     * @brief 检查数据
     * @param deviceId 设备 ID
     * @param data 遥测数据
     */
    void checkData(const QString& deviceId, const TelemetryData& data);
    
    /**
     * @brief 获取告警记录
     * @param deviceId 设备 ID
     * @return 告警记录列表
     */
    QVector<AlarmRecord> getAlarmRecords(const QString& deviceId) const;
    
    /**
     * @brief 获取所有告警记录
     * @return 告警记录列表
     */
    QVector<AlarmRecord> getAllAlarmRecords() const;
    
    /**
     * @brief 确认告警
     * @param deviceId 设备 ID
     * @param channelId 通道 ID
     */
    void acknowledgeAlarm(const QString& deviceId, int channelId);
    
    /**
     * @brief 清除告警记录
     * @param deviceId 设备 ID
     */
    void clearRecords(const QString& deviceId);
    
signals:
    /**
     * @brief 告警触发
     * @param deviceId 设备 ID
     * @param record 告警记录
     */
    void alarmTriggered(const QString& deviceId, const AlarmRecord& record);
    
    /**
     * @brief 告警恢复
     * @param deviceId 设备 ID
     * @param channelId 通道 ID
     */
    void alarmCleared(const QString& deviceId, int channelId);
    
private:
    QMap<QString, QVector<AlarmConfig>> alarmConfigs_;
    QMap<QString, QVector<AlarmRecord>> alarmRecords_;
    QMap<QString, QMap<int, bool>> activeAlarms_;  // deviceId -> channelId -> active
    
    void triggerAlarm(const QString& deviceId, int channelId, AlarmLevel level,
                     float value, const QString& message);
};

} // namespace HeteroLink
