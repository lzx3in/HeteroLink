/**
 * HeteroLink Host - 配置面板
 * 
 * @file ConfigPanel.h
 * @brief 参数配置、告警设置
 */

#pragma once

#include <QWidget>
#include <QMap>

namespace HeteroLink {

class AlarmSystem;
class ConfigManager;

/**
 * @brief 配置面板类
 * 
 * UART 配置、MQTT 配置、告警阈值设置
 */
class ConfigPanel : public QWidget
{
    Q_OBJECT
    
public:
    explicit ConfigPanel(QWidget *parent = nullptr);
    ~ConfigPanel();
    
    /**
     * @brief 设置告警系统
     * @param alarmSystem 告警系统
     */
    void setAlarmSystem(AlarmSystem* alarmSystem);
    
    /**
     * @brief 设置配置管理器
     * @param configManager 配置管理器
     */
    void setConfigManager(ConfigManager* configManager);
    
    /**
     * @brief 加载配置
     */
    void loadConfig();
    
    /**
     * @brief 保存配置
     */
    void saveConfig();
    
signals:
    /**
     * @brief 配置已更改
     */
    void configChanged();
    
private slots:
    void onUartBaudRateChanged(int index);
    void onUartPortChanged(int index);
    void onMqttHostChanged(const QString& text);
    void onMqttPortChanged(int value);
    void onAlarmThresholdChanged(double value);
    void onAlarmEnabledChanged(int state);
    void onSaveClicked();
    void onResetClicked();
    
private:
    // UART 配置
    QComboBox *uartPortCombo_;
    QComboBox *uartBaudCombo_;
    
    // MQTT 配置
    QLineEdit *mqttHostEdit_;
    QSpinBox *mqttPortSpin_;
    QLineEdit *mqttUserEdit_;
    QLineEdit *mqttPassEdit_;
    
    // 告警配置
    QDoubleSpinBox *alarmLowerSpin_;
    QDoubleSpinBox *alarmUpperSpin_;
    QCheckBox *alarmLowerCheck_;
    QCheckBox *alarmUpperCheck_;
    QComboBox *alarmLevelCombo_;
    
    // 按钮
    QPushButton *saveBtn_;
    QPushButton *resetBtn_;
    
    AlarmSystem* alarmSystem_ = nullptr;
    ConfigManager* configManager_ = nullptr;
    
    void setupUI();
    void loadUartConfig();
    void loadMqttConfig();
    void loadAlarmConfig();
};

} // namespace HeteroLink
