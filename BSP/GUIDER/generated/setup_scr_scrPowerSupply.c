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

void setup_scr_scrPowerSupply(lv_ui *ui)
{
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

	//Write style for scrPowerSupply_labelTitle, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelTitle, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelTitle, &lv_font_ShanHaiZhongXiaYeWuYuW_45, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrPowerSupply_labelTitle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrPowerSupply_labelTitle, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrPowerSupply_labelTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
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

	//Write style for scrPowerSupply_btnBack, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPowerSupply_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_btnBack, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_btnBack, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrPowerSupply_btnBack, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrPowerSupply_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrPowerSupply_btnBack, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === LEFT PANEL: Display Cards ===
	//Write codes scrPowerSupply_contLeft - Left panel for display cards
	ui->scrPowerSupply_contLeft = lv_obj_create(ui->scrPowerSupply);
	lv_obj_set_pos(ui->scrPowerSupply_contLeft, 15, 80);
	lv_obj_set_size(ui->scrPowerSupply_contLeft, 370, 390);
	lv_obj_set_scrollbar_mode(ui->scrPowerSupply_contLeft, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrPowerSupply_contLeft
	lv_obj_set_style_border_width(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === VOLTAGE CARD ===
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

	// === CURRENT CARD ===
	ui->scrPowerSupply_contCurrent = lv_obj_create(ui->scrPowerSupply_contLeft);
	lv_obj_set_pos(ui->scrPowerSupply_contCurrent, 0, 130);
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
	lv_label_set_text(ui->scrPowerSupply_labelCurrentValue, "0.00");
	lv_obj_set_pos(ui->scrPowerSupply_labelCurrentValue, 10, 30);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelCurrentValue, &lv_font_Collins_66, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelCurrentValue, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelCurrentUnit = lv_label_create(ui->scrPowerSupply_contCurrent);
	lv_label_set_text(ui->scrPowerSupply_labelCurrentUnit, "A");
	lv_obj_set_pos(ui->scrPowerSupply_labelCurrentUnit, 285, 50);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelCurrentUnit, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelCurrentUnit, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);

	// === POWER CARD ===
	ui->scrPowerSupply_contPower = lv_obj_create(ui->scrPowerSupply_contLeft);
	lv_obj_set_pos(ui->scrPowerSupply_contPower, 0, 260);
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
	lv_label_set_text(ui->scrPowerSupply_labelPowerValue, "0.0000");
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
	lv_obj_set_style_pad_top(ui->scrPowerSupply_contRight, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrPowerSupply_contRight, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrPowerSupply_contRight, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrPowerSupply_contRight, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_contRight, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrPowerSupply_contRight, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrPowerSupply_contRight, 80, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrPowerSupply_contRight, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === POWER SWITCH ===
	lv_obj_t * label_output = lv_label_create(ui->scrPowerSupply_contRight);
	lv_label_set_text(label_output, "Off/On");
	lv_obj_set_pos(label_output, 10, 10);
	lv_obj_set_style_text_font(label_output, &lv_font_montserratMedium_19, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_output, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_switchPower = lv_switch_create(ui->scrPowerSupply_contRight);
	lv_obj_set_pos(ui->scrPowerSupply_switchPower, 260, 0);
	lv_obj_set_size(ui->scrPowerSupply_switchPower, 90, 40);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_switchPower, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_switchPower, lv_color_hex(0x27ae60), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_switchPower, lv_color_hex(0xecf0f1), LV_PART_KNOB|LV_STATE_DEFAULT);

	// === MODE SWITCH (CC/CV) ===
	lv_obj_t * label_mode = lv_label_create(ui->scrPowerSupply_contRight);
	lv_label_set_text(label_mode, "MODE");
	lv_obj_set_pos(label_mode, 10, 55);
	lv_obj_set_style_text_font(label_mode, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_mode, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);

	// Mode toggle button (reusing labelModeStatus as button)
	ui->scrPowerSupply_labelModeStatus = lv_btn_create(ui->scrPowerSupply_contRight);
	lv_obj_t * mode_label = lv_label_create(ui->scrPowerSupply_labelModeStatus);
	lv_label_set_text(mode_label, "CC");
	lv_obj_center(mode_label);
	lv_obj_set_pos(ui->scrPowerSupply_labelModeStatus, 248, 50);
	lv_obj_set_size(ui->scrPowerSupply_labelModeStatus, 105, 35);
	lv_obj_set_style_bg_opa(ui->scrPowerSupply_labelModeStatus, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_labelModeStatus, lv_color_hex(0xf39c12), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_labelModeStatus, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelModeStatus, &lv_font_montserratMedium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelModeStatus, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrPowerSupply_labelModeStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPowerSupply_labelModeStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->scrPowerSupply_labelModeStatus, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_state(ui->scrPowerSupply_labelModeStatus, LV_STATE_CHECKED);  // Default CC mode

	// === SETTINGS CARD ===
	lv_obj_t * settings_card = lv_obj_create(ui->scrPowerSupply_contRight);
	lv_obj_set_pos(settings_card, 8, 95);  // Centered: (390 - 360) / 2 = 15
	lv_obj_set_size(settings_card, 348, 90);
	lv_obj_set_scrollbar_mode(settings_card, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(settings_card, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling
	lv_obj_set_style_border_width(settings_card, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(settings_card, lv_color_hex(0xdfe6e9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(settings_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(settings_card, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(settings_card, lv_color_hex(0xf8f9fa), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(settings_card, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(settings_card, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Voltage Set
	lv_obj_t * label_v_set = lv_label_create(settings_card);
	lv_label_set_text(label_v_set, "Voltage Set");
	lv_obj_set_pos(label_v_set, 3, -2);
	lv_obj_set_style_text_font(label_v_set, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_v_set, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelVoltageSet = lv_label_create(settings_card);
	lv_label_set_text(ui->scrPowerSupply_labelVoltageSet, "5.00 V");
	lv_obj_set_pos(ui->scrPowerSupply_labelVoltageSet, 260, -2);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelVoltageSet, &lv_font_montserratMedium_19, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelVoltageSet, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_sliderVoltage = lv_slider_create(settings_card);
	lv_slider_set_range(ui->scrPowerSupply_sliderVoltage, 0, 1200);  // 0-12.00V in 0.01V steps
	lv_slider_set_value(ui->scrPowerSupply_sliderVoltage, 500, LV_ANIM_OFF);
	lv_obj_set_pos(ui->scrPowerSupply_sliderVoltage, -5, 20);
	lv_obj_set_size(ui->scrPowerSupply_sliderVoltage, 330, 10);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderVoltage, lv_color_hex(0xdfe6e9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderVoltage, lv_color_hex(0xe74c3c), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderVoltage, lv_color_hex(0xc0392b), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderVoltage, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderVoltage, 50, LV_PART_KNOB|LV_STATE_DEFAULT);

	// Current Limit
	lv_obj_t * label_i_set = lv_label_create(settings_card);
	lv_label_set_text(label_i_set, "Current Limit");
	lv_obj_set_pos(label_i_set, 3, 40);
	lv_obj_set_style_text_font(label_i_set, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_i_set, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_labelCurrentSet = lv_label_create(settings_card);
	lv_label_set_text(ui->scrPowerSupply_labelCurrentSet, "0.50 A");
	lv_obj_set_pos(ui->scrPowerSupply_labelCurrentSet, 260, 40);
	lv_obj_set_style_text_font(ui->scrPowerSupply_labelCurrentSet, &lv_font_montserratMedium_19, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrPowerSupply_labelCurrentSet, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_sliderCurrent = lv_slider_create(settings_card);
	lv_slider_set_range(ui->scrPowerSupply_sliderCurrent, 0, 1000);  // 0-1.00A (display 2 decimals)
	lv_slider_set_value(ui->scrPowerSupply_sliderCurrent, 500, LV_ANIM_OFF);
	lv_obj_set_pos(ui->scrPowerSupply_sliderCurrent, -5, 60);
	lv_obj_set_size(ui->scrPowerSupply_sliderCurrent, 330, 10);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderCurrent, lv_color_hex(0xdfe6e9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderCurrent, lv_color_hex(0x3498db), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_sliderCurrent, lv_color_hex(0x2980b9), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderCurrent, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrPowerSupply_sliderCurrent, 50, LV_PART_KNOB|LV_STATE_DEFAULT);

	// === POWER CHART ===
	lv_obj_t * label_chart = lv_label_create(ui->scrPowerSupply_contRight);
	lv_label_set_text(label_chart, "Power Curve (W)");
	lv_obj_set_pos(label_chart, 10, 195);
	lv_obj_set_style_text_font(label_chart, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_chart, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrPowerSupply_chartPower = lv_chart_create(ui->scrPowerSupply_contRight);
	lv_chart_set_type(ui->scrPowerSupply_chartPower, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(ui->scrPowerSupply_chartPower, 5, 8);
	lv_chart_set_point_count(ui->scrPowerSupply_chartPower, 50);
	lv_obj_set_pos(ui->scrPowerSupply_chartPower, 30, 220);
	lv_obj_set_size(ui->scrPowerSupply_chartPower, 320, 145);
	lv_chart_set_range(ui->scrPowerSupply_chartPower, LV_CHART_AXIS_PRIMARY_Y, 0, 120);  // 0-12W (scaled by 10)
	lv_chart_set_update_mode(ui->scrPowerSupply_chartPower, LV_CHART_UPDATE_MODE_SHIFT);

	lv_obj_set_style_bg_opa(ui->scrPowerSupply_chartPower, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrPowerSupply_chartPower, lv_color_hex(0xfafafa), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrPowerSupply_chartPower, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrPowerSupply_chartPower, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui->scrPowerSupply_chartPower, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_line_color(ui->scrPowerSupply_chartPower, lv_color_hex(0x27ae60), LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_line_rounded(ui->scrPowerSupply_chartPower, true, LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_size(ui->scrPowerSupply_chartPower, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	
	// Y-axis labels (Power W)
	lv_obj_t * label_y_max = lv_label_create(ui->scrPowerSupply_contRight);
	lv_label_set_text(label_y_max, "12");
	lv_obj_set_pos(label_y_max, 8, 220);
	lv_obj_set_style_text_font(label_y_max, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_y_max, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);
	
	lv_obj_t * label_y_mid = lv_label_create(ui->scrPowerSupply_contRight);
	lv_label_set_text(label_y_mid, "6");
	lv_obj_set_pos(label_y_mid, 13, 285);
	lv_obj_set_style_text_font(label_y_mid, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_y_mid, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);
	
	lv_obj_t * label_y_min = lv_label_create(ui->scrPowerSupply_contRight);
	lv_label_set_text(label_y_min, "0");
	lv_obj_set_pos(label_y_min, 13, 350);
	lv_obj_set_style_text_font(label_y_min, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label_y_min, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);
	

	// Add power series and initialize with zero data
	ui->scrPowerSupply_chartPowerSeries = lv_chart_add_series(ui->scrPowerSupply_chartPower, lv_color_hex(0x27ae60), LV_CHART_AXIS_PRIMARY_Y);
	
	// Initialize with zero data
	for(int i = 0; i < 50; i++) {
		lv_chart_set_next_value(ui->scrPowerSupply_chartPower, ui->scrPowerSupply_chartPowerSeries, 0);
	}

	//Update current screen layout.
	lv_obj_update_layout(ui->scrPowerSupply);

	//Init events for screen.
	events_init_scrPowerSupply(ui);
}

