/**
 * HeteroLink Host - 添加设备对话框
 * 
 * @file AddDeviceDialog.h
 * @brief 用于添加新设备的对话框
 */

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QStackedWidget>

namespace HeteroLink {

class DeviceManager;

/**
 * @brief 添加设备对话框
 */
class AddDeviceDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AddDeviceDialog(DeviceManager* manager, QWidget *parent = nullptr);
    ~AddDeviceDialog();
    
    /**
     * @brief 获取设备 ID
     * @return 设备 ID
     */
    QString deviceId() const;
    
    /**
     * @brief 获取设备名称
     * @return 设备名称
     */
    QString deviceName() const;
    
    /**
     * @brief 获取连接类型
     * @return "UART" 或 "MQTT"
     */
    QString connectionType() const;
    
    /**
     * @brief 获取选中的串口名称（UART 模式）
     * @return 串口名称
     */
    QString portName() const;
    
    /**
     * @brief 获取波特率（UART 模式）
     * @return 波特率
     */
    int baudRate() const;
    
    /**
     * @brief 获取 MQTT Broker 地址
     * @return Broker 地址
     */
    QString brokerHost() const;
    
    /**
     * @brief 获取 MQTT Broker 端口
     * @return Broker 端口
     */
    int brokerPort() const;
    
private:
    QComboBox *connectionTypeCombo_;
    QStackedWidget *configStack_;
    QComboBox *portCombo_;
    QSpinBox *baudRateSpin_;
    QLineEdit *brokerHostEdit_;
    QSpinBox *brokerPortSpin_;
    QLineEdit *deviceIdEdit_;
    QLineEdit *deviceNameEdit_;
    
    DeviceManager* deviceManager_;
    
    void setupUI();
    void loadPorts();
};

} // namespace HeteroLink
