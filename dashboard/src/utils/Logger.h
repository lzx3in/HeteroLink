/**
 * HeteroLink Host - 日志工具类
 * 
 * @file Logger.h
 * @brief 统一的日志记录接口
 */

#pragma once

#include <string>
#include <memory>
#include <QString>
#include <QtGlobal>

namespace HeteroLink {

class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
    /**
     * @brief 初始化日志系统
     * @param logFile 日志文件路径（可选，默认为控制台输出）
     */
    static void init(const std::string& logFile = "");
    
    /**
     * @brief 设置详细日志模式
     * @param verbose true 启用 DEBUG 级别日志
     */
    static void setVerbose(bool verbose);
    
    /**
     * @brief 设置 JSON 格式输出
     * @param jsonFormat true 启用 JSON 格式日志
     */
    static void setJsonFormat(bool jsonFormat);
    
    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源文件行号
     */
    static void log(Level level, const std::string& message, 
                   const char* file, int line);
    static void log(Level level, const QString& message, 
                   const char* file, int line);
    static void log(Level level, const char* message, 
                   const char* file, int line);
    
    /**
     * @brief 安装 Qt 内部日志处理器
     * @brief 捕获 Qt 框架的警告、错误、调试信息
     */
    static void installQtMessageHandler();
    
    /**
     * @brief Qt 消息处理器回调（静态）
     * @param type Qt 消息类型
     * @param context 消息上下文
     * @param message 消息内容
     */
    static void qtMessageHandler(QtMsgType type, 
                                 const QMessageLogContext& context, 
                                 const QString& message);
    
private:
    static std::unique_ptr<Logger> instance_;
    bool verbose_;
    bool jsonFormat_;
    std::string logFile_;
    
    Logger();
    
    std::string formatMessage(Level level, const std::string& message,
                             const char* file, int line);
    std::string formatMessageJson(Level level, const std::string& message,
                                  const char* file, int line);
    std::string levelToString(Level level);
    std::string escapeJson(const std::string& str);
    
public:
    ~Logger();  // 需要公有以便 unique_ptr 删除
};

// 日志宏定义
#define LOG_DEBUG(msg) \
    HeteroLink::Logger::log(HeteroLink::Logger::Level::DEBUG, msg, __FILE__, __LINE__)

#define LOG_INFO(msg) \
    HeteroLink::Logger::log(HeteroLink::Logger::Level::INFO, msg, __FILE__, __LINE__)

#define LOG_WARNING(msg) \
    HeteroLink::Logger::log(HeteroLink::Logger::Level::WARNING, msg, __FILE__, __LINE__)

#define LOG_ERROR(msg) \
    HeteroLink::Logger::log(HeteroLink::Logger::Level::ERROR, msg, __FILE__, __LINE__)

} // namespace HeteroLink
