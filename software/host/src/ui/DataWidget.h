/**
 * HeteroLink Host - 数据可视化组件
 * 
 * @file DataWidget.h
 * @brief 波形图、数据表格显示
 */

#pragma once

#include <QWidget>
#include <QMap>
#include <QVector>
#include <memory>

class QCustomPlot;
class QTableWidget;

namespace HeteroLink {

class DataProcessor;

/**
 * @brief 数据可视化组件类
 * 
 * 显示实时波形图和数据表格
 */
class DataWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit DataWidget(QWidget *parent = nullptr);
    ~DataWidget();
    
    /**
     * @brief 设置数据处理器
     * @param processor 数据处理器
     */
    void setDataProcessor(DataProcessor* processor);
    
    /**
     * @brief 选择显示的设备
     * @param deviceId 设备 ID
     */
    void selectDevice(const QString& deviceId);
    
    /**
     * @brief 设置显示模式
     * @param mode "wave" 波形 / "table" 表格 / "both" 两者
     */
    void setDisplayMode(const QString& mode);
    
    /**
     * @brief 清除显示
     */
    void clear();
    
public slots:
    /**
     * @brief 更新数据
     * @param deviceId 设备 ID
     */
    void updateData(const QString& deviceId);
    
private slots:
    void onDeviceSelected(int index);
    void onModeChanged(int index);
    void onExportClicked();
    void onClearClicked();
    
private:
    QCustomPlot *plot_;
    QTableWidget *table_;
    QComboBox *deviceCombo_;
    QComboBox *modeCombo_;
    QPushButton *exportBtn_;
    QPushButton *clearBtn_;
    
    DataProcessor* dataProcessor_ = nullptr;
    QString currentDevice_;
    QString displayMode_ = "wave";
    
    void setupUI();
    void setupPlot();
    void setupTable();
    void updatePlot(const QString& deviceId);
    void updateTable(const QString& deviceId);
    QVector<QColor> getChannelColors() const;
};

} // namespace HeteroLink
