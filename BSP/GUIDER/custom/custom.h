/*
* Copyright 2023 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef __CUSTOM_H_
#define __CUSTOM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"

void custom_init(lv_ui *ui);

void configure_scrHome_scroll(lv_ui *ui);


void slider_adjust_img_cb(lv_obj_t * img, int32_t brightValue, int16_t hueValue);

void update_time_display(lv_timer_t * timer);

void setup_contMain_carousel_effect(lv_ui *ui);

void contMain_scroll_event_cb(lv_event_t * e);

void update_wifi_status(lv_ui *ui, bool is_connected);

void set_wifi_connected_status(bool is_connected);

void restore_wifi_status_on_screen_load(lv_ui *ui);

void sntp_init_task(void *pvParameters);

void create_runtime_display(lv_ui *ui);

void update_runtime_display(lv_timer_t * timer);

void update_data_timestamp(void);

void init_signal_generator(lv_ui *ui);

void deinit_signal_generator(void);

void update_waveform_display(lv_obj_t * chart, int wave_type, uint32_t frequency, uint16_t amplitude);

void init_digital_multimeter(lv_ui *ui);

void stop_digital_multimeter(void);

/* AI Chat functions */
void init_ai_chat(lv_ui *ui);

void ai_chat_add_message(lv_ui *ui, const char* message, bool is_ai);

void ai_chat_add_loading_animation(lv_ui *ui);

void ai_chat_remove_loading_animation(lv_ui *ui);

void ai_chat_start_voice_input(lv_ui *ui);

void ai_chat_stop_voice_input(lv_ui *ui);

const char* ai_chat_get_greeting_by_time(void);

/* OneNet integration functions */
void onenet_ui_update_location_display(void);

void onenet_ui_trigger_location_request(void);

/* Weather display functions */
void weather_ui_update_display(void);

void reset_weather_display_pointers(void);

/* Screenshot functions */
#include "screenshot.h"

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
