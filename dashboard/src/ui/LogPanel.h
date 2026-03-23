/**
 * HeteroLink Host - 日志查看面板
 * 
 * @file LogPanel.h
 * @brief 实时日志查看器组件
 */

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QQueue>
#include <QTextCharFormat>
#include <QString>

#include "utils/Logger.h"

namespace HeteroLink {

/**
 * @brief 日志查看面板
 * 
 * 显示实时日志流，支持级别过滤、清空、导出
 */
class LogPanel : public QWidget
{
    Q_OBJECT
    
public:
    explicit LogPanel(QWidget *parent = nullptr);
    ~LogPanel();
    
    /**
     * @brief 添加日志条目
     * @param level 日志级别
     * @param message 日志消息
     */
    void addLogEntry(Logger::Level level, const QString& message);
    
    /**
     * @brief 清空日志显示
     */
    void clearLogs();
    
    /**
     * @brief 导出日志到文件
     * @param filePath 文件路径
     * @return 是否成功
     */
    bool exportLogs(const QString& filePath);
    
    /**
     * @brief 设置日志级别过滤
     * @param minLevel 最低显示级别
     */
    void setLevelFilter(Logger::Level minLevel);
    
    /**
     * @brief 设置最大显示行数
     * @param maxLines 最大行数
     */
    void setMaxLines(int maxLines);
    
public slots:
    /**
     * @brief 处理待显示的日志队列
     */
    void processLogQueue();
    
signals:
    /**
     * @brief 日志条目已添加
     * @param count 当前日志总数
     */
    void logCountChanged(int count);
    
private:
    QTextEdit* textEdit_;
    QComboBox* levelFilterCombo_;
    QCheckBox* autoScrollCheck_;
    QToolButton* clearButton_;
    QToolButton* exportButton_;
    
    QQueue<QPair<Logger::Level, QString>> logQueue_;
    QTimer* processTimer_;
    
    Logger::Level minLevel_;
    int maxLines_;
    int logCount_;
    
    QTextCharFormat formatDebug_;
    QTextCharFormat formatInfo_;
    QTextCharFormat formatWarning_;
    QTextCharFormat formatError_;
    
    void setupUI();
    void setupFormats();
    void connectSignals();
    QString levelToString(Logger::Level level);
    QString formatTimestamp();
};

} // namespace HeteroLink
