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
    
    chartView_->setVisible(showWave);
    table_->setVisible(showTable);
    
    if (!currentDevice_.isEmpty() && dataProcessor_) {
        updateData(currentDevice_);
    }
}

void DataWidget::clear()
{
    // 清理图表数据
    for (auto series : channelSeries_.values()) {
        series->clear();
        for (int i = 0; i < 100; ++i) {
            series->append(i, 0);
        }
    }
    
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
    toolbar->addWidget(exportBtn_);
    
    clearBtn_ = new QPushButton("清除");
    toolbar->addWidget(clearBtn_);
    
    layout->addLayout(toolbar);
    
    // 波形图（使用 Qt Charts）
    auto plotGroup = new QGroupBox("实时波形");
    auto plotLayout = new QVBoxLayout(plotGroup);
    
    setupPlot();
    plotLayout->addWidget(chartView_);
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
    
    // 初始化表格
    setupTable();
}

void DataWidget::setupPlot()
{
    // 创建图表
    chart_ = new QChart();
    chart_->setTitle("实时数据波形");
    chart_->setAnimationOptions(QChart::NoAnimation);
    chart_->legend()->setVisible(true);
    chart_->legend()->setAlignment(Qt::AlignBottom);
    
    // 创建坐标轴
    axisX_ = new QValueAxis();
    axisX_->setTitleText("采样点");
    axisX_->setRange(0, 100);
    axisX_->setTickCount(11);
    
    axisY_ = new QValueAxis();
    axisY_->setTitleText("数值");
    axisY_->setRange(-10, 10);
    axisY_->setTickCount(9);
    
    chart_->addAxis(axisX_, Qt::AlignBottom);
    chart_->addAxis(axisY_, Qt::AlignLeft);
    
    // 创建 16 个通道的数据系列
    auto colors = getChannelColors();
    for (int ch = 0; ch < 16; ++ch) {
        auto series = new QLineSeries();
        series->setName(QString("通道 %1").arg(ch));
        series->setColor(colors[ch % colors.size()]);
        series->setPen(QPen(colors[ch % colors.size()], 1.5));
        
        // 初始化空数据
        for (int i = 0; i < 100; ++i) {
            series->append(i, 0);
        }
        
        chart_->addSeries(series);
        series->attachAxis(axisX_);
        series->attachAxis(axisY_);
        
        channelSeries_[ch] = series;
    }
    
    // 创建图表视图
    chartView_ = new QChartView(chart_);
    chartView_->setRenderHint(QPainter::Antialiasing);
    chartView_->setMinimumHeight(300);
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
    if (!dataProcessor_) {
        return;
    }
    
    auto data = dataProcessor_->getLatestData(deviceId, 100);
    if (data.isEmpty()) {
        return;
    }
    
    // 更新每个通道的数据系列
    for (int ch = 0; ch < 16; ++ch) {
        if (!channelSeries_.contains(ch)) {
            continue;
        }
        
        channelSeries_[ch]->clear();
        
        for (int i = 0; i < data.size() && i < 100; ++i) {
            double value = (ch < data[i].channels.size()) ? data[i].channels[ch] : 0;
            channelSeries_[ch]->append(i, value);
        }
    }
    
    // 更新 X 轴范围
    axisX_->setRange(0, qMax(100, data.size()));
    
    // 自动调整 Y 轴范围
    double minY = 10, maxY = -10;
    for (int ch = 0; ch < 16; ++ch) {
        if (!channelSeries_.contains(ch)) {
            continue;
        }
        for (const auto& point : channelSeries_[ch]->points()) {
            if (point.y() < minY) minY = point.y();
            if (point.y() > maxY) maxY = point.y();
        }
    }
    if (minY < maxY) {
        double padding = (maxY - minY) * 0.1;
        axisY_->setRange(minY - padding, maxY + padding);
    }
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
