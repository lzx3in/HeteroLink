/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file spi_dma.c
 * @brief SPI+DMA 高速数据传输驱动实现
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "spi_dma.h"

static const char *TAG = "spi_dma";

esp_err_t spi_dma_init(const spi_dma_config_t *config, spi_dma_device_t *out_handle)
{
    if (!config || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing SPI+DMA...");
    ESP_LOGI(TAG, "  Host ID: %d", config->host_id);
    ESP_LOGI(TAG, "  SCLK: GPIO%d, MISO: GPIO%d, MOSI: GPIO%d, CS: GPIO%d", 
             config->sclk_io_num, config->miso_io_num, 
             config->mosi_io_num, config->cs_io_num);
    ESP_LOGI(TAG, "  DMA Channel: %d", config->dma_channel);
    ESP_LOGI(TAG, "  Max Transfer Size: %zu bytes", config->max_transfer_sz);
    ESP_LOGI(TAG, "  Clock Speed: %lu Hz", config->clock_speed_hz);

    // 初始化 SPI 总线
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = config->sclk_io_num,
        .mosi_io_num = config->mosi_io_num,
        .miso_io_num = config->miso_io_num,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = config->max_transfer_sz,
        .flags = 0,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0,
    };

    esp_err_t ret = spi_bus_initialize(config->host_id, &bus_cfg, config->dma_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // 配置 SPI 设备
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = config->clock_speed_hz,
        .mode = 0,
        .spics_io_num = config->cs_io_num,
        .queue_size = 4,
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    // 添加设备到总线
    spi_device_handle_t spi;
    ret = spi_bus_add_device(config->host_id, &dev_cfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(config->host_id);
        return ret;
    }

    // 填充句柄
    out_handle->spi = spi;
    out_handle->config = *config;
    out_handle->initialized = true;

    ESP_LOGI(TAG, "SPI+DMA initialized successfully");
    return ESP_OK;
}

esp_err_t spi_dma_deinit(spi_dma_device_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deinitializing SPI+DMA...");

    spi_bus_remove_device(handle->spi);
    spi_bus_free(handle->config.host_id);
    
    handle->initialized = false;
    
    ESP_LOGI(TAG, "SPI+DMA deinitialized");
    return ESP_OK;
}

esp_err_t spi_dma_send(spi_dma_device_t *handle, const uint8_t *tx_data, size_t tx_len)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!tx_data || tx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t trans = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = tx_len * 8,
        .txlength = tx_len * 8,
        .tx_buffer = tx_data,
        .rx_buffer = NULL,
    };

    esp_err_t ret = spi_device_transmit(handle->spi, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "SPI sent %zu bytes", tx_len);
    return ESP_OK;
}

esp_err_t spi_dma_receive(spi_dma_device_t *handle, uint8_t *rx_data, size_t rx_len)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!rx_data || rx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t trans = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = rx_len * 8,
        .txlength = 0,
        .tx_buffer = NULL,
        .rx_buffer = rx_data,
    };

    esp_err_t ret = spi_device_transmit(handle->spi, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI receive failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "SPI received %zu bytes", rx_len);
    return ESP_OK;
}

esp_err_t spi_dma_transfer(spi_dma_device_t *handle, 
                           const uint8_t *tx_data, 
                           uint8_t *rx_data, 
                           size_t len)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t trans = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = len * 8,
        .txlength = len * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t ret = spi_device_transmit(handle->spi, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transfer failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "SPI transferred %zu bytes (full-duplex)", len);
    return ESP_OK;
}

esp_err_t spi_dma_send_async(spi_dma_device_t *handle, 
                             const uint8_t *tx_data, 
                             size_t tx_len,
                             spi_transaction_t *trans,
                             void *user_data)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!tx_data || tx_len == 0 || !trans) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(trans, 0, sizeof(spi_transaction_t));
    trans->length = tx_len * 8;
    trans->txlength = tx_len * 8;
    trans->tx_buffer = tx_data;
    trans->user = user_data;

    esp_err_t ret = spi_device_queue_trans(handle->spi, trans, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI queue transmit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "SPI async send queued: %zu bytes", tx_len);
    return ESP_OK;
}

int spi_dma_get_descriptor_count(size_t buffer_size)
{
    const int DMA_DESC_SIZE = 4092;  // ESP32 DMA 描述符最大大小
    return (buffer_size + DMA_DESC_SIZE - 1) / DMA_DESC_SIZE;
}
