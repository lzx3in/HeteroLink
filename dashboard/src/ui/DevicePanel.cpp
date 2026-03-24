/**
 * HeteroLink Host - 设备面板实现
 */

#include "DevicePanel.h"
#include "ui/AddDeviceDialog.h"
#include "core/DeviceManager.h"
#include "protocol/UartChannel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>

namespace HeteroLink {

DevicePanel::DevicePanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

DevicePanel::~DevicePanel()
{
}

void DevicePanel::setDeviceManager(DeviceManager* manager)
{
    deviceManager_ = manager;
    if (deviceManager_) {
        connect(deviceManager_, &DeviceManager::devicesChanged,
                this, &DevicePanel::onDevicesChanged);
        connect(deviceManager_, &DeviceManager::deviceStatusChanged,
                this, &DevicePanel::onDeviceStatusChanged);
        refreshDevices();
    }
}

void DevicePanel::refreshDevices()
{
    if (deviceManager_) {
        devices_ = deviceManager_->getDevices();
        updateDeviceList();
    }
}

void DevicePanel::refreshPorts()
{
    portCombo_->clear();
    if (deviceManager_) {
        auto ports = deviceManager_->getAvailablePorts();
        for (const auto& port : ports) {
            // 格式：COM3 - USB Serial Device (FTDI)
            QString displayText = port.portName();
            
            if (!port.description().isEmpty()) {
                displayText += " - " + port.description();
            }
            
            if (!port.manufacturer().isEmpty()) {
                displayText += " (" + port.manufacturer() + ")";
            }
            
            portCombo_->addItem(displayText, port.portName());
        }
    }
}

QString DevicePanel::getCurrentDeviceId() const
{
    if (deviceList_->currentItem()) {
        return deviceList_->currentItem()->data(Qt::UserRole).toString();
    }
    return QString();
}

void DevicePanel::setupUI()
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    
    // 设备列表
    auto deviceGroup = new QGroupBox("设备列表");
    auto deviceLayout = new QVBoxLayout(deviceGroup);
    
    deviceList_ = new QListWidget();
    deviceList_->setSelectionMode(QAbstractItemView::SingleSelection);
    deviceLayout->addWidget(deviceList_);
    
    auto deviceBtnLayout = new QHBoxLayout();
    addBtn_ = new QPushButton("添加");
    removeBtn_ = new QPushButton("移除");
    deviceBtnLayout->addWidget(addBtn_);
    deviceBtnLayout->addWidget(removeBtn_);
    deviceLayout->addLayout(deviceBtnLayout);
    
    layout->addWidget(deviceGroup);
    
    // 连接配置
    auto connectGroup = new QGroupBox("连接配置");
    auto connectLayout = new QVBoxLayout(connectGroup);
    
    // 连接类型标签
    typeLabel_ = new QLabel("类型：UART");
    typeLabel_->setStyleSheet("font-weight: bold;");
    connectLayout->addWidget(typeLabel_);
    
    // UART 配置
    uartWidget_ = new QWidget();
    auto uartLayout = new QVBoxLayout(uartWidget_);
    uartLayout->setContentsMargins(0, 0, 0, 0);
    
    auto portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("串口:"));
    portCombo_ = new QComboBox();
    portCombo_->setEditable(false);
    portLayout->addWidget(portCombo_);
    uartLayout->addLayout(portLayout);
    
    refreshBtn_ = new QPushButton("刷新串口");
    uartLayout->addWidget(refreshBtn_);
    
    connectLayout->addWidget(uartWidget_);
    
    // MQTT 配置
    mqttWidget_ = new QWidget();
    auto mqttLayout = new QVBoxLayout(mqttWidget_);
    mqttLayout->setContentsMargins(0, 0, 0, 0);
    
    mqttInfoLabel_ = new QLabel("MQTT 设备在添加时配置 Broker 信息\n连接时自动使用配置的地址");
    mqttInfoLabel_->setStyleSheet("color: gray; font-style: italic;");
    mqttInfoLabel_->setAlignment(Qt::AlignCenter);
    mqttLayout->addWidget(mqttInfoLabel_);
    
    connectLayout->addWidget(mqttWidget_);
    
    connectBtn_ = new QPushButton("连接");
    connectBtn_->setIcon(QIcon::fromTheme("network-connect"));
    connectLayout->addWidget(connectBtn_);
    
    disconnectBtn_ = new QPushButton("断开");
    disconnectBtn_->setIcon(QIcon::fromTheme("network-disconnect"));
    disconnectBtn_->setEnabled(false);
    connectLayout->addWidget(disconnectBtn_);
    
    layout->addWidget(connectGroup);
    
    layout->addStretch();
    
    // 连接信号
    connect(deviceList_, &QListWidget::currentItemChanged,
            this, &DevicePanel::onDeviceSelectionChanged);
    connect(connectBtn_, &QPushButton::clicked, this, &DevicePanel::onConnectClicked);
    connect(disconnectBtn_, &QPushButton::clicked, this, &DevicePanel::onDisconnectClicked);
    connect(addBtn_, &QPushButton::clicked, this, &DevicePanel::onAddDeviceClicked);
    connect(removeBtn_, &QPushButton::clicked, this, &DevicePanel::onRemoveDeviceClicked);
    connect(refreshBtn_, &QPushButton::clicked, this, &DevicePanel::refreshPorts);
    connect(portCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DevicePanel::onPortSelected);
}

void DevicePanel::updateDeviceList()
{
    deviceList_->clear();
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        const auto& info = it.value();
        QListWidgetItem* item = new QListWidgetItem(info.name);
        item->setIcon(getStatusIcon(info.connected, info.online));
        item->setData(Qt::UserRole, it.key());
        deviceList_->addItem(item);
    }
}

QIcon DevicePanel::getStatusIcon(bool connected, bool online) const
{
    if (connected && online) {
        return QIcon::fromTheme("network-wireless", QIcon(":icons/device_online.svg"));
    } else if (connected) {
        return QIcon::fromTheme("network-wired", QIcon(":icons/device_online.svg"));
    } else {
        return QIcon::fromTheme("network-offline", QIcon(":icons/device_offline.svg"));
    }
}

void DevicePanel::onDevicesChanged(const QMap<QString, DeviceInfo>& devices)
{
    devices_ = devices;
    updateDeviceList();
}

void DevicePanel::onDeviceStatusChanged(const QString& deviceId, bool connected, bool online)
{
    Q_UNUSED(deviceId)
    Q_UNUSED(connected)
    Q_UNUSED(online)
    refreshDevices();
}

void DevicePanel::onDeviceSelectionChanged()
{
    // 当设备选中时，更新连接配置显示
    updateConnectionConfig();
    
    // 同步串口下拉框到该设备保存的串口
    if (deviceList_->currentItem()) {
        QString deviceId = deviceList_->currentItem()->data(Qt::UserRole).toString();
        if (devices_.contains(deviceId) && !devices_[deviceId].port.isEmpty()) {
            if (devices_[deviceId].connectionType == "UART") {
                selectPort(devices_[deviceId].port);
            }
        }
    }
}

void DevicePanel::updateConnectionConfig()
{
    if (!deviceList_->currentItem()) {
        uartWidget_->hide();
        mqttWidget_->hide();
        typeLabel_->setText("类型：-");
        return;
    }
    
    QString deviceId = deviceList_->currentItem()->data(Qt::UserRole).toString();
    if (!devices_.contains(deviceId)) {
        return;
    }
    
    const auto& info = devices_[deviceId];
    
    if (info.connectionType == "UART") {
        uartWidget_->show();
        mqttWidget_->hide();
        typeLabel_->setText("类型：UART (串口)");
    } else {
        uartWidget_->hide();
        mqttWidget_->show();
        typeLabel_->setText("类型：MQTT");
    }
}

void DevicePanel::selectPort(const QString& portName)
{
    for (int i = 0; i < portCombo_->count(); ++i) {
        if (portCombo_->itemData(i).toString() == portName) {
            portCombo_->setCurrentIndex(i);
            return;
        }
    }
}

void DevicePanel::onConnectClicked()
{
    if (!deviceList_->currentItem()) {
        return;
    }
    
    QString deviceId = deviceList_->currentItem()->data(Qt::UserRole).toString();
    if (!devices_.contains(deviceId)) {
        return;
    }
    
    const auto& info = devices_[deviceId];
    
    if (info.connectionType == "UART") {
        // UART 连接
        if (portCombo_->currentIndex() < 0) {
            QMessageBox::warning(this, "提示", "请选择串口");
            return;
        }
        
        QString portName = portCombo_->currentData(Qt::UserRole).toString();
        
        // 更新设备保存的串口
        devices_[deviceId].port = portName;
        deviceManager_->updateDevice(devices_[deviceId]);
        
        emit requestConnect(deviceId, portName);
    } else {
        // MQTT 连接
        QString brokerHost = info.metadata["mqtt_broker_host"].toString();
        int brokerPort = info.metadata["mqtt_broker_port"].toInt();
        
        if (brokerHost.isEmpty()) {
            QMessageBox::warning(this, "错误", "设备未配置 MQTT Broker 信息");
            return;
        }
        
        emit requestConnectMqtt(deviceId, brokerHost, brokerPort);
    }
}

void DevicePanel::onDisconnectClicked()
{
    if (deviceList_->currentItem()) {
        QString deviceId = deviceList_->currentItem()->data(Qt::UserRole).toString();
        emit requestDisconnect(deviceId);
    }
}

void DevicePanel::onAddDeviceClicked()
{
    if (!deviceManager_) {
        QMessageBox::warning(this, "错误", "设备管理器未初始化");
        return;
    }
    
    AddDeviceDialog dialog(deviceManager_, this);
    if (dialog.exec() == QDialog::Accepted) {
        // 创建设备信息
        DeviceInfo info;
        info.id = dialog.deviceId();
        info.name = dialog.deviceName();
        info.connectionType = dialog.connectionType();
        info.connected = false;
        info.online = false;
        info.lastSeen = 0;
        
        if (dialog.connectionType() == "UART") {
            info.port = dialog.portName();
            info.baudRate = dialog.baudRate();
        } else {
            info.port = dialog.brokerHost() + ":" + QString::number(dialog.brokerPort());
            info.metadata["mqtt_broker_host"] = dialog.brokerHost();
            info.metadata["mqtt_broker_port"] = dialog.brokerPort();
        }
        
        // 添加设备
        if (deviceManager_->addDevice(info)) {
            // 添加成功后，自动选中该设备
            refreshDevices();
            
            // 如果是 UART 模式，刷新串口列表并选中
            if (info.connectionType == "UART") {
                refreshPorts();
                selectPort(info.port);
            }
        } else {
            QMessageBox::warning(this, "添加失败", "设备 ID 已存在");
        }
    }
}

void DevicePanel::onRemoveDeviceClicked()
{
    if (deviceList_->currentItem()) {
        QString deviceId = deviceList_->currentItem()->data(Qt::UserRole).toString();
        if (deviceManager_) {
            deviceManager_->removeDevice(deviceId);
        }
    }
}

void DevicePanel::onPortSelected(int index)
{
    Q_UNUSED(index)
    connectBtn_->setEnabled(!portCombo_->currentText().isEmpty());
}

} // namespace HeteroLink
