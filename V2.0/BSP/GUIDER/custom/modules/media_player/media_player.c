/**
 * @file media_player.c
 * @brief Media Player with Prefetch Architecture + Adaptive Sync
 * 
 * Architecture:
 * - SD Prefetch Task: Continuously reads AVI data, separates audio/video
 * - Audio Ring Buffer: 1MB buffer (~5.6s at 44.1kHz stereo)
 * - Video Frame Queue: 6 raw MJPEG frames
 * - Audio Task: Plays from ring buffer at constant rate (I2S DMA)
 * - Video Task: Decodes and displays, adapts to audio buffer level
 * 
 * Key Features:
 * - SD card reading decoupled from playback
 * - Large buffers absorb SD card latency spikes
 * - Adaptive sync without aggressive frame dropping
 * - Audio is master clock (I2S provides precise timing)
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
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "avi_player.h"
#include "avi_def.h"

static const char *TAG = "MEDIA_PLAYER";

/* ============== Display Configuration ============== */
#define PHYSICAL_WIDTH   480
#define PHYSICAL_HEIGHT  800

#define ALIGN_UP_BY(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* ============== Buffer Configuration ============== */
#define AUDIO_RING_BUFFER_SIZE      (1024 * 1024)
#define AUDIO_PREFILL_THRESHOLD     (128 * 1024)
#define AUDIO_TARGET_LEVEL          (256 * 1024)
#define AUDIO_CHUNK_SIZE            (2048)

#define VIDEO_RAW_QUEUE_SIZE        (6)
#define VIDEO_RAW_FRAME_MAX_SIZE    (300 * 1024)

#define NUM_DECODE_BUFFERS          (2)

/* ============== Task Configuration ============== */
#define PREFETCH_TASK_STACK         (8192)
#define PREFETCH_TASK_PRIO          (10)
#define AUDIO_TASK_STACK            (4096)
#define AUDIO_TASK_PRIO             (configMAX_PRIORITIES - 1)
#define VIDEO_TASK_STACK            (8192)
#define VIDEO_TASK_PRIO             (9)

/* ============== State Variables ============== */
static bool s_initialized = false;
static media_player_state_t s_state = MEDIA_PLAYER_STATE_IDLE;
static media_info_t s_media_info = {0};
static media_player_event_cb_t s_event_cb = NULL;
static void *s_event_user_data = NULL;
static char s_current_path[256] = {0};
static volatile bool s_stop_requested = false;

/* ============== Audio Ring Buffer ============== */
typedef struct {
    uint8_t *buffer;
    size_t size;
    volatile size_t read_pos;
    volatile size_t write_pos;
} audio_ringbuf_t;

static audio_ringbuf_t s_audio_rb = {0};

/* ============== Video Raw Frame Queue ============== */
typedef struct {
    uint8_t *data;
    size_t size;
    uint32_t width;
    uint32_t height;
    uint32_t pts_ms;
} video_raw_frame_t;

static QueueHandle_t s_video_raw_queue = NULL;
static uint8_t *s_video_raw_buffers[VIDEO_RAW_QUEUE_SIZE] = {NULL};
static volatile int s_raw_buf_write_idx = 0;

/* ============== Decode Buffers ============== */
static uint8_t *s_decode_buffers[NUM_DECODE_BUFFERS] = {NULL};
static uint8_t *s_rotated_buffers[NUM_DECODE_BUFFERS] = {NULL};
static size_t s_decode_buffer_size = 0;
static size_t s_rotated_buffer_size = 0;
static volatile int s_decode_buf_idx = 0;

/* ============== Hardware Handles ============== */
static jpeg_decoder_handle_t s_jpeg_decoder = NULL;
static ppa_client_handle_t s_ppa_handle = NULL;

/* ============== Task Handles ============== */
static TaskHandle_t s_prefetch_task = NULL;
static TaskHandle_t s_audio_task = NULL;
static TaskHandle_t s_video_task = NULL;
static volatile bool s_prefetch_running = false;
static volatile bool s_audio_running = false;
static volatile bool s_video_running = false;

/* ============== AVI Parsing State ============== */
static FILE *s_avi_file = NULL;
static uint32_t s_avi_movi_start = 0;
static uint32_t s_avi_movi_size = 0;
static uint32_t s_avi_read_offset = 0;
static uint32_t s_video_fps = 25;
static uint32_t s_total_frames = 0;
static uint32_t s_audio_sample_rate = 44100;
static uint8_t s_audio_channels = 2;
static uint8_t s_audio_bits = 16;
static bool s_audio_configured = false;

/* ============== Playback State ============== */
static volatile bool s_audio_prefill_done = false;
static volatile uint32_t s_video_frame_index = 0;
static volatile uint32_t s_audio_bytes_played = 0;
static int64_t s_playback_start_time = 0;

/* ============== Adaptive Sync ============== */
static int32_t s_video_delay_adjust_us = 0;

/* ============== Video Display Parameters ============== */
static float s_scale = 1.0f;
static int s_out_w = 0;
static int s_out_h = 0;
static int s_offset_x = 0;
static int s_offset_y = 0;
static bool s_scale_valid = false;
static bool s_direct_display = false;

/* ============== UI ============== */
static lv_obj_t *s_player_screen = NULL;
static bool s_video_mode = false;

/* ============== Image Display (BMP) ============== */
static lv_color_t *s_image_buffer = NULL;
static lv_img_dsc_t s_img_dsc = {0};
static lv_obj_t *s_image_obj = NULL;
static lv_obj_t *s_close_btn = NULL;
static lv_obj_t *s_info_label = NULL;

/* ============== Forward Declarations ============== */
static void notify_state(media_player_state_t state);
static esp_err_t play_video(const char *filepath);
static esp_err_t show_bmp_image(const char *filepath);
static void cleanup_playback(void);

/* ============== Logging Control ============== */
static void suppress_logs(void)
{
    esp_log_level_set("jpeg.decoder", ESP_LOG_NONE);
    esp_log_level_set("jpeg.common", ESP_LOG_NONE);
    esp_log_level_set("avi player", ESP_LOG_NONE);
    esp_log_level_set("avifile", ESP_LOG_NONE);
    esp_log_level_set("I2S_IF", ESP_LOG_NONE);
    esp_log_level_set("Adev_Codec", ESP_LOG_NONE);
    esp_log_level_set("ES8311", ESP_LOG_NONE);
    esp_log_level_set("gpio", ESP_LOG_NONE);
    esp_log_level_set("i2s_common", ESP_LOG_NONE);
}

static void restore_logs(void)
{
    esp_log_level_set("jpeg.decoder", ESP_LOG_INFO);
    esp_log_level_set("jpeg.common", ESP_LOG_INFO);
    esp_log_level_set("avi player", ESP_LOG_INFO);
    esp_log_level_set("avifile", ESP_LOG_INFO);
    esp_log_level_set("I2S_IF", ESP_LOG_INFO);
    esp_log_level_set("Adev_Codec", ESP_LOG_INFO);
    esp_log_level_set("ES8311", ESP_LOG_INFO);
    esp_log_level_set("gpio", ESP_LOG_INFO);
    esp_log_level_set("i2s_common", ESP_LOG_INFO);
}

/* ============== Audio Ring Buffer Implementation ============== */
static inline size_t rb_data_available(void)
{
    size_t w = s_audio_rb.write_pos;
    size_t r = s_audio_rb.read_pos;
    return (w >= r) ? (w - r) : (s_audio_rb.size - r + w);
}

static inline size_t rb_space_available(void)
{
    return s_audio_rb.size - rb_data_available() - 1;
}

static size_t rb_write(const uint8_t *data, size_t len)
{
    if (!s_audio_rb.buffer || !data || len == 0) return 0;
    
    size_t space = rb_space_available();
    size_t to_write = (len < space) ? len : space;
    if (to_write == 0) return 0;
    
    size_t wp = s_audio_rb.write_pos;
    size_t first = s_audio_rb.size - wp;
    
    if (first >= to_write) {
        memcpy(s_audio_rb.buffer + wp, data, to_write);
    } else {
        memcpy(s_audio_rb.buffer + wp, data, first);
        memcpy(s_audio_rb.buffer, data + first, to_write - first);
    }
    
    __sync_synchronize();
    s_audio_rb.write_pos = (wp + to_write) % s_audio_rb.size;
    return to_write;
}

static size_t rb_read(uint8_t *data, size_t len)
{
    if (!s_audio_rb.buffer || !data || len == 0) return 0;
    
    size_t avail = rb_data_available();
    size_t to_read = (len < avail) ? len : avail;
    if (to_read == 0) return 0;
    
    size_t rp = s_audio_rb.read_pos;
    size_t first = s_audio_rb.size - rp;
    
    if (first >= to_read) {
        memcpy(data, s_audio_rb.buffer + rp, to_read);
    } else {
        memcpy(data, s_audio_rb.buffer + rp, first);
        memcpy(data + first, s_audio_rb.buffer, to_read - first);
    }
    
    __sync_synchronize();
    s_audio_rb.read_pos = (rp + to_read) % s_audio_rb.size;
    return to_read;
}

static void rb_reset(void)
{
    s_audio_rb.read_pos = 0;
    s_audio_rb.write_pos = 0;
}


/* ============== AVI Header Parsing ============== */
static esp_err_t parse_avi_header(void)
{
    if (!s_avi_file) return ESP_ERR_INVALID_STATE;
    
    fseek(s_avi_file, 0, SEEK_SET);
    
    uint8_t header[512];
    size_t read_size = fread(header, 1, sizeof(header), s_avi_file);
    if (read_size < 256) {
        ESP_LOGE(TAG, "AVI file too small");
        return ESP_ERR_INVALID_SIZE;
    }
    
    /* Verify RIFF/AVI */
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "AVI ", 4) != 0) {
        ESP_LOGE(TAG, "Not a valid AVI file");
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Find avih chunk */
    const uint8_t *p = header + 12;
    const uint8_t *end = header + read_size - 56;
    
    while (p < end) {
        if (memcmp(p, "avih", 4) == 0) {
            const AVI_AVIH_CHUNK *avih = (const AVI_AVIH_CHUNK *)p;
            s_total_frames = avih->total_frames;
            if (avih->us_per_frame > 0) {
                s_video_fps = 1000000 / avih->us_per_frame;
                if (s_video_fps == 0) s_video_fps = 25;
            }
            s_media_info.width = avih->width;
            s_media_info.height = avih->height;
            s_media_info.duration_ms = (s_total_frames * 1000) / s_video_fps;
            ESP_LOGI(TAG, "AVI: %lux%lu, %lu fps, %lu frames",
                     (unsigned long)avih->width, (unsigned long)avih->height,
                     (unsigned long)s_video_fps, (unsigned long)s_total_frames);
            break;
        }
        p++;
    }
    
    /* Find strh/strf for audio info */
    p = header + 12;
    while (p < end) {
        if (memcmp(p, "strh", 4) == 0) {
            const uint8_t *strh = p + 8;
            if (memcmp(strh, "auds", 4) == 0) {
                /* Audio stream header found, look for strf */
                const uint8_t *q = p + 8;
                while (q < end) {
                    if (memcmp(q, "strf", 4) == 0) {
                        /* WAVEFORMATEX structure */
                        const uint8_t *wf = q + 8;
                        s_audio_channels = wf[2] | (wf[3] << 8);
                        s_audio_sample_rate = wf[4] | (wf[5] << 8) | (wf[6] << 16) | (wf[7] << 24);
                        s_audio_bits = wf[14] | (wf[15] << 8);
                        s_media_info.has_audio = true;
                        s_media_info.audio_sample_rate = s_audio_sample_rate;
                        ESP_LOGI(TAG, "Audio: %lu Hz, %u ch, %u bit",
                                 (unsigned long)s_audio_sample_rate, s_audio_channels, s_audio_bits);
                        break;
                    }
                    q++;
                }
            }
        }
        p++;
    }
    
    /* Find movi chunk */
    fseek(s_avi_file, 0, SEEK_SET);
    uint8_t buf[12];
    uint32_t offset = 0;
    
    while (fread(buf, 1, 12, s_avi_file) == 12) {
        if (memcmp(buf, "LIST", 4) == 0 && memcmp(buf + 8, "movi", 4) == 0) {
            s_avi_movi_start = offset + 12;
            s_avi_movi_size = (buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24)) - 4;
            ESP_LOGI(TAG, "movi chunk at %lu, size %lu", 
                     (unsigned long)s_avi_movi_start, (unsigned long)s_avi_movi_size);
            return ESP_OK;
        }
        offset++;
        fseek(s_avi_file, offset, SEEK_SET);
    }
    
    ESP_LOGE(TAG, "movi chunk not found");
    return ESP_ERR_NOT_FOUND;
}

/* ============== AVI Chunk Reading ============== */
#define AVI_CHUNK_VIDEO  1
#define AVI_CHUNK_AUDIO  2
#define AVI_CHUNK_OTHER  0
#define AVI_CHUNK_END    -1

static int read_next_chunk(uint8_t *buffer, size_t buf_size, size_t *out_size, uint32_t *out_pts_ms)
{
    if (!s_avi_file || s_avi_read_offset >= s_avi_movi_size) {
        return AVI_CHUNK_END;
    }
    
    fseek(s_avi_file, s_avi_movi_start + s_avi_read_offset, SEEK_SET);
    
    uint8_t chunk_header[8];
    if (fread(chunk_header, 1, 8, s_avi_file) != 8) {
        return AVI_CHUNK_END;
    }
    
    uint32_t fourcc = chunk_header[0] | (chunk_header[1] << 8) | 
                      (chunk_header[2] << 16) | (chunk_header[3] << 24);
    uint32_t chunk_size = chunk_header[4] | (chunk_header[5] << 8) | 
                          (chunk_header[6] << 16) | (chunk_header[7] << 24);
    
    /* Align to word boundary */
    uint32_t padded_size = (chunk_size + 1) & ~1;
    
    s_avi_read_offset += 8 + padded_size;
    
    /* Check chunk type - high 16 bits indicate type (dc/wb) */
    uint16_t chunk_type = (fourcc >> 16) & 0xFFFF;
    
    /* 'dc' or 'DC' = video, 'wb' or 'WB' = audio */
    bool is_video = (chunk_type == 0x6364 || chunk_type == 0x4443);  /* 'dc' or 'DC' */
    bool is_audio = (chunk_type == 0x6277 || chunk_type == 0x4257);  /* 'wb' or 'WB' */
    
    if (!is_video && !is_audio) {
        *out_size = 0;
        return AVI_CHUNK_OTHER;
    }
    
    if (chunk_size > buf_size) {
        ESP_LOGW(TAG, "Chunk too large: %lu > %u", (unsigned long)chunk_size, (unsigned)buf_size);
        fseek(s_avi_file, chunk_size, SEEK_CUR);
        *out_size = 0;
        return is_video ? AVI_CHUNK_VIDEO : AVI_CHUNK_AUDIO;
    }
    
    if (fread(buffer, 1, chunk_size, s_avi_file) != chunk_size) {
        return AVI_CHUNK_END;
    }
    
    *out_size = chunk_size;
    
    if (is_video) {
        /* Calculate PTS based on frame index */
        *out_pts_ms = (s_video_frame_index * 1000) / s_video_fps;
        s_video_frame_index++;
        return AVI_CHUNK_VIDEO;
    } else {
        *out_pts_ms = 0;
        return AVI_CHUNK_AUDIO;
    }
}


/* ============== SD Prefetch Task ============== */
static void prefetch_task(void *arg)
{
    (void)arg;
    
    ESP_LOGI(TAG, "Prefetch task started");
    
    /* Allocate read buffer */
    uint8_t *read_buf = heap_caps_malloc(VIDEO_RAW_FRAME_MAX_SIZE, MALLOC_CAP_SPIRAM);
    if (!read_buf) {
        ESP_LOGE(TAG, "Failed to allocate read buffer");
        s_prefetch_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    /* Mono to stereo conversion buffer */
    int16_t *stereo_buf = heap_caps_malloc(32768, MALLOC_CAP_SPIRAM);
    if (!stereo_buf) {
        heap_caps_free(read_buf);
        ESP_LOGE(TAG, "Failed to allocate stereo buffer");
        s_prefetch_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    uint32_t audio_chunks = 0;
    uint32_t video_chunks = 0;
    int64_t last_log_time = esp_timer_get_time();
    
    while (s_prefetch_running && !s_stop_requested) {
        size_t chunk_size = 0;
        uint32_t pts_ms = 0;
        
        int chunk_type = read_next_chunk(read_buf, VIDEO_RAW_FRAME_MAX_SIZE, &chunk_size, &pts_ms);
        
        if (chunk_type == AVI_CHUNK_END) {
            ESP_LOGI(TAG, "Prefetch: end of file");
            break;
        }
        
        if (chunk_type == AVI_CHUNK_AUDIO && chunk_size > 0) {
            /* Process audio chunk */
            uint8_t *audio_data = read_buf;
            size_t audio_len = chunk_size;
            
            /* Convert mono to stereo if needed */
            if (s_audio_channels == 1 && s_audio_bits == 16) {
                size_t samples = chunk_size / 2;
                int16_t *mono = (int16_t *)read_buf;
                size_t max_samples = 16384 / 2;
                if (samples > max_samples) samples = max_samples;
                
                for (size_t i = 0; i < samples; i++) {
                    stereo_buf[i * 2] = mono[i];
                    stereo_buf[i * 2 + 1] = mono[i];
                }
                audio_data = (uint8_t *)stereo_buf;
                audio_len = samples * 4;
            }
            
            /* Write to ring buffer - may need to wait if full */
            size_t written = 0;
            while (written < audio_len && s_prefetch_running && !s_stop_requested) {
                size_t w = rb_write(audio_data + written, audio_len - written);
                written += w;
                if (written < audio_len) {
                    vTaskDelay(pdMS_TO_TICKS(5));
                }
            }
            audio_chunks++;
            
        } else if (chunk_type == AVI_CHUNK_VIDEO && chunk_size > 0) {
            /* Process video chunk */
            
            /* Wait if video queue is full */
            while (uxQueueSpacesAvailable(s_video_raw_queue) == 0 && 
                   s_prefetch_running && !s_stop_requested) {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            
            if (!s_prefetch_running || s_stop_requested) break;
            
            /* Get buffer for this frame */
            uint8_t *frame_buf = s_video_raw_buffers[s_raw_buf_write_idx];
            if (frame_buf && chunk_size <= VIDEO_RAW_FRAME_MAX_SIZE) {
                memcpy(frame_buf, read_buf, chunk_size);
                
                video_raw_frame_t frame = {
                    .data = frame_buf,
                    .size = chunk_size,
                    .width = s_media_info.width,
                    .height = s_media_info.height,
                    .pts_ms = pts_ms,
                };
                
                if (xQueueSend(s_video_raw_queue, &frame, pdMS_TO_TICKS(100)) == pdTRUE) {
                    s_raw_buf_write_idx = (s_raw_buf_write_idx + 1) % VIDEO_RAW_QUEUE_SIZE;
                    video_chunks++;
                }
            }
        }
        
        /* Log progress periodically */
        int64_t now = esp_timer_get_time();
        if (now - last_log_time > 5000000) {
            size_t audio_level = rb_data_available();
            UBaseType_t video_level = uxQueueMessagesWaiting(s_video_raw_queue);
            ESP_LOGI(TAG, "Prefetch: audio=%uKB, video=%u frames, chunks: a=%lu v=%lu",
                     (unsigned)(audio_level / 1024), (unsigned)video_level,
                     (unsigned long)audio_chunks, (unsigned long)video_chunks);
            last_log_time = now;
        }
        
        /* Small yield to prevent starving other tasks */
        taskYIELD();
    }
    
    ESP_LOGI(TAG, "Prefetch task stopped: audio=%lu, video=%lu chunks",
             (unsigned long)audio_chunks, (unsigned long)video_chunks);
    
    heap_caps_free(read_buf);
    heap_caps_free(stereo_buf);
    s_prefetch_task = NULL;
    vTaskDelete(NULL);
}

/* ============== Audio Playback Task ============== */
static void audio_task(void *arg)
{
    (void)arg;
    
    ESP_LOGI(TAG, "Audio task started");
    
    uint8_t *chunk = heap_caps_malloc(AUDIO_CHUNK_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!chunk) {
        chunk = heap_caps_malloc(AUDIO_CHUNK_SIZE, MALLOC_CAP_SPIRAM);
    }
    if (!chunk) {
        ESP_LOGE(TAG, "Failed to allocate audio chunk");
        s_audio_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    uint32_t underruns = 0;
    int64_t last_log_time = esp_timer_get_time();
    
    /* Wait for prefill */
    ESP_LOGI(TAG, "Audio: waiting for prefill...");
    while (s_audio_running && !s_stop_requested) {
        size_t avail = rb_data_available();
        if (avail >= AUDIO_PREFILL_THRESHOLD) {
            s_audio_prefill_done = true;
            s_playback_start_time = esp_timer_get_time();
            s_audio_bytes_played = 0;
            ESP_LOGI(TAG, "Audio: prefill done (%uKB), starting playback", (unsigned)(avail / 1024));
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    /* Main playback loop */
    while (s_audio_running && !s_stop_requested) {
        size_t avail = rb_data_available();
        
        if (avail >= AUDIO_CHUNK_SIZE) {
            size_t read_len = rb_read(chunk, AUDIO_CHUNK_SIZE);
            if (read_len > 0) {
                size_t written = 0;
                bsp_extra_i2s_write(chunk, read_len, &written, portMAX_DELAY);
                s_audio_bytes_played += written;
            }
        } else if (avail > 0) {
            /* Play remaining data */
            size_t read_len = rb_read(chunk, avail);
            if (read_len > 0) {
                size_t written = 0;
                bsp_extra_i2s_write(chunk, read_len, &written, portMAX_DELAY);
                s_audio_bytes_played += written;
            }
            vTaskDelay(pdMS_TO_TICKS(2));
        } else {
            /* Underrun */
            underruns++;
            if (underruns <= 5 || underruns % 50 == 0) {
                ESP_LOGW(TAG, "Audio underrun #%lu", (unsigned long)underruns);
            }
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        
        /* Log status periodically */
        int64_t now = esp_timer_get_time();
        if (now - last_log_time > 3000000) {
            uint32_t bytes_per_sec = s_audio_sample_rate * 2 * 2;
            uint32_t buffer_ms = (avail * 1000) / bytes_per_sec;
            ESP_LOGI(TAG, "Audio: buffer=%uKB (~%lums), played=%luKB",
                     (unsigned)(avail / 1024), (unsigned long)buffer_ms,
                     (unsigned long)(s_audio_bytes_played / 1024));
            last_log_time = now;
        }
    }
    
    ESP_LOGI(TAG, "Audio task stopped, underruns=%lu", (unsigned long)underruns);
    heap_caps_free(chunk);
    s_audio_task = NULL;
    vTaskDelete(NULL);
}


/* ============== JPEG Decoder ============== */
static esp_err_t init_jpeg_decoder(void)
{
    if (s_jpeg_decoder) return ESP_OK;
    
    jpeg_decode_engine_cfg_t cfg = {
        .intr_priority = 0,
        .timeout_ms = 1000,
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

static esp_err_t decode_mjpeg_frame(const uint8_t *jpeg_data, size_t jpeg_size, 
                                     uint32_t width, uint32_t height, int buf_idx)
{
    if (!s_jpeg_decoder || !jpeg_data || !s_decode_buffers[buf_idx]) {
        return ESP_ERR_INVALID_STATE;
    }
    
    /* Validate JPEG header */
    if (jpeg_size < 2 || jpeg_data[0] != 0xFF || jpeg_data[1] != 0xD8) {
        return ESP_ERR_INVALID_ARG;
    }
    
    jpeg_decode_cfg_t decode_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
    
    uint32_t out_size = 0;
    esp_err_t ret = jpeg_decoder_process(s_jpeg_decoder, &decode_cfg,
                                          jpeg_data, jpeg_size,
                                          s_decode_buffers[buf_idx], s_decode_buffer_size,
                                          &out_size);
    return ret;
}

/* ============== Adaptive Sync Update ============== */
static void update_adaptive_sync(void)
{
    size_t audio_level = rb_data_available();
    int32_t diff = (int32_t)audio_level - (int32_t)AUDIO_TARGET_LEVEL;
    
    /* Adjust video timing based on audio buffer level */
    if (diff > 128 * 1024) {
        /* Audio buffer too full - slow down video slightly */
        s_video_delay_adjust_us = 3000;  /* +3ms per frame */
    } else if (diff > 64 * 1024) {
        s_video_delay_adjust_us = 1000;  /* +1ms */
    } else if (diff < -128 * 1024) {
        /* Audio buffer too empty - speed up video */
        s_video_delay_adjust_us = -3000;  /* -3ms */
    } else if (diff < -64 * 1024) {
        s_video_delay_adjust_us = -1000;  /* -1ms */
    } else {
        s_video_delay_adjust_us = 0;
    }
}

/* ============== Video Decode/Display Task ============== */
static void video_task(void *arg)
{
    (void)arg;
    
    ESP_LOGI(TAG, "Video task started");
    
    uint32_t frames_displayed = 0;
    uint32_t frame_interval_us = 1000000 / s_video_fps;
    int64_t next_frame_time = 0;
    int64_t last_log_time = esp_timer_get_time();
    bool first_frame = true;
    
    /* Wait for audio prefill before starting video */
    ESP_LOGI(TAG, "Video: waiting for audio prefill...");
    while (s_video_running && !s_stop_requested && !s_audio_prefill_done) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    if (!s_video_running || s_stop_requested) {
        s_video_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Video: starting playback");
    next_frame_time = esp_timer_get_time();
    
    while (s_video_running && !s_stop_requested) {
        video_raw_frame_t frame;
        
        /* Wait for next frame time with adaptive adjustment */
        int64_t now = esp_timer_get_time();
        int64_t wait_us = next_frame_time - now + s_video_delay_adjust_us;
        
        if (wait_us > 1000) {
            vTaskDelay(pdMS_TO_TICKS(wait_us / 1000));
        }
        
        /* Get frame from queue */
        if (xQueueReceive(s_video_raw_queue, &frame, pdMS_TO_TICKS(100)) != pdTRUE) {
            /* Queue empty - prefetch might be slow */
            continue;
        }
        
        if (!frame.data || frame.size == 0) continue;
        
        /* Validate MJPEG data */
        if (frame.size < 2 || frame.data[0] != 0xFF || frame.data[1] != 0xD8) {
            continue;
        }
        
        int buf_idx = s_decode_buf_idx;
        
        /* Decode MJPEG frame */
        esp_err_t ret = decode_mjpeg_frame(frame.data, frame.size, 
                                            frame.width, frame.height, buf_idx);
        if (ret != ESP_OK) {
            continue;
        }
        
        /* Get LCD panel */
        esp_lcd_panel_handle_t panel = bsp_display_get_panel_handle();
        if (!panel) continue;
        
        /* Calculate display parameters on first frame */
        if (!s_scale_valid) {
            uint32_t vid_w = frame.width;
            uint32_t vid_h = frame.height;
            
            s_direct_display = (vid_w == PHYSICAL_WIDTH && vid_h == PHYSICAL_HEIGHT);
            bool direct_rotate = (vid_w == PHYSICAL_HEIGHT && vid_h == PHYSICAL_WIDTH);
            
            if (s_direct_display) {
                s_scale = 1.0f;
                s_out_w = PHYSICAL_WIDTH;
                s_out_h = PHYSICAL_HEIGHT;
                s_offset_x = 0;
                s_offset_y = 0;
                ESP_LOGI(TAG, "Direct display: %lux%lu", (unsigned long)vid_w, (unsigned long)vid_h);
            } else if (direct_rotate) {
                s_scale = 1.0f;
                s_out_w = PHYSICAL_WIDTH;
                s_out_h = PHYSICAL_HEIGHT;
                s_offset_x = 0;
                s_offset_y = 0;
                ESP_LOGI(TAG, "Direct rotate: %lux%lu -> %dx%d", 
                         (unsigned long)vid_w, (unsigned long)vid_h, s_out_w, s_out_h);
            } else {
                float rotated_w = (float)vid_h;
                float rotated_h = (float)vid_w;
                float scale_x = (float)PHYSICAL_WIDTH / rotated_w;
                float scale_y = (float)PHYSICAL_HEIGHT / rotated_h;
                s_scale = (scale_x < scale_y) ? scale_x : scale_y;
                if (s_scale < 0.125f) s_scale = 0.125f;
                if (s_scale > 8.0f) s_scale = 8.0f;
                
                s_out_w = (int)(rotated_w * s_scale);
                s_out_h = (int)(rotated_h * s_scale);
                s_offset_x = (PHYSICAL_WIDTH - s_out_w) / 2;
                s_offset_y = (PHYSICAL_HEIGHT - s_out_h) / 2;
                if (s_offset_x < 0) s_offset_x = 0;
                if (s_offset_y < 0) s_offset_y = 0;
                
                ESP_LOGI(TAG, "Scale mode: %lux%lu -> %dx%d (scale=%.2f)",
                         (unsigned long)vid_w, (unsigned long)vid_h, s_out_w, s_out_h, s_scale);
            }
            s_scale_valid = true;
            
            /* Clear screen if needed */
            if (s_offset_x > 0 || s_offset_y > 0) {
                memset(s_decode_buffers[buf_idx], 0, PHYSICAL_WIDTH * PHYSICAL_HEIGHT * 2);
                esp_lcd_panel_draw_bitmap(panel, 0, 0, PHYSICAL_WIDTH, PHYSICAL_HEIGHT, 
                                          s_decode_buffers[buf_idx]);
            }
        }
        
        /* Display frame */
        if (s_direct_display) {
            esp_lcd_panel_draw_bitmap(panel, 0, 0, frame.width, frame.height, 
                                      s_decode_buffers[buf_idx]);
        } else {
            /* Use PPA for rotation */
            if (s_ppa_handle && s_rotated_buffers[buf_idx]) {
                uint32_t aligned_w = ((frame.width + 15) / 16) * 16;
                
                ppa_srm_oper_config_t ppa_cfg = {
                    .in.buffer = s_decode_buffers[buf_idx],
                    .in.pic_w = aligned_w,
                    .in.pic_h = frame.height,
                    .in.block_w = frame.width,
                    .in.block_h = frame.height,
                    .in.block_offset_x = 0,
                    .in.block_offset_y = 0,
                    .in.srm_cm = PPA_SRM_COLOR_MODE_RGB565,
                    
                    .out.buffer = s_rotated_buffers[buf_idx],
                    .out.buffer_size = s_rotated_buffer_size,
                    .out.pic_w = s_out_w,
                    .out.pic_h = s_out_h,
                    .out.block_offset_x = 0,
                    .out.block_offset_y = 0,
                    .out.srm_cm = PPA_SRM_COLOR_MODE_RGB565,
                    
                    .rotation_angle = PPA_SRM_ROTATION_ANGLE_90,
                    .scale_x = s_scale,
                    .scale_y = s_scale,
                    .rgb_swap = 0,
                    .byte_swap = 0,
                    .mode = PPA_TRANS_MODE_BLOCKING,
                };
                
                ret = ppa_do_scale_rotate_mirror(s_ppa_handle, &ppa_cfg);
                if (ret == ESP_OK) {
                    esp_lcd_panel_draw_bitmap(panel, s_offset_x, s_offset_y,
                                              s_offset_x + s_out_w, s_offset_y + s_out_h,
                                              s_rotated_buffers[buf_idx]);
                }
            }
        }
        
        frames_displayed++;
        s_decode_buf_idx = (buf_idx + 1) % NUM_DECODE_BUFFERS;
        
        /* Update adaptive sync */
        update_adaptive_sync();
        
        /* Calculate next frame time */
        next_frame_time += frame_interval_us;
        
        /* Prevent drift - reset if too far behind */
        now = esp_timer_get_time();
        if (now > next_frame_time + 200000) {
            next_frame_time = now;
        }
        
        /* Log status */
        if (now - last_log_time > 5000000) {
            size_t audio_level = rb_data_available();
            UBaseType_t video_queue = uxQueueMessagesWaiting(s_video_raw_queue);
            ESP_LOGI(TAG, "Video: %lu frames, audio=%uKB, queue=%u, adj=%ldus",
                     (unsigned long)frames_displayed, (unsigned)(audio_level / 1024),
                     (unsigned)video_queue, (long)s_video_delay_adjust_us);
            last_log_time = now;
        }
        
        if (first_frame) {
            first_frame = false;
            ESP_LOGI(TAG, "First video frame displayed");
        }
    }
    
    ESP_LOGI(TAG, "Video task stopped, %lu frames displayed", (unsigned long)frames_displayed);
    s_video_task = NULL;
    vTaskDelete(NULL);
}


/* ============== UI Functions ============== */
static void touch_handler(lv_event_t *e)
{
    (void)e;
    s_stop_requested = true;
    media_player_stop();
}

static void create_video_ui(void)
{
    if (!bsp_display_lock(100)) return;
    
    s_video_mode = true;
    
    lv_disp_t *disp = lv_disp_get_default();
    if (disp) {
        lv_disp_enable_invalidation(disp, false);
    }
    
    s_player_screen = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_player_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(s_player_screen, 0, 0);
    lv_obj_set_style_bg_opa(s_player_screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_player_screen, 0, 0);
    lv_obj_set_style_pad_all(s_player_screen, 0, 0);
    lv_obj_clear_flag(s_player_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_player_screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_player_screen, touch_handler, LV_EVENT_CLICKED, NULL);
    
    bsp_display_unlock();
}

static void destroy_video_ui(void)
{
    bsp_display_lock(100);
    
    if (s_player_screen && lv_obj_is_valid(s_player_screen)) {
        lv_obj_del(s_player_screen);
    }
    s_player_screen = NULL;
    
    if (s_video_mode) {
        lv_disp_t *disp = lv_disp_get_default();
        if (disp) {
            lv_disp_enable_invalidation(disp, true);
            lv_obj_invalidate(lv_scr_act());
        }
        s_video_mode = false;
    }
    
    bsp_display_unlock();
}

/* ============== Resource Allocation ============== */
static esp_err_t allocate_buffers(void)
{
    size_t align = CONFIG_CACHE_L2_CACHE_LINE_SIZE;
    
    /* Audio ring buffer */
    s_audio_rb.buffer = heap_caps_malloc(AUDIO_RING_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (!s_audio_rb.buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio ring buffer");
        return ESP_ERR_NO_MEM;
    }
    s_audio_rb.size = AUDIO_RING_BUFFER_SIZE;
    rb_reset();
    
    /* Video raw frame buffers */
    s_video_raw_queue = xQueueCreate(VIDEO_RAW_QUEUE_SIZE, sizeof(video_raw_frame_t));
    if (!s_video_raw_queue) {
        ESP_LOGE(TAG, "Failed to create video queue");
        return ESP_ERR_NO_MEM;
    }
    
    for (int i = 0; i < VIDEO_RAW_QUEUE_SIZE; i++) {
        s_video_raw_buffers[i] = heap_caps_malloc(VIDEO_RAW_FRAME_MAX_SIZE, MALLOC_CAP_SPIRAM);
        if (!s_video_raw_buffers[i]) {
            ESP_LOGE(TAG, "Failed to allocate video raw buffer %d", i);
            return ESP_ERR_NO_MEM;
        }
    }
    
    /* Decode buffers */
    s_decode_buffer_size = ALIGN_UP_BY(5 * 1024 * 1024, align);
    s_rotated_buffer_size = ALIGN_UP_BY(2 * 1024 * 1024, align);
    
    for (int i = 0; i < NUM_DECODE_BUFFERS; i++) {
        s_decode_buffers[i] = heap_caps_aligned_alloc(align, s_decode_buffer_size,
                                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_decode_buffers[i]) {
            ESP_LOGE(TAG, "Failed to allocate decode buffer %d", i);
            return ESP_ERR_NO_MEM;
        }
        memset(s_decode_buffers[i], 0, s_decode_buffer_size);
        
        s_rotated_buffers[i] = heap_caps_aligned_alloc(align, s_rotated_buffer_size,
                                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_rotated_buffers[i]) {
            ESP_LOGE(TAG, "Failed to allocate rotated buffer %d", i);
            return ESP_ERR_NO_MEM;
        }
        memset(s_rotated_buffers[i], 0, s_rotated_buffer_size);
    }
    
    return ESP_OK;
}

static void free_buffers(void)
{
    if (s_audio_rb.buffer) {
        heap_caps_free(s_audio_rb.buffer);
        s_audio_rb.buffer = NULL;
    }
    s_audio_rb.size = 0;
    
    if (s_video_raw_queue) {
        vQueueDelete(s_video_raw_queue);
        s_video_raw_queue = NULL;
    }
    
    for (int i = 0; i < VIDEO_RAW_QUEUE_SIZE; i++) {
        if (s_video_raw_buffers[i]) {
            heap_caps_free(s_video_raw_buffers[i]);
            s_video_raw_buffers[i] = NULL;
        }
    }
    
    for (int i = 0; i < NUM_DECODE_BUFFERS; i++) {
        if (s_decode_buffers[i]) {
            heap_caps_free(s_decode_buffers[i]);
            s_decode_buffers[i] = NULL;
        }
        if (s_rotated_buffers[i]) {
            heap_caps_free(s_rotated_buffers[i]);
            s_rotated_buffers[i] = NULL;
        }
    }
    s_decode_buffer_size = 0;
    s_rotated_buffer_size = 0;
}

/* ============== Task Management ============== */
static esp_err_t start_tasks(void)
{
    /* Start prefetch task on Core 0 */
    s_prefetch_running = true;
    BaseType_t ret = xTaskCreatePinnedToCore(prefetch_task, "prefetch", PREFETCH_TASK_STACK,
                                              NULL, PREFETCH_TASK_PRIO, &s_prefetch_task, 0);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create prefetch task");
        return ESP_FAIL;
    }
    
    /* Start audio task on Core 0 */
    s_audio_running = true;
    ret = xTaskCreatePinnedToCore(audio_task, "audio", AUDIO_TASK_STACK,
                                   NULL, AUDIO_TASK_PRIO, &s_audio_task, 0);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        return ESP_FAIL;
    }
    
    /* Start video task on Core 1 */
    s_video_running = true;
    ret = xTaskCreatePinnedToCore(video_task, "video", VIDEO_TASK_STACK,
                                   NULL, VIDEO_TASK_PRIO, &s_video_task, 1);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create video task");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

static void stop_tasks(void)
{
    s_prefetch_running = false;
    s_audio_running = false;
    s_video_running = false;
    
    /* Wait for tasks to finish */
    int timeout = 100;
    while ((s_prefetch_task || s_audio_task || s_video_task) && timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    /* Force delete if still running */
    if (s_prefetch_task) {
        vTaskDelete(s_prefetch_task);
        s_prefetch_task = NULL;
    }
    if (s_audio_task) {
        vTaskDelete(s_audio_task);
        s_audio_task = NULL;
    }
    if (s_video_task) {
        vTaskDelete(s_video_task);
        s_video_task = NULL;
    }
}

/* ============== Cleanup ============== */
static void cleanup_playback(void)
{
    stop_tasks();
    
    /* Close AVI file */
    if (s_avi_file) {
        fclose(s_avi_file);
        s_avi_file = NULL;
    }
    
    /* Deinit hardware */
    deinit_jpeg_decoder();
    
    if (s_ppa_handle) {
        ppa_unregister_client(s_ppa_handle);
        s_ppa_handle = NULL;
    }
    
    /* Stop audio codec */
    if (s_audio_configured) {
        bsp_extra_codec_dev_stop();
        s_audio_configured = false;
    }
    
    /* Destroy UI */
    destroy_video_ui();
    
    /* Free buffers */
    free_buffers();
    
    /* Free image buffer if any */
    if (s_image_buffer) {
        heap_caps_free(s_image_buffer);
        s_image_buffer = NULL;
    }
    
    /* Reset state */
    s_avi_movi_start = 0;
    s_avi_movi_size = 0;
    s_avi_read_offset = 0;
    s_video_fps = 25;
    s_total_frames = 0;
    s_audio_sample_rate = 44100;
    s_audio_channels = 2;
    s_audio_bits = 16;
    s_audio_prefill_done = false;
    s_video_frame_index = 0;
    s_audio_bytes_played = 0;
    s_playback_start_time = 0;
    s_video_delay_adjust_us = 0;
    s_scale_valid = false;
    s_direct_display = false;
    s_decode_buf_idx = 0;
    s_raw_buf_write_idx = 0;
    
    memset(&s_media_info, 0, sizeof(s_media_info));
    s_current_path[0] = '\0';
    
    restore_logs();
}


/* ============== Video Playback Entry ============== */
static esp_err_t play_video(const char *filepath)
{
    esp_err_t ret;
    
    suppress_logs();
    
    /* Open AVI file */
    s_avi_file = fopen(filepath, "rb");
    if (!s_avi_file) {
        ESP_LOGE(TAG, "Cannot open file: %s", filepath);
        return ESP_ERR_NOT_FOUND;
    }
    
    /* Parse AVI header */
    ret = parse_avi_header();
    if (ret != ESP_OK) {
        fclose(s_avi_file);
        s_avi_file = NULL;
        return ret;
    }
    
    /* Allocate buffers */
    ret = allocate_buffers();
    if (ret != ESP_OK) {
        cleanup_playback();
        return ret;
    }
    
    /* Initialize JPEG decoder */
    ret = init_jpeg_decoder();
    if (ret != ESP_OK) {
        cleanup_playback();
        return ret;
    }
    
    /* Initialize PPA */
    if (!s_ppa_handle) {
        ppa_client_config_t ppa_cfg = {
            .oper_type = PPA_OPERATION_SRM,
        };
        ret = ppa_register_client(&ppa_cfg, &s_ppa_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "PPA init failed, rotation disabled");
            s_ppa_handle = NULL;
        }
    }
    
    /* Configure audio codec */
    if (s_media_info.has_audio) {
        ret = bsp_extra_codec_set_fs_play(s_audio_sample_rate, s_audio_bits, I2S_SLOT_MODE_STEREO);
        if (ret == ESP_OK) {
            s_audio_configured = true;
            bsp_extra_codec_mute_set(false);
            int vol;
            bsp_extra_codec_volume_set(90, &vol);
        }
    }
    
    /* Create UI */
    create_video_ui();
    
    /* Reset read position */
    s_avi_read_offset = 0;
    s_video_frame_index = 0;
    s_stop_requested = false;
    
    /* Start tasks */
    ret = start_tasks();
    if (ret != ESP_OK) {
        cleanup_playback();
        return ret;
    }
    
    notify_state(MEDIA_PLAYER_STATE_PLAYING);
    
    ESP_LOGI(TAG, "Video playback started: %s", filepath);
    return ESP_OK;
}

/* ============== BMP Image Display ============== */
static void create_image_ui(void)
{
    if (!bsp_display_lock(100)) return;
    
    s_player_screen = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_player_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(s_player_screen, 0, 0);
    lv_obj_set_style_bg_color(s_player_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_player_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_player_screen, 0, 0);
    lv_obj_set_style_pad_all(s_player_screen, 0, 0);
    lv_obj_clear_flag(s_player_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Close button */
    s_close_btn = lv_btn_create(s_player_screen);
    lv_obj_set_size(s_close_btn, 50, 50);
    lv_obj_align(s_close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(s_close_btn, lv_color_hex(0xE53935), 0);
    lv_obj_set_style_radius(s_close_btn, 25, 0);
    lv_obj_add_event_cb(s_close_btn, touch_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *x = lv_label_create(s_close_btn);
    lv_label_set_text(x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x, lv_color_white(), 0);
    lv_obj_center(x);
    
    /* Info label */
    s_info_label = lv_label_create(s_player_screen);
    lv_label_set_text(s_info_label, "Loading...");
    lv_obj_set_style_text_color(s_info_label, lv_color_white(), 0);
    lv_obj_align(s_info_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    bsp_display_unlock();
}

static esp_err_t show_bmp_image(const char *filepath)
{
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open BMP: %s", filepath);
        return ESP_ERR_NOT_FOUND;
    }
    
    uint8_t header[54];
    if (fread(header, 1, 54, f) != 54) {
        fclose(f);
        return ESP_FAIL;
    }
    
    if (header[0] != 'B' || header[1] != 'M') {
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
    
    s_img_dsc.header.always_zero = 0;
    s_img_dsc.header.w = w;
    s_img_dsc.header.h = h;
    s_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_img_dsc.data_size = buf_size;
    s_img_dsc.data = (const uint8_t *)s_image_buffer;
    
    if (bsp_display_lock(100)) {
        s_image_obj = lv_img_create(s_player_screen);
        lv_img_set_src(s_image_obj, &s_img_dsc);
        
        int32_t scale_x = (int32_t)LV_HOR_RES * 256 / w;
        int32_t scale_y = (int32_t)LV_VER_RES * 256 / h;
        int32_t scale = (scale_x < scale_y) ? scale_x : scale_y;
        if (scale < 32) scale = 32;
        if (scale > 512) scale = 512;
        lv_img_set_zoom(s_image_obj, scale);
        lv_obj_center(s_image_obj);
        lv_obj_move_foreground(s_close_btn);
        
        if (s_info_label) {
            char info[64];
            snprintf(info, sizeof(info), "%ldx%ld BMP | %lu KB", 
                     (long)w, (long)h, (unsigned long)(s_media_info.file_size / 1024));
            lv_label_set_text(s_info_label, info);
        }
        
        bsp_display_unlock();
    }
    
    notify_state(MEDIA_PLAYER_STATE_PLAYING);
    return ESP_OK;
}


/* ============== Public API Implementation ============== */

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
    ESP_LOGI(TAG, "Media player initialized (Prefetch + Adaptive Sync)");
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
    s_current_path[sizeof(s_current_path) - 1] = '\0';
    
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
    s_stop_requested = true;
    
    cleanup_playback();
    
    s_stop_requested = false;
    notify_state(MEDIA_PLAYER_STATE_IDLE);
    
    ESP_LOGI(TAG, "Playback stopped");
    return ESP_OK;
}

esp_err_t media_player_pause(void)
{
    /* Not implemented for this version */
    return ESP_OK;
}

esp_err_t media_player_resume(void)
{
    /* Not implemented for this version */
    return ESP_OK;
}

media_player_state_t media_player_get_state(void)
{
    return s_state;
}

bool media_player_is_active(void)
{
    return s_state != MEDIA_PLAYER_STATE_IDLE;
}

bool media_player_is_video_playing(void)
{
    return s_media_info.type == MEDIA_TYPE_AVI && s_state == MEDIA_PLAYER_STATE_PLAYING;
}

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

int media_player_get_volume(void)
{
    return bsp_extra_codec_volume_get();
}

esp_err_t media_player_set_mute(bool mute)
{
    return bsp_extra_codec_mute_set(mute);
}

media_type_t media_player_get_type(const char *filepath)
{
    if (!filepath) return MEDIA_TYPE_UNKNOWN;
    
    const char *ext = strrchr(filepath, '.');
    if (!ext) return MEDIA_TYPE_UNKNOWN;
    ext++;
    
    if (strcasecmp(ext, "png") == 0) return MEDIA_TYPE_PNG;
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return MEDIA_TYPE_JPEG;
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
        case MEDIA_TYPE_PNG: return "PNG";
        case MEDIA_TYPE_JPEG: return "JPEG";
        case MEDIA_TYPE_BMP: return "BMP";
        case MEDIA_TYPE_AVI: return "AVI Video";
        default: return "Unknown";
    }
}
