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

/* Wireless Serial module */
#include "wireless_serial.h"

/* Font declarations for Chinese support */
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_16)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_18)

/* ESP-DSP library for hardware-accelerated FFT */
#include "esp_dsp.h"

/* High-performance oscilloscope drawing module */
#include "oscilloscope_draw.h"

/* Oscilloscope integration layer - Real ADC sampling */
#include "oscilloscope_integration.h"
#include "oscilloscope_core.h"

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

/* Helper function to set mode indicator style */
static void ps_set_mode_indicator_style(const char* mode_text, uint32_t bg_color, uint32_t grad_color, uint32_t border_color, uint32_t shadow_color)
{
	lv_label_set_text(guider_ui.scrPowerSupply_labelModeStatus, mode_text);
	lv_obj_set_style_bg_color(guider_ui.scrPowerSupply_labelModeStatus, lv_color_hex(bg_color), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(guider_ui.scrPowerSupply_labelModeStatus, lv_color_hex(grad_color), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(guider_ui.scrPowerSupply_labelModeStatus, lv_color_hex(border_color), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(guider_ui.scrPowerSupply_labelModeStatus, lv_color_hex(shadow_color), LV_PART_MAIN|LV_STATE_DEFAULT);
}

/* Set CV mode style - blue theme */
static void ps_set_cv_mode_style(void)
{
	ps_set_mode_indicator_style("CV", 0x3498db, 0x2980b9, 0x5dade2, 0x2980b9);
}

/* Set CC mode style - orange theme */
static void ps_set_cc_mode_style(void)
{
	ps_set_mode_indicator_style("CC", 0xf39c12, 0xe67e22, 0xf5b041, 0xe67e22);
}

/* Set disabled mode style - gray theme */
static void ps_set_disabled_mode_style(void)
{
	ps_set_mode_indicator_style("--", 0x95a5a6, 0x7f8c8d, 0xbdc3c7, 0x7f8c8d);
}

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
		// Toggle mode based on current state
		if (lv_obj_has_state(guider_ui.scrPowerSupply_labelModeStatus, LV_STATE_CHECKED)) {
			// Currently CC, switch to CV
			ps_cc_mode = false;
			ps_set_cv_mode_style();
			lv_obj_clear_state(guider_ui.scrPowerSupply_labelModeStatus, LV_STATE_CHECKED);
		} else {
			// Currently CV, switch to CC
			ps_cc_mode = true;
			ps_set_cc_mode_style();
			lv_obj_add_state(guider_ui.scrPowerSupply_labelModeStatus, LV_STATE_CHECKED);
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

		if (!ps_output_enabled) {
			// Output disabled - reset displays to zero
			lv_label_set_text(guider_ui.scrPowerSupply_labelVoltageValue, "0.00");
			lv_label_set_text(guider_ui.scrPowerSupply_labelCurrentValue, "0.00");
			lv_label_set_text(guider_ui.scrPowerSupply_labelPowerValue, "0.0000");

			// Show disabled mode
			ps_set_disabled_mode_style();

			// Update chart with zero
			lv_chart_set_next_value(guider_ui.scrPowerSupply_chartPower, guider_ui.scrPowerSupply_chartPowerSeries, 0);
		} else {
			// Output enabled - restore mode display
			if (ps_cc_mode) {
				ps_set_cc_mode_style();
			} else {
				ps_set_cv_mode_style();
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
	if (is_cc_mode) {
		ps_set_cc_mode_style();
		lv_obj_add_state(guider_ui.scrPowerSupply_labelModeStatus, LV_STATE_CHECKED);
	} else {
		ps_set_cv_mode_style();
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
		lv_obj_set_style_text_font(lv_obj_get_child(btn, 1), &lv_font_ShanHaiZhongXiaYeWuYuW_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	} else {
		// Static storage for SSID strings (needed because pointers must remain valid)
		static char stored_ssids[15][64];

		// Add WiFi networks to list
		for (int i = 0; i < count && i < 15; i++) {  // Limit to 15 networks
			char item_text[128];
			const char *signal_icon = LV_SYMBOL_WIFI;

			// Store SSID in static buffer
			strncpy(stored_ssids[i], wifi_networks[i], sizeof(stored_ssids[i]) - 1);
			stored_ssids[i][sizeof(stored_ssids[i]) - 1] = '\0';

			// Determine signal strength icon and color
			lv_color_t signal_color;
			if (signal_strengths[i] > -50) {
				signal_icon = LV_SYMBOL_WIFI;  // Strong
				signal_color = lv_color_hex(0x4caf50);  // Green
			} else if (signal_strengths[i] > -70) {
				signal_icon = LV_SYMBOL_WIFI;  // Medium
				signal_color = lv_color_hex(0xff9800);  // Orange
			} else {
				signal_icon = LV_SYMBOL_WIFI;  // Weak
				signal_color = lv_color_hex(0xf44336);  // Red
			}

			snprintf(item_text, sizeof(item_text), "%s (%d dBm)", wifi_networks[i], signal_strengths[i]);
			lv_obj_t * btn = lv_list_add_btn(guider_ui.scrSettings_listWifi, signal_icon, item_text);

			// Set font to support Chinese characters
			lv_obj_set_style_text_font(lv_obj_get_child(btn, 1), &lv_font_ShanHaiZhongXiaYeWuYuW_16, LV_PART_MAIN|LV_STATE_DEFAULT);

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
		// Update IP display when screen loads
		wireless_serial_update_ip_on_screen_load();
		
		// Start TCP server automatically
		wireless_serial_start_server();
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
			size_t send_len;

			// Always send as ASCII (HEX conversion not implemented)
			send_len = snprintf(send_buffer, sizeof(send_buffer), "%s%s",
			                    text, ws_send_newline ? "\r\n" : "");
			
			if (ws_hex_send) {
				ESP_LOGI("WS_SEND", "Sending (HEX mode): %s", text);
			} else {
				ESP_LOGI("WS_SEND", "Sending (ASCII mode): %s", text);
			}

			// ESP32 Integration Point: Send data via WiFi
			extern void wireless_serial_send_data(const char *data, size_t len);
			wireless_serial_send_data(send_buffer, send_len);

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

// FFT display parameters
static int osc_fft_freq_range_index = 2;  // 0=1kHz, 1=10kHz, 2=25kHz (Nyquist), 3=100kHz, 4=1MHz
static const float osc_fft_freq_ranges[] = {1000.0f, 10000.0f, 25000.0f, 100000.0f, 1000000.0f};
static const char *osc_fft_freq_range_labels[] = {"1kHz", "10kHz", "25kHz", "100kHz", "1MHz"};
static int osc_fft_amp_range_index = 2;  // 0=-20dB, 1=-40dB, 2=-60dB, 3=-80dB, 4=-100dB
static const float osc_fft_amp_ranges[] = {20.0f, 40.0f, 60.0f, 80.0f, 100.0f};
static const char *osc_fft_amp_range_labels[] = {"-20dB", "-40dB", "-60dB", "-80dB", "-100dB"};

// Cursor measurement mode
typedef enum {
	OSC_CURSOR_OFF = 0,      // 游标关闭
	OSC_CURSOR_HORIZONTAL,   // 横轴游标（时间/频率）
	OSC_CURSOR_VERTICAL,     // 纵轴游标（电压/幅值）
} osc_cursor_mode_t;

static osc_cursor_mode_t osc_cursor_mode = OSC_CURSOR_OFF;
static lv_obj_t *osc_cursor_line = NULL;        // 游标线对象
static lv_obj_t *osc_cursor_label = NULL;       // 游标值标签
static int osc_cursor_position = 360;           // 游标位置（像素）
static lv_timer_t *osc_waveform_timer = NULL;
static int osc_waveform_phase = 0;
static int osc_time_scale_index = 14;  // Start at 1ms (index 14, matching UI default "1ms")
static int osc_volt_scale_index = 7;   // Start at 2V (index 7)
static float osc_x_offset = 0.0;  // X axis offset - horizontal offset of trigger point from screen center (in seconds)
static float osc_y_offset = 0.0;  // Y axis offset - vertical offset of channel zero point from screen center (in volts)
static float osc_x_offset_base = 0.0;  // X offset base value when drag starts
static float osc_y_offset_base = 0.0;  // Y offset base value when drag starts
static bool osc_x_offset_active = false;  // X offset control active
static bool osc_y_offset_active = false;  // Y offset control active
static lv_coord_t osc_last_touch_y = 0;  // Last touch Y position for offset control

// High-performance drawing context (replaces chart widget)
static osc_draw_ctx_t *osc_draw_ctx = NULL;
static bool osc_use_hw_accel = false;  // 禁用硬件加速，Chart 模式性能更好

// Grid and display constants (matching setup_scr_scrOscilloscope.c)
// WAVEFORM_WIDTH = GRID_SIZE * GRID_COLS + 4 = 43 * 16 + 4 = 692
// Chart point count = WAVEFORM_WIDTH - 4 = 688
#define OSC_GRID_SIZE           43
#define OSC_GRID_COLS           16
#define OSC_GRID_ROWS           9
#define OSC_WAVEFORM_WIDTH      (OSC_GRID_SIZE * OSC_GRID_COLS + 4)  // 692
#define OSC_GRID_WIDTH          (OSC_WAVEFORM_WIDTH - 4)  // 688 data points
#define OSC_GRID_HEIGHT         (OSC_GRID_SIZE * OSC_GRID_ROWS + 4 - 6)  // 385

// Chart Y range constants
#define OSC_CHART_Y_MIN         0
#define OSC_CHART_Y_MAX         1000
#define OSC_CHART_Y_CENTER      500
#define OSC_CHART_Y_RANGE       1000

// Frozen waveform data for STOP mode - 完整采集的波形数据
static float osc_frozen_voltage_data[OSC_GRID_WIDTH];  // Store actual voltage values when frozen
static bool osc_frozen_data_valid = false;
static int osc_frozen_time_scale_index = 1;  // Time scale when frozen
static int osc_frozen_volt_scale_index = 7;  // Voltage scale when frozen
static float osc_frozen_x_offset_at_stop = 0.0f;  // X offset when stopped (trigger position)

// Trigger settings
static float osc_trigger_voltage = 2.54f;  // Trigger voltage level in volts
static int osc_trigger_mode = 0;  // 0=RISE, 1=FALL, 2=EDGE
static lv_obj_t *osc_trigger_line = NULL;  // Trigger level indicator line
static lv_obj_t *osc_trigger_marker = NULL;  // Trigger marker label
static bool osc_trigger_active = false;  // Trigger voltage control active

// Y offset baseline indicator
static lv_obj_t *osc_y_baseline = NULL;  // Y offset baseline (horizontal line)
static lv_obj_t *osc_y_baseline_marker = NULL;  // Y offset marker label

// Waveform preview mask objects (for showing visible data window)
static lv_obj_t *osc_preview_mask_left = NULL;   // Left mask (hidden data)
static lv_obj_t *osc_preview_mask_right = NULL;  // Right mask (hidden data)
static lv_obj_t *osc_preview_trigger_line = NULL;  // Trigger position indicator

// Time scale values in seconds per division (s/div)
// Larger value = slower sweep = fewer cycles visible
static const float time_scale_values[] = {
	10e-9,   // 10ns/div
	50e-9,   // 50ns/div
	100e-9,  // 100ns/div
	200e-9,  // 200ns/div
	500e-9,  // 500ns/div
	1e-6,    // 1us/div
	2e-6,    // 2us/div
	5e-6,    // 5us/div
	10e-6,   // 10us/div
	20e-6,   // 20us/div
	50e-6,   // 50us/div
	100e-6,  // 100us/div
	200e-6,  // 200us/div
	500e-6,  // 500us/div
	1e-3,    // 1ms/div
	2e-3,    // 2ms/div
	5e-3,    // 5ms/div
	10e-3,   // 10ms/div
	20e-3,   // 20ms/div
	50e-3,   // 50ms/div
	100e-3,  // 100ms/div
	200e-3,  // 200ms/div
	500e-3,  // 500ms/div
	1.0      // 1s/div
};
static const char *time_scale_labels[] = {
	"10ns", "50ns", "100ns", "200ns", "500ns",
	"1us", "2us", "5us", "10us", "20us", "50us",
	"100us", "200us", "500us",
	"1ms", "2ms", "5ms", "10ms", "20ms", "50ms",
	"100ms", "200ms", "500ms", "1s"
};
#define TIME_SCALE_COUNT (sizeof(time_scale_values) / sizeof(time_scale_values[0]))

// Voltage scale values in volts per division (V/div)
static const float volt_scale_values[] = {
	0.01,   // 10mV/div
	0.02,   // 20mV/div
	0.05,   // 50mV/div
	0.1,    // 100mV/div
	0.2,    // 200mV/div
	0.5,    // 500mV/div
	1.0,    // 1V/div
	2.0,    // 2V/div
	5.0,    // 5V/div
	10.0    // 10V/div
};
static const char *volt_scale_labels[] = {
	"10mV", "20mV", "50mV", "100mV", "200mV", "500mV", "1V", "2V", "5V", "10V"
};
#define VOLT_SCALE_COUNT (sizeof(volt_scale_values) / sizeof(volt_scale_values[0]))

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

// Helper function to update waveform preview mask based on X offset
// 真实示波器行为：
// - 蓝色遮罩表示不在当前显示窗口的数据（隐藏数据）
// - 横轴偏移时，遮罩位置相应变化
// - 停止模式下，只能查看已采集的波形数据
static void update_preview_mask(void)
{
	if (!osc_frozen_data_valid || osc_running) {
		// 运行模式或没有冻结数据：不显示遮罩（或显示默认遮罩）
		if (osc_preview_mask_left != NULL) {
			lv_obj_add_flag(osc_preview_mask_left, LV_OBJ_FLAG_HIDDEN);
		}
		if (osc_preview_mask_right != NULL) {
			lv_obj_add_flag(osc_preview_mask_right, LV_OBJ_FLAG_HIDDEN);
		}
		return;
	}

	// 停止模式：根据X偏移更新遮罩位置
	if (osc_preview_mask_left == NULL || osc_preview_mask_right == NULL) {
		return;  // 遮罩对象未创建
	}

	// 获取预览区域的宽度
	lv_obj_t *preview_container = guider_ui.scrOscilloscope_sliderWavePos;
	if (preview_container == NULL) return;
	
	lv_coord_t preview_w = lv_obj_get_width(preview_container);
	lv_coord_t preview_h = lv_obj_get_height(preview_container) - 4;

	// 计算X偏移对应的预览区域位置
	// X偏移范围：-max_offset 到 +max_offset
	// 预览区域：0 到 preview_w
	float time_per_div = time_scale_values[osc_time_scale_index];
	float max_offset = time_per_div * (float)OSC_GRID_COLS;  // 一个屏幕宽度
	
	// 相对偏移（相对于停止时的触发位置）
	float relative_x_offset = osc_x_offset - osc_frozen_x_offset_at_stop;
	
	// 归一化偏移：-1.0（完全向左）到 +1.0（完全向右）
	float normalized_offset = relative_x_offset / max_offset;
	// 限制范围
	if (normalized_offset < -1.0f) normalized_offset = -1.0f;
	if (normalized_offset > 1.0f) normalized_offset = 1.0f;
	
	// 计算可见窗口在预览区域中的位置
	// 中心位置：preview_w / 2
	// 偏移量：normalized_offset * preview_w / 2
	float visible_center = (float)preview_w / 2.0f - normalized_offset * (float)preview_w / 2.0f;
	
	// 可见窗口宽度（固定为预览区域的中间部分，例如40%）
	float visible_width = (float)preview_w * 0.4f;
	float visible_left = visible_center - visible_width / 2.0f;
	float visible_right = visible_center + visible_width / 2.0f;
	
	// 更新左侧遮罩
	if (visible_left > 0) {
		lv_obj_clear_flag(osc_preview_mask_left, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_pos(osc_preview_mask_left, 0, 0);
		lv_obj_set_size(osc_preview_mask_left, (lv_coord_t)visible_left, preview_h);
	} else {
		lv_obj_add_flag(osc_preview_mask_left, LV_OBJ_FLAG_HIDDEN);
	}
	
	// 更新右侧遮罩
	if (visible_right < preview_w) {
		lv_obj_clear_flag(osc_preview_mask_right, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_pos(osc_preview_mask_right, (lv_coord_t)visible_right, 0);
		lv_obj_set_size(osc_preview_mask_right, preview_w - (lv_coord_t)visible_right, preview_h);
	} else {
		lv_obj_add_flag(osc_preview_mask_right, LV_OBJ_FLAG_HIDDEN);
	}
	
	// 更新触发位置指示器
	if (osc_preview_trigger_line != NULL) {
		// 触发位置在预览区域的中心（停止时的位置）
		float trigger_pos = (float)preview_w / 2.0f - normalized_offset * (float)preview_w / 2.0f;
		lv_obj_set_x(osc_preview_trigger_line, (lv_coord_t)trigger_pos);
	}
}

// Waveform update timer callback - Generate dynamic waveform data
// Grid: 43x43 pixels per division, 16 columns x 9 rows
// Time scale logic (Real Oscilloscope Behavior):
// - Larger time/div = MORE compressed waveform = MORE cycles visible in same screen width
// - Smaller time/div = MORE expanded waveform = FEWER cycles visible (zoomed in)
// - Horizontal offset allows viewing different parts of the waveform
// Example: 1kHz signal (period=1ms)
//   - At 1ms/div with 16 divisions: total_time = 16ms, shows 16 cycles (compressed)
//   - At 100us/div with 16 divisions: total_time = 1.6ms, shows 1.6 cycles (expanded)
//   - At 10us/div with 16 divisions: total_time = 160us, shows 0.16 cycles (highly zoomed)
static void osc_waveform_update_cb(lv_timer_t *timer)
{
	// Debug: Log timer callback execution (减少频率)
	static uint32_t timer_call_count = 0;
	timer_call_count++;
	if (timer_call_count <= 5 || timer_call_count % 1000 == 0) {
		ESP_LOGI("OSC_TIMER", "🔄 Timer callback #%lu executed", timer_call_count);
	}
	
	// Use hardware-accelerated drawing if available
	if (osc_use_hw_accel && osc_draw_ctx != NULL) {
		// Clear canvas
		osc_draw_clear(osc_draw_ctx);
		
		// Draw grid if enabled
		if (osc_grid_enabled) {
			osc_draw_grid(osc_draw_ctx);
		}
		
		// Get real ADC data from oscilloscope core
		float display_buffer[OSC_DISPLAY_WIDTH];
		uint32_t display_count = 0;
		
		// Update oscilloscope core (check for new ADC data)
		if (g_osc_core != NULL) {
			osc_core_update(g_osc_core);
			esp_err_t ret = osc_core_get_display_waveform(g_osc_core, display_buffer, &display_count);
			
			// Debug: Log data status every 500 frames (减少日志输出)
			static uint32_t frame_counter = 0;
			frame_counter++;
			if (frame_counter % 500 == 0) {
				ESP_LOGI("OSC_UI", "Frame %lu: ret=%s, count=%lu, running=%d", 
				         frame_counter, esp_err_to_name(ret), display_count,
				         osc_integration_is_running());
			}
		}
		
		// Prepare waveform parameters
		osc_waveform_params_t params;
		params.time_per_div = time_scale_values[osc_time_scale_index];
		params.volts_per_div = volt_scale_values[osc_volt_scale_index];
		params.x_offset = osc_x_offset;
		params.y_offset = osc_y_offset;
		params.fft_mode = osc_fft_enabled;
		
		// Pass real ADC data to drawing function
		params.voltage_buffer = (display_count > 0) ? display_buffer : NULL;
		params.voltage_count = display_count;
		
		// Debug: Log first few voltage values when we have data (减少日志输出)
		if (display_count > 0) {
			static uint32_t data_log_counter = 0;
			data_log_counter++;
			if (data_log_counter % 500 == 0) {
				ESP_LOGI("OSC_UI", "Data sample: [0]=%.3fV, [1]=%.3fV, [2]=%.3fV", 
				         display_buffer[0], display_buffer[1], display_buffer[2]);
			}
		}
		
		// Draw waveform or FFT
		if (osc_fft_enabled) {
			osc_draw_fft(osc_draw_ctx, &params);
		} else {
			osc_draw_waveform(osc_draw_ctx, &params);
		}
		
		// Update display
		osc_draw_update(osc_draw_ctx);
		
		// Update measurements from real ADC data
		if (g_osc_core != NULL) {
			char buf[64];
			
			if (osc_fft_enabled) {
				// FFT mode - show frequency domain measurements
				float freq_hz, vmax, vmin, vpp, vrms;
				if (osc_core_get_measurements(g_osc_core, &freq_hz, &vmax, &vmin, &vpp, &vrms) == ESP_OK) {
					// Fundamental frequency
					if (guider_ui.scrOscilloscope_labelFreqTitle != NULL) {
						if (freq_hz >= 1e6f) {
							snprintf(buf, sizeof(buf), "Fund: %.2fMHz", freq_hz / 1e6f);
						} else if (freq_hz >= 1e3f) {
							snprintf(buf, sizeof(buf), "Fund: %.2fkHz", freq_hz / 1e3f);
						} else {
							snprintf(buf, sizeof(buf), "Fund: %.1fHz", freq_hz);
						}
						lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, buf);
					}
					
					// Calculate sampling rate and Nyquist frequency
					float total_time = params.time_per_div * (float)OSC_GRID_COLS;
					float sampling_rate = display_count / total_time;
					float nyquist_freq = sampling_rate / 2.0f;
					
					// Peak amplitude (from Vpp)
					if (guider_ui.scrOscilloscope_labelVmaxTitle != NULL) {
						snprintf(buf, sizeof(buf), "Peak: %.2fV", vpp / 2.0f);
						lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, buf);
					}
					
					// Nyquist frequency
					if (guider_ui.scrOscilloscope_labelVminTitle != NULL) {
						if (nyquist_freq >= 1e6f) {
							snprintf(buf, sizeof(buf), "Nyq: %.1fMHz", nyquist_freq / 1e6f);
						} else if (nyquist_freq >= 1e3f) {
							snprintf(buf, sizeof(buf), "Nyq: %.1fkHz", nyquist_freq / 1e3f);
						} else {
							snprintf(buf, sizeof(buf), "Nyq: %.0fHz", nyquist_freq);
						}
						lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, buf);
					}
					
					// THD (placeholder - would need harmonic analysis)
					if (guider_ui.scrOscilloscope_labelVppTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, "THD: ---");
					}
					
					// SFDR (placeholder - would need spurious analysis)
					if (guider_ui.scrOscilloscope_labelVrmsTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, "SFDR: ---");
					}
				} else {
					// No valid measurements - show waiting message
					if (guider_ui.scrOscilloscope_labelFreqTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, "Fund: ---");
					}
					if (guider_ui.scrOscilloscope_labelVmaxTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, "Peak: ---");
					}
					if (guider_ui.scrOscilloscope_labelVminTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, "Nyq: ---");
					}
					if (guider_ui.scrOscilloscope_labelVppTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, "THD: ---");
					}
					if (guider_ui.scrOscilloscope_labelVrmsTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, "SFDR: ---");
					}
				}
			} else {
				// Time domain mode - show normal measurements
				float freq_hz, vmax, vmin, vpp, vrms;
				if (osc_core_get_measurements(g_osc_core, &freq_hz, &vmax, &vmin, &vpp, &vrms) == ESP_OK) {
					// Frequency
					if (guider_ui.scrOscilloscope_labelFreqTitle != NULL) {
						if (freq_hz >= 1e6f) {
							snprintf(buf, sizeof(buf), "Freq: %.2fMHz", freq_hz / 1e6f);
						} else if (freq_hz >= 1e3f) {
							snprintf(buf, sizeof(buf), "Freq: %.2fkHz", freq_hz / 1e3f);
						} else {
							snprintf(buf, sizeof(buf), "Freq: %.1fHz", freq_hz);
						}
						lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, buf);
					}
					
					// Vmax
					if (guider_ui.scrOscilloscope_labelVmaxTitle != NULL) {
						snprintf(buf, sizeof(buf), "Vmax: %.2fV", vmax);
						lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, buf);
					}
					
					// Vmin
					if (guider_ui.scrOscilloscope_labelVminTitle != NULL) {
						snprintf(buf, sizeof(buf), "Vmin: %.2fV", vmin);
						lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, buf);
					}
					
					// Vpp
					if (guider_ui.scrOscilloscope_labelVppTitle != NULL) {
						snprintf(buf, sizeof(buf), "Vp-p: %.2fV", vpp);
						lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, buf);
					}
					
					// Vrms
					if (guider_ui.scrOscilloscope_labelVrmsTitle != NULL) {
						snprintf(buf, sizeof(buf), "Vrms: %.2fV", vrms);
						lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, buf);
					}
				} else {
					// No valid measurements - show waiting message
					if (guider_ui.scrOscilloscope_labelFreqTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, "Freq: ---");
					}
					if (guider_ui.scrOscilloscope_labelVmaxTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, "Vmax: ---");
					}
					if (guider_ui.scrOscilloscope_labelVminTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, "Vmin: ---");
					}
					if (guider_ui.scrOscilloscope_labelVppTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, "Vp-p: ---");
					}
					if (guider_ui.scrOscilloscope_labelVrmsTitle != NULL) {
						lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, "Vrms: ---");
					}
				}
			}
		}
		
		return;
	}
	
	// Fallback to original chart-based rendering
	lv_chart_series_t *ser = lv_chart_get_series_next(guider_ui.scrOscilloscope_chartWaveform, NULL);
	if (ser == NULL) return;

	// Chart display parameters - 43x43 pixel grid, 16x9 divisions
	const int num_points = OSC_GRID_WIDTH;  // 688 data points (WAVEFORM_WIDTH - 4)
	const float chart_center = 500.0f;      // Y center (0V reference) for 0-1000 range
	const float chart_range = 1000.0f;      // Full Y range (0-1000)
	const float display_divisions = (float)OSC_GRID_ROWS;  // 9 vertical divisions

	// If not running (STOP mode), display frozen data with current scale settings and offsets
	// 真实示波器行为：
	// 1. 停止时冻结整个波形窗口（记录触发点位置）
	// 2. 横轴偏移时，只显示已采集的波形数据
	// 3. 超出原始窗口的部分不显示（显示为空白或零）
	if (!osc_running && osc_frozen_data_valid) {
		// Get current voltage scale
		float volts_per_div = volt_scale_values[osc_volt_scale_index];
		// Calculate units per volt: chart has 9 divisions, each division = chart_range/9 units
		float units_per_division = chart_range / display_divisions;
		float units_per_volt = units_per_division / volts_per_div;

		// Get current time scale
		float time_per_div = time_scale_values[osc_time_scale_index];
		float frozen_time_per_div = time_scale_values[osc_frozen_time_scale_index];
		// Time scale ratio for zoom effect
		float time_scale_ratio = frozen_time_per_div / time_per_div;

		// Calculate X offset in pixels based on horizontal offset
		// X offset represents the shift from the frozen trigger position
		float pixels_per_div = (float)num_points / (float)OSC_GRID_COLS;
		float pixels_per_second = pixels_per_div / time_per_div;
		// Total offset = current offset - frozen offset (relative to stop position)
		float relative_x_offset = osc_x_offset - osc_frozen_x_offset_at_stop;
		int x_offset_pixels = (int)(relative_x_offset * pixels_per_second);

		// Redraw frozen waveform with current scale and offsets
		for(int i = 0; i < num_points; i++) {
			// Calculate source position in frozen data
			// Apply time scale ratio and horizontal offset
			float source_pos = ((float)i - (float)num_points / 2.0f) * time_scale_ratio + (float)num_points / 2.0f - x_offset_pixels;
			int source_index = (int)source_pos;

			float voltage;
			// 真实示波器行为：超出原始采集窗口的数据不显示
			if (source_index < 0 || source_index >= num_points) {
				// 超出范围，显示为零（或者可以显示为空白）
				voltage = 0.0f;
			} else {
				// Linear interpolation for smooth scaling
				float frac = source_pos - (float)source_index;
				if (source_index + 1 < num_points && frac > 0.0f) {
					voltage = osc_frozen_voltage_data[source_index] * (1.0f - frac) +
					          osc_frozen_voltage_data[source_index + 1] * frac;
				} else {
					voltage = osc_frozen_voltage_data[source_index];
				}
			}

			// Apply Y offset (vertical position of channel zero point)
			voltage += osc_y_offset;
			// IMPORTANT: LVGL chart Y-axis: 0=bottom, 1000=top (inverted!)
			float y_float = chart_center + (voltage * units_per_volt);

			int val = (int)(y_float + 0.5f);  // Round to nearest integer
			if (val < 0) val = 0;
			if (val > (int)chart_range) val = (int)chart_range;
			ser->y_points[i] = val;
		}

		lv_chart_refresh(guider_ui.scrOscilloscope_chartWaveform);
		
		// 更新波形预览区域的遮罩
		update_preview_mask();
		
		return;
	}

	// Chart fallback 模式下，无论运行还是停止都需要绘制
	// 运行模式：绘制实时 ADC 数据
	// 停止模式：绘制冻结的数据（已在上面处理）

	// Get current voltage scale (volts per division)
	float volts_per_div = volt_scale_values[osc_volt_scale_index];
	float units_per_division = chart_range / display_divisions;
	float units_per_volt = units_per_division / volts_per_div;

	// Time scale logic - Real oscilloscope behavior with scrolling
	// The time scale (s/div) determines how much time is represented per horizontal division
	// Total time visible on screen = time_per_div * number_of_horizontal_divisions
	float time_per_div = time_scale_values[osc_time_scale_index];
	float total_time_on_screen = time_per_div * (float)OSC_GRID_COLS;  // 16 horizontal divisions

	// Simulated signal: 1kHz sine wave (period = 1ms = 0.001s)
	const float signal_frequency = 1000.0f;  // 1kHz
	const float signal_period = 1.0f / signal_frequency;  // 0.001s = 1ms

	// Calculate how many complete cycles fit on screen at current time scale
	// cycles_on_screen = total_time_on_screen / signal_period
	// Example: At 100us/div: total_time = 1.6ms, cycles = 1.6ms / 1ms = 1.6 cycles
	// Example: At 1ms/div: total_time = 16ms, cycles = 16ms / 1ms = 16 cycles
	// Example: At 10ms/div: total_time = 160ms, cycles = 160ms / 1ms = 160 cycles
	// Example: At 100ms/div: total_time = 1.6s, cycles = 1600 cycles (极度压缩！)
	// Example: At 1s/div: total_time = 16s, cycles = 16000 cycles (屏幕只显示一小部分)
	float cycles_on_screen = total_time_on_screen / signal_period;
	
	// 真实示波器行为：不限制周期数
	// - 小时基：显示完整波形（如1-10个周期）
	// - 大时基：波形被极度压缩，屏幕只显示完整波形的一小部分
	// - 通过X-Pos偏移可以左右平移查看不同段落
	// 只设置最小值防止除零错误
	if (cycles_on_screen < 0.1f) cycles_on_screen = 0.1f;
	// 移除上限！允许任意多的周期数，真实反映时基设置

	// Angular frequency for waveform generation
	// omega = 2*PI * cycles / num_points
	// This gives us 'cycles' complete sine waves across 'num_points' samples
	float omega = 2.0f * M_PI * cycles_on_screen / (float)num_points;

	// Variables for measurement calculation
	float min_voltage = 0.0f, max_voltage = 0.0f;
	float sum_squares = 0;
	int zero_crossings = 0;
	int last_sign = 0;
	bool first_sample = true;

	// Signal amplitude constant (used in both time domain and FFT)
	const float signal_amplitude = 1.5f;  // ±1.5V peak (3V peak-to-peak)

	// FFT display points (used in FFT mode)
	const int fft_display_points = 256;  // Number of bars in FFT spectrum

	if (osc_fft_enabled) {
		// FFT mode - show frequency spectrum using REAL ADC data
		
		// Change waveform color to purple for FFT mode
		lv_chart_set_series_color(guider_ui.scrOscilloscope_chartWaveform, ser, lv_color_hex(0xE040FB));  // Purple
		
		// Get real ADC data from oscilloscope core
		float display_buffer[OSC_DISPLAY_WIDTH];
		uint32_t display_count = 0;
		
		// Calculate frequency parameters for display
		float sampling_rate = 50000.0f;  // 50 kHz (from ADC)
		float freq_resolution = sampling_rate / 512.0f;  // FFT bin resolution
		float nyquist_freq = sampling_rate / 2.0f;  // Maximum displayable frequency
		float signal_frequency = 1000.0f;  // Placeholder (will be calculated from FFT peak)
		
		// 谐波分析变量（在外部定义，以便在显示时使用）
		float fundamental_magnitude = 0.0f;  // 基波（1次谐波）
		float harmonic_3_magnitude = 0.0f;  // 3次谐波
		float thd = 0.0f;  // 总谐波失真
		
		if (g_osc_core != NULL) {
			osc_core_update(g_osc_core);
			esp_err_t ret = osc_core_get_display_waveform(g_osc_core, display_buffer, &display_count);
			
			if (ret == ESP_OK && display_count > 0) {
				// We have real ADC data - perform FFT using ESP-DSP hardware acceleration
				const int fft_size = 512;  // Use 512 points for FFT (power of 2)
				
				// ESP-DSP requires aligned buffers
				__attribute__((aligned(16))) static float fft_input_real[512];
				__attribute__((aligned(16))) static float fft_input_imag[512];
				__attribute__((aligned(16))) static float fft_output[512];
				static float fft_magnitude[256];  // Only need N/2 for real signal
				
				// Sample or interpolate voltage data to FFT size
				for (int i = 0; i < fft_size; i++) {
					if (display_count >= (uint32_t)fft_size) {
						// Downsample
						uint32_t src_idx = (i * display_count) / fft_size;
						if (src_idx >= display_count) src_idx = display_count - 1;
						fft_input_real[i] = display_buffer[src_idx];
					} else {
						// Upsample with interpolation
						float src_pos = (float)i * display_count / fft_size;
						uint32_t src_idx = (uint32_t)src_pos;
						if (src_idx >= display_count - 1) {
							fft_input_real[i] = display_buffer[display_count - 1];
						} else {
							float frac = src_pos - src_idx;
							fft_input_real[i] = display_buffer[src_idx] * (1.0f - frac) + 
							                    display_buffer[src_idx + 1] * frac;
						}
					}
					fft_input_imag[i] = 0.0f;  // Real signal, imaginary part is zero
				}
				
				// Apply Hanning window to reduce spectral leakage
				for (int i = 0; i < fft_size; i++) {
					float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fft_size - 1)));
					fft_input_real[i] *= window;
				}
				
				// Perform hardware-accelerated FFT using ESP-DSP
				// dsps_fft2r_fc32 performs real FFT (faster than complex FFT)
				esp_err_t fft_ret = dsps_fft2r_init_fc32(NULL, fft_size);
				if (fft_ret == ESP_OK) {
					// Copy to interleaved format (real, imag, real, imag, ...)
					for (int i = 0; i < fft_size; i++) {
						fft_output[i * 2] = fft_input_real[i];
						fft_output[i * 2 + 1] = fft_input_imag[i];
					}
					
					// Perform FFT (hardware accelerated on ESP32-P4)
					dsps_fft2r_fc32(fft_output, fft_size);
					
					// Bit reverse for correct output order
					dsps_bit_rev_fc32(fft_output, fft_size);
					
					// Convert to magnitude
					for (int i = 0; i < fft_size / 2; i++) {
						float real = fft_output[i * 2];
						float imag = fft_output[i * 2 + 1];
						fft_magnitude[i] = sqrtf(real * real + imag * imag) / fft_size;
					}
				} else {
					// Fallback: simple DFT if hardware FFT fails
					ESP_LOGW("OSC_FFT", "Hardware FFT init failed, using software fallback");
					for (int k = 0; k < fft_size / 2; k += 4) {  // Calculate every 4th bin for speed
						float real_sum = 0.0f;
						float imag_sum = 0.0f;
						for (int t = 0; t < fft_size; t++) {
							float angle = -2.0f * M_PI * k * t / fft_size;
							real_sum += fft_input_real[t] * cosf(angle);
							imag_sum += fft_input_real[t] * sinf(angle);
						}
						fft_magnitude[k] = sqrtf(real_sum * real_sum + imag_sum * imag_sum) / fft_size;
					}
					// Interpolate skipped bins
					for (int k = 1; k < fft_size / 2; k++) {
						if (k % 4 != 0) {
							int k0 = (k / 4) * 4;
							int k1 = k0 + 4;
							if (k1 >= fft_size / 2) k1 = fft_size / 2 - 1;
							float frac = (float)(k - k0) / 4.0f;
							fft_magnitude[k] = fft_magnitude[k0] * (1.0f - frac) + fft_magnitude[k1] * frac;
						}
					}
				}
				
				// Find maximum magnitude for normalization and peak frequency
				float max_magnitude = 0.0f;
				int peak_bin = 0;
				for (int i = 1; i < fft_size / 2; i++) {  // Skip DC component (i=0)
					if (fft_magnitude[i] > max_magnitude) {
						max_magnitude = fft_magnitude[i];
						peak_bin = i;
					}
				}
				
				// Calculate signal frequency from peak bin
				signal_frequency = peak_bin * freq_resolution;
				
				if (max_magnitude < 0.001f) max_magnitude = 0.001f;  // Avoid division by zero
				
				// 计算谐波幅值和THD
				fundamental_magnitude = fft_magnitude[peak_bin];  // 基波（1次谐波）
				
				// 计算3次谐波幅值
				int harmonic_3_bin = peak_bin * 3;
				if (harmonic_3_bin < fft_size / 2) {
					// 在3次谐波频率附近搜索峰值（±2个bin）
					float max_h3 = 0.0f;
					for (int i = harmonic_3_bin - 2; i <= harmonic_3_bin + 2; i++) {
						if (i > 0 && i < fft_size / 2) {
							if (fft_magnitude[i] > max_h3) {
								max_h3 = fft_magnitude[i];
							}
						}
					}
					harmonic_3_magnitude = max_h3;
				}
				
				// 计算THD（总谐波失真）
				// THD = sqrt(sum(harmonics^2)) / fundamental
				// 考虑2-10次谐波
				float harmonics_sum_sq = 0.0f;
				for (int h = 2; h <= 10; h++) {
					int h_bin = peak_bin * h;
					if (h_bin < fft_size / 2) {
						// 在谐波频率附近搜索峰值
						float max_h = 0.0f;
						for (int i = h_bin - 2; i <= h_bin + 2; i++) {
							if (i > 0 && i < fft_size / 2) {
								if (fft_magnitude[i] > max_h) {
									max_h = fft_magnitude[i];
								}
							}
						}
						harmonics_sum_sq += max_h * max_h;
					}
				}
				
				if (fundamental_magnitude > 0.001f) {
					thd = sqrtf(harmonics_sum_sq) / fundamental_magnitude * 100.0f;  // 转换为百分比
				}
				
				// Get FFT display parameters
				float max_freq = osc_fft_freq_ranges[osc_fft_freq_range_index];  // Maximum frequency to display
				float amp_range = osc_fft_amp_ranges[osc_fft_amp_range_index];   // Amplitude range in dB
				
				// Convert FFT magnitude to chart coordinates (BAR CHART style)
				// IMPORTANT: Chart Y-axis: 0=bottom, 1000=top
				// Each point represents a vertical bar from bottom to magnitude
				for (int i = 0; i < fft_display_points; i++) {
					// Calculate frequency for this display point
					float freq = (float)i * max_freq / (float)fft_display_points;
					
					// Find corresponding FFT bin
					int fft_bin = (int)(freq / freq_resolution);
					if (fft_bin >= fft_size / 2) fft_bin = fft_size / 2 - 1;
					if (fft_bin < 0) fft_bin = 0;
					
					// Get magnitude and convert to dB
					float magnitude = fft_magnitude[fft_bin];
					float normalized = magnitude / max_magnitude;
					float db = 20.0f * log10f(normalized + 0.001f);  // dB relative to peak
					
					// Map dB range to chart height
					// 0dB (peak) -> y=chart_range (top)
					// -amp_range dB (floor) -> y=0 (bottom)
					float db_normalized = (db + amp_range) / amp_range;  // 0.0 to 1.0
					if (db_normalized < 0.0f) db_normalized = 0.0f;
					if (db_normalized > 1.0f) db_normalized = 1.0f;
					
					// Bar height: from 0 (bottom) to calculated height
					int val = (int)(db_normalized * chart_range);
					
					if (val < 0) val = 0;
					if (val > (int)chart_range) val = (int)chart_range;
					
					ser->y_points[i] = val;
				}
			} else {
				// No ADC data - draw empty spectrum at bottom
				for (int i = 0; i < fft_display_points; i++) {
					ser->y_points[i] = 0;  // Bottom of chart
				}
			}
		} else {
			// No oscilloscope core - draw empty spectrum
			for (int i = 0; i < fft_display_points; i++) {
				ser->y_points[i] = 0;
			}
		}
		
		// Update FFT measurement displays
		char buf[64];
		
		// Peak frequency (fundamental) - 保持不变，显示信号频率
		if (guider_ui.scrOscilloscope_labelFreqTitle != NULL) {
			if (signal_frequency >= 1e6f) {
				snprintf(buf, sizeof(buf), "Freq: %.2fMHz", signal_frequency / 1e6f);
			} else if (signal_frequency >= 1e3f) {
				snprintf(buf, sizeof(buf), "Freq: %.2fkHz", signal_frequency / 1e3f);
			} else {
				snprintf(buf, sizeof(buf), "Freq: %.1fHz", signal_frequency);
			}
			lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, buf);
		}
		
		// Span改为显示1次谐波幅值（基波）
		if (guider_ui.scrOscilloscope_labelVmaxTitle != NULL) {
			snprintf(buf, sizeof(buf), "H1: %.3fV", fundamental_magnitude);
			lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, buf);
		}
		
		// Range改为显示3次谐波幅值
		if (guider_ui.scrOscilloscope_labelVminTitle != NULL) {
			snprintf(buf, sizeof(buf), "H3: %.3fV", harmonic_3_magnitude);
			lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, buf);
		}
		
		// Res改为显示THD（总谐波失真）
		if (guider_ui.scrOscilloscope_labelVppTitle != NULL) {
			snprintf(buf, sizeof(buf), "THD: %.2f%%", thd);
			lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, buf);
		}
		
		// FFT size - 保持不变
		if (guider_ui.scrOscilloscope_labelVrmsTitle != NULL) {
			snprintf(buf, sizeof(buf), "FFT: 512pt");
			lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, buf);
		}
		
	} else {
		// Time domain mode - restore yellow color for waveform
		lv_chart_set_series_color(guider_ui.scrOscilloscope_chartWaveform, ser, lv_color_hex(0xFFFF00));  // Yellow
		
		// 只在运行模式下获取新的 ADC 数据
		if (!osc_running) {
			// STOP 模式：不更新数据，使用已冻结的数据（在上面已处理）
			return;
		}
		
		// 运行模式：获取真实 ADC 数据
		float display_buffer[OSC_DISPLAY_WIDTH];
		uint32_t display_count = 0;
		
		// Update oscilloscope core (check for new ADC data)
		if (g_osc_core != NULL) {
			osc_core_update(g_osc_core);
			esp_err_t ret = osc_core_get_display_waveform(g_osc_core, display_buffer, &display_count);
			
			// Debug: Log data status every 500 frames
			static uint32_t frame_counter = 0;
			frame_counter++;
			if (frame_counter % 500 == 0) {
				ESP_LOGI("OSC_CHART", "Frame %lu: ret=%s, count=%lu", 
				         frame_counter, esp_err_to_name(ret), display_count);
			}
		}
		
		// Get current voltage scale
		float volts_per_div = volt_scale_values[osc_volt_scale_index];
		float units_per_division = chart_range / display_divisions;
		float units_per_volt = units_per_division / volts_per_div;
		
		// Draw waveform from real ADC data or flat line if no data
		if (display_count > 0) {
			// Use real ADC data
			ESP_LOGI("OSC_CHART", "Drawing REAL ADC data: %lu points", display_count);
			
			// Initialize with zero (will be updated with first sample)
			float min_voltage = 0.0f;
			float max_voltage = 0.0f;
			float sum_squares = 0.0f;
			bool first_sample = true;
			
			for(int i = 0; i < num_points; i++) {
				float voltage;
				
				// Sample from voltage buffer (with interpolation if needed)
				if (display_count >= (uint32_t)num_points) {
					// Downsample: pick every Nth sample
					uint32_t src_idx = (i * display_count) / num_points;
					if (src_idx >= display_count) src_idx = display_count - 1;
					voltage = display_buffer[src_idx];
				} else {
					// Upsample: interpolate between samples
					float src_pos = (float)i * display_count / num_points;
					uint32_t src_idx = (uint32_t)src_pos;
					if (src_idx >= display_count - 1) {
						voltage = display_buffer[display_count - 1];
					} else {
						float frac = src_pos - src_idx;
						voltage = display_buffer[src_idx] * (1.0f - frac) + 
						         display_buffer[src_idx + 1] * frac;
					}
				}
				
				// Store voltage data for potential freezing (without Y offset)
				if (i < OSC_GRID_WIDTH) {
					osc_frozen_voltage_data[i] = voltage;
				}
				
				// Initialize min/max with first sample
				if (first_sample) {
					min_voltage = voltage;
					max_voltage = voltage;
					first_sample = false;
				}
				
				// Track min/max voltage for measurements
				if (voltage < min_voltage) min_voltage = voltage;
				if (voltage > max_voltage) max_voltage = voltage;
				sum_squares += voltage * voltage;
				
				// Apply Y offset
				voltage += osc_y_offset;
				
				// Convert voltage to chart Y coordinate
				// IMPORTANT: LVGL chart Y-axis: 0=bottom, 1000=top (inverted from canvas!)
				// Positive voltage -> above center (larger Y in chart coordinates)
				// Negative voltage -> below center (smaller Y in chart coordinates)
				float y_float = chart_center + (voltage * units_per_volt);
				
				// Clamp to valid chart range
				int val = (int)(y_float + 0.5f);
				if (val < 0) val = 0;
				if (val > (int)chart_range) val = (int)chart_range;
				ser->y_points[i] = val;
			}
			
			// Mark frozen data as valid
			osc_frozen_data_valid = true;
			osc_frozen_time_scale_index = osc_time_scale_index;
			osc_frozen_volt_scale_index = osc_volt_scale_index;
			
			// Update measurement displays from real data
			char buf[64];
			float vmax = max_voltage;
			float vmin = min_voltage;
			float vpp = vmax - vmin;
			float vrms = sqrtf(sum_squares / (float)num_points);
			
			// Sanity check: oscilloscope range is -50V to +50V
			// Note: In testing mode with ESP32 ADC, actual values will be 0-3.3V
			const float MAX_VOLTAGE = 55.0f;   // +50V + 5V margin
			const float MIN_VOLTAGE = -55.0f;  // -50V - 5V margin
			bool measurements_valid = (vmax <= MAX_VOLTAGE && vmin >= MIN_VOLTAGE);
			
			if (!measurements_valid) {
				ESP_LOGW("OSC_CHART", "Measurements out of range: Vmax=%.2fV, Vmin=%.2fV - showing ---", vmax, vmin);
			}
			
			// Get frequency from oscilloscope core
			float freq_hz = 0.0f;
			float dummy_vmax, dummy_vmin, dummy_vpp, dummy_vrms;
			if (g_osc_core != NULL) {
				osc_core_get_measurements(g_osc_core, &freq_hz, &dummy_vmax, &dummy_vmin, &dummy_vpp, &dummy_vrms);
			}
			
			if (guider_ui.scrOscilloscope_labelFreqTitle != NULL) {
				if (freq_hz >= 1e6f) {
					snprintf(buf, sizeof(buf), "Freq: %.2fMHz", freq_hz / 1e6f);
				} else if (freq_hz >= 1e3f) {
					snprintf(buf, sizeof(buf), "Freq: %.2fkHz", freq_hz / 1e3f);
				} else {
					snprintf(buf, sizeof(buf), "Freq: %.1fHz", freq_hz);
				}
				lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, buf);
			}
			
			if (guider_ui.scrOscilloscope_labelVmaxTitle != NULL) {
				if (measurements_valid) {
					snprintf(buf, sizeof(buf), "Vmax: %.2fV", vmax);
				} else {
					snprintf(buf, sizeof(buf), "Vmax: ---");
				}
				lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, buf);
			}
			
			if (guider_ui.scrOscilloscope_labelVminTitle != NULL) {
				if (measurements_valid) {
					snprintf(buf, sizeof(buf), "Vmin: %.2fV", vmin);
				} else {
					snprintf(buf, sizeof(buf), "Vmin: ---");
				}
				lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, buf);
			}
			
			if (guider_ui.scrOscilloscope_labelVppTitle != NULL) {
				if (measurements_valid) {
					snprintf(buf, sizeof(buf), "Vp-p: %.2fV", vpp);
				} else {
					snprintf(buf, sizeof(buf), "Vp-p: ---");
				}
				lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, buf);
			}
			
			if (guider_ui.scrOscilloscope_labelVrmsTitle != NULL) {
				if (measurements_valid) {
					snprintf(buf, sizeof(buf), "Vrms: %.2fV", vrms);
				} else {
					snprintf(buf, sizeof(buf), "Vrms: ---");
				}
				lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, buf);
			}
		} else {
			// No ADC data available - draw flat line at center
			ESP_LOGW("OSC_CHART", "NO ADC data - drawing flat line");
			
			for(int i = 0; i < num_points; i++) {
				ser->y_points[i] = (int)chart_center;
			}
		}
	}

	lv_chart_refresh(guider_ui.scrOscilloscope_chartWaveform);
}

// Screen load event handler
static void scrOscilloscope_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_SCREEN_LOADED:
	{
		// Initialize oscilloscope integration (real ADC sampling)
		esp_err_t ret = osc_integration_init();
		if (ret != ESP_OK) {
			ESP_LOGE("OSC", "Failed to initialize oscilloscope integration: %s", esp_err_to_name(ret));
		}
		
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
		ret = osc_export_init();
		if (ret != ESP_OK) {
			ESP_LOGE("OSC", "Failed to initialize export module: %s", esp_err_to_name(ret));
		}
		
		// Start ADC sampling
		osc_integration_start();

		// Initialize offset displays
		lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, "0us");
		lv_label_set_text(guider_ui.scrOscilloscope_labelYOffsetValue, "0V");

		// Initialize cursor/survey display (游标功能初始为关闭)
		lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, "OFF");

		// Initialize waveform preview mask references
		osc_preview_mask_left = guider_ui.scrOscilloscope_sliderWaveMask;
		osc_preview_mask_right = guider_ui.scrOscilloscope_sliderWaveMaskRight;
		osc_preview_trigger_line = guider_ui.scrOscilloscope_sliderWaveTrigger;
		
		// 初始状态：运行模式，隐藏遮罩
		if (osc_preview_mask_left != NULL) {
			lv_obj_add_flag(osc_preview_mask_left, LV_OBJ_FLAG_HIDDEN);
		}
		if (osc_preview_mask_right != NULL) {
			lv_obj_add_flag(osc_preview_mask_right, LV_OBJ_FLAG_HIDDEN);
		}

		// 触发电压线已移除 - 不再需要

		// Start waveform update timer (10ms = 100Hz refresh rate for smooth animation)
		if (osc_waveform_timer == NULL) {
			osc_waveform_timer = lv_timer_create(osc_waveform_update_cb, 10, NULL);
			if (osc_waveform_timer != NULL) {
				ESP_LOGI("OSC_TIMER", "✅ Waveform update timer created successfully (10ms period)");
			} else {
				ESP_LOGE("OSC_TIMER", "❌ Failed to create waveform update timer!");
			}
		} else {
			ESP_LOGI("OSC_TIMER", "⚠️ Timer already exists, reusing");
		}
		
		// Initialize hardware-accelerated drawing context
		if (osc_draw_ctx == NULL && osc_use_hw_accel) {
			// Canvas position: inside waveform container at (2, 2)
			osc_draw_ctx = osc_draw_init(guider_ui.scrOscilloscope_contWaveform, 2, 2);
			if (osc_draw_ctx != NULL) {
				ESP_LOGI("OSC", "Hardware-accelerated drawing initialized (FPS monitoring enabled)");
				// Hide the original chart widget (keep it for fallback)
				if (guider_ui.scrOscilloscope_chartWaveform != NULL) {
					lv_obj_add_flag(guider_ui.scrOscilloscope_chartWaveform, LV_OBJ_FLAG_HIDDEN);
				}
			} else {
				ESP_LOGW("OSC", "Failed to initialize hardware acceleration, falling back to chart");
				osc_use_hw_accel = false;
				// Make sure chart is visible
				if (guider_ui.scrOscilloscope_chartWaveform != NULL) {
					lv_obj_clear_flag(guider_ui.scrOscilloscope_chartWaveform, LV_OBJ_FLAG_HIDDEN);
				}
			}
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
		
		// Clear waveform preview mask references (objects are deleted by LVGL)
		osc_preview_mask_left = NULL;
		osc_preview_mask_right = NULL;
		osc_preview_trigger_line = NULL;

		// Deinitialize export module
		osc_export_deinit();
		
		// Deinitialize hardware-accelerated drawing context
		if (osc_draw_ctx != NULL) {
			osc_draw_deinit(osc_draw_ctx);
			osc_draw_ctx = NULL;
			ESP_LOGI("OSC", "Hardware-accelerated drawing deinitialized");
		}
		
		// Show the original chart widget again (if it was hidden)
		if (guider_ui.scrOscilloscope_chartWaveform != NULL) {
			lv_obj_clear_flag(guider_ui.scrOscilloscope_chartWaveform, LV_OBJ_FLAG_HIDDEN);
		}
		
		// Deinitialize oscilloscope integration (stop ADC sampling)
		osc_integration_deinit();

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
			// 恢复运行 - 启动ADC采样
			lv_label_set_text(guider_ui.scrOscilloscope_btnStartStop_label, "RUN");
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnStartStop, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			// 清除冻结数据标记，重新开始采集
			osc_frozen_data_valid = false;
			// 启动真实ADC采样
			osc_integration_start();
		} else {
			// 停止运行 - 停止ADC采样，冻结波形
			lv_label_set_text(guider_ui.scrOscilloscope_btnStartStop_label, "STOP");
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnStartStop, lv_color_hex(0xFF0000), LV_PART_MAIN|LV_STATE_DEFAULT);
			// 记录停止时的触发位置（当前X偏移）
			osc_frozen_x_offset_at_stop = osc_x_offset;
			// 停止真实ADC采样
			osc_integration_stop();
			// 冻结数据已在波形更新回调中保存
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
			// Safety check - ensure UI objects are valid
			if (!guider_ui.scrOscilloscope_btnFFT || !lv_obj_is_valid(guider_ui.scrOscilloscope_btnFFT)) return;
			
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnFFT, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);  // Bright yellow when active

			// 切换到柱状图模式
			lv_chart_set_type(guider_ui.scrOscilloscope_chartWaveform, LV_CHART_TYPE_BAR);
			
			// 增加数据点数量，让柱子更多更密集
			// 原来是688点，现在增加到256点（FFT频谱的一半，更合理）
			lv_chart_set_point_count(guider_ui.scrOscilloscope_chartWaveform, 256);
			
			// 设置柱状图样式
			lv_obj_set_style_pad_column(guider_ui.scrOscilloscope_chartWaveform, 1, LV_PART_ITEMS);  // 柱子间距1px
			
			// FFT mode - update labels to show frequency domain information
			// 横轴：频率范围 (Frequency Span)
			// 纵轴：幅度范围 (Amplitude Range in dB)
			
			// Update time scale label to show frequency span
			if (guider_ui.scrOscilloscope_labelTimeScaleValue && lv_obj_is_valid(guider_ui.scrOscilloscope_labelTimeScaleValue))
				lv_label_set_text(guider_ui.scrOscilloscope_labelTimeScaleValue, osc_fft_freq_range_labels[osc_fft_freq_range_index]);
			
			// Update voltage scale label to show amplitude range
			if (guider_ui.scrOscilloscope_labelVoltScaleValue && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVoltScaleValue))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVoltScaleValue, osc_fft_amp_range_labels[osc_fft_amp_range_index]);
			
			// Update measurement labels for FFT
			if (guider_ui.scrOscilloscope_labelFreqTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelFreqTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, "Freq: ---");
			if (guider_ui.scrOscilloscope_labelVmaxTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVmaxTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, "H1: ---");
			if (guider_ui.scrOscilloscope_labelVminTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVminTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, "H3: ---");
			if (guider_ui.scrOscilloscope_labelVppTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVppTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, "THD: ---");
			if (guider_ui.scrOscilloscope_labelVrmsTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVrmsTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, "FFT: ---");

			// Trigger immediate waveform update to show FFT
			if (osc_waveform_timer != NULL) {
				lv_timer_ready(osc_waveform_timer);
			}
		} else {
			// Safety check - ensure UI objects are valid
			if (!guider_ui.scrOscilloscope_btnFFT || !lv_obj_is_valid(guider_ui.scrOscilloscope_btnFFT)) return;
			
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_btnFFT, lv_color_hex(0xA0A000), LV_PART_MAIN|LV_STATE_DEFAULT);  // Dark yellow when inactive

			// 恢复线图模式
			lv_chart_set_type(guider_ui.scrOscilloscope_chartWaveform, LV_CHART_TYPE_LINE);
			
			// 恢复原来的数据点数量
			lv_chart_set_point_count(guider_ui.scrOscilloscope_chartWaveform, OSC_GRID_WIDTH);
			
			// Restore time domain labels
			// Update time scale label back to time
			if (guider_ui.scrOscilloscope_labelTimeScaleValue && lv_obj_is_valid(guider_ui.scrOscilloscope_labelTimeScaleValue)) {
				const char *time_str = osc_core_get_time_scale_str((osc_time_scale_t)osc_time_scale_index);
				lv_label_set_text(guider_ui.scrOscilloscope_labelTimeScaleValue, time_str);
			}
			
			// Update voltage scale label back to voltage
			if (guider_ui.scrOscilloscope_labelVoltScaleValue && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVoltScaleValue)) {
				const char *volt_str = osc_core_get_volt_scale_str((osc_volt_scale_t)osc_volt_scale_index);
				lv_label_set_text(guider_ui.scrOscilloscope_labelVoltScaleValue, volt_str);
			}
			
			// Restore normal measurement displays
			if (guider_ui.scrOscilloscope_labelFreqTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelFreqTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelFreqTitle, "Freq: ---");
			if (guider_ui.scrOscilloscope_labelVmaxTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVmaxTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVmaxTitle, "Vmax: ---");
			if (guider_ui.scrOscilloscope_labelVminTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVminTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVminTitle, "Vmin: ---");
			if (guider_ui.scrOscilloscope_labelVppTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVppTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVppTitle, "Vp-p: ---");
			if (guider_ui.scrOscilloscope_labelVrmsTitle && lv_obj_is_valid(guider_ui.scrOscilloscope_labelVrmsTitle))
				lv_label_set_text(guider_ui.scrOscilloscope_labelVrmsTitle, "Vrms: ---");

			// Trigger immediate waveform update to restore time domain
			if (osc_waveform_timer != NULL) {
				lv_timer_ready(osc_waveform_timer);
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

				/* If FFT is enabled, store FFT data with proper frequency spectrum */
				if (osc_fft_enabled) {
					static osc_waveform_data_t fft_data;
					memset(&fft_data, 0, sizeof(osc_waveform_data_t));
					fft_data.is_fft = true;
					
					/* FFT display points constant */
					const int fft_display_points = 256;  // Number of bars in FFT spectrum
					
					/* Get FFT-specific measurements from labels */
					const char *freq_text = lv_label_get_text(guider_ui.scrOscilloscope_labelFreqTitle);
					const char *h1_text = lv_label_get_text(guider_ui.scrOscilloscope_labelVmaxTitle);
					const char *h3_text = lv_label_get_text(guider_ui.scrOscilloscope_labelVminTitle);
					const char *thd_text = lv_label_get_text(guider_ui.scrOscilloscope_labelVppTitle);
					
					/* Parse FFT measurements */
					float fundamental_freq = 0.0f, h1_mag = 0.0f, h3_mag = 0.0f, thd_val = 0.0f;
					if (freq_text) sscanf(freq_text, "Freq: %f", &fundamental_freq);
					if (h1_text) sscanf(h1_text, "H1: %f", &h1_mag);
					if (h3_text) sscanf(h3_text, "H3: %f", &h3_mag);
					if (thd_text) sscanf(thd_text, "THD: %f", &thd_val);
					
					/* Store FFT parameters */
					fft_data.frequency = fundamental_freq;
					fft_data.vmax = h1_mag;  // Reuse vmax for H1 (fundamental)
					fft_data.vmin = h3_mag;  // Reuse vmin for H3
					fft_data.vpp = thd_val;  // Reuse vpp for THD
					fft_data.vrms = 512.0f;  // FFT size
					
					/* Get FFT frequency and amplitude ranges */
					const float fft_freq_ranges[] = {1e3, 5e3, 10e3, 25e3, 50e3};  // Hz
					const float fft_amp_ranges[] = {20, 40, 60, 80, 100};  // dB
					fft_data.time_scale = fft_freq_ranges[osc_fft_freq_range_index];  // Max frequency
					fft_data.volt_scale = fft_amp_ranges[osc_fft_amp_range_index];    // Amplitude range
					
					/* Copy FFT spectrum data from chart (frequency domain) */
					fft_data.num_points = (fft_display_points < OSC_MAX_DATA_POINTS) ? fft_display_points : OSC_MAX_DATA_POINTS;
					for (int i = 0; i < fft_data.num_points; i++) {
						/* Convert chart Y coordinate to dB magnitude
						 * Chart Y: 0=bottom (min dB), 100=top (0 dB)
						 * dB = (y / 100) * amp_range - amp_range
						 */
						float y_coord = (float)ser->y_points[i];
						float normalized = y_coord / 100.0f;  // 0.0 to 1.0
						fft_data.data[i] = (normalized * fft_data.volt_scale) - fft_data.volt_scale;  // dB value
					}
					
					/* Get timestamp */
					osc_export_get_timestamp(fft_data.timestamp, sizeof(fft_data.timestamp));
					
					/* Store FFT data */
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
// 真实示波器行为：
// - 时间参数增大 → 波形被压缩 → 屏幕上显示更多周期
// - 时间参数减小 → 波形被拉伸 → 屏幕上显示更少周期（放大效果）
// - 配合X-Pos偏移可以查看波形的不同部分
// 例如：1kHz信号（周期1ms）
//   - 1ms/div × 16格 = 16ms总时间 → 显示16个周期（压缩）
//   - 100us/div × 16格 = 1.6ms总时间 → 显示1.6个周期（正常）
//   - 10us/div × 16格 = 160us总时间 → 显示0.16个周期（放大）
static void scrOscilloscope_contTimeScale_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// In FFT mode, adjust frequency range instead of time scale
		if (osc_fft_enabled) {
			// Cycle through frequency ranges
			osc_fft_freq_range_index = (osc_fft_freq_range_index + 1) % 5;
			lv_label_set_text(guider_ui.scrOscilloscope_labelTimeScaleValue, osc_fft_freq_range_labels[osc_fft_freq_range_index]);
			return;
		}
		
		// Cycle through all time scales (use global time_scale_labels array)
		osc_time_scale_index = (osc_time_scale_index + 1) % TIME_SCALE_COUNT;
		lv_label_set_text(guider_ui.scrOscilloscope_labelTimeScaleValue, time_scale_labels[osc_time_scale_index]);

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
		// In FFT mode, adjust amplitude range instead of voltage scale
		if (osc_fft_enabled) {
			// Cycle through amplitude ranges
			osc_fft_amp_range_index = (osc_fft_amp_range_index + 1) % 5;
			lv_label_set_text(guider_ui.scrOscilloscope_labelVoltScaleValue, osc_fft_amp_range_labels[osc_fft_amp_range_index]);
			return;
		}
		
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
// Cursor measurement button event handler (刻度线查询功能)
// 只处理点击事件切换模式，禁止旋转操作
static void scrOscilloscope_contTrigger_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *target = lv_event_get_target(e);

	switch (code) {
	case LV_EVENT_CLICKED:
	{
		// 循环切换游标模式：关闭 → 横轴 → 纵轴 → 关闭
		osc_cursor_mode = (osc_cursor_mode + 1) % 3;
		
		// 关闭X/Y偏移模式（互斥）
		if (osc_cursor_mode != OSC_CURSOR_OFF) {
			osc_x_offset_active = false;
			osc_y_offset_active = false;
			
			// 恢复X-Pos按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contXOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			if (guider_ui.scrOscilloscope_labelXOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelXOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			
			// 恢复Y-Pos按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contYOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			if (guider_ui.scrOscilloscope_labelYOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelYOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			
			// 隐藏Y基线
			if (osc_y_baseline != NULL) {
				lv_obj_add_flag(osc_y_baseline, LV_OBJ_FLAG_HIDDEN);
			}
			if (osc_y_baseline_marker != NULL) {
				lv_obj_add_flag(osc_y_baseline_marker, LV_OBJ_FLAG_HIDDEN);
			}
		}
		
		// 删除旧的游标对象
		if (osc_cursor_line != NULL) {
			lv_obj_del(osc_cursor_line);
			osc_cursor_line = NULL;
		}
		if (osc_cursor_label != NULL) {
			lv_obj_del(osc_cursor_label);
			osc_cursor_label = NULL;
		}
		
		// 根据模式创建新的游标
		if (osc_cursor_mode == OSC_CURSOR_OFF) {
			// 关闭游标 - 恢复正常边框和背景
			lv_obj_set_style_border_color(target, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			// 恢复原始背景色（黑色）
			lv_obj_set_style_bg_color(target, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_MAIN|LV_STATE_DEFAULT);
			
			// 恢复文字颜色为白色
			if (guider_ui.scrOscilloscope_labelTriggerTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelTriggerValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
				lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, "OFF");
			}
		} else if (osc_cursor_mode == OSC_CURSOR_HORIZONTAL) {
			// 横轴游标（时间/频率）- 青色边框，白色背景
			lv_obj_set_style_border_color(target, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);  // 青色边框
			lv_obj_set_style_border_width(target, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
			// 整个背景填充白色
			lv_obj_set_style_bg_color(target, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_MAIN|LV_STATE_DEFAULT);
			
			// 文字改为黑色（在白色背景上显示）
			if (guider_ui.scrOscilloscope_labelTriggerTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerTitle, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelTriggerValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerValue, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			
			// 创建垂直游标线（虚线样式）
			osc_cursor_line = lv_line_create(guider_ui.scrOscilloscope_contWaveform);
			static lv_point_t line_points_h[2];
			osc_cursor_position = 360;  // 中心位置
			line_points_h[0].x = osc_cursor_position;
			line_points_h[0].y = 0;
			line_points_h[1].x = osc_cursor_position;
			line_points_h[1].y = 400;
			lv_line_set_points(osc_cursor_line, line_points_h, 2);
			
			// 专业示波器风格：亮青色虚线，带发光效果
			lv_obj_set_style_line_color(osc_cursor_line, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_width(osc_cursor_line, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_dash_width(osc_cursor_line, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_dash_gap(osc_cursor_line, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_width(osc_cursor_line, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_color(osc_cursor_line, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_opa(osc_cursor_line, LV_OPA_50, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_spread(osc_cursor_line, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			
			// 创建专业标签（带边框和圆角）
			osc_cursor_label = lv_label_create(guider_ui.scrOscilloscope_contWaveform);
			// 根据当前模式显示初始文本
			if (osc_fft_enabled) {
				lv_label_set_text(osc_cursor_label, "0.0Hz");
			} else {
				lv_label_set_text(osc_cursor_label, "0.0ms");
			}
			lv_obj_set_pos(osc_cursor_label, osc_cursor_position + 8, 8);
			lv_obj_set_style_text_color(osc_cursor_label, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_text_font(osc_cursor_label, &lv_font_montserrat_14, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(osc_cursor_label, lv_color_hex(0x001a1a), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_opa(osc_cursor_label, LV_OPA_90, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_color(osc_cursor_label, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(osc_cursor_label, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_opa(osc_cursor_label, LV_OPA_60, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_radius(osc_cursor_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_pad_all(osc_cursor_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_width(osc_cursor_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_color(osc_cursor_label, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_opa(osc_cursor_label, LV_OPA_30, LV_PART_MAIN|LV_STATE_DEFAULT);
			
			// 更新按钮标签
			if (guider_ui.scrOscilloscope_labelTriggerValue != NULL) {
				lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, "H-CUR");
			}
		} else if (osc_cursor_mode == OSC_CURSOR_VERTICAL) {
			// 纵轴游标（电压/幅值）- 黄色边框，白色背景
			lv_obj_set_style_border_color(target, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);  // 黄色边框
			lv_obj_set_style_border_width(target, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
			// 整个背景填充白色
			lv_obj_set_style_bg_color(target, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_MAIN|LV_STATE_DEFAULT);
			
			// 文字改为黑色（在白色背景上显示）
			if (guider_ui.scrOscilloscope_labelTriggerTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerTitle, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelTriggerValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerValue, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			
			// 创建水平游标线（虚线样式）
			osc_cursor_line = lv_line_create(guider_ui.scrOscilloscope_contWaveform);
			static lv_point_t line_points_v[2];
			osc_cursor_position = 200;  // 中心位置
			line_points_v[0].x = 0;
			line_points_v[0].y = osc_cursor_position;
			line_points_v[1].x = 720;
			line_points_v[1].y = osc_cursor_position;
			lv_line_set_points(osc_cursor_line, line_points_v, 2);
			
			// 专业示波器风格：亮黄色虚线，带发光效果
			lv_obj_set_style_line_color(osc_cursor_line, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_width(osc_cursor_line, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_dash_width(osc_cursor_line, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_line_dash_gap(osc_cursor_line, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_width(osc_cursor_line, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_color(osc_cursor_line, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_opa(osc_cursor_line, LV_OPA_50, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_spread(osc_cursor_line, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			
			// 创建专业标签（带边框和圆角）
			osc_cursor_label = lv_label_create(guider_ui.scrOscilloscope_contWaveform);
			// 根据当前模式显示初始文本
			if (osc_fft_enabled) {
				lv_label_set_text(osc_cursor_label, "0.0dB");
			} else {
				lv_label_set_text(osc_cursor_label, "0.0V");
			}
			lv_obj_set_pos(osc_cursor_label, 8, osc_cursor_position - 22);  // 标签在线上方
			lv_obj_set_style_text_color(osc_cursor_label, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_text_font(osc_cursor_label, &lv_font_montserrat_14, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(osc_cursor_label, lv_color_hex(0x1a1a00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_opa(osc_cursor_label, LV_OPA_90, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_color(osc_cursor_label, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(osc_cursor_label, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_opa(osc_cursor_label, LV_OPA_60, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_radius(osc_cursor_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_pad_all(osc_cursor_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_width(osc_cursor_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_color(osc_cursor_label, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_shadow_opa(osc_cursor_label, LV_OPA_30, LV_PART_MAIN|LV_STATE_DEFAULT);
			
			// 更新按钮标签
			if (guider_ui.scrOscilloscope_labelTriggerValue != NULL) {
				lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, "V-CUR");
			}
		}
		break;
	}
	default:
		break;
	}
}

// X-Pos (X offset) button event handler
// 水平偏移控制 - 允许查看波形的不同时间段
// 在大时间参数下，可以通过偏移查看被压缩的波形的不同部分
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
			// 关闭游标模式和Y偏移（互斥）
			osc_cursor_mode = OSC_CURSOR_OFF;
			osc_y_offset_active = false;
			
			// 删除游标对象
			if (osc_cursor_line != NULL) {
				lv_obj_del(osc_cursor_line);
				osc_cursor_line = NULL;
			}
			if (osc_cursor_label != NULL) {
				lv_obj_del(osc_cursor_label);
				osc_cursor_label = NULL;
			}
			
			// 恢复Survey按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contTrigger, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contTrigger, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contTrigger, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			if (guider_ui.scrOscilloscope_labelTriggerTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelTriggerValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
				lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, "OFF");
			}
			
			// 恢复Y-Pos按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contYOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			if (guider_ui.scrOscilloscope_labelYOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelYOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			
			// 隐藏Y基线
			if (osc_y_baseline != NULL) {
				lv_obj_add_flag(osc_y_baseline, LV_OBJ_FLAG_HIDDEN);
			}
			if (osc_y_baseline_marker != NULL) {
				lv_obj_add_flag(osc_y_baseline_marker, LV_OBJ_FLAG_HIDDEN);
			}
			
			// 恢复Y-Pos按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contYOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contYOffset, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			// 恢复Y-Pos文字颜色
			if (guider_ui.scrOscilloscope_labelYOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelYOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}

			// 激活X-Pos：白色背景+黑色文字
			lv_obj_set_style_border_color(target, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);  // 紫色边框
			lv_obj_set_style_border_width(target, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(target, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);  // 白色背景
			lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_MAIN|LV_STATE_DEFAULT);
			// 文字改为黑色
			if (guider_ui.scrOscilloscope_labelXOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetTitle, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelXOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetValue, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
		} else {
			// 恢复正常样式：黑色背景+白色文字
			lv_obj_set_style_border_color(target, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(target, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_MAIN|LV_STATE_DEFAULT);
			// 文字改为白色
			if (guider_ui.scrOscilloscope_labelXOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelXOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
		}
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
			// 关闭游标模式和X偏移（互斥）
			osc_cursor_mode = OSC_CURSOR_OFF;
			osc_x_offset_active = false;
			
			// 删除游标对象
			if (osc_cursor_line != NULL) {
				lv_obj_del(osc_cursor_line);
				osc_cursor_line = NULL;
			}
			if (osc_cursor_label != NULL) {
				lv_obj_del(osc_cursor_label);
				osc_cursor_label = NULL;
			}
			
			// 恢复Survey按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contTrigger, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contTrigger, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contTrigger, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			if (guider_ui.scrOscilloscope_labelTriggerTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelTriggerValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelTriggerValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
				lv_label_set_text(guider_ui.scrOscilloscope_labelTriggerValue, "OFF");
			}
			
			// 恢复X-Pos按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contXOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			if (guider_ui.scrOscilloscope_labelXOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelXOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			
			// 恢复X-Pos按钮样式
			lv_obj_set_style_border_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0xFF00FF), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(guider_ui.scrOscilloscope_contXOffset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(guider_ui.scrOscilloscope_contXOffset, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			// 恢复X-Pos文字颜色
			if (guider_ui.scrOscilloscope_labelXOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelXOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelXOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}

			// 激活Y-Pos：白色背景+黑色文字
			lv_obj_set_style_border_color(target, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);  // 绿色边框
			lv_obj_set_style_border_width(target, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(target, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);  // 白色背景
			lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_MAIN|LV_STATE_DEFAULT);
			// 文字改为黑色
			if (guider_ui.scrOscilloscope_labelYOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetTitle, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelYOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetValue, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			}

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
			// 恢复正常样式：黑色背景+白色文字
			lv_obj_set_style_border_color(target, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_border_width(target, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(target, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_MAIN|LV_STATE_DEFAULT);
			// 文字改为白色
			if (guider_ui.scrOscilloscope_labelYOffsetTitle != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}
			if (guider_ui.scrOscilloscope_labelYOffsetValue != NULL) {
				lv_obj_set_style_text_color(guider_ui.scrOscilloscope_labelYOffsetValue, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
			}

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
			// Chart display parameters
			const float chart_center = 500.0f;  // Y center in chart units (0-1000 range)
			const float units_per_division = 100.0f;  // 100 chart units = 1 division
			const int num_points = OSC_GRID_WIDTH;  // 688 data points
			const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};
			const char *volt_scales[] = {"10mV", "20mV", "50mV", "100mV", "200mV", "500mV", "1V", "2V", "5V", "12V"};
			const char *time_scales[] = {"1us", "10us", "20us", "50us", "100us", "200us", "500us", "1ms", "10ms", "100ms", "1s"};

			// Step 1: 分析波形的电压范围
			float min_voltage = 0.0f, max_voltage = 0.0f;
			float current_volts_per_div = volt_scale_values[osc_volt_scale_index];
			float current_units_per_volt = units_per_division / current_volts_per_div;
			bool first_point = true;

			for(int i = 0; i < num_points; i++) {
				float voltage = ((float)ser->y_points[i] - chart_center) / current_units_per_volt;
				
				if (first_point) {
					min_voltage = voltage;
					max_voltage = voltage;
					first_point = false;
				}
				
				if (voltage < min_voltage) min_voltage = voltage;
				if (voltage > max_voltage) max_voltage = voltage;
			}

			// Step 2: 计算信号的峰峰值和中心
			float vpp = max_voltage - min_voltage;
			float signal_center = (max_voltage + min_voltage) / 2.0f;

			// Step 3: 选择合适的电压档位
			// 目标：信号占据屏幕的60-80%（6-8个division）
			// 改进：使用更激进的策略，让信号更充分地显示
			int best_volt_idx = 6;  // Default to 1V
			
			if (vpp > 0.001f) {  // 有效信号
				// 计算需要的V/div：让峰峰值占据6-7个division
				float target_volts_per_div = vpp / 6.5f;
				
				// 找到最接近的标准档位（稍大一点）
				for(int i = 0; i < 10; i++) {
					if (volt_scale_values[i] >= target_volts_per_div) {
						best_volt_idx = i;
						break;
					}
				}
				
				// 如果信号太小，至少使用10mV档
				if (best_volt_idx == 6 && vpp < 0.1f) {
					best_volt_idx = 0;  // 10mV
				}
			}

			osc_volt_scale_index = best_volt_idx;

			// Step 4: 计算Y偏移，让信号居中显示
			// 信号中心应该在屏幕中心（0V）
			osc_y_offset = -signal_center;
			
			// 限制Y偏移范围
			float max_y_offset = volt_scale_values[best_volt_idx] * 3.0f;
			if (osc_y_offset > max_y_offset) osc_y_offset = max_y_offset;
			if (osc_y_offset < -max_y_offset) osc_y_offset = -max_y_offset;

			// Step 5: 分析波形频率，选择合适的时间档位
			// 改进：使用更鲁棒的频率检测方法
			int zero_crossings = 0;
			int last_crossing_pos = -1;
			int total_period_samples = 0;
			int period_count = 0;
			
			// 计算信号的平均值作为零点参考
			float avg_y = 0.0f;
			for(int i = 0; i < num_points; i++) {
				avg_y += ser->y_points[i];
			}
			avg_y /= num_points;

			// 检测过零点
			for(int i = 1; i < num_points; i++) {
				if (ser->y_points[i-1] < avg_y && ser->y_points[i] >= avg_y) {
					zero_crossings++;
					if (last_crossing_pos >= 0) {
						total_period_samples += (i - last_crossing_pos);
						period_count++;
					}
					last_crossing_pos = i;
				}
			}

			int best_time_idx = 7;  // Default to 1ms
			
			if (period_count > 0) {
				// 计算平均周期（采样点数）
				float avg_period_samples = (float)total_period_samples / (float)period_count;
				
				// 目标：显示2-3个完整周期
				// 屏幕宽度 = 18个division = 720像素 = num_points采样点
				// 理想情况：2.5个周期 → 每个周期约 720/2.5 = 288个采样点
				float cycles_on_screen = (float)num_points / avg_period_samples;
				
				// 根据当前显示的周期数调整时间档位
				if (cycles_on_screen < 1.5f) {
					// 周期太少，放大时间（减小时基）
					best_time_idx = (osc_time_scale_index > 0) ? osc_time_scale_index - 1 : 0;
				} else if (cycles_on_screen > 4.0f) {
					// 周期太多，缩小时间（增大时基）
					best_time_idx = (osc_time_scale_index < 10) ? osc_time_scale_index + 1 : 10;
				} else {
					// 当前时基合适，保持不变
					best_time_idx = osc_time_scale_index;
				}
			}

			osc_time_scale_index = best_time_idx;

			// Step 6: 更新显示
			lv_label_set_text(guider_ui.scrOscilloscope_labelTimeScaleValue, time_scales[osc_time_scale_index]);
			lv_label_set_text(guider_ui.scrOscilloscope_labelVoltScaleValue, volt_scales[osc_volt_scale_index]);

			// 更新Y偏移显示
			char offset_str[32];
			format_voltage_offset(offset_str, sizeof(offset_str), osc_y_offset, osc_volt_scale_index);
			lv_label_set_text(guider_ui.scrOscilloscope_labelYOffsetValue, offset_str);
			
			// X偏移重置为0
			osc_x_offset = 0.0f;
			lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, "0us");
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
		// Record initial touch position and current offset values
		lv_indev_t *indev = lv_indev_get_act();
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		waveform_touch_start_x = point.x;
		waveform_touch_start_y = point.y;
		
		// 记录拖动开始时的偏移量基准值
		osc_x_offset_base = osc_x_offset;
		osc_y_offset_base = osc_y_offset;
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

		// 优先处理X/Y偏移（如果激活）
		if (osc_x_offset_active && waveform_touch_start_x != 0) {
			// Horizontal swipe for X offset (time offset)
			const float time_scale_values[] = {1e-6, 10e-6, 20e-6, 50e-6, 100e-6, 200e-6, 500e-6, 1e-3, 10e-3, 100e-3, 1.0};
			float time_per_div = time_scale_values[osc_time_scale_index];

			// Convert pixel movement to time offset
			// 使用总的移动距离，而不是增量，避免累加误差
			// 灵敏度：100像素 = 1个division
			float offset_change = (delta_x / 100.0f) * time_per_div;
			
			// 限制偏移范围
			float max_offset = time_per_div * 10.0f;  // ±10个division
			float new_offset = osc_x_offset_base + offset_change;
			if (new_offset > max_offset) new_offset = max_offset;
			if (new_offset < -max_offset) new_offset = -max_offset;
			osc_x_offset = new_offset;

			// Update display with appropriate unit based on time scale
			char offset_str[32];
			format_time_offset(offset_str, sizeof(offset_str), osc_x_offset, osc_time_scale_index);
			lv_label_set_text(guider_ui.scrOscilloscope_labelXOffsetValue, offset_str);

			// If in STOP mode, trigger immediate waveform redraw
			if (!osc_running && osc_frozen_data_valid) {
				osc_waveform_update_cb(NULL);
			}
			break;
		}

		if (osc_y_offset_active && waveform_touch_start_y != 0) {
			// Vertical swipe for Y offset (voltage offset)
			const float volt_scale_values[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 12.0};
			float volts_per_div = volt_scale_values[osc_volt_scale_index];

			// Convert pixel movement to voltage offset
			// 使用总的移动距离，而不是增量，避免累加误差
			// 灵敏度：100像素 = 1个division
			// Negative delta_y because screen Y increases downward
			float offset_change = (-delta_y / 100.0f) * volts_per_div;
			
			// 限制偏移范围
			float max_offset = volts_per_div * 5.0f;  // ±5个division
			float new_offset = osc_y_offset_base + offset_change;
			if (new_offset > max_offset) new_offset = max_offset;
			if (new_offset < -max_offset) new_offset = -max_offset;
			osc_y_offset = new_offset;

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
			break;
		}

		// 如果X/Y偏移都未激活，才处理游标拖动
		if (osc_cursor_mode != OSC_CURSOR_OFF && osc_cursor_line != NULL) {
			// 转换为相对于波形容器的坐标
			lv_obj_t *waveform_cont = guider_ui.scrOscilloscope_contWaveform;
			lv_coord_t cont_x = lv_obj_get_x(waveform_cont);
			lv_coord_t cont_y = lv_obj_get_y(waveform_cont);
			int rel_x = point.x - cont_x;
			int rel_y = point.y - cont_y;
			
			if (osc_cursor_mode == OSC_CURSOR_HORIZONTAL) {
				// 横轴游标 - 更新 X 位置
				if (rel_x < 0) rel_x = 0;
				if (rel_x > 720) rel_x = 720;
				osc_cursor_position = rel_x;
				
				// 更新线的位置
				static lv_point_t h_line_points[2];
				h_line_points[0].x = osc_cursor_position;
				h_line_points[0].y = 0;
				h_line_points[1].x = osc_cursor_position;
				h_line_points[1].y = 400;
				lv_line_set_points(osc_cursor_line, h_line_points, 2);
				
				// 更新标签位置和值
				int label_x = osc_cursor_position + 8;
				// 防止标签超出右边界
				if (label_x > 640) label_x = osc_cursor_position - 70;
				lv_obj_set_pos(osc_cursor_label, label_x, 8);
				
				// 计算时间/频率值
				char buf[32];
				if (osc_fft_enabled) {
					// FFT 模式 - 显示频率
					float max_freq = osc_fft_freq_ranges[osc_fft_freq_range_index];
					float freq = (float)osc_cursor_position * max_freq / 720.0f;
					if (freq >= 1e6f) {
						snprintf(buf, sizeof(buf), "%.2fMHz", freq / 1e6f);
					} else if (freq >= 1e3f) {
						snprintf(buf, sizeof(buf), "%.2fkHz", freq / 1e3f);
					} else {
						snprintf(buf, sizeof(buf), "%.1fHz", freq);
					}
				} else {
					// 时域模式 - 显示时间
					float time_per_div = time_scale_values[osc_time_scale_index];
					float total_time = time_per_div * OSC_GRID_COLS;
					float time = (float)osc_cursor_position * total_time / 720.0f;
					if (time >= 1.0f) {
						snprintf(buf, sizeof(buf), "%.2fs", time);
					} else if (time >= 1e-3f) {
						snprintf(buf, sizeof(buf), "%.2fms", time * 1e3f);
					} else if (time >= 1e-6f) {
						snprintf(buf, sizeof(buf), "%.2fus", time * 1e6f);
					} else {
						snprintf(buf, sizeof(buf), "%.2fns", time * 1e9f);
					}
				}
				lv_label_set_text(osc_cursor_label, buf);
				
			} else if (osc_cursor_mode == OSC_CURSOR_VERTICAL) {
				// 纵轴游标 - 更新 Y 位置
				if (rel_y < 0) rel_y = 0;
				if (rel_y > 400) rel_y = 400;
				osc_cursor_position = rel_y;
				
				// 更新线的位置
				static lv_point_t v_line_points[2];
				v_line_points[0].x = 0;
				v_line_points[0].y = osc_cursor_position;
				v_line_points[1].x = 720;
				v_line_points[1].y = osc_cursor_position;
				lv_line_set_points(osc_cursor_line, v_line_points, 2);
				
				// 更新标签位置和值
				int label_y = osc_cursor_position - 22;  // 标签在线上方
				// 防止标签超出上边界
				if (label_y < 0) label_y = osc_cursor_position + 4;
				lv_obj_set_pos(osc_cursor_label, 8, label_y);
				
				// 计算电压/幅值
				char buf[32];
				if (osc_fft_enabled) {
					// FFT 模式 - 显示幅值（dB）
					float amp_range = osc_fft_amp_ranges[osc_fft_amp_range_index];
					// Chart 坐标：Y=0 在底部，Y=400 在顶部
					// 转换为 chart 坐标（0-1000 范围）
					float chart_y = (400.0f - osc_cursor_position) * 1000.0f / 400.0f;
					float db = (chart_y / 1000.0f) * amp_range - amp_range;  // -amp_range 到 0 dB
					snprintf(buf, sizeof(buf), "%.1fdB", db);
				} else {
					// 时域模式 - 显示电压
					float volts_per_div = volt_scale_values[osc_volt_scale_index];
					// Chart 坐标：Y=0 在底部，Y=400 在顶部，中心=200
					// 转换为 chart 坐标（0-1000 范围）
					float chart_y = (400.0f - osc_cursor_position) * 1000.0f / 400.0f;
					float voltage = ((chart_y - 500.0f) / 100.0f) * volts_per_div;  // 100 units = 1 division
					snprintf(buf, sizeof(buf), "%.3fV", voltage);
				}
				lv_label_set_text(osc_cursor_label, buf);
			}
			
			// 游标模式下不处理偏移调整
			break;
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
// Note: Trigger mode selector removed - using auto trigger mode only
// Note: btnPanZoom (GRID) removed - grid is always visible now
void events_init_scrOscilloscope(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->scrOscilloscope, scrOscilloscope_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnBack, scrOscilloscope_btnBack_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnAuto, scrOscilloscope_btnAuto_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnStartStop, scrOscilloscope_btnStartStop_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnFFT, scrOscilloscope_btnFFT_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_btnExport, scrOscilloscope_btnExport_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contWaveform, scrOscilloscope_contWaveform_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contTimeScale, scrOscilloscope_contTimeScale_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contVoltScale, scrOscilloscope_contVoltScale_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contXOffset, scrOscilloscope_contXOffset_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contYOffset, scrOscilloscope_contYOffset_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contTrigger, scrOscilloscope_contTrigger_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->scrOscilloscope_contCoupling, scrOscilloscope_contCoupling_event_handler, LV_EVENT_ALL, ui);
	// Trigger mode selector removed - using auto trigger mode (RISE edge) by default
}
