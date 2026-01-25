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
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

/* Static references for input textareas */
static lv_obj_t * voltage_input_ta = NULL;
static lv_obj_t * current_input_ta = NULL;
static lv_obj_t * numpad_kb = NULL;
static lv_obj_t * active_ta = NULL;

/* Numpad keyboard event callback */
static void power_numpad_kb_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * kb = lv_event_get_target(e);

	if (!kb || !lv_obj_is_valid(kb)) return;

	if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
		if (code == LV_EVENT_READY && active_ta && lv_obj_is_valid(active_ta)) {
			const char * text = lv_textarea_get_text(active_ta);
			float value = atof(text);

			if (active_ta == voltage_input_ta) {
				if (value < 0) value = 0;
				if (value > 12.0f) value = 12.0f;
				uint16_t v_val = (uint16_t)(value * 100);
				lv_slider_set_value(guider_ui.scrPowerSupply_sliderVoltage, v_val, LV_ANIM_ON);
				char buf[16];
				snprintf(buf, sizeof(buf), "%.2f V", value);
				lv_label_set_text(guider_ui.scrPowerSupply_labelVoltageSet, buf);
			} else if (active_ta == current_input_ta) {
				if (value < 0) value = 0;
				if (value > 1.0f) value = 1.0f;
				uint16_t i_val = (uint16_t)(value * 1000);
				lv_slider_set_value(guider_ui.scrPowerSupply_sliderCurrent, i_val, LV_ANIM_ON);
				char buf[16];
				snprintf(buf, sizeof(buf), "%.3f A", value);
				lv_label_set_text(guider_ui.scrPowerSupply_labelCurrentSet, buf);
			}
		}
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		active_ta = NULL;
	}
}

/* Input textarea focus/click event */
static void power_input_ta_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * ta = lv_event_get_target(e);

	if (!ta || !lv_obj_is_valid(ta)) return;
	if (!numpad_kb || !lv_obj_is_valid(numpad_kb)) return;

	if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
		active_ta = ta;
		lv_keyboard_set_textarea(numpad_kb, ta);
		lv_obj_clear_flag(numpad_kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_foreground(numpad_kb);
	}
}

/* Voltage +/- button callback */
static void voltage_btn_event_cb(lv_event_t * e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	if (!guider_ui.scrPowerSupply_sliderVoltage) return;

	int32_t dir = (int32_t)(intptr_t)lv_event_get_user_data(e);
	int32_t current_val = lv_slider_get_value(guider_ui.scrPowerSupply_sliderVoltage);
	int32_t new_val = current_val + dir * 5;  // Step 0.05V
	if (new_val < 0) new_val = 0;
	if (new_val > 1200) new_val = 1200;

	lv_slider_set_value(guider_ui.scrPowerSupply_sliderVoltage, new_val, LV_ANIM_ON);
	char buf[16];
	snprintf(buf, sizeof(buf), "%.2f V", new_val / 100.0f);
	lv_label_set_text(guider_ui.scrPowerSupply_labelVoltageSet, buf);

	if (voltage_input_ta && lv_obj_is_valid(voltage_input_ta)) {
		char input_buf[16];
		snprintf(input_buf, sizeof(input_buf), "%.2f", new_val / 100.0f);
		lv_textarea_set_text(voltage_input_ta, input_buf);
	}
}

/* Current +/- button callback */
static void current_btn_event_cb(lv_event_t * e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	if (!guider_ui.scrPowerSupply_sliderCurrent) return;

	int32_t dir = (int32_t)(intptr_t)lv_event_get_user_data(e);
	int32_t current_val = lv_slider_get_value(guider_ui.scrPowerSupply_sliderCurrent);
	int32_t new_val = current_val + dir * 50;  // Step 0.05A
	if (new_val < 0) new_val = 0;
	if (new_val > 1000) new_val = 1000;

	lv_slider_set_value(guider_ui.scrPowerSupply_sliderCurrent, new_val, LV_ANIM_ON);
	char buf[16];
	snprintf(buf, sizeof(buf), "%.3f A", new_val / 1000.0f);
	lv_label_set_text(guider_ui.scrPowerSupply_labelCurrentSet, buf);

	if (current_input_ta && lv_obj_is_valid(current_input_ta)) {
		char input_buf[16];
		snprintf(input_buf, sizeof(input_buf), "%.3f", new_val / 1000.0f);
		lv_textarea_set_text(current_input_ta, input_buf);
	}
}

/* Clear static references */
void scrPowerSupply_clear_static_refs(void)
{
	voltage_input_ta = NULL;
	current_input_ta = NULL;
	numpad_kb = NULL;
	active_ta = NULL;
}

void setup_scr_scrPowerSupply(lv_ui *ui)
{
	/* Reset all static references at the start */
	scrPowerSupply_clear_static_refs();

	//Write codes scrPowerSupply - Digital Power Supply Interface
	ui->scrPowerSupply = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrPowerSupply, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrPowerSupply, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrPowerSupply, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply, lv_color_hex(0xF3F8FE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrPowerSupply, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrPowerSupply_contBG - Top bar
	ui->scrPowerSupply_contBG = lv_obj_create(ui->scrPowerSupply);
	lv_obj_set_pos(ui->scrPowerSupply_contBG, 0, 0);
	lv_obj_set_size(ui->scrPowerSupply_contBG, 800, 70);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply_contBG, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrPowerSupply_contBG, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrPowerSupply_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_contBG, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_contBG, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrPowerSupply_contBG, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrPowerSupply_labelTitle
	ui->scrPowerSupply_labelTitle = lv_label_create(ui->scrPowerSupply);
	lv_label_set_text(ui->scrPowerSupply_labelTitle, "DC POWER SUPPLY");
	lv_label_set_long_mode(ui->scrPowerSupply_labelTitle, LV_LABEL_LONG_CLIP);
	lv_obj_set_pos(ui->scrPowerSupply_labelTitle, 150, 12);
	lv_obj_set_size(ui->scrPowerSupply_labelTitle, 500, 50);

	//Write style for scrPowerSupply_labelTitle
	lv_obj_set_style_border_width(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelTitle, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelTitle, &lv_font_ShanHaiZhongXiaYeWuYuW_45, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrPowerSupply_labelTitle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrPowerSupply_labelTitle, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrPowerSupply_labelTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrPowerSupply_btnBack
	ui->scrPowerSupply_btnBack = lv_btn_create(ui->scrPowerSupply);
	ui->scrPowerSupply_btnBack_label = lv_label_create(ui->scrPowerSupply_btnBack);
	lv_label_set_text(ui->scrPowerSupply_btnBack_label, "<");
	lv_label_set_long_mode(ui->scrPowerSupply_btnBack_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrPowerSupply_btnBack_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrPowerSupply_btnBack, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrPowerSupply_btnBack_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrPowerSupply_btnBack, 20, 12);
	lv_obj_set_size(ui->scrPowerSupply_btnBack, 50, 50);

	//Write style for scrPowerSupply_btnBack
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPowerSupply_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_btnBack, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_btnBack, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrPowerSupply_btnBack, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrPowerSupply_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrPowerSupply_btnBack, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === LEFT PANEL: Display Cards ===
	// Left panel: y=80, height=390 (bottom at y=470)
	// Right panel: y=80, height=390 (bottom at y=470)
	// Left cards: 3 cards, each 120px high, with 15px gap between
	// Total: 120*3 + 15*2 = 390px - perfect fit
	ui->scrPowerSupply_contLeft = lv_obj_create(ui->scrPowerSupply);
	lv_obj_set_pos(ui->scrPowerSupply_contLeft, 15, 80);
	lv_obj_set_size(ui->scrPowerSupply_contLeft, 370, 390);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply_contLeft, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrPowerSupply_contLeft
	lv_obj_set_style_border_width(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === VOLTAGE CARD === (y=0, height=120)
	ui->scrPowerSupply_contVoltage = lv_obj_create(ui->scrPowerSupply_contLeft);
	lv_obj_set_pos(ui->scrPowerSupply_contVoltage, 0, 0);
	lv_obj_set_size(ui->scrPowerSupply_contVoltage, 370, 120);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply_contVoltage, LV_SCROLLBAR_MODE_OFF);

	lv_obj_set_style_border_width(ui->scrPowerSupply_contVoltage, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_contVoltage, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_contVoltage, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_contVoltage, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_contVoltage, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_contVoltage, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_contVoltage, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_contVoltage, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contVoltage, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPowerSupply_contVoltage, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPowerSupply_contVoltage, 80, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrPowerSupply_contVoltage, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelVoltageTitle = lv_label_create(ui->scrPowerSupply_contVoltage);
	lv_label_set_text(ui->scrPowerSupply_labelVoltageTitle, "OUTPUT VOLTAGE");
	lv_obj_set_pos(ui->scrPowerSupply_labelVoltageTitle, 10, 5);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelVoltageTitle, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelVoltageTitle, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelVoltageValue = lv_label_create(ui->scrPowerSupply_contVoltage);
	lv_label_set_text(ui->scrPowerSupply_labelVoltageValue, "0.00");
	lv_obj_set_pos(ui->scrPowerSupply_labelVoltageValue, 10, 30);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelVoltageValue, &lv_font_Collins_66, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelVoltageValue, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelVoltageUnit = lv_label_create(ui->scrPowerSupply_contVoltage);
	lv_label_set_text(ui->scrPowerSupply_labelVoltageUnit, "V");
	lv_obj_set_pos(ui->scrPowerSupply_labelVoltageUnit, 285, 50);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelVoltageUnit, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelVoltageUnit, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);

	// === CURRENT CARD === (y=135, height=120, gap=15)
	ui->scrPowerSupply_contCurrent = lv_obj_create(ui->scrPowerSupply_contLeft);
	lv_obj_set_pos(ui->scrPowerSupply_contCurrent, 0, 135);
	lv_obj_set_size(ui->scrPowerSupply_contCurrent, 370, 120);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply_contCurrent, LV_SCROLLBAR_MODE_OFF);

	lv_obj_set_style_border_width(ui->scrPowerSupply_contCurrent, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_contCurrent, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_contCurrent, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_contCurrent, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_contCurrent, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_contCurrent, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_contCurrent, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_contCurrent, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contCurrent, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPowerSupply_contCurrent, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPowerSupply_contCurrent, 80, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrPowerSupply_contCurrent, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelCurrentTitle = lv_label_create(ui->scrPowerSupply_contCurrent);
	lv_label_set_text(ui->scrPowerSupply_labelCurrentTitle, "OUTPUT CURRENT");
	lv_obj_set_pos(ui->scrPowerSupply_labelCurrentTitle, 10, 5);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelCurrentTitle, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelCurrentTitle, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelCurrentValue = lv_label_create(ui->scrPowerSupply_contCurrent);
	lv_label_set_text(ui->scrPowerSupply_labelCurrentValue, "0.000");
	lv_obj_set_pos(ui->scrPowerSupply_labelCurrentValue, 10, 30);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelCurrentValue, &lv_font_Collins_66, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelCurrentValue, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelCurrentUnit = lv_label_create(ui->scrPowerSupply_contCurrent);
	lv_label_set_text(ui->scrPowerSupply_labelCurrentUnit, "A");
	lv_obj_set_pos(ui->scrPowerSupply_labelCurrentUnit, 285, 50);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelCurrentUnit, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelCurrentUnit, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);

	// === POWER CARD === (y=270, height=120, gap=15)
	ui->scrPowerSupply_contPower = lv_obj_create(ui->scrPowerSupply_contLeft);
	lv_obj_set_pos(ui->scrPowerSupply_contPower, 0, 270);
	lv_obj_set_size(ui->scrPowerSupply_contPower, 370, 120);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply_contPower, LV_SCROLLBAR_MODE_OFF);

	lv_obj_set_style_border_width(ui->scrPowerSupply_contPower, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_contPower, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_contPower, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_contPower, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_contPower, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_contPower, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_contPower, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_contPower, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contPower, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPowerSupply_contPower, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPowerSupply_contPower, 80, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrPowerSupply_contPower, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelPowerTitle = lv_label_create(ui->scrPowerSupply_contPower);
	lv_label_set_text(ui->scrPowerSupply_labelPowerTitle, "OUTPUT POWER");
	lv_obj_set_pos(ui->scrPowerSupply_labelPowerTitle, 10, 5);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelPowerTitle, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelPowerTitle, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelPowerValue = lv_label_create(ui->scrPowerSupply_contPower);
	lv_label_set_text(ui->scrPowerSupply_labelPowerValue, "0.000");
	lv_obj_set_pos(ui->scrPowerSupply_labelPowerValue, 10, 30);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelPowerValue, &lv_font_Collins_66, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelPowerValue, lv_color_hex(0x27ae60), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelPowerUnit = lv_label_create(ui->scrPowerSupply_contPower);
	lv_label_set_text(ui->scrPowerSupply_labelPowerUnit, "W");
	lv_obj_set_pos(ui->scrPowerSupply_labelPowerUnit, 280, 50);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelPowerUnit, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelPowerUnit, lv_color_hex(0x27ae60), LV_PART_MAIN|LV_STATE_DEFAULT);

	// === RIGHT PANEL: Controls & Chart ===
	ui->scrPowerSupply_contRight = lv_obj_create(ui->scrPowerSupply);
	lv_obj_set_pos(ui->scrPowerSupply_contRight, 395, 80);
	lv_obj_set_size(ui->scrPowerSupply_contRight, 390, 390);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply_contRight, LV_SCROLLBAR_MODE_OFF);

	lv_obj_set_style_border_width(ui->scrPowerSupply_contRight, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_contRight, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_contRight, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_contRight, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_contRight, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_contRight, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_contRight, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_contRight, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contRight, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPowerSupply_contRight, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPowerSupply_contRight, 80, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrPowerSupply_contRight, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === TOP ROW: Power Switch & Mode Status ===
	lv_obj_t * top_row = lv_obj_create(ui->scrPowerSupply_contRight);
	lv_obj_set_pos(top_row, 0, 0);
	lv_obj_set_size(top_row, 366, 50);
	lv_obj_set_scrollbar_mode(top_row, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);

	// Style - light gray theme background
	lv_obj_set_style_bg_opa(top_row, 255, LV_PART_MAIN);
	lv_obj_set_style_bg_color(top_row, lv_color_hex(0xF4F6F7), LV_PART_MAIN);
	lv_obj_set_style_radius(top_row, 12, LV_PART_MAIN);
	lv_obj_set_style_border_width(top_row, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(top_row, lv_color_hex(0xD5D8DC), LV_PART_MAIN);
	lv_obj_set_style_pad_all(top_row, 0, LV_PART_MAIN);

	// Power switch label - centered vertically with switch
	lv_obj_t * label_output = lv_label_create(top_row);
	lv_label_set_text(label_output, "OUTPUT:");
	lv_obj_set_pos(label_output, 12, 15);
	lv_obj_set_style_text_font(label_output, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_output, lv_color_hex(0x566573), LV_PART_MAIN|LV_STATE_DEFAULT);

	// Power switch - slide toggle style
	ui->scrPowerSupply_switchPower = lv_switch_create(top_row);
	lv_obj_set_pos(ui->scrPowerSupply_switchPower, 105, 10);
	lv_obj_set_size(ui->scrPowerSupply_switchPower, 50, 28);

	// OFF state style - gray background, no shadow
	lv_obj_set_style_bg_color(ui->scrPowerSupply_switchPower, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_switchPower, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_switchPower, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPowerSupply_switchPower, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_switchPower, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Indicator (knob) style - no shadow
	lv_obj_set_style_bg_color(ui->scrPowerSupply_switchPower, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_switchPower, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_switchPower, LV_RADIUS_CIRCLE, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrPowerSupply_switchPower, -3, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_switchPower, 0, LV_PART_KNOB|LV_STATE_DEFAULT);

	// ON state style - green background, no shadow
	lv_obj_set_style_bg_color(ui->scrPowerSupply_switchPower, lv_color_hex(0x27ae60), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_switchPower, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_switchPower, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	// Mode label - moved right, centered vertically
	lv_obj_t * label_mode = lv_label_create(top_row);
	lv_label_set_text(label_mode, "MODE:");
	lv_obj_set_pos(label_mode, 185, 15);
	lv_obj_set_style_text_font(label_mode, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_mode, lv_color_hex(0x566573), LV_PART_MAIN|LV_STATE_DEFAULT);

	// Mode indicator (display only, auto-switching between CC/CV) - moved right 10px to align with values below
	ui->scrPowerSupply_labelModeStatus = lv_label_create(top_row);
	lv_label_set_text(ui->scrPowerSupply_labelModeStatus, "CV");
	lv_obj_set_pos(ui->scrPowerSupply_labelModeStatus, 260, 7);
	lv_obj_set_size(ui->scrPowerSupply_labelModeStatus, 60, 36);

	// Enhanced style - gradient background with shadow
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_labelModeStatus, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_labelModeStatus, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scrPowerSupply_labelModeStatus, lv_color_hex(0x2980b9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrPowerSupply_labelModeStatus, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_labelModeStatus, 18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPowerSupply_labelModeStatus, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrPowerSupply_labelModeStatus, lv_color_hex(0x5dade2), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_labelModeStatus, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPowerSupply_labelModeStatus, lv_color_hex(0x2980b9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPowerSupply_labelModeStatus, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrPowerSupply_labelModeStatus, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Text style
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelModeStatus, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelModeStatus, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrPowerSupply_labelModeStatus, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_labelModeStatus, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_labelModeStatus, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_labelModeStatus, 10, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === VOLTAGE SETTING SECTION ===
	lv_obj_t * voltage_section = lv_obj_create(ui->scrPowerSupply_contRight);
	lv_obj_set_pos(voltage_section, 0, 55);
	lv_obj_set_size(voltage_section, 366, 85);
	lv_obj_set_style_bg_color(voltage_section, lv_color_hex(0xFDEDED), LV_PART_MAIN);
	lv_obj_set_style_radius(voltage_section, 12, LV_PART_MAIN);
	lv_obj_set_style_border_width(voltage_section, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(voltage_section, lv_color_hex(0xF5B7B1), LV_PART_MAIN);
	lv_obj_set_style_pad_all(voltage_section, 10, LV_PART_MAIN);
	lv_obj_clear_flag(voltage_section, LV_OBJ_FLAG_SCROLLABLE);

	// Voltage title
	lv_obj_t * v_title = lv_label_create(voltage_section);
	lv_label_set_text(v_title, "Voltage Set");
	lv_obj_set_pos(v_title, 0, 0);
	lv_obj_set_style_text_font(v_title, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN);
	lv_obj_set_style_text_color(v_title, lv_color_hex(0xc0392b), LV_PART_MAIN);

	// Voltage value display
	ui->scrPowerSupply_labelVoltageSet = lv_label_create(voltage_section);
	lv_label_set_text(ui->scrPowerSupply_labelVoltageSet, "5.00 V");
	lv_obj_set_pos(ui->scrPowerSupply_labelVoltageSet, 260, 0);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelVoltageSet, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelVoltageSet, lv_color_hex(0xe74c3c), LV_PART_MAIN);

	// Voltage input row with +/- buttons
	lv_obj_t * v_input_row = lv_obj_create(voltage_section);
	lv_obj_set_pos(v_input_row, 0, 25);
	lv_obj_set_size(v_input_row, 345, 40);
	lv_obj_set_style_bg_opa(v_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(v_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(v_input_row, 0, LV_PART_MAIN);
	lv_obj_clear_flag(v_input_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(v_input_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(v_input_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Voltage minus button
	lv_obj_t * v_minus_btn = lv_btn_create(v_input_row);
	lv_obj_set_size(v_minus_btn, 36, 36);
	lv_obj_set_style_bg_color(v_minus_btn, lv_color_hex(0xe74c3c), LV_PART_MAIN);
	lv_obj_set_style_radius(v_minus_btn, 8, LV_PART_MAIN);
	lv_obj_set_style_pad_all(v_minus_btn, 0, LV_PART_MAIN);
	lv_obj_set_style_shadow_width(v_minus_btn, 0, LV_PART_MAIN);
	lv_obj_t * v_minus_lbl = lv_label_create(v_minus_btn);
	lv_label_set_text(v_minus_lbl, LV_SYMBOL_MINUS);
	lv_obj_align(v_minus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(v_minus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(v_minus_btn, voltage_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

	// Voltage input textarea
	voltage_input_ta = lv_textarea_create(v_input_row);
	lv_textarea_set_one_line(voltage_input_ta, true);
	lv_textarea_set_text(voltage_input_ta, "5.00");
	lv_obj_set_size(voltage_input_ta, 80, 36);
	lv_obj_set_style_bg_color(voltage_input_ta, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_set_style_border_color(voltage_input_ta, lv_color_hex(0xe74c3c), LV_PART_MAIN);
	lv_obj_set_style_border_width(voltage_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_radius(voltage_input_ta, 8, LV_PART_MAIN);
	lv_obj_set_style_text_font(voltage_input_ta, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN);
	lv_obj_set_style_text_align(voltage_input_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_pad_left(voltage_input_ta, 4, LV_PART_MAIN);
	lv_obj_set_style_pad_right(voltage_input_ta, 4, LV_PART_MAIN);
	lv_obj_set_style_pad_top(voltage_input_ta, 6, LV_PART_MAIN);
	lv_obj_add_event_cb(voltage_input_ta, power_input_ta_event_cb, LV_EVENT_CLICKED, NULL);

	// Voltage plus button
	lv_obj_t * v_plus_btn = lv_btn_create(v_input_row);
	lv_obj_set_size(v_plus_btn, 36, 36);
	lv_obj_set_style_bg_color(v_plus_btn, lv_color_hex(0xe74c3c), LV_PART_MAIN);
	lv_obj_set_style_radius(v_plus_btn, 8, LV_PART_MAIN);
	lv_obj_set_style_pad_all(v_plus_btn, 0, LV_PART_MAIN);
	lv_obj_set_style_shadow_width(v_plus_btn, 0, LV_PART_MAIN);
	lv_obj_t * v_plus_lbl = lv_label_create(v_plus_btn);
	lv_label_set_text(v_plus_lbl, LV_SYMBOL_PLUS);
	lv_obj_align(v_plus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(v_plus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(v_plus_btn, voltage_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

	// Voltage unit label
	lv_obj_t * v_unit = lv_label_create(v_input_row);
	lv_label_set_text(v_unit, "V");
	lv_obj_set_style_text_font(v_unit, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN);
	lv_obj_set_style_text_color(v_unit, lv_color_hex(0xc0392b), LV_PART_MAIN);
	lv_obj_set_style_pad_left(v_unit, 5, LV_PART_MAIN);

	// Voltage slider
	ui->scrPowerSupply_sliderVoltage = lv_slider_create(v_input_row);
	lv_slider_set_range(ui->scrPowerSupply_sliderVoltage, 0, 1200);  // 0-12.00V
	lv_slider_set_value(ui->scrPowerSupply_sliderVoltage, 500, LV_ANIM_OFF);
	lv_obj_set_size(ui->scrPowerSupply_sliderVoltage, 140, 12);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderVoltage, lv_color_hex(0xF5B7B1), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderVoltage, 6, LV_PART_MAIN);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderVoltage, lv_color_hex(0xe74c3c), LV_PART_INDICATOR);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderVoltage, 6, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderVoltage, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
	lv_obj_set_style_border_width(ui->scrPowerSupply_sliderVoltage, 2, LV_PART_KNOB);
	lv_obj_set_style_border_color(ui->scrPowerSupply_sliderVoltage, lv_color_hex(0xe74c3c), LV_PART_KNOB);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderVoltage, LV_RADIUS_CIRCLE, LV_PART_KNOB);
	lv_obj_set_style_pad_all(ui->scrPowerSupply_sliderVoltage, 4, LV_PART_KNOB);

	// === CURRENT SETTING SECTION ===
	lv_obj_t * current_section = lv_obj_create(ui->scrPowerSupply_contRight);
	lv_obj_set_pos(current_section, 0, 145);
	lv_obj_set_size(current_section, 366, 85);
	lv_obj_set_style_bg_color(current_section, lv_color_hex(0xEBF5FB), LV_PART_MAIN);
	lv_obj_set_style_radius(current_section, 12, LV_PART_MAIN);
	lv_obj_set_style_border_width(current_section, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(current_section, lv_color_hex(0xAED6F1), LV_PART_MAIN);
	lv_obj_set_style_pad_all(current_section, 10, LV_PART_MAIN);
	lv_obj_clear_flag(current_section, LV_OBJ_FLAG_SCROLLABLE);

	// Current title
	lv_obj_t * i_title = lv_label_create(current_section);
	lv_label_set_text(i_title, "Current Limit");
	lv_obj_set_pos(i_title, 0, 0);
	lv_obj_set_style_text_font(i_title, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN);
	lv_obj_set_style_text_color(i_title, lv_color_hex(0x2980b9), LV_PART_MAIN);

	// Current value display
	ui->scrPowerSupply_labelCurrentSet = lv_label_create(current_section);
	lv_label_set_text(ui->scrPowerSupply_labelCurrentSet, "0.500 A");
	lv_obj_set_pos(ui->scrPowerSupply_labelCurrentSet, 250, 0);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelCurrentSet, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelCurrentSet, lv_color_hex(0x3498db), LV_PART_MAIN);

	// Current input row with +/- buttons
	lv_obj_t * i_input_row = lv_obj_create(current_section);
	lv_obj_set_pos(i_input_row, 0, 25);
	lv_obj_set_size(i_input_row, 345, 40);
	lv_obj_set_style_bg_opa(i_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(i_input_row, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(i_input_row, 0, LV_PART_MAIN);
	lv_obj_clear_flag(i_input_row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(i_input_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(i_input_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Current minus button
	lv_obj_t * i_minus_btn = lv_btn_create(i_input_row);
	lv_obj_set_size(i_minus_btn, 36, 36);
	lv_obj_set_style_bg_color(i_minus_btn, lv_color_hex(0x3498db), LV_PART_MAIN);
	lv_obj_set_style_radius(i_minus_btn, 8, LV_PART_MAIN);
	lv_obj_set_style_pad_all(i_minus_btn, 0, LV_PART_MAIN);
	lv_obj_set_style_shadow_width(i_minus_btn, 0, LV_PART_MAIN);
	lv_obj_t * i_minus_lbl = lv_label_create(i_minus_btn);
	lv_label_set_text(i_minus_lbl, LV_SYMBOL_MINUS);
	lv_obj_align(i_minus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(i_minus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(i_minus_btn, current_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

	// Current input textarea
	current_input_ta = lv_textarea_create(i_input_row);
	lv_textarea_set_one_line(current_input_ta, true);
	lv_textarea_set_text(current_input_ta, "0.500");
	lv_obj_set_size(current_input_ta, 80, 36);
	lv_obj_set_style_bg_color(current_input_ta, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_set_style_border_color(current_input_ta, lv_color_hex(0x3498db), LV_PART_MAIN);
	lv_obj_set_style_border_width(current_input_ta, 2, LV_PART_MAIN);
	lv_obj_set_style_radius(current_input_ta, 8, LV_PART_MAIN);
	lv_obj_set_style_text_font(current_input_ta, &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN);
	lv_obj_set_style_text_align(current_input_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_pad_left(current_input_ta, 4, LV_PART_MAIN);
	lv_obj_set_style_pad_right(current_input_ta, 4, LV_PART_MAIN);
	lv_obj_set_style_pad_top(current_input_ta, 6, LV_PART_MAIN);
	lv_obj_add_event_cb(current_input_ta, power_input_ta_event_cb, LV_EVENT_CLICKED, NULL);

	// Current plus button
	lv_obj_t * i_plus_btn = lv_btn_create(i_input_row);
	lv_obj_set_size(i_plus_btn, 36, 36);
	lv_obj_set_style_bg_color(i_plus_btn, lv_color_hex(0x3498db), LV_PART_MAIN);
	lv_obj_set_style_radius(i_plus_btn, 8, LV_PART_MAIN);
	lv_obj_set_style_pad_all(i_plus_btn, 0, LV_PART_MAIN);
	lv_obj_set_style_shadow_width(i_plus_btn, 0, LV_PART_MAIN);
	lv_obj_t * i_plus_lbl = lv_label_create(i_plus_btn);
	lv_label_set_text(i_plus_lbl, LV_SYMBOL_PLUS);
	lv_obj_align(i_plus_lbl, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_text_color(i_plus_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_add_event_cb(i_plus_btn, current_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

	// Current unit label
	lv_obj_t * i_unit = lv_label_create(i_input_row);
	lv_label_set_text(i_unit, "A");
	lv_obj_set_style_text_font(i_unit, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN);
	lv_obj_set_style_text_color(i_unit, lv_color_hex(0x2980b9), LV_PART_MAIN);
	lv_obj_set_style_pad_left(i_unit, 5, LV_PART_MAIN);

	// Current slider
	ui->scrPowerSupply_sliderCurrent = lv_slider_create(i_input_row);
	lv_slider_set_range(ui->scrPowerSupply_sliderCurrent, 0, 1000);  // 0-1.000A
	lv_slider_set_value(ui->scrPowerSupply_sliderCurrent, 500, LV_ANIM_OFF);
	lv_obj_set_size(ui->scrPowerSupply_sliderCurrent, 140, 12);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderCurrent, lv_color_hex(0xAED6F1), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderCurrent, 6, LV_PART_MAIN);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderCurrent, lv_color_hex(0x3498db), LV_PART_INDICATOR);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderCurrent, 6, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderCurrent, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
	lv_obj_set_style_border_width(ui->scrPowerSupply_sliderCurrent, 2, LV_PART_KNOB);
	lv_obj_set_style_border_color(ui->scrPowerSupply_sliderCurrent, lv_color_hex(0x3498db), LV_PART_KNOB);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderCurrent, LV_RADIUS_CIRCLE, LV_PART_KNOB);
	lv_obj_set_style_pad_all(ui->scrPowerSupply_sliderCurrent, 4, LV_PART_KNOB);

	// === POWER CHART SECTION ===
	lv_obj_t * chart_section = lv_obj_create(ui->scrPowerSupply_contRight);
	lv_obj_set_pos(chart_section, 0, 235);
	lv_obj_set_size(chart_section, 366, 130);
	lv_obj_set_style_bg_color(chart_section, lv_color_hex(0xE8F8F5), LV_PART_MAIN);
	lv_obj_set_style_radius(chart_section, 12, LV_PART_MAIN);
	lv_obj_set_style_border_width(chart_section, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(chart_section, lv_color_hex(0xA9DFBF), LV_PART_MAIN);
	lv_obj_set_style_pad_all(chart_section, 8, LV_PART_MAIN);
	lv_obj_clear_flag(chart_section, LV_OBJ_FLAG_SCROLLABLE);

	// Chart title
	lv_obj_t * chart_title = lv_label_create(chart_section);
	lv_label_set_text(chart_title, "Power Curve (W)");
	lv_obj_set_pos(chart_title, 5, 0);
	lv_obj_set_style_text_font(chart_title, &lv_font_ShanHaiZhongXiaYeWuYuW_16, LV_PART_MAIN);
	lv_obj_set_style_text_color(chart_title, lv_color_hex(0x27ae60), LV_PART_MAIN);

	// Power chart
	ui->scrPowerSupply_chartPower = lv_chart_create(chart_section);
	lv_chart_set_type(ui->scrPowerSupply_chartPower, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(ui->scrPowerSupply_chartPower, 4, 6);
	lv_chart_set_point_count(ui->scrPowerSupply_chartPower, 50);
	lv_obj_set_pos(ui->scrPowerSupply_chartPower, 25, 20);
	lv_obj_set_size(ui->scrPowerSupply_chartPower, 320, 95);
	lv_chart_set_range(ui->scrPowerSupply_chartPower, LV_CHART_AXIS_PRIMARY_Y, 0, 120);  // 0-12W
	lv_chart_set_update_mode(ui->scrPowerSupply_chartPower, LV_CHART_UPDATE_MODE_SHIFT);

	lv_obj_set_style_bg_opa(ui->scrPowerSupply_chartPower, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_chartPower, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPowerSupply_chartPower, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrPowerSupply_chartPower, lv_color_hex(0xA9DFBF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_chartPower, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui->scrPowerSupply_chartPower, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_line_color(ui->scrPowerSupply_chartPower, lv_color_hex(0x27ae60), LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_line_rounded(ui->scrPowerSupply_chartPower, true, LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_size(ui->scrPowerSupply_chartPower, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	// Y-axis labels
	lv_obj_t * label_y_max = lv_label_create(chart_section);
	lv_label_set_text(label_y_max, "12");
	lv_obj_set_pos(label_y_max, 5, 20);
	lv_obj_set_style_text_font(label_y_max, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_y_max, lv_color_hex(0x7f8c8d), LV_PART_MAIN);

	lv_obj_t * label_y_mid = lv_label_create(chart_section);
	lv_label_set_text(label_y_mid, "6");
	lv_obj_set_pos(label_y_mid, 10, 60);
	lv_obj_set_style_text_font(label_y_mid, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_y_mid, lv_color_hex(0x7f8c8d), LV_PART_MAIN);

	lv_obj_t * label_y_min = lv_label_create(chart_section);
	lv_label_set_text(label_y_min, "0");
	lv_obj_set_pos(label_y_min, 10, 100);
	lv_obj_set_style_text_font(label_y_min, &lv_font_ShanHaiZhongXiaYeWuYuW_14, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_y_min, lv_color_hex(0x7f8c8d), LV_PART_MAIN);

	// Add power series and initialize with zero data
	ui->scrPowerSupply_chartPowerSeries = lv_chart_add_series(ui->scrPowerSupply_chartPower, lv_color_hex(0x27ae60), LV_CHART_AXIS_PRIMARY_Y);

	// Initialize with zero data
	for(int i = 0; i < 50; i++) {
		lv_chart_set_next_value(ui->scrPowerSupply_chartPower, ui->scrPowerSupply_chartPowerSeries, 0);
	}

	// === NUMPAD KEYBOARD (hidden by default) ===
	numpad_kb = lv_keyboard_create(ui->scrPowerSupply);
	lv_keyboard_set_mode(numpad_kb, LV_KEYBOARD_MODE_NUMBER);
	lv_obj_set_size(numpad_kb, 400, 200);
	lv_obj_align(numpad_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_add_flag(numpad_kb, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_event_cb(numpad_kb, power_numpad_kb_event_cb, LV_EVENT_ALL, NULL);

	//Update current screen layout.
	lv_obj_update_layout(ui->scrPowerSupply);

	//Init events for screen.
	events_init_scrPowerSupply(ui);
}
