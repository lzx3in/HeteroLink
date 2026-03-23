/**
 * HeteroLink Host - 数据记录器
 * 
 * @file DataLogger.h
 * @brief 数据持久化存储
 */

#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <memory>

#include "protocol/Protocol.h"

namespace HeteroLink {

class DataProcessor;

/**
 * @brief 数据记录器类
 * 
 * 将采集的数据写入磁盘（CSV 格式）
 */
class DataLogger : public QObject
{
    Q_OBJECT
    
public:
    explicit DataLogger(QObject *parent = nullptr);
    ~DataLogger();
    
    /**
     * @brief 开始记录
     * @param basePath 基础路径
     * @param deviceId 设备 ID
     * @return 是否成功
     */
    bool startRecording(const QString& basePath, const QString& deviceId);
    
    /**
     * @brief 停止记录
     */
    void stopRecording();
    
    /**
     * @brief 检查是否正在记录
     * @return 是否正在记录
     */
    bool isRecording() const;
    
    /**
     * @brief 获取当前文件路径
     * @return 文件路径
     */
    QString currentFilePath() const;
    
    /**
     * @brief 设置最大文件大小（MB）
     * @param sizeMB 大小（MB）
     */
    void setMaxFileSize(int sizeMB);
    
    /**
     * @brief 设置自动分段
     * @param enabled 是否启用
     * @param intervalMs 间隔（毫秒）
     */
    void setAutoSplit(bool enabled, int intervalMs = 3600000);
    
public slots:
    /**
     * @brief 写入数据
     * @param deviceId 设备 ID
     * @param data 遥测数据
     */
    void writeData(const QString& deviceId, const TelemetryData& data);
    
signals:
    /**
     * @brief 记录已开始
     * @param filePath 文件路径
     */
    void recordingStarted(const QString& filePath);
    
    /**
     * @brief 记录已停止
     */
    void recordingStopped();
    
    /**
     * @brief 文件已分割
     * @param oldPath 旧文件路径
     * @param newPath 新文件路径
     */
    void fileSplit(const QString& oldPath, const QString& newPath);
    
    /**
     * @brief 发生错误
     * @param error 错误描述
     */
    void errorOccurred(const QString& error);
    
private:
    std::unique_ptr<QFile> file_;
    std::unique_ptr<QTextStream> stream_;
    QString basePath_;
    QString deviceId_;
    bool recording_ = false;
    int maxFileSize_ = 100;  // MB
    bool autoSplit_ = false;
    int splitInterval_ = 3600000;  // ms
    qint64 bytesWritten_ = 0;
    qint64 lastSplitTime_ = 0;
    
    bool openFile();
    void closeFile();
    QString generateFilePath() const;
    void writeHeader();
    void splitFile();
};

} // namespace HeteroLink
