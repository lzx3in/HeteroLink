/*
 * SPDX-FileCopyrightText: 2026 HeteroLink Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file adc_gpio_probe.c
 * @brief ADC/GPIO 双模点测功能实现（ESP-IDF v6.0 适配）
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adc_gpio_probe.h"

// ESP-IDF v6.0 ADC 校准 API 因芯片而异：
// esp32                                     → line_fitting
// esp32s2, esp32s3, esp32c3, esp32c6       → curve_fitting
#if defined(CONFIG_IDF_TARGET_ESP32)
#define ADC_CALI_USE_LINE_FITTING 1
#else
#define ADC_CALI_USE_LINE_FITTING 0
#endif

static const char *TAG = "adc_gpio_probe";

int adc_gpio_probe_adc_to_mv(uint32_t adc_value, adc_atten_t attenuation)
{
    // 简单估算：根据衰减计算最大电压
    // ESP32-C6 ADC 参考电压约 3.3V
    // ESP-IDF v6.0: ADC_ATTEN_DB_12 替代了 ADC_ATTEN_DB_11
    switch (attenuation) {
        case ADC_ATTEN_DB_0:   return (adc_value * 950) / 4095;
        case ADC_ATTEN_DB_2_5: return (adc_value * 1250) / 4095;
        case ADC_ATTEN_DB_6:   return (adc_value * 1750) / 4095;
        case ADC_ATTEN_DB_12:  return (adc_value * 2450) / 4095;
        default: return (adc_value * 2450) / 4095;
    }
}

esp_err_t adc_gpio_probe_init(const adc_gpio_probe_config_t *config, 
                              adc_gpio_probe_device_t *out_handle)
{
    if (!config || !out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing ADC/GPIO Probe (ESP-IDF v6.0)...");
    ESP_LOGI(TAG, "  Channel Count: %zu", config->channel_count);
    ESP_LOGI(TAG, "  Sample Interval: %lu ms", config->sample_interval_ms);

    // 初始化句柄
    memset(out_handle, 0, sizeof(adc_gpio_probe_device_t));
    out_handle->adc1_handle = NULL;
    out_handle->adc2_handle = NULL;
    out_handle->cali_handle = NULL;
    for (size_t i = 0; i < PROBE_MAX_CHANNELS; i++) {
        out_handle->cali_handles[i] = NULL;
    }

    // 检查是否需要 ADC
    bool need_adc1 = false;
    bool need_adc2 = false;
    
    for (size_t i = 0; i < config->channel_count; i++) {
        if (config->channels[i].mode == PROBE_MODE_ADC) {
            if (config->channels[i].adc_unit == 1) {
                need_adc1 = true;
            } else {
                need_adc2 = true;
            }
        }
    }

    // 初始化 ADC1
    if (need_adc1) {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
        };
        esp_err_t ret = adc_oneshot_new_unit(&init_config, &out_handle->adc1_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize ADC1 unit: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "  ADC1 unit initialized");
    }

    // 初始化 ADC2（如果需要）
    if (need_adc2) {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_2,
        };
        esp_err_t ret = adc_oneshot_new_unit(&init_config, &out_handle->adc2_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize ADC2 unit: %s", esp_err_to_name(ret));
            if (out_handle->adc1_handle) {
                adc_oneshot_del_unit(out_handle->adc1_handle);
            }
            return ret;
        }
        ESP_LOGI(TAG, "  ADC2 unit initialized");
    }

    // 配置 ADC 通道和校准
    for (size_t i = 0; i < config->channel_count; i++) {
        const probe_channel_config_t *ch = &config->channels[i];
        
        if (ch->mode == PROBE_MODE_ADC) {
            // 配置 ADC 通道
            adc_oneshot_chan_cfg_t chan_config = {
                .atten = ch->attenuation,
                .bitwidth = ch->bit_width,
            };
            
            esp_err_t ret;
            if (ch->adc_unit == 1) {
                ret = adc_oneshot_config_channel(out_handle->adc1_handle, 
                                                  ch->adc_channel, &chan_config);
            } else {
                ret = adc_oneshot_config_channel(out_handle->adc2_handle, 
                                                  ch->adc_channel, &chan_config);
            }
            
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure ADC channel %d: %s", 
                         ch->adc_channel, esp_err_to_name(ret));
                if (out_handle->adc1_handle) {
                    adc_oneshot_del_unit(out_handle->adc1_handle);
                }
                if (out_handle->adc2_handle) {
                    adc_oneshot_del_unit(out_handle->adc2_handle);
                }
                return ret;
            }
            
            ESP_LOGI(TAG, "  CH%u: ADC (unit=%d, ch=%d, ATTEN=%d)", 
                     ch->channel_num, ch->adc_unit, ch->adc_channel, ch->attenuation);
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
                if (out_handle->adc1_handle) {
                    adc_oneshot_del_unit(out_handle->adc1_handle);
                }
                if (out_handle->adc2_handle) {
                    adc_oneshot_del_unit(out_handle->adc2_handle);
                }
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

    // 为每个 ADC 通道创建校准句柄
    for (size_t i = 0; i < config->channel_count; i++) {
        const probe_channel_config_t *ch = &config->channels[i];
        if (ch->mode != PROBE_MODE_ADC) continue;

#if ADC_CALI_USE_LINE_FITTING
        // line_fitting: 校准针对整个 ADC 单元，无 chan 字段
        adc_cali_line_fitting_config_t cali_cfg = {
            .unit_id  = (ch->adc_unit == 1) ? ADC_UNIT_1 : ADC_UNIT_2,
            .atten    = ch->attenuation,
            .bitwidth = ch->bit_width,
        };

        adc_cali_handle_t cali = NULL;
        esp_err_t cali_ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &cali);
#else
        adc_cali_curve_fitting_config_t cali_cfg = {
            .unit_id  = (ch->adc_unit == 1) ? ADC_UNIT_1 : ADC_UNIT_2,
            .chan     = ch->adc_channel,
            .atten    = ch->attenuation,
            .bitwidth = ch->bit_width,
        };

        adc_cali_handle_t cali = NULL;
        esp_err_t cali_ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali);
#endif
        if (cali_ret == ESP_OK) {
            out_handle->cali_handles[i] = cali;
            ESP_LOGI(TAG, "  CH%u: ADC calibration created", ch->channel_num);
        } else {
            ESP_LOGW(TAG, "  CH%u: ADC calibration not available: %s", ch->channel_num, esp_err_to_name(cali_ret));
        }
    }

    // 复制配置
    out_handle->config = *config;
    out_handle->initialized = true;
    out_handle->sampling = false;

    ESP_LOGI(TAG, "ADC/GPIO Probe initialized successfully (ESP-IDF v6.0)");
    return ESP_OK;
}

esp_err_t adc_gpio_probe_deinit(adc_gpio_probe_device_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deinitializing ADC/GPIO Probe...");
    
    // 删除 ADC 句柄
    if (handle->adc1_handle) {
        adc_oneshot_del_unit(handle->adc1_handle);
        handle->adc1_handle = NULL;
    }
    if (handle->adc2_handle) {
        adc_oneshot_del_unit(handle->adc2_handle);
        handle->adc2_handle = NULL;
    }
    // 删除校准句柄
    for (size_t i = 0; i < PROBE_MAX_CHANNELS; i++) {
        if (handle->cali_handles[i]) {
#if ADC_CALI_USE_LINE_FITTING
            adc_cali_delete_scheme_line_fitting(handle->cali_handles[i]);
#else
            adc_cali_delete_scheme_curve_fitting(handle->cali_handles[i]);
#endif
            handle->cali_handles[i] = NULL;
        }
    }
    handle->cali_handle = NULL;
    
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
    const probe_channel_config_t *ch_config = NULL;
    
    for (size_t i = 0; i < handle->config.channel_count; i++) {
        if (handle->config.channels[i].channel_num == channel_num) {
            state = &handle->states[i];
            ch_config = &handle->config.channels[i];
            break;
        }
    }
    
    if (!state || !state->enabled || !ch_config) {
        ESP_LOGE(TAG, "Channel %u not found or disabled", channel_num);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 根据模式读取
    if (state->mode == PROBE_MODE_ADC) {
        // 读取 ADC 值
        int adc_value = 0;
        esp_err_t ret;
        
        if (ch_config->adc_unit == 1) {
            ret = adc_oneshot_read(handle->adc1_handle, ch_config->adc_channel, &adc_value);
        } else {
            ret = adc_oneshot_read(handle->adc2_handle, ch_config->adc_channel, &adc_value);
        }
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // Use calibration to convert raw ADC to millivolts
        size_t ch_idx = 0;
        for (size_t j = 0; j < handle->config.channel_count; j++) {
            if (handle->config.channels[j].channel_num == channel_num) {
                ch_idx = j;
                break;
            }
        }

        uint32_t result;
        if (handle->cali_handles[ch_idx]) {
            int voltage_mv = 0;
            esp_err_t cali_ret = adc_cali_raw_to_voltage(
                handle->cali_handles[ch_idx], adc_value, &voltage_mv);
            if (cali_ret == ESP_OK) {
                result = (uint32_t)voltage_mv;
            } else {
                result = (uint32_t)adc_value;
                ESP_LOGW(TAG, "Calibration conversion failed, using raw value");
            }
        } else {
            result = (uint32_t)adc_value;
        }

        state->data.adc_value = result;
        *out_value = result;
        
        ESP_LOGD(TAG, "CH%u ADC: raw=%d, value=%lu", channel_num, adc_value, result);
    } else {
        // GPIO 模式
        bool value = gpio_get_level(ch_config->gpio_num);
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
