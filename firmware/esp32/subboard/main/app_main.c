/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app_main.c
 * @brief HeteroLink ESP32 Subboard - 主程序
 * 
 * 功能：
 * - WiFi 连接管理
 * - MQTT5 远端通道（设备状态上云、远程控制、告警推送）
 * - UART 自定义二进制协议（近端通道）
 * - SPI+DMA 高速数据传输（板间通道）
 * - ADC/GPIO 双模点测
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "protocol_examples_common.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

// 组件头文件
#include "spi_dma.h"
#include "uart_protocol.h"
#include "adc_gpio_probe.h"

static const char *TAG = "heterolink";

// 全局句柄
static esp_mqtt_client_handle_t mqtt_client = NULL;
static spi_dma_device_t spi_dev;
static uart_protocol_device_t uart_dev;
static adc_gpio_probe_device_t probe_dev;

// 系统状态
static bool wifi_connected = false;
static bool mqtt_connected = false;
static uint32_t telemetry_interval_ms = CONFIG_HETERO_DATA_UPLOAD_INTERVAL;
static uint32_t last_telemetry_time = 0;

/**
 * @brief 发布遥测数据到 MQTT
 */
static void mqtt_publish_telemetry(float *channels, size_t count)
{
    if (!mqtt_connected) {
        return;
    }
    
    char json_payload[512];
    int offset = 0;
    
    // 构建 JSON
    offset += snprintf(json_payload + offset, sizeof(json_payload) - offset, 
                       "{\"ts\":%lu,\"device_id\":\"%s\",\"channels\":{",
                       (unsigned long)esp_timer_get_time() / 1000,
                       CONFIG_HETERO_DEVICE_ID);
    
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            offset += snprintf(json_payload + offset, sizeof(json_payload) - offset, ",");
        }
        offset += snprintf(json_payload + offset, sizeof(json_payload) - offset, 
                           "\"ch%u\":%.3f", (unsigned)(i + 1), channels[i]);
    }
    
    offset += snprintf(json_payload + offset, sizeof(json_payload) - offset, 
                       "},\"sample_rate\":%lu}", (unsigned long)(1000 / telemetry_interval_ms));
    
    // 发布到 MQTT
    esp_mqtt_client_publish(mqtt_client, 
                            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/telemetry",
                            json_payload, 0, 0, 0);
    
    ESP_LOGD(TAG, "Published telemetry: %s", json_payload);
}

/**
 * @brief 发布设备状态
 */
static void mqtt_publish_status(const char *status)
{
    if (!mqtt_connected) {
        return;
    }
    
    char json_payload[256];
    snprintf(json_payload, sizeof(json_payload),
             "{\"state\":\"%s\",\"timestamp\":%lu,\"firmware_version\":\"0.1.0\",\"uptime\":%lu}",
             status,
             (unsigned long)esp_timer_get_time() / 1000,
             (unsigned long)(esp_get_free_heap_size()));  // 用空闲内存模拟 uptime
    
    esp_mqtt_client_publish(mqtt_client,
                            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/status",
                            json_payload, 0, 1, 1);
    
    ESP_LOGI(TAG, "Published status: %s", status);
}

/**
 * @brief 发布告警
 */
static void mqtt_publish_alarm(const char *level, uint8_t channel, float value, float threshold)
{
    if (!mqtt_connected) {
        return;
    }
    
    char json_payload[256];
    snprintf(json_payload, sizeof(json_payload),
             "{\"level\":\"%s\",\"channel\":%u,\"value\":%.3f,\"threshold\":%.3f,\"message\":\"Channel %u exceeded threshold\"}",
             level, channel, value, threshold, channel);
    
    esp_mqtt_client_publish(mqtt_client,
                            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/alarm",
                            json_payload, 0, 1, 0);
    
    ESP_LOGW(TAG, "Published alarm: %s", json_payload);
}

/**
 * @brief 处理 MQTT 命令
 */
static void mqtt_handle_command(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG, "Handling MQTT command from %s", topic);
    ESP_LOGI(TAG, "Command: %.*s", data_len, data);
    
    // TODO: 解析 JSON 命令并执行
    // 示例命令：
    // {"cmd": "start", "params": {"sample_rate": 1000, "channels": [1,2,3,4]}}
    // {"cmd": "stop"}
    // {"cmd": "set_gpio", "channel": 4, "value": 1}
    
    // 简单示例：如果收到 "start" 命令，启动采集
    if (strstr(data, "\"start\"")) {
        ESP_LOGI(TAG, "Received START command");
        adc_gpio_probe_start_sampling(&probe_dev);
    } else if (strstr(data, "\"stop\"")) {
        ESP_LOGI(TAG, "Received STOP command");
        adc_gpio_probe_stop_sampling(&probe_dev);
    }
}

/**
 * @brief MQTT 事件处理
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;
        
        // 订阅控制命令 topic
        msg_id = esp_mqtt_client_subscribe(client, 
                                           "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/command", 
                                           1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        
        // 发布上线消息
        mqtt_publish_status("online");
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;
        break;
        
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        
        // 处理命令
        mqtt_handle_command(event->topic, event->data, event->data_len);
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "Transport error: 0x%x", event->error_handle->esp_tls_last_esp_err);
        }
        break;
        
    default:
        ESP_LOGD(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/**
 * @brief 启动 MQTT 客户端
 */
static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "MQTT Broker URI: %s", CONFIG_HETERO_MQTT_BROKER_URI);
    
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_HETERO_MQTT_BROKER_URI,
            .verification.certificate = NULL,
        },
        .session = {
            .protocol_ver = MQTT_PROTOCOL_V_5,
            .client_id = CONFIG_HETERO_MQTT_CLIENT_ID,
            .last_will = {
                .topic = "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/status",
                .msg = "offline",
                .msg_len = 7,
                .qos = 1,
                .retain = true,
            },
        },
        .network = {
            .timeout_ms = 10000,
            .reconnect_timeout_ms = 5000,
        },
    };
    
    // 如果有用户名/密码
    if (strlen(CONFIG_HETERO_MQTT_USERNAME) > 0) {
        mqtt_cfg.credentials.username = CONFIG_HETERO_MQTT_USERNAME;
        mqtt_cfg.credentials.authentication = CONFIG_HETERO_MQTT_PASSWORD;
    }
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    
    ESP_LOGI(TAG, "MQTT client started");
}

/**
 * @brief 初始化所有组件
 */
static void components_init(void)
{
    esp_err_t ret;
    
    // 初始化 UART 协议（近端通道）
#if CONFIG_HETERO_ENABLE_UART_PROTOCOL
    uart_protocol_config_t uart_config = UART_PROTOCOL_DEFAULT_CONFIG();
    ret = uart_protocol_init(&uart_config, &uart_dev);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "UART Protocol initialized");
    } else {
        ESP_LOGE(TAG, "Failed to initialize UART Protocol: %s", esp_err_to_name(ret));
    }
#endif
    
    // 初始化 SPI+DMA（板间通道）
#if CONFIG_HETERO_ENABLE_SPI_DMA
    spi_dma_config_t spi_config = SPI_DMA_DEFAULT_CONFIG();
    ret = spi_dma_init(&spi_config, &spi_dev);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPI+DMA initialized");
    } else {
        ESP_LOGE(TAG, "Failed to initialize SPI+DMA: %s", esp_err_to_name(ret));
    }
#endif
    
    // 初始化 ADC/GPIO 探测
#if CONFIG_HETERO_ENABLE_ADC_GPIO_PROBE
    adc_gpio_probe_config_t probe_config = ADC_GPIO_PROBE_DEFAULT_CONFIG();
    ret = adc_gpio_probe_init(&probe_config, &probe_dev);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC/GPIO Probe initialized");
        adc_gpio_probe_start_sampling(&probe_dev);
    } else {
        ESP_LOGE(TAG, "Failed to initialize ADC/GPIO Probe: %s", esp_err_to_name(ret));
    }
#endif
}

/**
 * @brief 主循环任务
 */
static void main_loop_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Main loop task started");
    
    while (1) {
        uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000);
        
        // 定期发布遥测数据
        if (mqtt_connected && (current_time - last_telemetry_time >= telemetry_interval_ms)) {
            last_telemetry_time = current_time;
            
            // 读取 ADC/GPIO 数据
#if CONFIG_HETERO_ENABLE_ADC_GPIO_PROBE
            uint32_t values[PROBE_MAX_CHANNELS];
            size_t count = PROBE_MAX_CHANNELS;
            
            if (adc_gpio_probe_read_all(&probe_dev, values, &count) == ESP_OK) {
                // 转换为 float（ADC 值转电压）
                float channels[PROBE_MAX_CHANNELS];
                for (size_t i = 0; i < count; i++) {
                    channels[i] = (float)values[i];
                }
                
                // 发布到 MQTT
                mqtt_publish_telemetry(channels, count);
                
                // 同时通过 UART 发送
#if CONFIG_HETERO_ENABLE_UART_PROTOCOL
                uart_protocol_send_telemetry(&uart_dev, current_time, channels, count);
#endif
            }
#endif
        }
        
        // 处理 UART 命令
#if CONFIG_HETERO_ENABLE_UART_PROTOCOL
        if (uart_protocol_receive(&uart_dev, 0) == ESP_OK) {
            uint8_t cmd;
            const uint8_t *payload;
            size_t length;
            
            if (uart_protocol_parse(&uart_dev, &cmd, &payload, &length) == ESP_OK) {
                ESP_LOGI(TAG, "UART command: 0x%02X, length=%zu", cmd, length);
                
                // 处理命令
                if (cmd == UART_CMD_HEARTBEAT) {
                    uart_protocol_send_heartbeat(&uart_dev);
                } else if (cmd == UART_CMD_START_ACQ) {
                    adc_gpio_probe_start_sampling(&probe_dev);
                } else if (cmd == UART_CMD_STOP_ACQ) {
                    adc_gpio_probe_stop_sampling(&probe_dev);
                }
            }
        }
#endif
        
        // 短暂延迟
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  HeteroLink ESP32 Subboard");
    ESP_LOGI(TAG, "  Version: 0.1.0");
    ESP_LOGI(TAG, "  Device ID: %s", CONFIG_HETERO_DEVICE_ID);
    ESP_LOGI(TAG, "========================================");
    
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    
    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 初始化网络
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 连接 WiFi
    ESP_LOGI(TAG, "Connecting to WiFi...");
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "WiFi connected!");
    wifi_connected = true;
    
    // 初始化所有组件
    components_init();
    
    // 启动 MQTT 客户端
    mqtt_app_start();
    
    // 创建主循环任务
    xTaskCreate(main_loop_task, "main_loop", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Initialization complete!");
}
