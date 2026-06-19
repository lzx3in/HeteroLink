/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app_main.c
 * @brief HeteroLink ESP32 Subboard - Main firmware
 *
 * Features:
 * - WiFi connection with exponential-backoff auto-reconnect
 * - MQTT5 cloud channel (status, telemetry, alarms, command/response)
 * - cJSON-based structured command parsing
 * - ADC/GPIO dual-mode probing
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
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_random.h"
#include "protocol_examples_common.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "sdkconfig.h"

/* Component headers */
#include "spi_dma.h"
#include "adc_gpio_probe.h"

static const char *TAG = "heterolink";

/* ── Global handles ─────────────────────────────────────────── */
static esp_mqtt_client_handle_t mqtt_client = NULL;
static spi_dma_device_t spi_dev;
static adc_gpio_probe_device_t probe_dev;

/* ── System state ───────────────────────────────────────────── */
static bool wifi_connected = false;
static bool mqtt_connected = false;
static uint32_t telemetry_interval_ms = CONFIG_HETERO_DATA_UPLOAD_INTERVAL;
static uint32_t last_telemetry_time = 0;

#if CONFIG_HETERO_ENABLE_ALARM
/* ── Alarm threshold state ──────────────────────────────────── */
static bool     alarm_enabled = true;
static float    alarm_upper_limit = CONFIG_HETERO_ALARM_UPPER_LIMIT;
static float    alarm_lower_limit = CONFIG_HETERO_ALARM_LOWER_LIMIT;
static bool     alarm_active[PROBE_MAX_CHANNELS] = { false };
#endif

/* ── WiFi exponential backoff state ─────────────────────────── */
#define WIFI_BACKOFF_BASE_MS   1000
#define WIFI_BACKOFF_MAX_MS    60000
#define WIFI_BACKOFF_JITTER_MS 500

static int      wifi_backoff_ms    = WIFI_BACKOFF_BASE_MS;
static int      wifi_reconnect_cnt = 0;
static TaskHandle_t wifi_backoff_task_handle = NULL;

/* ================================================================
 *  MQTT publish helpers
 * ================================================================ */

static void mqtt_publish_telemetry(float *channels, size_t count)
{
    if (!mqtt_connected) return;

    char json_payload[512];
    int off = 0;

    off += snprintf(json_payload + off, sizeof(json_payload) - off,
                    "{\"ts\":%lu,\"device_id\":\"%s\",\"channels\":{",
                    (unsigned long)(esp_timer_get_time() / 1000),
                    CONFIG_HETERO_DEVICE_ID);

    for (size_t i = 0; i < count; i++) {
        if (i > 0)
            off += snprintf(json_payload + off, sizeof(json_payload) - off, ",");
        off += snprintf(json_payload + off, sizeof(json_payload) - off,
                        "\"ch%u\":%.3f", (unsigned)(i + 1), channels[i]);
    }

    off += snprintf(json_payload + off, sizeof(json_payload) - off,
                    "},\"sample_rate\":%lu}",
                    (unsigned long)(1000 / telemetry_interval_ms));

    esp_mqtt_client_publish(mqtt_client,
                            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/telemetry",
                            json_payload, 0, 0, 0);
    ESP_LOGD(TAG, "Telemetry: %s", json_payload);
}

static void mqtt_publish_status(const char *status)
{
    if (!mqtt_connected) return;

    char json_payload[256];
    snprintf(json_payload, sizeof(json_payload),
             "{\"state\":\"%s\",\"timestamp\":%lu,\"firmware_version\":\"0.1.0\",\"uptime\":%lu}",
             status,
             (unsigned long)(esp_timer_get_time() / 1000),
             (unsigned long)(esp_timer_get_time() / 1000000ULL));

    esp_mqtt_client_publish(mqtt_client,
                            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/status",
                            json_payload, 0, 1, 1);
    ESP_LOGI(TAG, "Status: %s", status);
}

static void mqtt_publish_alarm(const char *level, uint8_t channel,
                               float value, float threshold)
{
    if (!mqtt_connected) return;

    char json_payload[256];
    snprintf(json_payload, sizeof(json_payload),
             "{\"level\":\"%s\",\"channel\":%u,\"value\":%.3f,"
             "\"threshold\":%.3f,\"message\":\"Channel %u exceeded threshold\"}",
             level, channel, value, threshold, channel);

    esp_mqtt_client_publish(mqtt_client,
                            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/alarm",
                            json_payload, 0, 1, 0);
    ESP_LOGW(TAG, "Alarm: %s", json_payload);
}

#if CONFIG_HETERO_ENABLE_ALARM
static void check_alarm_thresholds(float *channels, size_t count)
{
    if (!alarm_enabled) return;

    for (size_t i = 0; i < count && i < PROBE_MAX_CHANNELS; i++) {
        bool exceeded = false;
        float threshold = 0.0f;
        const char *level = "warning";

        if (channels[i] > alarm_upper_limit) {
            exceeded = true;
            threshold = alarm_upper_limit;
            level = "critical";
        } else if (channels[i] < alarm_lower_limit) {
            exceeded = true;
            threshold = alarm_lower_limit;
            level = "warning";
        }

        if (exceeded && !alarm_active[i]) {
            alarm_active[i] = true;
            mqtt_publish_alarm(level, (uint8_t)i, channels[i], threshold);
        } else if (!exceeded && alarm_active[i]) {
            alarm_active[i] = false;
            ESP_LOGI(TAG, "Alarm cleared: ch%u value=%.3f back in range",
                     (unsigned)i, channels[i]);
        }
    }
}
#endif

/**
 * @brief Publish a structured JSON response to a command.
 */
static void mqtt_publish_response(const char *cmd, const char *status,
                                  const char *message)
{
    if (!mqtt_connected) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "cmd",     cmd);
    cJSON_AddStringToObject(root, "status",  status);
    cJSON_AddStringToObject(root, "message", message);
    cJSON_AddNumberToObject(root, "timestamp",
                            (double)(esp_timer_get_time() / 1000));

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload) {
        esp_mqtt_client_publish(
            mqtt_client,
            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/response",
            payload, 0, 0, 0);
        ESP_LOGI(TAG, "Response: %s", payload);
        cJSON_free(payload);
    }
}

/* ================================================================
 *  cJSON-based command handler
 * ================================================================ */

static void mqtt_handle_command(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG, "Command from %.*s: %.*s",
             (int)strlen(topic), topic, data_len, data);

    /* Parse JSON */
    cJSON *root = cJSON_ParseWithLength(data, (size_t)data_len);
    if (!root) {
        ESP_LOGW(TAG, "Invalid JSON");
        mqtt_publish_response("unknown", "error", "Invalid JSON");
        return;
    }

    cJSON *cmd_item = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    if (!cJSON_IsString(cmd_item) || cmd_item->valuestring == NULL) {
        ESP_LOGW(TAG, "Missing 'cmd' field");
        mqtt_publish_response("unknown", "error", "Missing 'cmd' field");
        cJSON_Delete(root);
        return;
    }

    const char *cmd = cmd_item->valuestring;

    /* ── start ─────────────────────────────────────────── */
    if (strcmp(cmd, "start") == 0) {
        cJSON *params = cJSON_GetObjectItemCaseSensitive(root, "params");
        int rate = 1000;
        if (cJSON_IsObject(params)) {
            cJSON *sr = cJSON_GetObjectItemCaseSensitive(params, "sample_rate");
            if (cJSON_IsNumber(sr)) rate = sr->valueint;
        }
        telemetry_interval_ms = (rate > 0) ? (uint32_t)(1000 / rate) : 1;

        adc_gpio_probe_start_sampling(&probe_dev);

        char msg[96];
        snprintf(msg, sizeof(msg), "Sampling started at %d Hz", rate);
        mqtt_publish_response(cmd, "ok", msg);
        ESP_LOGI(TAG, "%s", msg);
    }
    /* ── stop ──────────────────────────────────────────── */
    else if (strcmp(cmd, "stop") == 0) {
        adc_gpio_probe_stop_sampling(&probe_dev);
        mqtt_publish_response(cmd, "ok", "Sampling stopped");
        ESP_LOGI(TAG, "Sampling stopped");
    }
    /* ── set_gpio ──────────────────────────────────────── */
    else if (strcmp(cmd, "set_gpio") == 0) {
        cJSON *ch_item = cJSON_GetObjectItemCaseSensitive(root, "channel");
        cJSON *val_item = cJSON_GetObjectItemCaseSensitive(root, "value");

        int channel = cJSON_IsNumber(ch_item) ? ch_item->valueint : 4;
        int value   = cJSON_IsNumber(val_item) ? val_item->valueint : 0;

#if CONFIG_HETERO_ENABLE_ADC_GPIO_PROBE
        esp_err_t ret = adc_gpio_probe_set_gpio(&probe_dev, channel, value);
        char msg[96];
        if (ret == ESP_OK) {
            snprintf(msg, sizeof(msg), "GPIO ch%d = %d", channel, value);
            mqtt_publish_response(cmd, "ok", msg);
        } else {
            snprintf(msg, sizeof(msg), "GPIO set failed: %s", esp_err_to_name(ret));
            mqtt_publish_response(cmd, "error", msg);
        }
        ESP_LOGI(TAG, "%s", msg);
#else
        mqtt_publish_response(cmd, "error", "GPIO probe not enabled");
#endif
    }
    /* ── set_alarm ──────────────────────────────────────── */
    else if (strcmp(cmd, "set_alarm") == 0) {
#if CONFIG_HETERO_ENABLE_ALARM
        cJSON *upper = cJSON_GetObjectItemCaseSensitive(root, "upper");
        cJSON *lower = cJSON_GetObjectItemCaseSensitive(root, "lower");
        cJSON *enable = cJSON_GetObjectItemCaseSensitive(root, "enabled");

        if (cJSON_IsNumber(upper)) alarm_upper_limit = (float)upper->valuedouble;
        if (cJSON_IsNumber(lower)) alarm_lower_limit = (float)lower->valuedouble;
        if (cJSON_IsBool(enable))  alarm_enabled = cJSON_IsTrue(enable);

        /* Reset active alarms when thresholds change */
        memset(alarm_active, 0, sizeof(alarm_active));

        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Alarm: enabled=%d upper=%.1f lower=%.1f",
                 alarm_enabled, alarm_upper_limit, alarm_lower_limit);
        mqtt_publish_response(cmd, "ok", msg);
        ESP_LOGI(TAG, "%s", msg);
#else
        mqtt_publish_response(cmd, "error", "Alarm not enabled in config");
#endif
    }
    /* ── unknown ───────────────────────────────────────── */
    else {
        char msg[80];
        snprintf(msg, sizeof(msg), "Unknown command: %s", cmd);
        mqtt_publish_response(cmd, "error", msg);
        ESP_LOGW(TAG, "%s", msg);
    }

    cJSON_Delete(root);
}

/* ================================================================
 *  WiFi exponential-backoff reconnect task
 * ================================================================ */

static void wifi_backoff_task(void *pvParameters)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (wifi_connected) continue;          /* spurious wake */

        wifi_reconnect_cnt++;
        /* Add jitter: pseudo-random 0..JITTER ms */
        int jitter   = esp_random() % WIFI_BACKOFF_JITTER_MS;
        int delay_ms = wifi_backoff_ms + jitter;

        ESP_LOGW(TAG, "WiFi reconnect #%d in %d ms (base %d ms)",
                 wifi_reconnect_cnt, delay_ms, wifi_backoff_ms);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));

        if (!wifi_connected) {
            esp_err_t ret = esp_wifi_connect();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(ret));
                /* Re-notify so we retry */
                xTaskNotifyGive(wifi_backoff_task_handle);
                continue;
            }
        }

        /* Exponential backoff for next attempt */
        wifi_backoff_ms *= 2;
        if (wifi_backoff_ms > WIFI_BACKOFF_MAX_MS)
            wifi_backoff_ms = WIFI_BACKOFF_MAX_MS;
    }
}

/* ================================================================
 *  WiFi event handler (for backoff reconnection)
 * ================================================================ */

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base != WIFI_EVENT) return;

    switch (id) {
    case WIFI_EVENT_STA_DISCONNECTED: {
        wifi_event_sta_disconnected_t *evt =
            (wifi_event_sta_disconnected_t *)data;
        ESP_LOGW(TAG, "WiFi disconnected (reason %d)", evt->reason);

        wifi_connected = false;

        /* Stop MQTT while WiFi is down */
        if (mqtt_client && mqtt_connected) {
            esp_mqtt_client_stop(mqtt_client);
        }

        /* Wake the backoff task */
        if (wifi_backoff_task_handle)
            xTaskNotifyGive(wifi_backoff_task_handle);
        break;
    }

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "WiFi connected");
        wifi_connected   = true;
        wifi_backoff_ms  = WIFI_BACKOFF_BASE_MS;   /* reset backoff */
        break;

    default:
        break;
    }
}

/* ================================================================
 *  MQTT event handler
 * ================================================================ */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_connected = true;

        msg_id = esp_mqtt_client_subscribe(
            client,
            "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/command", 1);
        ESP_LOGI(TAG, "Subscribed to command topic, msg_id=%d", msg_id);

        mqtt_publish_status("online");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT disconnected");
        mqtt_connected = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGD(TAG, "Subscribed msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "Published msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        mqtt_handle_command(event->topic, event->data, event->data_len);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            ESP_LOGE(TAG, "Transport error: 0x%x",
                     event->error_handle->esp_tls_last_esp_err);
        break;

    default:
        break;
    }
}

/* ================================================================
 *  MQTT client start
 * ================================================================ */

static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "MQTT Broker URI: %s", CONFIG_HETERO_MQTT_BROKER_URI);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_HETERO_MQTT_BROKER_URI,
        },
        .session = {
            .protocol_ver = MQTT_PROTOCOL_V_5,
            .last_will = {
                .topic   = "heterolink/subboard/" CONFIG_HETERO_DEVICE_ID "/status",
                .msg     = "offline",
                .msg_len = 7,
                .qos     = 1,
                .retain  = true,
            },
        },
        .network = {
            .timeout_ms        = 10000,
            .reconnect_timeout_ms = 5000,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT client started");
}

/* ================================================================
 *  Component init
 * ================================================================ */

static void components_init(void)
{
    esp_err_t ret;

#if CONFIG_HETERO_ENABLE_SPI_DMA
    spi_dma_config_t spi_config = SPI_DMA_DEFAULT_CONFIG();
    ret = spi_dma_init(&spi_config, &spi_dev);
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "SPI+DMA initialized");
    else
        ESP_LOGE(TAG, "SPI+DMA init failed: %s", esp_err_to_name(ret));
#endif

#if CONFIG_HETERO_ENABLE_ADC_GPIO_PROBE
    adc_gpio_probe_config_t probe_config = ADC_GPIO_PROBE_DEFAULT_CONFIG();
    ret = adc_gpio_probe_init(&probe_config, &probe_dev);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC/GPIO Probe initialized");
        adc_gpio_probe_start_sampling(&probe_dev);
    } else {
        ESP_LOGE(TAG, "ADC/GPIO Probe init failed: %s", esp_err_to_name(ret));
    }
#endif
}

/* ================================================================
 *  Main loop task (telemetry publishing)
 * ================================================================ */

static void main_loop_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Main loop task started");

    while (1) {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

        if (mqtt_connected && probe_dev.sampling &&
            (now - last_telemetry_time >= telemetry_interval_ms))
        {
            last_telemetry_time = now;

#if CONFIG_HETERO_ENABLE_ADC_GPIO_PROBE
            uint32_t values[PROBE_MAX_CHANNELS];
            size_t count = PROBE_MAX_CHANNELS;

            if (adc_gpio_probe_read_all(&probe_dev, values, &count) == ESP_OK) {
                float channels[PROBE_MAX_CHANNELS];
                for (size_t i = 0; i < count; i++)
                    channels[i] = (float)values[i];
                mqtt_publish_telemetry(channels, count);
#if CONFIG_HETERO_ENABLE_ALARM
                check_alarm_thresholds(channels, count);
#endif
            }
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ================================================================
 *  app_main
 * ================================================================ */

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  HeteroLink ESP32 Subboard");
    ESP_LOGI(TAG, "  Version: 0.1.0");
    ESP_LOGI(TAG, "  Device ID: %s", CONFIG_HETERO_DEVICE_ID);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    /* ── NVS ── */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ── Network stack ── */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* ── WiFi backoff task (created before connect so it's ready) ── */
    xTaskCreate(wifi_backoff_task, "wifi_backoff", 4096, NULL, 5,
                &wifi_backoff_task_handle);

    /* ── Register custom WiFi event handler BEFORE connect ── */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        wifi_event_handler, NULL, NULL));
    ESP_LOGI(TAG, "WiFi backoff reconnect handler registered");

    /* ── Initial WiFi connection (blocking) ── */
    ESP_LOGI(TAG, "Connecting to WiFi...");
    esp_err_t wifi_ret = example_connect();
    if (wifi_ret == ESP_OK) {
        wifi_connected = true;
        ESP_LOGI(TAG, "WiFi connected!");
    } else {
        ESP_LOGW(TAG, "Initial WiFi connect failed: %s – backoff task will retry",
                 esp_err_to_name(wifi_ret));
        /* Don't abort; the backoff task keeps retrying */
    }

    /* ── Components ── */
    components_init();

    /* ── MQTT (starts even without WiFi; will connect once WiFi is up) ── */
    mqtt_app_start();

    /* ── Telemetry loop ── */
    xTaskCreate(main_loop_task, "main_loop", 4096, NULL, 5, NULL);

#if CONFIG_HETERO_ENABLE_ALARM
    ESP_LOGI(TAG, "Alarm: enabled=%d upper=%.1f lower=%.1f",
             alarm_enabled, alarm_upper_limit, alarm_lower_limit);
#endif

    ESP_LOGI(TAG, "Initialization complete!");
}
