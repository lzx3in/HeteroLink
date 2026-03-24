/**
 * HeteroLink Host - 主窗口实现
 */

#include "ui_MainWindow.h"
#include "ui/MainWindow.h"
#include "ui/DevicePanel.h"
#include "ui/DataWidget.h"
#include "ui/ConfigPanel.h"
#include "ui/LogPanel.h"
#include "app/Application.h"
#include "utils/Logger.h"
#include <QMessageBox>
#include <QStatusBar>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QDockWidget>

namespace HeteroLink {

MainWindow::MainWindow(Application *app, QWidget *parent)
    : QMainWindow(parent)
    , application_(app)
{
    ui = new Ui::MainWindow();
    ui->setupUi(this);
    
    setupUI();
    setupLogPanel();
    setupConnections();
    
    LOG_INFO("Main window created");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    setWindowTitle("HeteroLink Host - 异构协处理器监控平台");
    setMinimumSize(1200, 800);
    
    // 创建设备面板
    devicePanel_ = new DevicePanel(this);
    devicePanel_->setMaximumWidth(300);
    devicePanel_->setDeviceManager(application_->deviceManager());
    
    // 创建数据可视化组件
    dataWidget_ = new DataWidget(this);
    dataWidget_->setDataProcessor(application_->dataProcessor());
    
    // 创建配置面板
    configPanel_ = new ConfigPanel(this);
    configPanel_->setAlarmSystem(application_->alarmSystem());
    configPanel_->setConfigManager(application_->configManager());
    
    // 设置中央部件（使用水平布局）
    auto centralWidget = new QWidget();
    auto layout = new QHBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    layout->addWidget(devicePanel_);
    layout->addWidget(dataWidget_, 1);  // 数据组件占据剩余空间
    
    setCentralWidget(centralWidget);
    
    // 创建状态栏
    auto statusLabel = new QLabel("就绪");
    statusBar()->addWidget(statusLabel, 1);
    
    auto connLabel = new QLabel("未连接");
    statusBar()->addPermanentWidget(connLabel);
    
    // 连接菜单动作
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::on_actionConnect_triggered);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::on_actionDisconnect_triggered);
    connect(ui->actionStart, &QAction::triggered, this, &MainWindow::on_actionStart_triggered);
    connect(ui->actionStop, &QAction::triggered, this, &MainWindow::on_actionStop_triggered);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::on_actionExport_triggered);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::on_actionSettings_triggered);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::on_actionAbout_triggered);
}

void MainWindow::setupLogPanel()
{
    // 创建日志面板
    logPanel_ = new LogPanel(this);
    
    // 创建停靠窗口
    logDock_ = new QDockWidget("运行日志", this);
    logDock_->setWidget(logPanel_);
    logDock_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, logDock_);
    
    // 设置 Logger 的 UI 回调
    Logger::setUiLogCallback([this](Logger::Level level, const QString& message) {
        logPanel_->addLogEntry(level, message);
    });
    
    LOG_INFO("Log panel initialized");
}

void MainWindow::setupConnections()
{
    // 连接设备管理器信号
    connect(application_->deviceManager(), &DeviceManager::deviceStatusChanged,
            this, &MainWindow::onDeviceStatusChanged);
    connect(application_->deviceManager(), &DeviceManager::telemetryReceived,
            this, &MainWindow::onTelemetryReceived);
    
    // 连接告警系统信号
    connect(application_->alarmSystem(), &AlarmSystem::alarmTriggered,
            this, &MainWindow::onAlarmTriggered);
    
    // 连接设备面板信号
    connect(devicePanel_, &DevicePanel::requestConnect,
            this, [this](const QString& deviceId, const QString& portName) {
        UartConfig config;
        config.portName = portName;
        config.baudRate = 921600;
        application_->deviceManager()->connectDevice(deviceId, config);
    });
    
    connect(devicePanel_, &DevicePanel::requestConnectMqtt,
            this, [this](const QString& deviceId, const QString& brokerHost, int brokerPort) {
        application_->deviceManager()->connectDeviceMqtt(deviceId, brokerHost, brokerPort);
    });
    
    // 初始化时刷新串口列表
    devicePanel_->refreshPorts();
    
    connect(devicePanel_, &DevicePanel::requestDisconnect,
            this, [this](const QString& deviceId) {
        application_->deviceManager()->disconnectDevice(deviceId);
    });
}

void MainWindow::updateStatusBar()
{
    auto devices = application_->deviceManager()->getDevices();
    int connected = 0;
    int online = 0;
    
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        if (it.value().connected) connected++;
        if (it.value().online) online++;
    }
    
    QString status = QString("设备：%1 已连接，%2 在线").arg(connected).arg(online);
    statusBar()->findChild<QLabel*>()->setText(status);
}

void MainWindow::showAlarmNotification(const AlarmRecord& record)
{
    QString title;
    QMessageBox::Icon icon;
    
    switch (record.level) {
        case AlarmLevel::INFO:
            title = "信息";
            icon = QMessageBox::Icon::Information;
            break;
        case AlarmLevel::WARNING:
            title = "警告";
            icon = QMessageBox::Icon::Warning;
            break;
        case AlarmLevel::CRITICAL:
            title = "严重";
            icon = QMessageBox::Icon::Critical;
            break;
    }
    
    QString message = QString("设备 %1 通道 %2\n%3\n当前值：%4")
                         .arg(record.deviceId)
                         .arg(record.channelId)
                         .arg(record.message)
                         .arg(record.value, 0, 'f', 2);
    
    // 使用系统托盘通知（如果可用）
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QSystemTrayIcon* tray = findChild<QSystemTrayIcon*>();
        if (tray) {
            tray->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
        }
    }
    
    // 同时在状态栏显示
    statusBar()->showMessage(message, 10000);
}

void MainWindow::on_actionConnect_triggered()
{
    devicePanel_->refreshPorts();
}

void MainWindow::on_actionDisconnect_triggered()
{
    QString deviceId = devicePanel_->getCurrentDeviceId();
    if (!deviceId.isEmpty()) {
        application_->deviceManager()->disconnectDevice(deviceId);
    }
}

void MainWindow::on_actionStart_triggered()
{
    application_->start();
    statusBar()->showMessage("数据采集已启动", 3000);
}

void MainWindow::on_actionStop_triggered()
{
    application_->stop();
    statusBar()->showMessage("数据采集已停止", 3000);
}

void MainWindow::on_actionExport_triggered()
{
    QString deviceId = devicePanel_->getCurrentDeviceId();
    if (!deviceId.isEmpty()) {
        application_->dataProcessor()->exportToCsv(deviceId, "~/heterolink_export.csv");
        statusBar()->showMessage("数据已导出", 3000);
    }
}

void MainWindow::on_actionSettings_triggered()
{
    configPanel_->loadConfig();
    // TODO: 显示配置对话框
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "关于 HeteroLink",
        "<h2>HeteroLink Host</h2>"
        "<p>异构协处理器监控平台</p>"
        "<p>版本：0.1.0</p>"
        "<p>基于 Qt/C++ 开发</p>");
}

void MainWindow::onDeviceStatusChanged(const QString& deviceId, bool connected, bool online)
{
    Q_UNUSED(deviceId)
    updateStatusBar();
    
    QString status = connected ? (online ? "在线" : "已连接") : "离线";
    statusBar()->showMessage(QString("设备 %1 状态：%2").arg(deviceId).arg(status), 3000);
}

void MainWindow::onTelemetryReceived(const QString& deviceId, const TelemetryData& data)
{
    Q_UNUSED(deviceId)
    Q_UNUSED(data)
    // 数据更新由 DataWidget 处理
}

void MainWindow::onAlarmTriggered(const QString& deviceId, const AlarmRecord& record)
{
    showAlarmNotification(record);
}

} // namespace HeteroLink
