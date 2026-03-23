/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file spi_dma.h
 * @brief SPI+DMA 高速数据传输驱动
 * 
 * 功能：
 * - 使用 ESP32 SPI 主机模式 + DMA 实现高速数据传输
 * - 支持全双工通信
 * - 零拷贝数据传输
 */

#ifndef __SPI_DMA_H__
#define __SPI_DMA_H__

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI DMA 配置结构
 */
typedef struct {
    int host_id;                    /*!< SPI 主机 ID (SPI2_HOST 或 SPI3_HOST) */
    int sclk_io_num;                /*!< SPI 时钟引脚 */
    int miso_io_num;                /*!< MISO 引脚 */
    int mosi_io_num;                /*!< MOSI 引脚 */
    int cs_io_num;                  /*!< 片选引脚 */
    int dma_channel;                /*!< DMA 通道 (1 或 2) */
    size_t max_transfer_sz;         /*!< 最大传输字节数 */
    uint32_t clock_speed_hz;        /*!< SPI 时钟频率 (Hz) */
} spi_dma_config_t;

/**
 * @brief SPI DMA 设备句柄
 */
typedef struct {
    spi_device_handle_t spi;
    spi_dma_config_t config;
    bool initialized;
} spi_dma_device_t;

/**
 * @brief 默认配置（ESP32-C6）
 */
#define SPI_DMA_DEFAULT_CONFIG() { \
    .host_id = SPI2_HOST, \
    .sclk_io_num = GPIO_NUM_7, \
    .miso_io_num = GPIO_NUM_8, \
    .mosi_io_num = GPIO_NUM_9, \
    .cs_io_num = GPIO_NUM_10, \
    .dma_channel = 2, \
    .max_transfer_sz = 4096, \
    .clock_speed_hz = 10 * 1000 * 1000, \
}

/**
 * @brief 初始化 SPI+DMA 驱动
 * 
 * @param config 配置结构
 * @param out_handle 输出设备句柄
 * @return esp_err_t
 */
esp_err_t spi_dma_init(const spi_dma_config_t *config, spi_dma_device_t *out_handle);

/**
 * @brief 反初始化 SPI+DMA 驱动
 * 
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t spi_dma_deinit(spi_dma_device_t *handle);

/**
 * @brief 发送数据（阻塞模式）
 * 
 * @param handle 设备句柄
 * @param tx_data 发送数据缓冲区
 * @param tx_len 发送数据长度
 * @return esp_err_t
 */
esp_err_t spi_dma_send(spi_dma_device_t *handle, const uint8_t *tx_data, size_t tx_len);

/**
 * @brief 接收数据（阻塞模式）
 * 
 * @param handle 设备句柄
 * @param rx_data 接收数据缓冲区
 * @param rx_len 接收数据长度
 * @return esp_err_t
 */
esp_err_t spi_dma_receive(spi_dma_device_t *handle, uint8_t *rx_data, size_t rx_len);

/**
 * @brief 全双工传输（阻塞模式）
 * 
 * @param handle 设备句柄
 * @param tx_data 发送数据缓冲区（可为 NULL）
 * @param rx_data 接收数据缓冲区（可为 NULL）
 * @param len 传输长度
 * @return esp_err_t
 */
esp_err_t spi_dma_transfer(spi_dma_device_t *handle, 
                           const uint8_t *tx_data, 
                           uint8_t *rx_data, 
                           size_t len);

/**
 * @brief 异步发送（DMA 模式）
 * 
 * @param handle 设备句柄
 * @param tx_data 发送数据缓冲区
 * @param tx_len 发送数据长度
 * @param callback 传输完成回调
 * @param user_data 用户数据
 * @return esp_err_t
 */
esp_err_t spi_dma_send_async(spi_dma_device_t *handle, 
                             const uint8_t *tx_data, 
                             size_t tx_len,
                             spi_transaction_t *trans,
                             void *user_data);

/**
 * @brief 获取 DMA 描述符数量
 * 
 * @param buffer_size 缓冲区大小
 * @return int 描述符数量
 */
int spi_dma_get_descriptor_count(size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // __SPI_DMA_H__
