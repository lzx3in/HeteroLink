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
#include <QRadioButton>
#include <QButtonGroup>

namespace HeteroLink {

AddDeviceDialog::AddDeviceDialog(DeviceManager* manager, QWidget *parent)
    : QDialog(parent)
    , deviceManager_(manager)
{
    setWindowTitle("添加设备");
    setModal(true);
    resize(450, 400);
    
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
    
    // 连接类型
    auto connGroup = new QGroupBox("连接方式");
    auto connLayout = new QVBoxLayout(connGroup);
    
    connectionTypeCombo_ = new QComboBox();
    connectionTypeCombo_->addItem("串口 (UART)", "UART");
    connectionTypeCombo_->addItem("MQTT", "MQTT");
    connLayout->addWidget(connectionTypeCombo_);
    
    connect(connectionTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        configStack_->setCurrentIndex(index);
    });
    
    // 配置堆叠
    configStack_ = new QStackedWidget();
    
    // UART 配置页面
    auto uartPage = new QWidget();
    auto uartLayout = new QFormLayout(uartPage);
    
    portCombo_ = new QComboBox();
    portCombo_->setEditable(false);
    uartLayout->addRow("串口:", portCombo_);
    
    baudRateSpin_ = new QSpinBox();
    baudRateSpin_->setRange(1200, 921600);
    baudRateSpin_->setValue(921600);
    baudRateSpin_->setToolTip("波特率");
    uartLayout->addRow("波特率:", baudRateSpin_);
    
    configStack_->addWidget(uartPage);
    
    // MQTT 配置页面
    auto mqttPage = new QWidget();
    auto mqttLayout = new QFormLayout(mqttPage);
    
    brokerHostEdit_ = new QLineEdit();
    brokerHostEdit_->setPlaceholderText("例如：192.168.1.100 或 broker.emqx.io");
    brokerHostEdit_->setText("broker.emqx.io");  // 默认使用 HeteroLink 公共 Broker
    brokerHostEdit_->setToolTip("MQTT Broker 地址，默认使用 HeteroLink 公共 Broker (broker.emqx.io)");
    mqttLayout->addRow("Broker 地址:", brokerHostEdit_);
    
    brokerPortSpin_ = new QSpinBox();
    brokerPortSpin_->setRange(1, 65535);
    brokerPortSpin_->setValue(1883);
    brokerPortSpin_->setToolTip("MQTT Broker 端口");
    mqttLayout->addRow("Broker 端口:", brokerPortSpin_);
    
    configStack_->addWidget(mqttPage);
    
    connLayout->addWidget(configStack_);
    layout->addWidget(connGroup);
    
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
        
        if (connectionTypeCombo_->currentData().toString() == "UART") {
            if (portCombo_->currentIndex() < 0) {
                QMessageBox::warning(this, "输入错误", "请选择串口");
                return;
            }
        } else {
            if (brokerHostEdit_->text().isEmpty()) {
                QMessageBox::warning(this, "输入错误", "请输入 Broker 地址");
                return;
            }
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

QString AddDeviceDialog::connectionType() const
{
    return connectionTypeCombo_->currentData().toString();
}

QString AddDeviceDialog::portName() const
{
    return portCombo_->currentData(Qt::UserRole).toString();
}

int AddDeviceDialog::baudRate() const
{
    return baudRateSpin_->value();
}

QString AddDeviceDialog::brokerHost() const
{
    return brokerHostEdit_->text();
}

int AddDeviceDialog::brokerPort() const
{
    return brokerPortSpin_->value();
}

} // namespace HeteroLink
