/**
 * @file media_player.h
 * @brief Media Player Module - Image viewing and Video playback
 * 
 * Supports:
 * - PNG/BMP image viewing (fullscreen)
 * - AVI video playback (MJPEG/H.264 + PCM audio) using ESP-IDF avi_player
 */

#ifndef MEDIA_PLAYER_H
#define MEDIA_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============== Types ============== */

typedef enum {
    MEDIA_TYPE_UNKNOWN = 0,
    MEDIA_TYPE_PNG,
    MEDIA_TYPE_JPEG,
    MEDIA_TYPE_BMP,
    MEDIA_TYPE_AVI
} media_type_t;

typedef enum {
    MEDIA_PLAYER_STATE_IDLE = 0,
    MEDIA_PLAYER_STATE_LOADING,
    MEDIA_PLAYER_STATE_PLAYING,
    MEDIA_PLAYER_STATE_PAUSED,
    MEDIA_PLAYER_STATE_ERROR
} media_player_state_t;

typedef struct {
    media_type_t type;
    uint32_t width;
    uint32_t height;
    uint32_t file_size;
    uint32_t duration_ms;
    bool has_audio;
    uint32_t audio_sample_rate;
} media_info_t;

typedef void (*media_player_event_cb_t)(media_player_state_t state, void *user_data);

/* ============== API ============== */

esp_err_t media_player_init(void);
void media_player_deinit(void);
void media_player_set_callback(media_player_event_cb_t cb, void *user_data);

esp_err_t media_player_play(const char *filepath);
esp_err_t media_player_stop(void);
esp_err_t media_player_pause(void);
esp_err_t media_player_resume(void);

media_player_state_t media_player_get_state(void);
bool media_player_is_active(void);
bool media_player_is_video_playing(void);
esp_err_t media_player_get_info(media_info_t *info);

esp_err_t media_player_set_volume(int volume);
int media_player_get_volume(void);
esp_err_t media_player_set_mute(bool mute);

media_type_t media_player_get_type(const char *filepath);
bool media_player_is_supported(const char *filepath);
const char* media_player_get_type_name(media_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_PLAYER_H */
