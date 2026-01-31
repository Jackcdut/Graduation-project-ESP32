/**
 * @file oscilloscope_export.h
 * @brief Oscilloscope Data Export - SD Card
 * 
 * Exports oscilloscope waveform data to SD card as CSV files.
 * Naming: Oscilloscope_001.csv, Oscilloscope_002.csv, etc.
 */

#ifndef OSCILLOSCOPE_EXPORT_H
#define OSCILLOSCOPE_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Maximum number of waveform data points
 */
#define OSC_MAX_DATA_POINTS 1000

/**
 * @brief Export file format
 */
typedef enum {
    OSC_EXPORT_FORMAT_TXT = 0,  // Plain text format
    OSC_EXPORT_FORMAT_CSV = 1,  // CSV format (Excel compatible)
    OSC_EXPORT_FORMAT_BOTH = 2  // Both TXT and CSV
} osc_export_format_t;

/**
 * @brief Waveform data structure
 */
typedef struct {
    float data[OSC_MAX_DATA_POINTS];     // Waveform data points in volts (actual voltage values)
    int num_points;                      // Number of valid data points
    float time_scale;                    // Time scale (seconds per division)
    float volt_scale;                    // Voltage scale (volts per division)
    float frequency;                     // Measured frequency (Hz)
    float vmax;                          // Maximum voltage
    float vmin;                          // Minimum voltage
    float vpp;                           // Peak-to-peak voltage
    float vrms;                          // RMS voltage
    bool is_fft;                         // True if FFT data
    char timestamp[32];                  // Timestamp string
} osc_waveform_data_t;

/**
 * @brief Initialize oscilloscope export module
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_init(void);

/**
 * @brief Deinitialize oscilloscope export module
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_deinit(void);

/**
 * @brief Store current waveform data for export
 * 
 * @param waveform Waveform data to store
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_store_waveform(const osc_waveform_data_t *waveform);

/**
 * @brief Store current FFT data for export
 * 
 * @param fft_data FFT data to store
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_store_fft(const osc_waveform_data_t *fft_data);

/**
 * @brief Start USB MSC mode for data export
 * 
 * Creates virtual disk with TXT files containing waveform data
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_start_usb(void);

/**
 * @brief Stop USB MSC mode
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_stop_usb(void);

/**
 * @brief Check if USB MSC is active
 * 
 * @return true if USB MSC is active, false otherwise
 */
bool osc_export_is_usb_active(void);

/**
 * @brief Clear all stored waveform data
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_clear_data(void);

/**
 * @brief Set export file format
 *
 * @param format Export format (TXT, CSV, or both)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_set_format(osc_export_format_t format);

/**
 * @brief Get current export file format
 *
 * @return Current export format
 */
osc_export_format_t osc_export_get_format(void);

/**
 * @brief Get export file counter (number of exports performed)
 *
 * @return Export counter value
 */
uint32_t osc_export_get_counter(void);

/**
 * @brief Reset export file counter
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_reset_counter(void);

/**
 * @brief Get current timestamp string (from RTC if available)
 *
 * @param buffer Buffer to store timestamp string
 * @param buffer_size Size of buffer
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t osc_export_get_timestamp(char *buffer, size_t buffer_size);

/**
 * @brief Save waveform/FFT data to SD card
 * 
 * @return ESP_OK on success
 */
esp_err_t osc_export_save_to_sd(void);

/**
 * @brief Check if SD card is available for exports
 *
 * @return true if SD card is available
 */
bool osc_export_is_sd_available(void);

/**
 * @brief Get oscilloscope export directory path
 *
 * @return Path to oscilloscope directory
 */
const char* osc_export_get_dir(void);

/* Deprecated - kept for compatibility */
uint8_t* osc_export_get_disk_image(uint32_t *disk_size);

#ifdef __cplusplus
}
#endif

#endif /* OSCILLOSCOPE_EXPORT_H */

