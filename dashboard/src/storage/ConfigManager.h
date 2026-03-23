/**
 * HeteroLink Host - 配置管理器
 * 
 * @file ConfigManager.h
 * @brief 应用配置管理
 */

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace HeteroLink {

/**
 * @brief 配置管理器类
 * 
 * 管理应用配置（JSON 格式）
 */
class ConfigManager : public QObject
{
    Q_OBJECT
    
public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();
    
    /**
     * @brief 加载配置
     * @param filePath 配置文件路径
     * @return 是否成功
     */
    bool load(const QString& filePath = "");
    
    /**
     * @brief 保存配置
     * @param filePath 配置文件路径
     * @return 是否成功
     */
    bool save(const QString& filePath = "") const;
    
    /**
     * @brief 获取配置值
     * @param key 配置键（支持点分隔，如 "uart.baudRate"）
     * @param defaultValue 默认值
     * @return 配置值
     */
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;
    
    /**
     * @brief 设置配置值
     * @param key 配置键
     * @param value 配置值
     */
    void set(const QString& key, const QVariant& value);
    
    /**
     * @brief 获取默认配置
     * @return 默认配置
     */
    static QVariantMap getDefaultConfig();
    
    /**
     * @brief 重置为默认配置
     */
    void reset();
    
signals:
    /**
     * @brief 配置已更改
     * @param key 更改的配置键
     * @param value 新值
     */
    void configChanged(const QString& key, const QVariant& value);
    
private:
    QVariantMap config_;
    QString configPath_;
    
    QVariantMap getNested(const QVariantMap& map, const QStringList& keys) const;
    void setNested(QVariantMap& map, const QStringList& keys, const QVariant& value);
};

} // namespace HeteroLink
