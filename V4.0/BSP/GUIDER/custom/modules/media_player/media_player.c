/**
 * @file media_player.c
 * @brief Media Player using Official AVI Player API
 * 
 * This implementation uses the official espressif__avi_player component
 * for fast, efficient video playback with minimal latency.
 */

#include "sdkconfig.h"
#include "media_player.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "bsp_board_extra.h"
#include "bsp/esp32_p4_function_ev_board.h"
#include "esp_lcd_panel_ops.h"
#include "driver/ppa.h"
#include "lvgl.h"
#include "driver/jpeg_decode.h"
#include "driver/i2s_std.h"
#include "avi_player.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

static const char *TAG = "MEDIA_PLAYER";

/* Display Configuration */
#define PHYSICAL_WIDTH   480
#define PHYSICAL_HEIGHT  800

/* State Variables */
static bool s_initialized = false;
static media_player_state_t s_state = MEDIA_PLAYER_STATE_IDLE;
static media_info_t s_media_info = {0};
static media_player_event_cb_t s_event_cb = NULL;
static void *s_event_user_data = NULL;
static char s_current_path[256] = {0};

/* AVI Player */
static avi_player_handle_t s_avi_handle = NULL;
static volatile bool s_playing = false;
static volatile bool s_first_frame = true;

/* Hardware */
static jpeg_decoder_handle_t s_jpeg_decoder = NULL;
static ppa_client_handle_t s_ppa_handle = NULL;
static bool s_audio_configured = false;

/* LVGL flush callback backup */
static lv_disp_drv_t *s_disp_drv = NULL;
static void (*s_original_flush_cb)(struct _lv_disp_drv_t *, const lv_area_t *, lv_color_t *) = NULL;

/* Decode Buffers */
static uint8_t *s_decode_buffer = NULL;
static size_t s_decode_buffer_size = 0;

/* UI */
static lv_obj_t *s_player_screen = NULL;
static lv_obj_t *s_touch_layer = NULL;

/* BMP Image Display */
static lv_color_t *s_image_buffer = NULL;
static lv_img_dsc_t s_img_dsc = {0};
static lv_obj_t *s_image_obj = NULL;
static lv_obj_t *s_close_btn = NULL;
static lv_obj_t *s_info_label = NULL;

/* Forward Declarations */
static void notify_state(media_player_state_t state);
static esp_err_t play_video(const char *filepath);
static esp_err_t show_bmp_image(const char *filepath);
static void cleanup_playback(void);

/* JPEG Decoder */
static esp_err_t init_jpeg_decoder(void)
{
    if (s_jpeg_decoder) return ESP_OK;
    
    jpeg_decode_engine_cfg_t cfg = {
        .intr_priority = 0,
        .timeout_ms = 3000,  /* Longer timeout for complex frames */
    };
    
    esp_err_t ret = jpeg_new_decoder_engine(&cfg, &s_jpeg_decoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create JPEG decoder: %d", ret);
    }
    return ret;
}

static void deinit_jpeg_decoder(void)
{
    if (s_jpeg_decoder) {
        jpeg_del_decoder_engine(s_jpeg_decoder);
        s_jpeg_decoder = NULL;
    }
}

/* UI Functions */
static void stop_btn_handler(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Stop button clicked");
    media_player_stop();
}

/* AVI Player Callbacks */
static void video_frame_cb(frame_data_t *frame, void *arg)
{
    if (!frame || !frame->data || !s_playing || !s_decode_buffer) return;
    
    video_frame_info_t *info = &frame->video_info;
    if (info->frame_format != FORMAT_MJEPG) return;
    
    jpeg_decode_cfg_t cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
    
    uint32_t out_size = 0;
    if (jpeg_decoder_process(s_jpeg_decoder, &cfg, frame->data, frame->data_bytes,
                              s_decode_buffer, s_decode_buffer_size, &out_size) != ESP_OK) {
        return;
    }
    
    esp_lcd_panel_handle_t panel = bsp_display_get_panel_handle();
    if (panel) {
        esp_lcd_panel_draw_bitmap(panel, 0, 0, info->width, info->height, s_decode_buffer);
    }
    
    if (s_first_frame) {
        s_first_frame = false;
    }
}

static void audio_frame_cb(frame_data_t *frame, void *arg)
{
    if (!frame || !frame->data || !s_playing) return;
    
    size_t written = 0;
    bsp_extra_i2s_write(frame->data, frame->data_bytes, &written, portMAX_DELAY);
}

static void audio_clock_callback(uint32_t rate, uint32_t bits, uint32_t ch, void *arg)
{
    if (s_audio_configured) return;
    
    i2s_slot_mode_t mode = (ch == 2) ? I2S_SLOT_MODE_STEREO : I2S_SLOT_MODE_MONO;
    if (bsp_extra_codec_set_fs_play(rate, bits, mode) == ESP_OK) {
        s_audio_configured = true;
        bsp_extra_codec_mute_set(false);
        int vol;
        bsp_extra_codec_volume_set(90, &vol);
    }
}

static void play_end_cb(void *arg)
{
    ESP_LOGI(TAG, "Playback ended, auto-stopping");
    s_playing = false;
    media_player_stop();
}

/* Touch handler for video playback */
static void video_touch_handler(lv_event_t *e)
{
    ESP_LOGI(TAG, "Screen touched, stopping playback");
    media_player_stop();
}

/* Dummy flush callback - does nothing during video playback */
static void dummy_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    lv_disp_flush_ready(drv);
}

/* Video Playback */
static esp_err_t play_video(const char *filepath)
{
    esp_err_t ret;
    
    /* Create transparent touch layer for exit control */
    if (bsp_display_lock(100)) {
        s_touch_layer = lv_obj_create(lv_layer_top());
        lv_obj_set_size(s_touch_layer, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_style_bg_opa(s_touch_layer, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(s_touch_layer, 0, 0);
        lv_obj_add_flag(s_touch_layer, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(s_touch_layer, video_touch_handler, LV_EVENT_CLICKED, NULL);
        
        /* Replace LVGL flush callback to block all LCD access */
        lv_disp_t *disp = lv_disp_get_default();
        if (disp) {
            s_disp_drv = disp->driver;
            s_original_flush_cb = s_disp_drv->flush_cb;
            s_disp_drv->flush_cb = dummy_flush_cb;
        }
        bsp_display_unlock();
    }
    
    /* Initialize hardware */
    ret = init_jpeg_decoder();
    if (ret != ESP_OK) {
        cleanup_playback();
        return ret;
    }
    
    /* Pre-allocate decode buffer */
    if (!s_decode_buffer) {
        jpeg_decode_memory_alloc_cfg_t mem_cfg = {
            .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
        };
        s_decode_buffer_size = 5 * 1024 * 1024;
        s_decode_buffer = jpeg_alloc_decoder_mem(s_decode_buffer_size, &mem_cfg, &s_decode_buffer_size);
    }
    
    /* Initialize AVI player */
    if (!s_avi_handle) {
        avi_player_config_t config = {
            .buffer_size = 8 * 1024 * 1024,  /* 8MB buffer for complex scenes */
            .video_cb = video_frame_cb,
            .audio_cb = audio_frame_cb,
            .audio_set_clock_cb = audio_clock_callback,
            .avi_play_end_cb = play_end_cb,
            .priority = 20,  /* Maximum priority for real-time playback */
            .coreID = 0,  /* Core 0 for better performance */
            .user_data = NULL,
            .stack_size = 20480,  /* 20KB stack for complex frames */
            .stack_in_psram = false,
        };
        
        ret = avi_player_init(config, &s_avi_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init AVI player: %d", ret);
            cleanup_playback();
            return ret;
        }
    }
    
    /* Start playback */
    s_playing = true;
    s_first_frame = true;
    
    ret = avi_player_play_from_file(s_avi_handle, filepath);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to play: %d", ret);
        cleanup_playback();
        return ret;
    }
    
    notify_state(MEDIA_PLAYER_STATE_PLAYING);
    ESP_LOGI(TAG, "Video playback started: %s", filepath);
    return ESP_OK;
}

/* BMP Image Display */
static void create_image_ui(void)
{
    if (!bsp_display_lock(100)) return;
    
    s_player_screen = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_player_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(s_player_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_player_screen, LV_OPA_COVER, 0);
    
    s_close_btn = lv_btn_create(s_player_screen);
    lv_obj_set_size(s_close_btn, 50, 50);
    lv_obj_align(s_close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(s_close_btn, lv_color_hex(0xE53935), 0);
    lv_obj_set_style_radius(s_close_btn, 25, 0);
    lv_obj_add_event_cb(s_close_btn, stop_btn_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *x = lv_label_create(s_close_btn);
    lv_label_set_text(x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x, lv_color_white(), 0);
    lv_obj_center(x);
    
    s_info_label = lv_label_create(s_player_screen);
    lv_label_set_text(s_info_label, "Loading...");
    lv_obj_set_style_text_color(s_info_label, lv_color_white(), 0);
    lv_obj_align(s_info_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    bsp_display_unlock();
}

static esp_err_t show_bmp_image(const char *filepath)
{
    FILE *f = fopen(filepath, "rb");
    if (!f) return ESP_ERR_NOT_FOUND;
    
    uint8_t header[54];
    if (fread(header, 1, 54, f) != 54 || header[0] != 'B' || header[1] != 'M') {
        fclose(f);
        return ESP_ERR_INVALID_ARG;
    }
    
    uint32_t data_offset = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);
    int32_t w = (int32_t)(header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24));
    int32_t h = (int32_t)(header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24));
    uint16_t bpp = header[28] | (header[29] << 8);
    
    bool bottom_up = (h > 0);
    if (h < 0) h = -h;
    
    if (w <= 0 || w > 4096 || h <= 0 || h > 4096 || bpp != 24) {
        fclose(f);
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    s_media_info.width = w;
    s_media_info.height = h;
    
    size_t buf_size = w * h * sizeof(lv_color_t);
    s_image_buffer = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!s_image_buffer) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    
    fseek(f, data_offset, SEEK_SET);
    
    int row_size = ((w * 3 + 3) / 4) * 4;
    uint8_t *row = malloc(row_size);
    if (!row) {
        heap_caps_free(s_image_buffer);
        s_image_buffer = NULL;
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    
    for (int y = 0; y < h; y++) {
        int dest_y = bottom_up ? (h - 1 - y) : y;
        if (fread(row, 1, row_size, f) != (size_t)row_size) break;
        for (int x = 0; x < w; x++) {
            s_image_buffer[dest_y * w + x] = lv_color_make(row[x*3+2], row[x*3+1], row[x*3]);
        }
    }
    free(row);
    fclose(f);
    
    create_image_ui();
    
    s_img_dsc.header.w = w;
    s_img_dsc.header.h = h;
    s_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_img_dsc.data = (const uint8_t *)s_image_buffer;
    
    if (bsp_display_lock(100)) {
        s_image_obj = lv_img_create(s_player_screen);
        lv_img_set_src(s_image_obj, &s_img_dsc);
        
        int32_t scale = ((LV_HOR_RES * 256 / w) < (LV_VER_RES * 256 / h)) ? 
                        (LV_HOR_RES * 256 / w) : (LV_VER_RES * 256 / h);
        if (scale < 32) scale = 32;
        if (scale > 512) scale = 512;
        lv_img_set_zoom(s_image_obj, scale);
        lv_obj_center(s_image_obj);
        lv_obj_move_foreground(s_close_btn);
        
        if (s_info_label) {
            char info[64];
            snprintf(info, sizeof(info), "%ldx%ld BMP", (long)w, (long)h);
            lv_label_set_text(s_info_label, info);
        }
        
        bsp_display_unlock();
    }
    
    notify_state(MEDIA_PLAYER_STATE_PLAYING);
    return ESP_OK;
}

/* Cleanup */
static void cleanup_playback(void)
{
    s_playing = false;
    
    /* Remove touch layer and restore LVGL flush callback */
    if (bsp_display_lock(100)) {
        if (s_touch_layer) {
            lv_obj_del(s_touch_layer);
            s_touch_layer = NULL;
        }
        
        if (s_disp_drv && s_original_flush_cb) {
            s_disp_drv->flush_cb = s_original_flush_cb;
            s_original_flush_cb = NULL;
            s_disp_drv = NULL;
        }
        
        /* Delete player screen UI (for BMP images) */
        if (s_player_screen && lv_obj_is_valid(s_player_screen)) {
            lv_obj_del(s_player_screen);
            s_player_screen = NULL;
            s_close_btn = NULL;
            s_info_label = NULL;
            s_image_obj = NULL;
        }
        
        lv_obj_invalidate(lv_scr_act());
        bsp_display_unlock();
    }
    
    if (s_avi_handle) {
        avi_player_play_stop(s_avi_handle);
        avi_player_deinit(s_avi_handle);
        s_avi_handle = NULL;
    }
    
    deinit_jpeg_decoder();
    
    if (s_ppa_handle) {
        ppa_unregister_client(s_ppa_handle);
        s_ppa_handle = NULL;
    }
    
    if (s_audio_configured) {
        bsp_extra_codec_dev_stop();
        s_audio_configured = false;
    }
    
    if (s_decode_buffer) {
        heap_caps_free(s_decode_buffer);
        s_decode_buffer = NULL;
    }
    
    if (s_image_buffer) {
        heap_caps_free(s_image_buffer);
        s_image_buffer = NULL;
    }
    
    s_first_frame = true;
    memset(&s_media_info, 0, sizeof(s_media_info));
}

/* Public API */
static void notify_state(media_player_state_t state)
{
    s_state = state;
    if (s_event_cb) {
        s_event_cb(state, s_event_user_data);
    }
}

esp_err_t media_player_init(void)
{
    if (s_initialized) return ESP_OK;
    
    bsp_extra_codec_init();
    
    s_initialized = true;
    ESP_LOGI(TAG, "Media player initialized (Official AVI Player API)");
    return ESP_OK;
}

void media_player_deinit(void)
{
    media_player_stop();
    s_initialized = false;
}

void media_player_set_callback(media_player_event_cb_t cb, void *user_data)
{
    s_event_cb = cb;
    s_event_user_data = user_data;
}

esp_err_t media_player_play(const char *filepath)
{
    if (!s_initialized || !filepath) return ESP_ERR_INVALID_ARG;
    
    media_player_stop();
    
    strncpy(s_current_path, filepath, sizeof(s_current_path) - 1);
    
    media_type_t type = media_player_get_type(filepath);
    s_media_info.type = type;
    
    struct stat st;
    if (stat(filepath, &st) == 0) {
        s_media_info.file_size = st.st_size;
    }
    
    notify_state(MEDIA_PLAYER_STATE_LOADING);
    
    esp_err_t ret = ESP_FAIL;
    switch (type) {
        case MEDIA_TYPE_BMP:
            ret = show_bmp_image(filepath);
            break;
        case MEDIA_TYPE_AVI:
            ret = play_video(filepath);
            break;
        default:
            ESP_LOGE(TAG, "Unsupported format");
            ret = ESP_ERR_NOT_SUPPORTED;
            break;
    }
    
    if (ret != ESP_OK) {
        notify_state(MEDIA_PLAYER_STATE_ERROR);
    }
    return ret;
}

esp_err_t media_player_stop(void)
{
    if (s_state == MEDIA_PLAYER_STATE_IDLE) return ESP_OK;
    
    ESP_LOGI(TAG, "Stopping playback...");
    cleanup_playback();
    notify_state(MEDIA_PLAYER_STATE_IDLE);
    
    return ESP_OK;
}

esp_err_t media_player_pause(void) { return ESP_OK; }
esp_err_t media_player_resume(void) { return ESP_OK; }

media_player_state_t media_player_get_state(void) { return s_state; }
bool media_player_is_active(void) { return s_state != MEDIA_PLAYER_STATE_IDLE; }
bool media_player_is_video_playing(void) { return s_media_info.type == MEDIA_TYPE_AVI && s_state == MEDIA_PLAYER_STATE_PLAYING; }

esp_err_t media_player_get_info(media_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    memcpy(info, &s_media_info, sizeof(media_info_t));
    return ESP_OK;
}

esp_err_t media_player_set_volume(int volume)
{
    int vol_set;
    return bsp_extra_codec_volume_set(volume, &vol_set);
}

int media_player_get_volume(void) { return bsp_extra_codec_volume_get(); }
esp_err_t media_player_set_mute(bool mute) { return bsp_extra_codec_mute_set(mute); }

media_type_t media_player_get_type(const char *filepath)
{
    if (!filepath) return MEDIA_TYPE_UNKNOWN;
    
    const char *ext = strrchr(filepath, '.');
    if (!ext) return MEDIA_TYPE_UNKNOWN;
    ext++;
    
    if (strcasecmp(ext, "bmp") == 0) return MEDIA_TYPE_BMP;
    if (strcasecmp(ext, "avi") == 0) return MEDIA_TYPE_AVI;
    
    return MEDIA_TYPE_UNKNOWN;
}

bool media_player_is_supported(const char *filepath)
{
    media_type_t t = media_player_get_type(filepath);
    return t == MEDIA_TYPE_BMP || t == MEDIA_TYPE_AVI;
}

const char* media_player_get_type_name(media_type_t type)
{
    switch (type) {
        case MEDIA_TYPE_BMP: return "BMP";
        case MEDIA_TYPE_AVI: return "AVI Video";
        default: return "Unknown";
    }
}
