/**
 * @file oscilloscope_core.c
 * @brief Oscilloscope Core Implementation - DS100 Mini Compatible
 */

#include "oscilloscope_core.h"
#include "oscilloscope_adc.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <math.h>

static const char *TAG = "OscCore";

/* Time scale lookup table (seconds per division) */
static const float time_scale_table[] = {
    [OSC_TIME_8NS]    = 8e-9f,
    [OSC_TIME_20NS]   = 20e-9f,
    [OSC_TIME_40NS]   = 40e-9f,
    [OSC_TIME_80NS]   = 80e-9f,
    [OSC_TIME_200NS]  = 200e-9f,
    [OSC_TIME_400NS]  = 400e-9f,
    [OSC_TIME_800NS]  = 800e-9f,
    [OSC_TIME_2US]    = 2e-6f,
    [OSC_TIME_4US]    = 4e-6f,
    [OSC_TIME_8US]    = 8e-6f,
    [OSC_TIME_20US]   = 20e-6f,
    [OSC_TIME_40US]   = 40e-6f,
    [OSC_TIME_80US]   = 80e-6f,
    [OSC_TIME_200US]  = 200e-6f,
    [OSC_TIME_400US]  = 400e-6f,
    [OSC_TIME_800US]  = 800e-6f,
    [OSC_TIME_2MS]    = 2e-3f,
    [OSC_TIME_4MS]    = 4e-3f,
    [OSC_TIME_8MS]    = 8e-3f,
    [OSC_TIME_20MS]   = 20e-3f,
    [OSC_TIME_40MS]   = 40e-3f,
    [OSC_TIME_80MS]   = 80e-3f,
    [OSC_TIME_200MS]  = 200e-3f,  // ROLL mode starts
    [OSC_TIME_400MS]  = 400e-3f,
    [OSC_TIME_800MS]  = 800e-3f,
    [OSC_TIME_2S]     = 2.0f,
    [OSC_TIME_4S]     = 4.0f,
    [OSC_TIME_8S]     = 8.0f,
    [OSC_TIME_20S]    = 20.0f,
    [OSC_TIME_40S]    = 40.0f,
};

/* Time scale string representations */
static const char *time_scale_strings[] = {
    [OSC_TIME_8NS]    = "8ns",
    [OSC_TIME_20NS]   = "20ns",
    [OSC_TIME_40NS]   = "40ns",
    [OSC_TIME_80NS]   = "80ns",
    [OSC_TIME_200NS]  = "200ns",
    [OSC_TIME_400NS]  = "400ns",
    [OSC_TIME_800NS]  = "800ns",
    [OSC_TIME_2US]    = "2us",
    [OSC_TIME_4US]    = "4us",
    [OSC_TIME_8US]    = "8us",
    [OSC_TIME_20US]   = "20us",
    [OSC_TIME_40US]   = "40us",
    [OSC_TIME_80US]   = "80us",
    [OSC_TIME_200US]  = "200us",
    [OSC_TIME_400US]  = "400us",
    [OSC_TIME_800US]  = "800us",
    [OSC_TIME_2MS]    = "2ms",
    [OSC_TIME_4MS]    = "4ms",
    [OSC_TIME_8MS]    = "8ms",
    [OSC_TIME_20MS]   = "20ms",
    [OSC_TIME_40MS]   = "40ms",
    [OSC_TIME_80MS]   = "80ms",
    [OSC_TIME_200MS]  = "200ms",
    [OSC_TIME_400MS]  = "400ms",
    [OSC_TIME_800MS]  = "800ms",
    [OSC_TIME_2S]     = "2s",
    [OSC_TIME_4S]     = "4s",
    [OSC_TIME_8S]     = "8s",
    [OSC_TIME_20S]    = "20s",
    [OSC_TIME_40S]    = "40s",
};

/* Voltage scale lookup table (volts per division) */
static const float volt_scale_table[] = {
    [OSC_VOLT_10MV]   = 0.01f,
    [OSC_VOLT_20MV]   = 0.02f,
    [OSC_VOLT_50MV]   = 0.05f,
    [OSC_VOLT_100MV]  = 0.1f,
    [OSC_VOLT_200MV]  = 0.2f,
    [OSC_VOLT_500MV]  = 0.5f,
    [OSC_VOLT_1V]     = 1.0f,
    [OSC_VOLT_2V]     = 2.0f,
    [OSC_VOLT_5V]     = 5.0f,
};

/* Voltage scale string representations */
static const char *volt_scale_strings[] = {
    [OSC_VOLT_10MV]   = "10mV",
    [OSC_VOLT_20MV]   = "20mV",
    [OSC_VOLT_50MV]   = "50mV",
    [OSC_VOLT_100MV]  = "100mV",
    [OSC_VOLT_200MV]  = "200mV",
    [OSC_VOLT_500MV]  = "500mV",
    [OSC_VOLT_1V]     = "1V",
    [OSC_VOLT_2V]     = "2V",
    [OSC_VOLT_5V]     = "5V",
};

/* Oscilloscope core context */
struct osc_core_ctx_t {
    /* ADC sampling */
    osc_adc_ctx_t *adc_ctx;
    
    /* Current settings */
    osc_time_scale_t time_scale;
    osc_volt_scale_t volt_scale;
    float x_offset;                 // Horizontal offset in seconds
    float y_offset;                 // Vertical offset in volts
    
    /* State */
    osc_state_t state;
    osc_mode_t mode;
    
    /* Captured waveform data */
    osc_waveform_t captured_waveform;
    osc_waveform_t frozen_waveform;  // Frozen when stopped
    bool has_frozen_data;
    
    /* Trigger configuration */
    osc_trigger_config_t trigger;
    
    /* Measurements cache */
    float measured_freq;
    float measured_vmax;
    float measured_vmin;
    float measured_vpp;
    float measured_vrms;
    bool measurements_valid;
    
    /* Synchronization */
    SemaphoreHandle_t mutex;
};

/**
 * @brief Determine sampling rate based on time scale
 */
static osc_sample_rate_t get_sample_rate_for_time_scale(osc_time_scale_t time_scale)
{
    float time_per_div = time_scale_table[time_scale];
    float total_time = time_per_div * OSC_GRID_COLS;
    
    // Nyquist: need at least 2 samples per period
    // For good display: want at least 10 samples per division
    // Target: OSC_DISPLAY_WIDTH samples for total display time
    float required_rate = OSC_DISPLAY_WIDTH / total_time;
    
    // Select appropriate sampling rate (realistic for ESP32-P4)
    if (required_rate >= 500e3f) return OSC_SAMPLE_RATE_1MSPS;      // >= 500 kHz
    if (required_rate >= 200e3f) return OSC_SAMPLE_RATE_500KSPS;    // >= 200 kHz
    if (required_rate >= 100e3f) return OSC_SAMPLE_RATE_200KSPS;    // >= 100 kHz
    if (required_rate >= 50e3f) return OSC_SAMPLE_RATE_100KSPS;     // >= 50 kHz
    if (required_rate >= 10e3f) return OSC_SAMPLE_RATE_50KSPS;      // >= 10 kHz
    if (required_rate >= 1e3f) return OSC_SAMPLE_RATE_10KSPS;       // >= 1 kHz
    return OSC_SAMPLE_RATE_1KSPS;                                    // < 1 kHz
}

/**
 * @brief Determine storage depth based on time scale
 */
static uint32_t get_storage_depth_for_time_scale(osc_time_scale_t time_scale)
{
    float time_per_div = time_scale_table[time_scale];
    float total_time = time_per_div * OSC_GRID_COLS;
    
    // Calculate required storage depth
    // For fast time scales: use maximum depth for detail
    // For slow time scales: use less depth (limited by memory)
    
    if (time_scale <= OSC_TIME_800NS) {
        return 1024;  // Small depth for very fast signals
    } else if (time_scale <= OSC_TIME_80US) {
        return 10240;  // 10K points
    } else if (time_scale <= OSC_TIME_8MS) {
        return 51200;  // 50K points
    } else {
        return 102400;  // 100K points (close to DS100's 128K)
    }
}

/**
 * @brief Calculate measurements from waveform data
 * 
 * Handles both positive and negative voltages correctly
 */
static void calculate_measurements(osc_core_ctx_t *ctx, const osc_waveform_t *waveform)
{
    if (waveform == NULL || waveform->voltage_data == NULL || waveform->num_points == 0) {
        // No valid data - set measurements to zero
        ctx->measured_vmax = 0.0f;
        ctx->measured_vmin = 0.0f;
        ctx->measured_vpp = 0.0f;
        ctx->measured_vrms = 0.0f;
        ctx->measured_freq = 0.0f;
        ctx->measurements_valid = false;
        ESP_LOGW(TAG, "No valid waveform data for measurements");
        return;
    }
    
    // Initialize with zero (will be updated if valid data exists)
    float vmax = 0.0f;
    float vmin = 0.0f;
    float sum = 0.0f;
    float sum_squares = 0.0f;
    int zero_crossings = 0;
    int last_sign = 0;
    bool first_sample = true;
    
    for (uint32_t i = 0; i < waveform->num_points; i++) {
        float v = waveform->voltage_data[i];
        
        // Initialize min/max with first valid sample
        if (first_sample) {
            vmax = v;
            vmin = v;
            first_sample = false;
        }
        
        // Find max and min
        if (v > vmax) vmax = v;
        if (v < vmin) vmin = v;
        
        // Accumulate for average and RMS
        sum += v;
        sum_squares += v * v;
        
        // Count zero crossings for frequency
        int sign = (v >= 0.0f) ? 1 : -1;
        if (last_sign != 0 && sign != last_sign) {
            zero_crossings++;
        }
        last_sign = sign;
    }
    
    // Sanity check: if values are outside oscilloscope range (-50V to +50V), mark as invalid
    // Note: In testing mode with ESP32 ADC, actual values will be 0-3.3V
    const float MAX_VOLTAGE = OSC_DISPLAY_VOLTAGE_MAX + 5.0f;  // +50V + 5V margin = 55V
    const float MIN_VOLTAGE = OSC_DISPLAY_VOLTAGE_MIN - 5.0f;  // -50V - 5V margin = -55V
    
    if (vmax > MAX_VOLTAGE || vmin < MIN_VOLTAGE) {
        ESP_LOGW(TAG, "Measurements out of range: Vmax=%.2fV, Vmin=%.2fV (oscilloscope range: -50V to +50V) - marking invalid", 
                 vmax, vmin);
        ctx->measured_vmax = 0.0f;
        ctx->measured_vmin = 0.0f;
        ctx->measured_vpp = 0.0f;
        ctx->measured_vrms = 0.0f;
        ctx->measured_freq = 0.0f;
        ctx->measurements_valid = false;
        return;
    }
    
    ctx->measured_vmax = vmax;
    ctx->measured_vmin = vmin;
    ctx->measured_vpp = vmax - vmin;
    ctx->measured_vrms = sqrtf(sum_squares / waveform->num_points);
    
    // Calculate frequency from zero crossings
    if (zero_crossings >= 2) {
        float total_time = waveform->time_per_sample * waveform->num_points;
        float periods = zero_crossings / 2.0f;
        ctx->measured_freq = periods / total_time;
    } else {
        ctx->measured_freq = 0.0f;
    }
    
    ctx->measurements_valid = true;
    
    // Debug log - ALWAYS print first few times to verify code is running
    static uint32_t log_counter = 0;
    log_counter++;
    if (log_counter <= 5 || log_counter % 50 == 0) {  // Print first 5 times, then every 50
        ESP_LOGI(TAG, "📊 Measurements #%lu: Vmax=%.3fV, Vmin=%.3fV, Vpp=%.3fV, Vrms=%.3fV, Freq=%.1fHz (from %lu samples)",
                 log_counter, vmax, vmin, vmax - vmin, ctx->measured_vrms, ctx->measured_freq, waveform->num_points);
    }
}

/**
 * @brief Initialize oscilloscope core
 */
osc_core_ctx_t *osc_core_init(void)
{
    ESP_LOGI(TAG, "Initializing oscilloscope core");
    
    osc_core_ctx_t *ctx = heap_caps_malloc(sizeof(osc_core_ctx_t), MALLOC_CAP_8BIT);
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return NULL;
    }
    memset(ctx, 0, sizeof(osc_core_ctx_t));
    
    // Initialize default settings
    ctx->time_scale = OSC_TIME_2MS;  // 2ms/div default
    ctx->volt_scale = OSC_VOLT_1V;   // 1V/div default
    ctx->x_offset = 0.0f;
    ctx->y_offset = 0.0f;
    ctx->state = OSC_STATE_STOPPED;
    ctx->mode = OSC_MODE_NORMAL;
    
    // Initialize trigger (AUTO mode by default for easier signal viewing)
    ctx->trigger.enabled = false;  // false = AUTO trigger mode
    ctx->trigger.level_voltage = 0.0f;  // 0V (center of -50V to +50V range)
    ctx->trigger.rising_edge = true;
    ctx->trigger.pre_trigger_ratio = 0.5f;
    
    // Create mutex
    ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(ctx);
        return NULL;
    }
    
    // Initialize ADC with appropriate settings
    osc_sample_rate_t sample_rate = get_sample_rate_for_time_scale(ctx->time_scale);
    uint32_t storage_depth = get_storage_depth_for_time_scale(ctx->time_scale);
    
    ctx->adc_ctx = osc_adc_init(sample_rate, storage_depth);
    if (ctx->adc_ctx == NULL) {
        ESP_LOGE(TAG, "Failed to initialize ADC");
        vSemaphoreDelete(ctx->mutex);
        free(ctx);
        return NULL;
    }
    
    // Configure ADC trigger
    osc_adc_set_trigger(ctx->adc_ctx, &ctx->trigger);
    
    // Allocate waveform buffers
    ctx->captured_waveform.storage_depth = storage_depth;
    ctx->captured_waveform.voltage_data = heap_caps_malloc(storage_depth * sizeof(float), MALLOC_CAP_SPIRAM);
    
    ctx->frozen_waveform.storage_depth = storage_depth;
    ctx->frozen_waveform.voltage_data = heap_caps_malloc(storage_depth * sizeof(float), MALLOC_CAP_SPIRAM);
    
    if (ctx->captured_waveform.voltage_data == NULL || ctx->frozen_waveform.voltage_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate waveform buffers");
        if (ctx->captured_waveform.voltage_data) heap_caps_free(ctx->captured_waveform.voltage_data);
        if (ctx->frozen_waveform.voltage_data) heap_caps_free(ctx->frozen_waveform.voltage_data);
        osc_adc_deinit(ctx->adc_ctx);
        vSemaphoreDelete(ctx->mutex);
        free(ctx);
        return NULL;
    }
    
    ESP_LOGI(TAG, "Oscilloscope core initialized successfully");
    return ctx;
}

/**
 * @brief Deinitialize oscilloscope core
 */
void osc_core_deinit(osc_core_ctx_t *ctx)
{
    if (ctx == NULL) return;
    
    if (ctx->state == OSC_STATE_RUNNING) {
        osc_core_stop(ctx);
    }
    
    if (ctx->adc_ctx) {
        osc_adc_deinit(ctx->adc_ctx);
    }
    
    if (ctx->captured_waveform.voltage_data) {
        heap_caps_free(ctx->captured_waveform.voltage_data);
    }
    
    if (ctx->frozen_waveform.voltage_data) {
        heap_caps_free(ctx->frozen_waveform.voltage_data);
    }
    
    if (ctx->mutex) {
        vSemaphoreDelete(ctx->mutex);
    }
    
    free(ctx);
    ESP_LOGI(TAG, "Oscilloscope core deinitialized");
}

/**
 * @brief Start oscilloscope
 */
esp_err_t osc_core_start(osc_core_ctx_t *ctx)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (ctx->state == OSC_STATE_RUNNING) {
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }
    
    // Start ADC sampling
    esp_err_t ret = osc_adc_start(ctx->adc_ctx);
    if (ret == ESP_OK) {
        ctx->state = OSC_STATE_RUNNING;
        ctx->has_frozen_data = false;
        ESP_LOGI(TAG, "Oscilloscope started");
    }
    
    xSemaphoreGive(ctx->mutex);
    return ret;
}

/**
 * @brief Stop oscilloscope (freeze current waveform)
 */
esp_err_t osc_core_stop(osc_core_ctx_t *ctx)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (ctx->state == OSC_STATE_STOPPED) {
        xSemaphoreGive(ctx->mutex);
        return ESP_OK;
    }
    
    // Stop ADC sampling
    esp_err_t ret = osc_adc_stop(ctx->adc_ctx);
    if (ret == ESP_OK) {
        ctx->state = OSC_STATE_STOPPED;
        
        // Freeze current waveform
        if (ctx->captured_waveform.num_points > 0) {
            memcpy(ctx->frozen_waveform.voltage_data, ctx->captured_waveform.voltage_data,
                   ctx->captured_waveform.num_points * sizeof(float));
            ctx->frozen_waveform.num_points = ctx->captured_waveform.num_points;
            ctx->frozen_waveform.time_per_sample = ctx->captured_waveform.time_per_sample;
            ctx->frozen_waveform.trigger_position = ctx->captured_waveform.trigger_position;
            ctx->frozen_waveform.time_scale = ctx->time_scale;
            ctx->frozen_waveform.volt_scale = ctx->volt_scale;
            ctx->has_frozen_data = true;
        }
        
        ESP_LOGI(TAG, "Oscilloscope stopped (waveform frozen)");
    }
    
    xSemaphoreGive(ctx->mutex);
    return ret;
}

/**
 * @brief Set time scale
 */
esp_err_t osc_core_set_time_scale(osc_core_ctx_t *ctx, osc_time_scale_t time_scale)
{
    if (ctx == NULL || time_scale >= OSC_TIME_MAX) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    ctx->time_scale = time_scale;
    
    // Determine if ROLL mode should be used
    if (time_scale >= OSC_TIME_200MS) {
        ctx->mode = OSC_MODE_ROLL;
    } else {
        ctx->mode = OSC_MODE_NORMAL;
    }
    
    // Update ADC sampling rate if running
    if (ctx->state == OSC_STATE_RUNNING) {
        osc_sample_rate_t sample_rate = get_sample_rate_for_time_scale(time_scale);
        osc_adc_set_sample_rate(ctx->adc_ctx, sample_rate);
    }
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

/**
 * @brief Set voltage scale
 */
esp_err_t osc_core_set_volt_scale(osc_core_ctx_t *ctx, osc_volt_scale_t volt_scale)
{
    if (ctx == NULL || volt_scale >= OSC_VOLT_MAX) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->volt_scale = volt_scale;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

/**
 * @brief Set horizontal offset
 */
esp_err_t osc_core_set_x_offset(osc_core_ctx_t *ctx, float offset_seconds)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    // In STOP mode, limit offset to captured data range
    if (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen_data) {
        float max_time = ctx->frozen_waveform.num_points * ctx->frozen_waveform.time_per_sample;
        float display_time = time_scale_table[ctx->time_scale] * OSC_GRID_COLS;
        float max_offset = (max_time - display_time) / 2.0f;
        
        if (offset_seconds > max_offset) offset_seconds = max_offset;
        if (offset_seconds < -max_offset) offset_seconds = -max_offset;
    }
    
    ctx->x_offset = offset_seconds;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

/**
 * @brief Set vertical offset
 */
esp_err_t osc_core_set_y_offset(osc_core_ctx_t *ctx, float offset_volts)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->y_offset = offset_volts;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

/**
 * @brief Set trigger configuration
 */
esp_err_t osc_core_set_trigger(osc_core_ctx_t *ctx, const osc_trigger_config_t *trigger)
{
    if (ctx == NULL || trigger == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    memcpy(&ctx->trigger, trigger, sizeof(osc_trigger_config_t));
    osc_adc_set_trigger(ctx->adc_ctx, trigger);
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

// Continued in next part...

/**
 * @brief Get current waveform for display
 */
esp_err_t osc_core_get_display_waveform(osc_core_ctx_t *ctx, float *display_buffer, uint32_t *actual_count)
{
    if (ctx == NULL || display_buffer == NULL || actual_count == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    osc_waveform_t *waveform = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen_data) ?
                                &ctx->frozen_waveform : &ctx->captured_waveform;
    
    if (waveform->num_points == 0) {
        xSemaphoreGive(ctx->mutex);
        *actual_count = 0;
        return ESP_ERR_NOT_FOUND;
    }
    
    // Calculate display parameters
    float time_per_div = time_scale_table[ctx->time_scale];
    float display_time = time_per_div * OSC_GRID_COLS;
    
    // In STOP mode, time scale can be different from capture time scale
    // This implements the zoom behavior
    float time_scale_ratio = 1.0f;
    if (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen_data) {
        float frozen_time_per_div = time_scale_table[waveform->time_scale];
        time_scale_ratio = frozen_time_per_div / time_per_div;
    }
    
    // Calculate start position based on offset
    float trigger_time = waveform->trigger_position * waveform->time_per_sample;
    float start_time = trigger_time - (display_time / 2.0f) + ctx->x_offset;
    
    if (start_time < 0.0f) start_time = 0.0f;
    
    uint32_t start_idx = (uint32_t)(start_time / waveform->time_per_sample);
    if (start_idx >= waveform->num_points) start_idx = waveform->num_points - 1;
    
    // Resample waveform data to display width
    float sample_step = (display_time / waveform->time_per_sample) / OSC_DISPLAY_WIDTH;
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < OSC_DISPLAY_WIDTH && count < OSC_DISPLAY_WIDTH; i++) {
        uint32_t src_idx = start_idx + (uint32_t)(i * sample_step);
        
        if (src_idx >= waveform->num_points) break;
        
        // Apply Y offset
        display_buffer[count] = waveform->voltage_data[src_idx] + ctx->y_offset;
        count++;
    }
    
    *actual_count = count;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

/**
 * @brief Get preview waveform
 */
esp_err_t osc_core_get_preview_waveform(osc_core_ctx_t *ctx, float *preview_buffer, uint32_t preview_width, uint32_t *actual_count)
{
    if (ctx == NULL || preview_buffer == NULL || actual_count == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    osc_waveform_t *waveform = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen_data) ?
                                &ctx->frozen_waveform : &ctx->captured_waveform;
    
    if (waveform->num_points == 0) {
        xSemaphoreGive(ctx->mutex);
        *actual_count = 0;
        return ESP_ERR_NOT_FOUND;
    }
    
    // Downsample complete waveform to preview width
    float sample_step = (float)waveform->num_points / preview_width;
    
    for (uint32_t i = 0; i < preview_width; i++) {
        uint32_t src_idx = (uint32_t)(i * sample_step);
        if (src_idx >= waveform->num_points) src_idx = waveform->num_points - 1;
        
        preview_buffer[i] = waveform->voltage_data[src_idx];
    }
    
    *actual_count = preview_width;
    xSemaphoreGive(ctx->mutex);
    
    return ESP_OK;
}

/**
 * @brief Get visible window position in preview area
 */
esp_err_t osc_core_get_preview_window(osc_core_ctx_t *ctx, uint32_t preview_width, float *window_start, float *window_width)
{
    if (ctx == NULL || window_start == NULL || window_width == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    osc_waveform_t *waveform = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen_data) ?
                                &ctx->frozen_waveform : &ctx->captured_waveform;
    
    if (waveform->num_points == 0) {
        xSemaphoreGive(ctx->mutex);
        *window_start = 0.0f;
        *window_width = 1.0f;
        return ESP_ERR_NOT_FOUND;
    }
    
    // Calculate total captured time
    float total_time = waveform->num_points * waveform->time_per_sample;
    
    // Calculate display time (visible window time)
    float time_per_div = time_scale_table[ctx->time_scale];
    float display_time = time_per_div * OSC_GRID_COLS;
    
    // Calculate window width as fraction of total time
    *window_width = display_time / total_time;
    if (*window_width > 1.0f) *window_width = 1.0f;
    
    // Calculate window start position
    float trigger_time = waveform->trigger_position * waveform->time_per_sample;
    float start_time = trigger_time - (display_time / 2.0f) + ctx->x_offset;
    
    if (start_time < 0.0f) start_time = 0.0f;
    if (start_time > total_time - display_time) start_time = total_time - display_time;
    
    *window_start = start_time / total_time;
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

/**
 * @brief Get current oscilloscope state
 */
osc_state_t osc_core_get_state(osc_core_ctx_t *ctx)
{
    if (ctx == NULL) return OSC_STATE_STOPPED;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    osc_state_t state = ctx->state;
    xSemaphoreGive(ctx->mutex);
    
    return state;
}

/**
 * @brief Get current operating mode
 */
osc_mode_t osc_core_get_mode(osc_core_ctx_t *ctx)
{
    if (ctx == NULL) return OSC_MODE_NORMAL;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    osc_mode_t mode = ctx->mode;
    xSemaphoreGive(ctx->mutex);
    
    return mode;
}

/**
 * @brief Get time scale value in seconds per division
 */
float osc_core_get_time_per_div(osc_time_scale_t time_scale)
{
    if (time_scale >= OSC_TIME_MAX) return 0.0f;
    return time_scale_table[time_scale];
}

/**
 * @brief Get voltage scale value in volts per division
 */
float osc_core_get_volts_per_div(osc_volt_scale_t volt_scale)
{
    if (volt_scale >= OSC_VOLT_MAX) return 0.0f;
    return volt_scale_table[volt_scale];
}

/**
 * @brief Get time scale string representation
 */
const char *osc_core_get_time_scale_str(osc_time_scale_t time_scale)
{
    if (time_scale >= OSC_TIME_MAX) return "???";
    return time_scale_strings[time_scale];
}

/**
 * @brief Get voltage scale string representation
 */
const char *osc_core_get_volt_scale_str(osc_volt_scale_t volt_scale)
{
    if (volt_scale >= OSC_VOLT_MAX) return "???";
    return volt_scale_strings[volt_scale];
}

/**
 * @brief Perform auto-adjust (improved algorithm for -50V to +50V range)
 */
esp_err_t osc_core_auto_adjust(osc_core_ctx_t *ctx)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    osc_waveform_t *waveform = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen_data) ?
                                &ctx->frozen_waveform : &ctx->captured_waveform;
    
    if (waveform->num_points == 0) {
        xSemaphoreGive(ctx->mutex);
        ESP_LOGW(TAG, "No waveform data for auto-adjust");
        return ESP_ERR_NOT_FOUND;
    }
    
    // Calculate signal characteristics
    float vmax = 50.0f;
    float vmin = -50.0f;
    int zero_crossings = 0;
    int last_sign = 0;
    
    for (uint32_t i = 0; i < waveform->num_points; i++) {
        float v = waveform->voltage_data[i];
        if (v > vmax) vmax = v;
        if (v < vmin) vmin = v;
        
        // Count zero crossings for frequency estimation
        int sign = (v >= 0.0f) ? 1 : -1;
        if (last_sign != 0 && sign != last_sign) {
            zero_crossings++;
        }
        last_sign = sign;
    }
    
    float vpp = vmax - vmin;
    float vcenter = (vmax + vmin) / 2.0f;
    
    // Check if signal is valid
    if (vpp < 0.01f) {
        xSemaphoreGive(ctx->mutex);
        ESP_LOGW(TAG, "Signal too small for auto-adjust (Vpp=%.3fV)", vpp);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Auto-adjust voltage scale
    // Target: signal occupies 6-7 divisions (out of 9 total) for good visibility
    float target_volts_per_div = vpp / 6.5f;
    osc_volt_scale_t best_volt_scale = OSC_VOLT_1V;
    float best_diff = 1000.0f;
    
    for (int i = 0; i < OSC_VOLT_MAX; i++) {
        float diff = fabsf(volt_scale_table[i] - target_volts_per_div);
        if (diff < best_diff) {
            best_diff = diff;
            best_volt_scale = (osc_volt_scale_t)i;
        }
    }
    
    ctx->volt_scale = best_volt_scale;
    
    // Auto-adjust time scale based on frequency
    // Estimate frequency from zero crossings
    if (zero_crossings >= 2) {
        float total_time = waveform->num_points * waveform->time_per_sample;
        float periods = zero_crossings / 2.0f;
        float estimated_freq = periods / total_time;
        
        if (estimated_freq > 0.1f) {
            // Target: show 2-4 complete cycles on screen
            float signal_period = 1.0f / estimated_freq;
            float target_cycles = 3.0f;
            float total_time_needed = signal_period * target_cycles;
            float target_time_per_div = total_time_needed / OSC_GRID_COLS;
            
            osc_time_scale_t best_time_scale = OSC_TIME_2MS;
            best_diff = 1000.0f;
            
            for (int i = 0; i < OSC_TIME_MAX; i++) {
                // Skip ROLL mode time scales for auto-adjust
                if (i >= OSC_TIME_200MS) break;
                
                float diff = fabsf(time_scale_table[i] - target_time_per_div);
                if (diff < best_diff) {
                    best_diff = diff;
                    best_time_scale = (osc_time_scale_t)i;
                }
            }
            
            ctx->time_scale = best_time_scale;
            
            ESP_LOGI(TAG, "Auto-adjust: Freq≈%.1fHz, T/div=%s", 
                     estimated_freq, time_scale_strings[best_time_scale]);
        }
    }
    
    // Auto-adjust Y offset to center signal on screen
    // The signal center should be at 0V (screen center)
    // Y_offset shifts the waveform: positive offset moves waveform up
    // To center signal at vcenter, we need: y_offset = -vcenter
    ctx->y_offset = -vcenter;
    
    // Reset X offset to center on trigger
    ctx->x_offset = 0.0f;
    
    ESP_LOGI(TAG, "Auto-adjust complete: V/div=%s, Vpp=%.2fV, Vcenter=%.2fV, Y-offset=%.2fV", 
             volt_scale_strings[ctx->volt_scale], vpp, vcenter, ctx->y_offset);
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

/**
 * @brief Get waveform measurements
 */
esp_err_t osc_core_get_measurements(osc_core_ctx_t *ctx, float *freq_hz, float *vmax, float *vmin, float *vpp, float *vrms)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    // Recalculate measurements if not valid
    if (!ctx->measurements_valid) {
        osc_waveform_t *waveform = (ctx->state == OSC_STATE_STOPPED && ctx->has_frozen_data) ?
                                    &ctx->frozen_waveform : &ctx->captured_waveform;
        calculate_measurements(ctx, waveform);
    }
    
    // Return measurements (will be 0 if invalid)
    if (freq_hz) *freq_hz = ctx->measured_freq;
    if (vmax) *vmax = ctx->measured_vmax;
    if (vmin) *vmin = ctx->measured_vmin;
    if (vpp) *vpp = ctx->measured_vpp;
    if (vrms) *vrms = ctx->measured_vrms;
    
    xSemaphoreGive(ctx->mutex);
    
    // Return error if measurements are invalid
    return ctx->measurements_valid ? ESP_OK : ESP_ERR_NOT_FOUND;
}

/**
 * @brief Update oscilloscope (call periodically)
 */
esp_err_t osc_core_update(osc_core_ctx_t *ctx)
{
    if (ctx == NULL) return ESP_ERR_INVALID_ARG;
    
    // Debug: Log function entry
    static uint32_t update_call_count = 0;
    update_call_count++;
    if (update_call_count <= 5 || update_call_count % 100 == 0) {
        ESP_LOGI(TAG, "📥 osc_core_update() called #%lu, state=%d", update_call_count, ctx->state);
    }
    
    // Only update if running
    if (ctx->state != OSC_STATE_RUNNING) {
        if (update_call_count <= 5) {
            ESP_LOGW(TAG, "Not running, skipping update");
        }
        return ESP_OK;
    }
    
    // Check for new ADC data
    bool has_data = osc_adc_has_new_data(ctx->adc_ctx);
    if (update_call_count <= 5 || update_call_count % 100 == 0) {
        ESP_LOGI(TAG, "ADC has_new_data=%d", has_data);
    }
    
    if (!has_data) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "✅ New ADC data available!");
    
    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    // Get new data from ADC
    uint32_t actual_count = 0;
    esp_err_t ret = osc_adc_get_data(ctx->adc_ctx, ctx->captured_waveform.voltage_data,
                                      ctx->captured_waveform.storage_depth, &actual_count);
    
    if (ret == ESP_OK && actual_count > 0) {
        ctx->captured_waveform.num_points = actual_count;
        ctx->captured_waveform.time_per_sample = 1.0f / osc_adc_get_sample_rate_hz(ctx->adc_ctx);
        ctx->captured_waveform.trigger_position = actual_count / 2;  // Assume trigger at center
        ctx->captured_waveform.time_scale = ctx->time_scale;
        ctx->captured_waveform.volt_scale = ctx->volt_scale;
        
        // Invalidate measurements (will be recalculated on next request)
        ctx->measurements_valid = false;
        
        ESP_LOGI("OscCore", "Captured %lu samples, time_per_sample=%.6f us", 
                 actual_count, ctx->captured_waveform.time_per_sample * 1e6f);
    } else {
        ESP_LOGW("OscCore", "Failed to get ADC data: %s", esp_err_to_name(ret));
    }
    
    xSemaphoreGive(ctx->mutex);
    return ret;
}
