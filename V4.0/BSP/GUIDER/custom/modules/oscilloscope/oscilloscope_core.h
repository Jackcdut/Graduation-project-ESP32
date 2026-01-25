/**
 * @file oscilloscope_core.h
 * @brief Oscilloscope Core Logic - DS100 Mini Compatible
 * 
 * This module implements the core oscilloscope behavior matching DS100 Mini:
 * - Proper preview area with fixed visible window
 * - Correct time scale and offset behavior in stop mode
 * - Sampling buffer management
 * - ROLL mode for large time scales
 */

#ifndef OSCILLOSCOPE_CORE_H
#define OSCILLOSCOPE_CORE_H

#include "esp_err.h"
#include "oscilloscope_adc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Display parameters */
#define OSC_DISPLAY_WIDTH       688     // Waveform display width in pixels
#define OSC_DISPLAY_HEIGHT      381     // Waveform display height in pixels
#define OSC_GRID_COLS           16      // Horizontal divisions
#define OSC_GRID_ROWS           9       // Vertical divisions

/* Time scale settings (seconds per division) */
typedef enum {
    OSC_TIME_8NS = 0,       // 8ns/div
    OSC_TIME_20NS,          // 20ns/div
    OSC_TIME_40NS,          // 40ns/div
    OSC_TIME_80NS,          // 80ns/div
    OSC_TIME_200NS,         // 200ns/div
    OSC_TIME_400NS,         // 400ns/div
    OSC_TIME_800NS,         // 800ns/div
    OSC_TIME_2US,           // 2us/div
    OSC_TIME_4US,           // 4us/div
    OSC_TIME_8US,           // 8us/div
    OSC_TIME_20US,          // 20us/div
    OSC_TIME_40US,          // 40us/div
    OSC_TIME_80US,          // 80us/div
    OSC_TIME_200US,         // 200us/div
    OSC_TIME_400US,         // 400us/div
    OSC_TIME_800US,         // 800us/div
    OSC_TIME_2MS,           // 2ms/div
    OSC_TIME_4MS,           // 4ms/div
    OSC_TIME_8MS,           // 8ms/div
    OSC_TIME_20MS,          // 20ms/div
    OSC_TIME_40MS,          // 40ms/div
    OSC_TIME_80MS,          // 80ms/div
    OSC_TIME_200MS,         // 200ms/div (ROLL mode starts here)
    OSC_TIME_400MS,         // 400ms/div (ROLL mode)
    OSC_TIME_800MS,         // 800ms/div (ROLL mode)
    OSC_TIME_2S,            // 2s/div (ROLL mode)
    OSC_TIME_4S,            // 4s/div (ROLL mode)
    OSC_TIME_8S,            // 8s/div (ROLL mode)
    OSC_TIME_20S,           // 20s/div (ROLL mode)
    OSC_TIME_40S,           // 40s/div (ROLL mode)
    OSC_TIME_MAX
} osc_time_scale_t;

/* Voltage scale settings (volts per division) */
typedef enum {
    OSC_VOLT_10MV = 0,      // 10mV/div
    OSC_VOLT_20MV,          // 20mV/div
    OSC_VOLT_50MV,          // 50mV/div
    OSC_VOLT_100MV,         // 100mV/div
    OSC_VOLT_200MV,         // 200mV/div
    OSC_VOLT_500MV,         // 500mV/div
    OSC_VOLT_1V,            // 1V/div
    OSC_VOLT_2V,            // 2V/div
    OSC_VOLT_5V,            // 5V/div
    OSC_VOLT_MAX
} osc_volt_scale_t;

/* Oscilloscope operating mode */
typedef enum {
    OSC_MODE_NORMAL = 0,    // Normal triggered mode
    OSC_MODE_ROLL,          // Roll mode (for large time scales)
    OSC_MODE_SINGLE,        // Single shot mode
} osc_mode_t;

/* Oscilloscope state */
typedef enum {
    OSC_STATE_RUNNING = 0,  // Continuously acquiring
    OSC_STATE_STOPPED,      // Stopped (frozen waveform)
    OSC_STATE_WAITING,      // Waiting for trigger
} osc_state_t;

/* Waveform data structure */
typedef struct {
    float *voltage_data;            // Voltage samples
    uint32_t num_points;            // Number of valid points
    uint32_t storage_depth;         // Total storage capacity
    float time_per_sample;          // Time between samples (seconds)
    uint32_t trigger_position;      // Trigger position in buffer
    osc_time_scale_t time_scale;    // Time scale when captured
    osc_volt_scale_t volt_scale;    // Voltage scale when captured
} osc_waveform_t;

/* Oscilloscope core context */
typedef struct osc_core_ctx_t osc_core_ctx_t;

/**
 * @brief Initialize oscilloscope core
 * 
 * @return Core context or NULL on error
 */
osc_core_ctx_t *osc_core_init(void);

/**
 * @brief Deinitialize oscilloscope core
 * 
 * @param ctx Core context
 */
void osc_core_deinit(osc_core_ctx_t *ctx);

/**
 * @brief Start oscilloscope (RUN mode)
 * 
 * @param ctx Core context
 * @return ESP_OK on success
 */
esp_err_t osc_core_start(osc_core_ctx_t *ctx);

/**
 * @brief Stop oscilloscope (STOP mode - freeze current waveform)
 * 
 * @param ctx Core context
 * @return ESP_OK on success
 */
esp_err_t osc_core_stop(osc_core_ctx_t *ctx);

/**
 * @brief Set time scale
 * 
 * @param ctx Core context
 * @param time_scale Time scale setting
 * @return ESP_OK on success
 */
esp_err_t osc_core_set_time_scale(osc_core_ctx_t *ctx, osc_time_scale_t time_scale);

/**
 * @brief Set voltage scale
 * 
 * @param ctx Core context
 * @param volt_scale Voltage scale setting
 * @return ESP_OK on success
 */
esp_err_t osc_core_set_volt_scale(osc_core_ctx_t *ctx, osc_volt_scale_t volt_scale);

/**
 * @brief Set horizontal offset (time position)
 * 
 * In STOP mode, this allows viewing different parts of captured data
 * 
 * @param ctx Core context
 * @param offset_seconds Offset in seconds (can be negative)
 * @return ESP_OK on success
 */
esp_err_t osc_core_set_x_offset(osc_core_ctx_t *ctx, float offset_seconds);

/**
 * @brief Set vertical offset (voltage position)
 * 
 * @param ctx Core context
 * @param offset_volts Offset in volts
 * @return ESP_OK on success
 */
esp_err_t osc_core_set_y_offset(osc_core_ctx_t *ctx, float offset_volts);

/**
 * @brief Set trigger configuration
 * 
 * @param ctx Core context
 * @param trigger Trigger configuration
 * @return ESP_OK on success
 */
esp_err_t osc_core_set_trigger(osc_core_ctx_t *ctx, const osc_trigger_config_t *trigger);

/**
 * @brief Get current waveform for display
 * 
 * This function returns the waveform data to be displayed on screen,
 * taking into account current time scale, offset, and zoom settings.
 * 
 * @param ctx Core context
 * @param display_buffer Output buffer for display points (OSC_DISPLAY_WIDTH points)
 * @param actual_count Actual number of points returned
 * @return ESP_OK on success
 */
esp_err_t osc_core_get_display_waveform(osc_core_ctx_t *ctx, float *display_buffer, uint32_t *actual_count);

/**
 * @brief Get preview waveform (complete captured data overview)
 * 
 * Returns downsampled waveform for preview area display
 * 
 * @param ctx Core context
 * @param preview_buffer Output buffer for preview points
 * @param preview_width Width of preview area in pixels
 * @param actual_count Actual number of points returned
 * @return ESP_OK on success
 */
esp_err_t osc_core_get_preview_waveform(osc_core_ctx_t *ctx, float *preview_buffer, uint32_t preview_width, uint32_t *actual_count);

/**
 * @brief Get visible window position in preview area
 * 
 * Returns the position and size of the visible window (what's shown on main display)
 * relative to the complete captured data in the preview area.
 * 
 * @param ctx Core context
 * @param preview_width Width of preview area in pixels
 * @param window_start Output: start position of visible window (0.0-1.0)
 * @param window_width Output: width of visible window (0.0-1.0)
 * @return ESP_OK on success
 */
esp_err_t osc_core_get_preview_window(osc_core_ctx_t *ctx, uint32_t preview_width, float *window_start, float *window_width);

/**
 * @brief Get current oscilloscope state
 * 
 * @param ctx Core context
 * @return Current state
 */
osc_state_t osc_core_get_state(osc_core_ctx_t *ctx);

/**
 * @brief Get current operating mode
 * 
 * @param ctx Core context
 * @return Current mode
 */
osc_mode_t osc_core_get_mode(osc_core_ctx_t *ctx);

/**
 * @brief Get time scale value in seconds per division
 * 
 * @param time_scale Time scale enum
 * @return Seconds per division
 */
float osc_core_get_time_per_div(osc_time_scale_t time_scale);

/**
 * @brief Get voltage scale value in volts per division
 * 
 * @param volt_scale Voltage scale enum
 * @return Volts per division
 */
float osc_core_get_volts_per_div(osc_volt_scale_t volt_scale);

/**
 * @brief Get time scale string representation
 * 
 * @param time_scale Time scale enum
 * @return String representation (e.g., "1ms", "10us")
 */
const char *osc_core_get_time_scale_str(osc_time_scale_t time_scale);

/**
 * @brief Get voltage scale string representation
 * 
 * @param volt_scale Voltage scale enum
 * @return String representation (e.g., "1V", "500mV")
 */
const char *osc_core_get_volt_scale_str(osc_volt_scale_t volt_scale);

/**
 * @brief Perform auto-adjust (AUTO button)
 * 
 * Automatically adjusts time scale, voltage scale, and offsets
 * to optimally display the current signal.
 * 
 * @param ctx Core context
 * @return ESP_OK on success
 */
esp_err_t osc_core_auto_adjust(osc_core_ctx_t *ctx);

/**
 * @brief Get waveform measurements
 * 
 * @param ctx Core context
 * @param freq_hz Output: frequency in Hz
 * @param vmax Output: maximum voltage
 * @param vmin Output: minimum voltage
 * @param vpp Output: peak-to-peak voltage
 * @param vrms Output: RMS voltage
 * @return ESP_OK on success
 */
esp_err_t osc_core_get_measurements(osc_core_ctx_t *ctx, float *freq_hz, float *vmax, float *vmin, float *vpp, float *vrms);

/**
 * @brief Update oscilloscope (call periodically from timer)
 * 
 * This function should be called periodically (e.g., every 10-50ms) to:
 * - Check for new ADC data
 * - Update display in ROLL mode
 * - Handle trigger conditions
 * 
 * @param ctx Core context
 * @return ESP_OK on success
 */
esp_err_t osc_core_update(osc_core_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // OSCILLOSCOPE_CORE_H
