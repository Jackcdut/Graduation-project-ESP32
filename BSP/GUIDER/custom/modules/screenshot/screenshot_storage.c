/**
 * @file screenshot_storage.c
 * @brief Screenshot Storage - SD Card Only
 * 
 * Simplified storage that saves screenshots directly to SD card.
 * Naming convention: Screenshot_001.bmp, Screenshot_002.bmp, etc.
 */

#include "screenshot_storage.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

static const char *TAG = "SCR_STORAGE";

/* SD Card paths */
#define SD_MOUNT_POINT      "/sdcard"
#define SCREENSHOT_DIR      "/sdcard/Screenshots"
#define SCREENSHOT_PREFIX   "Screenshot_"
#define SCREENSHOT_EXT      ".bmp"

/* Screenshot parameters */
#define SCREENSHOT_BPP      3   /* 24-bit RGB */

/* State variables */
static bool g_initialized = false;
static bool g_sd_available = false;
static int g_screenshot_count = 0;
static int g_next_number = 1;  /* Next screenshot number */

/* Screenshot list cache */
#define MAX_CACHED_SCREENSHOTS 100
static screenshot_storage_info_t g_screenshot_cache[MAX_CACHED_SCREENSHOTS];
static int g_cache_count = 0;

/* Mutex for thread safety */
static SemaphoreHandle_t g_mutex = NULL;

/* Forward declarations */
static esp_err_t ensure_screenshot_dir(void);
static esp_err_t scan_screenshots(void);
static int get_next_screenshot_number(void);

esp_err_t screenshot_storage_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing screenshot storage (SD Card)...");
    
    /* Create mutex */
    g_mutex = xSemaphoreCreateMutex();
    if (g_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    /* Check if SD card is mounted */
    struct stat st;
    if (stat(SD_MOUNT_POINT, &st) == 0) {
        g_sd_available = true;
        ESP_LOGI(TAG, "SD card is available");
        
        /* Create screenshot directory */
        ensure_screenshot_dir();
        
        /* Scan existing screenshots */
        scan_screenshots();
        
        ESP_LOGI(TAG, "Found %d screenshots on SD card", g_cache_count);
    } else {
        g_sd_available = false;
        ESP_LOGW(TAG, "SD card not available - screenshots will not be saved");
    }

    g_initialized = true;
    ESP_LOGI(TAG, "Screenshot storage initialized");
    return ESP_OK;
}

esp_err_t screenshot_storage_deinit(void)
{
    if (!g_initialized) {
            return ESP_OK;
        }
    
    if (g_mutex != NULL) {
        vSemaphoreDelete(g_mutex);
        g_mutex = NULL;
    }
    
    g_initialized = false;
    g_sd_available = false;
    g_cache_count = 0;
    
    ESP_LOGI(TAG, "Screenshot storage deinitialized");
    return ESP_OK;
}

/**
 * @brief Ensure screenshot directory exists
 */
static esp_err_t ensure_screenshot_dir(void)
{
    struct stat st;
    if (stat(SCREENSHOT_DIR, &st) != 0) {
        ESP_LOGI(TAG, "Creating screenshot directory: %s", SCREENSHOT_DIR);
        if (mkdir(SCREENSHOT_DIR, 0755) != 0) {
            ESP_LOGE(TAG, "Failed to create directory: %s", strerror(errno));
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

/**
 * @brief Scan existing screenshots and find next number
 */
static esp_err_t scan_screenshots(void)
{
    DIR *dir = opendir(SCREENSHOT_DIR);
    if (dir == NULL) {
        ESP_LOGW(TAG, "Cannot open screenshot directory");
        return ESP_FAIL;
    }
    
    g_cache_count = 0;
    g_next_number = 1;
    int max_number = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && g_cache_count < MAX_CACHED_SCREENSHOTS) {
        /* Check if it's a screenshot file */
        if (strncmp(entry->d_name, SCREENSHOT_PREFIX, strlen(SCREENSHOT_PREFIX)) == 0 &&
            strstr(entry->d_name, SCREENSHOT_EXT) != NULL) {
            
            /* Extract number from filename */
            int num = 0;
            sscanf(entry->d_name, SCREENSHOT_PREFIX "%d", &num);
            if (num > max_number) {
                max_number = num;
            }
            
            /* Add to cache */
            snprintf(g_screenshot_cache[g_cache_count].filename,
                     sizeof(g_screenshot_cache[g_cache_count].filename),
                     "%s", entry->d_name);

            /* Get file info */
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/%s", SCREENSHOT_DIR, entry->d_name);
            
            struct stat st;
            if (stat(filepath, &st) == 0) {
                g_screenshot_cache[g_cache_count].size = st.st_size;
                g_screenshot_cache[g_cache_count].timestamp = st.st_mtime;
            }

            g_screenshot_cache[g_cache_count].location = STORAGE_SD;
            g_cache_count++;
        }
    }

    closedir(dir);
    
    g_next_number = max_number + 1;
    g_screenshot_count = g_cache_count;
    
    ESP_LOGI(TAG, "Scanned %d screenshots, next number: %d", g_cache_count, g_next_number);
    return ESP_OK;
}

/**
 * @brief Get next screenshot number
 */
static int get_next_screenshot_number(void)
{
    return g_next_number++;
}

/**
 * @brief Generate screenshot filename
 */
esp_err_t screenshot_storage_generate_filename(char *filename, size_t size)
{
    if (!g_sd_available) {
        ESP_LOGE(TAG, "SD card not available");
        return ESP_ERR_NOT_FOUND;
    }
    
    int num = get_next_screenshot_number();
    snprintf(filename, size, "%s%03d%s", SCREENSHOT_PREFIX, num, SCREENSHOT_EXT);
    return ESP_OK;
}

/**
 * @brief Get full path for a screenshot
 */
esp_err_t screenshot_storage_get_path(const char *filename, char *path, size_t size)
{
    snprintf(path, size, "%s/%s", SCREENSHOT_DIR, filename);
    return ESP_OK;
}

/**
 * @brief Save screenshot data to SD card
 */
esp_err_t screenshot_storage_save(const uint8_t *data, int width, int height, const char *filename)
{
    if (!g_sd_available) {
        ESP_LOGE(TAG, "SD card not available");
        return ESP_ERR_NOT_FOUND;
    }
    
    if (data == NULL || filename == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(g_mutex, portMAX_DELAY);
    
    /* Construct full path */
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", SCREENSHOT_DIR, filename);

    ESP_LOGI(TAG, "Saving screenshot: %s (%dx%d)", filepath, width, height);

    /* Open file */
    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", strerror(errno));
        xSemaphoreGive(g_mutex);
        return ESP_FAIL;
    }

    /* Calculate sizes */
    int row_size = ((width * 3 + 3) / 4) * 4;  /* Row size must be multiple of 4 */
    int image_size = row_size * height;
    int file_size = 54 + image_size;  /* BMP header + image data */

    /* Write BMP header */
    uint8_t bmp_header[54] = {
        /* BMP file header (14 bytes) */
        'B', 'M',                           /* Signature */
        file_size & 0xFF,                   /* File size */
        (file_size >> 8) & 0xFF,
        (file_size >> 16) & 0xFF,
        (file_size >> 24) & 0xFF,
        0, 0, 0, 0,                         /* Reserved */
        54, 0, 0, 0,                        /* Data offset */
        
        /* DIB header (40 bytes) */
        40, 0, 0, 0,                        /* Header size */
        width & 0xFF,                       /* Width */
        (width >> 8) & 0xFF,
        (width >> 16) & 0xFF,
        (width >> 24) & 0xFF,
        height & 0xFF,                      /* Height (positive = bottom-up) */
        (height >> 8) & 0xFF,
        (height >> 16) & 0xFF,
        (height >> 24) & 0xFF,
        1, 0,                               /* Planes */
        24, 0,                              /* Bits per pixel */
        0, 0, 0, 0,                         /* Compression (none) */
        image_size & 0xFF,                  /* Image size */
        (image_size >> 8) & 0xFF,
        (image_size >> 16) & 0xFF,
        (image_size >> 24) & 0xFF,
        0x13, 0x0B, 0, 0,                   /* X pixels per meter */
        0x13, 0x0B, 0, 0,                   /* Y pixels per meter */
        0, 0, 0, 0,                         /* Colors in color table */
        0, 0, 0, 0,                         /* Important colors */
    };

    fwrite(bmp_header, 1, 54, f);

    /* Write image data (bottom-up, BGR format) */
    uint8_t *row_buffer = malloc(row_size);
    if (row_buffer == NULL) {
        fclose(f);
        xSemaphoreGive(g_mutex);
        return ESP_ERR_NO_MEM;
    }

    for (int y = height - 1; y >= 0; y--) {
        const uint8_t *src_row = data + y * width * 3;
        
        /* Convert RGB to BGR */
        for (int x = 0; x < width; x++) {
            row_buffer[x * 3 + 0] = src_row[x * 3 + 2];  /* B */
            row_buffer[x * 3 + 1] = src_row[x * 3 + 1];  /* G */
            row_buffer[x * 3 + 2] = src_row[x * 3 + 0];  /* R */
        }
        
        /* Pad row to multiple of 4 bytes */
        for (int i = width * 3; i < row_size; i++) {
            row_buffer[i] = 0;
        }
        
        fwrite(row_buffer, 1, row_size, f);
    }

    free(row_buffer);
    fclose(f);

    /* Update cache */
    if (g_cache_count < MAX_CACHED_SCREENSHOTS) {
        strncpy(g_screenshot_cache[g_cache_count].filename, filename,
                sizeof(g_screenshot_cache[g_cache_count].filename) - 1);
        g_screenshot_cache[g_cache_count].size = file_size;
        g_screenshot_cache[g_cache_count].timestamp = time(NULL);
        g_screenshot_cache[g_cache_count].location = STORAGE_SD;
        g_cache_count++;
    }
    
    g_screenshot_count++;
    
    xSemaphoreGive(g_mutex);
    
    ESP_LOGI(TAG, "Screenshot saved: %s (%d bytes)", filename, file_size);
    return ESP_OK;
}

/**
 * @brief Delete a screenshot
 */
esp_err_t screenshot_storage_delete(const char *filename)
{
    if (!g_sd_available) {
        return ESP_ERR_NOT_FOUND;
    }
    
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", SCREENSHOT_DIR, filename);

    if (remove(filepath) != 0) {
        ESP_LOGE(TAG, "Failed to delete: %s", strerror(errno));
        xSemaphoreGive(g_mutex);
        return ESP_FAIL;
    }
    
    /* Remove from cache */
    for (int i = 0; i < g_cache_count; i++) {
        if (strcmp(g_screenshot_cache[i].filename, filename) == 0) {
            /* Shift remaining entries */
            for (int j = i; j < g_cache_count - 1; j++) {
                g_screenshot_cache[j] = g_screenshot_cache[j + 1];
            }
            g_cache_count--;
            break;
        }
    }
    
    g_screenshot_count--;
    
    xSemaphoreGive(g_mutex);
    
    ESP_LOGI(TAG, "Screenshot deleted: %s", filename);
    return ESP_OK;
}

/**
 * @brief Get screenshot count
 */
int screenshot_storage_get_count(void)
{
    return g_cache_count;
}

/**
 * @brief Get screenshot info by index
 */
esp_err_t screenshot_storage_get_info(int index, screenshot_storage_info_t *info)
{
    if (index < 0 || index >= g_cache_count || info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *info = g_screenshot_cache[index];
        return ESP_OK;
    }

/**
 * @brief Refresh screenshot list
 */
esp_err_t screenshot_storage_refresh(void)
{
    if (!g_sd_available) {
        return ESP_ERR_NOT_FOUND;
    }
    
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    scan_screenshots();
    xSemaphoreGive(g_mutex);
    
    return ESP_OK;
}

/**
 * @brief Check if SD card is available
 */
bool screenshot_storage_is_available(void)
{
    return g_sd_available;
}

/**
 * @brief Get screenshot directory path
 */
const char* screenshot_storage_get_dir(void)
{
    return SCREENSHOT_DIR;
}
