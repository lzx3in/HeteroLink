/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uart_protocol.h
 * @brief UART 自定义二进制协议栈
 * 
 * 帧格式:
 * ┌────────┬────────┬────────┬──────────┬────────┬──────────┐
 * │ Preamble│ Type   │ Length │ Payload  │ CRC16  │ Postamble│
 * │ 0xAA   │ 1 byte │ 2 bytes│ N bytes  │ 2 bytes│ 0x55     │
 * └────────┴────────┴────────┴──────────┴────────┴──────────┘
 */

#ifndef __UART_PROTOCOL_H__
#define __UART_PROTOCOL_H__

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 帧头标志
 */
#define UART_FRAME_PREAMBLE   0xAA
#define UART_FRAME_POSTAMBLE  0x55

/**
 * @brief 最大载荷大小
 */
#define UART_MAX_PAYLOAD_SIZE  1024

/**
 * @brief 命令字定义（主机 → 设备）
 */
typedef enum {
    UART_CMD_HEARTBEAT      = 0x01,  /*!< 心跳请求 */
    UART_CMD_CONFIG_REQ     = 0x02,  /*!< 请求设备配置 */
    UART_CMD_CONTROL        = 0x20,  /*!< 控制命令 */
    UART_CMD_START_ACQ      = 0x21,  /*!< 开始采集 */
    UART_CMD_STOP_ACQ       = 0x22,  /*!< 停止采集 */
    UART_CMD_SET_SAMPLE_RATE = 0x23, /*!< 设置采样率 */
    UART_CMD_SET_CHANNELS   = 0x24,  /*!< 设置通道 */
} uart_cmd_t;

/**
 * @brief 响应命令字定义（设备 → 主机）
 */
typedef enum {
    UART_RSP_HEARTBEAT      = 0x01,  /*!< 心跳响应 */
    UART_RSP_CONFIG_RSP     = 0x03,  /*!< 配置响应 */
    UART_RSP_TELEMETRY      = 0x10,  /*!< 遥测数据 */
    UART_RSP_ERROR          = 0xFF,  /*!< 错误报告 */
} uart_rsp_t;

/**
 * @brief UART 帧结构
 */
typedef struct {
    uint8_t preamble;       /*!< 帧头 0xAA */
    uint8_t cmd;            /*!< 命令字 */
    uint16_t length;        /*!< 载荷长度 */
    uint8_t payload[UART_MAX_PAYLOAD_SIZE]; /*!< 载荷数据 */
    uint16_t crc;           /*!< CRC16 校验 */
    uint8_t postamble;      /*!< 帧尾 0x55 */
} uart_frame_t;

/**
 * @brief UART 协议配置
 */
typedef struct {
    uart_port_t uart_num;       /*!< UART 端口号 */
    int tx_io_num;              /*!< TX 引脚 */
    int rx_io_num;              /*!< RX 引脚 */
    int rts_io_num;             /*!< RTS 引脚 (-1 表示不用) */
    int cts_io_num;             /*!< CTS 引脚 (-1 表示不用) */
    uint32_t baud_rate;         /*!< 波特率 */
    size_t rx_buffer_size;      /*!< RX 缓冲区大小 */
    size_t tx_buffer_size;      /*!< TX 缓冲区大小 */
} uart_protocol_config_t;

/**
 * @brief UART 协议设备句柄
 */
typedef struct {
    uart_protocol_config_t config;
    uart_frame_t rx_frame;
    uart_frame_t tx_frame;
    bool initialized;
} uart_protocol_device_t;

/**
 * @brief 默认配置
 */
#define UART_PROTOCOL_DEFAULT_CONFIG() { \
    .uart_num = UART_NUM_0, \
    .tx_io_num = GPIO_NUM_16, \
    .rx_io_num = GPIO_NUM_17, \
    .rts_io_num = -1, \
    .cts_io_num = -1, \
    .baud_rate = 921600, \
    .rx_buffer_size = 2048, \
    .tx_buffer_size = 1024, \
}

/**
 * @brief 计算 CRC16 (MODBUS)
 * 
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return uint16_t CRC16 值
 */
uint16_t uart_crc16(const uint8_t *data, size_t length);

/**
 * @brief 初始化 UART 协议栈
 * 
 * @param config 配置结构
 * @param out_handle 输出设备句柄
 * @return esp_err_t
 */
esp_err_t uart_protocol_init(const uart_protocol_config_t *config, 
                             uart_protocol_device_t *out_handle);

/**
 * @brief 反初始化 UART 协议栈
 * 
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t uart_protocol_deinit(uart_protocol_device_t *handle);

/**
 * @brief 发送帧
 * 
 * @param handle 设备句柄
 * @param cmd 命令字
 * @param payload 载荷数据
 * @param length 载荷长度
 * @return esp_err_t
 */
esp_err_t uart_protocol_send(uart_protocol_device_t *handle, 
                             uint8_t cmd, 
                             const uint8_t *payload, 
                             size_t length);

/**
 * @brief 接收帧（阻塞模式）
 * 
 * @param handle 设备句柄
 * @param timeout_ms 超时时间（毫秒）
 * @return esp_err_t ESP_OK 表示成功接收，ESP_ERR_TIMEOUT 表示超时
 */
esp_err_t uart_protocol_receive(uart_protocol_device_t *handle, 
                                TickType_t timeout_ms);

/**
 * @brief 解析接收到的帧
 * 
 * @param handle 设备句柄
 * @param out_cmd 输出命令字
 * @param out_payload 输出载荷指针
 * @param out_length 输出载荷长度
 * @return esp_err_t
 */
esp_err_t uart_protocol_parse(uart_protocol_device_t *handle,
                              uint8_t *out_cmd,
                              const uint8_t **out_payload,
                              size_t *out_length);

/**
 * @brief 发送心跳响应
 * 
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t uart_protocol_send_heartbeat(uart_protocol_device_t *handle);

/**
 * @brief 发送遥测数据
 * 
 * @param handle 设备句柄
 * @param timestamp 时间戳（毫秒）
 * @param channels 通道数据数组（float）
 * @param channel_count 通道数量
 * @return esp_err_t
 */
esp_err_t uart_protocol_send_telemetry(uart_protocol_device_t *handle,
                                       uint32_t timestamp,
                                       const float *channels,
                                       size_t channel_count);

/**
 * @brief 发送错误报告
 * 
 * @param handle 设备句柄
 * @param error_code 错误代码
 * @param message 错误消息
 * @return esp_err_t
 */
esp_err_t uart_protocol_send_error(uart_protocol_device_t *handle,
                                   uint8_t error_code,
                                   const char *message);

#ifdef __cplusplus
}
#endif

#endif // __UART_PROTOCOL_H__
