/**
 * HeteroLink Host - 应用主类
 * 
 * @file Application.h
 * @brief 应用程序核心逻辑
 */

#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "core/DeviceManager.h"
#include "core/DataProcessor.h"
#include "core/AlarmSystem.h"
#include "protocol/UartChannel.h"
#include "protocol/MqttChannel.h"
#include "storage/ConfigManager.h"
#include "storage/DataLogger.h"

namespace HeteroLink {

/**
 * @brief 应用程序主类
 * 
 * 管理所有核心模块的生命周期和协调
 */
class Application : public QObject
{
    Q_OBJECT
    
public:
    explicit Application(const QString& configFile = "", QObject *parent = nullptr);
    ~Application();
    
    /**
     * @brief 初始化应用
     * @return 是否成功
     */
    bool initialize();
    
    /**
     * @brief 获取设备管理器
     * @return 设备管理器
     */
    DeviceManager* deviceManager() const { return deviceManager_.get(); }
    
    /**
     * @brief 获取数据处理器
     * @return 数据处理器
     */
    DataProcessor* dataProcessor() const { return dataProcessor_.get(); }
    
    /**
     * @brief 获取告警系统
     * @return 告警系统
     */
    AlarmSystem* alarmSystem() const { return alarmSystem_.get(); }
    
    /**
     * @brief 获取配置管理器
     * @return 配置管理器
     */
    ConfigManager* configManager() const { return configManager_.get(); }
    
    /**
     * @brief 获取数据记录器
     * @return 数据记录器
     */
    DataLogger* dataLogger() const { return dataLogger_.get(); }
    
    /**
     * @brief 启动应用
     */
    void start();
    
    /**
     * @brief 停止应用
     */
    void stop();
    
signals:
    /**
     * @brief 应用已启动
     */
    void started();
    
    /**
     * @brief 应用已停止
     */
    void stopped();
    
private:
    std::unique_ptr<DeviceManager> deviceManager_;
    std::unique_ptr<DataProcessor> dataProcessor_;
    std::unique_ptr<AlarmSystem> alarmSystem_;
    std::unique_ptr<ConfigManager> configManager_;
    std::unique_ptr<DataLogger> dataLogger_;
    std::shared_ptr<MqttChannel> mqttChannel_;
    
    QString configFile_;
    bool initialized_ = false;
    
    bool loadConfig();
    bool saveConfig();
    void setupConnections();
};

} // namespace HeteroLink
