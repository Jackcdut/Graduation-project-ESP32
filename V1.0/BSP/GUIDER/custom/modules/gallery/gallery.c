/**
 * @file gallery.c
 * @brief Screenshot Gallery - SD Card Only
 *
 * Displays and manages screenshots stored on SD card.
 * Simplified: No more FLASH/PSRAM distinction. No USB MSC.
 */

#include "gallery.h"
#include "screenshot.h"
#include "screenshot_storage.h"
#include "sdcard_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "GALLERY";

/* Gallery state */
static lv_ui *g_ui = NULL;
static bool g_initialized = false;
static int g_current_image_index = -1;
static char g_current_filename[64] = {0};
static lv_color_t *g_image_buffer = NULL;

/* Forward declarations */
static void btn_gallery_item_clicked(lv_event_t *e);
static void btn_image_close_clicked(lv_event_t *e);
static void btn_image_delete_clicked(lv_event_t *e);
static void btn_image_prev_clicked(lv_event_t *e);
static void btn_image_next_clicked(lv_event_t *e);
static void update_gallery_count(void);

/**
 * @brief Initialize gallery module
 */
esp_err_t gallery_init(lv_ui *ui)
{
    if (ui == NULL) {
        ESP_LOGE(TAG, "UI pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    g_ui = ui;
    g_initialized = true;

    /* Initialize screenshot storage */
    screenshot_storage_init();

    /* Add event handlers for image viewer buttons */
    if (ui->scrSettings_btnImageClose) {
        lv_obj_add_event_cb(ui->scrSettings_btnImageClose, btn_image_close_clicked, LV_EVENT_CLICKED, NULL);
    }
    if (ui->scrSettings_btnImageDelete) {
        lv_obj_add_event_cb(ui->scrSettings_btnImageDelete, btn_image_delete_clicked, LV_EVENT_CLICKED, NULL);
    }
    if (ui->scrSettings_btnImagePrev) {
        lv_obj_add_event_cb(ui->scrSettings_btnImagePrev, btn_image_prev_clicked, LV_EVENT_CLICKED, NULL);
    }
    if (ui->scrSettings_btnImageNext) {
        lv_obj_add_event_cb(ui->scrSettings_btnImageNext, btn_image_next_clicked, LV_EVENT_CLICKED, NULL);
    }

    ESP_LOGI(TAG, "Gallery module initialized");
    return ESP_OK;
}

/**
 * @brief Deinitialize gallery module
 */
esp_err_t gallery_deinit(void)
{
    if (g_image_buffer != NULL) {
        heap_caps_free(g_image_buffer);
        g_image_buffer = NULL;
    }

    g_initialized = false;
    ESP_LOGI(TAG, "Gallery module deinitialized");
    return ESP_OK;
}

/**
 * @brief Update gallery count display
 */
static void update_gallery_count(void)
{
    if (!g_initialized || g_ui == NULL) return;

    int count = screenshot_storage_get_count();
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d screenshots", count);
    
    if (g_ui->scrSettings_labelGalleryCount) {
        lv_label_set_text(g_ui->scrSettings_labelGalleryCount, count_str);
    }
}

/**
 * @brief Refresh gallery list
 */
esp_err_t gallery_refresh(void)
{
    if (!g_initialized || g_ui == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Refresh storage */
    screenshot_storage_refresh();

    /* Clear existing list */
    if (g_ui->scrSettings_listGallery) {
        lv_obj_clean(g_ui->scrSettings_listGallery);

        /* Ensure scrolling is enabled for the list */
        lv_obj_set_scroll_dir(g_ui->scrSettings_listGallery, LV_DIR_VER);
        lv_obj_clear_flag(g_ui->scrSettings_listGallery, LV_OBJ_FLAG_SCROLL_ELASTIC);
        
        /* Show scrollbar when scrolling, hide when idle */
        lv_obj_set_scrollbar_mode(g_ui->scrSettings_listGallery, LV_SCROLLBAR_MODE_AUTO);
        
        /* Style scrollbar - use LVGL v8 compatible functions */
        lv_obj_set_style_width(g_ui->scrSettings_listGallery, 6, LV_PART_SCROLLBAR);
        lv_obj_set_style_bg_color(g_ui->scrSettings_listGallery, lv_color_hex(0x888888), LV_PART_SCROLLBAR);
        lv_obj_set_style_bg_opa(g_ui->scrSettings_listGallery, LV_OPA_50, LV_PART_SCROLLBAR);
    }

    /* Update count */
    update_gallery_count();

    int count = screenshot_storage_get_count();
    ESP_LOGI(TAG, "Refreshing gallery: %d screenshots", count);

    if (count == 0) {
        /* Show "No screenshots" message */
        if (g_ui->scrSettings_listGallery) {
            lv_obj_t *label = lv_label_create(g_ui->scrSettings_listGallery);
            lv_label_set_text(label, "No screenshots on SD card");
            lv_obj_set_style_text_color(label, lv_color_hex(0x888888), 0);
            lv_obj_set_style_pad_all(label, 20, 0);
        }
        return ESP_OK;
    }

    /* Add screenshots to list */
    for (int i = 0; i < count; i++) {
        screenshot_storage_info_t info;
        if (screenshot_storage_get_info(i, &info) == ESP_OK) {
            /* Create list button */
            char btn_text[128];
            snprintf(btn_text, sizeof(btn_text), LV_SYMBOL_IMAGE " %s", info.filename);

            lv_obj_t *btn = lv_list_add_btn(g_ui->scrSettings_listGallery, NULL, btn_text);
            
            /* Style button */
            lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xf5f5f5), LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xe0e0e0), LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_set_style_pad_ver(btn, 12, LV_PART_MAIN);  /* More padding for easier touch */

            /* Store index as user data */
            lv_obj_set_user_data(btn, (void *)(intptr_t)i);
            lv_obj_add_event_cb(btn, btn_gallery_item_clicked, LV_EVENT_CLICKED, NULL);
        }
    }

    /* Scroll to top */
    if (g_ui->scrSettings_listGallery) {
        lv_obj_scroll_to_y(g_ui->scrSettings_listGallery, 0, LV_ANIM_OFF);
    }

    ESP_LOGI(TAG, "Gallery refreshed with %d items", count);
    return ESP_OK;
}

/**
 * @brief Gallery item clicked
 */
static void btn_gallery_item_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(btn);
    
    ESP_LOGI(TAG, "Opening image index: %d", index);
    
    screenshot_storage_info_t info;
    if (screenshot_storage_get_info(index, &info) == ESP_OK) {
        g_current_image_index = index;
        snprintf(g_current_filename, sizeof(g_current_filename), "%s", info.filename);
        gallery_view_image(info.filename);
    }
}

/**
 * @brief View an image
 */
esp_err_t gallery_view_image(const char *filename)
{
    if (!g_initialized || g_ui == NULL || filename == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Viewing image: %s", filename);

    /* Get full path */
    char filepath[256];
    screenshot_storage_get_path(filename, filepath, sizeof(filepath));

    /* Open BMP file */
    FILE *f = fopen(filepath, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return ESP_ERR_NOT_FOUND;
    }

    /* Read BMP header */
    uint8_t header[54];
    fread(header, 1, 54, f);

    /* Parse dimensions */
    int width = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
    int height = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
    
    if (height < 0) height = -height;  /* Handle top-down BMPs */

    ESP_LOGI(TAG, "Image size: %dx%d", width, height);

    /* Allocate buffer for image */
    size_t buffer_size = width * height * sizeof(lv_color_t);
    
    if (g_image_buffer != NULL) {
        heap_caps_free(g_image_buffer);
    }
    
    g_image_buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (g_image_buffer == NULL) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to allocate image buffer");
        return ESP_ERR_NO_MEM;
    }

    /* Read and convert BMP data (bottom-up, BGR) to LVGL format */
    int row_size = ((width * 3 + 3) / 4) * 4;
    uint8_t *row_buffer = malloc(row_size);
    
    if (row_buffer == NULL) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    for (int y = height - 1; y >= 0; y--) {
        fread(row_buffer, 1, row_size, f);
        
        for (int x = 0; x < width; x++) {
            uint8_t b = row_buffer[x * 3 + 0];
            uint8_t g = row_buffer[x * 3 + 1];
            uint8_t r = row_buffer[x * 3 + 2];
            
            g_image_buffer[y * width + x] = lv_color_make(r, g, b);
        }
    }

    free(row_buffer);
    fclose(f);

    /* Show image viewer */
    if (g_ui->scrSettings_contImageViewer) {
        lv_obj_clear_flag(g_ui->scrSettings_contImageViewer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_ui->scrSettings_contImageViewer);
    }

    /* Set image */
    if (g_ui->scrSettings_imgViewer) {
        static lv_img_dsc_t img_dsc;
        img_dsc.header.always_zero = 0;
        img_dsc.header.w = width;
        img_dsc.header.h = height;
        img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
        img_dsc.data_size = buffer_size;
        img_dsc.data = (const uint8_t *)g_image_buffer;

        lv_img_set_src(g_ui->scrSettings_imgViewer, &img_dsc);
    }

    /* Update info label */
    if (g_ui->scrSettings_labelImageInfo) {
        int count = screenshot_storage_get_count();
        char info_str[64];
        snprintf(info_str, sizeof(info_str), "%d / %d", g_current_image_index + 1, count);
        lv_label_set_text(g_ui->scrSettings_labelImageInfo, info_str);
    }

    /* Record access in SD card manager */
    sdcard_manager_record_access(filepath);

    return ESP_OK;
}

/**
 * @brief Close image viewer
 */
esp_err_t gallery_close_viewer(void)
{
    if (!g_initialized || g_ui == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Hide viewer */
    if (g_ui->scrSettings_contImageViewer) {
        lv_obj_add_flag(g_ui->scrSettings_contImageViewer, LV_OBJ_FLAG_HIDDEN);
    }

    /* Free image buffer */
    if (g_image_buffer != NULL) {
        heap_caps_free(g_image_buffer);
        g_image_buffer = NULL;
    }

    g_current_image_index = -1;
    g_current_filename[0] = '\0';

    return ESP_OK;
}

/**
 * @brief Delete current image
 */
esp_err_t gallery_delete_current_image(void)
{
    if (!g_initialized || g_current_filename[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deleting: %s", g_current_filename);

    esp_err_t ret = screenshot_storage_delete(g_current_filename);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Image deleted");
    } else {
        ESP_LOGE(TAG, "Failed to delete image");
    }

    gallery_close_viewer();
    gallery_refresh();

    return ret;
}

/**
 * @brief Button callbacks
 */
static void btn_image_close_clicked(lv_event_t *e)
{
    (void)e;
    gallery_close_viewer();
}

static void btn_image_delete_clicked(lv_event_t *e)
{
    (void)e;
    gallery_delete_current_image();
}

static void btn_image_prev_clicked(lv_event_t *e)
{
    (void)e;
    int count = screenshot_storage_get_count();
    if (count == 0) return;

    g_current_image_index--;
    if (g_current_image_index < 0) {
        g_current_image_index = count - 1;
    }

    screenshot_storage_info_t info;
    if (screenshot_storage_get_info(g_current_image_index, &info) == ESP_OK) {
        snprintf(g_current_filename, sizeof(g_current_filename), "%s", info.filename);
        gallery_view_image(info.filename);
    }
}

static void btn_image_next_clicked(lv_event_t *e)
{
    (void)e;
    int count = screenshot_storage_get_count();
    if (count == 0) return;

    g_current_image_index++;
    if (g_current_image_index >= count) {
        g_current_image_index = 0;
    }

    screenshot_storage_info_t info;
    if (screenshot_storage_get_info(g_current_image_index, &info) == ESP_OK) {
        snprintf(g_current_filename, sizeof(g_current_filename), "%s", info.filename);
        gallery_view_image(info.filename);
    }
}

/**
 * @brief USB switch changed (legacy compatibility - now just refreshes)
 */
void gallery_usb_switch_changed(lv_ui *ui, bool enabled)
{
    (void)enabled;
    
    if (!g_initialized || ui == NULL) return;
    
    /* Just refresh gallery */
    gallery_refresh();
    
    /* Update USB status label - now just shows SD card info */
    if (ui->scrSettings_labelUSB) {
        int count = screenshot_storage_get_count();
        char label_text[48];
        snprintf(label_text, sizeof(label_text), LV_SYMBOL_SD_CARD " %d files on SD", count);
        lv_label_set_text(ui->scrSettings_labelUSB, label_text);
    }
}

/**
 * @brief Check if gallery is available
 */
bool gallery_is_available(void)
{
    return screenshot_storage_is_available();
}

/**
 * @brief Show USB mode dialog (stub - USB MSC removed)
 */
void gallery_show_usb_mode_dialog(void)
{
    ESP_LOGW(TAG, "USB MSC functionality removed - use SD card manager instead");
    /* USB MSC has been disabled due to SDIO conflict with ESP-Hosted WiFi */
    /* Users should access SD card files directly or use SD card manager UI */
}
