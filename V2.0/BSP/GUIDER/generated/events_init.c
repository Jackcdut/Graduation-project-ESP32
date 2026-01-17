/*
* Copyright 2025 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "lvgl.h"
#include "lv_textprogress.h"

#if LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

#include "custom.h"
#include "lv_textprogress.h"

/* WiFi manager includes */
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_log.h"

/* FreeRTOS includes for thread-safe timing */
#include "freertos/FreeRTOS.h"

/* Oscilloscope export module */
#include "oscilloscope_export.h"
#include "freertos/task.h"

/* SD Card Manager */
#include "sdcard_manager.h"
#include "sdcard_manager_ui.h"

/* Cloud Manager */
#include "cloud_manager.h"
#include "cloud_manager_ui.h"

/* BSP includes for backlight control */
#include "bsp/esp32_p4_function_ev_board.h"

/* WiFi scan check timer callback - NON-BLOCKING version */
static void wifi_scan_check_timer_cb(lv_timer_t *timer)
{
	// Get scan results (non-blocking check)
	wifi_ap_record_t ap_records[20];
	uint16_t ap_count = 20;

	extern esp_err_t wifi_manager_get_scan_results(wifi_ap_record_t *, uint16_t *);
	extern void wifi_scan_result_callback(const char *[], int [], int);

	esp_err_t ret = wifi_manager_get_scan_results(ap_records, &ap_count);

	if (ret == ESP_OK) {
		// Scan completed successfully - convert to string arrays for callback
		static const char *ssids[20];
		static int rssi_values[20];

		for (int i = 0; i < ap_count && i < 20; i++) {
			ssids[i] = (const char*)ap_records[i].ssid;
			rssi_values[i] = ap_records[i].rssi;
		}

		wifi_scan_result_callback(ssids, rssi_values, ap_count);

		// Delete timer after successful scan
		if (timer != NULL) {
			lv_timer_del(timer);
		}
	} else if (ret == ESP_ERR_NOT_FINISHED) {
		// Scan still in progress - keep checking (timer will repeat)
		// Do nothing, timer will fire again
	} else {
		// Scan failed or error - stop timer and show error
		ESP_LOGE("WIFI_SCAN", "Scan failed: %s", esp_err_to_name(ret));
		if (timer != NULL) {
			lv_timer_del(timer);
		}

		// Show error in UI
		extern lv_ui guider_ui;
		if (guider_ui.scrSettings_listWifi != NULL) {
			lv_obj_clean(guider_ui.scrSettings_listWifi);
			lv_obj_t * btn = lv_list_add_btn(guider_ui.scrSettings_listWifi, LV_SYMBOL_WARNING, "Scan failed");
			lv_obj_set_style_text_color(lv_obj_get_child(btn, 1), lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
		}

		// Re-enable scan button
		if (guider_ui.scrSettings_btnWifiScan != NULL) {
			lv_obj_clear_state(guider_ui.scrSettings_btnWifiScan, LV_STATE_DISABLED);
			lv_label_set_text(guider_ui.scrSettings_btnWifiScan_label, LV_SYMBOL_REFRESH " Scan");
		}

		// Hide spinner
		if (guider_ui.scrSettings_spinnerWifiScan != NULL) {
			lv_obj_add_flag(guider_ui.scrSettings_spinnerWifiScan, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void scrHome_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		/* Reset weather display pointers to prevent accessing invalid objects */
		reset_weather_display_pointers();

		/* Re-initialize carousel effect when screen is loaded */
		configure_scrHome_scroll(&guider_ui);
		setup_contMain_carousel_effect(&guider_ui);

		/* Restore WiFi status display with animation */
		restore_wifi_status_on_screen_load(&guider_ui);
		break;
	}
	default:
		break;
	}
}
static void scrHome_contPrint_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrPrintMenu, guider_ui.scrPrintMenu_del, &guider_ui.scrHome_del, setup_scr_scrPrintMenu, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
static void scrHome_contCopy_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrCopy, guider_ui.scrCopy_del, &guider_ui.scrHome_del, setup_scr_scrCopy, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
static void scrHome_contScan_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Jump to Oscilloscope screen instead of Scan screen
		ui_load_scr_animation(&guider_ui, &guider_ui.scrOscilloscope, guider_ui.scrOscilloscope_del, &guider_ui.scrHome_del, setup_scr_scrOscilloscope, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
static void scrHome_cont_5_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrPowerSupply, guider_ui.scrPowerSupply_del, &guider_ui.scrHome_del, setup_scr_scrPowerSupply, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
static void scrHome_btn_1_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// 只显示文字，不跳转页面
		break;
	}
	default:
		break;
	}
}
static void scrHome_cont_1_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrWirelessSerial, guider_ui.scrWirelessSerial_del, &guider_ui.scrHome_del, setup_scr_scrWirelessSerial, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
static void scrHome_cont_2_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ESP_LOGI("SETTINGS_BTN", "Settings button clicked! Navigating to scrSettings...");
		ui_load_scr_animation(&guider_ui, &guider_ui.scrSettings, guider_ui.scrSettings_del, &guider_ui.scrHome_del, setup_scr_scrSettings, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	case LV_EVENT_PRESSED:
	{
		ESP_LOGI("SETTINGS_BTN", "Settings button pressed");
		break;
	}
	case LV_EVENT_RELEASED:
	{
		ESP_LOGI("SETTINGS_BTN", "Settings button released");
		break;
	}
	default:
		break;
	}
}
void events_init_scrHome(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrHome, scrHome_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrHome_contPrint, scrHome_contPrint_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrHome_contCopy, scrHome_contCopy_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrHome_contScan, scrHome_contScan_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrHome_cont_5, scrHome_cont_5_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrHome_cont_1, scrHome_cont_1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrHome_cont_2, scrHome_cont_2_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrHome_btn_1, scrHome_btn_1_event_handler, LV_EVENT_ALL, ui);
}
/* Signal generator state variables */
static int current_wave_type = 0;  // 0=sine, 1=triangle, 2=square
static lv_obj_t * scrCopy_slider_amp = NULL;  // Reference to amplitude slider

/* Set amplitude slider reference for safe access in event handlers */
void scrCopy_set_amplitude_slider(lv_obj_t *slider)
{
	scrCopy_slider_amp = slider;
}

static void scrCopy_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		/* Safety check before animation */
		if (guider_ui.scrCopy_contBG && lv_obj_is_valid(guider_ui.scrCopy_contBG)) {
			ui_scale_animation(guider_ui.scrCopy_contBG, 150, 0, 800, 80, &lv_anim_path_ease_out, 0, 0, 0, 0, NULL, NULL, NULL);
		}
		break;
	}
	case LV_EVENT_SCREEN_UNLOADED:
	{
		/* Clean up signal generator resources when leaving screen */
		extern void deinit_signal_generator(void);
		deinit_signal_generator();
		/* Clear amplitude slider reference to prevent use-after-free */
		scrCopy_slider_amp = NULL;
		/* Clear all static references in setup_scr_scrCopy.c */
		scrCopy_clear_static_refs();

		/* Clear guider_ui member pointers for scrCopy to prevent use-after-free */
		guider_ui.scrCopy = NULL;
		guider_ui.scrCopy_contBG = NULL;
		guider_ui.scrCopy_contPanel = NULL;
		guider_ui.scrCopy_imgScanned = NULL;
		guider_ui.scrCopy_sliderBright = NULL;
		guider_ui.scrCopy_sliderHue = NULL;
		guider_ui.scrCopy_btnNext = NULL;
		guider_ui.scrCopy_imgColor = NULL;
		guider_ui.scrCopy_imgBright = NULL;
		guider_ui.scrCopy_btnBack = NULL;
		break;
	}
	default:
		break;
	}
}

/* Frequency slider event handler - Range: 0-2MHz */
static void scrCopy_sliderBright_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	/* Safety check - ensure screen objects are valid */
	if (!guider_ui.scrCopy_sliderBright || !lv_obj_is_valid(guider_ui.scrCopy_sliderBright)) return;

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		/* Get frequency value (0-1000 representing 0Hz-2MHz) */
		int32_t freq_val = lv_slider_get_value(guider_ui.scrCopy_sliderBright);
		uint32_t frequency;
		char freq_str[32];

		/* Convert slider value to frequency:
		 * 0-100: 0-1kHz (step 10Hz), 100-190: 1k-10kHz (step 100Hz)
		 * 190-590: 10k-110kHz (step 250Hz), 590-1000: 110kHz-2MHz */
		if (freq_val <= 100) {
			frequency = freq_val * 10;  // 0-1kHz
			snprintf(freq_str, sizeof(freq_str), "%lu Hz", (unsigned long)frequency);
		} else if (freq_val <= 190) {
			frequency = 1000 + (freq_val - 100) * 100;  // 1k-10kHz
			if (frequency < 10000) {
				snprintf(freq_str, sizeof(freq_str), "%.1f kHz", frequency / 1000.0f);
			} else {
				snprintf(freq_str, sizeof(freq_str), "%.0f kHz", frequency / 1000.0f);
			}
		} else if (freq_val <= 590) {
			frequency = 10000 + (freq_val - 190) * 250;  // 10k-110kHz
			snprintf(freq_str, sizeof(freq_str), "%.1f kHz", frequency / 1000.0f);
		} else if (freq_val >= 1000) {
			frequency = 2000000;  // Clamp to max 2MHz
			snprintf(freq_str, sizeof(freq_str), "2.00 MHz");
		} else {
			/* 590-999: 110kHz to ~2MHz, use interpolation */
			frequency = 110000 + (uint32_t)((freq_val - 590) * 4609.756f);
			if (frequency >= 1000000) {
				snprintf(freq_str, sizeof(freq_str), "%.2f MHz", frequency / 1000000.0f);
			} else {
				snprintf(freq_str, sizeof(freq_str), "%.1f kHz", frequency / 1000.0f);
			}
		}

		/* Update frequency label */
		lv_label_set_text(guider_ui.scrCopy_imgBright, freq_str);

		/* Update frequency input textarea */
		extern void scrCopy_update_freq_input(uint32_t freq_hz);
		scrCopy_update_freq_input(frequency);

		/* Get current amplitude safely */
		uint16_t amplitude = 150;  // Default 1.5V
		if (scrCopy_slider_amp != NULL && lv_obj_is_valid(scrCopy_slider_amp)) {
			amplitude = lv_slider_get_value(scrCopy_slider_amp);
		}

		/* Update waveform display with animation */
		update_waveform_display(guider_ui.scrCopy_imgScanned, current_wave_type, frequency, amplitude);
		break;
	}
	default:
		break;
	}
}

/* Helper function to calculate frequency from slider value (0-2MHz range) */
static uint32_t calc_frequency_from_slider(int32_t freq_val)
{
	if (freq_val <= 100) {
		return freq_val * 10;  // 0-1kHz (step 10Hz)
	} else if (freq_val <= 190) {
		return 1000 + (freq_val - 100) * 100;  // 1k-10kHz (step 100Hz)
	} else if (freq_val <= 590) {
		return 10000 + (freq_val - 190) * 250;  // 10k-110kHz (step 250Hz)
	} else if (freq_val >= 1000) {
		return 2000000;  // Clamp to max 2MHz
	} else {
		/* 590-999: 110kHz to ~2MHz, use interpolation */
		/* (2000000 - 110000) / (1000 - 590) = 1890000 / 410 = 4609.756 */
		return 110000 + (uint32_t)((freq_val - 590) * 4609.756f);
	}
}

/* Helper function to safely get amplitude value */
static uint16_t get_amplitude_value(void)
{
	if (scrCopy_slider_amp != NULL && lv_obj_is_valid(scrCopy_slider_amp)) {
		return lv_slider_get_value(scrCopy_slider_amp);
	}
	return 150;  // Default 1.5V
}

/* Square wave button event handler (reusing sliderHue variable) */
static void scrCopy_sliderHue_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	/* Safety check - ensure screen objects are valid */
	if (!guider_ui.scrCopy_sliderBright || !lv_obj_is_valid(guider_ui.scrCopy_sliderBright)) return;
	if (!guider_ui.scrCopy_imgScanned || !lv_obj_is_valid(guider_ui.scrCopy_imgScanned)) return;

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		/* Update button states */
		if (guider_ui.scrCopy_btnNext && lv_obj_is_valid(guider_ui.scrCopy_btnNext))
			lv_obj_clear_state(guider_ui.scrCopy_btnNext, LV_STATE_CHECKED);  // Sine
		if (guider_ui.scrCopy_imgColor && lv_obj_is_valid(guider_ui.scrCopy_imgColor))
			lv_obj_clear_state(guider_ui.scrCopy_imgColor, LV_STATE_CHECKED);  // Triangle
		if (guider_ui.scrCopy_sliderHue && lv_obj_is_valid(guider_ui.scrCopy_sliderHue))
			lv_obj_add_state(guider_ui.scrCopy_sliderHue, LV_STATE_CHECKED);  // Square

		/* Set wave type to square */
		current_wave_type = 2;

		/* Update waveform with current parameters */
		int32_t freq_val = lv_slider_get_value(guider_ui.scrCopy_sliderBright);
		uint32_t frequency = calc_frequency_from_slider(freq_val);
		uint16_t amplitude = get_amplitude_value();

		update_waveform_display(guider_ui.scrCopy_imgScanned, 2, frequency, amplitude);
		break;
	}
	default:
		break;
	}
}

/* Sine wave button event handler (reusing btnNext variable) */
static void scrCopy_btnNext_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	/* Safety check - ensure screen objects are valid */
	if (!guider_ui.scrCopy_sliderBright || !lv_obj_is_valid(guider_ui.scrCopy_sliderBright)) return;
	if (!guider_ui.scrCopy_imgScanned || !lv_obj_is_valid(guider_ui.scrCopy_imgScanned)) return;

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		/* Update button states */
		if (guider_ui.scrCopy_btnNext && lv_obj_is_valid(guider_ui.scrCopy_btnNext))
			lv_obj_add_state(guider_ui.scrCopy_btnNext, LV_STATE_CHECKED);  // Sine
		if (guider_ui.scrCopy_imgColor && lv_obj_is_valid(guider_ui.scrCopy_imgColor))
			lv_obj_clear_state(guider_ui.scrCopy_imgColor, LV_STATE_CHECKED);  // Triangle
		if (guider_ui.scrCopy_sliderHue && lv_obj_is_valid(guider_ui.scrCopy_sliderHue))
			lv_obj_clear_state(guider_ui.scrCopy_sliderHue, LV_STATE_CHECKED);  // Square

		/* Set wave type to sine */
		current_wave_type = 0;

		/* Update waveform with current parameters */
		int32_t freq_val = lv_slider_get_value(guider_ui.scrCopy_sliderBright);
		uint32_t frequency = calc_frequency_from_slider(freq_val);
		uint16_t amplitude = get_amplitude_value();

		update_waveform_display(guider_ui.scrCopy_imgScanned, 0, frequency, amplitude);
		break;
	}
	default:
		break;
	}
}

/* Triangle wave button event handler (reusing imgColor variable) */
static void scrCopy_imgColor_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	/* Safety check - ensure screen objects are valid */
	if (!guider_ui.scrCopy_sliderBright || !lv_obj_is_valid(guider_ui.scrCopy_sliderBright)) return;
	if (!guider_ui.scrCopy_imgScanned || !lv_obj_is_valid(guider_ui.scrCopy_imgScanned)) return;

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		/* Update button states */
		if (guider_ui.scrCopy_btnNext && lv_obj_is_valid(guider_ui.scrCopy_btnNext))
			lv_obj_clear_state(guider_ui.scrCopy_btnNext, LV_STATE_CHECKED);  // Sine
		if (guider_ui.scrCopy_imgColor && lv_obj_is_valid(guider_ui.scrCopy_imgColor))
			lv_obj_add_state(guider_ui.scrCopy_imgColor, LV_STATE_CHECKED);  // Triangle
		if (guider_ui.scrCopy_sliderHue && lv_obj_is_valid(guider_ui.scrCopy_sliderHue))
			lv_obj_clear_state(guider_ui.scrCopy_sliderHue, LV_STATE_CHECKED);  // Square

		/* Set wave type to triangle */
		current_wave_type = 1;

		/* Update waveform with current parameters */
		int32_t freq_val = lv_slider_get_value(guider_ui.scrCopy_sliderBright);
		uint32_t frequency = calc_frequency_from_slider(freq_val);
		uint16_t amplitude = get_amplitude_value();

		update_waveform_display(guider_ui.scrCopy_imgScanned, 1, frequency, amplitude);
		break;
	}
	default:
		break;
	}
}

static void scrCopy_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrCopy_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}

void events_init_scrCopy(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrCopy, scrCopy_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrCopy_sliderBright, scrCopy_sliderBright_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrCopy_sliderHue, scrCopy_sliderHue_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrCopy_btnNext, scrCopy_btnNext_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrCopy_imgColor, scrCopy_imgColor_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrCopy_btnBack, scrCopy_btnBack_event_handler, LV_EVENT_ALL, ui);
}
static void scrScan_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_scale_animation(guider_ui.scrScan_contBG, 150, 0, 800, 150, &lv_anim_path_ease_out, 0, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	default:
		break;
	}
}
static void scrScan_sliderBright_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		slider_adjust_img_cb(guider_ui.scrScan_imgScanned, lv_slider_get_value(guider_ui.scrScan_sliderBright), lv_slider_get_value(guider_ui.scrScan_sliderHue));
		break;
	}
	default:
		break;
	}
}
static void scrScan_sliderHue_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		slider_adjust_img_cb(guider_ui.scrScan_imgScanned, lv_slider_get_value(guider_ui.scrScan_sliderBright), lv_slider_get_value(guider_ui.scrScan_sliderHue));
		break;
	}
	default:
		break;
	}
}
static void scrScan_btnNext_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// 跳转到主页
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrScan_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
static void scrScan_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrScan_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
void events_init_scrScan(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrScan, scrScan_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrScan_sliderBright, scrScan_sliderBright_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrScan_sliderHue, scrScan_sliderHue_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrScan_btnNext, scrScan_btnNext_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrScan_btnBack, scrScan_btnBack_event_handler, LV_EVENT_ALL, ui);
}
static void scrPrintMenu_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_scale_animation(guider_ui.scrPrintMenu_contBG, 0, 0, 800, 150, &lv_anim_path_ease_out, 0, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	default:
		break;
	}
}
static void scrPrintMenu_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrPrintMenu_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}
void events_init_scrPrintMenu(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrPrintMenu, scrPrintMenu_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrPrintMenu_btnBack, scrPrintMenu_btnBack_event_handler, LV_EVENT_ALL, ui);
}

/* Power Supply state variables */
// Settings (controlled by UI)
static float ps_voltage_set = 5.0f;       // Voltage setting (0-12V)
static float ps_current_set = 0.5f;      // Current limit setting (0-1A)
static bool ps_output_enabled = false;   // Output ON/OFF state

// Actual measurements (updated by ESP32)
static float ps_voltage_actual = 5.0f;   // Actual measured voltage
static float ps_current_actual = 0.24f;  // Actual measured current
static float ps_power_actual = 1.1750f;  // Actual measured power
static bool ps_cc_mode = true;           // true = CC mode, false = CV mode

// Chart data
static int ps_chart_index = 0;

static void scrPowerSupply_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_scale_animation(guider_ui.scrPowerSupply_contBG, 150, 0, 800, 70, &lv_anim_path_ease_out, 0, 0, 0, 0, NULL, NULL, NULL);
		break;
	}
	default:
		break;
	}
}

/* Mode switch handler (CC/CV) */
static void scrPowerSupply_labelModeStatus_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Get the label (first child)
		lv_obj_t * label = lv_obj_get_child(guider_ui.scrPowerSupply_labelModeStatus, 0);

		// Toggle mode based on current state
		if (lv_obj_has_state(guider_ui.scrPowerSupply_labelModeStatus, LV_STATE_CHECKED)) {
			// Currently CC, switch to CV
			ps_cc_mode = false;
			lv_label_set_text(label, "CV");
			lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus,
			                          lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
		} else {
			// Currently CV, switch to CC
			ps_cc_mode = true;
			lv_label_set_text(label, "CC");
			lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus,
			                          lv_color_hex(0xf39c12), LV_PART_MAIN|LV_STATE_DEFAULT);
		}
		// Force refresh
		lv_obj_invalidate(guider_ui.scrPowerSupply_labelModeStatus);

		// NOTE: ESP32 should monitor ps_get_mode() and adjust regulation mode accordingly
		break;
	}
	default:
		break;
	}
}

/* Power switch event handler */
static void scrPowerSupply_switchPower_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		ps_output_enabled = lv_obj_has_state(guider_ui.scrPowerSupply_switchPower, LV_STATE_CHECKED);

		// Get mode button label
		lv_obj_t * mode_label = lv_obj_get_child(guider_ui.scrPowerSupply_labelModeStatus, 0);

		if (!ps_output_enabled) {
			// Output disabled - reset displays to zero
			lv_label_set_text(guider_ui.scrPowerSupply_labelVoltageValue, "0.00");
			lv_label_set_text(guider_ui.scrPowerSupply_labelCurrentValue, "0.00");
			lv_label_set_text(guider_ui.scrPowerSupply_labelPowerValue, "0.0000");

			// Show disabled mode
			lv_label_set_text(mode_label, "--");
			lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus, lv_color_hex(0x95a5a6), LV_PART_MAIN|LV_STATE_DEFAULT);

			// Update chart with zero
			lv_chart_set_next_value(guider_ui.scrPowerSupply_chartPower, guider_ui.scrPowerSupply_chartPowerSeries, 0);
		} else {
			// Output enabled - restore mode display
			if (ps_cc_mode) {
				lv_label_set_text(mode_label, "CC");
				lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus, lv_color_hex(0xf39c12), LV_PART_MAIN|LV_STATE_DEFAULT);
			} else {
				lv_label_set_text(mode_label, "CV");
				lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
		}
		// When enabled, ESP32 will update actual measurements via ps_update_xxx() functions
		// NOTE: ESP32 should monitor ps_get_output_enabled() and start/stop output accordingly
		break;
	}
	default:
		break;
	}
}

/* Voltage slider event handler */
static void scrPowerSupply_sliderVoltage_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		int32_t slider_val = lv_slider_get_value(guider_ui.scrPowerSupply_sliderVoltage);
		ps_voltage_set = slider_val / 100.0f;  // Convert to voltage (0-12.00V)

		char voltage_str[16];
		snprintf(voltage_str, sizeof(voltage_str), "%.2f V", ps_voltage_set);
		lv_label_set_text(guider_ui.scrPowerSupply_labelVoltageSet, voltage_str);

		// NOTE: ESP32 should monitor ps_get_voltage_set() and adjust output accordingly
		// Actual voltage will be updated by ESP32 via ps_update_voltage_actual()
		break;
	}
	default:
		break;
	}
}

/* Current slider event handler */
static void scrPowerSupply_sliderCurrent_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		int32_t slider_val = lv_slider_get_value(guider_ui.scrPowerSupply_sliderCurrent);
		ps_current_set = slider_val / 1000.0f;  // Convert to current (0-1.000A)

		char current_str[16];
		snprintf(current_str, sizeof(current_str), "%.2f A", ps_current_set);
		lv_label_set_text(guider_ui.scrPowerSupply_labelCurrentSet, current_str);

		// NOTE: ESP32 should monitor ps_get_current_set() and adjust current limit accordingly
		// Actual current will be updated by ESP32 via ps_update_current_actual()
		break;
	}
	default:
		break;
	}
}

/* Back button event handler */
static void scrPowerSupply_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrPowerSupply_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}

void events_init_scrPowerSupply(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrPowerSupply, scrPowerSupply_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrPowerSupply_switchPower, scrPowerSupply_switchPower_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrPowerSupply_labelModeStatus, scrPowerSupply_labelModeStatus_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrPowerSupply_sliderVoltage, scrPowerSupply_sliderVoltage_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrPowerSupply_sliderCurrent, scrPowerSupply_sliderCurrent_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrPowerSupply_btnBack, scrPowerSupply_btnBack_event_handler, LV_EVENT_ALL, ui);
}

void events_init(lv_ui *ui)
{

}

/* ========================================
 * ESP32 Interface Functions
 * These functions should be called by ESP32 to update actual measurements
 * ========================================*/

/**
 * @brief Update actual voltage measurement (called by ESP32)
 * @param voltage_actual Measured voltage in volts (0-12V)
 */
void ps_update_voltage_actual(float voltage_actual)
{
	ps_voltage_actual = voltage_actual;
	char voltage_str[16];
	snprintf(voltage_str, sizeof(voltage_str), "%.2f", ps_voltage_actual);
	lv_label_set_text(guider_ui.scrPowerSupply_labelVoltageValue, voltage_str);
}

/**
 * @brief Update actual current measurement (called by ESP32)
 * @param current_actual Measured current in amps (0-1A)
 */
void ps_update_current_actual(float current_actual)
{
	ps_current_actual = current_actual;
	char current_str[16];
	snprintf(current_str, sizeof(current_str), "%.2f", ps_current_actual);
	lv_label_set_text(guider_ui.scrPowerSupply_labelCurrentValue, current_str);
}

/**
 * @brief Update actual power measurement (called by ESP32)
 * @param power_actual Measured power in watts
 */
void ps_update_power_actual(float power_actual)
{
	ps_power_actual = power_actual;
	char power_str[16];
	snprintf(power_str, sizeof(power_str), "%.4f", ps_power_actual);
	lv_label_set_text(guider_ui.scrPowerSupply_labelPowerValue, power_str);

	// Update chart with actual power (scaled by 10 for chart range 0-120)
	lv_chart_set_next_value(guider_ui.scrPowerSupply_chartPower,
	                        guider_ui.scrPowerSupply_chartPowerSeries,
	                        (int)(ps_power_actual * 10));
}

/**
 * @brief Update operating mode display (NOT typically called - mode is set by user button)
 * @param is_cc_mode true for CC (Constant Current) mode, false for CV (Constant Voltage) mode
 * @note This function is provided for compatibility, but mode should be set by user via button
 */
void ps_update_mode(bool is_cc_mode)
{
	ps_cc_mode = is_cc_mode;
	lv_obj_t * label = lv_obj_get_child(guider_ui.scrPowerSupply_labelModeStatus, 0);
	if (is_cc_mode) {
		lv_label_set_text(label, "CC");
		lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus,
		                          lv_color_hex(0xf39c12), LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_state(guider_ui.scrPowerSupply_labelModeStatus, LV_STATE_CHECKED);
	} else {
		lv_label_set_text(label, "CV");
		lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus,
		                          lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_clear_state(guider_ui.scrPowerSupply_labelModeStatus, LV_STATE_CHECKED);
	}
}

/**
 * @brief Update all measurements at once (called by ESP32)
 * @param voltage_actual Measured voltage in volts
 * @param current_actual Measured current in amps
 * @param power_actual Measured power in watts
 * @param is_cc_mode Current operating mode
 */
void ps_update_measurements(float voltage_actual, float current_actual, float power_actual, bool is_cc_mode)
{
	ps_update_voltage_actual(voltage_actual);
	ps_update_current_actual(current_actual);
	ps_update_power_actual(power_actual);
	ps_update_mode(is_cc_mode);
}

/**
 * @brief Get current voltage setting
 * @return Voltage setting in volts
 */
float ps_get_voltage_set(void)
{
	return ps_voltage_set;
}

/**
 * @brief Get current limit setting
 * @return Current limit in amps
 */
float ps_get_current_set(void)
{
	return ps_current_set;
}

/**
 * @brief Get output enable state
 * @return true if output is enabled, false otherwise
 */
bool ps_get_output_enabled(void)
{
	return ps_output_enabled;
}

/**
 * @brief Get current operating mode
 * @return true for CC mode, false for CV mode
 */
bool ps_get_mode(void)
{
	return ps_cc_mode;
}

/* ========================================
 * AI Chat Screen Event Handlers
 * ========================================*/

// 保存当前的键盘对象指针和textarea指针，用于事件处理
// Store current keyboard and textarea for event handling
static lv_obj_t * current_keyboard = NULL;
static lv_obj_t * current_textarea = NULL;

// 辅助函数 - 关闭键盘
// Helper function to close keyboard
static void close_keyboard(void)
{
	if (current_keyboard != NULL) {
		lv_obj_del(current_keyboard);
		current_keyboard = NULL;
		current_textarea = NULL;
	}
}

// 键盘事件处理 - 点击确认或取消按钮时关闭键盘
// Keyboard event handler - close on OK/Cancel button
static void keyboard_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
		// 用户点击了 OK (√) 或 Cancel 按钮
		// User clicked OK (√) or Cancel button
		close_keyboard();
	}
}

// 屏幕点击事件处理 - 点击键盘和输入框外部时关闭键盘
// Screen event handler - close keyboard when clicking outside
static void screen_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_PRESSED) {  // 使用 PRESSED 事件更可靠
		lv_obj_t * target = lv_event_get_target(e);

		// 如果没有键盘，直接返回
		if (current_keyboard == NULL) {
			return;
		}

		// 检查点击目标是否是键盘或其子对象
		bool is_keyboard_or_child = false;
		lv_obj_t * parent = target;
		lv_obj_t * scr = lv_scr_act();

		while (parent != NULL && parent != scr) {
			if (parent == current_keyboard) {
				is_keyboard_or_child = true;
				break;
			}
			parent = lv_obj_get_parent(parent);
		}

		// 检查点击目标是否是textarea或其子对象
		bool is_textarea_or_child = false;
		parent = target;

		while (parent != NULL && parent != scr) {
			if (parent == current_textarea) {
				is_textarea_or_child = true;
				break;
			}
			parent = lv_obj_get_parent(parent);
		}

		// 如果点击的既不是键盘也不是textarea，则关闭键盘
		// If clicked outside keyboard and textarea, close keyboard
		if (!is_keyboard_or_child && !is_textarea_or_child) {
			close_keyboard();
		}
	}
}

static void scrAIChat_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * scr = lv_event_get_target(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		ui_scale_animation(guider_ui.scrAIChat_contBG, 0, 0, 800, 150, &lv_anim_path_ease_out, 0, 0, 0, 0, NULL, NULL, NULL);

		// 初始化AI聊天界面
		// Initialize AI chat with basic setup (no messages for now)
		init_ai_chat(&guider_ui);

		// 注册全局屏幕事件处理器，用于监听点击键盘外部的操作
		// Register global screen event handler for detecting clicks outside keyboard
		lv_obj_add_event_cb(scr, screen_event_handler, LV_EVENT_PRESSED, NULL);
		break;
	}
	case LV_EVENT_SCREEN_UNLOADED:
	{
		// 屏幕卸载时，关闭键盘并清理全局指针
		// Close keyboard and clean up when screen is unloaded
		close_keyboard();
		break;
	}
	default:
		break;
	}
}

static void scrAIChat_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrAIChat_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}

static void scrAIChat_btnVoiceInput_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_PRESSED:
	{
		// Start voice input when button is pressed
		ai_chat_start_voice_input(&guider_ui);
		break;
	}
	case LV_EVENT_RELEASED:
	{
		// Stop voice input when button is released
		ai_chat_stop_voice_input(&guider_ui);

		/* ESP32 Integration Point:
		 * After voice recognition is complete, call:
		 * 1. ai_chat_add_message(&guider_ui, recognized_text, false);  // Add user message
		 * 2. ai_chat_add_loading_animation(&guider_ui);                // Show AI thinking
		 * 3. Send to AI backend for processing
		 * 4. When AI responds:
		 *    - ai_chat_remove_loading_animation(&guider_ui);
		 *    - ai_chat_add_message(&guider_ui, ai_response, true);
		 *
		 * Example demonstration (for testing UI):
		 */

		// Simulate user message after 1 second (for testing)
		// In real implementation, this would come from voice recognition
		// Uncomment below for testing:
		/*
		static bool demo_added = false;
		if (!demo_added) {
			demo_added = true;
			// Add a demo conversation after delay
			// This would be replaced by actual voice recognition callback
		}
		*/
		break;
	}
	default:
		break;
	}
}

static void scrAIChat_btnSend_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Get text from textarea
		const char* text = lv_textarea_get_text(ui->scrAIChat_textAreaInput);

		// 检查输入是否为空或只有空格
		// Check if input is empty or only whitespace
		if (text != NULL && strlen(text) > 0) {
			// 检查是否全是空格
			bool has_content = false;
			for (size_t i = 0; i < strlen(text); i++) {
				if (text[i] != ' ' && text[i] != '\t' && text[i] != '\n' && text[i] != '\r') {
					has_content = true;
					break;
				}
			}

			if (has_content) {
				// Add user message to chat
				ai_chat_add_message(ui, text, false);

				// Clear the input field
				lv_textarea_set_text(ui->scrAIChat_textAreaInput, "");

				// Close keyboard after sending
				close_keyboard();

				// Show AI thinking animation
				ai_chat_add_loading_animation(ui);

				/* ESP32 Integration Point:
				 * Send the user's message to AI backend for processing
				 * When response is received, call:
				 * 1. ai_chat_remove_loading_animation(ui);
				 * 2. ai_chat_add_message(ui, ai_response, true);
				 */
			}
		}
		break;
	}
	default:
		break;
	}
}

// 文本输入框事件处理 - 点击时显示键盘
// Textarea event handler - show keyboard when clicked
static void scrAIChat_textAreaInput_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// 如果键盘不存在，则创建键盘
		// Create keyboard if it doesn't exist
		if (current_keyboard == NULL) {
			lv_obj_t * scr = lv_scr_act();

			// 创建键盘
			// Create keyboard
			current_keyboard = lv_keyboard_create(scr);
			current_textarea = ui->scrAIChat_textAreaInput;

			lv_keyboard_set_textarea(current_keyboard, current_textarea);
			lv_keyboard_set_mode(current_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);

			// 设置键盘位置和大小
			// Position keyboard at bottom
			lv_obj_set_size(current_keyboard, 800, 240);
			lv_obj_align(current_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

			// 键盘样式设置
			// Style the keyboard
			lv_obj_set_style_bg_color(current_keyboard, lv_color_hex(0xf0f0f0), LV_PART_MAIN);
			lv_obj_set_style_text_color(current_keyboard, lv_color_hex(0x2c3e50), LV_PART_MAIN);

			// 添加键盘事件处理（监听OK和Cancel按钮）
			// Add event handler for keyboard buttons
			lv_obj_add_event_cb(current_keyboard, keyboard_event_handler, LV_EVENT_ALL, NULL);
		}
		break;
	}
	default:
		break;
	}
}

// 删除按钮事件处理 - 删除一个字符
// Delete button event handler - Delete one character at a time
static void scrAIChat_btnDelete_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// 删除输入框中的最后一个字符
		// Delete the last character in the text area
		lv_textarea_del_char(ui->scrAIChat_textAreaInput);
		break;
	}
	default:
		break;
	}
}

void events_init_scrAIChat(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrAIChat, scrAIChat_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrAIChat_btnBack, scrAIChat_btnBack_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrAIChat_textAreaInput, scrAIChat_textAreaInput_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrAIChat_btnVoiceInput, scrAIChat_btnVoiceInput_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrAIChat_btnDelete, scrAIChat_btnDelete_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrAIChat_btnSend, scrAIChat_btnSend_event_handler, LV_EVENT_ALL, ui);
}

/* ========================================
 * Settings Screen Event Handlers
 * ========================================*/

// Current selected menu (0=Brightness, 1=WiFi, 2=About)
static int current_settings_menu = 0;

// WiFi state management
static bool wifi_scanning = false;
static char wifi_ssid_buffer[64] = "";
static char wifi_password_buffer[64] = "";

// Track where to return from scrPrintInternet (0=PrintMenu, 1=Settings)
static int scrPrintInternet_return_target = 0;

/* WiFi status flags - safe cross-task communication */
typedef enum {
	WIFI_STATUS_IDLE = 0,
	WIFI_STATUS_CONNECTING,
	WIFI_STATUS_CONNECTED,
	WIFI_STATUS_FAILED
} wifi_status_flag_t;

static volatile wifi_status_flag_t s_wifi_status_pending = WIFI_STATUS_IDLE;
static char s_wifi_pending_ssid[64] = {0};
static lv_timer_t *s_wifi_status_poll_timer = NULL;
static uint32_t s_wifi_status_change_tick = 0;

/* Forward declaration of poll timer callback */
static void wifi_status_poll_timer_cb(lv_timer_t * timer);

/* Screen transition callbacks removed - staying on Settings screen */

/* SNTP removed - using OneNet cloud time instead */

// Helper function to switch menu panels with animation
static void switch_settings_panel(lv_ui *ui, int menu_index)
{
	if (current_settings_menu == menu_index) return;

	// Update menu buttons state
	lv_obj_clear_state(ui->scrSettings_btnMenuBrightness, LV_STATE_CHECKED);
	lv_obj_clear_state(ui->scrSettings_btnMenuWifi, LV_STATE_CHECKED);
	lv_obj_clear_state(ui->scrSettings_btnMenuAbout, LV_STATE_CHECKED);
	lv_obj_clear_state(ui->scrSettings_btnMenuGallery, LV_STATE_CHECKED);
	if (ui->scrSettings_btnMenuCloud) {
		lv_obj_clear_state(ui->scrSettings_btnMenuCloud, LV_STATE_CHECKED);
	}

	// Fade out current panel
	lv_obj_t *current_panel = NULL;
	switch (current_settings_menu) {
		case 0: current_panel = ui->scrSettings_contBrightnessPanel; break;
		case 1: current_panel = ui->scrSettings_contWifiPanel; break;
		case 2: current_panel = ui->scrSettings_contAboutPanel; break;
		case 3: 
			/* SD Card Manager is full-screen overlay - destroy it */
			sdcard_manager_ui_destroy();
			current_panel = NULL;
			break;
		case 4:
			/* Cloud Manager - destroy UI */
			cloud_manager_ui_destroy();
			current_panel = ui->scrSettings_contCloudPanel;
			break;
	}

	if (current_panel) {
		lv_obj_add_flag(current_panel, LV_OBJ_FLAG_HIDDEN);
	}

	// Fade in new panel with animation
	lv_obj_t *new_panel = NULL;
	switch (menu_index) {
		case 0:
			new_panel = ui->scrSettings_contBrightnessPanel;
			lv_obj_add_state(ui->scrSettings_btnMenuBrightness, LV_STATE_CHECKED);
			break;
		case 1:
			new_panel = ui->scrSettings_contWifiPanel;
			lv_obj_add_state(ui->scrSettings_btnMenuWifi, LV_STATE_CHECKED);
			break;
		case 2:
			new_panel = ui->scrSettings_contAboutPanel;
			lv_obj_add_state(ui->scrSettings_btnMenuAbout, LV_STATE_CHECKED);
			break;
		case 3:
			/* SD Card Manager - create full-screen overlay UI */
			lv_obj_add_state(ui->scrSettings_btnMenuGallery, LV_STATE_CHECKED);
			/* First destroy existing UI if any */
			sdcard_manager_ui_destroy();
			/* Create full-screen SD Card Manager UI (overlays on screen) */
			sdcard_manager_ui_create(NULL);
			/* Don't set new_panel - the UI is a full-screen overlay */
			new_panel = NULL;
			break;
		case 4:
			/* Cloud Manager - create UI in panel */
			if (ui->scrSettings_btnMenuCloud) {
				lv_obj_add_state(ui->scrSettings_btnMenuCloud, LV_STATE_CHECKED);
			}
			/* First show the cloud panel */
			if (ui->scrSettings_contCloudPanel) {
				lv_obj_clear_flag(ui->scrSettings_contCloudPanel, LV_OBJ_FLAG_HIDDEN);
			}
			/* Create Cloud Manager UI inside the panel */
			cloud_manager_ui_create(ui->scrSettings_contCloudPanel);
			/* Don't set new_panel - we already showed it */
			new_panel = NULL;
			break;
	}

	if (new_panel) {
		lv_obj_clear_flag(new_panel, LV_OBJ_FLAG_HIDDEN);

		// Slide in animation
		lv_obj_set_x(new_panel, 50);
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_var(&a, new_panel);
		lv_anim_set_values(&a, 50, 0);
		lv_anim_set_time(&a, 300);
		lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
		lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
		lv_anim_start(&a);
	}

	current_settings_menu = menu_index;
}

static void scrSettings_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		// Initialize menu state - default to Brightness
		current_settings_menu = 0;
		lv_obj_add_state(guider_ui.scrSettings_btnMenuBrightness, LV_STATE_CHECKED);
		lv_obj_clear_flag(guider_ui.scrSettings_contBrightnessPanel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(guider_ui.scrSettings_contWifiPanel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(guider_ui.scrSettings_contAboutPanel, LV_OBJ_FLAG_HIDDEN);

		// Animate top bar and content panels on screen load
		ui_scale_animation(guider_ui.scrSettings_contBG, 0, 0, 800, 70, &lv_anim_path_ease_out, 0, 0, 0, 0, NULL, NULL, NULL);

		// Slide in left menu
		lv_obj_set_x(guider_ui.scrSettings_contLeft, -240);
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_var(&a, guider_ui.scrSettings_contLeft);
		lv_anim_set_values(&a, -240, 20);
		lv_anim_set_time(&a, 400);
		lv_anim_set_delay(&a, 100);
		lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
		lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
		lv_anim_start(&a);

		// Slide in right content
		lv_obj_set_x(guider_ui.scrSettings_contRight, 800);
		lv_anim_t b;
		lv_anim_init(&b);
		lv_anim_set_var(&b, guider_ui.scrSettings_contRight);
		lv_anim_set_values(&b, 800, 260);
		lv_anim_set_time(&b, 400);
		lv_anim_set_delay(&b, 0);
		lv_anim_set_exec_cb(&b, (lv_anim_exec_xcb_t)lv_obj_set_x);
		lv_anim_set_path_cb(&b, lv_anim_path_ease_out);
		lv_anim_start(&b);

		// Reset to brightness panel
		current_settings_menu = 0;

		/* Start WiFi status polling timer (100ms interval) */
		if (s_wifi_status_poll_timer == NULL) {
			s_wifi_status_poll_timer = lv_timer_create(wifi_status_poll_timer_cb, 100, NULL);
		}
		break;
	}
	case LV_EVENT_SCREEN_UNLOADED:
	{
		/* Stop WiFi status polling timer when leaving Settings */
		if (s_wifi_status_poll_timer != NULL) {
			lv_timer_del(s_wifi_status_poll_timer);
			s_wifi_status_poll_timer = NULL;
		}
		break;
	}
	default:
		break;
	}
}

static void scrSettings_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrSettings_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}

/* Menu button event handlers */
static void scrSettings_btnMenuBrightness_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		switch_settings_panel(ui, 0);
		break;
	}
	default:
		break;
	}
}

static void scrSettings_btnMenuWifi_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		switch_settings_panel(ui, 1);
		break;
	}
	default:
		break;
	}
}

static void scrSettings_btnMenuAbout_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		switch_settings_panel(ui, 2);
		break;
	}
	default:
		break;
	}
}

static void scrSettings_btnMenuGallery_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		switch_settings_panel(ui, 3);
		break;
	}
	default:
		break;
	}
}

static void scrSettings_btnMenuCloud_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		switch_settings_panel(ui, 4);
		break;
	}
	default:
		break;
	}
}


/* Brightness slider event handler */
static void scrSettings_sliderBrightness_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		int32_t brightness = lv_slider_get_value(ui->scrSettings_sliderBrightness);
		char brightness_str[16];
		snprintf(brightness_str, sizeof(brightness_str), "%d%%", (int)brightness);
		lv_label_set_text(ui->scrSettings_labelBrightnessValue, brightness_str);

		/* Call ESP32 BSP function to adjust actual screen brightness */
		bsp_display_brightness_set((int)brightness);
		ESP_LOGI("SETTINGS", "Brightness set to %d%%", (int)brightness);
		break;
	}
	default:
		break;
	}
}

/* WiFi scan button event handler */
static void scrSettings_btnWifiScan_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		if (wifi_scanning) return;  // Already scanning

		wifi_scanning = true;

		// Show scanning animation
		lv_obj_clear_flag(ui->scrSettings_spinnerWifiScan, LV_OBJ_FLAG_HIDDEN);

		// Disable scan button during scan
		lv_obj_add_state(ui->scrSettings_btnWifiScan, LV_STATE_DISABLED);
		lv_label_set_text(ui->scrSettings_btnWifiScan_label, "Scanning...");

		// Clear current list
		lv_obj_clean(ui->scrSettings_listWifi);

		/* Start WiFi scan using wifi_manager */
		extern esp_err_t wifi_manager_scan_start(void (*)(uint16_t));
		extern esp_err_t wifi_manager_get_scan_results(wifi_ap_record_t *, uint16_t *);
		extern void wifi_scan_result_callback(const char *[], int [], int);

		// Start scan (callback will be triggered when done)
		esp_err_t scan_ret = wifi_manager_scan_start(NULL);

		if (scan_ret != ESP_OK) {
			// Scan failed to start - reset state and show error
			ESP_LOGE("WIFI_SCAN", "Failed to start scan: %s", esp_err_to_name(scan_ret));
			wifi_scanning = false;
			lv_obj_add_flag(ui->scrSettings_spinnerWifiScan, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_state(ui->scrSettings_btnWifiScan, LV_STATE_DISABLED);
			lv_label_set_text(ui->scrSettings_btnWifiScan_label, LV_SYMBOL_REFRESH " Scan");

			// Show error message
			lv_obj_t * btn = lv_list_add_btn(ui->scrSettings_listWifi, LV_SYMBOL_WARNING, "Scan failed - WiFi not initialized");
			lv_obj_set_style_text_color(lv_obj_get_child(btn, 1), lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
			break;
		}

		// Add scanning message
		lv_obj_t * btn = lv_list_add_btn(ui->scrSettings_listWifi, LV_SYMBOL_WIFI, "Scanning...");
		lv_obj_set_style_text_color(lv_obj_get_child(btn, 1), lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);

		// Create a REPEATING timer to check scan results (non-blocking polling)
		// Timer will check every 500ms until scan completes or times out
		// Maximum 20 checks = 10 seconds total timeout
		lv_timer_t *scan_check_timer = lv_timer_create(wifi_scan_check_timer_cb, 500, NULL);
		lv_timer_set_repeat_count(scan_check_timer, 20);  // Check up to 20 times (10 seconds max)

		break;
	}
	default:
		break;
	}
}

// WiFi dialog keyboard pointer
static lv_obj_t * wifi_keyboard = NULL;
static lv_obj_t * wifi_active_textarea = NULL;

// Helper function to close WiFi keyboard
static void close_wifi_keyboard(void)
{
	if (wifi_keyboard != NULL) {
		lv_obj_del(wifi_keyboard);
		wifi_keyboard = NULL;
		wifi_active_textarea = NULL;
	}
}

// WiFi keyboard event handler
static void wifi_keyboard_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
		close_wifi_keyboard();
	}
}

// Helper function to show WiFi keyboard
static void show_wifi_keyboard(lv_ui *ui, lv_obj_t *textarea)
{
	if (wifi_keyboard == NULL) {
		wifi_keyboard = lv_keyboard_create(ui->scrSettings);
		lv_keyboard_set_textarea(wifi_keyboard, textarea);
		lv_keyboard_set_mode(wifi_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
		lv_obj_set_size(wifi_keyboard, 750, 240);
		lv_obj_align(wifi_keyboard, LV_ALIGN_BOTTOM_MID, 0, -10);
		lv_obj_set_style_bg_color(wifi_keyboard, lv_color_hex(0xf0f0f0), LV_PART_MAIN);
		wifi_active_textarea = textarea;

		// Add keyboard event to close on OK/Cancel
		lv_obj_add_event_cb(wifi_keyboard, wifi_keyboard_event_handler, LV_EVENT_ALL, NULL);
	} else {
		lv_keyboard_set_textarea(wifi_keyboard, textarea);
		wifi_active_textarea = textarea;
	}
}

/* WiFi custom input button event handler */
static void scrSettings_btnWifiCustom_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Show WiFi connection dialog
		lv_obj_clear_flag(ui->scrSettings_contWifiDialog, LV_OBJ_FLAG_HIDDEN);

		// Clear previous inputs
		lv_textarea_set_text(ui->scrSettings_textareaSSID, "");
		lv_textarea_set_text(ui->scrSettings_textareaPassword, "");

		// Move dialog to front
		lv_obj_move_foreground(ui->scrSettings_contWifiDialog);

		break;
	}
	default:
		break;
	}
}

/* WiFi Disconnect button timer callback */
static void wifi_disconnect_timer_cb(lv_timer_t * t)
{
	lv_ui *ui = (lv_ui*)t->user_data;
	lv_obj_set_style_bg_color(ui->scrSettings_btnWifiDisconnect, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_label_set_text(ui->scrSettings_btnWifiDisconnect_label, LV_SYMBOL_CLOSE " Disconnect");
	lv_timer_del(t);
}

/* WiFi Disconnect button event handler */
static void scrSettings_btnWifiDisconnect_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Visual feedback - change button color temporarily
		lv_obj_set_style_bg_color(ui->scrSettings_btnWifiDisconnect, lv_color_hex(0xc0392b), LV_PART_MAIN|LV_STATE_DEFAULT);
		
		// Disconnect WiFi
		esp_wifi_disconnect();
		
		// Update button text to show action
		lv_label_set_text(ui->scrSettings_btnWifiDisconnect_label, "Disconnecting...");
		
		// Create a timer to restore button state after 1 second
		lv_timer_create(wifi_disconnect_timer_cb, 1000, ui);
		
		break;
	}
	default:
		break;
	}
}

/* WiFi SSID textarea event handler */
static void scrSettings_textareaSSID_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		show_wifi_keyboard(ui, ui->scrSettings_textareaSSID);
		break;
	}
	default:
		break;
	}
}

/* WiFi Password textarea event handler */
static void scrSettings_textareaPassword_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		show_wifi_keyboard(ui, ui->scrSettings_textareaPassword);
		break;
	}
	default:
		break;
	}
}

/* WiFi Connect button event handler */
static void scrSettings_btnWifiConnect_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		const char *ssid = lv_textarea_get_text(ui->scrSettings_textareaSSID);
		const char *password = lv_textarea_get_text(ui->scrSettings_textareaPassword);

		// Validate input
		if (strlen(ssid) == 0) {
			lv_label_set_text(ui->scrSettings_labelWifiStatus, "Error: SSID is empty");
			lv_obj_set_style_text_color(ui->scrSettings_labelWifiStatus, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
			return;
		}

		// Close keyboard and dialog
		close_wifi_keyboard();
		lv_obj_add_flag(ui->scrSettings_contWifiDialog, LV_OBJ_FLAG_HIDDEN);

		// Update status
		char status_text[128];
		snprintf(status_text, sizeof(status_text), "Connecting to %s...", ssid);
		lv_label_set_text(ui->scrSettings_labelWifiStatus, status_text);
		lv_obj_set_style_text_color(ui->scrSettings_labelWifiStatus, lv_color_hex(0xf39c12), LV_PART_MAIN|LV_STATE_DEFAULT);

		/* Set connecting state flag */
		s_wifi_status_pending = WIFI_STATUS_CONNECTING;
		s_wifi_status_change_tick = lv_tick_get();

		/* Connect to WiFi using wifi_manager */
		extern esp_err_t wifi_manager_connect(const char *, const char *, void (*)(bool, const char *));
		extern void onenet_wifi_status_callback(bool, const char *);

		// Start WiFi connection with OneNet callback
		esp_err_t ret = wifi_manager_connect(ssid, strlen(password) > 0 ? password : NULL, onenet_wifi_status_callback);
		if (ret != ESP_OK) {
			lv_label_set_text(ui->scrSettings_labelWifiStatus, "Connection failed!");
			lv_obj_set_style_text_color(ui->scrSettings_labelWifiStatus, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
			s_wifi_status_pending = WIFI_STATUS_FAILED;
		}

		break;
	}
	default:
		break;
	}
}

/* WiFi Cancel button event handler */
static void scrSettings_btnWifiCancel_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Close keyboard and dialog
		close_wifi_keyboard();
		lv_obj_add_flag(ui->scrSettings_contWifiDialog, LV_OBJ_FLAG_HIDDEN);
		break;
	}
	default:
		break;
	}
}

static void scrSettings_switchUSB_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui*)lv_event_get_user_data(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		/* USB switch toggled */
		extern void gallery_usb_switch_changed(lv_ui *ui, bool enabled);
		bool enabled = lv_obj_has_state(ui->scrSettings_switchUSB, LV_STATE_CHECKED);
		gallery_usb_switch_changed(ui, enabled);
		break;
	}
	default:
		break;
	}
}



void events_init_scrSettings(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrSettings, scrSettings_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnBack, scrSettings_btnBack_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnMenuBrightness, scrSettings_btnMenuBrightness_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnMenuWifi, scrSettings_btnMenuWifi_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnMenuAbout, scrSettings_btnMenuAbout_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnMenuGallery, scrSettings_btnMenuGallery_event_handler, LV_EVENT_ALL, ui);
	if (ui->scrSettings_btnMenuCloud) {
		lv_obj_add_event_cb(ui->scrSettings_btnMenuCloud, scrSettings_btnMenuCloud_event_handler, LV_EVENT_ALL, ui);
	}
	lv_obj_add_event_cb(ui->scrSettings_sliderBrightness, scrSettings_sliderBrightness_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnWifiScan, scrSettings_btnWifiScan_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnWifiCustom, scrSettings_btnWifiCustom_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnWifiDisconnect, scrSettings_btnWifiDisconnect_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_textareaSSID, scrSettings_textareaSSID_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_textareaPassword, scrSettings_textareaPassword_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnWifiConnect, scrSettings_btnWifiConnect_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_btnWifiCancel, scrSettings_btnWifiCancel_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrSettings_switchUSB, scrSettings_switchUSB_event_handler, LV_EVENT_ALL, ui);

	/* Initialize gallery module */
	extern esp_err_t gallery_init(lv_ui *ui);
	gallery_init(ui);

}

/* ========================================
 * Success/Finish Screen Event Handlers
 * (Reused for WiFi connection success)
 * ========================================*/

static void scrScanFini_btnNxet_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		/* === SAFE: Only set flag and start SNTP, let screen load event handle UI update === */

		/* 1. Update WiFi status FLAG (will be applied by restore_wifi_status_on_screen_load) */
		extern void set_wifi_connected_status(bool is_connected);
		set_wifi_connected_status(true);

		/* 2. SNTP will be initialized AFTER returning to home screen (via delayed timer) */
		/* This avoids CPU overload during screen transition */

		/* 3. Return to Home screen - screen load event will update WiFi icon + start delayed SNTP */
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrScanFini_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}

void events_init_scrScanFini(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrScanFini_btnNxet, scrScanFini_btnNxet_event_handler, LV_EVENT_ALL, ui);
}

/* ========================================
 * ESP32 WiFi Interface Functions
 * These functions should be called by ESP32 after WiFi operations
 * ========================================*/

/* WiFi network list item click handler */
static void wifi_network_item_clicked(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t *btn = lv_event_get_target(e);

		/* Get SSID from button user data */
		const char *ssid = (const char *)lv_obj_get_user_data(btn);
		if (ssid == NULL) return;

		/* Copy SSID to input field and show dialog */
		lv_textarea_set_text(guider_ui.scrSettings_textareaSSID, ssid);
		lv_textarea_set_text(guider_ui.scrSettings_textareaPassword, "");

		/* Show WiFi connection dialog */
		lv_obj_clear_flag(guider_ui.scrSettings_contWifiDialog, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_foreground(guider_ui.scrSettings_contWifiDialog);

		/* Update status to show which network will be connected */
		char status_text[128];
		snprintf(status_text, sizeof(status_text), "Selected: %s", ssid);
		lv_label_set_text(guider_ui.scrSettings_labelWifiStatus, status_text);
		lv_obj_set_style_text_color(guider_ui.scrSettings_labelWifiStatus, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
	}
}

/**
 * @brief Update WiFi scan results (called by ESP32 after scan completes)
 * @param wifi_networks Array of WiFi network names
 * @param signal_strengths Array of signal strengths (RSSI)
 * @param count Number of networks found
 */
void wifi_scan_result_callback(const char *wifi_networks[], int signal_strengths[], int count)
{
	wifi_scanning = false;

	// Hide scanning animation
	lv_obj_add_flag(guider_ui.scrSettings_spinnerWifiScan, LV_OBJ_FLAG_HIDDEN);

	// Re-enable scan button
	lv_obj_clear_state(guider_ui.scrSettings_btnWifiScan, LV_STATE_DISABLED);
	lv_label_set_text(guider_ui.scrSettings_btnWifiScan_label, LV_SYMBOL_REFRESH " Scan");

	// Clear list
	lv_obj_clean(guider_ui.scrSettings_listWifi);

	if (count == 0) {
		lv_obj_t * btn = lv_list_add_btn(guider_ui.scrSettings_listWifi, LV_SYMBOL_WARNING, "No networks found");
		lv_obj_set_style_text_color(lv_obj_get_child(btn, 1), lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
	} else {
		// Static storage for SSID strings (needed because pointers must remain valid)
		static char stored_ssids[10][64];

		// Add WiFi networks to list
		for (int i = 0; i < count && i < 10; i++) {  // Limit to 10 networks
			char item_text[128];
			const char *signal_icon = LV_SYMBOL_WIFI;

			// Store SSID in static buffer
			strncpy(stored_ssids[i], wifi_networks[i], sizeof(stored_ssids[i]) - 1);
			stored_ssids[i][sizeof(stored_ssids[i]) - 1] = '\0';

			// Determine signal strength icon and color
			lv_color_t signal_color;
			if (signal_strengths[i] > -50) {
				signal_icon = LV_SYMBOL_WIFI;  // Strong
				signal_color = lv_color_hex(0x27ae60);  // Green
			} else if (signal_strengths[i] > -70) {
				signal_icon = LV_SYMBOL_WIFI;  // Medium
				signal_color = lv_color_hex(0xf39c12);  // Orange
			} else {
				signal_icon = LV_SYMBOL_WIFI;  // Weak
				signal_color = lv_color_hex(0xe74c3c);  // Red
			}

			snprintf(item_text, sizeof(item_text), "%s (%d dBm)", wifi_networks[i], signal_strengths[i]);
			lv_obj_t * btn = lv_list_add_btn(guider_ui.scrSettings_listWifi, signal_icon, item_text);

			// Color code the signal strength
			lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), signal_color, LV_PART_MAIN|LV_STATE_DEFAULT);

			// Store SSID pointer in user data for later connection
			lv_obj_set_user_data(btn, (void*)stored_ssids[i]);

			// Add click event to open connection dialog
			lv_obj_add_event_cb(btn, wifi_network_item_clicked, LV_EVENT_CLICKED, NULL);
		}
	}
}

/* LVGL task polling timer - checks WiFi status flags and updates UI safely */
static void wifi_status_poll_timer_cb(lv_timer_t * timer)
{
	wifi_status_flag_t current_status = s_wifi_status_pending;

	/* No pending status change */
	if (current_status == WIFI_STATUS_IDLE || current_status == WIFI_STATUS_CONNECTING) {
		return;
	}

	/* Check if enough time has passed (debounce) - use FreeRTOS ticks */
	uint32_t current_tick = (uint32_t)xTaskGetTickCount();
	uint32_t elapsed_ticks = current_tick - s_wifi_status_change_tick;
	/* Convert ticks to ms: 1 tick = 1ms typically on ESP32 */
	if (elapsed_ticks < pdMS_TO_TICKS(300)) {
		return; /* Wait at least 300ms before processing */
	}

	/* === Process WiFi status change in LVGL task context (SAFE!) === */

	if (current_status == WIFI_STATUS_CONNECTED) {
		/* CONNECTION SUCCESSFUL */

		/* Update status label */
		if (guider_ui.scrSettings_labelWifiStatus != NULL && lv_obj_is_valid(guider_ui.scrSettings_labelWifiStatus)) {
			char status_text[128];
			snprintf(status_text, sizeof(status_text), "Connected: %s", s_wifi_pending_ssid);
			lv_label_set_text(guider_ui.scrSettings_labelWifiStatus, status_text);
			lv_obj_set_style_text_color(guider_ui.scrSettings_labelWifiStatus, lv_color_hex(0x27ae60), LV_PART_MAIN|LV_STATE_DEFAULT);
		}

		/* === DEFER HEAVY OPERATIONS UNTIL USER RETURNS TO HOME === */

		/* Clear pending flag */
		s_wifi_status_pending = WIFI_STATUS_IDLE;

		/* Stop polling timer */
		if (s_wifi_status_poll_timer != NULL) {
			lv_timer_del(s_wifi_status_poll_timer);
			s_wifi_status_poll_timer = NULL;
		}

		/* ONLY jump to success page - NO SNTP, NO icon update yet */
		/* These will be triggered when user clicks Continue button */
		if (lv_scr_act() == guider_ui.scrSettings) {
			ui_load_scr_animation(&guider_ui, &guider_ui.scrScanFini, guider_ui.scrScanFini_del, &guider_ui.scrSettings_del, setup_scr_scrScanFini, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
		}

	} else if (current_status == WIFI_STATUS_FAILED) {
		/* CONNECTION FAILED */

		/* Update status label */
		if (guider_ui.scrSettings_labelWifiStatus != NULL && lv_obj_is_valid(guider_ui.scrSettings_labelWifiStatus)) {
			lv_label_set_text(guider_ui.scrSettings_labelWifiStatus, "Connection Failed");
			lv_obj_set_style_text_color(guider_ui.scrSettings_labelWifiStatus, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
		}

		/* === DEFER HEAVY OPERATIONS UNTIL USER RETURNS TO HOME === */

		/* Clear pending flag */
		s_wifi_status_pending = WIFI_STATUS_IDLE;

		/* Stop polling timer */
		if (s_wifi_status_poll_timer != NULL) {
			lv_timer_del(s_wifi_status_poll_timer);
			s_wifi_status_poll_timer = NULL;
		}

		/* ONLY jump to error page - NO icon update yet */
		/* Icon will be updated when user clicks Back button */
		if (lv_scr_act() == guider_ui.scrSettings) {
			scrPrintInternet_return_target = 1;
			ui_load_scr_animation(&guider_ui, &guider_ui.scrPrintInternet, guider_ui.scrPrintInternet_del, &guider_ui.scrSettings_del, setup_scr_scrPrintInternet, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
		}
	}
}

/**
 * @brief Update WiFi connection status (called by ESP32 WiFi task)
 * @param connected true if connected, false if disconnected
 * @param ssid Connected WiFi SSID (NULL if not connected)
 *
 * IMPORTANT: This function is called from WiFi event callback (different task!)
 * It ONLY sets flags - actual UI updates happen in LVGL task via polling timer
 *
 * CRITICAL: Do NOT call ANY LVGL functions here (including lv_tick_get)!
 */
void wifi_status_update_callback(bool connected, const char *ssid)
{
	if (connected && ssid != NULL) {
		/* Save SSID and set connected flag */
		strncpy(s_wifi_pending_ssid, ssid, sizeof(s_wifi_pending_ssid) - 1);
		s_wifi_pending_ssid[sizeof(s_wifi_pending_ssid) - 1] = '\0';

		/* Use FreeRTOS tick instead of lv_tick_get() for thread safety */
		s_wifi_status_change_tick = (uint32_t)xTaskGetTickCount();
		s_wifi_status_pending = WIFI_STATUS_CONNECTED;

	} else {
		/* Set failed flag only if this was an actual connection attempt */
		if (ssid != NULL || s_wifi_status_pending == WIFI_STATUS_CONNECTING) {
			s_wifi_status_change_tick = (uint32_t)xTaskGetTickCount();
			s_wifi_status_pending = WIFI_STATUS_FAILED;
		}
	}

	/* Polling timer is already running in Settings screen */
}

/**
 * @brief Get current brightness setting
 * @return Brightness value (0-100)
 */
int settings_get_brightness(void)
{
	return lv_slider_get_value(guider_ui.scrSettings_sliderBrightness);
}

/* ========================================
 * Print Internet Screen Event Handlers
 * ========================================*/

static void scrPrintInternet_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		/* === Update WiFi status FLAG to show disconnected state === */
		extern void set_wifi_connected_status(bool is_connected);
		set_wifi_connected_status(false);

		// Return to previous screen based on return target
		if (scrPrintInternet_return_target == 1) {
			// Return to Settings screen (WiFi icon not affected here)
			ui_load_scr_animation(&guider_ui, &guider_ui.scrSettings, guider_ui.scrSettings_del, &guider_ui.scrPrintInternet_del, setup_scr_scrSettings, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
			scrPrintInternet_return_target = 0; // Reset
		} else {
			// Return to Print Menu (default) - if user goes to Home later, icon will be updated
			ui_load_scr_animation(&guider_ui, &guider_ui.scrPrintMenu, guider_ui.scrPrintMenu_del, &guider_ui.scrPrintInternet_del, setup_scr_scrPrintMenu, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		}
		break;
	}
	default:
		break;
	}
}

void events_init_scrPrintInternet(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrPrintInternet_btnBack, scrPrintInternet_btnBack_event_handler, LV_EVENT_ALL, ui);
}

/* ========================================
 * Wireless Serial Screen Event Handlers
 * ========================================*/

// Serial configuration state
static uint32_t ws_baud_rate = 115200;
static uint8_t ws_stop_bits = 1;  // 0=1, 1=1.5, 2=2
static uint8_t ws_data_bits = 8;  // 5-8
static uint8_t ws_parity = 0;     // 0=None, 1=Odd, 2=Even
static bool ws_hex_send = false;
static bool ws_hex_receive = false;
static bool ws_send_newline = true;
static bool ws_connected = false;

// Keyboard object for send textarea
static lv_obj_t *ws_keyboard = NULL;

static void scrWirelessSerial_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		// Screen loaded - no animation needed for new design
		break;
	}
	default:
		break;
	}
}

static void scrWirelessSerial_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrWirelessSerial_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		// No animation needed for new design
		break;
	}
	default:
		break;
	}
}

static void scrWirelessSerial_btnConnect_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Execute command from send textarea
		const char *cmd = lv_textarea_get_text(guider_ui.scrWirelessSerial_textareaSend);
		if (cmd != NULL && strlen(cmd) > 0) {
			// Simulate command execution - add to receive area
			char response[512];
			snprintf(response, sizeof(response), "%s\n\nOK\n", lv_textarea_get_text(guider_ui.scrWirelessSerial_textareaReceive));
			lv_textarea_set_text(guider_ui.scrWirelessSerial_textareaReceive, response);

			// ESP32 Integration Point: Send AT command via UART
			// extern void wireless_serial_send_command(const char *cmd);
			// wireless_serial_send_command(cmd);
		}
		break;
	}
	default:
		break;
	}
}

// BaudRate dropdown event handler
static void scrWirelessSerial_dropdownBaudRate_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		uint16_t sel = lv_dropdown_get_selected(guider_ui.scrWirelessSerial_dropdownBaudRate);
		const uint32_t baud_rates[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200};
		if (sel < sizeof(baud_rates) / sizeof(baud_rates[0])) {
			ws_baud_rate = baud_rates[sel];
			ESP_LOGI("WS_CONFIG", "BaudRate changed to: %u", ws_baud_rate);

			// ESP32 Integration Point: Update UART baud rate
			// extern void wireless_serial_set_baudrate(uint32_t baudrate);
			// wireless_serial_set_baudrate(ws_baud_rate);
		}
		break;
	}
	default:
		break;
	}
}

// StopBits dropdown event handler
static void scrWirelessSerial_dropdownStopBits_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		ws_stop_bits = lv_dropdown_get_selected(guider_ui.scrWirelessSerial_dropdownStopBits);
		ESP_LOGI("WS_CONFIG", "StopBits changed to: %d", ws_stop_bits);

		// ESP32 Integration Point: Update UART stop bits
		// extern void wireless_serial_set_stopbits(uint8_t stopbits);
		// wireless_serial_set_stopbits(ws_stop_bits);
		break;
	}
	default:
		break;
	}
}

// Length (data bits) dropdown event handler
static void scrWirelessSerial_dropdownLength_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		uint16_t sel = lv_dropdown_get_selected(guider_ui.scrWirelessSerial_dropdownLength);
		ws_data_bits = 8 - sel;  // 8, 7, 6, 5
		ESP_LOGI("WS_CONFIG", "Data bits changed to: %d", ws_data_bits);

		// ESP32 Integration Point: Update UART data bits
		// extern void wireless_serial_set_databits(uint8_t databits);
		// wireless_serial_set_databits(ws_data_bits);
		break;
	}
	default:
		break;
	}
}

// Parity dropdown event handler
static void scrWirelessSerial_dropdownParity_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		ws_parity = lv_dropdown_get_selected(guider_ui.scrWirelessSerial_dropdownParity);
		ESP_LOGI("WS_CONFIG", "Parity changed to: %d", ws_parity);

		// ESP32 Integration Point: Update UART parity
		// extern void wireless_serial_set_parity(uint8_t parity);
		// wireless_serial_set_parity(ws_parity);
		break;
	}
	default:
		break;
	}
}

// Hex-Send checkbox event handler
static void scrWirelessSerial_checkboxHexSend_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		ws_hex_send = lv_obj_has_state(guider_ui.scrWirelessSerial_checkboxHexSend, LV_STATE_CHECKED);
		ESP_LOGI("WS_CONFIG", "Hex-Send: %s", ws_hex_send ? "ON" : "OFF");
		break;
	}
	default:
		break;
	}
}

// Hex-Receive checkbox event handler
static void scrWirelessSerial_checkboxHexReceive_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		ws_hex_receive = lv_obj_has_state(guider_ui.scrWirelessSerial_checkboxHexReceive, LV_STATE_CHECKED);
		ESP_LOGI("WS_CONFIG", "Hex-Receive: %s", ws_hex_receive ? "ON" : "OFF");
		break;
	}
	default:
		break;
	}
}

// Send-NewLine checkbox event handler
static void scrWirelessSerial_checkboxSendNewLine_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		ws_send_newline = lv_obj_has_state(guider_ui.scrWirelessSerial_checkboxSendNewLine, LV_STATE_CHECKED);
		ESP_LOGI("WS_CONFIG", "Send-NewLine: %s", ws_send_newline ? "ON" : "OFF");
		break;
	}
	default:
		break;
	}
}

// Send button event handler (btnClear repurposed)
static void scrWirelessSerial_btnClear_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Get text from send textarea
		const char *text = lv_textarea_get_text(guider_ui.scrWirelessSerial_textareaSend);
		if (text != NULL && strlen(text) > 0) {
			// Prepare data to send
			char send_buffer[512];
			size_t send_len = 0;

			if (ws_hex_send) {
				// Convert hex string to bytes
				// TODO: Implement hex string to bytes conversion
				ESP_LOGI("WS_SEND", "Sending HEX: %s", text);
			} else {
				// Send as ASCII
				send_len = snprintf(send_buffer, sizeof(send_buffer), "%s%s",
				                    text, ws_send_newline ? "\r\n" : "");
				ESP_LOGI("WS_SEND", "Sending ASCII: %s", send_buffer);
			}

			// ESP32 Integration Point: Send data via UART
			// extern void wireless_serial_send_data(const char *data, size_t len);
			// wireless_serial_send_data(send_buffer, send_len);

			// Optionally clear send textarea after sending
			// lv_textarea_set_text(guider_ui.scrWirelessSerial_textareaSend, "");
		}
		break;
	}
	default:
		break;
	}
}

// Clear receive button event handler
static void scrWirelessSerial_btnClearReceive_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Clear receive textarea
		lv_textarea_set_text(guider_ui.scrWirelessSerial_textareaReceive, "");
		ESP_LOGI("WS_UI", "Receive buffer cleared");
		break;
	}
	default:
		break;
	}
}

// Send textarea event handler - show/hide keyboard
static void scrWirelessSerial_textareaSend_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_FOCUSED:
	{
		// Create keyboard if it doesn't exist
		if (ws_keyboard == NULL) {
			ws_keyboard = lv_keyboard_create(guider_ui.scrWirelessSerial);
			lv_obj_set_size(ws_keyboard, 800, 240);
			lv_obj_align(ws_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
			lv_keyboard_set_textarea(ws_keyboard, guider_ui.scrWirelessSerial_textareaSend);
		}
		// Show keyboard
		lv_obj_clear_flag(ws_keyboard, LV_OBJ_FLAG_HIDDEN);
		break;
	}
	case LV_EVENT_DEFOCUSED:
	{
		// Hide keyboard when textarea loses focus
		if (ws_keyboard != NULL) {
			lv_obj_add_flag(ws_keyboard, LV_OBJ_FLAG_HIDDEN);
		}
		break;
	}
	case LV_EVENT_READY:
	case LV_EVENT_CANCEL:
	{
		// Hide keyboard when user presses OK or Cancel on keyboard
		if (ws_keyboard != NULL) {
			lv_obj_add_flag(ws_keyboard, LV_OBJ_FLAG_HIDDEN);
		}
		break;
	}
	default:
		break;
	}
}

// AT command type dropdown event handler
static void scrWirelessSerial_dropdownATType_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		uint16_t sel = lv_dropdown_get_selected(guider_ui.scrWirelessSerial_dropdownATType);

		// Update receive textarea with selected AT command set
		const char *at_text = NULL;
		switch (sel) {
		case 0:  // Basic AT Commands
			at_text = "=== Basic AT Commands ===\n"
				"AT - Test AT startup\n"
				"AT+RST - Restart module\n"
				"AT+GMR - View version info\n"
				"AT+CMD - Query supported commands\n"
				"AT+GSLP - Enter Deep-sleep mode\n"
				"ATE - Enable/Disable AT echo\n"
				"AT+RESTORE - Factory reset\n"
				"AT+UART_CUR - Set UART config (temp)\n"
				"AT+UART_DEF - Set UART config (save)\n"
				"AT+SLEEP - Set sleep mode\n"
				"AT+SYSRAM - Query heap usage\n"
				"AT+SYSMSG - Query/Set system messages\n"
				"AT+SYSFLASH - Read/Write flash\n"
				"AT+RFPOWER - Query/Set RF TX power\n"
				"AT+SYSSTORE - Set parameter storage\n";
			break;
		case 1:  // Wi-Fi AT Commands
			at_text = "=== Wi-Fi AT Commands ===\n"
				"AT+CWINIT - Init/Deinit Wi-Fi driver\n"
				"AT+CWMODE - Set Wi-Fi mode (STA/AP/STA+AP)\n"
				"AT+CWJAP - Connect to AP\n"
				"AT+CWLAP - Scan available APs\n"
				"AT+CWQAP - Disconnect from AP\n"
				"AT+CWSAP - Configure SoftAP\n"
				"AT+CWLIF - Query connected stations\n"
				"AT+CWDHCP - Enable/Disable DHCP\n"
				"AT+CIPSTA - Set Station IP address\n"
				"AT+CIPAP - Set SoftAP IP address\n"
				"AT+CIPSTAMAC - Set Station MAC\n"
				"AT+CIPAPMAC - Set SoftAP MAC\n"
				"AT+CWHOSTNAME - Set hostname\n"
				"AT+CWCOUNTRY - Set country code\n"
				"AT+CWSTARTSMART - Start SmartConfig\n"
				"AT+CWSTOPSMART - Stop SmartConfig\n"
				"AT+WPS - Set WPS function\n";
			break;
		case 2:  // TCP/IP AT Commands
			at_text = "=== TCP/IP AT Commands ===\n"
				"AT+CIPV6 - Enable/Disable IPv6\n"
				"AT+CIPSTATE - Query TCP/UDP/SSL info\n"
				"AT+CIPDOMAIN - DNS resolution\n"
				"AT+CIPSTART - Establish TCP/UDP/SSL\n"
				"AT+CIPSEND - Send data\n"
				"AT+CIPCLOSE - Close connection\n"
				"AT+CIFSR - Query local IP and MAC\n"
				"AT+CIPMUX - Enable/Disable multi-conn\n"
				"AT+CIPSERVER - Create/Close TCP server\n"
				"AT+CIPMODE - Query/Set transfer mode\n"
				"AT+CIPSTO - Set TCP server timeout\n"
				"AT+CIPSNTPCFG - Set SNTP config\n"
				"AT+CIPSNTPTIME - Query SNTP time\n"
				"AT+CIUPDATE - Upgrade firmware via Wi-Fi\n"
				"AT+CIPSSLCCONF - Set SSL client config\n"
				"AT+PING - Ping remote host\n"
				"AT+CIPDNS - Query/Set DNS server\n";
			break;
		case 3:  // MQTT AT Commands
			at_text = "=== MQTT AT Commands ===\n"
				"AT+MQTTUSERCFG - Set MQTT user config\n"
				"AT+MQTTLONGCLIENTID - Set client ID\n"
				"AT+MQTTLONGUSERNAME - Set username\n"
				"AT+MQTTLONGPASSWORD - Set password\n"
				"AT+MQTTCONNCFG - Set connection config\n"
				"AT+MQTTALPN - Set ALPN\n"
				"AT+MQTTSNI - Set SNI\n"
				"AT+MQTTCONN - Connect to MQTT broker\n"
				"AT+MQTTPUB - Publish MQTT message\n"
				"AT+MQTTPUBRAW - Publish long message\n"
				"AT+MQTTSUB - Subscribe to topic\n"
				"AT+MQTTUNSUB - Unsubscribe from topic\n"
				"AT+MQTTCLEAN - Disconnect MQTT\n";
			break;
		case 4:  // HTTP AT Commands
			at_text = "=== HTTP AT Commands ===\n"
				"AT+HTTPCLIENT - Send HTTP request\n"
				"AT+HTTPGETSIZE - Get HTTP resource size\n"
				"AT+HTTPCGET - Get HTTP resource\n"
				"AT+HTTPCPOST - Post HTTP data\n"
				"AT+HTTPCPUT - Put HTTP data\n"
				"AT+HTTPURLCFG - Set/Get long HTTP URL\n"
				"AT+HTTPCHEAD - Set/Query HTTP header\n"
				"AT+HTTPCFG - Set HTTP client config\n";
			break;
		case 5:  // User AT Commands
			at_text = "=== User AT Commands ===\n"
				"AT+USERRAM - Operate user free RAM\n"
				"AT+USEROTA - Upgrade firmware by URL\n"
				"AT+USERWKMCUCFG - Set wake MCU config\n"
				"AT+USERMCUSLEEP - MCU sleep indication\n"
				"AT+USERDOCS - Query user doc link\n";
			break;
		default:
			at_text = "Unknown AT command type";
			break;
		}

		if (at_text != NULL) {
			lv_textarea_set_text(guider_ui.scrWirelessSerial_textareaReceive, at_text);
			ESP_LOGI("WS_UI", "AT command type changed to: %d", sel);
		}
		break;
	}
	default:
		break;
	}
}


void events_init_scrWirelessSerial(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrWirelessSerial, scrWirelessSerial_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_btnBack, scrWirelessSerial_btnBack_event_handler, LV_EVENT_ALL, ui);
	// lv_obj_add_event_cb(ui->scrWirelessSerial_btnConnect, scrWirelessSerial_btnConnect_event_handler, LV_EVENT_ALL, ui);  // Button removed
	lv_obj_add_event_cb(ui->scrWirelessSerial_dropdownBaudRate, scrWirelessSerial_dropdownBaudRate_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_dropdownStopBits, scrWirelessSerial_dropdownStopBits_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_dropdownLength, scrWirelessSerial_dropdownLength_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_dropdownParity, scrWirelessSerial_dropdownParity_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_checkboxHexSend, scrWirelessSerial_checkboxHexSend_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_checkboxHexReceive, scrWirelessSerial_checkboxHexReceive_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_checkboxSendNewLine, scrWirelessSerial_checkboxSendNewLine_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_btnClear, scrWirelessSerial_btnClear_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_btnClearReceive, scrWirelessSerial_btnClearReceive_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_textareaSend, scrWirelessSerial_textareaSend_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrWirelessSerial_dropdownATType, scrWirelessSerial_dropdownATType_event_handler, LV_EVENT_ALL, ui);
}

/* ========================================
 * Wireless Serial Interface Functions (for ESP32 integration)
 * ========================================*/

/**
 * @brief Update received data in the receive textarea
 * @param data Received data string
 */
void wireless_serial_update_receive_data(const char *data)
{
	if (data == NULL) return;

	// Get current text
	const char *current_text = lv_textarea_get_text(guider_ui.scrWirelessSerial_textareaReceive);

	// Append new data
	char buffer[2048];
	if (ws_hex_receive) {
		// TODO: Convert bytes to hex string
		snprintf(buffer, sizeof(buffer), "%s%s", current_text, data);
	} else {
		snprintf(buffer, sizeof(buffer), "%s%s", current_text, data);
	}

	lv_textarea_set_text(guider_ui.scrWirelessSerial_textareaReceive, buffer);

	// Auto-scroll to bottom
	lv_textarea_set_cursor_pos(guider_ui.scrWirelessSerial_textareaReceive, LV_TEXTAREA_CURSOR_LAST);
}

/**
 * @brief Get current serial configuration
 */
void wireless_serial_get_config(uint32_t *baudrate, uint8_t *databits, uint8_t *stopbits, uint8_t *parity)
{
	if (baudrate) *baudrate = ws_baud_rate;
	if (databits) *databits = ws_data_bits;
	if (stopbits) *stopbits = ws_stop_bits;
	if (parity) *parity = ws_parity;
}

/* ========================================
 * Oscilloscope Screen Event Handlers
 * ========================================*/

// Oscilloscope state variables
static bool osc_running = true;
static bool osc_fft_enabled = false;
static bool osc_grid_enabled = true;  // Grid display enabled by default
static bool osc_export_enabled = false;
static lv_timer_t *osc_waveform_timer = NULL;
static int osc_waveform_phase = 0;
static int osc_time_scale_index = 2;  // Start at 20us
static int osc_volt_scale_index = 6;  // Start at 1V
static float osc_base_amplitude = 300.0;  // Base amplitude in chart units (for 0-800 range)
static float osc_x_offset = 0.0;  // X axis offset in time units
static float osc_y_offset = 0.0;  // Y axis offset in voltage units
static bool osc_x_offset_active = false;  // X offset control active
static bool osc_y_offset_active = false;  // Y offset control active
static lv_coord_t osc_last_touch_y = 0;  // Last touch Y position for offset control

// Frozen waveform data for STOP mode
static float osc_frozen_voltage_data[720];  // Store actual voltage values when frozen
static bool osc_frozen_data_valid = false;
static int osc_frozen_time_scale_index = 2;  // Time scale when frozen
static int osc_frozen_volt_scale_index = 6;  // Voltage scale when frozen

// Trigger settings
static float osc_trigger_voltage = 0.5f;  // Trigger voltage level in volts
static int osc_trigger_mode = 0;  // 0=RISE, 1=FALL, 2=EDGE
static lv_obj_t *osc_trigger_line = NULL;  // Trigger level indicator line
static lv_obj_t *osc_trigger_marker = NULL;  // Trigger marker label
static bool osc_trigger_active = false;  // Trigger voltage control active

// Y offset baseline indicator
static lv_obj_t *osc_y_baseline = NULL;  // Y offset baseline (horizontal line)
static lv_obj_t *osc_y_baseline_marker = NULL;  // Y offset marker label

// Helper function to format time offset with appropriate unit based on time scale
static void format_time_offset(char *buffer, size_t size, float time_seconds, int time_scale_index)
{
	// Time scale values: 1us, 10us, 20us, 50us, 100us, 200us, 500us, 1ms, 10ms, 100ms, 1s
	// Use unit based on time scale
	if (time_scale_index <= 6) {
		// us range (1us to 500us)
		snprintf(buffer, size, "%.1fus", time_seconds * 1e6);
	} else if (time_scale_index <= 9) {
		// ms range (1ms to 100ms)
		snprintf(buffer, size, "%.1fms", time_seconds * 1e3);
	} else {
		// s range (1s)
		snprintf(buffer, size, "%.2fs", time_seconds);
	}
}

// Helper function to format voltage offset with appropriate unit based on voltage scale
static void format_voltage_offset(char *buffer, size_t size, float voltage_volts, int volt_scale_index)
{
	// Voltage scale values: 10mV, 20mV, 50mV, 100mV, 200mV, 500mV, 1V, 2V, 5V, 12V
	// Use unit based on voltage scale
	if (volt_scale_index <= 5) {
		// mV range (10mV to 500mV)
		snprintf(buffer, size, "%.0fmV", voltage_volts * 1000);
	} else {
		// V range (1V to 12V)
		snprintf(buffer, size, "%.2fV", voltage_volts);
	}
}

// Waveform update timer callback - Generate dynamic waveform data
static void osc_waveform_update_cb(lv_timer_t *timer)
{
	lv_chart_series_t *ser = lv_chart_get_series_next(guider_ui.scrOscilloscope_chartWaveform, NULL);
	if (ser == NULL) return;

	// If not running (STOP mode), display frozen data with current scale settings and offsets
	if (!osc_running && osc_frozen_data_valid) {
		// Voltage scale values
		const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};
		const float chart_center = 500.0f;  // Center at 500 (0V reference) for 0-1000 range
		const float units_per_division = 100.0f;

		// Get current voltage scale
		float volts_per_div = volt_scale_values[osc_volt_scale_index];
		float units_per_volt = units_per_division / volts_per_div;

		// Time scale values for X offset calculation
		const float time_scale_values[] = {1e-6, 10e-6, 20e-6, 50e-6, 100e-6, 200e-6, 500e-6, 1e-3, 10e-3, 100e-3, 1.0};
		float time_per_div = time_scale_values[osc_time_scale_index];

		// Calculate time scale ratio (how much to zoom in/out)
		// If current time scale is larger (slower), we zoom out (compress data)
		// If current time scale is smaller (faster), we zoom in (expand data)
		float frozen_time_per_div = time_scale_values[osc_frozen_time_scale_index];
		float time_scale_ratio = frozen_time_per_div / time_per_div;

		// Calculate X offset in pixels (time offset to pixel offset)
		// Each division is 40 pixels, total 18 divisions = 720 pixels
		float pixels_per_second = 720.0f / (18.0f * time_per_div);
		int x_offset_pixels = (int)(osc_x_offset * pixels_per_second);

		// Redraw frozen waveform with current scale and offsets
		for(int i = 0; i < 720; i++) {
			// Apply time scale ratio to resample the frozen data
			// source_pos is the position in the frozen data array
			float source_pos = (i * time_scale_ratio) - x_offset_pixels;
			int source_index = (int)source_pos;

			// If source index is out of bounds, use edge value or set to center
			float voltage;
			if (source_index < 0 || source_index >= 720) {
				voltage = 0.0f;  // Out of range - show 0V
			} else {
				// Linear interpolation for smoother display when zooming
				float frac = source_pos - source_index;
				if (source_index + 1 < 720 && frac > 0.0f) {
					voltage = osc_frozen_voltage_data[source_index] * (1.0f - frac) +
					          osc_frozen_voltage_data[source_index + 1] * frac;
				} else {
					voltage = osc_frozen_voltage_data[source_index];
				}
			}

			// Apply Y offset (voltage shift)
			voltage += osc_y_offset;

			// Convert to chart coordinates
			float y_float = chart_center + (voltage * units_per_volt);

			// Clamp to valid range
			int val = (int)y_float;
			if (val < 0) val = 0;
			if (val > 1000) val = 1000;

			ser->y_points[i] = val;
		}

		lv_chart_refresh(guider_ui.scrOscilloscope_chartWaveform);
		return;
	}

	// Running mode - generate new waveform data
	if (!osc_running) return;

	// Voltage scale values (10mV, 20mV, 50mV, 100mV, 200mV, 500mV, 1V, 2V, 5V, 12V per division)
	const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};

	// Time scale factors (1us, 10us, 20us, 50us, 100us, 200us, 500us, 1ms, 10ms, 100ms, 1s)
	// Controls how many cycles are visible
	const float time_scale_factors[] = {0.5, 0.1, 0.05, 0.02, 0.01, 0.005, 0.002, 0.001, 0.0001, 0.00001, 0.000001};

	// Chart display parameters - NEW FIXED SIZE ARCHITECTURE
	// Display: 720x400 pixels, Grid: 18x10 divisions (40x40 pixels each - perfect square)
	// Y axis: 0-1000 chart units, center at 500 (0V reference)
	// Each division: 100 chart units (1000/10=100)
	const float chart_center = 500.0f;      // Y coordinate of center (0V reference)
	const float chart_height = 1000.0f;     // Total chart Y range (0-1000)
	const float units_per_division = 100.0f;  // 100 chart units = 1 division
	const int num_points = 720;  // Number of data points (one per horizontal pixel)

	// Get current voltage scale (volts per division)
	float volts_per_div = volt_scale_values[osc_volt_scale_index];

	// Calculate conversion factor: chart units per volt
	float units_per_volt = units_per_division / volts_per_div;

	// Time scale factor for waveform frequency
	float frequency = time_scale_factors[osc_time_scale_index];

	// Variables for measurement calculation
	float min_voltage = 1000.0f, max_voltage = -1000.0f;
	float sum_squares = 0;

	if (osc_fft_enabled) {
		// Update FFT spectrum with some animation
		for(int i = 0; i < num_points; i++) {
			if (i < 36) {
				// Fundamental frequency peak with slight variation
				ser->y_points[i] = 500 + (int)(300 * exp(-pow((i - 10) / 5.0, 2)) * (1.0 + 0.1 * sin(osc_waveform_phase * 0.1)));
			} else if (i < 108) {
				// Second harmonic
				ser->y_points[i] = 500 + (int)(150 * exp(-pow((i - 60) / 8.0, 2)) * (1.0 + 0.1 * sin(osc_waveform_phase * 0.15)));
			} else if (i < 180) {
				// Third harmonic
				ser->y_points[i] = 500 + (int)(75 * exp(-pow((i - 130) / 10.0, 2)) * (1.0 + 0.1 * sin(osc_waveform_phase * 0.2)));
			} else {
				// Noise floor
				ser->y_points[i] = 500 + (rand() % 20 - 10);
			}
		}
	} else {
		// Generate real voltage signal first (fixed amplitude, independent of display scale)
		// Signal: 1V peak-to-peak sine wave (±0.5V) with harmonics and noise
		const float signal_amplitude = 0.5f;  // ±0.5V = 1V peak-to-peak

		for(int i = 0; i < num_points; i++) {
			// Base sine wave with phase offset
			float phase = (i + osc_waveform_phase) * frequency;
			float base_wave = sin(phase);

			// Add 3rd harmonic (10% amplitude) for more realistic signal
			float harmonic = 0.1f * sin(3.0f * phase);

			// Add small random noise (2% amplitude)
			float noise = ((rand() % 100) - 50) * 0.0004f;

			// Combine all components to get actual voltage
			float combined = base_wave + harmonic + noise;
			float actual_voltage = signal_amplitude * combined;  // Real voltage in volts

			// Apply Y offset
			actual_voltage += osc_y_offset;

			// Store voltage data for potential freezing
			osc_frozen_voltage_data[i] = actual_voltage;

			// Track min/max voltage for measurements
			if (actual_voltage < min_voltage) min_voltage = actual_voltage;
			if (actual_voltage > max_voltage) max_voltage = actual_voltage;

			// Calculate RMS (sum of squares)
			sum_squares += actual_voltage * actual_voltage;

			// Convert voltage to chart Y coordinate
			// Formula: y = center + (voltage * units_per_volt)
			float y_float = chart_center + (actual_voltage * units_per_volt);

			// Clamp to valid chart range
			int val = (int)y_float;
			if (val < 0) val = 0;
			if (val > 1000) val = 1000;

			ser->y_points[i] = val;
		}

		// Mark frozen data as valid (will be used if STOP is pressed)
		osc_frozen_data_valid = true;
		osc_frozen_time_scale_index = osc_time_scale_index;
		osc_frozen_volt_scale_index = osc_volt_scale_index;

		// Calculate and update measurements (with NULL checks)
		if (!osc_fft_enabled) {
			char buf[64];

			// Calculate values from actual voltage (not screen coordinates)
			float vmax = max_voltage;
			float vmin = min_voltage;
			float vpp = vmax - vmin;
			float vrms = sqrt(sum_squares / 1000.0f);
			float freq_hz = 1.0f / (frequency * 1000.0f * 0.00002f);  // Rough estimate

			// Vmax - update combined label
			if (guider_ui.scrOscilloscope_labelVmaxTitle != NULL) {
				snprintf(buf, sizeof(buf), "Vmax:%.2fV", vmax);
				lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, buf);
			}

			// Vmin - update combined label
			if (guider_ui.scrOscilloscope_labelVminTitle != NULL) {
				snprintf(buf, sizeof(buf), "Vmin:%.2fV", vmin);
				lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, buf);
			}

			// Vp-p - update combined label
			if (guider_ui.scrOscilloscope_labelVppTitle != NULL) {
				snprintf(buf, sizeof(buf), "Vp-p:%.2fV", vpp);
				lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, buf);
			}

			// Vrms - update combined label
			if (guider_ui.scrOscilloscope_labelVrmsTitle != NULL) {
				snprintf(buf, sizeof(buf), "Vrms:%.2fV", vrms);
				lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, buf);
			}

			// Frequency - update combined label
			if (guider_ui.scrOscilloscope_labelFreqTitle != NULL) {
				if (freq_hz >= 1000000) {
					snprintf(buf, sizeof(buf), "Freq:%.1fMHz", freq_hz / 1000000.0);
				} else if (freq_hz >= 1000) {
					snprintf(buf, sizeof(buf), "Freq:%.1fkHz", freq_hz / 1000.0);
				} else {
					snprintf(buf, sizeof(buf), "Freq:%.1fHz", freq_hz);
				}
				lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, buf);
			}
		}
	}

	osc_waveform_phase += 5;  // Increment phase for animation
	if (osc_waveform_phase >= 1000) osc_waveform_phase = 0;

	lv_chart_refresh(guider_ui.scrOscilloscope_chartWaveform);
}

// Screen load event handler
static void scrOscilloscope_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		// Initialize oscilloscope state
		osc_running = true;
		osc_fft_enabled = false;
		osc_grid_enabled = true;
		osc_export_enabled = false;
		osc_waveform_phase = 0;
		osc_x_offset = 0.0f;
		osc_y_offset = 0.0f;
		osc_x_offset_active = false;
		osc_y_offset_active = false;
		osc_trigger_voltage = 0.5f;
		osc_trigger_mode = 0;
		osc_trigger_active = false;
		osc_frozen_data_valid = false;

		// Initialize oscilloscope export module
		esp_err_t ret = osc_export_init();
		if (ret != ESP_OK) {
			ESP_LOGE("OSC", "Failed to initialize export module: %s", esp_err_to_name(ret));
		}

		// Initialize offset displays
		lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, "0us");
		lv_label_set_text(guider_ui.scrOscilloscope_labelYOffsetValue, "0V");

		// Initialize trigger voltage display
		lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, "0.50V");

		// Create trigger level indicator line (horizontal line)
		if (osc_trigger_line == NULL) {
			osc_trigger_line = lv_line_create(guider_ui.scrOscilloscope_contWaveform);
			static lv_point_t trigger_line_points[2];
			trigger_line_points[0].x = 0;
			trigger_line_points[0].y = 200 - 50;  // Center (200) - offset for 0.5V
			trigger_line_points[1].x = 720;
			trigger_line_points[1].y = 200 - 50;
			lv_line_set_points(osc_trigger_line, trigger_line_points, 2);
			lv_obj_set_style_line_color(osc_trigger_line, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_width(osc_trigger_line, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_dash_width(osc_trigger_line, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_dash_gap(osc_trigger_line, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
		}

		// Create trigger marker (arrow on the right side)
		if (osc_trigger_marker == NULL) {
			osc_trigger_marker = lv_label_create(guider_ui.scrOscilloscope_contWaveform);
			lv_label_set_text(osc_trigger_marker, "<");
			lv_obj_set_pos(osc_trigger_marker, 700, 200 - 50 - 10);  // Right side, aligned with trigger line
			lv_obj_set_style_text_color(osc_trigger_marker, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_text_font(osc_trigger_marker, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
		}

		// Start waveform update timer (50ms = 20Hz refresh rate)
		if (osc_waveform_timer == NULL) {
			osc_waveform_timer = lv_timer_create(osc_waveform_update_cb, 50, NULL);
		}
		break;
	}
	case LV_EVENT_SCREEN_UNLOADED:
	{
		// Stop and delete timer when leaving screen
		if (osc_waveform_timer != NULL) {
			lv_timer_del(osc_waveform_timer);
			osc_waveform_timer = NULL;
		}

		// Clean up trigger line objects
		if (osc_trigger_line != NULL) {
			lv_obj_del(osc_trigger_line);
			osc_trigger_line = NULL;
		}
		if (osc_trigger_marker != NULL) {
			lv_obj_del(osc_trigger_marker);
			osc_trigger_marker = NULL;
		}

		// Clean up Y baseline objects
		if (osc_y_baseline != NULL) {
			lv_obj_del(osc_y_baseline);
			osc_y_baseline = NULL;
		}
		if (osc_y_baseline_marker != NULL) {
			lv_obj_del(osc_y_baseline_marker);
			osc_y_baseline_marker = NULL;
		}

		// Deinitialize export module
		osc_export_deinit();

		break;
	}
	default:
		break;
	}
}

// Back button event handler
static void scrOscilloscope_btnBack_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.scrHome, guider_ui.scrHome_del, &guider_ui.scrOscilloscope_del, setup_scr_scrHome, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, false, true);
		break;
	}
	default:
		break;
	}
}

// Start/Stop button event handler
static void scrOscilloscope_btnStartStop_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		osc_running = !osc_running;
		if (osc_running) {
			lv_label_set_text(guider_ui.scrOscilloscope_btnStartStop_label, "RUN");
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnStartStop, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
		} else {
			lv_label_set_text(guider_ui.scrOscilloscope_btnStartStop_label, "STOP");
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnStartStop, lv_color_hex(0xFF0000), LV_PART_MAIN|LV_STATE_DEFAULT);
		}
		break;
	}
	default:
		break;
	}
}

// GRID button event handler - Toggle grid display
static void scrOscilloscope_btnPanZoom_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		osc_grid_enabled = !osc_grid_enabled;
		if (osc_grid_enabled) {
			// Grid ON - show grid lines (18 horizontal divisions, 10 vertical divisions, 40x40 pixels each)
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnPanZoom, lv_color_hex(0x0080FF), LV_PART_MAIN|LV_STATE_DEFAULT);  // Bright blue
			lv_chart_set_div_line_count(guider_ui.scrOscilloscope_chartWaveform, 10, 18);  // (hdiv=10 horizontal lines, vdiv=18 vertical lines)
		} else {
			// Grid OFF - hide grid lines
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnPanZoom, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);  // Dark gray
			lv_chart_set_div_line_count(guider_ui.scrOscilloscope_chartWaveform, 0, 0);  // Hide grid
		}
		lv_chart_refresh(guider_ui.scrOscilloscope_chartWaveform);
		break;
	}
	default:
		break;
	}
}

// FFT button event handler
static void scrOscilloscope_btnFFT_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		osc_fft_enabled = !osc_fft_enabled;
		if (osc_fft_enabled) {
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnFFT, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);  // Bright yellow when active

			// Update measurement displays for FFT mode
			lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, "Fund:");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxValue, "1.2kHz");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, "Harm:");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVminValue, "2.4kHz");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, "THD:");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVppValue, "2.5%");

			// Change waveform to FFT spectrum (bar chart style)
			lv_chart_series_t *ser = lv_chart_get_series_next(guider_ui.scrOscilloscope_chartWaveform, NULL);
			if (ser != NULL) {
				// Generate FFT spectrum data (frequency domain)
				for(int i = 0; i < 1000; i++) {
					if (i < 50) {
						// Fundamental frequency peak
						ser->y_points[i] = 50 + (int)(80 * exp(-pow((i - 10) / 5.0, 2)));
					} else if (i < 150) {
						// Second harmonic
						ser->y_points[i] = 50 + (int)(40 * exp(-pow((i - 80) / 8.0, 2)));
					} else if (i < 250) {
						// Third harmonic
						ser->y_points[i] = 50 + (int)(20 * exp(-pow((i - 180) / 10.0, 2)));
					} else {
						// Noise floor
						ser->y_points[i] = 50 + (rand() % 5);
					}
				}
				lv_chart_refresh(guider_ui.scrOscilloscope_chartWaveform);
			}
		} else {
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnFFT, lv_color_hex(0xA0A000), LV_PART_MAIN|LV_STATE_DEFAULT);  // Dark yellow when inactive

			// Restore normal measurement displays
			lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, "Vmax:");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxValue, "3.30V");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, "Vmin:");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVminValue, "0.10V");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, "Vp-p:");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVppValue, "3.20V");

			// Restore normal sine wave
			lv_chart_series_t *ser = lv_chart_get_series_next(guider_ui.scrOscilloscope_chartWaveform, NULL);
			if (ser != NULL) {
				for(int i = 0; i < 1000; i++) {
					ser->y_points[i] = 50 + (int)(40 * sin(i * 0.02));
				}
				lv_chart_refresh(guider_ui.scrOscilloscope_chartWaveform);
			}
		}
		break;
	}
	default:
		break;
	}
}

// Export button event handler - Export waveform data via USB MSC
static void scrOscilloscope_btnExport_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_LONG_PRESSED:
	{
		/* Long press to cycle through export formats */
		if (!osc_export_enabled) {
			osc_export_format_t current_format = osc_export_get_format();
			osc_export_format_t new_format = (current_format + 1) % 3;
			osc_export_set_format(new_format);

			/* Show format on button */
			const char *format_names[] = {"TXT", "CSV", "BOTH"};
			static char format_text[16];
			snprintf(format_text, sizeof(format_text), "FMT:%s", format_names[new_format]);
			lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, format_text);

			ESP_LOGI("OSC", "Export format changed to: %s", format_names[new_format]);

			/* Note: Button text will be restored when clicked or when screen is refreshed */
		}
		break;
	}
	case LV_EVENT_CLICKED:
	{
		/* First restore button text if it was showing format */
		const char *current_text = lv_label_get_text(guider_ui.scrOscilloscope_btnExport_label);
		if (current_text != NULL && strncmp(current_text, "FMT:", 4) == 0) {
			lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, "EXPORT");
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnExport, lv_color_hex(0xA05000), LV_PART_MAIN|LV_STATE_DEFAULT);
			return;  /* Don't toggle export on format display click */
		}

		osc_export_enabled = !osc_export_enabled;
		if (osc_export_enabled) {
			/* Show preparing status */
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnExport, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, "PREPARING");
			lv_refr_now(NULL);

			/* Prepare waveform data for export - use static to avoid stack overflow */
			static osc_waveform_data_t waveform_data;
			memset(&waveform_data, 0, sizeof(waveform_data));

			/* Get current waveform data from chart */
			lv_chart_series_t *ser = lv_chart_get_series_next(guider_ui.scrOscilloscope_chartWaveform, NULL);
			if (ser != NULL) {
				/* Get current settings first (needed for voltage conversion) */
				const float time_scale_values[] = {1e-6, 10e-6, 20e-6, 50e-6, 100e-6, 200e-6, 500e-6, 1e-3, 10e-3, 100e-3, 1.0};
				const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};

				waveform_data.time_scale = time_scale_values[osc_time_scale_index];
				waveform_data.volt_scale = volt_scale_values[osc_volt_scale_index];

				/* Convert chart Y coordinates back to actual voltage values
				 * Chart Y range: 0-100, center at 50 (0V reference)
				 * Conversion: voltage = (y - center) / pixels_per_volt
				 * where pixels_per_volt = pixels_per_division / volts_per_div
				 */
				const float chart_center = 50.0f;
				const float pixels_per_division = 10.0f;
				float volts_per_div = volt_scale_values[osc_volt_scale_index];
				float pixels_per_volt = pixels_per_division / volts_per_div;

				waveform_data.num_points = (1000 < OSC_MAX_DATA_POINTS) ? 1000 : OSC_MAX_DATA_POINTS;
				for (int i = 0; i < waveform_data.num_points; i++) {
					// Convert chart Y coordinate back to actual voltage
					float y_coord = (float)ser->y_points[i];
					waveform_data.data[i] = (y_coord - chart_center) / pixels_per_volt;
				}

				/* Get measurement values from labels */
				const char *freq_text = lv_label_get_text(guider_ui.scrOscilloscope_labelFreqTitle);
				const char *vmax_text = lv_label_get_text(guider_ui.scrOscilloscope_labelVmaxTitle);
				const char *vmin_text = lv_label_get_text(guider_ui.scrOscilloscope_labelVminTitle);
				const char *vpp_text = lv_label_get_text(guider_ui.scrOscilloscope_labelVppTitle);
				const char *vrms_text = lv_label_get_text(guider_ui.scrOscilloscope_labelVrmsTitle);

				/* Parse values (simple parsing, skip label part) */
				sscanf(freq_text, "Freq:%f", &waveform_data.frequency);
				sscanf(vmax_text, "Vmax:%f", &waveform_data.vmax);
				sscanf(vmin_text, "Vmin:%f", &waveform_data.vmin);
				sscanf(vpp_text, "Vp-p:%f", &waveform_data.vpp);
				sscanf(vrms_text, "Vrms:%f", &waveform_data.vrms);

				waveform_data.is_fft = osc_fft_enabled;

				/* Get real timestamp from RTC */
				osc_export_get_timestamp(waveform_data.timestamp, sizeof(waveform_data.timestamp));

				/* Store waveform data */
				lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, "SAVING");
				lv_refr_now(NULL);
				osc_export_store_waveform(&waveform_data);

				/* If FFT is enabled, store FFT data as well */
				if (osc_fft_enabled) {
					static osc_waveform_data_t fft_data;
					memcpy(&fft_data, &waveform_data, sizeof(osc_waveform_data_t));
					fft_data.is_fft = true;
					osc_export_store_fft(&fft_data);
				}

				/* Save directly to SD card */
				lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, "SAVING...");
				lv_refr_now(NULL);

				esp_err_t ret = osc_export_save_to_sd();
				if (ret != ESP_OK) {
					ESP_LOGE("OSC", "Failed to save to SD card: %s", esp_err_to_name(ret));
					lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnExport, lv_color_hex(0xFF0000), LV_PART_MAIN|LV_STATE_DEFAULT);
					lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, "FAILED");
				} else {
					/* Show success */
					char status_text[32];
					uint32_t export_num = osc_export_get_counter();
					snprintf(status_text, sizeof(status_text), "SAVED #%u", (unsigned int)export_num);
					lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnExport, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
					lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, status_text);
					ESP_LOGI("OSC", "Oscilloscope data #%u saved to SD card", (unsigned int)export_num);
				}
				
				/* Reset button after delay */
				vTaskDelay(pdMS_TO_TICKS(2000));
				lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnExport, lv_color_hex(0xA05000), LV_PART_MAIN|LV_STATE_DEFAULT);
				lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, "EXPORT");
				osc_export_enabled = false;
			}
		} else {
			/* Just reset button state */
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnExport, lv_color_hex(0xA05000), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_label_set_text(guider_ui.scrOscilloscope_btnExport_label, "EXPORT");
			ESP_LOGI("OSC", "Export cancelled");
		}
		break;
	}
	default:
		break;
	}
}

// Time scale button event handler
static void scrOscilloscope_contTimeScale_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Cycle through time scales: 1us, 10us, 20us, 50us, 100us, 200us, 500us, 1ms, 10ms, 100ms, 1s
		const char *time_scales[] = {"1us", "10us", "20us", "50us", "100us", "200us", "500us", "1ms", "10ms", "100ms", "1s"};
		osc_time_scale_index = (osc_time_scale_index + 1) % 11;
		lv_label_set_text(guider_ui.scrOscilloscope_labelTimeScaleValue, time_scales[osc_time_scale_index]);

		// Update X offset display with new unit
		char offset_str[32];
		format_time_offset(offset_str, sizeof(offset_str), osc_x_offset, osc_time_scale_index);
		lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, offset_str);

		// In STOP mode, trigger a redraw with new scale
		if (!osc_running && osc_frozen_data_valid) {
			// Force timer callback to redraw frozen waveform
			osc_waveform_update_cb(NULL);
		}

		break;
	}
	default:
		break;
	}
}

// Voltage scale button event handler
static void scrOscilloscope_contVoltScale_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Cycle through voltage scales: 10mV, 20mV, 50mV, 100mV, 200mV, 500mV, 1V, 2V, 5V, 12V
		const char *volt_scales[] = {"10mV", "20mV", "50mV", "100mV", "200mV", "500mV", "1V", "2V", "5V", "12V"};
		osc_volt_scale_index = (osc_volt_scale_index + 1) % 10;
		lv_label_set_text(guider_ui.scrOscilloscope_labelVoltScaleValue, volt_scales[osc_volt_scale_index]);

		// Update Y offset display with new unit
		char offset_str[32];
		format_voltage_offset(offset_str, sizeof(offset_str), osc_y_offset, osc_volt_scale_index);
		lv_label_set_text(guider_ui.scrOscilloscope_labelYOffsetValue, offset_str);

		// Update Y baseline marker if visible
		if (osc_y_baseline_marker != NULL && !lv_obj_has_flag(osc_y_baseline_marker, LV_OBJ_FLAG_HIDDEN)) {
			lv_label_set_text(osc_y_baseline_marker, offset_str);
		}

		// In STOP mode, trigger a redraw with new scale
		if (!osc_running && osc_frozen_data_valid) {
			// Force timer callback to redraw frozen waveform
			osc_waveform_update_cb(NULL);
		}

		break;
	}
	default:
		break;
	}
}

// Coupling button event handler
static void scrOscilloscope_contCoupling_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Toggle between DC and AC coupling
		const char *current = lv_label_get_text(guider_ui.scrOscilloscope_labelCouplingValue);
		if (strcmp(current, "DC") == 0) {
			lv_label_set_text(guider_ui.scrOscilloscope_labelCouplingValue, "AC");
		} else {
			lv_label_set_text(guider_ui.scrOscilloscope_labelCouplingValue, "DC");
		}
		break;
	}
	default:
		break;
	}
}

// Trigger mode button event handler
static void scrOscilloscope_contTriggerMode_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Cycle through trigger modes: RISE, FALL, EDGE
		const char *trigger_modes[] = {"RISE", "FALL", "EDGE"};
		osc_trigger_mode = (osc_trigger_mode + 1) % 3;
		lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerModeValue, trigger_modes[osc_trigger_mode]);
		break;
	}
	default:
		break;
	}
}

// Trigger voltage button event handler
static void scrOscilloscope_contTrigger_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *target = lv_event_get_target(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Toggle trigger voltage control active state
		osc_trigger_active = !osc_trigger_active;

		if (osc_trigger_active) {
			// Deactivate other offset controls
			osc_x_offset_active = false;
			osc_y_offset_active = false;
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contXOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contYOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

			// Highlight trigger control
			lv_obj_set_style_border_color(target, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
		} else {
			// Restore normal border
			lv_obj_set_style_border_color(target, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
		}
		break;
	}
	case LV_EVENT_PRESSED:
	{
		if (!osc_trigger_active || osc_trigger_line == NULL) break;

		// Initialize touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		osc_last_touch_y = point.y;
		break;
	}
	case LV_EVENT_PRESSING:
	{
		if (!osc_trigger_active || osc_trigger_line == NULL) break;

		// Get touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);

		// Calculate trigger voltage change based on vertical swipe
		if (osc_last_touch_y != 0) {
			lv_coord_t delta_y = osc_last_touch_y - point.y;  // Positive = swipe up = increase voltage

			// Voltage scale values for trigger adjustment
			const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};
			float volts_per_div = volt_scale_values[osc_volt_scale_index];

			// Convert pixel movement to voltage (1 pixel = 1/40 division for 40x40 grid)
			float voltage_change = (delta_y / 40.0f) * volts_per_div;
			osc_trigger_voltage += voltage_change;

			// Clamp trigger voltage to reasonable range
			if (osc_trigger_voltage < -10.0f) osc_trigger_voltage = -10.0f;
			if (osc_trigger_voltage > 10.0f) osc_trigger_voltage = 10.0f;

			// Update trigger line position
			// Convert voltage to Y coordinate: y = center_pixel - (voltage * pixels_per_volt)
			// Center pixel: 200 (400/2), pixels per division: 40
			const float pixels_per_division = 40.0f;
			float pixels_per_volt = pixels_per_division / volts_per_div;
			int trigger_y = 200 - (int)(osc_trigger_voltage * pixels_per_volt);

			// Clamp to display range
			if (trigger_y < 0) trigger_y = 0;
			if (trigger_y > 400) trigger_y = 400;

			// Update trigger line
			static lv_point_t trigger_line_points[2];
			trigger_line_points[0].x = 0;
			trigger_line_points[0].y = trigger_y;
			trigger_line_points[1].x = 720;
			trigger_line_points[1].y = trigger_y;
			lv_line_set_points(osc_trigger_line, trigger_line_points, 2);

			// Update trigger marker position
			lv_obj_set_pos(osc_trigger_marker, 700, trigger_y - 10);

			// Update display
			char trigger_str[32];
			if (fabsf(osc_trigger_voltage) < 0.001) {
				snprintf(trigger_str, sizeof(trigger_str), "%.1fmV", osc_trigger_voltage * 1000);
			} else {
				snprintf(trigger_str, sizeof(trigger_str), "%.2fV", osc_trigger_voltage);
			}
			lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, trigger_str);
		}
		osc_last_touch_y = point.y;
		break;
	}
	case LV_EVENT_RELEASED:
	{
		osc_last_touch_y = 0;
		break;
	}
	default:
		break;
	}
}

// X-Pos (X offset) button event handler
static void scrOscilloscope_contXOffset_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *target = lv_event_get_target(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Toggle X offset control active state
		osc_x_offset_active = !osc_x_offset_active;

		if (osc_x_offset_active) {
			// Deactivate Y offset if it was active
			osc_y_offset_active = false;
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contYOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

			// Highlight X offset control
			lv_obj_set_style_border_color(target, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
		} else {
			// Restore normal border
			lv_obj_set_style_border_color(target, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
		}
		break;
	}
	case LV_EVENT_PRESSED:
	{
		if (!osc_x_offset_active) break;

		// Initialize touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		osc_last_touch_y = point.y;
		break;
	}
	case LV_EVENT_PRESSING:
	{
		if (!osc_x_offset_active) break;

		// Get touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);

		// Calculate offset change based on vertical swipe
		if (osc_last_touch_y != 0) {
			lv_coord_t delta_y = osc_last_touch_y - point.y;  // Positive = swipe up = move right

			// Time scale values for offset calculation
			const float time_scale_values[] = {1e-6, 10e-6, 20e-6, 50e-6, 100e-6, 200e-6, 500e-6, 1e-3, 10e-3, 100e-3, 1.0};
			float time_per_div = time_scale_values[osc_time_scale_index];

			// Convert pixel movement to time offset (1 pixel = 1/40 division for 40x40 grid)
			float offset_change = (delta_y / 40.0f) * time_per_div;
			osc_x_offset += offset_change;

			// Update display with appropriate unit based on time scale
			char offset_str[32];
			format_time_offset(offset_str, sizeof(offset_str), osc_x_offset, osc_time_scale_index);
			lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, offset_str);
		}
		osc_last_touch_y = point.y;
		break;
	}
	case LV_EVENT_RELEASED:
	{
		osc_last_touch_y = 0;
		break;
	}
	default:
		break;
	}
}

// Y-Pos (Y offset) button event handler
static void scrOscilloscope_contYOffset_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *target = lv_event_get_target(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Toggle Y offset control active state
		osc_y_offset_active = !osc_y_offset_active;

		if (osc_y_offset_active) {
			// Deactivate X offset if it was active
			osc_x_offset_active = false;
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contXOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

			// Highlight Y offset control
			lv_obj_set_style_border_color(target, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

			// Create Y baseline indicator (horizontal line at center)
			if (osc_y_baseline == NULL) {
				osc_y_baseline = lv_line_create(guider_ui.scrOscilloscope_contWaveform);
				static lv_point_t y_baseline_points[2];
				y_baseline_points[0].x = 0;
				y_baseline_points[0].y = 200;  // Center Y position
				y_baseline_points[1].x = 720;
				y_baseline_points[1].y = 200;
				lv_line_set_points(osc_y_baseline, y_baseline_points, 2);
				lv_obj_set_style_line_color(osc_y_baseline, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);  // Green
				lv_obj_set_style_line_width(osc_y_baseline, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
				lv_obj_set_style_line_dash_width(osc_y_baseline, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
				lv_obj_set_style_line_dash_gap(osc_y_baseline, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
			} else {
				lv_obj_clear_flag(osc_y_baseline, LV_OBJ_FLAG_HIDDEN);
			}

			// Create Y baseline marker
			if (osc_y_baseline_marker == NULL) {
				osc_y_baseline_marker = lv_label_create(guider_ui.scrOscilloscope_contWaveform);
				lv_label_set_text(osc_y_baseline_marker, "0V");
				lv_obj_set_pos(osc_y_baseline_marker, 5, 200 - 10);
				lv_obj_set_style_text_color(osc_y_baseline_marker, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
				lv_obj_set_style_text_font(osc_y_baseline_marker, &lv_font_montserrat_12, LV_PART_MAIN|LV_STATE_DEFAULT);
			} else {
				lv_obj_clear_flag(osc_y_baseline_marker, LV_OBJ_FLAG_HIDDEN);
			}
		} else {
			// Restore normal border
			lv_obj_set_style_border_color(target, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

			// Hide Y baseline
			if (osc_y_baseline != NULL) {
				lv_obj_add_flag(osc_y_baseline, LV_OBJ_FLAG_HIDDEN);
			}
			if (osc_y_baseline_marker != NULL) {
				lv_obj_add_flag(osc_y_baseline_marker, LV_OBJ_FLAG_HIDDEN);
			}
		}
		break;
	}
	case LV_EVENT_PRESSED:
	{
		if (!osc_y_offset_active) break;

		// Initialize touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		osc_last_touch_y = point.y;
		break;
	}
	case LV_EVENT_PRESSING:
	{
		if (!osc_y_offset_active) break;

		// Get touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);

		// Calculate offset change based on vertical swipe
		if (osc_last_touch_y != 0) {
			lv_coord_t delta_y = osc_last_touch_y - point.y;  // Positive = swipe up = move up

			// Voltage scale values for offset calculation
			const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};
			float volts_per_div = volt_scale_values[osc_volt_scale_index];

			// Convert pixel movement to voltage offset (1 pixel = 1/40 division for 40x40 grid)
			float offset_change = (delta_y / 40.0f) * volts_per_div;
			osc_y_offset += offset_change;

			// Update display with appropriate unit based on voltage scale
			char offset_str[32];
			format_voltage_offset(offset_str, sizeof(offset_str), osc_y_offset, osc_volt_scale_index);
			lv_label_set_text(guider_ui.scrOscilloscope_labelYOffsetValue, offset_str);
		}
		osc_last_touch_y = point.y;
		break;
	}
	case LV_EVENT_RELEASED:
	{
		osc_last_touch_y = 0;
		break;
	}
	default:
		break;
	}
}

// AUTO button event handler - Improved professional auto-adjust algorithm
static void scrOscilloscope_btnAuto_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// Visual feedback - show "AUTO" is running
		lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnAuto, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_label_set_text(guider_ui.scrOscilloscope_btnAuto_label, "WAIT");
		lv_refr_now(NULL);

		// Analyze current waveform to determine optimal settings
		lv_chart_series_t *ser = lv_chart_get_series_next(guider_ui.scrOscilloscope_chartWaveform, NULL);
		if (ser != NULL) {
			// Chart display parameters - NEW FIXED SIZE ARCHITECTURE
			const float chart_center = 500.0f;  // Y center in chart units (0-1000 range)
			const float units_per_division = 100.0f;  // 100 chart units = 1 division
			const int num_points = 720;  // Number of data points
			const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};
			const char *volt_scales[] = {"10mV", "20mV", "50mV", "100mV", "200mV", "500mV", "1V", "2V", "5V", "12V"};
			const char *time_scales[] = {"1us", "10us", "20us", "50us", "100us", "200us", "500us", "1ms", "10ms", "100ms", "1s"};

			// Step 1: Convert chart coordinates to actual voltage values
			float min_voltage = 1000.0f, max_voltage = -1000.0f;
			float current_volts_per_div = volt_scale_values[osc_volt_scale_index];
			float current_units_per_volt = units_per_division / current_volts_per_div;

			for(int i = 0; i < num_points; i++) {
				// Convert Y coordinate to voltage
				float voltage = ((float)ser->y_points[i] - chart_center) / current_units_per_volt;
				if (voltage < min_voltage) min_voltage = voltage;
				if (voltage > max_voltage) max_voltage = voltage;
			}

			// Step 2: Calculate actual peak-to-peak voltage and signal range
			float vpp = max_voltage - min_voltage;
			float signal_center = (max_voltage + min_voltage) / 2.0f;

			// Step 3: Determine optimal voltage scale
			// Professional oscilloscope approach:
			// - Display has 10 vertical divisions (5 above center, 5 below)
			// - Signal should fit within 8 divisions (4 above, 4 below center)
			// - This leaves 1 division margin at top and bottom

			int best_volt_idx = 6;  // Default to 1V

			// Calculate required divisions to display the signal with margin
			// We need to fit the signal from min to max, centered at signal_center
			// Maximum excursion from center: max(|max_voltage - signal_center|, |min_voltage - signal_center|)
			float max_excursion = fmaxf(fabsf(max_voltage - signal_center), fabsf(min_voltage - signal_center));

			// We want max_excursion to fit within 4 divisions (leaving 1 div margin)
			// So: max_excursion / volts_per_div <= 4.0
			// Or: volts_per_div >= max_excursion / 4.0
			float min_volts_per_div = max_excursion / 4.0f;

			// Find the smallest standard scale that is >= min_volts_per_div
			for(int i = 0; i < 10; i++) {
				if (volt_scale_values[i] >= min_volts_per_div) {
					best_volt_idx = i;
					break;
				}
			}

			// Safety check: if signal is too large even for 12V/div, use 12V/div
			if (best_volt_idx == 6 && min_volts_per_div > volt_scale_values[9]) {
				best_volt_idx = 9;  // 12V/div
			}

			osc_volt_scale_index = best_volt_idx;

			// Step 4: Determine optimal time scale based on frequency
			// Count zero crossings and measure period more accurately
			int zero_crossings = 0;
			int last_crossing_pos = -1;
			int total_period_samples = 0;
			int period_count = 0;

			for(int i = 1; i < num_points; i++) {
				// Detect upward zero crossing (more reliable than both directions)
				if (ser->y_points[i-1] < chart_center && ser->y_points[i] >= chart_center) {
					zero_crossings++;
					if (last_crossing_pos >= 0) {
						total_period_samples += (i - last_crossing_pos);
						period_count++;
					}
					last_crossing_pos = i;
				}
			}

			// Calculate average period in samples
			float avg_period_samples = (period_count > 0) ? ((float)total_period_samples / (float)period_count) : 720.0f;

			// Target: Show 2-3 complete cycles on screen (720 samples total)
			// Ideal samples per cycle: 720 / 2.5 = 288 samples
			float target_samples_per_cycle = 288.0f;

			int best_time_idx = 2;  // Default to 20us
			float best_time_score = 1000.0f;

			// Time scale factors (relative to base frequency)
			const float time_scale_factors[] = {0.5, 0.1, 0.05, 0.02, 0.01, 0.005, 0.002, 0.001, 0.0001, 0.00001, 0.000001};

			for(int i = 0; i < 11; i++) {
				// Estimate how many samples per cycle at this time scale
				// Smaller time_scale_factor = faster sweep = fewer samples per cycle
				float estimated_samples = avg_period_samples / time_scale_factors[i] * time_scale_factors[2];  // Normalize to current scale
				float time_score = fabsf(estimated_samples - target_samples_per_cycle);

				if (time_score < best_time_score) {
					best_time_score = time_score;
					best_time_idx = i;
				}
			}

			osc_time_scale_index = best_time_idx;

			// Step 5: Update display with new settings
			lv_label_set_text(guider_ui.scrOscilloscope_labelTimeScaleValue, time_scales[osc_time_scale_index]);
			lv_label_set_text(guider_ui.scrOscilloscope_labelVoltScaleValue, volt_scales[osc_volt_scale_index]);

			// Step 6: Reset offsets to center
			osc_x_offset = 0.0f;
			osc_y_offset = 0.0f;
			lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, "0us");
			lv_label_set_text(guider_ui.scrOscilloscope_labelYOffsetValue, "0V");
		}

		// Simulate analysis delay
		vTaskDelay(pdMS_TO_TICKS(300));

		// Restore button appearance
		lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnAuto, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_label_set_text(guider_ui.scrOscilloscope_btnAuto_label, "AUTO");

		break;
	}
	default:
		break;
	}
}

// Waveform area gesture handler - Handle X/Y offset adjustment via swipe
static void scrOscilloscope_contWaveform_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	static lv_coord_t waveform_touch_start_x = 0;
	static lv_coord_t waveform_touch_start_y = 0;

	switch (code) {
	case LV_EVENT_PRESSED:
	{
		// Record initial touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		waveform_touch_start_x = point.x;
		waveform_touch_start_y = point.y;
		break;
	}
	case LV_EVENT_PRESSING:
	{
		// Get current touch position
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);

		// Calculate movement delta
		lv_coord_t delta_x = point.x - waveform_touch_start_x;
		lv_coord_t delta_y = point.y - waveform_touch_start_y;

		// Only process if X or Y offset is active
		if (osc_x_offset_active && waveform_touch_start_x != 0) {
			// Horizontal swipe for X offset (time offset)
			const float time_scale_values[] = {1e-6, 10e-6, 20e-6, 50e-6, 100e-6, 200e-6, 500e-6, 1e-3, 10e-3, 100e-3, 1.0};
			float time_per_div = time_scale_values[osc_time_scale_index];

			// Convert pixel movement to time offset (1 pixel = 1/40 division for 40x40 grid)
			float offset_change = (delta_x / 40.0f) * time_per_div;
			osc_x_offset += offset_change;

			// Update display with appropriate unit based on time scale
			char offset_str[32];
			format_time_offset(offset_str, sizeof(offset_str), osc_x_offset, osc_time_scale_index);
			lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, offset_str);

			// If in STOP mode, trigger immediate waveform redraw
			if (!osc_running && osc_frozen_data_valid) {
				osc_waveform_update_cb(NULL);
			}

			// Update start position for next delta
			waveform_touch_start_x = point.x;
		}

		if (osc_y_offset_active && waveform_touch_start_y != 0) {
			// Vertical swipe for Y offset (voltage offset)
			const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};
			float volts_per_div = volt_scale_values[osc_volt_scale_index];

			// Convert pixel movement to voltage offset (1 pixel = 1/40 division for 40x40 grid)
			// Negative delta_y because screen Y increases downward
			float offset_change = (-delta_y / 40.0f) * volts_per_div;
			osc_y_offset += offset_change;

			// Update display with appropriate unit based on voltage scale
			char offset_str[32];
			format_voltage_offset(offset_str, sizeof(offset_str), osc_y_offset, osc_volt_scale_index);
			lv_label_set_text(guider_ui.scrOscilloscope_labelYOffsetValue, offset_str);

			// Update Y baseline position
			if (osc_y_baseline != NULL) {
				// Calculate baseline Y position based on voltage offset
				// Center is at Y=200, pixels_per_volt = 40 / volts_per_div
				float pixels_per_volt = 40.0f / volts_per_div;
				int baseline_y = 200 - (int)(osc_y_offset * pixels_per_volt);

				// Clamp to display range
				if (baseline_y < 0) baseline_y = 0;
				if (baseline_y > 400) baseline_y = 400;

				// Update line position
				static lv_point_t y_baseline_points[2];
				y_baseline_points[0].x = 0;
				y_baseline_points[0].y = baseline_y;
				y_baseline_points[1].x = 720;
				y_baseline_points[1].y = baseline_y;
				lv_line_set_points(osc_y_baseline, y_baseline_points, 2);

				// Update marker position and text
				if (osc_y_baseline_marker != NULL) {
					lv_obj_set_pos(osc_y_baseline_marker, 5, baseline_y - 10);
					lv_label_set_text(osc_y_baseline_marker, offset_str);
				}
			}

			// If in STOP mode, trigger immediate waveform redraw
			if (!osc_running && osc_frozen_data_valid) {
				osc_waveform_update_cb(NULL);
			}

			// Update start position for next delta
			waveform_touch_start_y = point.y;
		}

		break;
	}
	case LV_EVENT_RELEASED:
	{
		// Reset touch tracking
		waveform_touch_start_x = 0;
		waveform_touch_start_y = 0;
		break;
	}
	default:
		break;
	}
}

// Initialize all event handlers for oscilloscope screen
void events_init_scrOscilloscope(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrOscilloscope, scrOscilloscope_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnBack, scrOscilloscope_btnBack_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnAuto, scrOscilloscope_btnAuto_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnStartStop, scrOscilloscope_btnStartStop_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnPanZoom, scrOscilloscope_btnPanZoom_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnFFT, scrOscilloscope_btnFFT_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnExport, scrOscilloscope_btnExport_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contWaveform, scrOscilloscope_contWaveform_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contTimeScale, scrOscilloscope_contTimeScale_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contVoltScale, scrOscilloscope_contVoltScale_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contXOffset, scrOscilloscope_contXOffset_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contYOffset, scrOscilloscope_contYOffset_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contTrigger, scrOscilloscope_contTrigger_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contCoupling, scrOscilloscope_contCoupling_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contTriggerMode, scrOscilloscope_contTriggerMode_event_handler, LV_EVENT_ALL, ui);
}

