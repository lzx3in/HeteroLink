/**
 * HeteroLink Host - 配置面板实现
 */

#include "ui/ConfigPanel.h"
#include "core/AlarmSystem.h"
#include "storage/ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>

namespace HeteroLink {

ConfigPanel::ConfigPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

ConfigPanel::~ConfigPanel()
{
}

void ConfigPanel::setAlarmSystem(AlarmSystem* alarmSystem)
{
    alarmSystem_ = alarmSystem;
}

void ConfigPanel::setConfigManager(ConfigManager* configManager)
{
    configManager_ = configManager;
    if (configManager_) {
        loadConfig();
    }
}

void ConfigPanel::loadConfig()
{
    if (!configManager_) {
        return;
    }
    
    loadUartConfig();
    loadMqttConfig();
    loadAlarmConfig();
}

void ConfigPanel::saveConfig()
{
    if (!configManager_) {
        return;
    }
    
    // 保存 UART 配置
    configManager_->set("uart.portName", uartPortCombo_->currentText());
    configManager_->set("uart.baudRate", uartBaudCombo_->currentData().toInt());
    
    // 保存 MQTT 配置
    configManager_->set("mqtt.brokerHost", mqttHostEdit_->text());
    configManager_->set("mqtt.brokerPort", mqttPortSpin_->value());
    configManager_->set("mqtt.clientId", mqttClientIdEdit_->text());
    configManager_->set("mqtt.username", mqttUserEdit_->text());
    configManager_->set("mqtt.password", mqttPassEdit_->text());
    configManager_->set("mqtt.useTls", mqttTlsCheck_->isChecked());
    configManager_->set("mqtt.willEnabled", mqttWillCheck_->isChecked());
    configManager_->set("mqtt.willTopic", mqttWillTopicEdit_->text());
    configManager_->set("mqtt.willMessage", mqttWillMessageEdit_->text());
    
    // 保存告警配置
    configManager_->set("alarm.lowerLimit", alarmLowerSpin_->value());
    configManager_->set("alarm.upperLimit", alarmUpperSpin_->value());
    configManager_->set("alarm.lowerEnabled", alarmLowerCheck_->isChecked());
    configManager_->set("alarm.upperEnabled", alarmUpperCheck_->isChecked());
    configManager_->set("alarm.level", alarmLevelCombo_->currentData().toString());
    
    configManager_->save();
    emit configChanged();
}

void ConfigPanel::setupUI()
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    
    // 使用 Tab 组织配置
    auto tabs = new QTabWidget();
    
    // UART 配置页
    auto uartTab = createUartTab();
    tabs->addTab(uartTab, "UART 配置");
    
    // MQTT 配置页
    auto mqttTab = createMqttTab();
    tabs->addTab(mqttTab, "MQTT 配置");
    
    // 告警配置页
    auto alarmTab = createAlarmTab();
    tabs->addTab(alarmTab, "告警配置");
    
    layout->addWidget(tabs);
    
    // 底部按钮
    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    resetBtn_ = new QPushButton("重置");
    resetBtn_->setIcon(QIcon::fromTheme("edit-undo"));
    btnLayout->addWidget(resetBtn_);
    
    saveBtn_ = new QPushButton("保存");
    saveBtn_->setIcon(QIcon::fromTheme("document-save"));
    saveBtn_->setDefault(true);
    btnLayout->addWidget(saveBtn_);
    
    layout->addLayout(btnLayout);
    
    // 连接信号
    connect(saveBtn_, &QPushButton::clicked, this, &ConfigPanel::onSaveClicked);
    connect(resetBtn_, &QPushButton::clicked, this, &ConfigPanel::onResetClicked);
}

QWidget* ConfigPanel::createUartTab()
{
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    
    auto form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setSpacing(12);
    
    // 串口号
    uartPortCombo_ = new QComboBox();
    uartPortCombo_->setEditable(true);
    // TODO: 加载可用串口列表
    uartPortCombo_->addItem("COM1");
    uartPortCombo_->addItem("COM2");
    uartPortCombo_->addItem("COM3");
    form->addRow("串口号:", uartPortCombo_);
    
    // 波特率
    uartBaudCombo_ = new QComboBox();
    uartBaudCombo_->addItem("9600", 9600);
    uartBaudCombo_->addItem("19200", 19200);
    uartBaudCombo_->addItem("38400", 38400);
    uartBaudCombo_->addItem("57600", 57600);
    uartBaudCombo_->addItem("115200", 115200);
    uartBaudCombo_->addItem("230400", 230400);
    uartBaudCombo_->addItem("460800", 460800);
    uartBaudCombo_->addItem("921600", 921600);
    uartBaudCombo_->setCurrentText("921600");
    form->addRow("波特率:", uartBaudCombo_);
    
    layout->addLayout(form);
    layout->addStretch();
    
    connect(uartBaudCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigPanel::onUartBaudRateChanged);
    connect(uartPortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigPanel::onUartPortChanged);
    
    return widget;
}

QWidget* ConfigPanel::createMqttTab()
{
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    
    auto form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setSpacing(12);
    
    // Broker 地址
    mqttHostEdit_ = new QLineEdit("localhost");
    mqttHostEdit_->setPlaceholderText("例如：broker.emqx.io");
    form->addRow("Broker 地址:", mqttHostEdit_);
    
    // 端口
    mqttPortSpin_ = new QSpinBox();
    mqttPortSpin_->setRange(1, 65535);
    mqttPortSpin_->setValue(1883);
    form->addRow("端口:", mqttPortSpin_);
    
    // 客户端 ID
    mqttClientIdEdit_ = new QLineEdit();
    mqttClientIdEdit_->setPlaceholderText("留空则自动生成");
    form->addRow("客户端 ID:", mqttClientIdEdit_);
    
    // 用户名
    mqttUserEdit_ = new QLineEdit();
    mqttUserEdit_->setPlaceholderText("可选");
    form->addRow("用户名:", mqttUserEdit_);
    
    // 密码
    mqttPassEdit_ = new QLineEdit();
    mqttPassEdit_->setEchoMode(QLineEdit::Password);
    mqttPassEdit_->setPlaceholderText("可选");
    form->addRow("密码:", mqttPassEdit_);
    
    // TLS 加密
    mqttTlsCheck_ = new QCheckBox("启用 TLS 加密");
    form->addRow("", mqttTlsCheck_);
    
    // Last Will 配置
    mqttWillCheck_ = new QCheckBox("启用 Last Will 遗嘱消息");
    form->addRow("", mqttWillCheck_);
    
    mqttWillTopicEdit_ = new QLineEdit();
    mqttWillTopicEdit_->setPlaceholderText("例如：heterolink/subboard/status");
    form->addRow("Will Topic:", mqttWillTopicEdit_);
    
    mqttWillMessageEdit_ = new QLineEdit("offline");
    mqttWillMessageEdit_->setPlaceholderText("遗嘱消息内容");
    form->addRow("Will Message:", mqttWillMessageEdit_);
    
    // 添加说明文本
    auto label = new QLabel("💡 提示：MQTT 用于远端通道，支持设备集群管理和云端连接。");
    label->setWordWrap(true);
    label->setStyleSheet("color: #888; font-size: 12px;");
    layout->addWidget(label);
    
    layout->addLayout(form);
    layout->addStretch();
    
    connect(mqttHostEdit_, &QLineEdit::textChanged,
            this, &ConfigPanel::onMqttHostChanged);
    connect(mqttPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConfigPanel::onMqttPortChanged);
    connect(mqttWillCheck_, &QCheckBox::stateChanged, this, [this](int state) {
        mqttWillTopicEdit_->setEnabled(state == Qt::Checked);
        mqttWillMessageEdit_->setEnabled(state == Qt::Checked);
    });
    
    // 初始化 Will 编辑框状态
    mqttWillTopicEdit_->setEnabled(false);
    mqttWillMessageEdit_->setEnabled(false);
    
    return widget;
}

QWidget* ConfigPanel::createAlarmTab()
{
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    
    auto form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setSpacing(12);
    
    // 下限
    alarmLowerSpin_ = new QDoubleSpinBox();
    alarmLowerSpin_->setRange(-1000000, 1000000);
    alarmLowerSpin_->setDecimals(4);
    alarmLowerSpin_->setValue(-1000);
    form->addRow("下限:", alarmLowerSpin_);
    
    // 启用下限
    alarmLowerCheck_ = new QCheckBox("启用下限告警");
    form->addRow("", alarmLowerCheck_);
    
    // 上限
    alarmUpperSpin_ = new QDoubleSpinBox();
    alarmUpperSpin_->setRange(-1000000, 1000000);
    alarmUpperSpin_->setDecimals(4);
    alarmUpperSpin_->setValue(1000);
    form->addRow("上限:", alarmUpperSpin_);
    
    // 启用上限
    alarmUpperCheck_ = new QCheckBox("启用上限告警");
    alarmUpperCheck_->setChecked(true);
    form->addRow("", alarmUpperCheck_);
    
    // 告警级别
    alarmLevelCombo_ = new QComboBox();
    alarmLevelCombo_->addItem("信息", "info");
    alarmLevelCombo_->addItem("警告", "warning");
    alarmLevelCombo_->addItem("严重", "critical");
    alarmLevelCombo_->setCurrentIndex(1);
    form->addRow("告警级别:", alarmLevelCombo_);
    
    layout->addLayout(form);
    layout->addStretch();
    
    connect(alarmLowerSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigPanel::onAlarmThresholdChanged);
    connect(alarmUpperSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigPanel::onAlarmThresholdChanged);
    connect(alarmLowerCheck_, &QCheckBox::stateChanged,
            this, &ConfigPanel::onAlarmEnabledChanged);
    connect(alarmUpperCheck_, &QCheckBox::stateChanged,
            this, &ConfigPanel::onAlarmEnabledChanged);
    
    return widget;
}

void ConfigPanel::loadUartConfig()
{
    if (!configManager_) {
        return;
    }
    
    QString portName = configManager_->get("uart.portName", "").toString();
    int baudRate = configManager_->get("uart.baudRate", 921600).toInt();
    
    if (!portName.isEmpty()) {
        int idx = uartPortCombo_->findText(portName);
        if (idx >= 0) {
            uartPortCombo_->setCurrentIndex(idx);
        }
    }
    
    int baudIdx = uartBaudCombo_->findData(baudRate);
    if (baudIdx >= 0) {
        uartBaudCombo_->setCurrentIndex(baudIdx);
    }
}

void ConfigPanel::loadMqttConfig()
{
    if (!configManager_) {
        return;
    }
    
    mqttHostEdit_->setText(configManager_->get("mqtt.brokerHost", "localhost").toString());
    mqttPortSpin_->setValue(configManager_->get("mqtt.brokerPort", 1883).toInt());
    mqttClientIdEdit_->setText(configManager_->get("mqtt.clientId", "").toString());
    mqttUserEdit_->setText(configManager_->get("mqtt.username", "").toString());
    mqttPassEdit_->setText(configManager_->get("mqtt.password", "").toString());
    mqttTlsCheck_->setChecked(configManager_->get("mqtt.useTls", false).toBool());
    mqttWillCheck_->setChecked(configManager_->get("mqtt.willEnabled", false).toBool());
    mqttWillTopicEdit_->setText(configManager_->get("mqtt.willTopic", "").toString());
    mqttWillMessageEdit_->setText(configManager_->get("mqtt.willMessage", "offline").toString());
    
    // 初始化 Will 编辑框状态
    mqttWillTopicEdit_->setEnabled(mqttWillCheck_->isChecked());
    mqttWillMessageEdit_->setEnabled(mqttWillCheck_->isChecked());
}

void ConfigPanel::loadAlarmConfig()
{
    if (!configManager_) {
        return;
    }
    
    alarmLowerSpin_->setValue(configManager_->get("alarm.lowerLimit", -1000).toDouble());
    alarmUpperSpin_->setValue(configManager_->get("alarm.upperLimit", 1000).toDouble());
    alarmLowerCheck_->setChecked(configManager_->get("alarm.lowerEnabled", false).toBool());
    alarmUpperCheck_->setChecked(configManager_->get("alarm.upperEnabled", true).toBool());
    
    QString level = configManager_->get("alarm.level", "warning").toString();
    int idx = alarmLevelCombo_->findData(level);
    if (idx >= 0) {
        alarmLevelCombo_->setCurrentIndex(idx);
    }
}

void ConfigPanel::onUartBaudRateChanged(int index)
{
    Q_UNUSED(index)
    emit configChanged();
}

void ConfigPanel::onUartPortChanged(int index)
{
    Q_UNUSED(index)
    emit configChanged();
}

void ConfigPanel::onMqttHostChanged(const QString& text)
{
    Q_UNUSED(text)
    emit configChanged();
}

void ConfigPanel::onMqttPortChanged(int value)
{
    Q_UNUSED(value)
    emit configChanged();
}

void ConfigPanel::onAlarmThresholdChanged(double value)
{
    Q_UNUSED(value)
    emit configChanged();
}

void ConfigPanel::onAlarmEnabledChanged(int state)
{
    Q_UNUSED(state)
    emit configChanged();
}

void ConfigPanel::onSaveClicked()
{
    saveConfig();
}

void ConfigPanel::onResetClicked()
{
    if (configManager_) {
        configManager_->reset();
        loadConfig();
        emit configChanged();
    }
}

} // namespace HeteroLink
