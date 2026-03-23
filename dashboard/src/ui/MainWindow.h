/**
 * HeteroLink Host - 主窗口
 * 
 * @file MainWindow.h
 * @brief 应用程序主界面
 */

#pragma once

#include <QMainWindow>
#include <QMap>
#include <memory>
#include <QDockWidget>

namespace Ui { class MainWindow; }

namespace HeteroLink {

class Application;
class DevicePanel;
class DataWidget;
class ConfigPanel;
class LogPanel;
struct TelemetryData;
struct AlarmRecord;

/**
 * @brief 主窗口类
 * 
 * 应用程序主界面，包含设备面板、数据可视化、配置面板
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(Application *app, QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionStart_triggered();
    void on_actionStop_triggered();
    void on_actionExport_triggered();
    void on_actionSettings_triggered();
    void on_actionAbout_triggered();
    
    void onDeviceStatusChanged(const QString& deviceId, bool connected, bool online);
    void onTelemetryReceived(const QString& deviceId, const TelemetryData& data);
    void onAlarmTriggered(const QString& deviceId, const AlarmRecord& record);
    
private:
    Ui::MainWindow *ui;
    Application *application_;
    
    DevicePanel *devicePanel_;
    DataWidget *dataWidget_;
    ConfigPanel *configPanel_;
    QDockWidget *logDock_;
    LogPanel *logPanel_;
    
    void setupUI();
    void setupConnections();
    void updateStatusBar();
    void showAlarmNotification(const AlarmRecord& record);
    void setupLogPanel();
};

} // namespace HeteroLink
