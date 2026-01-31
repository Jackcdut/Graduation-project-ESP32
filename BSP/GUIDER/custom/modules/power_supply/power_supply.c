/**
 * @file power_supply.c
 * @brief 数控电源模块实现 (ESP32端)
 * 
 * 与现有UI集成：
 * - 使用 BSP/GUIDER/generated/setup_scr_scrPowerSupply.c 中的UI控件
 * - 通过 events_init.c 中的事件处理函数响应用户操作
 * - 本模块负责数据管理和STM32通信
 */

#include "power_supply.h"
#include "stm32_comm.h"
#include "gui_guider.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "POWER_SUPPLY";

/*============================================================================
 * 私有结构定义
 *============================================================================*/

struct ps_ctx_t {
    ps_data_t data;                 /* 电源数据 */
    SemaphoreHandle_t mutex;        /* 数据互斥锁 */
    bool initialized;               /* 初始化标志 */
    uint32_t last_update_time;      /* 上次更新时间 */
};

/*============================================================================
 * 全局实例
 *============================================================================*/

ps_ctx_t *g_ps_ctx = NULL;

/*============================================================================
 * 私有函数声明
 *============================================================================*/

static void ps_data_callback(const stm32_power_data_t *data);

/*============================================================================
 * 核心API实现
 *============================================================================*/

ps_ctx_t *ps_init(void)
{
    ps_ctx_t *ctx = calloc(1, sizeof(ps_ctx_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return NULL;
    }
    
    ctx->mutex = xSemaphoreCreateMutex();
    if (!ctx->mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(ctx);
        return NULL;
    }
    
    /* 初始化默认值 */
    ctx->data.state = PS_STATE_OFF;
    ctx->data.output_enable = false;
    ctx->data.mode = PS_MODE_AUTO;
    ctx->data.pd_voltage = PS_PD_12V;
    ctx->data.voltage_set = 5.0f;
    ctx->data.current_set = 1.0f;
    ctx->data.voltage_out = 0.0f;
    ctx->data.current_out = 0.0f;
    ctx->data.power_out = 0.0f;
    ctx->data.voltage_in = 0.0f;
    ctx->data.temperature = 25.0f;
    ctx->data.data_valid = false;
    
    ctx->initialized = true;
    
    ESP_LOGI(TAG, "Power supply module initialized");
    return ctx;
}

void ps_deinit(ps_ctx_t *ctx)
{
    if (!ctx) return;
    
    if (ctx->mutex) {
        vSemaphoreDelete(ctx->mutex);
    }
    
    free(ctx);
    ESP_LOGI(TAG, "Power supply module deinitialized");
}

esp_err_t ps_update(ps_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    
    /* 数据通过回调更新，这里可以做超时检测等 */
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        /* 检查数据超时 (3秒无更新则标记无效) */
        if (ctx->data.data_valid && (now - ctx->last_update_time > 3000)) {
            ctx->data.data_valid = false;
            ESP_LOGW(TAG, "Power data timeout");
        }
        xSemaphoreGive(ctx->mutex);
    }
    
    return ESP_OK;
}

/*============================================================================
 * 控制API实现
 *============================================================================*/

esp_err_t ps_enable_output(ps_ctx_t *ctx, bool enable)
{
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    
    esp_err_t ret = stm32_power_enable(enable);
    if (ret == ESP_OK) {
        if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            ctx->data.output_enable = enable;
            xSemaphoreGive(ctx->mutex);
        }
        ESP_LOGI(TAG, "Output %s", enable ? "enabled" : "disabled");
    }
    return ret;
}

esp_err_t ps_set_voltage(ps_ctx_t *ctx, float voltage)
{
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    
    /* 限制范围 */
    if (voltage < POWER_VOLTAGE_MIN) voltage = POWER_VOLTAGE_MIN;
    if (voltage > POWER_VOLTAGE_MAX) voltage = POWER_VOLTAGE_MAX;
    
    esp_err_t ret = stm32_power_set_voltage(voltage);
    if (ret == ESP_OK) {
        if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            ctx->data.voltage_set = voltage;
            xSemaphoreGive(ctx->mutex);
        }
        ESP_LOGI(TAG, "Voltage set to %.2fV", voltage);
    }
    return ret;
}

esp_err_t ps_set_current(ps_ctx_t *ctx, float current)
{
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    
    /* 限制范围 */
    if (current < POWER_CURRENT_MIN) current = POWER_CURRENT_MIN;
    if (current > POWER_CURRENT_MAX) current = POWER_CURRENT_MAX;
    
    esp_err_t ret = stm32_power_set_current(current);
    if (ret == ESP_OK) {
        if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            ctx->data.current_set = current;
            xSemaphoreGive(ctx->mutex);
        }
        ESP_LOGI(TAG, "Current set to %.2fA", current);
    }
    return ret;
}

esp_err_t ps_set_mode(ps_ctx_t *ctx, ps_mode_t mode)
{
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    
    esp_err_t ret = stm32_power_set_mode((uint8_t)mode);
    if (ret == ESP_OK) {
        if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            ctx->data.mode = mode;
            xSemaphoreGive(ctx->mutex);
        }
        ESP_LOGI(TAG, "Mode set to %s", ps_get_mode_str(mode));
    }
    return ret;
}

esp_err_t ps_set_pd_voltage(ps_ctx_t *ctx, ps_pd_voltage_t pd_voltage)
{
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    
    esp_err_t ret = stm32_power_set_pd_voltage((uint8_t)pd_voltage);
    if (ret == ESP_OK) {
        if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            ctx->data.pd_voltage = pd_voltage;
            xSemaphoreGive(ctx->mutex);
        }
        ESP_LOGI(TAG, "PD voltage set to %s", ps_get_pd_voltage_str(pd_voltage));
    }
    return ret;
}

esp_err_t ps_clear_protection(ps_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return ESP_ERR_INVALID_STATE;
    
    esp_err_t ret = stm32_power_clear_protection();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Protection cleared");
    }
    return ret;
}

/*============================================================================
 * 数据获取API实现
 *============================================================================*/

esp_err_t ps_get_data(ps_ctx_t *ctx, ps_data_t *data)
{
    if (!ctx || !ctx->initialized || !data) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(data, &ctx->data, sizeof(ps_data_t));
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

ps_state_t ps_get_state(ps_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return PS_STATE_ERROR;
    
    ps_state_t state = PS_STATE_ERROR;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state = ctx->data.state;
        xSemaphoreGive(ctx->mutex);
    }
    return state;
}

bool ps_is_output_enabled(ps_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return false;
    
    bool enabled = false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        enabled = ctx->data.output_enable;
        xSemaphoreGive(ctx->mutex);
    }
    return enabled;
}

bool ps_is_cc_mode(ps_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return false;
    
    bool cc_mode = false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        cc_mode = (ctx->data.state == PS_STATE_CC_MODE);
        xSemaphoreGive(ctx->mutex);
    }
    return cc_mode;
}

/*============================================================================
 * 工具函数实现
 *============================================================================*/

const char *ps_get_state_str(ps_state_t state)
{
    switch (state) {
        case PS_STATE_OFF:       return "OFF";
        case PS_STATE_SOFTSTART: return "SOFTSTART";
        case PS_STATE_RUNNING:   return "RUNNING";
        case PS_STATE_CC_MODE:   return "CC";
        case PS_STATE_CV_MODE:   return "CV";
        case PS_STATE_OTP:       return "OTP";
        case PS_STATE_OVP:       return "OVP";
        case PS_STATE_OCP:       return "OCP";
        case PS_STATE_ERROR:     return "ERROR";
        default:                 return "UNKNOWN";
    }
}

const char *ps_get_mode_str(ps_mode_t mode)
{
    switch (mode) {
        case PS_MODE_CV:   return "CV";
        case PS_MODE_CC:   return "CC";
        case PS_MODE_AUTO: return "AUTO";
        default:           return "UNKNOWN";
    }
}

const char *ps_get_pd_voltage_str(ps_pd_voltage_t pd_voltage)
{
    switch (pd_voltage) {
        case PS_PD_5V:  return "5V";
        case PS_PD_9V:  return "9V";
        case PS_PD_12V: return "12V";
        case PS_PD_20V: return "20V";
        case PS_PD_28V: return "28V";
        default:        return "UNKNOWN";
    }
}

/*============================================================================
 * 外部UI更新函数声明 (定义在events_init.c中)
 *============================================================================*/
extern void ps_update_voltage_actual(float voltage_actual);
extern void ps_update_current_actual(float current_actual);
extern void ps_update_power_actual(float power_actual);
extern void ps_update_mode(bool is_cc_mode);

/*============================================================================
 * 数据回调函数
 *============================================================================*/

static void ps_data_callback(const stm32_power_data_t *data)
{
    if (!g_ps_ctx || !g_ps_ctx->initialized || !data) return;
    
    if (xSemaphoreTake(g_ps_ctx->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_ps_ctx->data.state = (ps_state_t)data->state;
        g_ps_ctx->data.output_enable = data->output_enable != 0;
        g_ps_ctx->data.mode = (ps_mode_t)data->mode;
        g_ps_ctx->data.pd_voltage = (ps_pd_voltage_t)data->pd_voltage;
        g_ps_ctx->data.voltage_set = data->voltage_set;
        g_ps_ctx->data.current_set = data->current_set;
        g_ps_ctx->data.voltage_out = data->voltage_out;
        g_ps_ctx->data.current_out = data->current_out;
        g_ps_ctx->data.power_out = data->power_out;
        g_ps_ctx->data.voltage_in = data->voltage_in;
        g_ps_ctx->data.temperature = data->temperature;
        g_ps_ctx->data.data_valid = true;
        g_ps_ctx->last_update_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        xSemaphoreGive(g_ps_ctx->mutex);
        
        /* 更新UI显示 - 只有输出使能时才更新实际值 */
        if (data->output_enable) {
            ps_update_voltage_actual(data->voltage_out);
            ps_update_current_actual(data->current_out);
            ps_update_power_actual(data->power_out);
            
            /* 根据状态更新CC/CV模式显示 */
            bool is_cc = (data->state == POWER_STATE_CC_MODE);
            ps_update_mode(is_cc);
        }
        
        ESP_LOGD(TAG, "Power data: V=%.2f I=%.3f P=%.3f State=%d", 
                 data->voltage_out, data->current_out, data->power_out, data->state);
    }
}

/*============================================================================
 * 全局实例管理
 *============================================================================*/

esp_err_t ps_global_init(void)
{
    if (g_ps_ctx) {
        ESP_LOGW(TAG, "Global instance already initialized");
        return ESP_OK;
    }
    
    g_ps_ctx = ps_init();
    if (!g_ps_ctx) {
        return ESP_FAIL;
    }
    
    /* 注册数据回调 */
    stm32_comm_callbacks_t callbacks = {
        .power_data = ps_data_callback
    };
    stm32_comm_register_callbacks(&callbacks);
    
    ESP_LOGI(TAG, "Global power supply instance initialized");
    return ESP_OK;
}

void ps_global_deinit(void)
{
    if (g_ps_ctx) {
        ps_deinit(g_ps_ctx);
        g_ps_ctx = NULL;
        ESP_LOGI(TAG, "Global power supply instance deinitialized");
    }
}
