/**
 * @file oscilloscope.c
 * @brief 示波器核心实现 - 仅使用STM32数据源
 * 
 * 功能：
 * - 接收STM32发送的波形数据
 * - 波形处理与测量计算
 * - FFT分析独立于时域测量
 */

#include "oscilloscope.h"
#include "stm32_comm.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <math.h>

static const char *TAG = "OSC";

/*============================================================================
 * 查找表
 *============================================================================*/
static const float time_scale_table[] = {
    [OSC_TIME_8NS] = 8e-9f, [OSC_TIME_20NS] = 20e-9f, [OSC_TIME_40NS] = 40e-9f,
    [OSC_TIME_80NS] = 80e-9f, [OSC_TIME_200NS] = 200e-9f, [OSC_TIME_400NS] = 400e-9f,
    [OSC_TIME_800NS] = 800e-9f, [OSC_TIME_2US] = 2e-6f, [OSC_TIME_4US] = 4e-6f,
    [OSC_TIME_8US] = 8e-6f, [OSC_TIME_20US] = 20e-6f, [OSC_TIME_40US] = 40e-6f,
    [OSC_TIME_80US] = 80e-6f, [OSC_TIME_200US] = 200e-6f, [OSC_TIME_400US] = 400e-6f,
    [OSC_TIME_800US] = 800e-6f, [OSC_TIME_2MS] = 2e-3f, [OSC_TIME_4MS] = 4e-3f,
    [OSC_TIME_8MS] = 8e-3f, [OSC_TIME_20MS] = 20e-3f, [OSC_TIME_40MS] = 40e-3f,
    [OSC_TIME_80MS] = 80e-3f, [OSC_TIME_200MS] = 200e-3f, [OSC_TIME_400MS] = 400e-3f,
    [OSC_TIME_800MS] = 800e-3f, [OSC_TIME_2S] = 2.0f, [OSC_TIME_4S] = 4.0f,
    [OSC_TIME_8S] = 8.0f, [OSC_TIME_20S] = 20.0f, [OSC_TIME_40S] = 40.0f,
};

static const char *time_scale_strings[] = {
    "8ns", "20ns", "40ns", "80ns", "200ns", "400ns", "800ns",
    "2us", "4us", "8us", "20us", "40us", "80us", "200us", "400us", "800us",
    "2ms", "4ms", "8ms", "20ms", "40ms", "80ms",
    "200ms", "400ms", "800ms", "2s", "4s", "8s", "20s", "40s",
};

static const float volt_scale_table[] = {
    [OSC_VOLT_10MV] = 0.01f, [OSC_VOLT_20MV] = 0.02f, [OSC_VOLT_50MV] = 0.05f,
    [OSC_VOLT_100MV] = 0.1f, [OSC_VOLT_200MV] = 0.2f, [OSC_VOLT_500MV] = 0.5f,
    [OSC_VOLT_1V] = 1.0f, [OSC_VOLT_2V] = 2.0f, [OSC_VOLT_5V] = 5.0f,
};

static const char *volt_scale_strings[] = {
    "10mV", "20mV", "50mV", "100mV", "200mV", "500mV", "1V", "2V", "5V",
};

/*============================================================================
 * 示波器上下文结构
 *============================================================================*/
struct osc_ctx_t {
    /* STM32数据 */
    volatile bool stm32_data_ready;
    int16_t *stm32_buffer;
    uint32_t stm32_data_count;
    uint32_t stm32_sample_rate;
    uint32_t storage_depth;
    float stm32_voltage_range;      /* STM32发送的电压量程 (±V) */
    uint8_t stm32_auto_range;       /* STM32自动量程状态 */
    uint8_t stm32_pga_gain;         /* STM32 PGA增益档位 */
    
    /* 波形数据 */
    osc_waveform_t waveform;
    osc_waveform_t frozen;
    bool has_frozen;
    
    /* 设置 */
    osc_time_scale_t time_scale;    /* 时基枚举（兼容旧代码） */
    float time_per_div;             /* 时基值（秒/格），UI直接设置 */
    osc_volt_scale_t volt_scale;
    float volts_per_div;            /* 电压档位值（伏/格），UI直接设置 */
    float x_offset;
    float y_offset;
    osc_trigger_t trigger;
    
    /* 状态 */
    osc_state_t state;
    osc_mode_t mode;
    
    /* 测量缓存 */
    osc_measurement_t measurement;
    
    /* 同步 */
    SemaphoreHandle_t mutex;
};

/* 全局实例 */
osc_ctx_t *g_osc_ctx = NULL;

/*============================================================================
 * STM32数据回调
 *============================================================================*/

/**
 * @brief STM32示波器波形数据回调
 */
static void osc_stm32_waveform_callback(const stm32_osc_waveform_t *data, uint16_t total_len)
{
    if (!g_osc_ctx || !data) return;
    
    uint16_t data_count = data->data_count;
    if (data_count == 0 || data_count > 8192) return;
    
    xSemaphoreTake(g_osc_ctx->mutex, portMAX_DELAY);
    
    if (g_osc_ctx->stm32_buffer && data_count <= g_osc_ctx->storage_depth) {
        memcpy(g_osc_ctx->stm32_buffer, data->data, data_count * sizeof(int16_t));
        g_osc_ctx->stm32_data_count = data_count;
        g_osc_ctx->stm32_sample_rate = data->sample_rate;
        g_osc_ctx->stm32_voltage_range = data->voltage_range;
        g_osc_ctx->stm32_auto_range = data->auto_range;
        g_osc_ctx->stm32_pga_gain = data->gain;
        g_osc_ctx->stm32_data_ready = true;
    }
    
    xSemaphoreGive(g_osc_ctx->mutex);
}

/**
 * @brief STM32示波器测量结果回调
 */
static void osc_stm32_measurement_callback(const stm32_osc_measurement_t *data)
{
    if (!g_osc_ctx || !data) return;
    
    xSemaphoreTake(g_osc_ctx->mutex, portMAX_DELAY);
    
    g_osc_ctx->measurement.vpp = data->vpp;
    g_osc_ctx->measurement.vmax = data->vmax;
    g_osc_ctx->measurement.vmin = data->vmin;
    g_osc_ctx->measurement.dc_offset = data->vavg;
    g_osc_ctx->measurement.vrms = data->vrms;
    g_osc_ctx->measurement.freq = data->freq;
    g_osc_ctx->measurement.valid = true;
    
    xSemaphoreGive(g_osc_ctx->mutex);
}

/*============================================================================
 * 内部函数
 *============================================================================*/

static uint32_t select_storage_depth(osc_time_scale_t time_scale)
{
    if (time_scale <= OSC_TIME_800NS) return 1024;
    if (time_scale <= OSC_TIME_80US) return 10240;
    if (time_scale <= OSC_TIME_8MS) return 51200;
    return 102400;
}

static void calculate_measurements(osc_ctx_t *ctx, const osc_waveform_t *wf)
{
    osc_measurement_t *m = &ctx->measurement;
    
    if (wf == NULL || wf->data == NULL || wf->count == 0) {
        memset(m, 0, sizeof(osc_measurement_t));
        return;
    }
    
    float vmax = wf->data[0];
    float vmin = wf->data[0];
    float sum = 0.0f;
    float sum_sq = 0.0f;
    
    for (uint32_t i = 0; i < wf->count; i++) {
        float v = wf->data[i];
        if (v > vmax) vmax = v;
        if (v < vmin) vmin = v;
        sum += v;
        sum_sq += v * v;
    }
    
    m->vmax = vmax;
    m->vmin = vmin;
    m->vpp = vmax - vmin;
    m->dc_offset = sum / wf->count;
    m->vrms = sqrtf(sum_sq / wf->count);
    m->freq = 0.0f;
    m->valid = true;
}

/*============================================================================
 * 公共API实现
 *============================================================================*/

osc_ctx_t *osc_init(void)
{
    ESP_LOGI(TAG, "初始化示波器（STM32数据源）");
    
    osc_ctx_t *ctx = heap_caps_calloc(1, sizeof(osc_ctx_t), MALLOC_CAP_8BIT);
    if (!ctx) {
        ESP_LOGE(TAG, "内存分配失败");
        return NULL;
    }
    
    ctx->time_scale = OSC_TIME_2MS;
    ctx->time_per_div = 1e-3f;          /* 默认1ms/div */
    ctx->volt_scale = OSC_VOLT_1V;
    ctx->volts_per_div = 1.0f;          /* 默认1V/div */
    ctx->state = OSC_STATE_STOPPED;
    ctx->mode = OSC_MODE_NORMAL;
    ctx->storage_depth = select_storage_depth(ctx->time_scale);
    ctx->stm32_data_ready = false;
    ctx->stm32_voltage_range = 50.0f;   /* 默认±50V量程 */
    ctx->stm32_auto_range = 0;
    ctx->stm32_pga_gain = 0;
    
    ctx->trigger.enabled = false;
    ctx->trigger.level = 0.0f;
    ctx->trigger.rising_edge = true;
    ctx->trigger.pre_trigger = 0.5f;
    
    ctx->mutex = xSemaphoreCreateMutex();
    if (!ctx->mutex) {
        ESP_LOGE(TAG, "互斥锁创建失败");
        free(ctx);
        return NULL;
    }
    
    ctx->stm32_buffer = heap_caps_calloc(ctx->storage_depth, sizeof(int16_t), MALLOC_CAP_SPIRAM);
    ctx->waveform.data = heap_caps_calloc(ctx->storage_depth, sizeof(float), MALLOC_CAP_SPIRAM);
    ctx->waveform.capacity = ctx->storage_depth;
    ctx->frozen.data = heap_caps_calloc(ctx->storage_depth, sizeof(float), MALLOC_CAP_SPIRAM);
    ctx->frozen.capacity = ctx->storage_depth;
    
    if (!ctx->stm32_buffer || !ctx->waveform.data || !ctx->frozen.data) {
        ESP_LOGE(TAG, "缓冲区分配失败");
        if (ctx->stm32_buffer) heap_caps_free(ctx->stm32_buffer);
        if (ctx->waveform.data) heap_caps_free(ctx->waveform.data);
        if (ctx->frozen.data) heap_caps_free(ctx->frozen.data);
        vSemaphoreDelete(ctx->mutex);
        free(ctx);
        return NULL;
    }
    
    ESP_LOGI(TAG, "示波器初始化完成，深度=%lu", ctx->storage_depth);
    return ctx;
}

void osc_deinit(osc_ctx_t *ctx)
{
    if (!ctx) return;
    
    if (ctx->state == OSC_STATE_RUNNING) osc_stop(ctx);
    
    if (ctx->waveform.data) heap_caps_free(ctx->waveform.data);
    if (ctx->frozen.data) heap_caps_free(ctx->frozen.data);
    if (ctx->stm32_buffer) heap_caps_free(ctx->stm32_buffer);
    if (ctx->mutex) vSemaphoreDelete(ctx->mutex);
    
    free(ctx);
    ESP_LOGI(TAG, "示波器已释放");
}

esp_err_t osc_start(osc_ctx_t *ctx)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->has_frozen = false;
    ctx->state = OSC_STATE_RUNNING;
    xSemaphoreGive(ctx->mutex);
    
    stm32_osc_start();
    ESP_LOGI(TAG, "示波器启动");
    return ESP_OK;
}

esp_err_t osc_stop(osc_ctx_t *ctx)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;
    
    stm32_osc_stop();
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (ctx->waveform.count > 0) {
        memcpy(ctx->frozen.data, ctx->waveform.data, ctx->waveform.count * sizeof(float));
        ctx->frozen.count = ctx->waveform.count;
        ctx->frozen.time_per_sample = ctx->waveform.time_per_sample;
        ctx->frozen.trigger_pos = ctx->waveform.trigger_pos;
        ctx->frozen.time_scale = ctx->time_scale;
        ctx->frozen.volt_scale = ctx->volt_scale;
        ctx->has_frozen = true;
    }
    
    ctx->state = OSC_STATE_STOPPED;
    xSemaphoreGive(ctx->mutex);
    
    ESP_LOGI(TAG, "示波器停止");
    return ESP_OK;
}

esp_err_t osc_update(osc_ctx_t *ctx)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;
    if (ctx->state != OSC_STATE_RUNNING) return ESP_OK;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (ctx->stm32_data_ready) {
        uint32_t count = ctx->stm32_data_count;
        if (count > ctx->waveform.capacity) count = ctx->waveform.capacity;
        
        /* STM32发送的是int16_t有符号数据（相对于ADC中点2048的偏移值）
         * signed_adc = adc_raw - 2048，范围约-2048~+2047
         * 
         * 信号链路：输入 → 1/50衰减 → PGA(G) → 反相运放(1.65-Vin) → ADC
         * 
         * 电压转换公式：
         * V_in = -signed_adc × voltage_range × 2 / 4095
         * 
         * 其中 voltage_range = 82.5 / G（STM32发送的理论满量程）
         */
        float voltage_range = ctx->stm32_voltage_range;
        if (voltage_range <= 0.0f) voltage_range = 82.5f;  /* 默认G=1 */
        
        for (uint32_t i = 0; i < count; i++) {
            ctx->waveform.data[i] = -(float)ctx->stm32_buffer[i] * voltage_range * 2.0f / 4095.0f;
        }
        
        ctx->waveform.count = count;
        ctx->waveform.time_per_sample = 1.0f / ctx->stm32_sample_rate;
        ctx->waveform.trigger_pos = count / 2;
        ctx->waveform.time_scale = ctx->time_scale;
        ctx->waveform.volt_scale = ctx->volt_scale;
        
        ctx->stm32_data_ready = false;
        
        if (!ctx->measurement.valid) {
            calculate_measurements(ctx, &ctx->waveform);
        }
    }
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

osc_state_t osc_get_state(osc_ctx_t *ctx)
{
    return ctx ? ctx->state : OSC_STATE_STOPPED;
}

osc_mode_t osc_get_mode(osc_ctx_t *ctx)
{
    return ctx ? ctx->mode : OSC_MODE_NORMAL;
}


/*============================================================================
 * 设置API
 *============================================================================*/

esp_err_t osc_set_time_scale(osc_ctx_t *ctx, osc_time_scale_t scale)
{
    if (!ctx || scale >= OSC_TIME_MAX) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->time_scale = scale;
    ctx->mode = (scale >= OSC_TIME_200MS) ? OSC_MODE_ROLL : OSC_MODE_NORMAL;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

esp_err_t osc_set_volt_scale(osc_ctx_t *ctx, osc_volt_scale_t scale)
{
    if (!ctx || scale >= OSC_VOLT_MAX) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->volt_scale = scale;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

esp_err_t osc_set_x_offset(osc_ctx_t *ctx, float offset_s)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->x_offset = offset_s;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

esp_err_t osc_set_y_offset(osc_ctx_t *ctx, float offset_v)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->y_offset = offset_v;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

esp_err_t osc_set_trigger(osc_ctx_t *ctx, const osc_trigger_t *trigger)
{
    if (!ctx || !trigger) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    memcpy(&ctx->trigger, trigger, sizeof(osc_trigger_t));
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

esp_err_t osc_set_time_per_div(osc_ctx_t *ctx, float time_per_div)
{
    if (!ctx || time_per_div <= 0.0f) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->time_per_div = time_per_div;
    /* 200ms/div以上切换到滚动模式 */
    ctx->mode = (time_per_div >= 0.2f) ? OSC_MODE_ROLL : OSC_MODE_NORMAL;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

esp_err_t osc_set_volts_per_div(osc_ctx_t *ctx, float volts_per_div)
{
    if (!ctx || volts_per_div <= 0.0f) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->volts_per_div = volts_per_div;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

/*============================================================================
 * 数据获取API
 *============================================================================*/

esp_err_t osc_get_display_data(osc_ctx_t *ctx, float *buffer, uint32_t *count)
{
    if (!ctx || !buffer || !count) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    osc_waveform_t *wf = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen) ?
                         &ctx->frozen : &ctx->waveform;
    
    if (wf->count == 0) {
        xSemaphoreGive(ctx->mutex);
        *count = 0;
        return ESP_ERR_NOT_FOUND;
    }
    
    /* 使用UI直接设置的时基值 */
    float time_per_div = ctx->time_per_div;
    float display_time = time_per_div * OSC_GRID_COLS;
    float trigger_time = wf->trigger_pos * wf->time_per_sample;
    float start_time = trigger_time - (display_time / 2.0f) + ctx->x_offset;
    if (start_time < 0.0f) start_time = 0.0f;
    
    uint32_t start_idx = (uint32_t)(start_time / wf->time_per_sample);
    if (start_idx >= wf->count) start_idx = wf->count - 1;
    
    float sample_step = (display_time / wf->time_per_sample) / OSC_DISPLAY_WIDTH;
    
    uint32_t out_count = 0;
    for (uint32_t i = 0; i < OSC_DISPLAY_WIDTH && out_count < OSC_DISPLAY_WIDTH; i++) {
        uint32_t src_idx = start_idx + (uint32_t)(i * sample_step);
        if (src_idx >= wf->count) break;
        buffer[out_count++] = wf->data[src_idx] + ctx->y_offset;
    }
    
    *count = out_count;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

esp_err_t osc_get_measurements(osc_ctx_t *ctx, osc_measurement_t *meas)
{
    if (!ctx || !meas) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    memcpy(meas, &ctx->measurement, sizeof(osc_measurement_t));
    xSemaphoreGive(ctx->mutex);
    
    return ctx->measurement.valid ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t osc_get_raw_waveform(osc_ctx_t *ctx, const osc_waveform_t **waveform)
{
    if (!ctx || !waveform) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    *waveform = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen) ?
                &ctx->frozen : &ctx->waveform;
    xSemaphoreGive(ctx->mutex);
    
    return (*waveform)->count > 0 ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t osc_auto_adjust(osc_ctx_t *ctx)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    osc_waveform_t *wf = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen) ?
                         &ctx->frozen : &ctx->waveform;
    
    if (wf->count == 0) {
        xSemaphoreGive(ctx->mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    float vmax = wf->data[0], vmin = wf->data[0];
    float sum = 0.0f;
    
    for (uint32_t i = 0; i < wf->count; i++) {
        float v = wf->data[i];
        if (v > vmax) vmax = v;
        if (v < vmin) vmin = v;
        sum += v;
    }
    
    float vpp = vmax - vmin;
    float vcenter = sum / wf->count;
    
    if (vpp < 0.01f) {
        xSemaphoreGive(ctx->mutex);
        return ESP_ERR_INVALID_ARG;
    }
    
    float target_vpd = vpp / 6.5f;
    osc_volt_scale_t best_vs = OSC_VOLT_1V;
    float best_diff = 1000.0f;
    
    for (int i = 0; i < OSC_VOLT_MAX; i++) {
        float diff = fabsf(volt_scale_table[i] - target_vpd);
        if (diff < best_diff) {
            best_diff = diff;
            best_vs = (osc_volt_scale_t)i;
        }
    }
    ctx->volt_scale = best_vs;
    ctx->y_offset = -vcenter;
    ctx->x_offset = 0.0f;
    
    ESP_LOGI(TAG, "自动调整: V/div=%s, Vpp=%.2fV", volt_scale_strings[ctx->volt_scale], vpp);
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

/*============================================================================
 * 工具函数
 *============================================================================*/

float osc_get_time_per_div(osc_time_scale_t scale)
{
    return (scale < OSC_TIME_MAX) ? time_scale_table[scale] : 0.0f;
}

float osc_get_volts_per_div(osc_volt_scale_t scale)
{
    return (scale < OSC_VOLT_MAX) ? volt_scale_table[scale] : 0.0f;
}

const char *osc_get_time_scale_str(osc_time_scale_t scale)
{
    return (scale < OSC_TIME_MAX) ? time_scale_strings[scale] : "???";
}

const char *osc_get_volt_scale_str(osc_volt_scale_t scale)
{
    return (scale < OSC_VOLT_MAX) ? volt_scale_strings[scale] : "???";
}

uint32_t osc_get_sample_rate_hz(osc_ctx_t *ctx)
{
    return ctx ? ctx->stm32_sample_rate : 0;
}

/*============================================================================
 * 全局实例管理
 *============================================================================*/

esp_err_t osc_global_init(void)
{
    if (g_osc_ctx) return ESP_OK;
    
    g_osc_ctx = osc_init();
    if (!g_osc_ctx) return ESP_FAIL;
    
    osc_trigger_t trigger = {
        .enabled = true,
        .level = 1.65f,
        .rising_edge = true,
        .pre_trigger = 0.5f,
    };
    osc_set_trigger(g_osc_ctx, &trigger);
    
    /* 注册STM32示波器数据回调 */
    stm32_comm_callbacks_t callbacks = {
        .osc_waveform = osc_stm32_waveform_callback,
        .osc_measurement = osc_stm32_measurement_callback,
    };
    stm32_comm_register_callbacks(&callbacks);
    
    ESP_LOGI(TAG, "全局示波器实例已初始化");
    return ESP_OK;
}

void osc_global_deinit(void)
{
    if (g_osc_ctx) {
        stm32_osc_stop();
        osc_deinit(g_osc_ctx);
        g_osc_ctx = NULL;
    }
}
