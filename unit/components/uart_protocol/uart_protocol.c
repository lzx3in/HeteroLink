/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uart_protocol.c
 * @brief UART 自定义二进制协议栈实现
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "uart_protocol.h"

static const char *TAG = "uart_protocol";

/**
 * @brief CRC16/MODBUS 查找表
 */
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t uart_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }
    return crc;
}

esp_err_t uart_protocol_init(const uart_protocol_config_t *config, 
                             uart_protocol_device_t *out_handle)
{
    if (!config || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing UART Protocol...");
    ESP_LOGI(TAG, "  UART Port: %d", config->uart_num);
    ESP_LOGI(TAG, "  TX: GPIO%d, RX: GPIO%d", config->tx_io_num, config->rx_io_num);
    ESP_LOGI(TAG, "  Baud Rate: %lu", config->baud_rate);

    // 配置 UART 参数
    uart_config_t uart_config = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 安装 UART 驱动
    esp_err_t ret = uart_driver_install(config->uart_num, 
                                        config->rx_buffer_size, 
                                        config->tx_buffer_size, 
                                        0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(config->uart_num, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        uart_driver_delete(config->uart_num);
        return ret;
    }

    ret = uart_set_pin(config->uart_num, 
                       config->tx_io_num, 
                       config->rx_io_num, 
                       config->rts_io_num, 
                       config->cts_io_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        uart_driver_delete(config->uart_num);
        return ret;
    }

    // 填充句柄
    out_handle->config = *config;
    out_handle->initialized = true;
    memset(&out_handle->rx_frame, 0, sizeof(uart_frame_t));
    memset(&out_handle->tx_frame, 0, sizeof(uart_frame_t));

    ESP_LOGI(TAG, "UART Protocol initialized successfully");
    return ESP_OK;
}

esp_err_t uart_protocol_deinit(uart_protocol_device_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deinitializing UART Protocol...");
    uart_driver_delete(handle->config.uart_num);
    handle->initialized = false;
    
    ESP_LOGI(TAG, "UART Protocol deinitialized");
    return ESP_OK;
}

esp_err_t uart_protocol_send(uart_protocol_device_t *handle, 
                             uint8_t cmd, 
                             const uint8_t *payload, 
                             size_t length)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (length > UART_MAX_PAYLOAD_SIZE) {
        ESP_LOGE(TAG, "Payload too large: %zu > %d", length, UART_MAX_PAYLOAD_SIZE);
        return ESP_ERR_INVALID_ARG;
    }

    // 构建帧
    handle->tx_frame.preamble = UART_FRAME_PREAMBLE;
    handle->tx_frame.cmd = cmd;
    handle->tx_frame.length = length;
    
    if (payload && length > 0) {
        memcpy(handle->tx_frame.payload, payload, length);
    }
    
    // 计算 CRC（从 cmd 到 payload 结束）
    uint8_t crc_data[4 + UART_MAX_PAYLOAD_SIZE];
    crc_data[0] = handle->tx_frame.cmd;
    crc_data[1] = handle->tx_frame.length & 0xFF;
    crc_data[2] = (handle->tx_frame.length >> 8) & 0xFF;
    if (length > 0) {
        memcpy(&crc_data[3], payload, length);
    }
    handle->tx_frame.crc = uart_crc16(crc_data, 3 + length);
    handle->tx_frame.postamble = UART_FRAME_POSTAMBLE;

    // 发送帧
    size_t frame_size = 6 + length;  // preamble(1) + cmd(1) + length(2) + payload(N) + crc(2) + postamble(1)
    uint8_t frame_buffer[6 + UART_MAX_PAYLOAD_SIZE];
    
    frame_buffer[0] = handle->tx_frame.preamble;
    frame_buffer[1] = handle->tx_frame.cmd;
    frame_buffer[2] = handle->tx_frame.length & 0xFF;
    frame_buffer[3] = (handle->tx_frame.length >> 8) & 0xFF;
    if (length > 0) {
        memcpy(&frame_buffer[4], payload, length);
    }
    frame_buffer[4 + length] = handle->tx_frame.crc & 0xFF;
    frame_buffer[5 + length] = (handle->tx_frame.crc >> 8) & 0xFF;
    frame_buffer[6 + length] = handle->tx_frame.postamble;

    int sent = uart_write_bytes(handle->config.uart_num, frame_buffer, frame_size);
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to write UART data");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "UART sent frame: cmd=0x%02X, length=%zu, crc=0x%04X", cmd, length, handle->tx_frame.crc);
    return ESP_OK;
}

esp_err_t uart_protocol_receive(uart_protocol_device_t *handle, 
                                TickType_t timeout_ms)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t byte;
    size_t idx = 0;
    uint8_t frame_buffer[6 + UART_MAX_PAYLOAD_SIZE];
    
    // 等待帧头
    TickType_t start = xTaskGetTickCount();
    while (1) {
        if (uart_read_bytes(handle->config.uart_num, &byte, 1, pdMS_TO_TICKS(10)) <= 0) {
            // 检查超时
            if ((xTaskGetTickCount() - start) > timeout_ms) {
                return ESP_ERR_TIMEOUT;
            }
            continue;
        }
        
        if (byte == UART_FRAME_PREAMBLE) {
            frame_buffer[idx++] = byte;
            break;
        }
    }
    
    // 接收剩余帧数据（最少还需要 5 字节：cmd + length(2) + crc(2) + postamble）
    while (idx < 6) {
        if (uart_read_bytes(handle->config.uart_num, &byte, 1, pdMS_TO_TICKS(100)) <= 0) {
            return ESP_ERR_TIMEOUT;
        }
        frame_buffer[idx++] = byte;
    }
    
    // 解析长度
    uint16_t payload_len = frame_buffer[2] | (frame_buffer[3] << 8);
    if (payload_len > UART_MAX_PAYLOAD_SIZE) {
        ESP_LOGE(TAG, "Invalid payload length: %d", payload_len);
        return ESP_ERR_INVALID_SIZE;
    }
    
    // 接收载荷
    while (idx < 6 + payload_len) {
        if (uart_read_bytes(handle->config.uart_num, &byte, 1, pdMS_TO_TICKS(100)) <= 0) {
            return ESP_ERR_TIMEOUT;
        }
        frame_buffer[idx++] = byte;
    }
    
    // 接收帧尾
    while (idx < 7 + payload_len) {
        if (uart_read_bytes(handle->config.uart_num, &byte, 1, pdMS_TO_TICKS(100)) <= 0) {
            return ESP_ERR_TIMEOUT;
        }
        frame_buffer[idx++] = byte;
    }
    
    // 验证帧尾
    if (frame_buffer[idx - 1] != UART_FRAME_POSTAMBLE) {
        ESP_LOGE(TAG, "Invalid postamble: 0x%02X", frame_buffer[idx - 1]);
        return ESP_ERR_INVALID_CRC;
    }
    
    // 验证 CRC
    uint16_t received_crc = frame_buffer[4 + payload_len] | (frame_buffer[5 + payload_len] << 8);
    uint16_t calculated_crc = uart_crc16(&frame_buffer[1], 3 + payload_len);
    
    if (received_crc != calculated_crc) {
        ESP_LOGE(TAG, "CRC mismatch: received=0x%04X, calculated=0x%04X", received_crc, calculated_crc);
        return ESP_ERR_INVALID_CRC;
    }
    
    // 保存到句柄
    handle->rx_frame.preamble = frame_buffer[0];
    handle->rx_frame.cmd = frame_buffer[1];
    handle->rx_frame.length = payload_len;
    if (payload_len > 0) {
        memcpy(handle->rx_frame.payload, &frame_buffer[4], payload_len);
    }
    handle->rx_frame.crc = received_crc;
    handle->rx_frame.postamble = frame_buffer[6 + payload_len];
    
    ESP_LOGD(TAG, "UART received frame: cmd=0x%02X, length=%d, crc=0x%04X", 
             handle->rx_frame.cmd, handle->rx_frame.length, handle->rx_frame.crc);
    
    return ESP_OK;
}

esp_err_t uart_protocol_parse(uart_protocol_device_t *handle,
                              uint8_t *out_cmd,
                              const uint8_t **out_payload,
                              size_t *out_length)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (out_cmd) {
        *out_cmd = handle->rx_frame.cmd;
    }
    
    if (out_payload) {
        *out_payload = handle->rx_frame.payload;
    }
    
    if (out_length) {
        *out_length = handle->rx_frame.length;
    }
    
    return ESP_OK;
}

esp_err_t uart_protocol_send_heartbeat(uart_protocol_device_t *handle)
{
    return uart_protocol_send(handle, UART_RSP_HEARTBEAT, NULL, 0);
}

esp_err_t uart_protocol_send_telemetry(uart_protocol_device_t *handle,
                                       uint32_t timestamp,
                                       const float *channels,
                                       size_t channel_count)
{
    if (!channels || channel_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 构建载荷：timestamp(4B) + channels(N*4B)
    size_t payload_size = 4 + channel_count * 4;
    uint8_t payload[payload_size];
    
    // 小端编码时间戳
    payload[0] = timestamp & 0xFF;
    payload[1] = (timestamp >> 8) & 0xFF;
    payload[2] = (timestamp >> 16) & 0xFF;
    payload[3] = (timestamp >> 24) & 0xFF;
    
    // 编码通道数据（IEEE 754 float，小端）
    for (size_t i = 0; i < channel_count; i++) {
        uint32_t float_bits;
        memcpy(&float_bits, &channels[i], sizeof(float));
        payload[4 + i * 4 + 0] = float_bits & 0xFF;
        payload[4 + i * 4 + 1] = (float_bits >> 8) & 0xFF;
        payload[4 + i * 4 + 2] = (float_bits >> 16) & 0xFF;
        payload[4 + i * 4 + 3] = (float_bits >> 24) & 0xFF;
    }
    
    return uart_protocol_send(handle, UART_RSP_TELEMETRY, payload, payload_size);
}

esp_err_t uart_protocol_send_error(uart_protocol_device_t *handle,
                                   uint8_t error_code,
                                   const char *message)
{
    if (!message) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t msg_len = strlen(message);
    if (msg_len > 255) {
        msg_len = 255;  // 限制消息长度
    }
    
    // 载荷：error_code(1B) + message(NB)
    uint8_t payload[256];
    payload[0] = error_code;
    memcpy(&payload[1], message, msg_len);
    
    return uart_protocol_send(handle, UART_RSP_ERROR, payload, 1 + msg_len);
}
