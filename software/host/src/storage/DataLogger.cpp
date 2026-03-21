/**
 * HeteroLink Host - 数据记录器实现
 */

#include "storage/DataLogger.h"
#include "utils/Logger.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace HeteroLink {

DataLogger::DataLogger(QObject *parent)
    : QObject(parent)
{
}

DataLogger::~DataLogger()
{
    stopRecording();
}

bool DataLogger::startRecording(const QString& basePath, const QString& deviceId)
{
    if (recording_) {
        LOG_WARNING("Already recording");
        return false;
    }
    
    basePath_ = basePath;
    deviceId_ = deviceId;
    
    // 创建目录
    QDir dir(basePath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("Failed to create directory: " + basePath.toStdString());
            return false;
        }
    }
    
    if (!openFile()) {
        return false;
    }
    
    recording_ = true;
    bytesWritten_ = 0;
    lastSplitTime_ = QDateTime::currentMSecsSinceEpoch();
    
    LOG_INFO("Recording started: " + currentFilePath().toStdString());
    emit recordingStarted(currentFilePath());
    
    return true;
}

void DataLogger::stopRecording()
{
    if (!recording_) {
        return;
    }
    
    closeFile();
    recording_ = false;
    
    LOG_INFO("Recording stopped");
    emit recordingStopped();
}

bool DataLogger::isRecording() const
{
    return recording_;
}

QString DataLogger::currentFilePath() const
{
    if (file_ && file_->isOpen()) {
        return file_->fileName();
    }
    return "";
}

void DataLogger::setMaxFileSize(int sizeMB)
{
    maxFileSize_ = sizeMB * 1024 * 1024;  // 转换为字节
}

void DataLogger::setAutoSplit(bool enabled, int intervalMs)
{
    autoSplit_ = enabled;
    splitInterval_ = intervalMs;
}

void DataLogger::writeData(const QString& deviceId, const TelemetryData& data)
{
    if (!recording_ || deviceId != deviceId_) {
        return;
    }
    
    if (!stream_) {
        return;
    }
    
    // 写入数据行
    QString line = QString::number(data.timestamp);
    for (const auto& ch : data.channels) {
        line += "," + QString::number(ch, 'f', 6);
    }
    line += "\n";
    
    QByteArray bytes = line.toUtf8();
    qint64 written = stream_->writeString(bytes);
    bytesWritten_ += written;
    
    // 检查是否需要分割文件
    if (autoSplit_) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        
        // 检查文件大小
        if (bytesWritten_ >= maxFileSize_) {
            splitFile();
        }
        // 检查时间间隔
        else if (now - lastSplitTime_ >= splitInterval_) {
            splitFile();
        }
    }
}

bool DataLogger::openFile()
{
    QString filePath = generateFilePath();
    
    file_ = std::make_unique<QFile>(filePath);
    if (!file_->open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open file: " + filePath.toStdString());
        emit errorOccurred("Failed to open file: " + file_->errorString());
        file_.reset();
        stream_.reset();
        return false;
    }
    
    stream_ = std::make_unique<QTextStream>(file_.get());
    writeHeader();
    
    return true;
}

void DataLogger::closeFile()
{
    if (stream_) {
        stream_->flush();
        stream_.reset();
    }
    
    if (file_) {
        file_->close();
        file_.reset();
    }
}

QString DataLogger::generateFilePath() const
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_HHmmss");
    QString fileName = QString("heterolink_%1_%2.csv").arg(deviceId_).arg(timestamp);
    return basePath_ + "/" + fileName;
}

void DataLogger::writeHeader()
{
    if (!stream_) {
        return;
    }
    
    // 写入 CSV 表头（假设最多 16 个通道）
    QString header = "timestamp";
    for (int i = 0; i < 16; ++i) {
        header += ",ch" + QString::number(i);
    }
    header += "\n";
    
    stream_->writeString(header.toUtf8());
    stream_->flush();
}

void DataLogger::splitFile()
{
    QString oldPath = file_->fileName();
    
    closeFile();
    
    if (!openFile()) {
        return;
    }
    
    bytesWritten_ = 0;
    lastSplitTime_ = QDateTime::currentMSecsSinceEpoch();
    
    LOG_INFO("File split: " + oldPath.toStdString() + " -> " + currentFilePath().toStdString());
    emit fileSplit(oldPath, currentFilePath());
}

} // namespace HeteroLink
