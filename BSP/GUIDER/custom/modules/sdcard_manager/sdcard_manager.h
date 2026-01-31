/**
 * @file sdcard_manager.h
 * @brief SD Card Management Module
 * 
 * Provides SD card status monitoring, file statistics, and management functions
 * for the Settings page SD Card Manager UI.
 */

#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============== Constants ============== */

#define SDCARD_MOUNT_POINT          "/sdcard"
#define SDCARD_SCREENSHOT_DIR       "/sdcard/Screenshots"
#define SDCARD_OSCILLOSCOPE_DIR     "/sdcard/Oscilloscope"
#define SDCARD_CONFIG_DIR           "/sdcard/Config"
#define SDCARD_MEDIA_DIR            "/sdcard/Media"

#define SDCARD_MAX_RECENT_FILES     8
#define SDCARD_MAX_FILENAME_LEN     64
#define SDCARD_MAX_PATH_LEN         128

/* ============== Types ============== */

/**
 * @brief File type enumeration
 */
typedef enum {
    SDCARD_FILE_TYPE_UNKNOWN = 0,
    SDCARD_FILE_TYPE_SCREENSHOT,    /* .bmp files in Screenshots folder */
    SDCARD_FILE_TYPE_OSCILLOSCOPE,  /* .csv files in Oscilloscope folder */
    SDCARD_FILE_TYPE_CONFIG,        /* Configuration files */
    SDCARD_FILE_TYPE_MEDIA,         /* Media files (video/audio) in Media folder */
    SDCARD_FILE_TYPE_OTHER
} sdcard_file_type_t;

/**
 * @brief File information structure
 */
typedef struct {
    char name[SDCARD_MAX_FILENAME_LEN];
    char path[SDCARD_MAX_PATH_LEN];
    sdcard_file_type_t type;
    uint32_t size;                  /* File size in bytes */
    time_t created_time;            /* Creation/modification time */
    time_t accessed_time;           /* Last access time */
} sdcard_file_info_t;

/**
 * @brief Storage statistics structure
 */
typedef struct {
    /* Overall SD card stats */
    uint64_t total_bytes;           /* Total SD card capacity */
    uint64_t used_bytes;            /* Used space */
    uint64_t free_bytes;            /* Free space */
    
    /* Per-category stats */
    uint32_t screenshot_count;
    uint64_t screenshot_bytes;
    
    uint32_t oscilloscope_count;
    uint64_t oscilloscope_bytes;
    
    uint32_t config_count;
    uint64_t config_bytes;
    
    uint32_t media_count;
    uint64_t media_bytes;
    
    uint32_t other_count;
    uint64_t other_bytes;
    
    /* I/O stats */
    uint32_t read_speed_kbps;       /* Current read speed in KB/s */
    uint32_t write_speed_kbps;      /* Current write speed in KB/s */
    uint64_t today_write_bytes;     /* Bytes written today */
    uint32_t avg_file_size;         /* Average file size */
} sdcard_stats_t;

/**
 * @brief SD card status
 */
typedef enum {
    SDCARD_STATUS_NOT_PRESENT = 0,
    SDCARD_STATUS_MOUNTED,
    SDCARD_STATUS_ERROR,
    SDCARD_STATUS_READING,
    SDCARD_STATUS_WRITING
} sdcard_status_t;

/* ============== Initialization ============== */

/**
 * @brief Initialize SD card manager module
 * @return ESP_OK on success
 */
esp_err_t sdcard_manager_init(void);

/**
 * @brief Deinitialize SD card manager module
 */
void sdcard_manager_deinit(void);

/* ============== Status Functions ============== */

/**
 * @brief Get current SD card status
 * @return Current status
 */
sdcard_status_t sdcard_manager_get_status(void);

/**
 * @brief Check if SD card is inserted and mounted
 * @return true if mounted
 */
bool sdcard_manager_is_mounted(void);

/**
 * @brief Get storage statistics
 * @param[out] stats Pointer to stats structure to fill
 * @return ESP_OK on success
 */
esp_err_t sdcard_manager_get_stats(sdcard_stats_t *stats);

/**
 * @brief Refresh statistics (scan directories)
 * @return ESP_OK on success
 */
esp_err_t sdcard_manager_refresh_stats(void);

/* ============== File Operations ============== */

/**
 * @brief Get list of files in a category
 * @param[in] type File type to list
 * @param[out] files Array to store file info
 * @param[in] max_files Maximum number of files to return
 * @param[out] count Actual number of files found
 * @return ESP_OK on success
 */
esp_err_t sdcard_manager_list_files(sdcard_file_type_t type, 
                                     sdcard_file_info_t *files, 
                                     uint32_t max_files, 
                                     uint32_t *count);

/**
 * @brief Get file information
 * @param[in] path Full path to file
 * @param[out] info File information
 * @return ESP_OK on success
 */
esp_err_t sdcard_manager_get_file_info(const char *path, sdcard_file_info_t *info);

/**
 * @brief Delete a file
 * @param[in] path Full path to file
 * @return ESP_OK on success
 */
esp_err_t sdcard_manager_delete_file(const char *path);

/**
 * @brief Delete multiple files
 * @param[in] paths Array of file paths
 * @param[in] count Number of files to delete
 * @param[out] deleted Number of files successfully deleted
 * @return ESP_OK if all deleted, ESP_FAIL if some failed
 */
esp_err_t sdcard_manager_delete_files(const char **paths, uint32_t count, uint32_t *deleted);

/* ============== Recent Files ============== */

/**
 * @brief Get recent files list
 * @param[out] files Array to store file info
 * @param[in] max_files Maximum number of files to return
 * @param[out] count Actual number of files
 * @return ESP_OK on success
 */
esp_err_t sdcard_manager_get_recent_files(sdcard_file_info_t *files, 
                                           uint32_t max_files, 
                                           uint32_t *count);

/**
 * @brief Record file access (updates recent files list)
 * @param[in] path Full path to file
 */
void sdcard_manager_record_access(const char *path);

/* ============== Utility Functions ============== */

/**
 * @brief Format file size to human readable string
 * @param[in] bytes Size in bytes
 * @param[out] buffer Output buffer
 * @param[in] buffer_size Buffer size
 */
void sdcard_manager_format_size(uint64_t bytes, char *buffer, size_t buffer_size);

/**
 * @brief Format time to human readable string
 * @param[in] t Time value
 * @param[out] buffer Output buffer
 * @param[in] buffer_size Buffer size
 */
void sdcard_manager_format_time(time_t t, char *buffer, size_t buffer_size);

/**
 * @brief Get file type icon character (for LVGL)
 * @param[in] type File type
 * @return Icon character string
 */
const char* sdcard_manager_get_type_icon(sdcard_file_type_t type);

/**
 * @brief Get file type name
 * @param[in] type File type
 * @return Type name string
 */
const char* sdcard_manager_get_type_name(sdcard_file_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* SDCARD_MANAGER_H */

