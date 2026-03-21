/**
 * HeteroLink Host - 数据处理器实现
 */

#include "core/DataProcessor.h"
#include "utils/Logger.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

namespace HeteroLink {

DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent)
{
}

DataProcessor::~DataProcessor()
{
}

void DataProcessor::setBufferSize(const QString& deviceId, int size)
{
    if (!deviceData_.contains(deviceId)) {
        deviceData_[deviceId] = std::make_unique<DeviceData>(size);
    } else {
        deviceData_[deviceId]->buffer.resize(size);
    }
    LOG_INFO("Buffer size set for device " + deviceId.toStdString() + ": " + std::to_string(size));
}

void DataProcessor::addData(const QString& deviceId, const TelemetryData& data)
{
    if (!deviceData_.contains(deviceId)) {
        deviceData_[deviceId] = std::make_unique<DeviceData>(defaultBufferSize_);
    }
    
    deviceData_[deviceId]->buffer.append(data);
    deviceData_[deviceId]->lastUpdate = QDateTime::currentMSecsSinceEpoch();
    
    updateStats(deviceId, data);
    
    emit dataUpdated(deviceId);
}

QVector<TelemetryData> DataProcessor::getData(const QString& deviceId) const
{
    if (!deviceData_.contains(deviceId)) {
        return QVector<TelemetryData>();
    }
    
    QVector<TelemetryData> result;
    const auto& buffer = deviceData_[deviceId]->buffer;
    for (int i = 0; i < buffer.size(); ++i) {
        result.append(buffer.at(i));
    }
    return result;
}

QVector<TelemetryData> DataProcessor::getLatestData(const QString& deviceId, int count) const
{
    if (!deviceData_.contains(deviceId)) {
        return QVector<TelemetryData>();
    }
    
    QVector<TelemetryData> result;
    const auto& buffer = deviceData_[deviceId]->buffer;
    int start = qMax(0, buffer.size() - count);
    for (int i = start; i < buffer.size(); ++i) {
        result.append(buffer.at(i));
    }
    return result;
}

QMap<int, ChannelStats> DataProcessor::getStats(const QString& deviceId) const
{
    if (!deviceData_.contains(deviceId)) {
        return QMap<int, ChannelStats>();
    }
    
    return deviceData_[deviceId]->channelStats;
}

void DataProcessor::clearData(const QString& deviceId)
{
    if (deviceData_.contains(deviceId)) {
        deviceData_[deviceId]->buffer.clear();
        deviceData_[deviceId]->channelStats.clear();
        emit dataUpdated(deviceId);
        emit statsUpdated(deviceId);
    }
}

void DataProcessor::clearAll()
{
    deviceData_.clear();
    LOG_INFO("All data cleared");
}

bool DataProcessor::exportToCsv(const QString& deviceId, const QString& filePath) const
{
    if (!deviceData_.contains(deviceId)) {
        LOG_ERROR("No data for device: " + deviceId.toStdString());
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open file: " + filePath.toStdString());
        return false;
    }
    
    QTextStream out(&file);
    
    // 写入表头
    const auto& data = deviceData_[deviceId]->buffer;
    if (data.isEmpty()) {
        return true;
    }
    
    int channelCount = data[0].channels.size();
    out << "timestamp";
    for (int i = 0; i < channelCount; ++i) {
        out << ",channel" << i;
    }
    out << "\n";
    
    // 写入数据
    for (int i = 0; i < data.size(); ++i) {
        out << data[i].timestamp;
        for (int ch = 0; ch < data[i].channels.size(); ++ch) {
            out << "," << data[i].channels[ch];
        }
        out << "\n";
    }
    
    file.close();
    LOG_INFO("Data exported to CSV: " + filePath.toStdString());
    return true;
}

bool DataProcessor::exportToJson(const QString& deviceId, const QString& filePath) const
{
    if (!deviceData_.contains(deviceId)) {
        LOG_ERROR("No data for device: " + deviceId.toStdString());
        return false;
    }
    
    const auto& data = deviceData_[deviceId]->buffer;
    QJsonArray jsonArray;
    
    for (int i = 0; i < data.size(); ++i) {
        QJsonObject obj;
        obj["timestamp"] = static_cast<qint64>(data[i].timestamp);
        
        QJsonArray channels;
        for (const auto& ch : data[i].channels) {
            channels.append(ch);
        }
        obj["channels"] = channels;
        
        jsonArray.append(obj);
    }
    
    QJsonDocument doc(jsonArray);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open file: " + filePath.toStdString());
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    LOG_INFO("Data exported to JSON: " + filePath.toStdString());
    return true;
}

void DataProcessor::updateStats(const QString& deviceId, const TelemetryData& data)
{
    auto& devData = deviceData_[deviceId];
    
    for (int i = 0; i < data.channels.size(); ++i) {
        float value = data.channels[i];
        
        if (!devData->channelStats.contains(i)) {
            devData->channelStats[i] = ChannelStats();
        }
        
        auto& stats = devData->channelStats[i];
        
        // 更新最小/最大值
        if (stats.sampleCount == 0 || value < stats.min) {
            stats.min = value;
        }
        if (stats.sampleCount == 0 || value > stats.max) {
            stats.max = value;
        }
        
        // 更新平均值（增量计算）
        float oldAvg = stats.avg;
        stats.sampleCount++;
        stats.avg = oldAvg + (value - oldAvg) / stats.sampleCount;
        
        // 更新 RMS（均方根）
        stats.rms = std::sqrt(stats.rms * stats.rms * (stats.sampleCount - 1) / stats.sampleCount +
                             value * value / stats.sampleCount);
    }
    
    emit statsUpdated(deviceId);
}

ChannelStats DataProcessor::calculateStats(const QVector<float>& values)
{
    ChannelStats stats;
    
    if (values.isEmpty()) {
        return stats;
    }
    
    stats.min = values[0];
    stats.max = values[0];
    float sum = 0;
    float sumSq = 0;
    
    for (float v : values) {
        if (v < stats.min) stats.min = v;
        if (v > stats.max) stats.max = v;
        sum += v;
        sumSq += v * v;
    }
    
    stats.sampleCount = values.size();
    stats.avg = sum / stats.sampleCount;
    stats.rms = std::sqrt(sumSq / stats.sampleCount);
    
    return stats;
}

} // namespace HeteroLink
