/**
 * HeteroLink Host - 应用主类实现
 */

#include "Application.h"
#include "utils/Logger.h"

namespace HeteroLink {

Application::Application(const QString& configFile, QObject *parent)
    : QObject(parent)
    , configFile_(configFile)
{
    LOG_INFO("Creating Application instance");
}

Application::~Application()
{
    stop();
    LOG_INFO("Application destroyed");
}

bool Application::initialize()
{
    if (initialized_) {
        return true;
    }
    
    LOG_INFO("Initializing Application...");
    
    // 创建核心模块
    deviceManager_ = std::make_unique<DeviceManager>();
    dataProcessor_ = std::make_unique<DataProcessor>();
    alarmSystem_ = std::make_unique<AlarmSystem>();
    configManager_ = std::make_unique<ConfigManager>();
    dataLogger_ = std::make_unique<DataLogger>();
    mqttChannel_ = std::make_shared<MqttChannel>();
    
    // 加载配置
    if (!loadConfig()) {
        LOG_WARNING("Failed to load config, using defaults");
    }
    
    // 设置模块间连接
    setupConnections();
    
    initialized_ = true;
    LOG_INFO("Application initialized successfully");
    
    return true;
}

void Application::start()
{
    if (!initialized_) {
        LOG_ERROR("Application not initialized");
        return;
    }
    
    LOG_INFO("Starting Application...");
    emit started();
}

void Application::stop()
{
    if (!initialized_) {
        return;
    }
    
    LOG_INFO("Stopping Application...");
    
    // 停止数据记录
    if (dataLogger_) {
        dataLogger_->stopRecording();
    }
    
    // 断开所有设备
    if (deviceManager_) {
        auto devices = deviceManager_->getDevices();
        for (auto it = devices.begin(); it != devices.end(); ++it) {
            deviceManager_->disconnectDevice(it.key());
        }
    }
    
    initialized_ = false;
    emit stopped();
    
    LOG_INFO("Application stopped");
}

bool Application::loadConfig()
{
    if (configManager_) {
        return configManager_->load(configFile_);
    }
    return false;
}

bool Application::saveConfig()
{
    if (configManager_) {
        return configManager_->save(configFile_);
    }
    return false;
}

void Application::setupConnections()
{
    // 设备管理器 -> 数据处理器
    connect(deviceManager_.get(), &DeviceManager::telemetryReceived,
            dataProcessor_.get(), &DataProcessor::addData);
    
    // 设备管理器 -> 告警系统
    connect(deviceManager_.get(), &DeviceManager::telemetryReceived,
            alarmSystem_.get(), &AlarmSystem::checkData);
    
    // 数据记录器 -> 数据处理器
    connect(deviceManager_.get(), &DeviceManager::telemetryReceived,
            dataLogger_.get(), &DataLogger::writeData);
    
    // 设置 MQTT 通道
    deviceManager_->setMqttChannel(mqttChannel_);
}

} // namespace HeteroLink
