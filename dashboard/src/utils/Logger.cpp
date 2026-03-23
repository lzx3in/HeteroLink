/**
 * HeteroLink Host - 日志工具实现
 * 
 * @file Logger.cpp
 */

#include "Logger.h"
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <QByteArray>

namespace HeteroLink {

std::unique_ptr<Logger> Logger::instance_ = nullptr;

Logger::Logger() : verbose_(false), jsonFormat_(false), 
                   maxFileSize_(0), maxFiles_(5), currentSize_(0) {}

void Logger::setUiLogCallback(UiLogCallback callback)
{
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    instance_->uiCallback_ = callback;
}

Logger::~Logger() {}

void Logger::setJsonFormat(bool jsonFormat)
{
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    instance_->jsonFormat_ = jsonFormat;
}

void Logger::setRotationConfig(int maxSizeMB, int maxFiles)
{
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    instance_->maxFileSize_ = maxSizeMB > 0 ? maxSizeMB * 1024 * 1024 : 0;
    instance_->maxFiles_ = maxFiles > 0 ? maxFiles : 0;
    
    if (maxSizeMB > 0) {
        LOG_INFO("Log rotation enabled: max size=" + std::to_string(maxSizeMB) + 
                 "MB, max files=" + std::to_string(maxFiles));
    }
}

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
    
    // 调用 UI 回调（如果设置了）
    if (instance_->uiCallback_) {
        instance_->uiCallback_(level, QString::fromStdString(message));
    }
    
    // 输出到控制台
    std::cout << formattedMsg << std::endl;
    
    // 输出到文件（如果配置了）
    if (!instance_->logFile_.empty()) {
        std::ofstream file(instance_->logFile_, std::ios::app);
        if (file.is_open()) {
            file << formattedMsg << std::endl;
            instance_->currentSize_ += formattedMsg.size() + 1; // +1 for newline
            instance_->checkRotation();
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
    if (jsonFormat_) {
        return formatMessageJson(level, message, file, line);
    }
    
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

std::string Logger::formatMessageJson(Level level, const std::string& message,
                                      const char* file, int line)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    // 提取文件名
    std::string filename = file ? file : "";
    size_t pos = filename.find_last_of("/\\");
    if (pos != std::string::npos) {
        filename = filename.substr(pos + 1);
    }
    
    std::stringstream ss;
    ss << "{";
    ss << "\"timestamp\":\"" << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << "Z\",";
    ss << "\"level\":\"" << levelToString(level) << "\",";
    ss << "\"message\":\"" << escapeJson(message) << "\"";
    if (!filename.empty()) {
        ss << ",\"file\":\"" << escapeJson(filename) << "\",";
        ss << "\"line\":" << line;
    }
    ss << "}";
    return ss.str();
}

std::string Logger::escapeJson(const std::string& str)
{
    std::string result;
    result.reserve(str.size() + 10);
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // 控制字符，使用 Unicode 转义
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
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

void Logger::checkRotation()
{
    if (maxFileSize_ <= 0) {
        return; // 轮转禁用
    }
    
    if (currentSize_ >= maxFileSize_) {
        rotateFiles();
    }
}

void Logger::rotateFiles()
{
    if (logFile_.empty()) {
        return;
    }
    
    QString baseFile = QString::fromStdString(logFile_);
    QFileInfo fi(baseFile);
    QString dir = fi.absolutePath();
    QString fileName = fi.fileName();
    
    // 删除最旧的文件（如果达到上限）
    if (maxFiles_ > 0) {
        QString oldestFile = dir + "/" + fileName + "." + QString::number(maxFiles_);
        if (QFile::exists(oldestFile)) {
            QFile::remove(oldestFile);
        }
    }
    
    // 旋转现有文件：.1 -> .2, .2 -> .3, etc.
    if (maxFiles_ > 0) {
        for (int i = maxFiles_ - 1; i >= 1; --i) {
            QString oldPath = dir + "/" + fileName + "." + QString::number(i);
            QString newPath = dir + "/" + fileName + "." + QString::number(i + 1);
            if (QFile::exists(oldPath)) {
                QFile::rename(oldPath, newPath);
            }
        }
    }
    
    // 当前文件 -> .1
    QString rotatedPath = baseFile + ".1";
    if (QFile::exists(baseFile)) {
        QFile::rename(baseFile, rotatedPath);
    }
    
    // 重置当前大小
    currentSize_ = 0;
    
    // 记录轮转事件（输出到控制台）
    std::cerr << "[LOG] Rotated: " << logFile_ << " -> " << rotatedPath.toStdString() << std::endl;
}

} // namespace HeteroLink
