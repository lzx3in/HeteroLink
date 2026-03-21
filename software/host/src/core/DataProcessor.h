/**
 * HeteroLink Host - 数据处理器
 * 
 * @file DataProcessor.h
 * @brief 数据采集、缓存、统计
 */

#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <QCircularBuffer>
#include <memory>

#include "protocol/Protocol.h"

namespace HeteroLink {

/**
 * @brief 通道统计数据
 */
struct ChannelStats {
    float min = 0;
    float max = 0;
    float avg = 0;
    float rms = 0;
    quint32 sampleCount = 0;
    
    ChannelStats() = default;
};

/**
 * @brief 设备数据
 */
struct DeviceData {
    QString deviceId;
    QCircularBuffer<TelemetryData> buffer;  // 环形缓冲区
    QMap<int, ChannelStats> channelStats;  // 通道统计
    quint64 lastUpdate = 0;
    
    DeviceData(int bufferSize = 10000) : buffer(bufferSize) {}
};

/**
 * @brief 数据处理器类
 * 
 * 负责数据缓存、统计分析、数据导出
 */
class DataProcessor : public QObject
{
    Q_OBJECT
    
public:
    explicit DataProcessor(QObject *parent = nullptr);
    ~DataProcessor();
    
    /**
     * @brief 设置缓冲区大小
     * @param deviceId 设备 ID
     * @param size 缓冲区大小（样本数）
     */
    void setBufferSize(const QString& deviceId, int size);
    
    /**
     * @brief 添加数据
     * @param deviceId 设备 ID
     * @param data 遥测数据
     */
    void addData(const QString& deviceId, const TelemetryData& data);
    
    /**
     * @brief 获取数据
     * @param deviceId 设备 ID
     * @return 数据列表
     */
    QVector<TelemetryData> getData(const QString& deviceId) const;
    
    /**
     * @brief 获取最新数据
     * @param deviceId 设备 ID
     * @param count 数量
     * @return 数据列表
     */
    QVector<TelemetryData> getLatestData(const QString& deviceId, int count) const;
    
    /**
     * @brief 获取统计数据
     * @param deviceId 设备 ID
     * @return 通道统计
     */
    QMap<int, ChannelStats> getStats(const QString& deviceId) const;
    
    /**
     * @brief 清除数据
     * @param deviceId 设备 ID
     */
    void clearData(const QString& deviceId);
    
    /**
     * @brief 清除所有数据
     */
    void clearAll();
    
    /**
     * @brief 导出数据到 CSV
     * @param deviceId 设备 ID
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportToCsv(const QString& deviceId, const QString& filePath) const;
    
    /**
     * @brief 导出数据到 JSON
     * @param deviceId 设备 ID
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportToJson(const QString& deviceId, const QString& filePath) const;
    
signals:
    /**
     * @brief 数据更新
     * @param deviceId 设备 ID
     */
    void dataUpdated(const QString& deviceId);
    
    /**
     * @brief 统计更新
     * @param deviceId 设备 ID
     */
    void statsUpdated(const QString& deviceId);
    
private:
    QMap<QString, std::unique_ptr<DeviceData>> deviceData_;
    int defaultBufferSize_ = 10000;
    
    void updateStats(const QString& deviceId, const TelemetryData& data);
    ChannelStats calculateStats(const QVector<float>& values);
};

} // namespace HeteroLink
