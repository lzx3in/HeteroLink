/**
 * HeteroLink Host - 配置管理器实现
 */

#include "storage/ConfigManager.h"
#include "utils/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

namespace HeteroLink {

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
}

ConfigManager::~ConfigManager()
{
}

bool ConfigManager::load(const QString& filePath)
{
    QString path = filePath;
    if (path.isEmpty()) {
        // 使用默认配置文件路径
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir);
        path = configDir + "/config.json";
    }
    
    configPath_ = path;
    
    QFile file(path);
    if (!file.exists()) {
        LOG_INFO("Config file not found, using defaults: " + path.toStdString());
        config_ = getDefaultConfig();
        return true;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open config file: " + path.toStdString());
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR("Failed to parse config JSON: " + std::string(error.errorString().toUtf8()));
        config_ = getDefaultConfig();
        return false;
    }
    
    config_ = doc.object().toVariantMap();
    LOG_INFO("Config loaded: " + path.toStdString());
    return true;
}

bool ConfigManager::save(const QString& filePath) const
{
    QString path = filePath;
    if (path.isEmpty()) {
        path = configPath_;
    }
    
    if (path.isEmpty()) {
        LOG_ERROR("No config path specified");
        return false;
    }
    
    QJsonDocument doc(QJsonObject::fromVariantMap(config_));
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open config file for writing: " + path.toStdString());
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    LOG_INFO("Config saved: " + path.toStdString());
    return true;
}

QVariant ConfigManager::get(const QString& key, const QVariant& defaultValue) const
{
    QStringList keys = key.split('.');
    QVariantMap map = config_;
    
    for (int i = 0; i < keys.size() - 1; ++i) {
        if (!map.contains(keys[i])) {
            return defaultValue;
        }
        map = map.value(keys[i]).toMap();
    }
    
    if (!map.contains(keys.last())) {
        return defaultValue;
    }
    
    return map.value(keys.last());
}

void ConfigManager::set(const QString& key, const QVariant& value)
{
    QStringList keys = key.split('.');
    setNested(config_, keys, value);
    emit configChanged(key, value);
}

QVariantMap ConfigManager::getDefaultConfig()
{
    QVariantMap config;
    
    // UART 配置
    QVariantMap uart;
    uart["portName"] = "";
    uart["baudRate"] = 921600;
    config["uart"] = uart;
    
    // MQTT 配置
    QVariantMap mqtt;
    mqtt["brokerHost"] = "localhost";
    mqtt["brokerPort"] = 1883;
    mqtt["username"] = "";
    mqtt["password"] = "";
    mqtt["clientId"] = "heterolink-host";
    mqtt["useTls"] = false;
    config["mqtt"] = mqtt;
    
    // 数据配置
    QVariantMap data;
    data["bufferSize"] = 10000;
    data["autoExport"] = false;
    data["exportPath"] = "";
    config["data"] = data;
    
    // 告警配置
    QVariantMap alarm;
    alarm["enabled"] = true;
    alarm["lowerLimit"] = -1000;
    alarm["upperLimit"] = 1000;
    alarm["level"] = "warning";
    config["alarm"] = alarm;
    
    // UI 配置
    QVariantMap ui;
    ui["theme"] = "dark";
    ui["language"] = "zh-CN";
    config["ui"] = ui;
    
    return config;
}

void ConfigManager::reset()
{
    config_ = getDefaultConfig();
    LOG_INFO("Config reset to defaults");
    emit configChanged("", QVariant());
}

QVariantMap ConfigManager::getNested(const QVariantMap& map, const QStringList& keys) const
{
    QVariantMap result = map;
    for (const auto& key : keys) {
        if (!result.contains(key)) {
            return QVariantMap();
        }
        result = result.value(key).toMap();
    }
    return result;
}

void ConfigManager::setNested(QVariantMap& map, const QStringList& keys, const QVariant& value)
{
    if (keys.isEmpty()) {
        return;
    }
    
    if (keys.size() == 1) {
        map[keys[0]] = value;
        return;
    }
    
    if (!map.contains(keys[0])) {
        map[keys[0]] = QVariantMap();
    }
    
    QVariantMap nested = map[keys[0]].toMap();
    setNested(nested, keys.mid(1), value);
    map[keys[0]] = nested;
}

} // namespace HeteroLink
