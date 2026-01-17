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
#include <stdlib.h>
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

/* Static references for input textareas */
static lv_obj_t * freq_input_ta = NULL;
static lv_obj_t * amp_input_ta = NULL;
static lv_obj_t * freq_value_label = NULL;
static lv_obj_t * amp_value_label = NULL;
static lv_obj_t * freq_slider = NULL;
static lv_obj_t * amp_slider = NULL;
static lv_obj_t * numpad_kb = NULL;
static lv_obj_t * active_ta = NULL;

/* External image references for wave type icons */
LV_IMG_DECLARE(_aassd_alpha_25x25);
LV_IMG_DECLARE(_cosbb_alpha_30x30);
LV_IMG_DECLARE(_sinaaa_alpha_25x25);

/* Update frequency input textarea from slider value */
void scrCopy_update_freq_input(uint32_t freq_hz)
{
	if (freq_input_ta && lv_obj_is_valid(freq_input_ta)) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%lu", (unsigned long)freq_hz);
		lv_textarea_set_text(freq_input_ta, buf);
	}
}

/* Update amplitude input textarea from slider value */
void scrCopy_update_amp_input(uint16_t amp_val)
{
	if (amp_input_ta && lv_obj_is_valid(amp_input_ta)) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%.2f", amp_val / 100.0f);
		lv_textarea_set_text(amp_input_ta, buf);
	}
}

/* Helper function to update waveform from current settings */
static void refresh_waveform_display(void)
{
	/* Safety check: ensure all required objects are valid */
	if (!freq_slider || !amp_slider) return;
	if (!lv_obj_is_valid(freq_slider) || !lv_obj_is_valid(amp_slider)) return;
	if (!guider_ui.scrCopy_imgScanned || !lv_obj_is_valid(guider_ui.scrCopy_imgScanned)) return;
	if (!guider_ui.scrCopy_imgColor || !lv_obj_is_valid(guider_ui.scrCopy_imgColor)) return;
	if (!guider_ui.scrCopy_sliderHue || !lv_obj_is_valid(guider_ui.scrCopy_sliderHue)) return;

	int32_t freq_val = lv_slider_get_value(freq_slider);
	uint16_t amp_val = lv_slider_get_value(amp_slider);

	uint32_t frequency;
	if (freq_val <= 100) {
		frequency = freq_val * 10;  // 0-1kHz
	} else if (freq_val <= 190) {
		frequency = 1000 + (freq_val - 100) * 100;  // 1k-10kHz
	} else if (freq_val <= 590) {
		frequency = 10000 + (freq_val - 190) * 250;  // 10k-110kHz
	} else {
		frequency = 110000 + (freq_val - 590) * 4610;  // 110k-2MHz
	}

	int wave_type = 0;
	if (lv_obj_has_state(guider_ui.scrCopy_imgColor, LV_STATE_CHECKED)) {
		wave_type = 1;
	} else if (lv_obj_has_state(guider_ui.scrCopy_sliderHue, LV_STATE_CHECKED)) {
		wave_type = 2;
	}

	update_waveform_display(guider_ui.scrCopy_imgScanned, wave_type, frequency, amp_val);
}

/* Convert frequency to slider value */
static int32_t freq_to_slider(uint32_t freq_hz)
{
	if (freq_hz <= 1000) {
		return freq_hz / 10;  // 0-1kHz: slider 0-100 (step 10Hz)
	} else if (freq_hz <= 10000) {
		return 100 + (freq_hz - 1000) / 100;  // 1k-10kHz: slider 100-190 (step 100Hz)
	} else if (freq_hz <= 110000) {
		return 190 + (freq_hz - 10000) / 250;  // 10k-110kHz: slider 190-590 (step 250Hz)
	} else {
		return 590 + (freq_hz - 110000) / 4610;  // 110k-2MHz: slider 590-1000 (step ~4.6kHz)
	}
}

/* Convert slider value to frequency */
static uint32_t slider_to_freq(int32_t slider_val)
{
	if (slider_val <= 100) {
		return slider_val * 10;  // 0-1kHz (step 10Hz)
	} else if (slider_val <= 190) {
		return 1000 + (slider_val - 100) * 100;  // 1k-10kHz (step 100Hz)
	} else if (slider_val <= 590) {
		return 10000 + (slider_val - 190) * 250;  // 10k-110kHz (step 250Hz)
	} else {
		return 110000 + (slider_val - 590) * 4610;  // 110k-2MHz
	}
}

/* Numpad keyboard event callback */
static void numpad_kb_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * kb = lv_event_get_target(e);

	/* Safety check */
	if (!kb || !lv_obj_is_valid(kb)) return;

	if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
		if (code == LV_EVENT_READY && active_ta && lv_obj_is_valid(active_ta)) {
			const char * text = lv_textarea_get_text(active_ta);
			float value = atof(text);

			if (active_ta == freq_input_ta && freq_slider && lv_obj_is_valid(freq_slider)) {
				/* Frequency input in Hz - clamp to 0-2MHz */
				uint32_t freq_hz = (uint32_t)value;
				if (freq_hz > 2000000) freq_hz = 2000000;
				int32_t slider_val = freq_to_slider(freq_hz);
				if (slider_val > 1000) slider_val = 1000;
				lv_slider_set_value(freq_slider, slider_val, LV_ANIM_ON);

				char buf[32];
				snprintf(buf, sizeof(buf), "%lu Hz", (unsigned long)freq_hz);
				if (freq_value_label && lv_obj_is_valid(freq_value_label)) {
					lv_label_set_text(freq_value_label, buf);
				}
			} else if (active_ta == amp_input_ta && amp_slider && lv_obj_is_valid(amp_slider)) {
				/* Amplitude input - clamp to 0-3V */
				if (value < 0) value = 0;
				if (value > 3.0f) value = 3.0f;
				uint16_t amp_val = (uint16_t)(value * 100);
				lv_slider_set_value(amp_slider, amp_val, LV_ANIM_ON);

				char buf[32];
				snprintf(buf, sizeof(buf), "%.2f V", value);
				if (amp_value_label && lv_obj_is_valid(amp_value_label)) {
					lv_label_set_text(amp_value_label, buf);
				}
			}
			refresh_waveform_display();
		}
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		active_ta = NULL;
	}
}

/* Input textarea focus/click event */
static void input_ta_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * ta = lv_event_get_target(e);

	/* Safety check */
	if (!ta || !lv_obj_is_valid(ta)) return;
	if (!numpad_kb || !lv_obj_is_valid(numpad_kb)) return;

	if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
		active_ta = ta;
		lv_keyboard_set_textarea(numpad_kb, ta);
		lv_obj_clear_flag(numpad_kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_foreground(numpad_kb);
	}
}

/* Get frequency step based on current value */
static uint32_t get_freq_step(uint32_t current_freq_hz)
{
	if (current_freq_hz < 1000) return 10;           // <1kHz: step 10Hz
	else if (current_freq_hz < 10000) return 100;    // <10kHz: step 100Hz
	else if (current_freq_hz < 100000) return 1000;  // <100kHz: step 1kHz
	else if (current_freq_hz < 500000) return 10000; // <500kHz: step 10kHz
	else return 50000;                                // >=500kHz: step 50kHz
}

/* Frequency +/- button callback */
static void freq_btn_event_cb(lv_event_t * e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	if (!freq_slider || !lv_obj_is_valid(freq_slider)) return;
	if (!freq_value_label || !lv_obj_is_valid(freq_value_label)) return;

	int32_t dir = (int32_t)(intptr_t)lv_event_get_user_data(e);
	int32_t slider_val = lv_slider_get_value(freq_slider);
	uint32_t current_freq = slider_to_freq(slider_val);
	uint32_t step = get_freq_step(current_freq);

	int64_t new_freq = (int64_t)current_freq + dir * (int64_t)step;
	if (new_freq < 0) new_freq = 0;
	if (new_freq > 2000000) new_freq = 2000000;

	int32_t new_slider = freq_to_slider((uint32_t)new_freq);
	lv_slider_set_value(freq_slider, new_slider, LV_ANIM_ON);

	char buf[32];
	snprintf(buf, sizeof(buf), "%lu Hz", (unsigned long)new_freq);
	lv_label_set_text(freq_value_label, buf);

	if (freq_input_ta && lv_obj_is_valid(freq_input_ta)) {
		char input_buf[16];
		snprintf(input_buf, sizeof(input_buf), "%lu", (unsigned long)new_freq);
		lv_textarea_set_text(freq_input_ta, input_buf);
	}
	refresh_waveform_display();
}

/* Amplitude +/- button callback */
static void amp_btn_event_cb(lv_event_t * e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	if (!amp_slider || !lv_obj_is_valid(amp_slider)) return;
	if (!amp_value_label || !lv_obj_is_valid(amp_value_label)) return;

	int32_t dir = (int32_t)(intptr_t)lv_event_get_user_data(e);
	int32_t current_val = lv_slider_get_value(amp_slider);

	int32_t new_val = current_val + dir * 10;  // Step 0.1V
	if (new_val < 0) new_val = 0;
	if (new_val > 300) new_val = 300;

	lv_slider_set_value(amp_slider, new_val, LV_ANIM_ON);

	char buf[32];
	snprintf(buf, sizeof(buf), "%.2f V", new_val / 100.0f);
	lv_label_set_text(amp_value_label, buf);

	if (amp_input_ta && lv_obj_is_valid(amp_input_ta)) {
		char input_buf[16];
		snprintf(input_buf, sizeof(input_buf), "%.2f", new_val / 100.0f);
		lv_textarea_set_text(amp_input_ta, input_buf);
	}
	refresh_waveform_display();
}

/* Amplitude slider event handler */
static void amplitude_slider_event_cb(lv_event_t * e)
{
	/* Safety check */
	if (!amp_slider || !lv_obj_is_valid(amp_slider)) return;
	if (!amp_value_label || !lv_obj_is_valid(amp_value_label)) return;

	if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
		uint16_t amp_val = lv_slider_get_value(amp_slider);
		char amp_str[32];
		snprintf(amp_str, sizeof(amp_str), "%.2f V", amp_val / 100.0f);
		lv_label_set_text(amp_value_label, amp_str);

		/* Update amplitude input textarea */
		scrCopy_update_amp_input(amp_val);

		refresh_waveform_display();
	}
}

/* Function to clear static references when screen is unloaded */
void scrCopy_clear_static_refs(void)
{
	freq_input_ta = NULL;
	amp_input_ta = NULL;
	freq_value_label = NULL;
	amp_value_label = NULL;
	freq_slider = NULL;
	amp_slider = NULL;
	numpad_kb = NULL;
	active_ta = NULL;
}

void setup_scr_scrCopy(lv_ui *ui)
{
	/* Reset all static references at the start */
	scrCopy_clear_static_refs();

	//Write codes scrCopy - Signal Generator Interface
	ui->scrCopy = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrCopy, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrCopy, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrCopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrCopy, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrCopy, lv_color_hex(0xF3F8FE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrCopy, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrCopy_contBG - Top bar
	ui->scrCopy_contBG = lv_obj_create(ui->scrCopy);
	lv_obj_set_pos(ui->scrCopy_contBG, 0, 0);
	lv_obj_set_size(ui->scrCopy_contBG, 800, 80);
	lv_obj_set_scrollbar_mode(ui->scrCopy_contBG, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrCopy_contBG, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrCopy_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrCopy_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrCopy_contBG, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrCopy_contBG, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrCopy_contBG, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrCopy_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrCopy_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrCopy_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrCopy_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrCopy_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrCopy_labelTitle
	ui->scrCopy_labelTitle = lv_label_create(ui->scrCopy);
	lv_label_set_text(ui->scrCopy_labelTitle, "SIGNAL GENERATOR");
	lv_label_set_long_mode(ui->scrCopy_labelTitle, LV_LABEL_LONG_CLIP);
	lv_obj_set_pos(ui->scrCopy_labelTitle, 150, 22);
	lv_obj_set_size(ui->scrCopy_labelTitle, 500, 50);

	//Write style for scrCopy_labelTitle, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrCopy_labelTitle, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrCopy_labelTitle, &lv_font_ShanHaiZhongXiaYeWuYuW_45, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrCopy_labelTitle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrCopy_labelTitle, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrCopy_labelTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrCopy_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrCopy_btnBack
	ui->scrCopy_btnBack = lv_btn_create(ui->scrCopy);
	ui->scrCopy_btnBack_label = lv_label_create(ui->scrCopy_btnBack);
	lv_label_set_text(ui->scrCopy_btnBack_label, "<");
	lv_label_set_long_mode(ui->scrCopy_btnBack_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrCopy_btnBack_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrCopy_btnBack, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrCopy_btnBack_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrCopy_btnBack, 20, 15);
	lv_obj_set_size(ui->scrCopy_btnBack, 50, 50);

	//Write style for scrCopy_btnBack, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrCopy_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrCopy_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrCopy_btnBack, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrCopy_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrCopy_btnBack, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrCopy_btnBack, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrCopy_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrCopy_btnBack, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrCopy_contWaveform - Waveform display container
	ui->scrCopy_contPanel = lv_obj_create(ui->scrCopy);
	lv_obj_set_pos(ui->scrCopy_contPanel, 10, 90);
	lv_obj_set_size(ui->scrCopy_contPanel, 540, 380);
	lv_obj_set_scrollbar_mode(ui->scrCopy_contPanel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrCopy_contPanel, LV_OBJ_FLAG_SCROLLABLE);

	//Write style for scrCopy_contPanel, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrCopy_contPanel, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrCopy_contPanel, lv_color_hex(0xe0e0e0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrCopy_contPanel, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrCopy_contPanel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrCopy_contPanel, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrCopy_contPanel, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrCopy_contPanel, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrCopy_contPanel, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrCopy_contPanel, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrCopy_contPanel, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrCopy_contPanel, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrCopy_contPanel, lv_color_hex(0xcccccc), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrCopy_contPanel, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrCopy_chart - Waveform chart centered in panel
	// Panel inner size: 540-30(padding) = 510 wide, 380-30(padding) = 350 tall
	// Center chart: (510-500)/2 = 5, (350-340)/2 = 5
	ui->scrCopy_imgScanned = lv_chart_create(ui->scrCopy_contPanel);
	lv_chart_set_type(ui->scrCopy_imgScanned, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(ui->scrCopy_imgScanned, 6, 10);
	lv_chart_set_point_count(ui->scrCopy_imgScanned, 100);
	lv_obj_center(ui->scrCopy_imgScanned);  // Center in parent
	lv_obj_set_size(ui->scrCopy_imgScanned, 500, 340);
	lv_chart_set_range(ui->scrCopy_imgScanned, LV_CHART_AXIS_PRIMARY_Y, 0, 300);  // 0-3V
	lv_chart_set_update_mode(ui->scrCopy_imgScanned, LV_CHART_UPDATE_MODE_SHIFT);

	//Chart style with axis - smooth rounded lines
	lv_obj_set_style_bg_opa(ui->scrCopy_imgScanned, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrCopy_imgScanned, lv_color_hex(0xf8f8f8), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrCopy_imgScanned, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrCopy_imgScanned, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui->scrCopy_imgScanned, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_line_color(ui->scrCopy_imgScanned, lv_color_hex(0x2196F3), LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_line_rounded(ui->scrCopy_imgScanned, true, LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_size(ui->scrCopy_imgScanned, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes scrCopy_contControls - Control panel container
	lv_obj_t * control_cont = lv_obj_create(ui->scrCopy);
	lv_obj_set_pos(control_cont, 560, 90);
	lv_obj_set_size(control_cont, 230, 380);
	lv_obj_set_scrollbar_mode(control_cont, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(control_cont, LV_OBJ_FLAG_SCROLLABLE);

	//Write style for control panel
	lv_obj_set_style_border_width(control_cont, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(control_cont, lv_color_hex(0xe0e0e0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(control_cont, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(control_cont, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(control_cont, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(control_cont, 10, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* === Wave Type Section (centered, y=10) === */
	// Wave type buttons - horizontal row with image icons
	// Button 1: Sine wave (uses _aassd_alpha_25x25)
	ui->scrCopy_btnNext = lv_btn_create(control_cont);
	lv_obj_t * img_sine = lv_img_create(ui->scrCopy_btnNext);
	lv_img_set_src(img_sine, &_aassd_alpha_25x25);
	lv_obj_align(img_sine, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrCopy_btnNext, 0, 10);
	lv_obj_set_size(ui->scrCopy_btnNext, 65, 40);
	lv_obj_set_style_bg_color(ui->scrCopy_btnNext, lv_color_hex(0x2196F3), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrCopy_btnNext, lv_color_hex(0x0D47A1), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_radius(ui->scrCopy_btnNext, 8, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrCopy_btnNext, 0, LV_PART_MAIN);
	lv_obj_add_flag(ui->scrCopy_btnNext, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_state(ui->scrCopy_btnNext, LV_STATE_CHECKED);

	// Button 2: Triangle wave (uses _cosbb_alpha_30x30)
	ui->scrCopy_imgColor = lv_btn_create(control_cont);
	lv_obj_t * img_tri = lv_img_create(ui->scrCopy_imgColor);
	lv_img_set_src(img_tri, &_cosbb_alpha_30x30);
	lv_obj_align(img_tri, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrCopy_imgColor, 72, 10);
	lv_obj_set_size(ui->scrCopy_imgColor, 65, 40);
	lv_obj_set_style_bg_color(ui->scrCopy_imgColor, lv_color_hex(0x4CAF50), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrCopy_imgColor, lv_color_hex(0x1B5E20), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_radius(ui->scrCopy_imgColor, 8, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrCopy_imgColor, 0, LV_PART_MAIN);
	lv_obj_add_flag(ui->scrCopy_imgColor, LV_OBJ_FLAG_CHECKABLE);

	// Button 3: Square wave (uses _sinaaa_alpha_25x25)
	ui->scrCopy_sliderHue = lv_btn_create(control_cont);
	lv_obj_t * img_sq = lv_img_create(ui->scrCopy_sliderHue);
	lv_img_set_src(img_sq, &_sinaaa_alpha_25x25);
	lv_obj_align(img_sq, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrCopy_sliderHue, 144, 10);
	lv_obj_set_size(ui->scrCopy_sliderHue, 65, 40);
	lv_obj_set_style_bg_color(ui->scrCopy_sliderHue, lv_color_hex(0xFF9800), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrCopy_sliderHue, lv_color_hex(0xE65100), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_radius(ui->scrCopy_sliderHue, 8, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrCopy_sliderHue, 0, LV_PART_MAIN);
	lv_obj_add_flag(ui->scrCopy_sliderHue, LV_OBJ_FLAG_CHECKABLE);

	/* === Frequency Section (centered, y=60) === */
	lv_obj_t * freq_section = lv_obj_create(control_cont);
	lv_obj_set_pos(freq_section, 0, 60);
	lv_obj_set_size(freq_section, 210, 100);
	lv_obj_set_style_bg_color(freq_section, lv_color_hex(0xE3F2FD), LV_PART_MAIN);
	lv_obj_set_style_radius(freq_section, 10, LV_PART_MAIN);
	lv_obj_set_style_border_width(freq_section, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(freq_section, lv_color_hex(0x90CAF9), LV_PART_MAIN);
	lv_obj_set_style_pad_all(freq_section, 8, LV_PART_MAIN);
	lv_obj_clear_flag(freq_section, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t * freq_title = lv_label_create(freq_section);
	lv_label_set_text(freq_title, "FREQUENCY");
	lv_obj_set_pos(freq_title, 0, 0);
	lv_obj_set_style_text_font(freq_title, &lv_font_montserratMedium_12, LV_PART_MAIN);
	lv_obj_set_style_text_color(freq_title, lv_color_hex(0x1565C0), LV_PART_MAIN);

	// Frequency value display - right aligned
	ui->scrCopy_imgBright = lv_label_create(freq_section);
	lv_label_set_text(ui->scrCopy_imgBright, "10000 Hz");
	lv_obj_set_width(ui->scrCopy_imgBright, 100);
	lv_obj_set_style_text_align(ui->scrCopy_imgBright, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
	lv_obj_set_pos(ui->scrCopy_imgBright, 90, 0);
	lv_obj_set_style_text_font(ui->scrCopy_imgBright, &lv_font_montserratMedium_12, LV_PART_MAIN);
	lv_obj_set_style_text_color(ui->scrCopy_imgBright, lv_color_hex(0x0D47A1), LV_PART_MAIN);
	freq_value_label = ui->scrCopy_imgBright;

	// Frequency slider - iOS style
	ui->scrCopy_sliderBright = lv_slider_create(freq_section);
	lv_slider_set_range(ui->scrCopy_sliderBright, 0, 1000);
	lv_slider_set_value(ui->scrCopy_sliderBright, 190, LV_ANIM_OFF);  // 190 = 10kHz
	lv_obj_set_pos(ui->scrCopy_sliderBright, 0, 20);
	lv_obj_set_size(ui->scrCopy_sliderBright, 190, 12);
	lv_obj_set_style_bg_color(ui->scrCopy_sliderBright, lv_color_hex(0xBBDEFB), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrCopy_sliderBright, 6, LV_PART_MAIN);
	lv_obj_set_style_bg_color(ui->scrCopy_sliderBright, lv_color_hex(0x1976D2), LV_PART_INDICATOR);
	lv_obj_set_style_radius(ui->scrCopy_sliderBright, 6, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(ui->scrCopy_sliderBright, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
	lv_obj_set_style_border_width(ui->scrCopy_sliderBright, 2, LV_PART_KNOB);
	lv_obj_set_style_border_color(ui->scrCopy_sliderBright, lv_color_hex(0x1976D2), LV_PART_KNOB);
	lv_obj_set_style_radius(ui->scrCopy_sliderBright, LV_RADIUS_CIRCLE, LV_PART_KNOB);
	lv_obj_set_style_pad_all(ui->scrCopy_sliderBright, 5, LV_PART_KNOB);
	freq_slider = ui->scrCopy_sliderBright;

	// Frequency input row - with +/- buttons, left-aligned with horizontal centering
	lv_obj_t * freq_input_row = lv_obj_create(freq_section);
	lv_obj_set_pos(freq_input_row, 0, 40);
	lv_obj_set_size(freq_input_row, 190, 42);
	lv_obj_set_style_bg_opa(freq_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(freq_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(freq_input_row, 0, LV_PART_MAIN);
	lv_obj_clear_flag(freq_input_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(freq_input_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(freq_input_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Frequency minus button
	lv_obj_t * freq_minus_btn = lv_btn_create(freq_input_row);
	lv_obj_set_size(freq_minus_btn, 32, 32);
	lv_obj_set_style_bg_color(freq_minus_btn, lv_color_hex(0x1976D2), LV_PART_MAIN);
	lv_obj_set_style_radius(freq_minus_btn, 6, LV_PART_MAIN);
	lv_obj_set_style_pad_all(freq_minus_btn, 0, LV_PART_MAIN);
	lv_obj_t * freq_minus_lbl = lv_label_create(freq_minus_btn);
	lv_label_set_text(freq_minus_lbl, LV_SYMBOL_MINUS);
	lv_obj_align(freq_minus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(freq_minus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(freq_minus_btn, freq_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

	freq_input_ta = lv_textarea_create(freq_input_row);
	lv_textarea_set_one_line(freq_input_ta, true);
	lv_textarea_set_text(freq_input_ta, "10000");
	lv_obj_set_size(freq_input_ta, 70, 32);
	lv_obj_set_style_bg_color(freq_input_ta, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_set_style_border_color(freq_input_ta, lv_color_hex(0x1976D2), LV_PART_MAIN);
	lv_obj_set_style_border_width(freq_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_radius(freq_input_ta, 6, LV_PART_MAIN);
	lv_obj_set_style_text_font(freq_input_ta, &lv_font_montserratMedium_16, LV_PART_MAIN);
	lv_obj_set_style_text_align(freq_input_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_pad_left(freq_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_pad_right(freq_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_pad_top(freq_input_ta, 4, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(freq_input_ta, 4, LV_PART_MAIN);

	// Frequency plus button
	lv_obj_t * freq_plus_btn = lv_btn_create(freq_input_row);
	lv_obj_set_size(freq_plus_btn, 32, 32);
	lv_obj_set_style_bg_color(freq_plus_btn, lv_color_hex(0x1976D2), LV_PART_MAIN);
	lv_obj_set_style_radius(freq_plus_btn, 6, LV_PART_MAIN);
	lv_obj_set_style_pad_all(freq_plus_btn, 0, LV_PART_MAIN);
	lv_obj_t * freq_plus_lbl = lv_label_create(freq_plus_btn);
	lv_label_set_text(freq_plus_lbl, LV_SYMBOL_PLUS);
	lv_obj_align(freq_plus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(freq_plus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(freq_plus_btn, freq_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

	lv_obj_t * freq_unit = lv_label_create(freq_input_row);
	lv_label_set_text(freq_unit, "Hz");
	lv_obj_set_style_text_font(freq_unit, &lv_font_montserratMedium_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(freq_unit, lv_color_hex(0x1565C0), LV_PART_MAIN);
	lv_obj_set_style_pad_left(freq_unit, 0, LV_PART_MAIN);

	/* === Amplitude Section (centered, y=170) === */
	lv_obj_t * amp_section = lv_obj_create(control_cont);
	lv_obj_set_pos(amp_section, 0, 170);
	lv_obj_set_size(amp_section, 210, 100);
	lv_obj_set_style_bg_color(amp_section, lv_color_hex(0xE8F5E9), LV_PART_MAIN);
	lv_obj_set_style_radius(amp_section, 10, LV_PART_MAIN);
	lv_obj_set_style_border_width(amp_section, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(amp_section, lv_color_hex(0xA5D6A7), LV_PART_MAIN);
	lv_obj_set_style_pad_all(amp_section, 8, LV_PART_MAIN);
	lv_obj_clear_flag(amp_section, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t * amp_title = lv_label_create(amp_section);
	lv_label_set_text(amp_title, "AMPLITUDE");
	lv_obj_set_pos(amp_title, 0, 0);
	lv_obj_set_style_text_font(amp_title, &lv_font_montserratMedium_12, LV_PART_MAIN);
	lv_obj_set_style_text_color(amp_title, lv_color_hex(0x2E7D32), LV_PART_MAIN);

	// Amplitude value display - right aligned
	lv_obj_t * amp_val_label = lv_label_create(amp_section);
	lv_label_set_text(amp_val_label, "1.50 V");
	lv_obj_set_width(amp_val_label, 100);
	lv_obj_set_style_text_align(amp_val_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
	lv_obj_set_pos(amp_val_label, 90, 0);
	lv_obj_set_style_text_font(amp_val_label, &lv_font_montserratMedium_12, LV_PART_MAIN);
	lv_obj_set_style_text_color(amp_val_label, lv_color_hex(0x1B5E20), LV_PART_MAIN);
	amp_value_label = amp_val_label;

	// Amplitude slider - iOS style
	lv_obj_t * slider_amp_local = lv_slider_create(amp_section);
	lv_slider_set_range(slider_amp_local, 0, 300);
	lv_slider_set_value(slider_amp_local, 150, LV_ANIM_OFF);
	lv_obj_set_pos(slider_amp_local, 0, 20);
	lv_obj_set_size(slider_amp_local, 190, 12);
	lv_obj_set_style_bg_color(slider_amp_local, lv_color_hex(0xC8E6C9), LV_PART_MAIN);
	lv_obj_set_style_radius(slider_amp_local, 6, LV_PART_MAIN);
	lv_obj_set_style_bg_color(slider_amp_local, lv_color_hex(0x388E3C), LV_PART_INDICATOR);
	lv_obj_set_style_radius(slider_amp_local, 6, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(slider_amp_local, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
	lv_obj_set_style_border_width(slider_amp_local, 2, LV_PART_KNOB);
	lv_obj_set_style_border_color(slider_amp_local, lv_color_hex(0x388E3C), LV_PART_KNOB);
	lv_obj_set_style_radius(slider_amp_local, LV_RADIUS_CIRCLE, LV_PART_KNOB);
	lv_obj_set_style_pad_all(slider_amp_local, 5, LV_PART_KNOB);
	amp_slider = slider_amp_local;

	// Amplitude input row - with +/- buttons, left-aligned with horizontal centering
	lv_obj_t * amp_input_row = lv_obj_create(amp_section);
	lv_obj_set_pos(amp_input_row, 0, 40);
	lv_obj_set_size(amp_input_row, 190, 42);
	lv_obj_set_style_bg_opa(amp_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(amp_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(amp_input_row, 0, LV_PART_MAIN);
	lv_obj_clear_flag(amp_input_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(amp_input_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(amp_input_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Amplitude minus button
	lv_obj_t * amp_minus_btn = lv_btn_create(amp_input_row);
	lv_obj_set_size(amp_minus_btn, 32, 32);
	lv_obj_set_style_bg_color(amp_minus_btn, lv_color_hex(0x388E3C), LV_PART_MAIN);
	lv_obj_set_style_radius(amp_minus_btn, 6, LV_PART_MAIN);
	lv_obj_set_style_pad_all(amp_minus_btn, 0, LV_PART_MAIN);
	lv_obj_t * amp_minus_lbl = lv_label_create(amp_minus_btn);
	lv_label_set_text(amp_minus_lbl, LV_SYMBOL_MINUS);
	lv_obj_align(amp_minus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(amp_minus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(amp_minus_btn, amp_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

	amp_input_ta = lv_textarea_create(amp_input_row);
	lv_textarea_set_one_line(amp_input_ta, true);
	lv_textarea_set_text(amp_input_ta, "1.50");
	lv_obj_set_size(amp_input_ta, 70, 32);
	lv_obj_set_style_bg_color(amp_input_ta, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_set_style_border_color(amp_input_ta, lv_color_hex(0x388E3C), LV_PART_MAIN);
	lv_obj_set_style_border_width(amp_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_radius(amp_input_ta, 6, LV_PART_MAIN);
	lv_obj_set_style_text_font(amp_input_ta, &lv_font_montserratMedium_16, LV_PART_MAIN);
	lv_obj_set_style_text_align(amp_input_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_pad_left(amp_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_pad_right(amp_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_pad_top(amp_input_ta, 4, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(amp_input_ta, 4, LV_PART_MAIN);

	// Amplitude plus button
	lv_obj_t * amp_plus_btn = lv_btn_create(amp_input_row);
	lv_obj_set_size(amp_plus_btn, 32, 32);
	lv_obj_set_style_bg_color(amp_plus_btn, lv_color_hex(0x388E3C), LV_PART_MAIN);
	lv_obj_set_style_radius(amp_plus_btn, 6, LV_PART_MAIN);
	lv_obj_set_style_pad_all(amp_plus_btn, 0, LV_PART_MAIN);
	lv_obj_t * amp_plus_lbl = lv_label_create(amp_plus_btn);
	lv_label_set_text(amp_plus_lbl, LV_SYMBOL_PLUS);
	lv_obj_align(amp_plus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(amp_plus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(amp_plus_btn, amp_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

	lv_obj_t * amp_unit = lv_label_create(amp_input_row);
	lv_label_set_text(amp_unit, "V");
	lv_obj_set_style_text_font(amp_unit, &lv_font_montserratMedium_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(amp_unit, lv_color_hex(0x2E7D32), LV_PART_MAIN);
	lv_obj_set_style_pad_left(amp_unit, 3, LV_PART_MAIN);

	/* === Output Status Section (centered, y=280) === */
	lv_obj_t * status_section = lv_obj_create(control_cont);
	lv_obj_set_pos(status_section, 0, 280);
	lv_obj_set_size(status_section, 210, 70);
	lv_obj_set_style_bg_color(status_section, lv_color_hex(0xFFF3E0), LV_PART_MAIN);
	lv_obj_set_style_radius(status_section, 10, LV_PART_MAIN);
	lv_obj_set_style_border_width(status_section, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(status_section, lv_color_hex(0xFFCC80), LV_PART_MAIN);
	lv_obj_set_style_pad_all(status_section, 8, LV_PART_MAIN);
	lv_obj_clear_flag(status_section, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t * status_title = lv_label_create(status_section);
	lv_label_set_text(status_title, "OUTPUT");
	lv_obj_set_pos(status_title, 0, 0);
	lv_obj_set_style_text_font(status_title, &lv_font_montserratMedium_12, LV_PART_MAIN);
	lv_obj_set_style_text_color(status_title, lv_color_hex(0xE65100), LV_PART_MAIN);

	lv_obj_t * status_indicator = lv_label_create(status_section);
	lv_label_set_text(status_indicator, LV_SYMBOL_OK " ACTIVE");
	lv_obj_align(status_indicator, LV_ALIGN_CENTER, 0, 5);  // Center vertically and horizontally with slight offset
	lv_obj_set_style_text_font(status_indicator, &lv_font_montserratMedium_20, LV_PART_MAIN);
	lv_obj_set_style_text_color(status_indicator, lv_color_hex(0x2E7D32), LV_PART_MAIN);

	/* Create number keyboard (hidden initially) */
	numpad_kb = lv_keyboard_create(ui->scrCopy);
	lv_keyboard_set_mode(numpad_kb, LV_KEYBOARD_MODE_NUMBER);
	lv_obj_set_size(numpad_kb, 400, 180);
	lv_obj_align(numpad_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_add_flag(numpad_kb, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_event_cb(numpad_kb, numpad_kb_event_cb, LV_EVENT_ALL, NULL);

	/* Add event callbacks for textareas - both FOCUSED and CLICKED to handle repeat clicks */
	lv_obj_add_event_cb(freq_input_ta, input_ta_event_cb, LV_EVENT_FOCUSED, NULL);
	lv_obj_add_event_cb(freq_input_ta, input_ta_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(amp_input_ta, input_ta_event_cb, LV_EVENT_FOCUSED, NULL);
	lv_obj_add_event_cb(amp_input_ta, input_ta_event_cb, LV_EVENT_CLICKED, NULL);

	//Store references for event handling
	lv_obj_set_user_data(ui->scrCopy_sliderBright, (void*)ui->scrCopy_imgBright);
	lv_obj_set_user_data(slider_amp_local, (void*)amp_val_label);

	//Set amplitude slider reference for safe access in event handlers
	scrCopy_set_amplitude_slider(slider_amp_local);

	//Add amplitude slider event callback
	lv_obj_add_event_cb(slider_amp_local, amplitude_slider_event_cb, LV_EVENT_ALL, NULL);

	//The custom code of scrCopy.
	init_signal_generator(ui);

	//Update current screen layout.
	lv_obj_update_layout(ui->scrCopy);

	//Init events for screen.
	events_init_scrCopy(ui);
}
