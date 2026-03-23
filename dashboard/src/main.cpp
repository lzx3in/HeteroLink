/**
 * HeteroLink Host - 主程序入口
 * 
 * @file main.cpp
 * @brief 应用程序入口点
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStyleFactory>

#include "app/Application.h"
#include "ui/MainWindow.h"
#include "utils/Logger.h"

int main(int argc, char *argv[])
{
    // 创建 Qt 应用
    QApplication app(argc, argv);
    
    // 设置应用信息
    QApplication::setApplicationName("HeteroLink Host");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("HeteroLink");
    QApplication::setOrganizationDomain("heterolink.dev");
    
    // 设置样式（使用 Fusion 风格，跨平台一致）
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 初始化日志系统
    HeteroLink::Logger::init();
    HeteroLink::Logger::installQtMessageHandler();
    
    LOG_INFO("HeteroLink Host starting...");
    
    // 命令行解析
    QCommandLineParser parser;
    parser.setApplicationDescription("HeteroLink 上位机 - 设备管理与数据可视化平台");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // 自定义选项
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                    "使用指定的配置文件", "file");
    parser.addOption(configOption);
    
    QCommandLineOption verboseOption(QStringList() << "v" << "verbose",
                                     "启用详细日志输出");
    parser.addOption(verboseOption);
    
    QCommandLineOption jsonLogOption(QStringList() << "json-logs",
                                     "使用 JSON 格式输出日志");
    parser.addOption(jsonLogOption);
    
    QCommandLineOption logRotateOption(QStringList() << "log-rotate",
                                       "启用日志轮转，格式：--log-rotate <sizeMB>:<maxFiles>",
                                       "sizeMB:maxFiles");
    parser.addOption(logRotateOption);
    
    QCommandLineOption autoConnectOption(QStringList() << "auto-connect",
                                         "启动时自动连接设备");
    parser.addOption(autoConnectOption);
    
    parser.process(app);
    
    // 处理命令行参数
    QString configFile;
    if (parser.isSet(configOption)) {
        configFile = parser.value(configOption);
        LOG_INFO("Using config file: " + configFile);
    }
    
    if (parser.isSet(verboseOption)) {
        HeteroLink::Logger::setVerbose(true);
        LOG_DEBUG("Verbose logging enabled");
    }
    
    if (parser.isSet(jsonLogOption)) {
        HeteroLink::Logger::setJsonFormat(true);
        // JSON 模式下立即输出一条启动日志（JSON 格式）
        std::cout << "{\"timestamp\":\"" << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() 
                  << "\",\"level\":\"INFO\",\"message\":\"JSON logging enabled\"}" << std::endl;
    }
    
    if (parser.isSet(logRotateOption)) {
        QString value = parser.value(logRotateOption);
        QStringList parts = value.split(":");
        int sizeMB = parts[0].toInt();
        int maxFiles = parts.size() > 1 ? parts[1].toInt() : 5;
        HeteroLink::Logger::setRotationConfig(sizeMB, maxFiles);
        LOG_INFO("Log rotation configured: size=" + QString::number(sizeMB) + 
                 "MB, maxFiles=" + QString::number(maxFiles));
    }
    
    try {
        // 创建应用主类
        HeteroLink::Application application(configFile);
        
        // 创建并显示主窗口
        HeteroLink::MainWindow mainWindow(&application);
        mainWindow.show();
        
        LOG_INFO("Application initialized successfully");
        
        // 运行事件循环
        int result = app.exec();
        
        LOG_INFO("Application exiting...");
        
        return result;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " + QString::fromUtf8(e.what()));
        return -1;
    }
}
