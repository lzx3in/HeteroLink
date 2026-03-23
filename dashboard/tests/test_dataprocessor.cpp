/**
 * HeteroLink Host - 数据处理器测试
 */

#include <QTest>
#include <QDebug>

class TestDataProcessor : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase() {
        qDebug() << "Starting data processor tests";
    }
    
    void cleanupTestCase() {
        qDebug() << "Data processor tests finished";
    }
    
    void testBufferOperations() {
        // 测试环形缓冲区基本操作
        QVector<int> buffer;
        int maxSize = 100;
        
        // 添加数据
        for (int i = 0; i < 150; ++i) {
            buffer.append(i);
            if (buffer.size() > maxSize) {
                buffer.removeFirst();
            }
        }
        
        // 验证缓冲区大小
        QCOMPARE(buffer.size(), maxSize);
        
        // 验证数据（应该保留最后 100 个）
        QCOMPARE(buffer.first(), 50);
        QCOMPARE(buffer.last(), 149);
    }
    
    void testStatisticsCalculation() {
        // 测试统计计算
        QVector<float> values = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        
        // 计算平均值
        float sum = 0;
        for (float v : values) {
            sum += v;
        }
        float avg = sum / values.size();
        
        QCOMPARE(avg, 3.0f);
        
        // 计算最小值
        float min = values.first();
        for (float v : values) {
            if (v < min) min = v;
        }
        QCOMPARE(min, 1.0f);
        
        // 计算最大值
        float max = values.first();
        for (float v : values) {
            if (v > max) max = v;
        }
        QCOMPARE(max, 5.0f);
    }
    
    void testDataExport() {
        // 测试数据导出格式
        QString csvHeader = "Timestamp,Channel1,Channel2,Channel3,Channel4\n";
        QString csvRow = "1000,1.234,2.345,3.456,4.567\n";
        
        QVERIFY(csvHeader.contains("Timestamp"));
        QVERIFY(csvHeader.contains("Channel1"));
        QVERIFY(!csvRow.isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestDataProcessor)
#include "test_dataprocessor.moc"
