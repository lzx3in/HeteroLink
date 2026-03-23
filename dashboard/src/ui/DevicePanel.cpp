/**
 * HeteroLink Host - 设备面板实现
 */

#include "DevicePanel.h"
#include "core/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QGroupBox>

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
            portCombo_->addItem(port.portName());
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
    
    auto portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("串口:"));
    portCombo_ = new QComboBox();
    portCombo_->setEditable(false);
    portLayout->addWidget(portCombo_);
    connectLayout->addLayout(portLayout);
    
    connectBtn_ = new QPushButton("连接");
    connectBtn_->setIcon(QIcon::fromTheme("network-connect"));
    connectLayout->addWidget(connectBtn_);
    
    disconnectBtn_ = new QPushButton("断开");
    disconnectBtn_->setIcon(QIcon::fromTheme("network-disconnect"));
    disconnectBtn_->setEnabled(false);
    connectLayout->addWidget(disconnectBtn_);
    
    refreshBtn_ = new QPushButton("刷新串口");
    connectLayout->addWidget(refreshBtn_);
    
    layout->addWidget(connectGroup);
    
    layout->addStretch();
    
    // 连接信号
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

void DevicePanel::onConnectClicked()
{
    if (deviceList_->currentItem() && !portCombo_->currentText().isEmpty()) {
        QString deviceId = deviceList_->currentItem()->data(Qt::UserRole).toString();
        QString portName = portCombo_->currentText();
        emit requestConnect(deviceId, portName);
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
    // TODO: 打开添加设备对话框
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
