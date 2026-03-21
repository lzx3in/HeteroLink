/**
 * HeteroLink Host - 数据可视化组件实现
 */

#include "ui/DataWidget.h"
#include "core/DataProcessor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QScrollArea>

// 如果没有 QCustomPlot/QCharts，使用简单的占位实现
// Qt Charts 需要单独安装 (qt6-charts-dev)

namespace HeteroLink {

DataWidget::DataWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

DataWidget::~DataWidget()
{
}

void DataWidget::setDataProcessor(DataProcessor* processor)
{
    dataProcessor_ = processor;
    if (dataProcessor_) {
        connect(dataProcessor_, &DataProcessor::dataUpdated,
                this, &DataWidget::updateData);
    }
}

void DataWidget::selectDevice(const QString& deviceId)
{
    currentDevice_ = deviceId;
    
    // 更新设备下拉框
    int index = deviceCombo_->findData(deviceId);
    if (index >= 0) {
        deviceCombo_->setCurrentIndex(index);
    }
    
    if (displayMode_ == "wave" || displayMode_ == "both") {
        updatePlot(deviceId);
    }
    if (displayMode_ == "table" || displayMode_ == "both") {
        updateTable(deviceId);
    }
}

void DataWidget::setDisplayMode(const QString& mode)
{
    displayMode_ = mode;
    
    bool showWave = (mode == "wave" || mode == "both");
    bool showTable = (mode == "table" || mode == "both");
    
    plotPlaceholder_->setVisible(showWave);
    table_->setVisible(showTable);
    
    if (!currentDevice_.isEmpty() && dataProcessor_) {
        updateData(currentDevice_);
    }
}

void DataWidget::clear()
{
    // 图表清理（占位）
    
    if (table_) {
        table_->setRowCount(0);
    }
}

void DataWidget::setupUI()
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    
    // 顶部工具栏
    auto toolbar = new QHBoxLayout();
    
    toolbar->addWidget(new QLabel("设备:"));
    deviceCombo_ = new QComboBox();
    deviceCombo_->setMinimumWidth(200);
    toolbar->addWidget(deviceCombo_);
    
    toolbar->addWidget(new QLabel("显示:"));
    modeCombo_ = new QComboBox();
    modeCombo_->addItem("波形", "wave");
    modeCombo_->addItem("表格", "table");
    modeCombo_->addItem("两者", "both");
    modeCombo_->setCurrentIndex(0);
    toolbar->addWidget(modeCombo_);
    
    toolbar->addStretch();
    
    exportBtn_ = new QPushButton("导出");
    exportBtn_->setIcon(QIcon::fromTheme("document-export"));
    toolbar->addWidget(exportBtn_);
    
    clearBtn_ = new QPushButton("清除");
    clearBtn_->setIcon(QIcon::fromTheme("edit-clear"));
    toolbar->addWidget(clearBtn_);
    
    layout->addLayout(toolbar);
    
    // 波形图（占位 - 需要安装 QCustomPlot 或 Qt Charts）
    auto plotGroup = new QGroupBox("实时波形");
    auto plotLayout = new QVBoxLayout(plotGroup);
    
    plotPlaceholder_ = new QWidget();
    plotPlaceholder_->setMinimumHeight(300);
    auto placeholderLayout = new QVBoxLayout(plotPlaceholder_);
    placeholderLayout->setAlignment(Qt::AlignCenter);
    
    statusLabel_ = new QLabel("📊 图表功能需要安装 QCustomPlot 或 Qt Charts\n\n安装方法:\n  sudo apt install libqcustomplot-dev\n  或\n  sudo apt install qt6-charts-dev");
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setStyleSheet("color: #888; font-size: 14px;");
    placeholderLayout->addWidget(statusLabel_);
    
    plotLayout->addWidget(plotPlaceholder_);
    layout->addWidget(plotGroup);
    
    // 数据表格
    auto tableGroup = new QGroupBox("数据表格");
    auto tableLayout = new QVBoxLayout(tableGroup);
    
    table_ = new QTableWidget();
    table_->setAlternatingRowColors(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableLayout->addWidget(table_);
    
    layout->addWidget(tableGroup);
    
    // 连接信号
    connect(deviceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataWidget::onDeviceSelected);
    connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataWidget::onModeChanged);
    connect(exportBtn_, &QPushButton::clicked, this, &DataWidget::onExportClicked);
    connect(clearBtn_, &QPushButton::clicked, this, &DataWidget::onClearClicked);
}

void DataWidget::setupPlot()
{
    // 图表功能需要安装 QCustomPlot 或 Qt Charts
    // 当前为占位实现
}

void DataWidget::setupTable()
{
    table_->setColumnCount(17);  // timestamp + 16 channels
    
    QStringList headers;
    headers << "时间戳";
    for (int i = 0; i < 16; ++i) {
        headers << QString("通道 %1").arg(i);
    }
    table_->setHorizontalHeaderLabels(headers);
    
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
}

void DataWidget::updatePlot(const QString& deviceId)
{
    // 图表功能需要安装 QCustomPlot 或 Qt Charts
    // 当前为占位实现
    Q_UNUSED(deviceId)
}

void DataWidget::updateTable(const QString& deviceId)
{
    if (!dataProcessor_ || !table_) {
        return;
    }
    
    auto data = dataProcessor_->getLatestData(deviceId, 100);
    if (data.isEmpty()) {
        return;
    }
    
    table_->setRowCount(data.size());
    
    for (int i = 0; i < data.size(); ++i) {
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(data[i].timestamp)));
        
        for (int ch = 0; ch < data[i].channels.size() && ch < 16; ++ch) {
            table_->setItem(i, ch + 1, new QTableWidgetItem(QString::number(data[i].channels[ch], 'f', 4)));
        }
    }
}

void DataWidget::updateData(const QString& deviceId)
{
    if (deviceId != currentDevice_) {
        return;
    }
    
    if (displayMode_ == "wave" || displayMode_ == "both") {
        updatePlot(deviceId);
    }
    if (displayMode_ == "table" || displayMode_ == "both") {
        updateTable(deviceId);
    }
}

void DataWidget::onDeviceSelected(int index)
{
    QString deviceId = deviceCombo_->itemData(index).toString();
    if (!deviceId.isEmpty()) {
        currentDevice_ = deviceId;
        updateData(deviceId);
    }
}

void DataWidget::onModeChanged(int index)
{
    QString mode = modeCombo_->itemData(index).toString();
    setDisplayMode(mode);
}

void DataWidget::onExportClicked()
{
    if (!dataProcessor_ || currentDevice_.isEmpty()) {
        return;
    }
    
    QString filePath = QString("~/heterolink_%1_export.csv").arg(currentDevice_);
    if (dataProcessor_->exportToCsv(currentDevice_, filePath)) {
        // TODO: 显示成功提示
    }
}

void DataWidget::onClearClicked()
{
    if (dataProcessor_ && !currentDevice_.isEmpty()) {
        dataProcessor_->clearData(currentDevice_);
        clear();
    }
}

QVector<QColor> DataWidget::getChannelColors() const
{
    return {
        Qt::red, Qt::blue, Qt::green, Qt::magenta,
        Qt::cyan, Qt::darkYellow, Qt::darkGreen, Qt::darkBlue,
        Qt::darkMagenta, Qt::darkCyan, Qt::black, Qt::gray,
        Qt::darkRed, Qt::yellow, Qt::white, Qt::lightGray
    };
}

} // namespace HeteroLink
