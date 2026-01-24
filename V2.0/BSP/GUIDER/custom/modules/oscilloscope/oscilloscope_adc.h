/**
 * @file oscilloscope_adc.h
 * @brief ESP32-P4 ADC Sampling Module for Oscilloscope
 * 
 * Features:
 * - Continuous ADC sampling using DMA
 * - Maximum sampling rate configuration
 * - Circular buffer management
 * - Trigger detection
 */

#ifndef OSCILLOSCOPE_ADC_H
#define OSCILLOSCOPE_ADC_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum storage depth (128K points like DS100 Mini) */
#define OSC_MAX_STORAGE_DEPTH   (128 * 1024)

/* ADC configuration for ESP32-P4 */
#define OSC_ADC_UNIT            ADC_UNIT_1
#define OSC_ADC_CHANNEL         ADC_CHANNEL_6  // GPIO7 for ESP32-P4 (避免与SDIO/LCD冲突)
#define OSC_ADC_ATTEN           ADC_ATTEN_DB_12  // 0-3.3V range
#define OSC_ADC_BITWIDTH        ADC_BITWIDTH_12  // 12-bit resolution

/* Voltage display range configuration */
// Oscilloscope specification: -50V to +50V maximum range
// Testing mode: ESP32 ADC directly measures 0-3.3V (no mapping)
#define OSC_DISPLAY_VOLTAGE_MIN     -50.0f  // Oscilloscope minimum (-50V)
#define OSC_DISPLAY_VOLTAGE_MAX     50.0f   // Oscilloscope maximum (+50V)
#define OSC_DISPLAY_VOLTAGE_CENTER  0.0f    // Center voltage (0V)

/* ADC voltage range (testing with ESP32) */
// ESP32 ADC: 0-3.3V direct measurement (no conversion needed)
#define OSC_ADC_VOLTAGE_MIN         0.0f    // ADC minimum (0V)
#define OSC_ADC_VOLTAGE_MAX         3.3f    // ADC maximum (3.3V)
#define OSC_ADC_MIDPOINT            1.65f   // ADC midpoint (1.65V)

/* Sampling rate tiers - realistic for ESP32-P4 ADC */
typedef enum {
    OSC_SAMPLE_RATE_1MSPS = 0,    // 1 MSa/s (maximum practical)
    OSC_SAMPLE_RATE_500KSPS,      // 500 kSa/s
    OSC_SAMPLE_RATE_200KSPS,      // 200 kSa/s
    OSC_SAMPLE_RATE_100KSPS,      // 100 kSa/s
    OSC_SAMPLE_RATE_50KSPS,       // 50 kSa/s
    OSC_SAMPLE_RATE_10KSPS,       // 10 kSa/s
    OSC_SAMPLE_RATE_1KSPS,        // 1 kSa/s
} osc_sample_rate_t;

/* Trigger configuration */
typedef struct {
    bool enabled;                 // Trigger enabled
    float level_voltage;          // Trigger level in volts
    bool rising_edge;             // true = rising edge, false = falling edge
    float pre_trigger_ratio;      // Pre-trigger ratio (0.0-1.0, typically 0.5)
} osc_trigger_config_t;

/* ADC sampling context */
typedef struct osc_adc_ctx_t osc_adc_ctx_t;

/**
 * @brief Initialize ADC sampling module
 * 
 * @param sample_rate Initial sampling rate
 * @param storage_depth Storage depth in points
 * @return ADC context or NULL on error
 */
osc_adc_ctx_t *osc_adc_init(osc_sample_rate_t sample_rate, uint32_t storage_depth);

/**
 * @brief Deinitialize ADC sampling module
 * 
 * @param ctx ADC context
 */
void osc_adc_deinit(osc_adc_ctx_t *ctx);

/**
 * @brief Start continuous ADC sampling
 * 
 * @param ctx ADC context
 * @return ESP_OK on success
 */
esp_err_t osc_adc_start(osc_adc_ctx_t *ctx);

/**
 * @brief Stop ADC sampling
 * 
 * @param ctx ADC context
 * @return ESP_OK on success
 */
esp_err_t osc_adc_stop(osc_adc_ctx_t *ctx);

/**
 * @brief Configure trigger
 * 
 * @param ctx ADC context
 * @param trigger Trigger configuration
 * @return ESP_OK on success
 */
esp_err_t osc_adc_set_trigger(osc_adc_ctx_t *ctx, const osc_trigger_config_t *trigger);

/**
 * @brief Set sampling rate
 * 
 * @param ctx ADC context
 * @param sample_rate New sampling rate
 * @return ESP_OK on success
 */
esp_err_t osc_adc_set_sample_rate(osc_adc_ctx_t *ctx, osc_sample_rate_t sample_rate);

/**
 * @brief Get actual sampling rate in Hz
 * 
 * @param ctx ADC context
 * @return Sampling rate in Hz
 */
uint32_t osc_adc_get_sample_rate_hz(osc_adc_ctx_t *ctx);

/**
 * @brief Check if new data is available (trigger occurred)
 * 
 * @param ctx ADC context
 * @return true if new triggered data is available
 */
bool osc_adc_has_new_data(osc_adc_ctx_t *ctx);

/**
 * @brief Get captured waveform data
 * 
 * @param ctx ADC context
 * @param buffer Output buffer for voltage data
 * @param buffer_size Size of output buffer
 * @param actual_count Actual number of points copied
 * @return ESP_OK on success
 */
esp_err_t osc_adc_get_data(osc_adc_ctx_t *ctx, float *buffer, uint32_t buffer_size, uint32_t *actual_count);

/**
 * @brief Get raw ADC data (for advanced processing)
 * 
 * @param ctx ADC context
 * @param buffer Output buffer for raw ADC values
 * @param buffer_size Size of output buffer
 * @param actual_count Actual number of points copied
 * @return ESP_OK on success
 */
esp_err_t osc_adc_get_raw_data(osc_adc_ctx_t *ctx, uint16_t *buffer, uint32_t buffer_size, uint32_t *actual_count);

/**
 * @brief Convert ADC raw value to voltage
 * 
 * Maps ADC reading (0-3.3V) to actual voltage range (-50V to +50V)
 * ADC 0V (raw=0) -> -50V
 * ADC 1.65V (raw=2048) -> 0V (center)
 * ADC 3.3V (raw=4095) -> +50V
 * 
 * @param raw_value Raw ADC value (0-4095 for 12-bit)
 * @return Voltage in volts (-50V to +50V range)
 */
float osc_adc_raw_to_voltage(uint16_t raw_value);

/**
 * @brief Get storage depth
 * 
 * @param ctx ADC context
 * @return Storage depth in points
 */
uint32_t osc_adc_get_storage_depth(osc_adc_ctx_t *ctx);

/**
 * @brief Check if ADC is currently sampling
 * 
 * @param ctx ADC context
 * @return true if sampling is active
 */
bool osc_adc_is_running(osc_adc_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // OSCILLOSCOPE_ADC_H
