/*
* Copyright 2025 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "esp_log.h"
#include "bsp/esp32_p4_function_ev_board.h"
#if LV_USE_FREEMASTER
#include "gg_external_data.h"
#endif

static const char *TAG = "GUI_GUIDER";

void ui_init_style(lv_style_t * style)
{
	if (style->prop_cnt > 1)
		lv_style_reset(style);
	else
		lv_style_init(style);
}

/* Flag to prevent re-entrant screen loading */
static bool s_screen_loading = false;

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del)
{
	/* Prevent re-entrant calls - this can happen if user clicks rapidly during screen transitions */
	if (s_screen_loading) {
		ESP_LOGW(TAG, "Screen loading already in progress, ignoring request");
		return;
	}
	s_screen_loading = true;

	lv_disp_t * disp = lv_disp_get_default();
	if (disp == NULL) {
		ESP_LOGE(TAG, "No display found!");
		s_screen_loading = false;
		return;
	}

	ESP_LOGI(TAG, "ui_load_scr_animation: new_scr_del=%d, auto_del=%d", new_scr_del, auto_del);

	/* Get current screen */
	lv_obj_t * act_scr = lv_scr_act();

#if LV_USE_FREEMASTER
	if(auto_del && act_scr != NULL) {
		gg_edata_task_clear(act_scr);
	}
#endif
	/* NOTE: Removed lv_obj_clean(act_scr) - it can cause memory corruption
	 * when timers or other callbacks are still referencing child objects.
	 * Let lv_scr_load_anim handle the deletion with auto_del instead. */

	/* If screen needs to be created, first set pointer to NULL to avoid stale reference */
	if (new_scr_del) {
		*new_scr = NULL;  /* Clear old pointer before creating new screen */
		ESP_LOGI(TAG, "Calling setup_scr to create new screen");
		setup_scr(ui);
		ESP_LOGI(TAG, "Screen created: %p", (void*)*new_scr);
	}

	/* Safety check: Ensure new_scr is valid before loading */
	if (new_scr == NULL || *new_scr == NULL) {
		ESP_LOGE(TAG, "new_scr is NULL after setup!");
		s_screen_loading = false;
		return;
	}

	/* Additional safety: Check if the screen object is valid */
	if (!lv_obj_is_valid(*new_scr)) {
		ESP_LOGE(TAG, "Screen object is invalid: %p", (void*)*new_scr);
		s_screen_loading = false;
		return;
	}

	/* Verify the screen has a valid parent relationship (should be NULL for screens) */
	lv_obj_t * parent = lv_obj_get_parent(*new_scr);
	if (parent != NULL) {
		ESP_LOGE(TAG, "Screen has non-NULL parent: %p, this should not happen!", (void*)parent);
		s_screen_loading = false;
		return;
	}

	/* Re-get current screen */
	act_scr = lv_scr_act();
	ESP_LOGI(TAG, "Loading screen: %p (current: %p)", (void*)*new_scr, (void*)act_scr);

	/* Directly set the new screen as active, bypassing lv_scr_load_anim
	 * This avoids the problematic lv_obj_get_disp() call that can crash */

	/* Send unload events to old screen first */
	if (act_scr != NULL && lv_obj_is_valid(act_scr)) {
		lv_event_send(act_scr, LV_EVENT_SCREEN_UNLOAD_START, NULL);
	}

	/* Send load events to new screen */
	lv_event_send(*new_scr, LV_EVENT_SCREEN_LOAD_START, NULL);

	/* Directly set the active screen - this is the core of screen switching */
	disp->act_scr = *new_scr;
	disp->scr_to_load = NULL;
	disp->prev_scr = NULL;
	disp->del_prev = false;

	/* Send loaded event to new screen */
	lv_event_send(*new_scr, LV_EVENT_SCREEN_LOADED, NULL);

	/* Send unloaded event to old screen */
	if (act_scr != NULL && lv_obj_is_valid(act_scr) && act_scr != *new_scr) {
		lv_event_send(act_scr, LV_EVENT_SCREEN_UNLOADED, NULL);
	}

	/* Invalidate new screen to trigger redraw */
	lv_obj_invalidate(*new_scr);

	/* Delete old screen if auto_del is requested */
	if (auto_del && act_scr != NULL && act_scr != *new_scr) {
		/* Double-check the old screen is still valid before deleting */
		if (lv_obj_is_valid(act_scr)) {
			ESP_LOGI(TAG, "Deleting old screen: %p", (void*)act_scr);
			/* Use lv_obj_del_async to defer deletion to the next LVGL cycle
			 * This allows any pending callbacks to complete before the object is deleted */
			lv_obj_del_async(act_scr);
		} else {
			ESP_LOGW(TAG, "Old screen already invalid, skipping delete");
		}
	}

	*old_scr_del = auto_del;

	s_screen_loading = false;
}

void ui_move_animation(void * var, int32_t duration, int32_t delay, int32_t x_end, int32_t y_end, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);

	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
	lv_anim_set_values(&anim, lv_obj_get_x(var), x_end);
	lv_anim_start(&anim);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
	lv_anim_set_values(&anim, lv_obj_get_y(var), y_end);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void ui_scale_animation(void * var, int32_t duration, int32_t delay, int32_t width, int32_t height, lv_anim_path_cb_t path_cb,
                        uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                        lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);

	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_width);
	lv_anim_set_values(&anim, lv_obj_get_width(var), width);
	lv_anim_start(&anim);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_height);
	lv_anim_set_values(&anim, lv_obj_get_height(var), height);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void ui_img_zoom_animation(void * var, int32_t duration, int32_t delay, int32_t zoom, lv_anim_path_cb_t path_cb,
                           uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                           lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_img_set_zoom);
	lv_anim_set_values(&anim, lv_img_get_zoom(var), zoom);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void ui_img_rotate_animation(void * var, int32_t duration, int32_t delay, lv_coord_t x, lv_coord_t y, int32_t rotate,
                   lv_anim_path_cb_t path_cb, uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                   lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_var(&anim, var);
	lv_anim_set_time(&anim, duration);
	lv_anim_set_delay(&anim, delay);
	lv_img_set_pivot(var, x, y);
	lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_img_set_angle);
	lv_anim_set_values(&anim, 0, rotate*10);
	lv_anim_set_path_cb(&anim, path_cb);
	lv_anim_set_repeat_count(&anim, repeat_cnt);
	lv_anim_set_repeat_delay(&anim, repeat_delay);
	lv_anim_set_playback_time(&anim, playback_time);
	lv_anim_set_playback_delay(&anim, playback_delay);
	lv_anim_set_start_cb(&anim, start_cb);
	lv_anim_set_ready_cb(&anim, ready_cb);
	lv_anim_set_deleted_cb(&anim, deleted_cb);
	lv_anim_start(&anim);
}

void init_scr_del_flag(lv_ui *ui)
{

	ui->scrHome_del = true;
	ui->scrCopy_del = true;
	ui->scrScan_del = true;
	ui->scrPrintMenu_del = true;
	ui->scrPowerSupply_del = true;
	ui->scrAIChat_del = true;
	ui->scrSettings_del = true;
	ui->scrScanFini_del = true;
	ui->scrPrintInternet_del = true;
	ui->scrWirelessSerial_del = true;
	ui->scrOscilloscope_del = true;
}

void setup_ui(lv_ui *ui)
{
	init_scr_del_flag(ui);
	setup_scr_scrHome(ui);
	/* NOTE: Don't load scrHome here - it will be loaded by main.c after boot animation */
}
