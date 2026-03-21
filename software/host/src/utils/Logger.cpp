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

namespace HeteroLink {

std::unique_ptr<Logger> Logger::instance_ = nullptr;

Logger::Logger() : verbose_(false) {}

Logger::~Logger() {}

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
