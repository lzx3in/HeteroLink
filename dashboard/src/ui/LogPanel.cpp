/**
 * HeteroLink Host - 日志查看面板实现
 * 
 * @file LogPanel.cpp
 */

#include "ui/LogPanel.h"
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextDocument>
#include <QScrollBar>
#include <QFontDatabase>
#include <QLabel>
#include <QBrush>

namespace HeteroLink {

LogPanel::LogPanel(QWidget *parent)
    : QWidget(parent)
    , textEdit_(nullptr)
    , levelFilterCombo_(nullptr)
    , autoScrollCheck_(nullptr)
    , clearButton_(nullptr)
    , exportButton_(nullptr)
    , processTimer_(new QTimer(this))
    , minLevel_(Logger::Level::DEBUG)
    , maxLines_(1000)
    , logCount_(0)
{
    setupFormats();
    setupUI();
    connectSignals();
    
    // 启动定时器处理日志队列（每 100ms 处理一次）
    processTimer_->start(100);
}

LogPanel::~LogPanel()
{
}

void LogPanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    
    // 工具栏
    auto* toolbar = new QHBoxLayout();
    
    // 级别过滤
    toolbar->addWidget(new QLabel("级别:"));
    levelFilterCombo_ = new QComboBox();
    levelFilterCombo_->addItem("DEBUG", static_cast<int>(Logger::Level::DEBUG));
    levelFilterCombo_->addItem("INFO", static_cast<int>(Logger::Level::INFO));
    levelFilterCombo_->addItem("WARNING", static_cast<int>(Logger::Level::WARNING));
    levelFilterCombo_->addItem("ERROR", static_cast<int>(Logger::Level::ERROR));
    levelFilterCombo_->setCurrentIndex(0);
    toolbar->addWidget(levelFilterCombo_);
    
    toolbar->addSpacing(20);
    
    // 自动滚动
    autoScrollCheck_ = new QCheckBox("自动滚动");
    autoScrollCheck_->setChecked(true);
    toolbar->addWidget(autoScrollCheck_);
    
    toolbar->addStretch();
    
    // 清空按钮
    clearButton_ = new QToolButton();
    clearButton_->setText("清空");
    toolbar->addWidget(clearButton_);
    
    // 导出按钮
    exportButton_ = new QToolButton();
    exportButton_->setText("导出...");
    toolbar->addWidget(exportButton_);
    
    layout->addLayout(toolbar);
    
    // 日志文本区域
    textEdit_ = new QTextEdit();
    textEdit_->setReadOnly(true);
    textEdit_->setLineWrapMode(QTextEdit::NoWrap);
    textEdit_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(textEdit_);
    
    setLayout(layout);
}

void LogPanel::setupFormats()
{
    // DEBUG - 灰色
    formatDebug_.setForeground(QBrush(Qt::gray));
    
    // INFO - 黑色
    formatInfo_.setForeground(QBrush(Qt::black));
    
    // WARNING - 橙色
    formatWarning_.setForeground(QBrush(QColor(255, 140, 0)));
    
    // ERROR - 红色，加粗
    formatError_.setForeground(QBrush(Qt::red));
    formatError_.setFontWeight(QFont::Bold);
}

void LogPanel::connectSignals()
{
    connect(levelFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                minLevel_ = static_cast<Logger::Level>(levelFilterCombo_->itemData(index).toInt());
            });
    
    connect(clearButton_, &QToolButton::clicked,
            this, &LogPanel::clearLogs);
    
    connect(exportButton_, &QToolButton::clicked,
            [this]() {
                QString filePath = QFileDialog::getSaveFileName(
                    this, "导出日志", "", "Text Files (*.txt);;All Files (*)");
                if (!filePath.isEmpty()) {
                    exportLogs(filePath);
                }
            });
    
    connect(processTimer_, &QTimer::timeout,
            this, &LogPanel::processLogQueue);
}

void LogPanel::addLogEntry(Logger::Level level, const QString& message)
{
    // 将日志加入队列（线程安全）
    logQueue_.enqueue(qMakePair(level, message));
}

void LogPanel::clearLogs()
{
    textEdit_->clear();
    logCount_ = 0;
    logQueue_.clear();
    emit logCountChanged(0);
}

bool LogPanel::exportLogs(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out << textEdit_->toPlainText();
    file.close();
    return true;
}

void LogPanel::setLevelFilter(Logger::Level minLevel)
{
    minLevel_ = minLevel;
    for (int i = 0; i < levelFilterCombo_->count(); ++i) {
        if (levelFilterCombo_->itemData(i).toInt() == static_cast<int>(minLevel)) {
            levelFilterCombo_->setCurrentIndex(i);
            break;
        }
    }
}

void LogPanel::setMaxLines(int maxLines)
{
    maxLines_ = maxLines;
}

void LogPanel::processLogQueue()
{
    // 批量处理队列中的日志（最多 50 条/次）
    int count = 0;
    while (!logQueue_.isEmpty() && count < 50) {
        auto entry = logQueue_.dequeue();
        Logger::Level level = entry.first;
        QString message = entry.second;
        
        // 检查级别过滤
        if (level < minLevel_) {
            continue;
        }
        
        // 选择格式
        QTextCharFormat format;
        switch (level) {
            case Logger::Level::DEBUG:   format = formatDebug_; break;
            case Logger::Level::INFO:    format = formatInfo_; break;
            case Logger::Level::WARNING: format = formatWarning_; break;
            case Logger::Level::ERROR:   format = formatError_; break;
        }
        
        // 格式化时间戳和级别
        QString timestamp = formatTimestamp();
        QString levelStr = levelToString(level);
        QString line = QString("[%1] [%2] %3\n").arg(timestamp, levelStr, message);
        
        // 插入文本
        textEdit_->moveCursor(QTextCursor::End);
        textEdit_->textCursor().insertText(line, format);
        
        logCount_++;
        count++;
    }
    
    // 限制最大行数
    if (logCount_ > maxLines_) {
        QTextCursor cursor(textEdit_->document());
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor, logCount_ - maxLines_);
        cursor.removeSelectedText();
        logCount_ = maxLines_;
    }
    
    // 自动滚动
    if (autoScrollCheck_->isChecked()) {
        QScrollBar* scrollbar = textEdit_->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
    }
    
    emit logCountChanged(logCount_);
}

QString LogPanel::levelToString(Logger::Level level)
{
    switch (level) {
        case Logger::Level::DEBUG:   return "DEBUG";
        case Logger::Level::INFO:    return "INFO";
        case Logger::Level::WARNING: return "WARNING";
        case Logger::Level::ERROR:   return "ERROR";
        default:                     return "UNKNOWN";
    }
}

QString LogPanel::formatTimestamp()
{
    return QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
}

} // namespace HeteroLink
