/**
 * @file screenshot_storage.h
 * @brief Screenshot Storage - SD Card Only
 * 
 * Simplified storage that saves screenshots directly to SD card.
 * Naming convention: Screenshot_001.bmp, Screenshot_002.bmp, etc.
 */

#ifndef SCREENSHOT_STORAGE_H
#define SCREENSHOT_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Screenshot dimensions */
#define SCREENSHOT_WIDTH    1024
#define SCREENSHOT_HEIGHT   600

/* Storage location enum (kept for compatibility, but only SD is used) */
typedef enum {
    STORAGE_SD = 0,      /* SD Card storage */
    STORAGE_FLASH = 1,   /* Not used - kept for compatibility */
    STORAGE_PSRAM = 2,   /* Not used - kept for compatibility */
} storage_location_t;

/* Screenshot info structure */
typedef struct {
    char filename[64];           /* Filename */
    uint32_t size;               /* File size in bytes */
    time_t timestamp;            /* Creation timestamp */
    storage_location_t location; /* Always STORAGE_SD */
} screenshot_storage_info_t;

/**
 * @brief Initialize screenshot storage
 * 
 * Initializes SD card storage and scans for existing screenshots.
 * 
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_init(void);

/**
 * @brief Deinitialize screenshot storage
 * 
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_deinit(void);

/**
 * @brief Generate a new screenshot filename
 * 
 * Generates filename with format: Screenshot_001.bmp
 * 
 * @param filename Buffer to store the filename
 * @param size Buffer size
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_generate_filename(char *filename, size_t size);

/**
 * @brief Get full path for a screenshot
 * 
 * @param filename Screenshot filename
 * @param path Buffer to store the full path
 * @param size Buffer size
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_get_path(const char *filename, char *path, size_t size);

/**
 * @brief Save screenshot data to SD card
 * 
 * @param data RGB888 image data
 * @param width Image width
 * @param height Image height
 * @param filename Filename to save as
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_save(const uint8_t *data, int width, int height, const char *filename);

/**
 * @brief Delete a screenshot
 * 
 * @param filename Screenshot filename to delete
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_delete(const char *filename);

/**
 * @brief Get total screenshot count
 * 
 * @return Number of screenshots on SD card
 */
int screenshot_storage_get_count(void);

/**
 * @brief Get screenshot info by index
 * 
 * @param index Screenshot index (0-based)
 * @param info Pointer to store info
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_get_info(int index, screenshot_storage_info_t *info);

/**
 * @brief Refresh screenshot list from SD card
 * 
 * @return ESP_OK on success
 */
esp_err_t screenshot_storage_refresh(void);

/**
 * @brief Check if SD card storage is available
 *
 * @return true if SD card is available
 */
bool screenshot_storage_is_available(void);

/**
 * @brief Get screenshot directory path
 *
 * @return Path to screenshot directory
 */
const char* screenshot_storage_get_dir(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREENSHOT_STORAGE_H */
