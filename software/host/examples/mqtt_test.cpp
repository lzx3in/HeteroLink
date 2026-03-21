/**
 * HeteroLink Host - MQTT 功能测试程序
 * 
 * 用于测试 MQTT 通道的基本功能
 */

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include "protocol/MqttChannel.h"

using namespace HeteroLink;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== HeteroLink MQTT Test ===";
    
    // 创建 MQTT 通道
    MqttChannel mqttChannel;
    
    // 配置连接
    MqttConfig config;
    config.brokerHost = "broker.emqx.io";  // 公共测试 Broker
    config.brokerPort = 1883;
    config.clientId = "heterolink_test_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    config.willTopic = "heterolink/test/status";
    config.willMessage = "offline";
    
    qDebug() << "Connecting to" << config.brokerHost << ":" << config.brokerPort;
    
    // 连接信号
    QObject::connect(&mqttChannel, &MqttChannel::connectionChanged,
                     [](bool connected) {
        qDebug() << "[EVENT] Connection changed:" << (connected ? "connected" : "disconnected");
    });
    
    QObject::connect(&mqttChannel, &MqttChannel::messageReceived,
                     [](const QString& topic, const QByteArray& payload) {
        qDebug() << "[EVENT] Message received on" << topic << ":" << payload;
    });
    
    QObject::connect(&mqttChannel, &MqttChannel::errorOccurred,
                     [](const QString& error) {
        qDebug() << "[EVENT] Error:" << error;
    });
    
    QObject::connect(&mqttChannel, &MqttChannel::deviceStatusReceived,
                     [](const QString& deviceId, bool online) {
        qDebug() << "[EVENT] Device status:" << deviceId << "online =" << online;
    });
    
    QObject::connect(&mqttChannel, &MqttChannel::telemetryReceived,
                     [](const QString& deviceId, const QString& data) {
        qDebug() << "[EVENT] Telemetry from" << deviceId << ":" << data;
    });
    
    // 连接到 Broker
    if (!mqttChannel.connect(config)) {
        qCritical() << "Failed to connect to MQTT broker";
        return -1;
    }
    
    // 等待连接建立
    QTimer::singleShot(2000, [&mqttChannel]() {
        qDebug() << "\n=== Subscribing to topics ===";
        
        // 订阅所有设备状态
        mqttChannel.subscribeAllDeviceStatus();
        qDebug() << "Subscribed to: heterolink/subboard/+/status";
        
        // 订阅测试设备的命令
        mqttChannel.subscribeDeviceCommands("test_device");
        qDebug() << "Subscribed to: heterolink/subboard/test_device/command";
        
        // 发布测试消息
        qDebug() << "\n=== Publishing test messages ===";
        mqttChannel.publishDeviceStatus("test_device", true);
        qDebug() << "Published: heterolink/subboard/test_device/status = online";
        
        mqttChannel.publishTelemetry("test_device", R"({"temperature":25.5,"humidity":60})");
        qDebug() << "Published: heterolink/subboard/test_device/telemetry = {\"temperature\":25.5,\"humidity\":60}";
    });
    
    // 5 秒后退出
    QTimer::singleShot(10000, &app, &QCoreApplication::quit);
    
    qDebug() << "\nRunning for 10 seconds...\n";
    
    return app.exec();
}
