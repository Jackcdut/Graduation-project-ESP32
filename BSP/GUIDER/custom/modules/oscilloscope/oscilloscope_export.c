/**
 * @file oscilloscope_export.c
 * @brief Oscilloscope Data Export - SD Card
 * 
 * Exports oscilloscope waveform data to SD card as CSV files.
 * Naming convention: Oscilloscope_001.csv, Oscilloscope_002.csv, etc.
 */

#include "oscilloscope_export.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "bsp/esp-bsp.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

static const char *TAG = "OSC_EXPORT";

/* SD Card paths */
#define SD_MOUNT_POINT      "/sdcard"
#define OSCILLOSCOPE_DIR    "/sdcard/Oscilloscope"
#define OSC_PREFIX          "Oscilloscope_"
#define OSC_EXT             ".csv"

/* State variables */
static bool g_initialized = false;
static bool g_sd_available = false;
static int g_next_number = 1;
static uint32_t g_export_counter = 0;

/* Waveform data storage */
static osc_waveform_data_t *g_waveform_data = NULL;
static osc_waveform_data_t *g_fft_data = NULL;
static bool g_has_waveform = false;
static bool g_has_fft = false;
static osc_export_format_t g_export_format = OSC_EXPORT_FORMAT_CSV;

/* Forward declarations */
static esp_err_t ensure_osc_dir(void);
static esp_err_t scan_osc_files(void);
static void get_current_timestamp(char *buffer, size_t buffer_size);

esp_err_t osc_export_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing oscilloscope export module...");

    /* Allocate waveform data storage */
    g_waveform_data = heap_caps_malloc(sizeof(osc_waveform_data_t), MALLOC_CAP_SPIRAM);
    if (g_waveform_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate waveform data storage");
        return ESP_ERR_NO_MEM;
    }

    g_fft_data = heap_caps_malloc(sizeof(osc_waveform_data_t), MALLOC_CAP_SPIRAM);
    if (g_fft_data == NULL) {
        heap_caps_free(g_waveform_data);
        g_waveform_data = NULL;
        ESP_LOGE(TAG, "Failed to allocate FFT data storage");
        return ESP_ERR_NO_MEM;
    }

    /* Check if SD card is available */
    struct stat st;
    if (stat(SD_MOUNT_POINT, &st) == 0) {
        g_sd_available = true;
        ESP_LOGI(TAG, "SD card available");
        
        ensure_osc_dir();
        scan_osc_files();
    } else {
        g_sd_available = false;
        ESP_LOGW(TAG, "SD card not available - exports disabled");
    }

    g_has_waveform = false;
    g_has_fft = false;

    g_initialized = true;
    ESP_LOGI(TAG, "Oscilloscope export module initialized");
    return ESP_OK;
}

esp_err_t osc_export_deinit(void)
{
    if (g_waveform_data != NULL) {
        heap_caps_free(g_waveform_data);
        g_waveform_data = NULL;
    }

    if (g_fft_data != NULL) {
        heap_caps_free(g_fft_data);
        g_fft_data = NULL;
    }

    g_has_waveform = false;
    g_has_fft = false;
    g_initialized = false;

    ESP_LOGI(TAG, "Oscilloscope export module deinitialized");
    return ESP_OK;
}

/**
 * @brief Ensure oscilloscope directory exists
 */
static esp_err_t ensure_osc_dir(void)
{
    struct stat st;
    if (stat(OSCILLOSCOPE_DIR, &st) != 0) {
        ESP_LOGI(TAG, "Creating oscilloscope directory: %s", OSCILLOSCOPE_DIR);
        if (mkdir(OSCILLOSCOPE_DIR, 0755) != 0) {
            ESP_LOGE(TAG, "Failed to create directory: %s", strerror(errno));
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

/**
 * @brief Scan existing files to find next number
 */
static esp_err_t scan_osc_files(void)
{
    DIR *dir = opendir(OSCILLOSCOPE_DIR);
    if (dir == NULL) {
        return ESP_OK;
    }

    int max_number = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, OSC_PREFIX, strlen(OSC_PREFIX)) == 0) {
            int num = 0;
            sscanf(entry->d_name, OSC_PREFIX "%d", &num);
            if (num > max_number) {
                max_number = num;
            }
        }
    }

    closedir(dir);
    g_next_number = max_number + 1;
    
    ESP_LOGI(TAG, "Next oscilloscope file number: %d", g_next_number);
    return ESP_OK;
}

/**
 * @brief Get current timestamp
 */
static void get_current_timestamp(char *buffer, size_t buffer_size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    if (t != NULL) {
        snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        snprintf(buffer, buffer_size, "Unknown");
    }
}

esp_err_t osc_export_store_waveform(const osc_waveform_data_t *waveform)
{
    if (waveform == NULL || g_waveform_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(g_waveform_data, waveform, sizeof(osc_waveform_data_t));
    g_has_waveform = true;

    ESP_LOGI(TAG, "Waveform data stored: %d points, %.2f Hz", 
             waveform->num_points, waveform->frequency);
    return ESP_OK;
}

esp_err_t osc_export_store_fft(const osc_waveform_data_t *fft_data)
{
    if (fft_data == NULL || g_fft_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(g_fft_data, fft_data, sizeof(osc_waveform_data_t));
    g_has_fft = true;

    ESP_LOGI(TAG, "FFT data stored: %d points", fft_data->num_points);
    return ESP_OK;
}

/**
 * @brief Save waveform data to SD card
 */
esp_err_t osc_export_save_to_sd(void)
{
    if (!g_sd_available) {
        ESP_LOGE(TAG, "SD card not available");
        return ESP_ERR_NOT_FOUND;
    }

    if (!g_has_waveform && !g_has_fft) {
        ESP_LOGW(TAG, "No data to export");
        return ESP_ERR_INVALID_STATE;
    }

    ensure_osc_dir();

    char timestamp[32];
    get_current_timestamp(timestamp, sizeof(timestamp));

    esp_err_t ret = ESP_OK;

    /* Save waveform data */
    if (g_has_waveform) {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/%s%03d%s", 
                 OSCILLOSCOPE_DIR, OSC_PREFIX, g_next_number, OSC_EXT);

        FILE *f = fopen(filename, "w");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to create file: %s", strerror(errno));
        return ESP_FAIL;
    }

        /* Write CSV header */
        fprintf(f, "# Oscilloscope Waveform Data\n");
        fprintf(f, "# Timestamp: %s\n", timestamp);
        fprintf(f, "# Frequency: %.2f Hz\n", g_waveform_data->frequency);
        fprintf(f, "# Vmax: %.3f V\n", g_waveform_data->vmax);
        fprintf(f, "# Vmin: %.3f V\n", g_waveform_data->vmin);
        fprintf(f, "# Vpp: %.3f V\n", g_waveform_data->vpp);
        fprintf(f, "# Vrms: %.3f V\n", g_waveform_data->vrms);
        fprintf(f, "# Time Scale: %.6f s/div\n", g_waveform_data->time_scale);
        fprintf(f, "# Volt Scale: %.3f V/div\n", g_waveform_data->volt_scale);
        fprintf(f, "# Points: %d\n", g_waveform_data->num_points);
        fprintf(f, "#\n");
        fprintf(f, "Index,Time(s),Voltage(V)\n");

        /* Write data points */
        float time_per_point = g_waveform_data->time_scale * 10.0f / g_waveform_data->num_points;
        for (int i = 0; i < g_waveform_data->num_points; i++) {
            fprintf(f, "%d,%.9f,%.6f\n", i, i * time_per_point, g_waveform_data->data[i]);
        }

        fclose(f);
        ESP_LOGI(TAG, "Waveform saved: %s", filename);
        g_next_number++;
    g_export_counter++;
    }

    /* Save FFT data */
    if (g_has_fft) {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/%s%03d_FFT%s", 
                 OSCILLOSCOPE_DIR, OSC_PREFIX, g_next_number, OSC_EXT);

        FILE *f = fopen(filename, "w");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to create FFT file: %s", strerror(errno));
            return ESP_FAIL;
        }

        /* Write CSV header */
        fprintf(f, "# Oscilloscope FFT Data\n");
        fprintf(f, "# Timestamp: %s\n", timestamp);
        fprintf(f, "# Points: %d\n", g_fft_data->num_points);
        fprintf(f, "#\n");
        fprintf(f, "Index,Frequency(Hz),Magnitude(dB)\n");

        /* Write data points */
        float freq_per_point = 1.0f / (g_fft_data->time_scale * 2.0f);
        for (int i = 0; i < g_fft_data->num_points; i++) {
            fprintf(f, "%d,%.2f,%.3f\n", i, i * freq_per_point, g_fft_data->data[i]);
        }

        fclose(f);
        ESP_LOGI(TAG, "FFT saved: %s", filename);
        g_next_number++;
        g_export_counter++;
    }

    return ret;
}

/* Legacy functions for compatibility */
esp_err_t osc_export_start_usb(void)
{
    ESP_LOGI(TAG, "USB export deprecated - use osc_export_save_to_sd()");
    return osc_export_save_to_sd();
}

esp_err_t osc_export_stop_usb(void)
{
    return ESP_OK;
}

bool osc_export_is_usb_active(void)
{
    return false;
}

esp_err_t osc_export_clear_data(void)
{
    g_has_waveform = false;
    g_has_fft = false;

    if (g_waveform_data != NULL) {
        memset(g_waveform_data, 0, sizeof(osc_waveform_data_t));
    }

    if (g_fft_data != NULL) {
        memset(g_fft_data, 0, sizeof(osc_waveform_data_t));
    }

    ESP_LOGI(TAG, "All data cleared");
    return ESP_OK;
}

esp_err_t osc_export_set_format(osc_export_format_t format)
{
    g_export_format = format;
    return ESP_OK;
}

osc_export_format_t osc_export_get_format(void)
{
    return g_export_format;
}

uint32_t osc_export_get_counter(void)
{
    return g_export_counter;
}

esp_err_t osc_export_reset_counter(void)
{
    g_export_counter = 0;
    g_next_number = 1;
    return ESP_OK;
}

esp_err_t osc_export_get_timestamp(char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    get_current_timestamp(buffer, buffer_size);
    return ESP_OK;
}

bool osc_export_is_sd_available(void)
{
    return g_sd_available;
    }

const char* osc_export_get_dir(void)
{
    return OSCILLOSCOPE_DIR;
}

/* Deprecated - no longer needed */
uint8_t* osc_export_get_disk_image(uint32_t *disk_size)
{
    if (disk_size) *disk_size = 0;
    return NULL;
}
