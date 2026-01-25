#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_heap_caps.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "lvgl.h"
#include "gui_guider.h"
#include "custom.h"
#include "boot_animation.h"
#include "screenshot_storage.h"
#include "screenshot.h"

/* Time synchronization via OneNet cloud platform */
#include <time.h>
#include <sys/time.h>

/* WiFi management */
#include "wifi_manager.h"

static const char *TAG = "main";

// 定义全局 UI 对象
lv_ui guider_ui;

/* Time synchronization is handled by OneNet cloud platform */

void app_main(void)
{
    /* Initialize NVS (required for WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* === WIFI INITIALIZATION === */

    /* WiFi and OneNet will be initialized in custom_init() */
    /* This allows proper integration with LVGL UI */
    ESP_LOGI(TAG, "WiFi and OneNet initialization will be done in UI init");

    /* === TIME SYNCHRONIZATION === */

    /* Time sync will be handled by SNTP */
    ESP_LOGI(TAG, "Time synchronization will be done via SNTP");
    
    /* === DISPLAY INITIALIZATION === */
    
    /* Optimized LVGL port configuration for high performance and low CPU usage */
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_priority = 3;           /* Lower priority (was 4) to reduce CPU load, still responsive */
    lvgl_cfg.task_stack = 8192;           /* Increase stack (was 7168) for full-screen buffer handling */
    lvgl_cfg.task_affinity = 1;           /* Pin to Core 1 (main app on Core 0) for better parallelism */
    lvgl_cfg.task_max_sleep_ms = 10;      /* Reduce sleep (was 500) for higher frame rate */
    lvgl_cfg.timer_period_ms = 2;         /* Reduce tick period (was 5) for smoother animations */
    lvgl_cfg.task_stack_caps = MALLOC_CAP_SPIRAM;  /* Use PSRAM for task stack to save internal RAM */

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = lvgl_cfg,
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES,  /* Full screen: 480*800 = 384000 pixels = 768KB per buffer */
        .double_buffer = true,  /* Double buffering: 2 x 768KB = 1.5MB in PSRAM */
        .flags = {
            .buff_dma = false,     /* Don't set DMA flag (LVGL v8 limitation with PSRAM) */
            .buff_spiram = true,   /* Use PSRAM (32MB >> 1.5MB, plenty of space) */
            .sw_rotate = true,     /* Enable PPA hardware rotation (PPA can access PSRAM directly) */
        }
    };

    lv_display_t *disp = bsp_display_start_with_config(&cfg);

    /* Keep backlight off during initialization to avoid white screen */
    bsp_display_backlight_off();

    bsp_display_lock(0);

    /* Rotate display from portrait (480x800) to landscape (800x480) using PPA hardware acceleration */
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_90);

    /* === BOOT ANIMATION === */
    /* Create and display boot animation with school logo */
    boot_animation_create(disp);

    /* Force LVGL to render the boot animation */
    lv_refr_now(disp);

    bsp_display_unlock();

    /* Wait for rendering to complete */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Now turn on backlight with low brightness (1%) - boot logo is ready and visible */
    bsp_display_brightness_set(1);
    bsp_display_backlight_on();

    ESP_LOGI(TAG, "Boot logo displayed at 1%% brightness, preparing main UI...");

    /* Mount SD card for screenshots and data storage */
    ESP_LOGI(TAG, "Mounting SD card...");
    esp_err_t sd_ret = bsp_sdcard_mount();
    if (sd_ret == ESP_OK) {
        ESP_LOGI(TAG, "SD card mounted successfully at /sdcard");
    } else {
        ESP_LOGW(TAG, "SD card mount failed: %s", esp_err_to_name(sd_ret));
        ESP_LOGW(TAG, "Screenshots will not be available");
    }

    /* Pre-initialize SD card storage for screenshots. */
    ESP_LOGI(TAG, "Pre-initializing screenshot storage (SD Card)...");
    screenshot_storage_init();
    ESP_LOGI(TAG, "Screenshot storage initialized");

    /* Initialize screenshot gesture detection (three-finger swipe) */
    ESP_LOGI(TAG, "Initializing screenshot gesture detection...");
    screenshot_init();
    ESP_LOGI(TAG, "Screenshot gesture detection initialized");

    /* Setup main UI in background while boot logo is showing */
    bsp_display_lock(0);
    setup_ui(&guider_ui);
    custom_init(&guider_ui);
    bsp_display_unlock();

    ESP_LOGI(TAG, "Main UI prepared, displaying boot logo for 3 seconds...");

    /* Display boot logo for 3 seconds */
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Transition to main UI */
    bsp_display_lock(0);
    /* Directly set scrHome as active screen, bypassing lv_scr_load which can crash */
    if (disp != NULL && guider_ui.scrHome != NULL) {
        disp->act_scr = guider_ui.scrHome;
        disp->scr_to_load = NULL;
        disp->prev_scr = NULL;
        disp->del_prev = false;
        lv_obj_invalidate(guider_ui.scrHome);
        lv_event_send(guider_ui.scrHome, LV_EVENT_SCREEN_LOADED, NULL);
    }
    boot_animation_delete();
    bsp_display_unlock();

    /* Increase backlight brightness to 50% for main UI */
    bsp_display_brightness_set(50);

    ESP_LOGI(TAG, "Application started successfully! Backlight set to 50%%");

    /* Print memory info */
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG, "========== Memory Info ==========");
    ESP_LOGI(TAG, "Internal RAM - Free: %u KB, Total: %u KB",
             (unsigned)(internal_free / 1024), (unsigned)(internal_total / 1024));
    ESP_LOGI(TAG, "PSRAM - Free: %u KB (%.1f MB), Total: %u KB (%.1f MB)",
             (unsigned)(psram_free / 1024), psram_free / 1024.0 / 1024.0,
             (unsigned)(psram_total / 1024), psram_total / 1024.0 / 1024.0);
    ESP_LOGI(TAG, "PSRAM Usage: %.1f%%",
             100.0 * (1.0 - (float)psram_free / psram_total));
    ESP_LOGI(TAG, "=================================");
}
