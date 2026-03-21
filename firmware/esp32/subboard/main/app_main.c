/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app_main.c
 * @brief HeteroLink ESP32 Subboard - MQTT5 Client
 * 
 * 远端通道：MQTT over WiFi - 设备状态上云、远程控制、告警推送
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

static const char *TAG = "heterolink";

// MQTT 客户端句柄
static esp_mqtt_client_handle_t mqtt_client = NULL;

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
        
        // 订阅控制命令 topic
        msg_id = esp_mqtt_client_subscribe(client, "heterolink/subboard/+/command", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        
        // 发布上线消息
        msg_id = esp_mqtt_client_publish(client, "heterolink/subboard/status", 
                                         "online", 0, 1, 1);
        ESP_LOGI(TAG, "sent publish status=online, msg_id=%d", msg_id);
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
        
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        
        // TODO: 解析命令并执行
        // 例如：{"cmd": "read_adc", "channel": 0}
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "Transport error: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGE(TAG, "TLS stack error: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGE(TAG, "Socket errno: %d (%s)", 
                     event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        ESP_LOGE(TAG, "MQTT connect return code: %d", event->error_handle->connect_return_code);
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
    ESP_LOGI(TAG, "MQTT Broker URI: %s", CONFIG_EXAMPLE_MQTT_BROKER_URI);
    
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_EXAMPLE_MQTT_BROKER_URI,
            .verification.certificate = NULL,  // 不验证证书 (测试用)
        },
        .session = {
            .protocol_ver = MQTT_PROTOCOL_V_5,
            .last_will = {
                .topic = "heterolink/subboard/status",
                .msg = "offline",
                .msg_len = 7,
                .qos = 1,
                .retain = true,
            },
        },
        .network = {
            .timeout_ms = 10000,  // 增加超时时间
            .reconnect_timeout_ms = 5000,
        },
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    
    ESP_LOGI(TAG, "MQTT client started (will retry on failure)");
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  HeteroLink ESP32 Subboard");
    ESP_LOGI(TAG, "  Version: 0.1.0");
    ESP_LOGI(TAG, "  Channel: Remote (MQTT over WiFi)");
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
    
    // 连接 WiFi (使用 example_connect 阻塞直到连接成功)
    ESP_LOGI(TAG, "Connecting to WiFi...");
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "WiFi connected!");
    
    // 启动 MQTT 客户端
    mqtt_app_start();
    
    ESP_LOGI(TAG, "Initialization complete!");
}
