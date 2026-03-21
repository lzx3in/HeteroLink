/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file adc_gpio_probe.c
 * @brief ADC/GPIO 双模点测功能实现
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "adc_gpio_probe.h"

static const char *TAG = "adc_gpio_probe";

/**
 * @brief ADC 校准特性
 */
static esp_adc_cal_characteristics_t *adc_chars;

int adc_gpio_probe_adc_to_mv(uint32_t adc_value, adc_atten_t attenuation)
{
    if (!adc_chars) {
        // 简单估算：根据衰减计算
        switch (attenuation) {
            case ADC_ATTEN_DB_0:   return (adc_value * 950) / 4095;
            case ADC_ATTEN_DB_2_5: return (adc_value * 1250) / 4095;
            case ADC_ATTEN_DB_6:   return (adc_value * 1750) / 4095;
            case ADC_ATTEN_DB_11:  return (adc_value * 2450) / 4095;
            default: return (adc_value * 2450) / 4095;
        }
    }
    
    return esp_adc_cal_raw_to_voltage(adc_value, adc_chars);
}

esp_err_t adc_gpio_probe_init(const adc_gpio_probe_config_t *config, 
                              adc_gpio_probe_device_t *out_handle)
{
    if (!config || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing ADC/GPIO Probe...");
    ESP_LOGI(TAG, "  Channel Count: %zu", config->channel_count);
    ESP_LOGI(TAG, "  Sample Interval: %lu ms", config->sample_interval_ms);

    // 初始化 ADC 校准
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    if (!adc_chars) {
        ESP_LOGE(TAG, "Failed to allocate ADC characteristics");
        return ESP_ERR_NO_MEM;
    }
    
    // 配置 ADC1（假设使用 ADC1）
    adc1_config_width(config->channels[0].bit_width);
    
    for (size_t i = 0; i < config->channel_count; i++) {
        const probe_channel_config_t *ch = &config->channels[i];
        
        if (ch->mode == PROBE_MODE_ADC) {
            // 配置 ADC 通道
            adc1_config_channel_atten(ch->adc_channel, ch->attenuation);
            ESP_LOGI(TAG, "  CH%u: ADC (GPIO%d, ATTEN=%d)", 
                     ch->channel_num, ch->gpio_num, ch->attenuation);
        } else {
            // 配置 GPIO
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << ch->gpio_num),
                .mode = (ch->mode == PROBE_MODE_GPIO_INPUT) ? 
                        GPIO_MODE_INPUT : GPIO_MODE_OUTPUT,
                .pull_up_en = ch->pull_up_en ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
                .pull_down_en = ch->pull_down_en ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            
            esp_err_t ret = gpio_config(&io_conf);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure GPIO%d: %s", ch->gpio_num, esp_err_to_name(ret));
                free(adc_chars);
                return ret;
            }
            
            ESP_LOGI(TAG, "  CH%u: GPIO%d (%s)", 
                     ch->channel_num, ch->gpio_num,
                     ch->mode == PROBE_MODE_GPIO_INPUT ? "INPUT" : "OUTPUT");
        }
        
        // 初始化通道状态
        out_handle->states[i].channel_num = ch->channel_num;
        out_handle->states[i].mode = ch->mode;
        out_handle->states[i].enabled = true;
        out_handle->states[i].data.gpio_value = false;
    }

    // 复制配置
    out_handle->config = *config;
    out_handle->initialized = true;
    out_handle->sampling = false;

    ESP_LOGI(TAG, "ADC/GPIO Probe initialized successfully");
    return ESP_OK;
}

esp_err_t adc_gpio_probe_deinit(adc_gpio_probe_device_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deinitializing ADC/GPIO Probe...");
    
    if (adc_chars) {
        free(adc_chars);
        adc_chars = NULL;
    }
    
    handle->initialized = false;
    handle->sampling = false;
    
    ESP_LOGI(TAG, "ADC/GPIO Probe deinitialized");
    return ESP_OK;
}

esp_err_t adc_gpio_probe_start_sampling(adc_gpio_probe_device_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting continuous sampling...");
    handle->sampling = true;
    return ESP_OK;
}

esp_err_t adc_gpio_probe_stop_sampling(adc_gpio_probe_device_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Stopping sampling...");
    handle->sampling = false;
    return ESP_OK;
}

esp_err_t adc_gpio_probe_read_channel(adc_gpio_probe_device_t *handle,
                                      uint8_t channel_num,
                                      uint32_t *out_value)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!out_value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 查找通道
    probe_channel_state_t *state = NULL;
    for (size_t i = 0; i < handle->config.channel_count; i++) {
        if (handle->states[i].channel_num == channel_num) {
            state = &handle->states[i];
            break;
        }
    }
    
    if (!state || !state->enabled) {
        ESP_LOGE(TAG, "Channel %u not found or disabled", channel_num);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 根据模式读取
    if (state->mode == PROBE_MODE_ADC) {
        // 找到对应的 ADC 通道配置
        const probe_channel_config_t *ch_config = NULL;
        for (size_t i = 0; i < handle->config.channel_count; i++) {
            if (handle->config.channels[i].channel_num == channel_num) {
                ch_config = &handle->config.channels[i];
                break;
            }
        }
        
        if (!ch_config) {
            return ESP_ERR_NOT_FOUND;
        }
        
        // 读取 ADC 值
        uint32_t adc_value = adc1_get_raw(ch_config->adc_channel);
        state->data.adc_value = adc_value;
        *out_value = adc_value;
        
        ESP_LOGD(TAG, "CH%u ADC: %lu", channel_num, adc_value);
    } else {
        // GPIO 模式
        bool value = gpio_get_level(handle->config.channels[channel_num].gpio_num);
        state->data.gpio_value = value;
        *out_value = value ? 1 : 0;
        
        ESP_LOGD(TAG, "CH%u GPIO: %d", channel_num, value);
    }
    
    return ESP_OK;
}

esp_err_t adc_gpio_probe_read_all(adc_gpio_probe_device_t *handle,
                                  uint32_t *values,
                                  size_t *count)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!values || !count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t actual_count = 0;
    for (size_t i = 0; i < handle->config.channel_count; i++) {
        if (handle->states[i].enabled) {
            esp_err_t ret = adc_gpio_probe_read_channel(handle, 
                                                        handle->states[i].channel_num, 
                                                        &values[actual_count]);
            if (ret == ESP_OK) {
                actual_count++;
            }
        }
    }
    
    *count = actual_count;
    return ESP_OK;
}

esp_err_t adc_gpio_probe_set_gpio(adc_gpio_probe_device_t *handle,
                                  uint8_t channel_num,
                                  bool value)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 查找通道
    const probe_channel_config_t *ch_config = NULL;
    for (size_t i = 0; i < handle->config.channel_count; i++) {
        if (handle->config.channels[i].channel_num == channel_num &&
            handle->config.channels[i].mode == PROBE_MODE_GPIO_OUTPUT) {
            ch_config = &handle->config.channels[i];
            break;
        }
    }
    
    if (!ch_config) {
        ESP_LOGE(TAG, "Channel %u is not a GPIO output", channel_num);
        return ESP_ERR_NOT_FOUND;
    }
    
    gpio_set_level(ch_config->gpio_num, value ? 1 : 0);
    ESP_LOGD(TAG, "CH%u GPIO set to %d", channel_num, value);
    
    return ESP_OK;
}

esp_err_t adc_gpio_probe_get_state(adc_gpio_probe_device_t *handle,
                                   uint8_t channel_num,
                                   probe_channel_state_t *out_state)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!out_state) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (size_t i = 0; i < handle->config.channel_count; i++) {
        if (handle->states[i].channel_num == channel_num) {
            *out_state = handle->states[i];
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}
