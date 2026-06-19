/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file adc_gpio_probe.h
 * @brief ADC/GPIO 双模点测功能（ESP-IDF v6.0 适配）
 * 
 * 功能：
 * - GPIO 数字逻辑探测（双模：输入/输出）
 * - ADC 模拟信号采集
 * - 零代码即插即用
 */

#ifndef __ADC_GPIO_PROBE_H__
#define __ADC_GPIO_PROBE_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 最大探测通道数
 */
#define PROBE_MAX_CHANNELS  8

/**
 * @brief 探测模式
 */
typedef enum {
    PROBE_MODE_GPIO_INPUT,    /*!< GPIO 输入模式（数字读取） */
    PROBE_MODE_GPIO_OUTPUT,   /*!< GPIO 输出模式（数字写入） */
    PROBE_MODE_ADC,           /*!< ADC 模拟采集模式 */
} probe_mode_t;

/**
 * @brief 通道配置
 */
typedef struct {
    uint8_t channel_num;          /*!< 通道编号 (0-7) */
    probe_mode_t mode;            /*!< 探测模式 */
    
    // GPIO 模式配置
    gpio_num_t gpio_num;          /*!< GPIO 引脚编号 */
    bool pull_up_en;              /*!< 上拉使能 */
    bool pull_down_en;            /*!< 下拉使能 */
    
    // ADC 模式配置
    int adc_unit;                 /*!< ADC 单元 (1 或 2) */
    int adc_channel;              /*!< ADC 通道编号 */
    adc_atten_t attenuation;      /*!< 衰减设置 */
    adc_bitwidth_t bit_width;     /*!< 分辨率 */
} probe_channel_config_t;

/**
 * @brief 通道状态
 */
typedef struct {
    uint8_t channel_num;
    probe_mode_t mode;
    bool enabled;
    union {
        bool gpio_value;          /*!< GPIO 数字值 */
        uint32_t adc_value;       /*!< ADC 采样值 */
    } data;
} probe_channel_state_t;

/**
 * @brief 探测器配置
 */
typedef struct {
    probe_channel_config_t channels[PROBE_MAX_CHANNELS];
    size_t channel_count;
    uint32_t sample_interval_ms;  /*!< 采样间隔（毫秒） */
} adc_gpio_probe_config_t;

/**
 * @brief 探测器句柄
 */
typedef struct {
    adc_gpio_probe_config_t config;
    probe_channel_state_t states[PROBE_MAX_CHANNELS];
    bool initialized;
    bool sampling;
    // ESP-IDF v6.0 ADC 句柄
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_handle_t adc2_handle;
    adc_cali_handle_t cali_handle;
    /* Per-channel calibration handles (NULL for non-ADC channels) */
    adc_cali_handle_t cali_handles[PROBE_MAX_CHANNELS];
} adc_gpio_probe_device_t;

/**
 * @brief 默认配置（8 通道）
 */
#define ADC_GPIO_PROBE_DEFAULT_CONFIG() { \
    .channels = { \
        { .channel_num = 0, .mode = PROBE_MODE_ADC, .adc_unit = 1, \
          .adc_channel = 0, .attenuation = ADC_ATTEN_DB_12, \
          .bit_width = ADC_BITWIDTH_12, .gpio_num = GPIO_NUM_NC }, \
        { .channel_num = 1, .mode = PROBE_MODE_ADC, .adc_unit = 1, \
          .adc_channel = 1, .attenuation = ADC_ATTEN_DB_12, \
          .bit_width = ADC_BITWIDTH_12, .gpio_num = GPIO_NUM_NC }, \
        { .channel_num = 2, .mode = PROBE_MODE_GPIO_INPUT, .gpio_num = GPIO_NUM_1, \
          .pull_up_en = false, .pull_down_en = false }, \
        { .channel_num = 3, .mode = PROBE_MODE_GPIO_INPUT, .gpio_num = GPIO_NUM_2, \
          .pull_up_en = false, .pull_down_en = false }, \
        { .channel_num = 4, .mode = PROBE_MODE_GPIO_OUTPUT, .gpio_num = GPIO_NUM_3, \
          .pull_up_en = false, .pull_down_en = false }, \
        { .channel_num = 5, .mode = PROBE_MODE_GPIO_OUTPUT, .gpio_num = GPIO_NUM_4, \
          .pull_up_en = false, .pull_down_en = false }, \
    }, \
    .channel_count = 6, \
    .sample_interval_ms = 10, \
}

/**
 * @brief 初始化 ADC/GPIO 探测器
 * 
 * @param config 配置结构
 * @param out_handle 输出设备句柄
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_init(const adc_gpio_probe_config_t *config, 
                              adc_gpio_probe_device_t *out_handle);

/**
 * @brief 反初始化探测器
 * 
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_deinit(adc_gpio_probe_device_t *handle);

/**
 * @brief 启动连续采样
 * 
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_start_sampling(adc_gpio_probe_device_t *handle);

/**
 * @brief 停止采样
 * 
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_stop_sampling(adc_gpio_probe_device_t *handle);

/**
 * @brief 读取单个通道值
 * 
 * @param handle 设备句柄
 * @param channel_num 通道编号
 * @param out_value 输出值（数字或模拟）
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_read_channel(adc_gpio_probe_device_t *handle,
                                      uint8_t channel_num,
                                      uint32_t *out_value);

/**
 * @brief 读取所有通道值
 * 
 * @param handle 设备句柄
 * @param values 输出数组
 * @param count 通道数量
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_read_all(adc_gpio_probe_device_t *handle,
                                  uint32_t *values,
                                  size_t *count);

/**
 * @brief 设置 GPIO 输出电平
 * 
 * @param handle 设备句柄
 * @param channel_num 通道编号
 * @param value 输出值（0 或 1）
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_set_gpio(adc_gpio_probe_device_t *handle,
                                  uint8_t channel_num,
                                  bool value);

/**
 * @brief 获取通道状态
 * 
 * @param handle 设备句柄
 * @param channel_num 通道编号
 * @param out_state 输出状态
 * @return esp_err_t
 */
esp_err_t adc_gpio_probe_get_state(adc_gpio_probe_device_t *handle,
                                   uint8_t channel_num,
                                   probe_channel_state_t *out_state);

/**
 * @brief ADC 值转电压（毫伏）
 * 
 * @param adc_value ADC 原始值
 * @param attenuation 衰减设置
 * @return int 电压值（mV）
 */
int adc_gpio_probe_adc_to_mv(uint32_t adc_value, adc_atten_t attenuation);

#ifdef __cplusplus
}
#endif

#endif // __ADC_GPIO_PROBE_H__
