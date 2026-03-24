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
     * @brief 获取选中的串口名称
     * @return 串口名称
     */
    QString portName() const;
    
    /**
     * @brief 获取波特率
     * @return 波特率
     */
    int baudRate() const;
    
private:
    QComboBox *portCombo_;
    QLineEdit *deviceIdEdit_;
    QLineEdit *deviceNameEdit_;
    QSpinBox *baudRateSpin_;
    
    DeviceManager* deviceManager_;
    
    void setupUI();
    void loadPorts();
};

} // namespace HeteroLink
