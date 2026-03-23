/**
 * HeteroLink Host - 日志工具实现
 * 
 * @file Logger.cpp
 */

#include "Logger.h"
#include <QString>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <QByteArray>

namespace HeteroLink {

std::unique_ptr<Logger> Logger::instance_ = nullptr;

Logger::Logger() : verbose_(false) {}

Logger::~Logger() {}

void Logger::installQtMessageHandler()
{
    qInstallMessageHandler([](QtMsgType type, 
                               const QMessageLogContext& context, 
                               const QString& message) {
        Logger::qtMessageHandler(type, context, message);
    });
    LOG_INFO("Qt message handler installed");
}

void Logger::qtMessageHandler(QtMsgType type, 
                              const QMessageLogContext& context, 
                              const QString& message)
{
    // 将 Qt 消息类型映射到 Logger 级别
    Level level = Level::INFO;
    switch (type) {
        case QtDebugMsg:
            level = Level::DEBUG;
            break;
        case QtInfoMsg:
            level = Level::INFO;
            break;
        case QtWarningMsg:
            level = Level::WARNING;
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            level = Level::ERROR;
            break;
    }
    
    // 格式化消息，包含 Qt 上下文信息
    QString formattedMessage = message;
    if (context.file && !QString(context.file).isEmpty()) {
        // 提取文件名
        QString file = QString(context.file);
        int idx = file.lastIndexOf('/');
        if (idx == -1) {
            idx = file.lastIndexOf('\\');
        }
        if (idx != -1) {
            file = file.mid(idx + 1);
        }
        formattedMessage = QString("[%1:%2] %3").arg(file).arg(context.line).arg(message);
    }
    
    // 添加 Qt 前缀标识
    QString qtPrefix;
    switch (type) {
        case QtDebugMsg:   qtPrefix = "[Qt] "; break;
        case QtInfoMsg:    qtPrefix = "[Qt] "; break;
        case QtWarningMsg: qtPrefix = "[Qt-WARN] "; break;
        case QtCriticalMsg: qtPrefix = "[Qt-CRIT] "; break;
        case QtFatalMsg:   qtPrefix = "[Qt-FATAL] "; break;
    }
    
    log(level, qtPrefix + formattedMessage, context.file, context.line);
    
    // 致命错误时终止程序
    if (type == QtFatalMsg) {
        std::abort();
    }
}

void Logger::init(const std::string& logFile)
{
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    instance_->logFile_ = logFile;
    
    if (!logFile.empty()) {
        // 打开日志文件
        std::ofstream file(logFile, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Failed to open log file: " << logFile << std::endl;
        }
    }
}

void Logger::setVerbose(bool verbose)
{
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    instance_->verbose_ = verbose;
}

void Logger::log(Level level, const std::string& message,
                const char* file, int line)
{
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    
    // 跳过 DEBUG 级别（除非启用详细模式）
    if (level == Level::DEBUG && !instance_->verbose_) {
        return;
    }
    
    std::string formattedMsg = instance_->formatMessage(level, message, file, line);
    
    // 输出到控制台
    std::cout << formattedMsg << std::endl;
    
    // 输出到文件（如果配置了）
    if (!instance_->logFile_.empty()) {
        std::ofstream file(instance_->logFile_, std::ios::app);
        if (file.is_open()) {
            file << formattedMsg << std::endl;
        }
    }
}

void Logger::log(Level level, const QString& message,
                const char* file, int line)
{
    log(level, message.toStdString(), file, line);
}

void Logger::log(Level level, const char* message,
                const char* file, int line)
{
    log(level, std::string(message), file, line);
}

std::string Logger::formatMessage(Level level, const std::string& message,
                                 const char* file, int line)
{
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << std::setw(7) << std::left << levelToString(level) << "] ";
    
    // 添加文件和行号（DEBUG 级别）
    if (verbose_ || level == Level::ERROR) {
        // 提取文件名（去掉路径）
        std::string filename(file);
        size_t pos = filename.find_last_of("/\\");
        if (pos != std::string::npos) {
            filename = filename.substr(pos + 1);
        }
        ss << filename << ":" << line << " - ";
    }
    
    ss << message;
    return ss.str();
}

std::string Logger::levelToString(Level level)
{
    switch (level) {
        case Level::DEBUG:   return "DEBUG";
        case Level::INFO:    return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

} // namespace HeteroLink
