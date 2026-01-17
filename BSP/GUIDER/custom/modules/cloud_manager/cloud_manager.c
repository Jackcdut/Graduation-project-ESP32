/**
 * @file cloud_manager.c
 * @brief OneNet Cloud Platform Device Manager Implementation
 */

#include "cloud_manager.h"
#include "cloud_activation_server.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "wifi_manager.h"
#include "wifi_onenet.h"
#include "mbedtls/md.h"
#include "mbedtls/base64.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

/* OneNet API Configuration */
#define ONENET_API_HOST         "iot-api.heclouds.com"
#define ONENET_API_PORT         443
#define ONENET_API_CREATE       "/device/create"
/* Use ONENET_HTTP_PRODUCT_ID and ONENET_ACCESS_KEY from wifi_onenet.h for HTTP API */

/* OneNET 用户级别 API 鉴权配置（用于文件上传等）
 * 从 OneNET 用户中心获取：https://open.iot.10086.cn/
 * 用户ID: 在用户中心 -> 个人信息 中查看
 * 用户Access Key: 在用户中心 -> 安全设置 -> Access Key 中获取（需要手机验证码）
 */
#define ONENET_USER_ID          "420568"
#define ONENET_USER_ACCESS_KEY  "xaVmoFXwf9oB4QpVN8Vt8sL4hqhLoIyRp31g2j0gQKEt0VG5XEFbpYGvQst14YPX"

static const char *TAG = "CLOUD_MGR";

/* NVS Keys */
#define NVS_NAMESPACE_CLOUD     "cloud_mgr"
#define NVS_KEY_ACTIVATED       "activated"
#define NVS_KEY_DEVICE_CODE     "dev_code"
#define NVS_KEY_DEVICE_NAME     "dev_name"
#define NVS_KEY_DEVICE_ID       "dev_id"
#define NVS_KEY_SEC_KEY         "sec_key"
#define NVS_KEY_ACT_TIME        "act_time"
#define NVS_KEY_LONGITUDE       "longitude"
#define NVS_KEY_LATITUDE        "latitude"
#define NVS_KEY_LOCATION        "location"
#define NVS_KEY_TOTAL_UPLOAD    "total_upload"
#define NVS_KEY_AUTO_ENABLED    "auto_enabled"
#define NVS_KEY_AUTO_INTERVAL   "auto_interval"
#define NVS_KEY_DEVICE_INDEX    "dev_index"
#define NVS_KEY_UPLOAD_COUNT    "upload_cnt"
#define NVS_KEY_BYTES_UPLOADED  "bytes_up"
#define NVS_KEY_DATA_POINTS     "data_pts"
#define NVS_KEY_FILE_HASHES     "file_hash"
#define NVS_KEY_FILE_HASH_CNT   "hash_cnt"

/* Internal state */
static bool s_initialized = false;
static cloud_device_info_t s_device_info = {0};
static cloud_sync_status_t s_sync_status = {0};
static auto_upload_settings_t s_auto_settings = {
    .enabled = false,
    .interval_minutes = 30,
    .upload_sensor_data = true,
    .upload_oscilloscope = true,
    .upload_location = true
};
static cloud_today_stats_t s_today_stats = {0};

/* Upload queue */
static upload_task_t s_upload_tasks[CLOUD_MAX_PENDING_UPLOADS] = {0};
static uint32_t s_next_task_id = 1;
static SemaphoreHandle_t s_upload_mutex = NULL;

/* Upload logs */
static upload_log_entry_t s_upload_logs[CLOUD_UPLOAD_LOG_MAX] = {0};
static uint32_t s_log_count = 0;
static uint32_t s_next_log_id = 1;

/* Callbacks */
static cloud_activation_cb_t s_activation_cb = NULL;
static cloud_upload_progress_cb_t s_progress_cb = NULL;
static cloud_upload_complete_cb_t s_complete_cb = NULL;

/* Auto upload timer */
static uint32_t s_last_auto_upload_time = 0;

/* Uploaded files tracking (simple hash set) */
#define MAX_UPLOADED_FILES 32
static uint32_t s_uploaded_file_hashes[MAX_UPLOADED_FILES] = {0};
static uint32_t s_uploaded_file_count = 0;

/* Forward declarations */
static esp_err_t load_state_from_nvs(void);
static esp_err_t save_state_to_nvs(void);
static esp_err_t register_device_on_onenet(void);
static esp_err_t upload_file_data_to_onenet(upload_task_t *task);
static uint32_t hash_string(const char *str);
static void add_upload_log(const char *file_name, uint32_t data_points, bool success);

/* ==================== Initialization ==================== */

esp_err_t cloud_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing Cloud Manager...");
    
    /* Create mutex */
    s_upload_mutex = xSemaphoreCreateMutex();
    if (s_upload_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    /* Load state from NVS */
    esp_err_t ret = load_state_from_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No saved state found, device not activated");
        s_device_info.state = CLOUD_STATE_NOT_ACTIVATED;
    }
    
    /* Device code should only be generated after successful activation */
    /* Do NOT generate device code here - it will be set from OneNet device_id after activation */
    /* Clear any pre-existing device code if device is not activated */
    if (s_device_info.state != CLOUD_STATE_ACTIVATED) {
        s_device_info.device_code[0] = '\0';
    }
    
    /* Initialize sync status */
    s_sync_status.is_connected = false;
    s_sync_status.pending_count = 0;
    s_sync_status.failed_count = 0;
    
    /* Reset today stats */
    memset(&s_today_stats, 0, sizeof(s_today_stats));
    
    s_initialized = true;
    ESP_LOGI(TAG, "Cloud Manager initialized, device %s", 
             s_device_info.state == CLOUD_STATE_ACTIVATED ? "ACTIVATED" : "NOT ACTIVATED");
    
    return ESP_OK;
}

void cloud_manager_deinit(void)
{
    if (!s_initialized) return;
    
    /* Save state */
    save_state_to_nvs();
    
    /* Delete mutex */
    if (s_upload_mutex != NULL) {
        vSemaphoreDelete(s_upload_mutex);
        s_upload_mutex = NULL;
    }
    
    s_initialized = false;
    ESP_LOGI(TAG, "Cloud Manager deinitialized");
}

bool cloud_manager_is_initialized(void)
{
    return s_initialized;
}

/* ==================== Device Activation ==================== */

bool cloud_manager_is_activated(void)
{
    return s_device_info.state == CLOUD_STATE_ACTIVATED;
}

cloud_activation_state_t cloud_manager_get_state(void)
{
    return s_device_info.state;
}

esp_err_t cloud_manager_get_device_info(cloud_device_info_t *info)
{
    if (info == NULL) return ESP_ERR_INVALID_ARG;
    memcpy(info, &s_device_info, sizeof(cloud_device_info_t));
    return ESP_OK;
}

esp_err_t cloud_manager_get_device_code(char *code, size_t len)
{
    if (code == NULL || len == 0) return ESP_ERR_INVALID_ARG;
    
    /* Generate if not exists */
    if (strlen(s_device_info.device_code) == 0) {
        cloud_manager_generate_device_code(s_device_info.device_code);
    }
    
    strncpy(code, s_device_info.device_code, len - 1);
    code[len - 1] = '\0';
    return ESP_OK;
}

esp_err_t cloud_manager_set_activated(const char *device_id, const char *product_id, 
                                      const char *device_name, const char *sec_key)
{
    if (device_id == NULL) return ESP_ERR_INVALID_ARG;
    
    /* 保存设备ID (did) */
    strncpy(s_device_info.device_id, device_id, sizeof(s_device_info.device_id) - 1);
    s_device_info.device_id[sizeof(s_device_info.device_id) - 1] = '\0';
    
    /* 保存产品ID (pid) */
    if (product_id && strlen(product_id) > 0) {
        strncpy(s_device_info.product_id, product_id, sizeof(s_device_info.product_id) - 1);
    } else {
        /* 如果没有提供产品ID，使用配置的默认值 */
        strncpy(s_device_info.product_id, ONENET_HTTP_PRODUCT_ID, sizeof(s_device_info.product_id) - 1);
    }
    s_device_info.product_id[sizeof(s_device_info.product_id) - 1] = '\0';
    
    if (device_name) {
        strncpy(s_device_info.device_name, device_name, sizeof(s_device_info.device_name) - 1);
        s_device_info.device_name[sizeof(s_device_info.device_name) - 1] = '\0';
    }
    
    if (sec_key) {
        strncpy(s_device_info.sec_key, sec_key, sizeof(s_device_info.sec_key) - 1);
        s_device_info.sec_key[sizeof(s_device_info.sec_key) - 1] = '\0';
    }
    
    /* 设备码使用设备名称，更易识别 */
    if (device_name && strlen(device_name) > 0) {
        snprintf(s_device_info.device_code, sizeof(s_device_info.device_code), "%s", device_name);
    } else {
        snprintf(s_device_info.device_code, sizeof(s_device_info.device_code), "%s", device_id);
    }
    
    s_device_info.state = CLOUD_STATE_ACTIVATED;
    s_device_info.activation_time = time(NULL);
    
    /* Save to NVS */
    save_state_to_nvs();
    
    ESP_LOGI(TAG, "Device activated: DeviceID=%s, ProductID=%s, Name=%s", 
             s_device_info.device_id, s_device_info.product_id, 
             device_name ? device_name : "N/A");
    return ESP_OK;
}

esp_err_t cloud_manager_clear_activation(void)
{
    ESP_LOGI(TAG, "Clearing device activation state...");
    
    /* 清除内存中的设备信息 */
    memset(&s_device_info, 0, sizeof(s_device_info));
    s_device_info.state = CLOUD_STATE_NOT_ACTIVATED;
    
    /* 清除 NVS 中的激活状态 */
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE_CLOUD, NVS_READWRITE, &handle);
    if (ret == ESP_OK) {
        nvs_set_u8(handle, NVS_KEY_ACTIVATED, 0);
        nvs_erase_key(handle, NVS_KEY_DEVICE_CODE);
        nvs_erase_key(handle, NVS_KEY_DEVICE_NAME);
        nvs_erase_key(handle, NVS_KEY_DEVICE_ID);
        nvs_erase_key(handle, NVS_KEY_SEC_KEY);
        nvs_commit(handle);
        nvs_close(handle);
    }
    
    ESP_LOGI(TAG, "Device activation cleared. Please re-activate the device.");
    return ESP_OK;
}

esp_err_t cloud_manager_generate_device_code(char *code)
{
    if (code == NULL) return ESP_ERR_INVALID_ARG;
    
    /* Generate unique code based on MAC address and random bytes */
    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (ret != ESP_OK) {
        /* Fallback to random */
        for (int i = 0; i < 6; i++) {
            mac[i] = esp_random() & 0xFF;
        }
    }
    
    uint32_t rand_part = esp_random();
    
    /* Format: DVC-XXXXXXXX (where X is hex) */
    snprintf(code, CLOUD_DEVICE_CODE_LEN + 1, "DVC-%02X%02X%02X%02X",
             mac[4] ^ ((rand_part >> 24) & 0xFF),
             mac[5] ^ ((rand_part >> 16) & 0xFF),
             (rand_part >> 8) & 0xFF,
             rand_part & 0xFF);
    
    ESP_LOGI(TAG, "Generated device code: %s", code);
    return ESP_OK;
}

esp_err_t cloud_manager_start_activation(cloud_activation_cb_t callback)
{
    if (s_device_info.state == CLOUD_STATE_ACTIVATED) {
        ESP_LOGW(TAG, "Device already activated");
        if (callback) callback(CLOUD_STATE_ACTIVATED, s_device_info.device_code);
        return ESP_OK;
    }
    
    s_activation_cb = callback;
    s_device_info.state = CLOUD_STATE_ACTIVATING;
    
    if (callback) callback(CLOUD_STATE_ACTIVATING, NULL);
    
    /* Generate device code if not exists */
    if (strlen(s_device_info.device_code) == 0) {
        cloud_manager_generate_device_code(s_device_info.device_code);
    }
    
    /* Check WiFi connection */
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, waiting for connection...");
        /* UI should show QR code for WiFi configuration */
        return ESP_OK;
    }
    
    /* Try to get location */
    location_info_t location;
    if (wifi_location_get_last_result(&location) == ESP_OK && location.valid) {
        s_device_info.longitude = location.longitude;
        s_device_info.latitude = location.latitude;
        snprintf(s_device_info.location_name, sizeof(s_device_info.location_name), "%s", location.address);
    }
    
    /* Register device on OneNet */
    esp_err_t ret = register_device_on_onenet();
    if (ret == ESP_OK) {
        s_device_info.state = CLOUD_STATE_ACTIVATED;
        s_device_info.activation_time = time(NULL);
        save_state_to_nvs();
        
        ESP_LOGI(TAG, "Device activated successfully!");
        if (callback) callback(CLOUD_STATE_ACTIVATED, s_device_info.device_code);
    } else {
        s_device_info.state = CLOUD_STATE_ERROR;
        ESP_LOGE(TAG, "Device activation failed");
        if (callback) callback(CLOUD_STATE_ERROR, NULL);
    }
    
    return ret;
}

esp_err_t cloud_manager_generate_qr_data(char *qr_data, size_t max_len)
{
    if (qr_data == NULL || max_len < 64) return ESP_ERR_INVALID_ARG;
    
    /* Generate device code if not exists */
    if (strlen(s_device_info.device_code) == 0) {
        cloud_manager_generate_device_code(s_device_info.device_code);
    }
    
    /* QR code contains activation URL with device code */
    /* The mobile app will use this to configure WiFi and activate device */
    snprintf(qr_data, max_len, 
             "https://iot.example.com/activate?code=%s&product=%s",
             s_device_info.device_code, ONENET_PRODUCT_ID_CLOUD);
    
    return ESP_OK;
}

const char* cloud_manager_get_qr_url(void)
{
    static char qr_url[CLOUD_QR_DATA_MAX_LEN];
    cloud_manager_generate_qr_data(qr_url, sizeof(qr_url));
    return qr_url;
}

/* ==================== Data Upload ==================== */

esp_err_t cloud_manager_upload_file(const char *file_path, uint32_t *task_id)
{
    if (file_path == NULL || task_id == NULL) return ESP_ERR_INVALID_ARG;
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    
    /* Check if file exists */
    struct stat st;
    if (stat(file_path, &st) != 0) {
        ESP_LOGE(TAG, "File not found: %s", file_path);
        return ESP_ERR_NOT_FOUND;
    }
    
    /* Check if already uploaded */
    if (cloud_manager_is_file_uploaded(file_path)) {
        ESP_LOGW(TAG, "File already uploaded: %s", file_path);
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreTake(s_upload_mutex, portMAX_DELAY);
    
    /* Find empty slot */
    int slot = -1;
    for (int i = 0; i < CLOUD_MAX_PENDING_UPLOADS; i++) {
        if (s_upload_tasks[i].status == UPLOAD_STATUS_SUCCESS ||
            s_upload_tasks[i].status == UPLOAD_STATUS_CANCELLED ||
            s_upload_tasks[i].task_id == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        xSemaphoreGive(s_upload_mutex);
        ESP_LOGE(TAG, "Upload queue full");
        return ESP_ERR_NO_MEM;
    }
    
    /* Create task */
    upload_task_t *task = &s_upload_tasks[slot];
    memset(task, 0, sizeof(upload_task_t));
    
    task->task_id = s_next_task_id++;
    snprintf(task->file_path, sizeof(task->file_path), "%s", file_path);
    
    /* Extract filename */
    const char *fname = strrchr(file_path, '/');
    if (fname) fname++; else fname = file_path;
    snprintf(task->file_name, sizeof(task->file_name), "%s", fname);
    
    task->file_size = st.st_size;
    task->status = UPLOAD_STATUS_PENDING;
    task->create_time = time(NULL);
    
    /* Determine file type - OneNET支持: .jpg .jpeg .png .bmp .gif .webp .tiff .txt */
    const char *ext = strrchr(fname, '.');
    if (ext != NULL) {
        if (strcasecmp(ext, ".csv") == 0 || strcasecmp(ext, ".txt") == 0) {
            task->file_type = UPLOAD_FILE_TYPE_CSV;
            if (strcasecmp(ext, ".csv") == 0) {
                task->data_points = cloud_manager_parse_csv_count(file_path);
            }
        } else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 ||
                   strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0 ||
                   strcasecmp(ext, ".gif") == 0 || strcasecmp(ext, ".webp") == 0 ||
                   strcasecmp(ext, ".tiff") == 0) {
            task->file_type = UPLOAD_FILE_TYPE_OTHER;  /* 图片类型 */
        } else if (strcasecmp(ext, ".log") == 0) {
            task->file_type = UPLOAD_FILE_TYPE_LOG;
        } else if (strcasecmp(ext, ".json") == 0 || strcasecmp(ext, ".cfg") == 0) {
            task->file_type = UPLOAD_FILE_TYPE_CONFIG;
        } else {
            /* 不支持的文件格式 */
            ESP_LOGW(TAG, "Unsupported file format: %s", ext);
            task->file_type = UPLOAD_FILE_TYPE_OTHER;
        }
    } else {
        task->file_type = UPLOAD_FILE_TYPE_OTHER;
    }
    
    *task_id = task->task_id;
    s_sync_status.pending_count++;
    
    xSemaphoreGive(s_upload_mutex);
    
    ESP_LOGI(TAG, "Upload task created: %lu, file: %s, size: %lu bytes",
             (unsigned long)task->task_id, task->file_name, (unsigned long)task->file_size);
    
    return ESP_OK;
}

esp_err_t cloud_manager_cancel_upload(uint32_t task_id)
{
    xSemaphoreTake(s_upload_mutex, portMAX_DELAY);
    
    for (int i = 0; i < CLOUD_MAX_PENDING_UPLOADS; i++) {
        if (s_upload_tasks[i].task_id == task_id) {
            if (s_upload_tasks[i].status == UPLOAD_STATUS_PENDING ||
                s_upload_tasks[i].status == UPLOAD_STATUS_UPLOADING) {
                s_upload_tasks[i].status = UPLOAD_STATUS_CANCELLED;
                if (s_sync_status.pending_count > 0) s_sync_status.pending_count--;
                xSemaphoreGive(s_upload_mutex);
                ESP_LOGI(TAG, "Upload task %lu cancelled", (unsigned long)task_id);
                return ESP_OK;
            }
        }
    }
    
    xSemaphoreGive(s_upload_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t cloud_manager_get_upload_task(uint32_t task_id, upload_task_t *task)
{
    if (task == NULL) return ESP_ERR_INVALID_ARG;
    
    xSemaphoreTake(s_upload_mutex, portMAX_DELAY);
    
    for (int i = 0; i < CLOUD_MAX_PENDING_UPLOADS; i++) {
        if (s_upload_tasks[i].task_id == task_id) {
            memcpy(task, &s_upload_tasks[i], sizeof(upload_task_t));
            xSemaphoreGive(s_upload_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(s_upload_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t cloud_manager_get_pending_uploads(upload_task_t *tasks, uint32_t max_count, uint32_t *count)
{
    if (tasks == NULL || count == NULL) return ESP_ERR_INVALID_ARG;
    
    *count = 0;
    xSemaphoreTake(s_upload_mutex, portMAX_DELAY);
    
    for (int i = 0; i < CLOUD_MAX_PENDING_UPLOADS && *count < max_count; i++) {
        if (s_upload_tasks[i].status == UPLOAD_STATUS_PENDING ||
            s_upload_tasks[i].status == UPLOAD_STATUS_UPLOADING ||
            s_upload_tasks[i].status == UPLOAD_STATUS_FAILED) {
            memcpy(&tasks[*count], &s_upload_tasks[i], sizeof(upload_task_t));
            (*count)++;
        }
    }
    
    xSemaphoreGive(s_upload_mutex);
    return ESP_OK;
}

uint32_t cloud_manager_retry_failed_uploads(void)
{
    uint32_t retried = 0;
    
    xSemaphoreTake(s_upload_mutex, portMAX_DELAY);
    
    for (int i = 0; i < CLOUD_MAX_PENDING_UPLOADS; i++) {
        if (s_upload_tasks[i].status == UPLOAD_STATUS_FAILED &&
            s_upload_tasks[i].retry_count < 3) {
            s_upload_tasks[i].status = UPLOAD_STATUS_PENDING;
            s_upload_tasks[i].retry_count++;
            s_sync_status.pending_count++;
            if (s_sync_status.failed_count > 0) s_sync_status.failed_count--;
            retried++;
        }
    }
    
    xSemaphoreGive(s_upload_mutex);
    
    if (retried > 0) {
        ESP_LOGI(TAG, "Retrying %lu failed uploads", (unsigned long)retried);
    }
    
    return retried;
}

void cloud_manager_set_progress_callback(cloud_upload_progress_cb_t callback)
{
    s_progress_cb = callback;
}

void cloud_manager_set_complete_callback(cloud_upload_complete_cb_t callback)
{
    s_complete_cb = callback;
}

/* ==================== Sync Status ==================== */

esp_err_t cloud_manager_get_sync_status(cloud_sync_status_t *status)
{
    if (status == NULL) return ESP_ERR_INVALID_ARG;
    
    /* Update connection status - 使用HTTP状态判断 */
    s_sync_status.is_connected = (onenet_http_get_state() == ONENET_STATE_ONLINE);
    s_sync_status.total_uploaded = s_today_stats.upload_count;
    s_sync_status.today_uploaded = s_today_stats.upload_count;
    
    memcpy(status, &s_sync_status, sizeof(cloud_sync_status_t));
    return ESP_OK;
}

time_t cloud_manager_get_last_sync_time(sync_data_type_t type)
{
    if (type >= 4) return 0;
    return s_sync_status.last_sync_time[type];
}

esp_err_t cloud_manager_format_last_sync(sync_data_type_t type, char *buffer, size_t buf_size)
{
    if (buffer == NULL || buf_size < 16) return ESP_ERR_INVALID_ARG;
    
    time_t last_sync = cloud_manager_get_last_sync_time(type);
    if (last_sync == 0) {
        snprintf(buffer, buf_size, "Never");
        return ESP_OK;
    }
    
    time_t now = time(NULL);
    time_t diff = now - last_sync;
    
    if (diff < 60) {
        snprintf(buffer, buf_size, "Just now");
    } else if (diff < 3600) {
        snprintf(buffer, buf_size, "%ldm ago", (long)(diff / 60));
    } else if (diff < 86400) {
        snprintf(buffer, buf_size, "%ldh ago", (long)(diff / 3600));
    } else {
        struct tm timeinfo;
        localtime_r(&last_sync, &timeinfo);
        snprintf(buffer, buf_size, "%02d-%02d %02d:%02d",
                 timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min);
    }
    
    return ESP_OK;
}

/* ==================== Upload Logs ==================== */

esp_err_t cloud_manager_get_upload_logs(upload_log_entry_t *logs, uint32_t max_count, uint32_t *count)
{
    if (logs == NULL || count == NULL) return ESP_ERR_INVALID_ARG;
    
    *count = (s_log_count < max_count) ? s_log_count : max_count;
    memcpy(logs, s_upload_logs, (*count) * sizeof(upload_log_entry_t));
    
    return ESP_OK;
}

esp_err_t cloud_manager_clear_logs(void)
{
    memset(s_upload_logs, 0, sizeof(s_upload_logs));
    s_log_count = 0;
    return ESP_OK;
}

/* ==================== Auto Upload Settings ==================== */

esp_err_t cloud_manager_get_auto_settings(auto_upload_settings_t *settings)
{
    if (settings == NULL) return ESP_ERR_INVALID_ARG;
    memcpy(settings, &s_auto_settings, sizeof(auto_upload_settings_t));
    return ESP_OK;
}

esp_err_t cloud_manager_set_auto_settings(const auto_upload_settings_t *settings)
{
    if (settings == NULL) return ESP_ERR_INVALID_ARG;
    memcpy(&s_auto_settings, settings, sizeof(auto_upload_settings_t));
    save_state_to_nvs();
    return ESP_OK;
}

esp_err_t cloud_manager_set_auto_upload_enabled(bool enabled)
{
    s_auto_settings.enabled = enabled;
    save_state_to_nvs();
    ESP_LOGI(TAG, "Auto upload %s", enabled ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t cloud_manager_set_auto_interval(uint32_t minutes)
{
    /* 
     * 间隔范围: 最小1分钟（实际最小1秒由秒级设置函数处理），最大1440分钟（1天）
     * 注意: 如果需要秒级间隔，使用 cloud_manager_set_auto_interval_seconds()
     */
    if (minutes < 1) minutes = 1;
    if (minutes > 1440) minutes = 1440;
    s_auto_settings.interval_minutes = minutes;
    save_state_to_nvs();
    ESP_LOGI(TAG, "Auto upload interval set to %lu minutes", (unsigned long)minutes);
    return ESP_OK;
}

esp_err_t cloud_manager_set_auto_interval_seconds(uint32_t seconds)
{
    /* 转换为分钟（向上取整） */
    uint32_t minutes = (seconds + 59) / 60;
    return cloud_manager_set_auto_interval(minutes);
}

esp_err_t cloud_manager_set_auto_interval_ms(uint32_t ms)
{
    /* 转换为分钟（向上取整） */
    uint32_t minutes = (ms + 59999) / 60000;
    return cloud_manager_set_auto_interval(minutes);
}

/* ==================== Statistics ==================== */

esp_err_t cloud_manager_get_today_stats(cloud_today_stats_t *stats)
{
    if (stats == NULL) return ESP_ERR_INVALID_ARG;
    memcpy(stats, &s_today_stats, sizeof(cloud_today_stats_t));
    return ESP_OK;
}

uint32_t cloud_manager_get_total_uploaded(void)
{
    return s_sync_status.total_uploaded;
}

/* ==================== File Operations ==================== */

bool cloud_manager_is_file_uploaded(const char *file_path)
{
    if (file_path == NULL) return false;
    
    uint32_t hash = hash_string(file_path);
    for (uint32_t i = 0; i < s_uploaded_file_count; i++) {
        if (s_uploaded_file_hashes[i] == hash) {
            return true;
        }
    }
    return false;
}

esp_err_t cloud_manager_mark_file_uploaded(const char *file_path)
{
    if (file_path == NULL) return ESP_ERR_INVALID_ARG;
    
    if (s_uploaded_file_count >= MAX_UPLOADED_FILES) {
        /* Remove oldest entry */
        memmove(s_uploaded_file_hashes, s_uploaded_file_hashes + 1,
                (MAX_UPLOADED_FILES - 1) * sizeof(uint32_t));
        s_uploaded_file_count = MAX_UPLOADED_FILES - 1;
    }
    
    s_uploaded_file_hashes[s_uploaded_file_count++] = hash_string(file_path);
    
    /* Save to NVS immediately */
    save_state_to_nvs();
    
    return ESP_OK;
}

/* ==================== Process ==================== */

void cloud_manager_process(void)
{
    if (!s_initialized) return;
    
    /* Update connection status - 使用HTTP状态判断 */
    s_sync_status.is_connected = (onenet_http_get_state() == ONENET_STATE_ONLINE);
    
    /* Skip if not connected or not activated */
    if (!s_sync_status.is_connected && !cloud_manager_is_activated()) return;
    
    /* Process pending uploads */
    xSemaphoreTake(s_upload_mutex, portMAX_DELAY);
    
    for (int i = 0; i < CLOUD_MAX_PENDING_UPLOADS; i++) {
        upload_task_t *task = &s_upload_tasks[i];
        
        if (task->status == UPLOAD_STATUS_PENDING) {
            /* Start upload */
            task->status = UPLOAD_STATUS_UPLOADING;
            ESP_LOGI(TAG, "Uploading file: %s", task->file_name);
            
            /* Upload file data to OneNet */
            esp_err_t upload_ret = upload_file_data_to_onenet(task);
            
            if (upload_ret == ESP_OK) {
                /* Mark as success */
                task->status = UPLOAD_STATUS_SUCCESS;
                task->complete_time = time(NULL);
                task->progress = 100;
                task->uploaded_bytes = task->file_size;
                
                /* Update stats */
                s_today_stats.upload_count++;
                s_today_stats.data_points += task->data_points;
                s_today_stats.bytes_uploaded += task->file_size;
                s_today_stats.last_upload_time = task->complete_time;
                if (s_today_stats.first_upload_time == 0) {
                    s_today_stats.first_upload_time = task->complete_time;
                }
                
                s_sync_status.total_uploaded++;
                if (s_sync_status.pending_count > 0) s_sync_status.pending_count--;
                
                /* Mark file as uploaded */
                cloud_manager_mark_file_uploaded(task->file_path);
                
                /* Add log */
                add_upload_log(task->file_name, task->data_points, true);
                
                /* Callback */
                if (s_complete_cb) {
                    s_complete_cb(task->task_id, true, NULL);
                }
                
                ESP_LOGI(TAG, "Upload completed: %s", task->file_name);
            } else {
                /* Upload failed */
                task->status = UPLOAD_STATUS_FAILED;
                task->retry_count++;
                s_sync_status.failed_count++;
                
                add_upload_log(task->file_name, 0, false);
                
                if (s_complete_cb) {
                    s_complete_cb(task->task_id, false, "Upload failed");
                }
                
                ESP_LOGE(TAG, "Upload failed: %s", task->file_name);
            }
        }
    }
    
    xSemaphoreGive(s_upload_mutex);
    
    /* Check auto upload timer - 使用分钟级间隔进行批量上报 */
    if (s_auto_settings.enabled && cloud_manager_is_activated() && wifi_is_connected()) {
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        /* 使用分钟级间隔 */
        uint32_t interval_ms = s_auto_settings.interval_minutes * 60 * 1000;
        
        /* 限制范围: 最小1分钟，最大1小时 */
        if (interval_ms < 60000) interval_ms = 60000;
        if (interval_ms > 3600000) interval_ms = 3600000;
        
        if (now_ms - s_last_auto_upload_time >= interval_ms) {
            s_last_auto_upload_time = now_ms;
            ESP_LOGI(TAG, "Auto upload triggered (interval: %lu min)", interval_ms / 60000);
            
            /* 批量上报示波器数据 */
            if (s_auto_settings.upload_oscilloscope) {
                uint32_t buffered_count = onenet_get_oscilloscope_buffer_count();
                if (buffered_count > 0) {
                    ESP_LOGI(TAG, "Uploading %lu buffered oscilloscope data points...", buffered_count);
                    esp_err_t ret = onenet_report_oscilloscope_batch();
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "Oscilloscope batch upload success!");
                        s_sync_status.last_sync_time[SYNC_TYPE_OSCILLOSCOPE] = time(NULL);
                        s_today_stats.upload_count++;
                        s_today_stats.data_points += buffered_count;
                    } else {
                        ESP_LOGW(TAG, "Failed to upload oscilloscope batch data");
                    }
                } else {
                    ESP_LOGI(TAG, "No oscilloscope data buffered, skipping upload");
                }
            }
            
            /* 如果启用了定位上报，调整定位间隔与自动上报同步 */
            if (s_auto_settings.upload_location && wifi_location_is_reporting()) {
                wifi_location_set_report_interval(interval_ms);
                ESP_LOGI(TAG, "Location report interval synced to %lu min", interval_ms / 60000);
            }
        }
    }
}

esp_err_t cloud_manager_execute_pending_upload(uint32_t task_id)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    
    /* Update connection status */
    s_sync_status.is_connected = (onenet_http_get_state() == ONENET_STATE_ONLINE);
    
    xSemaphoreTake(s_upload_mutex, portMAX_DELAY);
    
    /* Find the task to execute */
    upload_task_t *task = NULL;
    for (int i = 0; i < CLOUD_MAX_PENDING_UPLOADS; i++) {
        if (task_id == 0) {
            /* Find first pending task */
            if (s_upload_tasks[i].status == UPLOAD_STATUS_PENDING) {
                task = &s_upload_tasks[i];
                break;
            }
        } else {
            /* Find specific task */
            if (s_upload_tasks[i].task_id == task_id && 
                s_upload_tasks[i].status == UPLOAD_STATUS_PENDING) {
                task = &s_upload_tasks[i];
                break;
            }
        }
    }
    
    if (task == NULL) {
        xSemaphoreGive(s_upload_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    /* Start upload */
    task->status = UPLOAD_STATUS_UPLOADING;
    ESP_LOGI(TAG, "Executing upload: %s (task_id=%lu)", task->file_name, (unsigned long)task->task_id);
    
    /* Upload file data to OneNet */
    esp_err_t upload_ret = upload_file_data_to_onenet(task);
    
    if (upload_ret == ESP_OK) {
        /* Mark as success */
        task->status = UPLOAD_STATUS_SUCCESS;
        task->complete_time = time(NULL);
        task->progress = 100;
        task->uploaded_bytes = task->file_size;
        
        /* Update stats */
        s_today_stats.upload_count++;
        s_today_stats.data_points += task->data_points;
        s_today_stats.bytes_uploaded += task->file_size;
        s_today_stats.last_upload_time = task->complete_time;
        if (s_today_stats.first_upload_time == 0) {
            s_today_stats.first_upload_time = task->complete_time;
        }
        
        s_sync_status.total_uploaded++;
        if (s_sync_status.pending_count > 0) s_sync_status.pending_count--;
        
        /* Mark file as uploaded (this also saves stats to NVS) */
        cloud_manager_mark_file_uploaded(task->file_path);
        
        /* Add log */
        add_upload_log(task->file_name, task->data_points, true);
        
        /* Save stats to NVS */
        save_state_to_nvs();
        
        /* Callback */
        if (s_complete_cb) {
            s_complete_cb(task->task_id, true, NULL);
        }
        
        ESP_LOGI(TAG, "Upload completed: %s (total: %lu uploads, %lu bytes)", 
                 task->file_name, (unsigned long)s_today_stats.upload_count, 
                 (unsigned long)s_today_stats.bytes_uploaded);
    } else {
        /* Upload failed */
        task->status = UPLOAD_STATUS_FAILED;
        task->retry_count++;
        s_sync_status.failed_count++;
        
        add_upload_log(task->file_name, 0, false);
        
        if (s_complete_cb) {
            s_complete_cb(task->task_id, false, task->error_msg);
        }
        
        ESP_LOGE(TAG, "Upload failed: %s", task->file_name);
    }
    
    xSemaphoreGive(s_upload_mutex);
    return upload_ret;
}

/* ==================== Utility ==================== */

int32_t cloud_manager_parse_csv_count(const char *file_path)
{
    if (file_path == NULL) return -1;
    
    FILE *f = fopen(file_path, "r");
    if (f == NULL) return -1;
    
    int32_t count = 0;
    char line[256];
    bool first_line = true;
    
    while (fgets(line, sizeof(line), f) != NULL) {
        if (first_line) {
            first_line = false; /* Skip header */
            continue;
        }
        if (strlen(line) > 1) count++;
    }
    
    fclose(f);
    return count;
}

void cloud_manager_format_bytes(uint64_t bytes, char *buffer, size_t buf_size)
{
    if (buffer == NULL || buf_size < 8) return;
    
    if (bytes < 1024) {
        snprintf(buffer, buf_size, "%lluB", (unsigned long long)bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, buf_size, "%.1fKB", (double)bytes / 1024);
    } else if (bytes < 1024 * 1024 * 1024) {
        snprintf(buffer, buf_size, "%.1fMB", (double)bytes / (1024 * 1024));
    } else {
        snprintf(buffer, buf_size, "%.2fGB", (double)bytes / (1024 * 1024 * 1024));
    }
}

/* ==================== Internal Functions ==================== */

static uint32_t hash_string(const char *str)
{
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static void add_upload_log(const char *file_name, uint32_t data_points, bool success)
{
    if (s_log_count >= CLOUD_UPLOAD_LOG_MAX) {
        /* Shift logs */
        memmove(s_upload_logs, s_upload_logs + 1,
                (CLOUD_UPLOAD_LOG_MAX - 1) * sizeof(upload_log_entry_t));
        s_log_count = CLOUD_UPLOAD_LOG_MAX - 1;
    }
    
    upload_log_entry_t *log = &s_upload_logs[s_log_count++];
    log->log_id = s_next_log_id++;
    snprintf(log->file_name, sizeof(log->file_name), "%s", file_name);
    log->data_points = data_points;
    log->upload_time = time(NULL);
    log->success = success;
}

static esp_err_t load_state_from_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE_CLOUD, NVS_READONLY, &handle);
    if (ret != ESP_OK) return ret;
    
    uint8_t activated = 0;
    ret = nvs_get_u8(handle, NVS_KEY_ACTIVATED, &activated);
    if (ret == ESP_OK && activated) {
        s_device_info.state = CLOUD_STATE_ACTIVATED;
        
        size_t len = sizeof(s_device_info.device_code);
        nvs_get_str(handle, NVS_KEY_DEVICE_CODE, s_device_info.device_code, &len);
        
        len = sizeof(s_device_info.device_name);
        nvs_get_str(handle, NVS_KEY_DEVICE_NAME, s_device_info.device_name, &len);
        
        len = sizeof(s_device_info.device_id);
        nvs_get_str(handle, NVS_KEY_DEVICE_ID, s_device_info.device_id, &len);
        
        len = sizeof(s_device_info.sec_key);
        esp_err_t sec_ret = nvs_get_str(handle, NVS_KEY_SEC_KEY, s_device_info.sec_key, &len);
        if (sec_ret != ESP_OK || strlen(s_device_info.sec_key) == 0) {
            ESP_LOGW(TAG, "WARNING: sec_key not found in NVS or empty (ret=%d, len=%d)", sec_ret, (int)len);
            ESP_LOGW(TAG, "Device needs to be re-activated to get a valid sec_key!");
        } else {
            /* 打印 sec_key 的前8个字符用于调试 */
            char debug_key[12] = {0};
            snprintf(debug_key, sizeof(debug_key), "%.8s", s_device_info.sec_key);
            ESP_LOGI(TAG, "sec_key loaded: %s... (len=%d)", debug_key, (int)strlen(s_device_info.sec_key));
        }
        
        int64_t act_time = 0;
        nvs_get_i64(handle, NVS_KEY_ACT_TIME, &act_time);
        s_device_info.activation_time = (time_t)act_time;
        
        /* Load auto settings */
        uint8_t auto_en = 0;
        nvs_get_u8(handle, NVS_KEY_AUTO_ENABLED, &auto_en);
        s_auto_settings.enabled = auto_en;
        
        nvs_get_u32(handle, NVS_KEY_AUTO_INTERVAL, &s_auto_settings.interval_minutes);
        if (s_auto_settings.interval_minutes == 0) s_auto_settings.interval_minutes = 30;
        
        nvs_get_u32(handle, NVS_KEY_TOTAL_UPLOAD, &s_sync_status.total_uploaded);
        
        /* Load upload statistics */
        nvs_get_u32(handle, NVS_KEY_UPLOAD_COUNT, &s_today_stats.upload_count);
        uint32_t bytes_low = 0;
        nvs_get_u32(handle, NVS_KEY_BYTES_UPLOADED, &bytes_low);
        s_today_stats.bytes_uploaded = bytes_low;
        nvs_get_u32(handle, NVS_KEY_DATA_POINTS, &s_today_stats.data_points);
        
        /* Load uploaded file hashes */
        nvs_get_u32(handle, NVS_KEY_FILE_HASH_CNT, &s_uploaded_file_count);
        if (s_uploaded_file_count > MAX_UPLOADED_FILES) {
            s_uploaded_file_count = MAX_UPLOADED_FILES;
        }
        if (s_uploaded_file_count > 0) {
            size_t hash_len = s_uploaded_file_count * sizeof(uint32_t);
            nvs_get_blob(handle, NVS_KEY_FILE_HASHES, s_uploaded_file_hashes, &hash_len);
            ESP_LOGI(TAG, "Loaded %lu uploaded file hashes from NVS", (unsigned long)s_uploaded_file_count);
        }
    }
    
    nvs_close(handle);
    return ESP_OK;
}

static esp_err_t save_state_to_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE_CLOUD, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;
    
    uint8_t activated = (s_device_info.state == CLOUD_STATE_ACTIVATED) ? 1 : 0;
    nvs_set_u8(handle, NVS_KEY_ACTIVATED, activated);
    
    if (activated) {
        nvs_set_str(handle, NVS_KEY_DEVICE_CODE, s_device_info.device_code);
        nvs_set_str(handle, NVS_KEY_DEVICE_NAME, s_device_info.device_name);
        nvs_set_str(handle, NVS_KEY_DEVICE_ID, s_device_info.device_id);
        nvs_set_str(handle, NVS_KEY_SEC_KEY, s_device_info.sec_key);
        nvs_set_i64(handle, NVS_KEY_ACT_TIME, (int64_t)s_device_info.activation_time);
    }
    
    nvs_set_u8(handle, NVS_KEY_AUTO_ENABLED, s_auto_settings.enabled ? 1 : 0);
    nvs_set_u32(handle, NVS_KEY_AUTO_INTERVAL, s_auto_settings.interval_minutes);
    nvs_set_u32(handle, NVS_KEY_TOTAL_UPLOAD, s_sync_status.total_uploaded);
    
    /* Save upload statistics */
    nvs_set_u32(handle, NVS_KEY_UPLOAD_COUNT, s_today_stats.upload_count);
    nvs_set_u32(handle, NVS_KEY_BYTES_UPLOADED, (uint32_t)s_today_stats.bytes_uploaded);
    nvs_set_u32(handle, NVS_KEY_DATA_POINTS, s_today_stats.data_points);
    
    /* Save uploaded file hashes */
    nvs_set_u32(handle, NVS_KEY_FILE_HASH_CNT, s_uploaded_file_count);
    if (s_uploaded_file_count > 0) {
        nvs_set_blob(handle, NVS_KEY_FILE_HASHES, s_uploaded_file_hashes, 
                     s_uploaded_file_count * sizeof(uint32_t));
    }
    
    nvs_commit(handle);
    nvs_close(handle);
    
    return ESP_OK;
}

/* Generate OneNet API authorization token (产品级别，用于设备注册等) */
static esp_err_t generate_onenet_token(const char *product_id, const char *access_key,
                                       char *token_buf, size_t buf_size)
{
    time_t now;
    time(&now);
    time_t expire_time = now + 365 * 24 * 3600; /* 1 year expiry */
    
    /* Build string to sign: et\nmethod\nres\nversion */
    char res_encoded[64];
    snprintf(res_encoded, sizeof(res_encoded), "products/%s", product_id);
    
    char string_to_sign[256];
    snprintf(string_to_sign, sizeof(string_to_sign),
             "%lld\nmd5\nproducts/%s\n2018-10-31",
             (long long)expire_time, product_id);
    
    /* Calculate HMAC-MD5 signature */
    uint8_t hmac[16];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), 1);
    
    /* Decode base64 access key */
    uint8_t key_decoded[64];
    size_t key_decoded_len = 0;
    int ret = mbedtls_base64_decode(key_decoded, sizeof(key_decoded), &key_decoded_len,
                                    (const uint8_t *)access_key, strlen(access_key));
    if (ret != 0) {
        /* If decode fails, use key directly */
        key_decoded_len = strlen(access_key);
        memcpy(key_decoded, access_key, key_decoded_len);
    }
    
    mbedtls_md_hmac_starts(&ctx, key_decoded, key_decoded_len);
    mbedtls_md_hmac_update(&ctx, (const uint8_t *)string_to_sign, strlen(string_to_sign));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);
    
    /* Base64 encode signature */
    char sign_base64[64];
    size_t sign_len = 0;
    mbedtls_base64_encode((uint8_t *)sign_base64, sizeof(sign_base64), &sign_len, hmac, 16);
    sign_base64[sign_len] = '\0';
    
    /* URL encode signature */
    char sign_encoded[128];
    char *p = sign_encoded;
    for (size_t i = 0; i < sign_len; i++) {
        if (sign_base64[i] == '+') {
            *p++ = '%'; *p++ = '2'; *p++ = 'B';
        } else if (sign_base64[i] == '/') {
            *p++ = '%'; *p++ = '2'; *p++ = 'F';
        } else if (sign_base64[i] == '=') {
            *p++ = '%'; *p++ = '3'; *p++ = 'D';
        } else {
            *p++ = sign_base64[i];
        }
    }
    *p = '\0';
    
    /* Build final token */
    snprintf(token_buf, buf_size,
             "version=2018-10-31&res=products%%2F%s&et=%lld&method=md5&sign=%s",
             product_id, (long long)expire_time, sign_encoded);
    
    return ESP_OK;
}

/**
 * @brief 生成 OneNET API 调用鉴权 Token（用户级别，用于文件上传等）
 * 
 * 根据 OneNET 文档：https://open.iot.10086.cn/doc/v5/fuse/detail/1464
 * - version: 2022-05-01
 * - res: userid/{用户ID}
 * - method: sha1
 * - access_key: 用户的密钥（在用户中心获取）
 * 
 * 签名字符串格式: {et}\n{method}\n{res}\n{version}
 */
static esp_err_t generate_onenet_user_token(const char *user_id, const char *user_access_key,
                                            char *token_buf, size_t buf_size)
{
    time_t now;
    time(&now);
    time_t expire_time = now + 100 * 24 * 3600; /* 100天有效期 */
    
    /* 构建签名字符串: et\nmethod\nres\nversion */
    char string_to_sign[256];
    snprintf(string_to_sign, sizeof(string_to_sign),
             "%lld\nsha1\nuserid/%s\n2022-05-01",
             (long long)expire_time, user_id);
    
    ESP_LOGI(TAG, "User token string_to_sign: %s", string_to_sign);
    
    /* 计算 HMAC-SHA1 签名 */
    uint8_t hmac[20];  /* SHA1 输出 20 字节 */
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    
    /* Base64 解码 access_key */
    uint8_t key_decoded[128];
    size_t key_decoded_len = 0;
    int ret = mbedtls_base64_decode(key_decoded, sizeof(key_decoded), &key_decoded_len,
                                    (const uint8_t *)user_access_key, strlen(user_access_key));
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to decode user access_key: %d", ret);
        mbedtls_md_free(&ctx);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "User access_key decoded: input_len=%d, decoded_len=%d", 
             (int)strlen(user_access_key), (int)key_decoded_len);
    
    mbedtls_md_hmac_starts(&ctx, key_decoded, key_decoded_len);
    mbedtls_md_hmac_update(&ctx, (const uint8_t *)string_to_sign, strlen(string_to_sign));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);
    
    /* Base64 编码签名 */
    char sign_base64[64];
    size_t sign_len = 0;
    mbedtls_base64_encode((uint8_t *)sign_base64, sizeof(sign_base64), &sign_len, hmac, 20);
    sign_base64[sign_len] = '\0';
    
    /* URL 编码签名和 res */
    char sign_encoded[128];
    char *p = sign_encoded;
    for (size_t i = 0; i < sign_len; i++) {
        if (sign_base64[i] == '+') {
            *p++ = '%'; *p++ = '2'; *p++ = 'B';
        } else if (sign_base64[i] == '/') {
            *p++ = '%'; *p++ = '2'; *p++ = 'F';
        } else if (sign_base64[i] == '=') {
            *p++ = '%'; *p++ = '3'; *p++ = 'D';
        } else {
            *p++ = sign_base64[i];
        }
    }
    *p = '\0';
    
    /* 构建最终 Token */
    snprintf(token_buf, buf_size,
             "version=2022-05-01&res=userid%%2F%s&et=%lld&method=sha1&sign=%s",
             user_id, (long long)expire_time, sign_encoded);
    
    ESP_LOGI(TAG, "Generated user token: version=2022-05-01&res=userid/%s&et=%lld&method=sha1&sign=...",
             user_id, (long long)expire_time);
    
    return ESP_OK;
}

static esp_err_t register_device_on_onenet(void)
{
    ESP_LOGI(TAG, "Registering device on OneNet (HTTP API)...");
    
    /* Get next device index */
    nvs_handle_t handle;
    uint32_t dev_index = 1;
    
    if (nvs_open(NVS_NAMESPACE_CLOUD, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_get_u32(handle, NVS_KEY_DEVICE_INDEX, &dev_index);
        dev_index++;
        nvs_set_u32(handle, NVS_KEY_DEVICE_INDEX, dev_index);
        nvs_commit(handle);
        nvs_close(handle);
    }
    
    /* Generate device name */
    snprintf(s_device_info.device_name, sizeof(s_device_info.device_name),
             "ExDebugTool_%lu", (unsigned long)dev_index);
    
    ESP_LOGI(TAG, "Device name: %s", s_device_info.device_name);
    ESP_LOGI(TAG, "Device code: %s", s_device_info.device_code);
    
    /* Build request JSON body */
    cJSON *body = cJSON_CreateObject();
    if (body == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return ESP_ERR_NO_MEM;
    }
    
    cJSON_AddStringToObject(body, "product_id", ONENET_HTTP_PRODUCT_ID);
    cJSON_AddStringToObject(body, "device_name", s_device_info.device_name);
    cJSON_AddStringToObject(body, "desc", "ESP32-P4 ExDebugTool Device");
    
    char lon_str[16], lat_str[16];
    snprintf(lon_str, sizeof(lon_str), "%.6f", s_device_info.longitude);
    snprintf(lat_str, sizeof(lat_str), "%.6f", s_device_info.latitude);
    cJSON_AddStringToObject(body, "lon", lon_str);
    cJSON_AddStringToObject(body, "lat", lat_str);
    
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    
    if (body_str == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Request body: %s", body_str);
    
    /* Generate authorization token using HTTP product credentials */
    char auth_token[256];
    generate_onenet_token(ONENET_HTTP_PRODUCT_ID, ONENET_ACCESS_KEY, auth_token, sizeof(auth_token));
    ESP_LOGI(TAG, "Auth token generated");
    
    /* Configure HTTPS client */
    char url[128];
    snprintf(url, sizeof(url), "https://%s%s", ONENET_API_HOST, ONENET_API_CREATE);
    
    esp_http_client_config_t config = {
        .url = url,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .skip_cert_common_name_check = true,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        free(body_str);
        return ESP_FAIL;
    }
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "authorization", auth_token);
    esp_http_client_set_post_field(client, body_str, strlen(body_str));
    
    /* Perform request */
    esp_err_t err = esp_http_client_perform(client);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        free(body_str);
        esp_http_client_cleanup(client);
        return err;
    }
    
    int status_code = esp_http_client_get_status_code(client);
    int content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d", status_code, content_length);
    
    /* Read response */
    char *response_buf = NULL;
    if (content_length > 0 && content_length < 4096) {
        response_buf = malloc(content_length + 1);
        if (response_buf) {
            int read_len = esp_http_client_read(client, response_buf, content_length);
            response_buf[read_len > 0 ? read_len : 0] = '\0';
            ESP_LOGI(TAG, "Response: %s", response_buf);
        }
    }
    
    free(body_str);
    esp_http_client_cleanup(client);
    
    if (response_buf == NULL) {
        ESP_LOGE(TAG, "Failed to read response");
        return ESP_FAIL;
    }
    
    /* Parse response JSON */
    cJSON *resp = cJSON_Parse(response_buf);
    free(response_buf);
    
    if (resp == NULL) {
        ESP_LOGE(TAG, "Failed to parse response JSON");
        return ESP_FAIL;
    }
    
    cJSON *code = cJSON_GetObjectItem(resp, "code");
    if (!cJSON_IsNumber(code) || code->valueint != 0) {
        cJSON *msg = cJSON_GetObjectItem(resp, "msg");
        ESP_LOGE(TAG, "API error: code=%d, msg=%s", 
                 cJSON_IsNumber(code) ? code->valueint : -1,
                 cJSON_IsString(msg) ? msg->valuestring : "unknown");
        cJSON_Delete(resp);
        return ESP_FAIL;
    }
    
    /* Extract device info from response */
    cJSON *data = cJSON_GetObjectItem(resp, "data");
    if (data) {
        cJSON *did = cJSON_GetObjectItem(data, "did");
        cJSON *sec_key = cJSON_GetObjectItem(data, "sec_key");
        
        if (cJSON_IsNumber(did)) {
            snprintf(s_device_info.device_id, sizeof(s_device_info.device_id),
                     "%d", did->valueint);
        } else if (cJSON_IsString(did)) {
            strncpy(s_device_info.device_id, did->valuestring, 
                    sizeof(s_device_info.device_id) - 1);
        }
        
        if (cJSON_IsString(sec_key)) {
            strncpy(s_device_info.sec_key, sec_key->valuestring,
                    sizeof(s_device_info.sec_key) - 1);
        }
        
        ESP_LOGI(TAG, "Device registered successfully!");
        ESP_LOGI(TAG, "  Device ID: %s", s_device_info.device_id);
        ESP_LOGI(TAG, "  Device Name: %s", s_device_info.device_name);
    }
    
    cJSON_Delete(resp);
    return ESP_OK;
}



/* Upload file data to OneNet using HTTP API - 支持示波器CSV格式 */
/**
 * @brief 使用 OneNET 官方文件上传 API 上传整个文件
 * 
 * API: POST https://iot-api.heclouds.com/device/file-upload
 * Content-Type: multipart/form-data
 * 
 * 支持格式: .jpg .jpeg .png .bmp .gif .webp .tiff .txt
 * 文件大小限制: 单个文件不超过 20MB
 * 
 * @note CSV文件需要重命名为.txt后缀才能上传
 */
static esp_err_t upload_file_data_to_onenet(upload_task_t *task)
{
    if (task == NULL || strlen(task->file_path) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Uploading file to OneNet: %s (size: %lu bytes)", 
             task->file_path, (unsigned long)task->file_size);
    
    /* 检查文件大小限制 (20MB) */
    if (task->file_size > 20 * 1024 * 1024) {
        ESP_LOGE(TAG, "File too large: %lu bytes (max 20MB)", (unsigned long)task->file_size);
        snprintf(task->error_msg, sizeof(task->error_msg), "File too large (max 20MB)");
        return ESP_FAIL;
    }
    
    /* 打开文件 */
    FILE *f = fopen(task->file_path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", task->file_path);
        snprintf(task->error_msg, sizeof(task->error_msg), "File open failed");
        return ESP_FAIL;
    }
    
    /* 读取整个文件到内存 */
    uint8_t *file_data = heap_caps_malloc(task->file_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (file_data == NULL) {
        file_data = malloc(task->file_size);
    }
    if (file_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for file: %lu bytes", (unsigned long)task->file_size);
        fclose(f);
        snprintf(task->error_msg, sizeof(task->error_msg), "Memory allocation failed");
        return ESP_FAIL;
    }
    
    size_t bytes_read = fread(file_data, 1, task->file_size, f);
    fclose(f);
    
    if (bytes_read != task->file_size) {
        ESP_LOGE(TAG, "Failed to read file: read %d, expected %lu", 
                 (int)bytes_read, (unsigned long)task->file_size);
        free(file_data);
        snprintf(task->error_msg, sizeof(task->error_msg), "File read failed");
        return ESP_FAIL;
    }
    
    task->progress = 10;
    if (s_progress_cb) {
        s_progress_cb(task->task_id, task->progress, 0);
    }
    
    ESP_LOGI(TAG, "Generating auth token...");
    
    /* 确定上传文件名 - CSV文件改为.txt后缀 */
    char upload_filename[128];
    strncpy(upload_filename, task->file_name, sizeof(upload_filename) - 1);
    upload_filename[sizeof(upload_filename) - 1] = '\0';
    
    /* 如果是CSV文件，改为.txt后缀 */
    char *ext = strrchr(upload_filename, '.');
    if (ext && strcasecmp(ext, ".csv") == 0) {
        strcpy(ext, ".txt");
        ESP_LOGI(TAG, "CSV file renamed to: %s", upload_filename);
    }
    
    /* 构建 multipart/form-data 请求体 */
    const char *boundary = "----ESP32FileUploadBoundary";
    
    /* 计算请求体大小 */
    char form_header[512];
    int header_len = snprintf(form_header, sizeof(form_header),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"product_id\"\r\n\r\n"
        "%s\r\n"
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"device_name\"\r\n\r\n"
        "%s\r\n"
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n",
        boundary, ONENET_HTTP_PRODUCT_ID,
        boundary, s_device_info.device_name,
        boundary, upload_filename);
    
    char form_footer[64];
    int footer_len = snprintf(form_footer, sizeof(form_footer), "\r\n--%s--\r\n", boundary);
    
    size_t total_len = header_len + task->file_size + footer_len;
    
    /* 分配请求体缓冲区 */
    ESP_LOGI(TAG, "Allocating body buffer: %d bytes", (int)total_len);
    uint8_t *body = heap_caps_malloc(total_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (body == NULL) {
        ESP_LOGW(TAG, "PSRAM alloc failed, trying internal RAM");
        body = malloc(total_len);
    }
    if (body == NULL) {
        ESP_LOGE(TAG, "Failed to allocate body buffer: %d bytes", (int)total_len);
        free(file_data);
        snprintf(task->error_msg, sizeof(task->error_msg), "Memory allocation failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Body buffer allocated at %p", body);
    
    /* 组装请求体 */
    ESP_LOGI(TAG, "Assembling request body...");
    memcpy(body, form_header, header_len);
    memcpy(body + header_len, file_data, task->file_size);
    memcpy(body + header_len + task->file_size, form_footer, footer_len);
    
    free(file_data);  /* 文件数据已复制，释放原缓冲区 */
    ESP_LOGI(TAG, "Request body assembled");
    
    task->progress = 30;
    if (s_progress_cb) {
        s_progress_cb(task->task_id, task->progress, 0);
    }
    
    ESP_LOGI(TAG, "Request body size: %d bytes", (int)total_len);
    
    /* 生成用户级别的 API 鉴权 token（使用 SHA1 和用户 access_key）
     * 文件上传 API 需要用户级别认证
     * version: 2022-05-01, res: userid/{用户ID}, method: sha1
     */
    char auth_token[512];
    esp_err_t token_ret = generate_onenet_user_token(ONENET_USER_ID, ONENET_USER_ACCESS_KEY, 
                                                      auth_token, sizeof(auth_token));
    if (token_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate user token");
        free(body);
        snprintf(task->error_msg, sizeof(task->error_msg), "User token generation failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "User auth token generated: %s", auth_token);
    
    /* 配置 HTTP 客户端 - 使用 iot-api.heclouds.com（官方文档指定） */
    esp_http_client_config_t config = {
        .url = "https://iot-api.heclouds.com/device/file-upload",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 120000,  /* 120秒超时，大文件需要更长时间 */
        .crt_bundle_attach = esp_crt_bundle_attach,  /* 使用证书bundle */
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        free(body);
        snprintf(task->error_msg, sizeof(task->error_msg), "HTTP init failed");
        return ESP_FAIL;
    }
    
    /* 设置请求头 - 使用用户级别 API 鉴权 token */
    char content_type[64];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);
    esp_http_client_set_header(client, "authorization", auth_token);
    ESP_LOGI(TAG, "Uploading to iot-api.heclouds.com with user API token (userid/%s)", ONENET_USER_ID);
    
    task->progress = 50;
    if (s_progress_cb) {
        s_progress_cb(task->task_id, task->progress, task->file_size / 2);
    }
    
    /* 执行请求 - 使用 open/write/fetch_headers/read/close 方式以正确读取响应 */
    ESP_LOGI(TAG, "Sending file upload request...");
    
    esp_err_t err = esp_http_client_open(client, total_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        free(body);
        esp_http_client_cleanup(client);
        snprintf(task->error_msg, sizeof(task->error_msg), "HTTP open failed");
        return ESP_FAIL;
    }
    
    /* 写入请求体 */
    int wlen = esp_http_client_write(client, (const char *)body, total_len);
    free(body);  /* 释放请求体缓冲区 */
    
    if (wlen < 0) {
        ESP_LOGE(TAG, "Failed to write request body");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        snprintf(task->error_msg, sizeof(task->error_msg), "HTTP write failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Wrote %d bytes to server", wlen);
    
    /* 获取响应头 */
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Upload HTTP Status = %d, Content-Length = %d", status_code, content_length);
    
    /* 读取响应体 - 循环读取直到没有更多数据 */
    char response[512] = {0};
    int total_read = 0;
    int read_len;
    while ((read_len = esp_http_client_read(client, response + total_read, 
                                             sizeof(response) - 1 - total_read)) > 0) {
        total_read += read_len;
        if (total_read >= (int)sizeof(response) - 1) break;
    }
    response[total_read] = '\0';
    ESP_LOGI(TAG, "Response (total_read=%d, content_length=%d): %s", total_read, content_length, response);
    
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    task->progress = 100;
    task->uploaded_bytes = task->file_size;
    if (s_progress_cb) {
        s_progress_cb(task->task_id, task->progress, task->uploaded_bytes);
    }
    
    /* 解析响应 */
    if (status_code == 200) {
        cJSON *resp = cJSON_Parse(response);
        if (resp != NULL) {
            cJSON *code = cJSON_GetObjectItem(resp, "code");
            cJSON *msg = cJSON_GetObjectItem(resp, "msg");
            cJSON *data = cJSON_GetObjectItem(resp, "data");
            
            if (cJSON_IsNumber(code) && code->valueint == 0) {
                /* 上传成功 */
                if (data != NULL) {
                    cJSON *fid = cJSON_GetObjectItem(data, "fid");
                    if (cJSON_IsString(fid)) {
                        ESP_LOGI(TAG, "File uploaded successfully! FID: %s", fid->valuestring);
                    }
                }
                cJSON_Delete(resp);
                return ESP_OK;
            } else {
                const char *err_msg = cJSON_IsString(msg) ? msg->valuestring : "Unknown error";
                ESP_LOGE(TAG, "Upload failed: code=%d, msg=%s", 
                         cJSON_IsNumber(code) ? code->valueint : -1, err_msg);
                snprintf(task->error_msg, sizeof(task->error_msg), "%s", err_msg);
                cJSON_Delete(resp);
                return ESP_FAIL;
            }
        }
        /* 无法解析响应，但状态码是200，认为成功 */
        ESP_LOGI(TAG, "File upload completed (status 200)");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "File upload failed with status: %d", status_code);
        snprintf(task->error_msg, sizeof(task->error_msg), "HTTP status: %d", status_code);
        return ESP_FAIL;
    }
}


/* ==================== 设备凭证获取函数（供wifi_onenet.c调用）==================== */

/**
 * @brief 获取设备凭证（用于HTTP API认证）
 * @param product_id 输出产品ID
 * @param pid_len 产品ID缓冲区长度
 * @param device_name 输出设备名称
 * @param name_len 设备名称缓冲区长度
 * @param sec_key 输出设备密钥
 * @param key_len 密钥缓冲区长度
 * @return ESP_OK表示成功
 */
esp_err_t cloud_manager_get_device_credentials(char *product_id, size_t pid_len,
                                                char *device_name, size_t name_len,
                                                char *sec_key, size_t key_len)
{
    if (!s_initialized || s_device_info.state != CLOUD_STATE_ACTIVATED) {
        ESP_LOGW(TAG, "Device not activated, cannot get credentials");
        return ESP_ERR_INVALID_STATE;
    }

    if (product_id != NULL && pid_len > 0) {
        /* 使用保存的产品ID，如果为空则使用默认值 */
        if (strlen(s_device_info.product_id) > 0) {
            strncpy(product_id, s_device_info.product_id, pid_len - 1);
        } else {
            strncpy(product_id, ONENET_HTTP_PRODUCT_ID, pid_len - 1);
        }
        product_id[pid_len - 1] = '\0';
    }

    if (device_name != NULL && name_len > 0) {
        strncpy(device_name, s_device_info.device_name, name_len - 1);
        device_name[name_len - 1] = '\0';
    }

    if (sec_key != NULL && key_len > 0) {
        strncpy(sec_key, s_device_info.sec_key, key_len - 1);
        sec_key[key_len - 1] = '\0';
    }

    /* 检查 sec_key 是否有效 */
    if (strlen(s_device_info.sec_key) == 0) {
        ESP_LOGE(TAG, "ERROR: sec_key is empty! Device needs to be re-activated.");
        ESP_LOGE(TAG, "Please go back to QR code page and re-activate the device.");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGD(TAG, "Device credentials: PID=%s, Name=%s, SecKey_len=%d", 
             product_id ? product_id : "NULL",
             device_name ? device_name : "NULL",
             (int)strlen(s_device_info.sec_key));

    return ESP_OK;
}
