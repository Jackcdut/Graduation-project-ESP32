/**
 * @file sdcard_manager.c
 * @brief SD Card Management Module Implementation
 */

#include "sdcard_manager.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "dirent.h"
#include "sys/stat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "SDCARD_MGR";

/* ============== State Variables ============== */

static bool g_initialized = false;
static SemaphoreHandle_t g_mutex = NULL;
static sdcard_stats_t g_stats = {0};
static sdcard_file_info_t g_recent_files[SDCARD_MAX_RECENT_FILES] = {0};
static uint32_t g_recent_count = 0;
static TimerHandle_t g_refresh_timer = NULL;
static TaskHandle_t g_refresh_task = NULL;
static SemaphoreHandle_t g_refresh_sem = NULL;

/* I/O tracking */
static uint64_t g_read_bytes_total = 0;
static uint64_t g_write_bytes_total = 0;
static time_t g_today_start = 0;

/* ============== Internal Functions ============== */

static bool is_mounted(void)
{
    struct stat st;
    return (stat(SDCARD_MOUNT_POINT, &st) == 0);
}

static sdcard_file_type_t get_file_type_from_path(const char *path)
{
    if (strstr(path, SDCARD_SCREENSHOT_DIR) != NULL) {
        return SDCARD_FILE_TYPE_SCREENSHOT;
    } else if (strstr(path, SDCARD_OSCILLOSCOPE_DIR) != NULL) {
        return SDCARD_FILE_TYPE_OSCILLOSCOPE;
    } else if (strstr(path, SDCARD_CONFIG_DIR) != NULL) {
        return SDCARD_FILE_TYPE_CONFIG;
    } else if (strstr(path, SDCARD_MEDIA_DIR) != NULL) {
        return SDCARD_FILE_TYPE_MEDIA;
    }
    return SDCARD_FILE_TYPE_OTHER;
}

static void ensure_directories(void)
{
    struct stat st;
    
    if (stat(SDCARD_SCREENSHOT_DIR, &st) != 0) {
        mkdir(SDCARD_SCREENSHOT_DIR, 0755);
        ESP_LOGI(TAG, "Created directory: %s", SDCARD_SCREENSHOT_DIR);
    }
    
    if (stat(SDCARD_OSCILLOSCOPE_DIR, &st) != 0) {
        mkdir(SDCARD_OSCILLOSCOPE_DIR, 0755);
        ESP_LOGI(TAG, "Created directory: %s", SDCARD_OSCILLOSCOPE_DIR);
    }
    
    if (stat(SDCARD_CONFIG_DIR, &st) != 0) {
        mkdir(SDCARD_CONFIG_DIR, 0755);
        ESP_LOGI(TAG, "Created directory: %s", SDCARD_CONFIG_DIR);
    }
    
    if (stat(SDCARD_MEDIA_DIR, &st) != 0) {
        mkdir(SDCARD_MEDIA_DIR, 0755);
        ESP_LOGI(TAG, "Created directory: %s", SDCARD_MEDIA_DIR);
    }
}

static void scan_directory(const char *dir_path, sdcard_file_type_t type,
                          uint32_t *count, uint64_t *bytes)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        return;
    }
    
    struct dirent *entry;
    struct stat st;
    char full_path[SDCARD_MAX_PATH_LEN];
    
    *count = 0;
    *bytes = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            (*count)++;
            (*bytes) += st.st_size;
        }
    }
    
    closedir(dir);
}

static void refresh_timer_callback(TimerHandle_t timer)
{
    (void)timer;
    if (g_refresh_sem) {
        xSemaphoreGive(g_refresh_sem);   /* Defer work to dedicated task to keep timer stack small */
    }
}

static void refresh_worker(void *arg)
{
    (void)arg;
    for (;;) {
        if (g_refresh_sem) {
            xSemaphoreTake(g_refresh_sem, portMAX_DELAY);
            sdcard_manager_refresh_stats();  /* Run with larger task stack */
        }
    }
}

/* ============== Public Functions ============== */

esp_err_t sdcard_manager_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing SD Card Manager...");
    
    g_mutex = xSemaphoreCreateMutex();
    if (g_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    if (!is_mounted()) {
        ESP_LOGW(TAG, "SD card not mounted");
        g_initialized = true;
        return ESP_OK;  /* Still initialize, will check later */
    }
    
    /* Create required directories */
    ensure_directories();
    
    /* Create refresh worker (bigger stack to avoid timer-task overflow) */
    g_refresh_sem = xSemaphoreCreateBinary();
    if (g_refresh_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create refresh semaphore");
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreatePinnedToCore(refresh_worker, "sd_ref_task", 4096, NULL, 5, &g_refresh_task, tskNO_AFFINITY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create refresh task");
        vSemaphoreDelete(g_refresh_sem);
        g_refresh_sem = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    /* Initial stats refresh */
    sdcard_manager_refresh_stats();
    
    /* Create periodic refresh timer (every 5 seconds) */
    g_refresh_timer = xTimerCreate("sd_refresh", pdMS_TO_TICKS(5000),
                                    pdTRUE, NULL, refresh_timer_callback);
    if (g_refresh_timer != NULL) {
        xTimerStart(g_refresh_timer, 0);
    }
    
    /* Initialize today tracking */
    time(&g_today_start);
    struct tm *tm_info = localtime(&g_today_start);
    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    g_today_start = mktime(tm_info);
    
    g_initialized = true;
    ESP_LOGI(TAG, "SD Card Manager initialized");
    
    return ESP_OK;
}

void sdcard_manager_deinit(void)
{
    if (!g_initialized) return;
    
    if (g_refresh_timer != NULL) {
        xTimerStop(g_refresh_timer, portMAX_DELAY);
        xTimerDelete(g_refresh_timer, portMAX_DELAY);
        g_refresh_timer = NULL;
    }
    
    if (g_refresh_task != NULL) {
        vTaskDelete(g_refresh_task);
        g_refresh_task = NULL;
    }
    if (g_refresh_sem != NULL) {
        vSemaphoreDelete(g_refresh_sem);
        g_refresh_sem = NULL;
    }
    
    if (g_mutex != NULL) {
        vSemaphoreDelete(g_mutex);
        g_mutex = NULL;
    }
    
    g_initialized = false;
    ESP_LOGI(TAG, "SD Card Manager deinitialized");
}

sdcard_status_t sdcard_manager_get_status(void)
{
    if (!is_mounted()) {
        return SDCARD_STATUS_NOT_PRESENT;
    }
    return SDCARD_STATUS_MOUNTED;
}

bool sdcard_manager_is_mounted(void)
{
    return is_mounted();
}

esp_err_t sdcard_manager_get_stats(sdcard_stats_t *stats)
{
    if (stats == NULL) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    memcpy(stats, &g_stats, sizeof(sdcard_stats_t));
    
    xSemaphoreGive(g_mutex);
    return ESP_OK;
}

esp_err_t sdcard_manager_refresh_stats(void)
{
    if (!is_mounted()) {
        ESP_LOGW(TAG, "SD card not mounted, cannot refresh stats");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    /* Get filesystem stats */
    FATFS *fs;
    DWORD fre_clust;
    
    if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
        uint64_t sector_size = fs->ssize;
        uint64_t cluster_size = sector_size * fs->csize;
        g_stats.total_bytes = (uint64_t)(fs->n_fatent - 2) * cluster_size;
        g_stats.free_bytes = (uint64_t)fre_clust * cluster_size;
        g_stats.used_bytes = g_stats.total_bytes - g_stats.free_bytes;
    }
    
    /* Scan directories */
    scan_directory(SDCARD_SCREENSHOT_DIR, SDCARD_FILE_TYPE_SCREENSHOT,
                   &g_stats.screenshot_count, &g_stats.screenshot_bytes);
    
    scan_directory(SDCARD_OSCILLOSCOPE_DIR, SDCARD_FILE_TYPE_OSCILLOSCOPE,
                   &g_stats.oscilloscope_count, &g_stats.oscilloscope_bytes);
    
    scan_directory(SDCARD_CONFIG_DIR, SDCARD_FILE_TYPE_CONFIG,
                   &g_stats.config_count, &g_stats.config_bytes);
    
    scan_directory(SDCARD_MEDIA_DIR, SDCARD_FILE_TYPE_MEDIA,
                   &g_stats.media_count, &g_stats.media_bytes);
    
    /* Calculate other files */
    uint64_t known_bytes = g_stats.screenshot_bytes + g_stats.oscilloscope_bytes + 
                           g_stats.config_bytes + g_stats.media_bytes;
    if (g_stats.used_bytes > known_bytes) {
        g_stats.other_bytes = g_stats.used_bytes - known_bytes;
    } else {
        g_stats.other_bytes = 0;
    }
    
    /* Calculate average file size */
    uint32_t total_files = g_stats.screenshot_count + g_stats.oscilloscope_count + 
                           g_stats.config_count + g_stats.media_count;
    if (total_files > 0) {
        g_stats.avg_file_size = (uint32_t)(g_stats.used_bytes / total_files);
    }
    
    xSemaphoreGive(g_mutex);
    
    ESP_LOGD(TAG, "Stats refreshed: %llu/%llu bytes used", 
             g_stats.used_bytes, g_stats.total_bytes);
    
    return ESP_OK;
}

esp_err_t sdcard_manager_list_files(sdcard_file_type_t type,
                                     sdcard_file_info_t *files,
                                     uint32_t max_files,
                                     uint32_t *count)
{
    if (files == NULL || count == NULL) return ESP_ERR_INVALID_ARG;
    
    const char *dir_path;
    switch (type) {
        case SDCARD_FILE_TYPE_SCREENSHOT:
            dir_path = SDCARD_SCREENSHOT_DIR;
            break;
        case SDCARD_FILE_TYPE_OSCILLOSCOPE:
            dir_path = SDCARD_OSCILLOSCOPE_DIR;
            break;
        case SDCARD_FILE_TYPE_CONFIG:
            dir_path = SDCARD_CONFIG_DIR;
            break;
        case SDCARD_FILE_TYPE_MEDIA:
            dir_path = SDCARD_MEDIA_DIR;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }
    
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        *count = 0;
        return ESP_ERR_NOT_FOUND;
    }
    
    struct dirent *entry;
    struct stat st;
    char full_path[SDCARD_MAX_PATH_LEN];
    *count = 0;
    
    while ((entry = readdir(dir)) != NULL && *count < max_files) {
        if (entry->d_name[0] == '.') continue;
        
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            sdcard_file_info_t *info = &files[*count];
            
            strncpy(info->name, entry->d_name, SDCARD_MAX_FILENAME_LEN - 1);
            info->name[SDCARD_MAX_FILENAME_LEN - 1] = '\0';
            
            strncpy(info->path, full_path, SDCARD_MAX_PATH_LEN - 1);
            info->path[SDCARD_MAX_PATH_LEN - 1] = '\0';
            
            info->type = type;
            info->size = st.st_size;
            info->created_time = st.st_mtime;
            info->accessed_time = st.st_atime;
            
            (*count)++;
        }
    }
    
    closedir(dir);
    return ESP_OK;
}

esp_err_t sdcard_manager_get_file_info(const char *path, sdcard_file_info_t *info)
{
    if (path == NULL || info == NULL) return ESP_ERR_INVALID_ARG;
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return ESP_ERR_NOT_FOUND;
    }
    
    /* Extract filename from path */
    const char *filename = strrchr(path, '/');
    if (filename != NULL) {
        filename++;
    } else {
        filename = path;
    }
    
    strncpy(info->name, filename, SDCARD_MAX_FILENAME_LEN - 1);
    info->name[SDCARD_MAX_FILENAME_LEN - 1] = '\0';
    
    strncpy(info->path, path, SDCARD_MAX_PATH_LEN - 1);
    info->path[SDCARD_MAX_PATH_LEN - 1] = '\0';
    
    info->type = get_file_type_from_path(path);
    info->size = st.st_size;
    info->created_time = st.st_mtime;
    info->accessed_time = st.st_atime;
    
    return ESP_OK;
}

esp_err_t sdcard_manager_delete_file(const char *path)
{
    if (path == NULL) return ESP_ERR_INVALID_ARG;
    
    if (remove(path) != 0) {
        ESP_LOGE(TAG, "Failed to delete file: %s", path);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Deleted file: %s", path);
    
    /* Refresh stats after deletion */
    sdcard_manager_refresh_stats();
    
    return ESP_OK;
}

esp_err_t sdcard_manager_delete_files(const char **paths, uint32_t count, uint32_t *deleted)
{
    if (paths == NULL || deleted == NULL) return ESP_ERR_INVALID_ARG;
    
    *deleted = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        if (paths[i] != NULL && remove(paths[i]) == 0) {
            (*deleted)++;
            ESP_LOGI(TAG, "Deleted file: %s", paths[i]);
        }
    }
    
    /* Refresh stats after deletion */
    sdcard_manager_refresh_stats();
    
    return (*deleted == count) ? ESP_OK : ESP_FAIL;
}

esp_err_t sdcard_manager_get_recent_files(sdcard_file_info_t *files,
                                           uint32_t max_files,
                                           uint32_t *count)
{
    if (files == NULL || count == NULL) return ESP_ERR_INVALID_ARG;
    
    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    *count = (g_recent_count < max_files) ? g_recent_count : max_files;
    memcpy(files, g_recent_files, (*count) * sizeof(sdcard_file_info_t));
    
    xSemaphoreGive(g_mutex);
    return ESP_OK;
}

void sdcard_manager_record_access(const char *path)
{
    if (path == NULL) return;
    
    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }
    
    /* Check if file already in recent list */
    for (uint32_t i = 0; i < g_recent_count; i++) {
        if (strcmp(g_recent_files[i].path, path) == 0) {
            /* Move to front */
            sdcard_file_info_t temp = g_recent_files[i];
            memmove(&g_recent_files[1], &g_recent_files[0], i * sizeof(sdcard_file_info_t));
            g_recent_files[0] = temp;
            time(&g_recent_files[0].accessed_time);
            xSemaphoreGive(g_mutex);
            return;
        }
    }
    
    /* Add new entry at front */
    if (g_recent_count < SDCARD_MAX_RECENT_FILES) {
        memmove(&g_recent_files[1], &g_recent_files[0], 
                g_recent_count * sizeof(sdcard_file_info_t));
        g_recent_count++;
    } else {
        memmove(&g_recent_files[1], &g_recent_files[0], 
                (SDCARD_MAX_RECENT_FILES - 1) * sizeof(sdcard_file_info_t));
    }
    
    sdcard_manager_get_file_info(path, &g_recent_files[0]);
    time(&g_recent_files[0].accessed_time);
    
    xSemaphoreGive(g_mutex);
}

void sdcard_manager_format_size(uint64_t bytes, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) return;
    
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        snprintf(buffer, buffer_size, "%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024ULL * 1024ULL) {
        snprintf(buffer, buffer_size, "%.1f MB", bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024ULL) {
        snprintf(buffer, buffer_size, "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(buffer, buffer_size, "%llu B", bytes);
    }
}

void sdcard_manager_format_time(time_t t, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) return;
    
    struct tm *tm_info = localtime(&t);
    strftime(buffer, buffer_size, "%m-%d %H:%M", tm_info);
}

const char* sdcard_manager_get_type_icon(sdcard_file_type_t type)
{
    switch (type) {
        case SDCARD_FILE_TYPE_SCREENSHOT:
            return LV_SYMBOL_IMAGE;
        case SDCARD_FILE_TYPE_OSCILLOSCOPE:
            return LV_SYMBOL_FILE;
        case SDCARD_FILE_TYPE_CONFIG:
            return LV_SYMBOL_SETTINGS;
        case SDCARD_FILE_TYPE_MEDIA:
            return LV_SYMBOL_VIDEO;
        default:
            return LV_SYMBOL_FILE;
    }
}

const char* sdcard_manager_get_type_name(sdcard_file_type_t type)
{
    switch (type) {
        case SDCARD_FILE_TYPE_SCREENSHOT:
            return "Screenshot";
        case SDCARD_FILE_TYPE_OSCILLOSCOPE:
            return "Oscilloscope";
        case SDCARD_FILE_TYPE_CONFIG:
            return "Config";
        case SDCARD_FILE_TYPE_MEDIA:
            return "Media";
        default:
            return "Other";
    }
}

