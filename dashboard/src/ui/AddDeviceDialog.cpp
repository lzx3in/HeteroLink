/**
 * HeteroLink Host - 添加设备对话框实现
 */

#include "AddDeviceDialog.h"
#include "core/DeviceManager.h"
#include "protocol/UartChannel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

namespace HeteroLink {

AddDeviceDialog::AddDeviceDialog(DeviceManager* manager, QWidget *parent)
    : QDialog(parent)
    , deviceManager_(manager)
{
    setWindowTitle("添加设备");
    setModal(true);
    resize(400, 300);
    
    setupUI();
    loadPorts();
}

AddDeviceDialog::~AddDeviceDialog()
{
}

void AddDeviceDialog::setupUI()
{
    auto layout = new QVBoxLayout(this);
    
    // 设备信息
    auto infoGroup = new QGroupBox("设备信息");
    auto infoLayout = new QFormLayout(infoGroup);
    
    deviceIdEdit_ = new QLineEdit();
    deviceIdEdit_->setPlaceholderText("例如：1");
    deviceIdEdit_->setToolTip("设备 ID，用于协议通信");
    infoLayout->addRow("设备 ID:", deviceIdEdit_);
    
    deviceNameEdit_ = new QLineEdit();
    deviceNameEdit_->setPlaceholderText("例如：ESP32 控制器");
    deviceNameEdit_->setToolTip("设备显示名称");
    infoLayout->addRow("设备名称:", deviceNameEdit_);
    
    layout->addWidget(infoGroup);
    
    // 串口配置
    auto portGroup = new QGroupBox("串口配置");
    auto portLayout = new QFormLayout(portGroup);
    
    portCombo_ = new QComboBox();
    portCombo_->setEditable(false);
    portLayout->addRow("串口:", portCombo_);
    
    baudRateSpin_ = new QSpinBox();
    baudRateSpin_->setRange(1200, 921600);
    baudRateSpin_->setValue(921600);
    baudRateSpin_->setToolTip("波特率");
    portLayout->addRow("波特率:", baudRateSpin_);
    
    layout->addWidget(portGroup);
    
    // 按钮
    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto okBtn = new QPushButton("确定");
    okBtn->setDefault(true);
    connect(okBtn, &QPushButton::clicked, this, [this]() {
        // 验证输入
        if (deviceIdEdit_->text().isEmpty()) {
            QMessageBox::warning(this, "输入错误", "请输入设备 ID");
            return;
        }
        
        if (portCombo_->currentIndex() < 0) {
            QMessageBox::warning(this, "输入错误", "请选择串口");
            return;
        }
        
        accept();
    });
    
    auto cancelBtn = new QPushButton("取消");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void AddDeviceDialog::loadPorts()
{
    portCombo_->clear();
    auto ports = UartChannel::availablePorts();
    for (const auto& port : ports) {
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

QString AddDeviceDialog::deviceId() const
{
    return deviceIdEdit_->text();
}

QString AddDeviceDialog::deviceName() const
{
    QString name = deviceNameEdit_->text();
    return name.isEmpty() ? ("Device " + deviceId()) : name;
}

QString AddDeviceDialog::portName() const
{
    return portCombo_->currentData(Qt::UserRole).toString();
}

int AddDeviceDialog::baudRate() const
{
    return baudRateSpin_->value();
}

} // namespace HeteroLink
