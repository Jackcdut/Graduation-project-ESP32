/**
 * @file oscilloscope_adc.c
 * @brief ESP32-P4 ADC Sampling Implementation
 */

#include "oscilloscope_adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <math.h>

static const char *TAG = "OscADC";

/* ADC context structure */
struct osc_adc_ctx_t {
    adc_oneshot_unit_handle_t adc_handle;
    
    /* Sampling configuration */
    osc_sample_rate_t sample_rate;
    uint32_t sample_rate_hz;
    uint32_t storage_depth;
    
    /* Circular buffer for continuous sampling */
    uint16_t *sample_buffer;        // Raw ADC values
    uint32_t buffer_write_idx;      // Current write position
    bool buffer_full;               // Buffer has wrapped around
    
    /* Triggered data buffer */
    uint16_t *triggered_buffer;     // Captured triggered data
    uint32_t triggered_count;       // Number of points in triggered buffer
    bool new_data_available;        // New triggered data ready
    
    /* Trigger configuration */
    osc_trigger_config_t trigger;
    bool trigger_armed;             // Waiting for trigger
    uint32_t trigger_position;      // Position where trigger occurred
    
    /* Synchronization */
    SemaphoreHandle_t mutex;
    TaskHandle_t sampling_task;
    
    /* Status */
    bool running;
};

/* Sampling rate to Hz conversion - realistic for ESP32-P4 ADC */
static const uint32_t sample_rate_table[] = {
    [OSC_SAMPLE_RATE_1MSPS]   = 1000000,    // 1 MSa/s (maximum practical for ESP32)
    [OSC_SAMPLE_RATE_500KSPS] = 500000,     // 500 kSa/s
    [OSC_SAMPLE_RATE_200KSPS] = 200000,     // 200 kSa/s
    [OSC_SAMPLE_RATE_100KSPS] = 100000,     // 100 kSa/s
    [OSC_SAMPLE_RATE_50KSPS]  = 50000,      // 50 kSa/s
    [OSC_SAMPLE_RATE_10KSPS]  = 10000,      // 10 kSa/s
    [OSC_SAMPLE_RATE_1KSPS]   = 1000,       // 1 kSa/s
};

/**
 * @brief ADC sampling task (简化版：只负责持续采集)
 */
static void adc_sampling_task(void *arg)
{
    osc_adc_ctx_t *ctx = (osc_adc_ctx_t *)arg;
    int adc_raw;
    uint32_t sample_count = 0;
    
    // Calculate delay between samples based on sample rate
    uint32_t delay_us = 0;
    TickType_t delay_ticks = 0;
    
    if (ctx->sample_rate_hz < 10000) {
        // Low sample rate: add delay
        delay_us = 1000000 / ctx->sample_rate_hz;
        delay_ticks = pdMS_TO_TICKS(delay_us / 1000);
        if (delay_ticks == 0) delay_ticks = 1;
        ESP_LOGI(TAG, "ADC sampling with delay: %lu us for %lu Hz", delay_us, ctx->sample_rate_hz);
    } else {
        // High sample rate: continuous sampling
        ESP_LOGI(TAG, "ADC sampling: CONTINUOUS (no delay) for %lu Hz", ctx->sample_rate_hz);
    }
    
    ESP_LOGI(TAG, "ADC sampling task started - direct circular buffer mode");
    
    while (ctx->running) {
        // Read ADC value
        esp_err_t ret = adc_oneshot_read(ctx->adc_handle, OSC_ADC_CHANNEL, &adc_raw);
        if (ret == ESP_OK) {
            uint16_t adc_value = (uint16_t)adc_raw;
            
            // Debug: Log first few ADC readings
            if (sample_count < 5) {
                ESP_LOGI(TAG, "🔍 ADC Read #%lu: adc_raw=%d, adc_value=%u", sample_count, adc_raw, adc_value);
            }
            
            xSemaphoreTake(ctx->mutex, portMAX_DELAY);
            
            // Store in circular buffer
            ctx->sample_buffer[ctx->buffer_write_idx] = adc_value;
            ctx->buffer_write_idx++;
            
            if (ctx->buffer_write_idx >= ctx->storage_depth) {
                ctx->buffer_write_idx = 0;
                ctx->buffer_full = true;
                
                // Log when buffer first fills up
                if (sample_count < ctx->storage_depth + 10) {
                    ESP_LOGI(TAG, "✅ Circular buffer filled, continuous sampling active");
                }
            }
            
            xSemaphoreGive(ctx->mutex);
            sample_count++;
        }
        
        // Add delay only for low sample rates
        if (delay_ticks > 0) {
            vTaskDelay(delay_ticks);
        } else {
            // For continuous sampling, yield periodically
            if ((sample_count % 100) == 0) {
                taskYIELD();
            }
        }
    }
    
    ESP_LOGI(TAG, "ADC sampling task stopped");
    vTaskDelete(NULL);
}

/**
 * @brief Initialize ADC sampling module
 */
osc_adc_ctx_t *osc_adc_init(osc_sample_rate_t sample_rate, uint32_t storage_depth)
{
    ESP_LOGI(TAG, "Initializing ADC sampling (rate=%d, depth=%lu)", sample_rate, storage_depth);
    
    // Validate parameters
    if (storage_depth > OSC_MAX_STORAGE_DEPTH) {
        ESP_LOGE(TAG, "Storage depth %lu exceeds maximum %d", storage_depth, OSC_MAX_STORAGE_DEPTH);
        return NULL;
    }
    
    // Allocate context
    osc_adc_ctx_t *ctx = heap_caps_malloc(sizeof(osc_adc_ctx_t), MALLOC_CAP_8BIT);
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return NULL;
    }
    memset(ctx, 0, sizeof(osc_adc_ctx_t));
    
    ctx->sample_rate = sample_rate;
    ctx->sample_rate_hz = sample_rate_table[sample_rate];
    ctx->storage_depth = storage_depth;
    
    // Allocate sample buffer in PSRAM
    ctx->sample_buffer = heap_caps_malloc(storage_depth * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (ctx->sample_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate sample buffer");
        free(ctx);
        return NULL;
    }
    // Initialize buffer to zero
    memset(ctx->sample_buffer, 0, storage_depth * sizeof(uint16_t));
    
    // Allocate triggered buffer in PSRAM
    ctx->triggered_buffer = heap_caps_malloc(storage_depth * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (ctx->triggered_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate triggered buffer");
        heap_caps_free(ctx->sample_buffer);
        free(ctx);
        return NULL;
    }
    // Initialize buffer to zero
    memset(ctx->triggered_buffer, 0, storage_depth * sizeof(uint16_t));
    
    // Create mutex
    ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        heap_caps_free(ctx->triggered_buffer);
        heap_caps_free(ctx->sample_buffer);
        free(ctx);
        return NULL;
    }
    
    // Configure ADC one-shot mode
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = OSC_ADC_UNIT,
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &ctx->adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC handle: %s", esp_err_to_name(ret));
        vSemaphoreDelete(ctx->mutex);
        heap_caps_free(ctx->triggered_buffer);
        heap_caps_free(ctx->sample_buffer);
        free(ctx);
        return NULL;
    }
    
    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = OSC_ADC_BITWIDTH,
        .atten = OSC_ADC_ATTEN,
    };
    
    ret = adc_oneshot_config_channel(ctx->adc_handle, OSC_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(ctx->adc_handle);
        vSemaphoreDelete(ctx->mutex);
        heap_caps_free(ctx->triggered_buffer);
        heap_caps_free(ctx->sample_buffer);
        free(ctx);
        return NULL;
    }
    
    // Initialize trigger configuration (disabled by default for auto-trigger mode)
    ctx->trigger.enabled = false;  // Disabled = auto-trigger mode
    ctx->trigger.level_voltage = 0.0f;  // 0V (center of -50V to +50V range)
    ctx->trigger.rising_edge = true;
    ctx->trigger.pre_trigger_ratio = 0.5f;
    ctx->trigger_armed = true;
    
    ESP_LOGI(TAG, "ADC sampling initialized successfully");
    ESP_LOGI(TAG, "  Sample rate: %lu Hz", ctx->sample_rate_hz);
    ESP_LOGI(TAG, "  Storage depth: %lu samples", ctx->storage_depth);
    ESP_LOGI(TAG, "  ADC range: 0-3.3V -> Display range: %.1fV to %.1fV", 
             OSC_DISPLAY_VOLTAGE_MIN, OSC_DISPLAY_VOLTAGE_MAX);
    ESP_LOGI(TAG, "  Trigger mode: %s", ctx->trigger.enabled ? "NORMAL" : "AUTO");
    ESP_LOGI(TAG, "  Trigger level: %.2fV", ctx->trigger.level_voltage);
    ESP_LOGI(TAG, "  Trigger edge: %s", ctx->trigger.rising_edge ? "RISING" : "FALLING");
    
    return ctx;
}

/**
 * @brief Deinitialize ADC sampling module
 */
void osc_adc_deinit(osc_adc_ctx_t *ctx)
{
    if (ctx == NULL) return;
    
    if (ctx->running) {
        osc_adc_stop(ctx);
    }
    
    if (ctx->adc_handle) {
        adc_oneshot_del_unit(ctx->adc_handle);
    }
    
    if (ctx->mutex) {
        vSemaphoreDelete(ctx->mutex);
    }
    
    if (ctx->triggered_buffer) {
        heap_caps_free(ctx->triggered_buffer);
    }
    
    if (ctx->sample_buffer) {
        heap_caps_free(ctx->sample_buffer);
    }
    
    free(ctx);
    ESP_LOGI(TAG, "ADC sampling deinitialized");
}

/**
 * @brief Start continuous ADC sampling
 */
esp_err_t osc_adc_start(osc_adc_ctx_t *ctx)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (ctx->running) {
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }
    
    // Reset buffers
    ctx->buffer_write_idx = 0;
    ctx->buffer_full = false;
    ctx->new_data_available = false;
    ctx->trigger_armed = true;
    ctx->running = true;
    
    xSemaphoreGive(ctx->mutex);
    
    // Create sampling task with lower priority to avoid blocking other tasks
    // Priority 3 is lower than most system tasks but higher than IDLE (0)
    BaseType_t ret = xTaskCreate(adc_sampling_task, "adc_sample", 4096, ctx, 3, &ctx->sampling_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sampling task");
        ctx->running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "ADC sampling started");
    return ESP_OK;
}

/**
 * @brief Stop ADC sampling
 */
esp_err_t osc_adc_stop(osc_adc_ctx_t *ctx)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (!ctx->running) {
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }
    
    ctx->running = false;
    xSemaphoreGive(ctx->mutex);
    
    // Wait for task to finish
    if (ctx->sampling_task != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));  // Give task time to exit
        ctx->sampling_task = NULL;
    }
    
    ESP_LOGI(TAG, "ADC sampling stopped");
    return ESP_OK;
}

/**
 * @brief Configure trigger
 */
esp_err_t osc_adc_set_trigger(osc_adc_ctx_t *ctx, const osc_trigger_config_t *trigger)
{
    if (ctx == NULL || trigger == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    memcpy(&ctx->trigger, trigger, sizeof(osc_trigger_config_t));
    ctx->trigger_armed = true;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

/**
 * @brief Set sampling rate
 */
esp_err_t osc_adc_set_sample_rate(osc_adc_ctx_t *ctx, osc_sample_rate_t sample_rate)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    bool was_running = ctx->running;
    if (was_running) {
        osc_adc_stop(ctx);
    }
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->sample_rate = sample_rate;
    ctx->sample_rate_hz = sample_rate_table[sample_rate];
    xSemaphoreGive(ctx->mutex);
    
    // In one-shot mode, sample rate is controlled by task delay
    // No need to reconfigure ADC hardware
    
    if (was_running) {
        osc_adc_start(ctx);
    }
    
    ESP_LOGI(TAG, "Sample rate changed to %lu Hz", ctx->sample_rate_hz);
    return ESP_OK;
}

/**
 * @brief Get actual sampling rate in Hz
 */
uint32_t osc_adc_get_sample_rate_hz(osc_adc_ctx_t *ctx)
{
    if (ctx == NULL) return 0;
    return ctx->sample_rate_hz;
}

/**
 * @brief Check if new data is available (简化版：只要有足够数据就返回 true)
 */
bool osc_adc_has_new_data(osc_adc_ctx_t *ctx)
{
    if (ctx == NULL) return false;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    // 只要缓冲区已满，或者有足够的数据（至少 1000 个样本），就认为有数据
    bool has_data = ctx->buffer_full || (ctx->buffer_write_idx >= 1000);
    
    xSemaphoreGive(ctx->mutex);
    
    return has_data;
}

/**
 * @brief Get captured waveform data (直接从循环缓冲区读取)
 */
esp_err_t osc_adc_get_data(osc_adc_ctx_t *ctx, float *buffer, uint32_t buffer_size, uint32_t *actual_count)
{
    if (ctx == NULL || buffer == NULL || actual_count == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    // 检查是否有足够的数据（至少 1000 个样本）
    uint32_t min_samples = 1000;
    if (!ctx->buffer_full && ctx->buffer_write_idx < min_samples) {
        // 缓冲区还没有足够数据
        xSemaphoreGive(ctx->mutex);
        *actual_count = 0;
        
        // Debug: Log why we're rejecting
        static uint32_t reject_count = 0;
        if (reject_count < 5) {
            ESP_LOGW(TAG, "Not enough data yet: write_idx=%lu, need %lu samples", 
                     ctx->buffer_write_idx, min_samples);
            reject_count++;
        }
        
        return ESP_ERR_NOT_FOUND;
    }
    
    // 确定要读取的样本数
    uint32_t available_samples = ctx->buffer_full ? ctx->storage_depth : ctx->buffer_write_idx;
    uint32_t count = (buffer_size < available_samples) ? buffer_size : available_samples;
    
    // 计算起始位置：从当前写入位置往前数 count 个样本
    uint32_t start_idx;
    if (ctx->buffer_full) {
        // 缓冲区已满，从 write_idx 往前数
        start_idx = (ctx->buffer_write_idx + ctx->storage_depth - count) % ctx->storage_depth;
    } else {
        // 缓冲区未满，从头开始读取所有可用数据
        start_idx = 0;
    }
    
    // 读取数据并转换为电压
    for (uint32_t i = 0; i < count; i++) {
        uint32_t src_idx = (start_idx + i) % ctx->storage_depth;
        buffer[i] = osc_adc_raw_to_voltage(ctx->sample_buffer[src_idx]);
    }
    
    // Debug: Log first few samples to verify conversion
    static uint32_t get_data_counter = 0;
    get_data_counter++;
    if (get_data_counter <= 3 || get_data_counter % 50 == 0) {
        // Calculate min/max for verification
        float vmin = buffer[0], vmax = buffer[0];
        for (uint32_t i = 1; i < count; i++) {
            if (buffer[i] < vmin) vmin = buffer[i];
            if (buffer[i] > vmax) vmax = buffer[i];
        }
        ESP_LOGI(TAG, "🔬 ADC Data #%lu: count=%lu, raw[0]=%u→%.3fV, raw[mid]=%u→%.3fV, Range: %.3fV to %.3fV",
                 get_data_counter, count,
                 ctx->sample_buffer[start_idx], buffer[0],
                 ctx->sample_buffer[(start_idx + count/2) % ctx->storage_depth], buffer[count/2],
                 vmin, vmax);
    }
    
    *actual_count = count;
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

/**
 * @brief Get raw ADC data
 */
esp_err_t osc_adc_get_raw_data(osc_adc_ctx_t *ctx, uint16_t *buffer, uint32_t buffer_size, uint32_t *actual_count)
{
    if (ctx == NULL || buffer == NULL || actual_count == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (!ctx->new_data_available) {
        xSemaphoreGive(ctx->mutex);
        *actual_count = 0;
        return ESP_ERR_NOT_FOUND;
    }
    
    uint32_t count = (buffer_size < ctx->triggered_count) ? buffer_size : ctx->triggered_count;
    memcpy(buffer, ctx->triggered_buffer, count * sizeof(uint16_t));
    
    *actual_count = count;
    ctx->new_data_available = false;
    ctx->trigger_armed = true;
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

/**
 * @brief Convert ADC raw value to voltage
 * 
 * Simple direct conversion for testing:
 * - ADC 0V (raw=0)    -> 0.00V
 * - ADC 1.65V (raw=2048) -> 1.65V
 * - ADC 3.3V (raw=4095)  -> 3.30V
 * 
 * Note: This is for testing with ESP32 ADC directly.
 * In production, external ADC hardware will be used with proper voltage scaling.
 */
float osc_adc_raw_to_voltage(uint16_t raw_value)
{
    // 12-bit ADC: 0-4095 maps to 0-3.3V (with 12dB attenuation)
    // Simple linear conversion, no offset or scaling
    float voltage = (float)raw_value * 3.3f / 4095.0f;
    
    return voltage;
}

/**
 * @brief Get storage depth
 */
uint32_t osc_adc_get_storage_depth(osc_adc_ctx_t *ctx)
{
    if (ctx == NULL) return 0;
    return ctx->storage_depth;
}

/**
 * @brief Check if ADC is currently sampling
 */
bool osc_adc_is_running(osc_adc_ctx_t *ctx)
{
    if (ctx == NULL) return false;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    bool running = ctx->running;
    xSemaphoreGive(ctx->mutex);
    
    return running;
}
