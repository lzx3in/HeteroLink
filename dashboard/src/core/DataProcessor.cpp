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
    qDeleteAll(deviceData_);
}

void DataProcessor::setBufferSize(const QString& deviceId, int size)
{
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        deviceData_[deviceId] = new DeviceData(size);
    } else {
        (*it)->maxBufferSize = size;
        // 修剪缓冲区
        while ((*it)->buffer.size() > size) {
            (*it)->buffer.dequeue();
        }
    }
    LOG_INFO("Buffer size set for device " + deviceId + ": " + QString::number(size));
}

void DataProcessor::addData(const QString& deviceId, const TelemetryData& data)
{
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        deviceData_[deviceId] = new DeviceData(defaultBufferSize_);
        it = deviceData_.find(deviceId);
    }
    
    (*it)->buffer.enqueue(data);
    
    // 保持缓冲区大小限制
    while ((*it)->buffer.size() > (*it)->maxBufferSize) {
        (*it)->buffer.dequeue();
    }
    
    (*it)->lastUpdate = QDateTime::currentMSecsSinceEpoch();
    
    updateStats(deviceId, data);
    
    emit dataUpdated(deviceId);
}

QVector<TelemetryData> DataProcessor::getData(const QString& deviceId) const
{
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        return QVector<TelemetryData>();
    }
    
    QVector<TelemetryData> result;
    const auto& buffer = (*it)->buffer;
    result.reserve(buffer.size());
    for (const auto& item : buffer) {
        result.append(item);
    }
    return result;
}

QVector<TelemetryData> DataProcessor::getLatestData(const QString& deviceId, int count) const
{
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        return QVector<TelemetryData>();
    }
    
    const auto& buffer = (*it)->buffer;
    int start = qMax(0, buffer.size() - count);
    QVector<TelemetryData> result;
    result.reserve(buffer.size() - start);
    
    auto bit = buffer.begin();
    std::advance(bit, start);
    for (; bit != buffer.end(); ++bit) {
        result.append(*bit);
    }
    return result;
}

QMap<int, ChannelStats> DataProcessor::getStats(const QString& deviceId) const
{
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        return QMap<int, ChannelStats>();
    }
    
    return (*it)->channelStats;
}

void DataProcessor::clearData(const QString& deviceId)
{
    auto it = deviceData_.find(deviceId);
    if (it != deviceData_.end()) {
        (*it)->buffer.clear();
        (*it)->channelStats.clear();
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
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        LOG_ERROR("No data for device: " + deviceId);
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open file: " + filePath);
        return false;
    }
    
    QTextStream out(&file);
    
    // 写入表头
    const auto& buffer = (*it)->buffer;
    if (buffer.isEmpty()) {
        return true;
    }
    
    // 转换为 QVector 以便索引访问
    QVector<TelemetryData> data;
    data.reserve(buffer.size());
    for (const auto& item : buffer) {
        data.append(item);
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
    LOG_INFO("Data exported to CSV: " + filePath);
    return true;
}

bool DataProcessor::exportToJson(const QString& deviceId, const QString& filePath) const
{
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        LOG_ERROR("No data for device: " + deviceId);
        return false;
    }
    
    const auto& buffer = (*it)->buffer;
    QJsonArray jsonArray;
    
    for (const auto& item : buffer) {
        QJsonObject obj;
        obj["timestamp"] = static_cast<qint64>(item.timestamp);
        
        QJsonArray channels;
        for (const auto& ch : item.channels) {
            channels.append(ch);
        }
        obj["channels"] = channels;
        
        jsonArray.append(obj);
    }
    
    QJsonDocument doc(jsonArray);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open file: " + filePath);
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    LOG_INFO("Data exported to JSON: " + filePath);
    return true;
}

void DataProcessor::updateStats(const QString& deviceId, const TelemetryData& data)
{
    auto it = deviceData_.find(deviceId);
    if (it == deviceData_.end()) {
        return;
    }
    
    auto& devData = (*it);
    
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
