/**
 * HeteroLink Host - 设备面板 UI
 * 
 * @file DevicePanel.h
 * @brief 设备列表和状态显示
 */

#pragma once

#include <QWidget>
#include <QListWidget>
#include <QMap>
#include <QComboBox>
#include <QPushButton>
#include <memory>

#include "core/DeviceManager.h"

namespace HeteroLink {

class DeviceManager;

/**
 * @brief 设备面板类
 * 
 * 显示设备列表、连接状态、快速操作
 */
class DevicePanel : public QWidget
{
    Q_OBJECT
    
public:
    explicit DevicePanel(QWidget *parent = nullptr);
    ~DevicePanel();
    
    /**
     * @brief 设置设备管理器
     * @param manager 设备管理器
     */
    void setDeviceManager(DeviceManager* manager);
    
    /**
     * @brief 刷新设备列表
     */
    void refreshDevices();
    
    /**
     * @brief 刷新串口列表
     */
    void refreshPorts();
    
    /**
     * @brief 获取当前选中的设备 ID
     * @return 设备 ID
     */
    QString getCurrentDeviceId() const;
    
signals:
    /**
     * @brief 请求连接设备
     * @param deviceId 设备 ID
     * @param portName 串口号
     */
    void requestConnect(const QString& deviceId, const QString& portName);
    
    /**
     * @brief 请求断开设备
     * @param deviceId 设备 ID
     */
    void requestDisconnect(const QString& deviceId);
    
private slots:
    void onDevicesChanged(const QMap<QString, DeviceInfo>& devices);
    void onDeviceStatusChanged(const QString& deviceId, bool connected, bool online);
    void onDeviceSelectionChanged();
    void onConnectClicked();
    void onDisconnectClicked();
    void onAddDeviceClicked();
    void onRemoveDeviceClicked();
    void onPortSelected(int index);
    
private:
    void selectPort(const QString& portName);
    
private:
    QListWidget *deviceList_;
    QComboBox *portCombo_;
    QPushButton *connectBtn_;
    QPushButton *disconnectBtn_;
    QPushButton *addBtn_;
    QPushButton *removeBtn_;
    QPushButton *refreshBtn_;
    
    DeviceManager* deviceManager_ = nullptr;
    QMap<QString, DeviceInfo> devices_;
    
    void setupUI();
    void updateDeviceList();
    QIcon getStatusIcon(bool connected, bool online) const;
};

} // namespace HeteroLink
