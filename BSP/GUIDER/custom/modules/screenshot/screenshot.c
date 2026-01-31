/**
 * @file screenshot.c
 * @brief Screenshot Module - SD Card Only
 * 
 * Takes screenshots and saves them directly to SD card.
 * Triggered by three-finger swipe gesture.
 */

#include "screenshot.h"
#include "screenshot_storage.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_touch.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

static const char *TAG = "SCREENSHOT";

/* LVGL port touch context structure (from esp_lvgl_port) */
typedef struct {
    esp_lcd_touch_handle_t   handle;
    lv_indev_drv_t           indev_drv;
    struct {
        float x;
        float y;
    } scale;
} lvgl_port_touch_ctx_t;

/* Gesture detection parameters */
#define GESTURE_MIN_FINGERS         3
#define GESTURE_SWIPE_THRESHOLD     30
#define GESTURE_TIMEOUT_MS          2000
#define GESTURE_HORIZONTAL_TOLERANCE 300

/* Gesture state tracking */
typedef struct {
    bool gesture_active;
    uint32_t start_time;
    int16_t start_y[5];
    int16_t start_x[5];
    uint8_t finger_count;
    bool screenshot_triggered;
} gesture_state_t;

static gesture_state_t g_gesture_state = {0};
static bool g_initialized = false;
static lv_indev_t *g_touch_indev = NULL;
static esp_lcd_touch_handle_t g_touch_handle = NULL;
static lv_timer_t *g_touch_poll_timer = NULL;

/* Forward declarations */
static void touch_poll_timer_cb(lv_timer_t *timer);
static uint8_t get_touch_point_count(uint16_t *x_arr, uint16_t *y_arr, uint8_t max_points);
static void show_screenshot_toast(bool success, const char *filename);

/**
 * @brief Initialize screenshot module
 */
esp_err_t screenshot_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing screenshot module...");

    /* Initialize screenshot storage */
    esp_err_t ret = screenshot_storage_init();
        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize screenshot storage");
        return ret;
    }

    if (!screenshot_storage_is_available()) {
        ESP_LOGW(TAG, "SD card not available - screenshots disabled");
        /* Continue initialization for gesture detection, but screenshots won't save */
    }

    /* Get the touch input device */
    g_touch_indev = lv_indev_get_next(NULL);
    while (g_touch_indev != NULL) {
        if (lv_indev_get_type(g_touch_indev) == LV_INDEV_TYPE_POINTER) {
            break;
        }
        g_touch_indev = lv_indev_get_next(g_touch_indev);
    }

    if (g_touch_indev == NULL) {
        ESP_LOGE(TAG, "No touch input device found");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Touch input device found: %p", g_touch_indev);

    /* Get GT911 touch handle for multi-touch support */
    if (g_touch_indev->driver != NULL && g_touch_indev->driver->user_data != NULL) {
        lvgl_port_touch_ctx_t *touch_ctx = (lvgl_port_touch_ctx_t *)g_touch_indev->driver->user_data;
        g_touch_handle = touch_ctx->handle;
        ESP_LOGI(TAG, "GT911 touch handle obtained: %p", g_touch_handle);
    }

    /* Create touch polling timer */
    g_touch_poll_timer = lv_timer_create(touch_poll_timer_cb, 50, NULL);
    if (g_touch_poll_timer != NULL) {
        ESP_LOGI(TAG, "Touch poll timer created (50ms interval)");
    }

    g_initialized = true;
    ESP_LOGI(TAG, "Screenshot module initialized successfully");
    ESP_LOGI(TAG, "Gesture: Three-finger swipe down to take screenshot");
    
    if (screenshot_storage_is_available()) {
        ESP_LOGI(TAG, "Storage: SD Card (%s)", screenshot_storage_get_dir());
    }

    return ESP_OK;
}

/**
 * @brief Deinitialize screenshot module
 */
esp_err_t screenshot_deinit(void)
{
    if (!g_initialized) {
    return ESP_OK;
}

    if (g_touch_poll_timer != NULL) {
        lv_timer_del(g_touch_poll_timer);
        g_touch_poll_timer = NULL;
    }

    screenshot_storage_deinit();
    g_initialized = false;

    ESP_LOGI(TAG, "Screenshot module deinitialized");
    return ESP_OK;
}

/**
 * @brief Get multi-touch points from GT911
 */
static uint8_t get_touch_point_count(uint16_t *x_arr, uint16_t *y_arr, uint8_t max_points)
{
    if (g_touch_handle == NULL) {
        return 0;
    }

        uint8_t point_count = 0;
    uint16_t strength[5];  /* API requires uint16_t* */

        esp_lcd_touch_read_data(g_touch_handle);
    esp_lcd_touch_get_coordinates(g_touch_handle, x_arr, y_arr, strength, &point_count, max_points);

            return point_count;
        }

/**
 * @brief Touch polling timer callback for gesture detection
 */
static void touch_poll_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (g_touch_handle == NULL) {
        return;
    }

    uint16_t x[5], y[5];
    uint8_t point_count = get_touch_point_count(x, y, 5);
    uint32_t now = lv_tick_get();

    /* Reset gesture if fingers lifted or timeout */
    if (point_count < GESTURE_MIN_FINGERS) {
        if (g_gesture_state.gesture_active) {
            g_gesture_state.gesture_active = false;
            g_gesture_state.screenshot_triggered = false;
        }
        return;
    }

    /* Handle gesture timeout */
    if (g_gesture_state.gesture_active &&
        (now - g_gesture_state.start_time) > GESTURE_TIMEOUT_MS) {
        g_gesture_state.gesture_active = false;
        g_gesture_state.screenshot_triggered = false;
        return;
    }

    /* Start new gesture */
    if (!g_gesture_state.gesture_active) {
        g_gesture_state.gesture_active = true;
        g_gesture_state.start_time = now;
        g_gesture_state.finger_count = point_count;
        g_gesture_state.screenshot_triggered = false;

        for (int i = 0; i < point_count && i < 5; i++) {
            g_gesture_state.start_x[i] = x[i];
            g_gesture_state.start_y[i] = y[i];
        }
            return;
        }

    /* Check for swipe gesture */
    if (g_gesture_state.gesture_active && !g_gesture_state.screenshot_triggered) {
        int32_t total_abs_delta_x = 0, total_abs_delta_y = 0;

        for (int i = 0; i < point_count && i < 5; i++) {
            int32_t dx = x[i] - g_gesture_state.start_x[i];
            int32_t dy = y[i] - g_gesture_state.start_y[i];
            total_abs_delta_x += abs(dx);
            total_abs_delta_y += abs(dy);
        }

        int32_t avg_abs_delta_x = total_abs_delta_x / point_count;
        int32_t avg_abs_delta_y = total_abs_delta_y / point_count;

        /* Detect swipe */
        if (avg_abs_delta_x > GESTURE_SWIPE_THRESHOLD || 
            avg_abs_delta_y > GESTURE_SWIPE_THRESHOLD) {
            ESP_LOGI(TAG, "Three-finger swipe detected!");
            g_gesture_state.screenshot_triggered = true;
            screenshot_take(NULL);
        }
    }
}

/**
 * @brief Show toast notification
 */
static void show_screenshot_toast(bool success, const char *filename)
{
    lv_obj_t *screen = lv_scr_act();
    
    /* Create toast container */
    lv_obj_t *toast = lv_obj_create(screen);
    lv_obj_set_size(toast, 350, 50);
    lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(toast, 25, 0);
    lv_obj_set_style_border_width(toast, 0, 0);
    lv_obj_set_style_pad_all(toast, 10, 0);
    lv_obj_move_foreground(toast);

    if (success) {
        lv_obj_set_style_bg_color(toast, lv_color_hex(0x4CAF50), 0);  /* Green */
    } else {
        lv_obj_set_style_bg_color(toast, lv_color_hex(0xF44336), 0);  /* Red */
    }
    lv_obj_set_style_bg_opa(toast, LV_OPA_90, 0);

    /* Create label */
    lv_obj_t *label = lv_label_create(toast);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);

    if (success) {
        char msg[128];
        int count = screenshot_storage_get_count();
        snprintf(msg, sizeof(msg), LV_SYMBOL_OK " Saved: %s (%d total)", filename, count);
            lv_label_set_text(label, msg);
    } else {
        lv_label_set_text(label, LV_SYMBOL_CLOSE " Screenshot Failed - No SD Card");
        }

    /* Auto-delete toast */
    lv_obj_del_delayed(toast, 2000);
}

/**
 * @brief Take a screenshot
 * @note This captures the entire display including all layers (active screen + top layer + sys layer)
 */
esp_err_t screenshot_take(lv_obj_t *screen)
{
    /* Check if storage is available */
    if (!screenshot_storage_is_available()) {
        ESP_LOGE(TAG, "SD card not available");
        show_screenshot_toast(false, NULL);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Taking screenshot...");

    /* Generate filename */
    char filename[64];
    esp_err_t ret = screenshot_storage_generate_filename(filename, sizeof(filename));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate filename");
        show_screenshot_toast(false, NULL);
        return ret;
    }

    /* Get display */
    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "No display found");
        show_screenshot_toast(false, NULL);
        return ESP_ERR_NOT_FOUND;
    }

    /* 
     * To capture popups and overlays, we need to snapshot the entire display.
     * LVGL has multiple layers:
     * 1. Active screen (lv_scr_act) - main content
     * 2. Top layer (lv_layer_top) - popups, dialogs
     * 3. Sys layer (lv_layer_sys) - system overlays
     * 
     * lv_snapshot_take() only captures a single object and its children.
     * To capture everything, we use lv_snapshot_take on the active screen,
     * but we need to ensure popups created on lv_scr_act() are included.
     * 
     * For popups on lv_layer_top(), we need a different approach:
     * Use lv_disp_get_layer_top() and composite the layers.
     * 
     * Simpler approach: Use lv_snapshot_take_to_buf with the display's draw buffer
     * or capture the framebuffer directly.
     * 
     * Best approach for LVGL v8: Snapshot the active screen which includes
     * all children (including popups created as children of lv_scr_act()).
     * For lv_layer_top() popups, we composite them manually.
     */
    
    lv_obj_t *act_scr = lv_scr_act();
    lv_obj_t *top_layer = lv_disp_get_layer_top(disp);
    
    /* Get display dimensions */
    lv_coord_t disp_w = lv_disp_get_hor_res(disp);
    lv_coord_t disp_h = lv_disp_get_ver_res(disp);
    
    ESP_LOGI(TAG, "Display size: %dx%d", disp_w, disp_h);

    /* First, take snapshot of active screen (includes children like SD card manager UI) */
    lv_img_dsc_t *scr_snapshot = lv_snapshot_take(act_scr, LV_IMG_CF_TRUE_COLOR);
    if (scr_snapshot == NULL) {
        ESP_LOGE(TAG, "Failed to take screen snapshot");
        show_screenshot_toast(false, NULL);
        return ESP_ERR_NO_MEM;
    }

    int width = scr_snapshot->header.w;
    int height = scr_snapshot->header.h;
    size_t rgb_size = width * height * 3;
    
    uint8_t *rgb_data = heap_caps_malloc(rgb_size, MALLOC_CAP_SPIRAM);
    if (rgb_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RGB buffer");
        lv_snapshot_free(scr_snapshot);
        show_screenshot_toast(false, NULL);
        return ESP_ERR_NO_MEM;
    }

    /* Convert RGB565 to RGB888 */
    const uint16_t *src = (const uint16_t *)scr_snapshot->data;
    for (int i = 0; i < width * height; i++) {
        uint16_t pixel = src[i];
        uint8_t r = ((pixel >> 11) & 0x1F) << 3;
        uint8_t g = ((pixel >> 5) & 0x3F) << 2;
        uint8_t b = (pixel & 0x1F) << 3;
        rgb_data[i * 3 + 0] = r;
        rgb_data[i * 3 + 1] = g;
        rgb_data[i * 3 + 2] = b;
    }

    lv_snapshot_free(scr_snapshot);

    /* Check if top layer has visible children (popups on lv_layer_top) */
    uint32_t top_child_cnt = lv_obj_get_child_cnt(top_layer);
    if (top_child_cnt > 0) {
        ESP_LOGI(TAG, "Top layer has %lu children, compositing...", (unsigned long)top_child_cnt);
        
        /* Take snapshot of top layer */
        lv_img_dsc_t *top_snapshot = lv_snapshot_take(top_layer, LV_IMG_CF_TRUE_COLOR);
        if (top_snapshot != NULL) {
            /* Composite top layer onto screen snapshot */
            const uint16_t *top_src = (const uint16_t *)top_snapshot->data;
            int top_w = top_snapshot->header.w;
            int top_h = top_snapshot->header.h;
            
            /* Simple alpha blending - top layer pixels that are not transparent */
            for (int y = 0; y < top_h && y < height; y++) {
                for (int x = 0; x < top_w && x < width; x++) {
                    int idx = y * top_w + x;
                    uint16_t pixel = top_src[idx];
                    
                    /* Skip fully transparent pixels (black with 0 alpha in RGB565 is 0x0000) */
                    /* For simplicity, we check if pixel is not the default background */
                    /* A more accurate approach would use ARGB8888 format */
                    if (pixel != 0x0000) {
                        uint8_t r = ((pixel >> 11) & 0x1F) << 3;
                        uint8_t g = ((pixel >> 5) & 0x3F) << 2;
                        uint8_t b = (pixel & 0x1F) << 3;
                        
                        int dst_idx = (y * width + x) * 3;
                        rgb_data[dst_idx + 0] = r;
                        rgb_data[dst_idx + 1] = g;
                        rgb_data[dst_idx + 2] = b;
                    }
                }
            }
            
            lv_snapshot_free(top_snapshot);
            ESP_LOGI(TAG, "Top layer composited");
        }
    }

    /* Show flash effect */
    lv_obj_t *flash = lv_obj_create(lv_scr_act());
    if (flash != NULL) {
        lv_obj_set_size(flash, LV_PCT(100), LV_PCT(100));
        lv_obj_set_pos(flash, 0, 0);
        lv_obj_clear_flag(flash, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(flash, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(flash, LV_OPA_60, 0);
        lv_obj_set_style_border_width(flash, 0, 0);
        lv_obj_move_foreground(flash);
        lv_obj_del_delayed(flash, 100);
    }

    /* Save to SD card */
    ret = screenshot_storage_save(rgb_data, width, height, filename);
    heap_caps_free(rgb_data);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Screenshot saved: %s", filename);
        show_screenshot_toast(true, filename);
    } else {
        ESP_LOGE(TAG, "Failed to save screenshot");
        show_screenshot_toast(false, NULL);
        }

        return ret;
    }

/**
 * @brief Get last screenshot path
 */
const char* screenshot_get_last_path(void)
{
    static char path[256];
    int count = screenshot_storage_get_count();
    
    if (count > 0) {
        screenshot_storage_info_t info;
        if (screenshot_storage_get_info(count - 1, &info) == ESP_OK) {
            screenshot_storage_get_path(info.filename, path, sizeof(path));
            return path;
        }
    }
    
    return NULL;
}

/**
 * @brief Check if SD card is available for screenshots
 */
bool screenshot_is_sd_available(void)
{
    return screenshot_storage_is_available();
}
