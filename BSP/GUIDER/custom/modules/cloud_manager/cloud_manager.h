/**
 * @file cloud_manager.h
 * @brief OneNet Cloud Platform Device Manager
 * 
 * This module handles:
 * - Device registration with OneNet platform
 * - Device activation state management
 * - Data upload to cloud (automatic and manual)
 * - Cloud sync status tracking
 * - Persistent state storage in NVS
 */

#ifndef CLOUD_MANAGER_H
#define CLOUD_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#define CLOUD_DEVICE_CODE_LEN       16      // Device code length (e.g., DVC-7A3B9C2E1F)
#define CLOUD_MAX_PENDING_UPLOADS   8       // Max pending upload tasks
#define CLOUD_UPLOAD_LOG_MAX        16      // Max upload log entries
#define CLOUD_QR_DATA_MAX_LEN       128     // Max QR code data length

/* OneNet API Configuration */
#define ONENET_API_BASE_URL         "https://iot-api.heclouds.com"
#define ONENET_PRODUCT_ID_CLOUD     "uC1lme9g6r"  // Your product ID
#define ONENET_API_KEY              "version=2018-10-31&res=products%2FuC1lme9g6r&et=2000000000&method=md5&sign=xxx"

/* ==================== Enums ==================== */

/**
 * @brief Device activation state
 */
typedef enum {
    CLOUD_STATE_NOT_ACTIVATED = 0,  // Device not activated
    CLOUD_STATE_ACTIVATING,         // Activation in progress
    CLOUD_STATE_ACTIVATED,          // Device activated successfully
    CLOUD_STATE_ERROR               // Activation error
} cloud_activation_state_t;

/**
 * @brief Upload task status
 */
typedef enum {
    UPLOAD_STATUS_PENDING = 0,      // Waiting to upload
    UPLOAD_STATUS_UPLOADING,        // Currently uploading
    UPLOAD_STATUS_SUCCESS,          // Upload completed
    UPLOAD_STATUS_FAILED,           // Upload failed (will retry)
    UPLOAD_STATUS_CANCELLED         // Upload cancelled
} upload_status_t;

/**
 * @brief File type for upload
 */
typedef enum {
    UPLOAD_FILE_TYPE_CSV = 0,       // CSV data file
    UPLOAD_FILE_TYPE_LOG,           // Log file
    UPLOAD_FILE_TYPE_CONFIG,        // Configuration file
    UPLOAD_FILE_TYPE_OTHER          // Other file types
} upload_file_type_t;

/**
 * @brief Sync data type
 */
typedef enum {
    SYNC_TYPE_SENSOR = 0,           // Sensor data
    SYNC_TYPE_DEVICE_STATUS,        // Device status
    SYNC_TYPE_LOCATION,             // Location data
    SYNC_TYPE_OSCILLOSCOPE          // Oscilloscope data
} sync_data_type_t;

/* ==================== Structures ==================== */

/**
 * @brief Device information
 */
typedef struct {
    char device_code[CLOUD_DEVICE_CODE_LEN + 1];  // Unique device code (e.g., DVC-7A3B9C2E1F)
    char device_name[64];           // OneNet device name (e.g., ExDebugTool_1)
    char device_id[32];             // OneNet device ID (did) - 每个设备唯一
    char product_id[32];            // OneNet product ID (pid) - 产品标识符
    char sec_key[64];               // Device security key
    float longitude;                // Device longitude
    float latitude;                 // Device latitude
    char location_name[128];        // Location name
    time_t activation_time;         // Activation timestamp
    cloud_activation_state_t state; // Current activation state
} cloud_device_info_t;

/**
 * @brief Upload task information
 */
typedef struct {
    uint32_t task_id;               // Unique task ID
    char file_path[64];             // File path on SD card
    char file_name[32];             // File name
    uint32_t file_size;             // File size in bytes
    upload_file_type_t file_type;   // File type
    upload_status_t status;         // Current status
    uint8_t retry_count;            // Number of retries
    uint32_t progress;              // Upload progress (0-100)
    uint32_t uploaded_bytes;        // Bytes uploaded
    uint32_t data_points;           // Number of data points (for CSV)
    time_t create_time;             // Task creation time
    time_t complete_time;           // Task completion time
    char error_msg[32];             // Error message if failed
} upload_task_t;

/**
 * @brief Cloud sync status
 */
typedef struct {
    bool is_connected;              // OneNet connection status
    uint32_t total_uploaded;        // Total records uploaded
    uint32_t today_uploaded;        // Records uploaded today
    time_t last_sync_time[4];       // Last sync time for each type
    uint32_t pending_count;         // Pending upload count
    uint32_t failed_count;          // Failed upload count
} cloud_sync_status_t;

/**
 * @brief Upload log entry
 */
typedef struct {
    uint32_t log_id;                // Log ID
    char file_name[32];             // File name (truncated)
    uint32_t data_points;           // Data points uploaded
    time_t upload_time;             // Upload timestamp
    bool success;                   // Success flag
} upload_log_entry_t;

/**
 * @brief Auto upload settings
 */
typedef struct {
    bool enabled;                   // Auto upload enabled
    uint32_t interval_minutes;      // Upload interval in minutes
    bool upload_sensor_data;        // Upload sensor data
    bool upload_oscilloscope;       // Upload oscilloscope data
    bool upload_location;           // Upload location data
} auto_upload_settings_t;

/**
 * @brief Today's statistics
 */
typedef struct {
    uint32_t upload_count;          // Number of uploads today
    uint32_t data_points;           // Total data points today
    uint32_t bytes_uploaded;        // Total bytes uploaded today
    uint32_t failed_count;          // Failed uploads today
    time_t first_upload_time;       // First upload time today
    time_t last_upload_time;        // Last upload time today
} cloud_today_stats_t;

/* ==================== Callback Types ==================== */

typedef void (*cloud_activation_cb_t)(cloud_activation_state_t state, const char *device_code);
typedef void (*cloud_upload_progress_cb_t)(uint32_t task_id, uint32_t progress, uint32_t uploaded_bytes);
typedef void (*cloud_upload_complete_cb_t)(uint32_t task_id, bool success, const char *error_msg);

/* ==================== API Functions ==================== */

/**
 * @brief Initialize cloud manager module
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_init(void);

/**
 * @brief Deinitialize cloud manager module
 */
void cloud_manager_deinit(void);

/* === Device Activation === */

/**
 * @brief Check if device is activated
 * @return true if activated
 */
bool cloud_manager_is_activated(void);

/**
 * @brief Get device activation state
 * @return Current activation state
 */
cloud_activation_state_t cloud_manager_get_state(void);

/**
 * @brief Get device information
 * @param info Output device info
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_device_info(cloud_device_info_t *info);

/**
 * @brief Start device activation process
 * @param callback Activation status callback
 * @return ESP_OK on success
 * 
 * This will:
 * 1. Generate unique device code
 * 2. Wait for WiFi connection (or prompt user to configure)
 * 3. Get WiFi location
 * 4. Register device on OneNet
 * 5. Save activation state to NVS
 */
esp_err_t cloud_manager_start_activation(cloud_activation_cb_t callback);

/**
 * @brief Generate QR code data for activation
 * @param qr_data Output buffer for QR data
 * @param max_len Maximum buffer length
 * @return ESP_OK on success
 * 
 * The QR code contains a URL that the user scans with their phone.
 * If WiFi is not connected, the URL will open a WiFi configuration page.
 * If WiFi is connected, activation proceeds immediately.
 */
esp_err_t cloud_manager_generate_qr_data(char *qr_data, size_t max_len);

/**
 * @brief Get activation QR code URL
 * @return QR code URL string
 */
const char* cloud_manager_get_qr_url(void);

/* === Data Upload === */

/**
 * @brief Create a manual upload task
 * @param file_path File path on SD card
 * @param task_id Output task ID
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_upload_file(const char *file_path, uint32_t *task_id);

/**
 * @brief Cancel an upload task
 * @param task_id Task ID to cancel
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_cancel_upload(uint32_t task_id);

/**
 * @brief Get upload task status
 * @param task_id Task ID
 * @param task Output task info
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_upload_task(uint32_t task_id, upload_task_t *task);

/**
 * @brief Get all pending upload tasks
 * @param tasks Output task array
 * @param max_count Maximum tasks to return
 * @param count Output actual count
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_pending_uploads(upload_task_t *tasks, uint32_t max_count, uint32_t *count);

/**
 * @brief Retry failed uploads
 * @return Number of tasks retried
 */
uint32_t cloud_manager_retry_failed_uploads(void);

/**
 * @brief Set upload progress callback
 */
void cloud_manager_set_progress_callback(cloud_upload_progress_cb_t callback);

/**
 * @brief Set upload complete callback
 */
void cloud_manager_set_complete_callback(cloud_upload_complete_cb_t callback);

/* === Sync Status === */

/**
 * @brief Get cloud sync status
 * @param status Output sync status
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_sync_status(cloud_sync_status_t *status);

/**
 * @brief Get last sync time for a data type
 * @param type Sync data type
 * @return Last sync timestamp, 0 if never synced
 */
time_t cloud_manager_get_last_sync_time(sync_data_type_t type);

/**
 * @brief Format last sync time as human-readable string
 * @param type Sync data type
 * @param buffer Output buffer
 * @param buf_size Buffer size
 * @return Formatted string (e.g., "刚刚", "2分钟前", "今天 09:30")
 */
esp_err_t cloud_manager_format_last_sync(sync_data_type_t type, char *buffer, size_t buf_size);

/* === Upload Logs === */

/**
 * @brief Get upload log entries
 * @param logs Output log array
 * @param max_count Maximum entries to return
 * @param count Output actual count
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_upload_logs(upload_log_entry_t *logs, uint32_t max_count, uint32_t *count);

/**
 * @brief Clear upload logs
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_clear_logs(void);

/* === Auto Upload Settings === */

/**
 * @brief Get auto upload settings
 * @param settings Output settings
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_auto_settings(auto_upload_settings_t *settings);

/**
 * @brief Set auto upload settings
 * @param settings New settings
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_set_auto_settings(const auto_upload_settings_t *settings);

/**
 * @brief Enable/disable auto upload
 * @param enabled Enable flag
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_set_auto_upload_enabled(bool enabled);

/**
 * @brief Set auto upload interval
 * @param minutes Interval in minutes (1-1440)
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_set_auto_interval(uint32_t minutes);

/**
 * @brief Set auto upload interval in seconds
 * @param seconds Interval in seconds (max 86400, i.e., 1 day)
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_set_auto_interval_seconds(uint32_t seconds);

/**
 * @brief Set auto upload interval in milliseconds
 * @param ms Interval in milliseconds (no minimum limit, max 86400000 i.e., 1 day)
 * @return ESP_OK on success
 * @note Supports ultra-fast intervals like 1ms, 10ms, 100ms for high-frequency data
 */
esp_err_t cloud_manager_set_auto_interval_ms(uint32_t ms);

/* === Statistics === */

/**
 * @brief Get today's upload statistics
 * @param stats Output statistics
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_today_stats(cloud_today_stats_t *stats);

/**
 * @brief Get total uploaded record count
 * @return Total records uploaded
 */
uint32_t cloud_manager_get_total_uploaded(void);

/* === File Operations === */

/**
 * @brief Check if file has been uploaded
 * @param file_path File path
 * @return true if already uploaded
 */
bool cloud_manager_is_file_uploaded(const char *file_path);

/**
 * @brief Mark file as uploaded
 * @param file_path File path
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_mark_file_uploaded(const char *file_path);

/**
 * @brief Get list of uploaded files
 * @param files Output file list (array of paths)
 * @param max_count Maximum files to return
 * @param count Output actual count
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_uploaded_files(char files[][64], uint32_t max_count, uint32_t *count);

/* === Process (call in main loop) === */

/**
 * @brief Process pending operations (call periodically)
 * 
 * This function handles:
 * - Pending uploads
 * - Auto upload timer
 * - Failed upload retry
 * - Connection monitoring
 * 
 * @note This function may cause linker issues on some platforms.
 *       Use cloud_manager_execute_pending_upload() for manual uploads instead.
 */
void cloud_manager_process(void);

/**
 * @brief Execute a single pending upload task
 * @param task_id Task ID to execute (0 = execute first pending task)
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no pending task
 * 
 * @note This is a lightweight alternative to cloud_manager_process()
 *       that only handles the specified upload task.
 */
esp_err_t cloud_manager_execute_pending_upload(uint32_t task_id);

/* === Device Info === */

/**
 * @brief Get device code
 * @param code Output buffer
 * @param len Buffer length
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_get_device_code(char *code, size_t len);

/**
 * @brief Set device as activated
 * @param device_id Device ID (did) from OneNet - 设备唯一标识
 * @param product_id Product ID (pid) from OneNet - 产品标识符
 * @param device_name Device name
 * @param sec_key Security key
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_set_activated(const char *device_id, const char *product_id,
                                      const char *device_name, const char *sec_key);

/**
 * @brief 清除设备激活状态（用于重新激活）
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_clear_activation(void);

/**
 * @brief 获取设备凭证（用于HTTP API认证）
 * @param product_id 输出产品ID
 * @param pid_len 产品ID缓冲区长度
 * @param device_name 输出设备名称
 * @param name_len 设备名称缓冲区长度
 * @param sec_key 输出设备密钥
 * @param key_len 密钥缓冲区长度
 * @return ESP_OK表示成功，ESP_ERR_INVALID_STATE表示设备未激活
 */
esp_err_t cloud_manager_get_device_credentials(char *product_id, size_t pid_len,
                                                char *device_name, size_t name_len,
                                                char *sec_key, size_t key_len);

/* === Utility === */

/**
 * @brief Generate unique device code
 * @param code Output buffer (min 17 bytes)
 * @return ESP_OK on success
 */
esp_err_t cloud_manager_generate_device_code(char *code);

/**
 * @brief Parse CSV file and count data points
 * @param file_path File path
 * @return Number of data points, -1 on error
 */
int32_t cloud_manager_parse_csv_count(const char *file_path);

/**
 * @brief Format bytes as human-readable string
 * @param bytes Byte count
 * @param buffer Output buffer
 * @param buf_size Buffer size
 */
void cloud_manager_format_bytes(uint64_t bytes, char *buffer, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* CLOUD_MANAGER_H */

