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
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

void setup_scr_scrSettings(lv_ui *ui)
{
	//Write codes scrSettings
	ui->scrSettings = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrSettings, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrSettings, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrSettings, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings, lv_color_hex(0xF3F8FE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrSettings, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrSettings_contBG (Top bar)
	ui->scrSettings_contBG = lv_obj_create(ui->scrSettings);
	lv_obj_set_pos(ui->scrSettings_contBG, 0, 0);
	lv_obj_set_size(ui->scrSettings_contBG, 800, 80);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contBG, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrSettings_contBG, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrSettings_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contBG, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_contBG, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_contBG, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrSettings_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrSettings_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrSettings_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrSettings_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrSettings_labelTitle
	ui->scrSettings_labelTitle = lv_label_create(ui->scrSettings_contBG);
	lv_label_set_text(ui->scrSettings_labelTitle, "SETTINGS");
	lv_label_set_long_mode(ui->scrSettings_labelTitle, LV_LABEL_LONG_CLIP);
	lv_obj_set_pos(ui->scrSettings_labelTitle, 150, 12);
	lv_obj_set_size(ui->scrSettings_labelTitle, 500, 50);

	//Write style for scrSettings_labelTitle, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_labelTitle, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelTitle, &lv_font_ShanHaiZhongXiaYeWuYuW_45, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrSettings_labelTitle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrSettings_labelTitle, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_labelTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrSettings_btnBack
	ui->scrSettings_btnBack = lv_btn_create(ui->scrSettings);
	ui->scrSettings_btnBack_label = lv_label_create(ui->scrSettings_btnBack);
	lv_label_set_text(ui->scrSettings_btnBack_label, "<");
	lv_label_set_long_mode(ui->scrSettings_btnBack_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrSettings_btnBack_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrSettings_btnBack, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrSettings_btnBack_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrSettings_btnBack, 20, 12);
	lv_obj_set_size(ui->scrSettings_btnBack, 50, 50);

	//Write style for scrSettings_btnBack, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnBack, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnBack, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnBack, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrSettings_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_btnBack, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrSettings_contLeft (Left menu panel)
	ui->scrSettings_contLeft = lv_obj_create(ui->scrSettings);
	lv_obj_set_pos(ui->scrSettings_contLeft, 20, 100);
	lv_obj_set_size(ui->scrSettings_contLeft, 220, 360);  // Fixed height: 360px
	lv_obj_set_scrollbar_mode(ui->scrSettings_contLeft, LV_SCROLLBAR_MODE_AUTO);  // Enable scrollbar when needed
	lv_obj_set_scroll_dir(ui->scrSettings_contLeft, LV_DIR_VER);  // Vertical scroll only
	lv_obj_clear_flag(ui->scrSettings_contLeft, LV_OBJ_FLAG_SCROLL_ELASTIC);  // Disable elastic scroll
	lv_obj_set_scroll_snap_y(ui->scrSettings_contLeft, LV_SCROLL_SNAP_NONE);  // No snap

	//Write style for scrSettings_contLeft, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrSettings_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contLeft, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contLeft, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_contLeft, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_contLeft, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrSettings_contLeft, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrSettings_contLeft, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrSettings_contLeft, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrSettings_contLeft, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_contLeft, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Ensure fixed height - prevent auto-sizing based on content
	lv_obj_set_style_max_height(ui->scrSettings_contLeft, 360, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_min_height(ui->scrSettings_contLeft, 360, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrSettings_contRight (Right content panel)
	ui->scrSettings_contRight = lv_obj_create(ui->scrSettings);
	lv_obj_set_pos(ui->scrSettings_contRight, 260, 100);
	lv_obj_set_size(ui->scrSettings_contRight, 520, 360);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contRight, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrSettings_contRight, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrSettings_contRight, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contRight, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contRight, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_contRight, lv_color_hex(0xF5F5F5), LV_PART_MAIN|LV_STATE_DEFAULT);  /* 浅灰色背景 */
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_contRight, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrSettings_contRight, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrSettings_contRight, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrSettings_contRight, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrSettings_contRight, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_contRight, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * LEFT MENU - Brightness Button
	 * ========================================*/
	ui->scrSettings_btnMenuBrightness = lv_btn_create(ui->scrSettings_contLeft);
	lv_obj_set_pos(ui->scrSettings_btnMenuBrightness, 0, 0);
	lv_obj_set_size(ui->scrSettings_btnMenuBrightness, 190, 80);
	lv_obj_add_flag(ui->scrSettings_btnMenuBrightness, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_state(ui->scrSettings_btnMenuBrightness, LV_STATE_CHECKED);  // Default selected
	
	// Set flex layout for centered icon + text
	lv_obj_set_layout(ui->scrSettings_btnMenuBrightness, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(ui->scrSettings_btnMenuBrightness, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui->scrSettings_btnMenuBrightness, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(ui->scrSettings_btnMenuBrightness, 12, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuBrightness, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuBrightness, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuBrightness, lv_color_hex(0xFFECB3), LV_PART_MAIN|LV_STATE_DEFAULT); // warm yellow
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuBrightness, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuBrightness, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnMenuBrightness, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuBrightness, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuBrightness, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuBrightness, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuBrightness, lv_color_hex(0xFF6F00), LV_PART_MAIN|LV_STATE_CHECKED); // vibrant orange
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuBrightness, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuBrightness, 0, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuBrightness, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_imgMenuBrightness = lv_img_create(ui->scrSettings_btnMenuBrightness);
	lv_img_set_src(ui->scrSettings_imgMenuBrightness, &_bright_alpha_33x33);
	lv_obj_set_size(ui->scrSettings_imgMenuBrightness, 33, 33);
	lv_obj_set_style_img_opa(ui->scrSettings_imgMenuBrightness, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrSettings_imgMenuBrightness, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrSettings_imgMenuBrightness, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_img_recolor(ui->scrSettings_imgMenuBrightness, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_btnMenuBrightness_label = lv_label_create(ui->scrSettings_btnMenuBrightness);
	lv_label_set_text(ui->scrSettings_btnMenuBrightness_label, "Brightness");
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuBrightness_label, lv_color_hex(0x37474f), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuBrightness_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->scrSettings_btnMenuBrightness_label, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * LEFT MENU - WiFi Button
	 * ========================================*/
	ui->scrSettings_btnMenuWifi = lv_btn_create(ui->scrSettings_contLeft);
	lv_obj_set_pos(ui->scrSettings_btnMenuWifi, 0, 90);  // 0 + 80 + 10 = 90
	lv_obj_set_size(ui->scrSettings_btnMenuWifi, 190, 80);
	lv_obj_add_flag(ui->scrSettings_btnMenuWifi, LV_OBJ_FLAG_CHECKABLE);
	
	// Set flex layout for centered icon + text
	lv_obj_set_layout(ui->scrSettings_btnMenuWifi, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(ui->scrSettings_btnMenuWifi, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui->scrSettings_btnMenuWifi, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(ui->scrSettings_btnMenuWifi, 12, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuWifi, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuWifi, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuWifi, lv_color_hex(0xB3E5FC), LV_PART_MAIN|LV_STATE_DEFAULT); // sky blue
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuWifi, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuWifi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnMenuWifi, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuWifi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuWifi, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuWifi, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuWifi, lv_color_hex(0x0091EA), LV_PART_MAIN|LV_STATE_CHECKED); // bright blue
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuWifi, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuWifi, 0, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuWifi, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_imgMenuWifi = lv_img_create(ui->scrSettings_btnMenuWifi);
	lv_img_set_src(ui->scrSettings_imgMenuWifi, &_wifi_alpha_39x34);
	lv_obj_set_size(ui->scrSettings_imgMenuWifi, 39, 34);
	lv_obj_set_style_img_opa(ui->scrSettings_imgMenuWifi, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrSettings_imgMenuWifi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrSettings_imgMenuWifi, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_img_recolor(ui->scrSettings_imgMenuWifi, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_btnMenuWifi_label = lv_label_create(ui->scrSettings_btnMenuWifi);
	lv_label_set_text(ui->scrSettings_btnMenuWifi_label, "WiFi");
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuWifi_label, lv_color_hex(0x37474f), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuWifi_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->scrSettings_btnMenuWifi_label, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * LEFT MENU - About Button
	 * ========================================*/
	ui->scrSettings_btnMenuAbout = lv_btn_create(ui->scrSettings_contLeft);
	lv_obj_set_pos(ui->scrSettings_btnMenuAbout, 0, 180);  // 90 + 80 + 10 = 180
	lv_obj_set_size(ui->scrSettings_btnMenuAbout, 190, 70);
	lv_obj_add_flag(ui->scrSettings_btnMenuAbout, LV_OBJ_FLAG_CHECKABLE);
	
	// Set flex layout for centered icon + text
	lv_obj_set_layout(ui->scrSettings_btnMenuAbout, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(ui->scrSettings_btnMenuAbout, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui->scrSettings_btnMenuAbout, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(ui->scrSettings_btnMenuAbout, 12, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuAbout, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuAbout, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuAbout, lv_color_hex(0xF3E5F5), LV_PART_MAIN|LV_STATE_DEFAULT); // light pink-purple
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuAbout, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuAbout, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnMenuAbout, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuAbout, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuAbout, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuAbout, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuAbout, lv_color_hex(0xAA00FF), LV_PART_MAIN|LV_STATE_CHECKED); // neon purple
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuAbout, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuAbout, 0, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuAbout, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_imgMenuAbout = lv_img_create(ui->scrSettings_btnMenuAbout);
	lv_img_set_src(ui->scrSettings_imgMenuAbout, &_about_alpha_36x36);
	lv_obj_set_size(ui->scrSettings_imgMenuAbout, 36, 36);
	lv_obj_set_style_img_opa(ui->scrSettings_imgMenuAbout, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrSettings_imgMenuAbout, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_img_recolor_opa(ui->scrSettings_imgMenuAbout, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_img_recolor(ui->scrSettings_imgMenuAbout, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_btnMenuAbout_label = lv_label_create(ui->scrSettings_btnMenuAbout);
	lv_label_set_text(ui->scrSettings_btnMenuAbout_label, "About");
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuAbout_label, lv_color_hex(0x37474f), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuAbout_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->scrSettings_btnMenuAbout_label, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * LEFT MENU - SD Card Button (SD卡管理)
	 * ========================================*/
	ui->scrSettings_btnMenuGallery = lv_btn_create(ui->scrSettings_contLeft);
	lv_obj_set_pos(ui->scrSettings_btnMenuGallery, 0, 260);  // 180 + 70 + 10 = 260
	lv_obj_set_size(ui->scrSettings_btnMenuGallery, 190, 70);
	lv_obj_add_flag(ui->scrSettings_btnMenuGallery, LV_OBJ_FLAG_CHECKABLE);

	// Set flex layout for centered icon + text
	lv_obj_set_layout(ui->scrSettings_btnMenuGallery, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(ui->scrSettings_btnMenuGallery, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui->scrSettings_btnMenuGallery, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(ui->scrSettings_btnMenuGallery, 12, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuGallery, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuGallery, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuGallery, lv_color_hex(0xC8E6C9), LV_PART_MAIN|LV_STATE_DEFAULT); // mint green
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuGallery, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuGallery, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnMenuGallery, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuGallery, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuGallery, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuGallery, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuGallery, lv_color_hex(0x00C853), LV_PART_MAIN|LV_STATE_CHECKED); // vivid green
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuGallery, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuGallery, 0, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuGallery, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	// Create SD Card icon
	ui->scrSettings_imgMenuGallery = lv_label_create(ui->scrSettings_btnMenuGallery);
	lv_label_set_text(ui->scrSettings_imgMenuGallery, LV_SYMBOL_SD_CARD);
	lv_obj_set_style_text_font(ui->scrSettings_imgMenuGallery, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_imgMenuGallery, lv_color_hex(0x37474f), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_imgMenuGallery, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_btnMenuGallery_label = lv_label_create(ui->scrSettings_btnMenuGallery);
	lv_label_set_text(ui->scrSettings_btnMenuGallery_label, "SD Card");
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuGallery_label, lv_color_hex(0x37474f), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuGallery_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->scrSettings_btnMenuGallery_label, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * LEFT MENU - Cloud Platform Button (云平台管理)
	 * ========================================*/
	ui->scrSettings_btnMenuCloud = lv_btn_create(ui->scrSettings_contLeft);
	lv_obj_set_pos(ui->scrSettings_btnMenuCloud, 0, 340);  // 260 + 70 + 10 = 340
	lv_obj_set_size(ui->scrSettings_btnMenuCloud, 190, 70);
	lv_obj_add_flag(ui->scrSettings_btnMenuCloud, LV_OBJ_FLAG_CHECKABLE);

	// Set flex layout for centered icon + text
	lv_obj_set_layout(ui->scrSettings_btnMenuCloud, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(ui->scrSettings_btnMenuCloud, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui->scrSettings_btnMenuCloud, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(ui->scrSettings_btnMenuCloud, 12, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuCloud, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuCloud, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuCloud, lv_color_hex(0xBBDEFB), LV_PART_MAIN|LV_STATE_DEFAULT); // soft blue
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuCloud, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuCloud, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnMenuCloud, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuCloud, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_btnMenuCloud, Part: LV_PART_MAIN, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->scrSettings_btnMenuCloud, 255, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->scrSettings_btnMenuCloud, lv_color_hex(0x304FFE), LV_PART_MAIN|LV_STATE_CHECKED); // indigo blue
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnMenuCloud, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->scrSettings_btnMenuCloud, 0, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnMenuCloud, 0, LV_PART_MAIN|LV_STATE_CHECKED);

	// Create Cloud icon using symbol
	ui->scrSettings_imgMenuCloud = lv_label_create(ui->scrSettings_btnMenuCloud);
	lv_label_set_text(ui->scrSettings_imgMenuCloud, LV_SYMBOL_UPLOAD);
	lv_obj_set_style_text_font(ui->scrSettings_imgMenuCloud, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_imgMenuCloud, lv_color_hex(0x37474f), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_imgMenuCloud, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);

	ui->scrSettings_btnMenuCloud_label = lv_label_create(ui->scrSettings_btnMenuCloud);
	lv_label_set_text(ui->scrSettings_btnMenuCloud_label, "Cloud");
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuCloud_label, lv_color_hex(0x37474f), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnMenuCloud_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui->scrSettings_btnMenuCloud_label, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * RIGHT CONTENT - Brightness Panel
	 * ========================================*/
	ui->scrSettings_contBrightnessPanel = lv_obj_create(ui->scrSettings_contRight);
	lv_obj_set_pos(ui->scrSettings_contBrightnessPanel, 0, 0);
	lv_obj_set_size(ui->scrSettings_contBrightnessPanel, 480, 320);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contBrightnessPanel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrSettings_contBrightnessPanel, LV_OBJ_FLAG_SCROLLABLE);

	//Write style for scrSettings_contBrightnessPanel
	lv_obj_set_style_border_width(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_contBrightnessPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 亮度值显示 - 居中显示
	ui->scrSettings_labelBrightnessValue = lv_label_create(ui->scrSettings_contBrightnessPanel);
	lv_label_set_text(ui->scrSettings_labelBrightnessValue, "80%");
	lv_obj_set_pos(ui->scrSettings_labelBrightnessValue, 140, 100);
	lv_obj_set_size(ui->scrSettings_labelBrightnessValue, 200, 80);
	lv_obj_set_style_text_color(ui->scrSettings_labelBrightnessValue, lv_color_hex(0x2196f3), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelBrightnessValue, &lv_font_Collins_66, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrSettings_labelBrightnessValue, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_labelBrightnessValue, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrSettings_sliderBrightness = lv_slider_create(ui->scrSettings_contBrightnessPanel);
	lv_slider_set_range(ui->scrSettings_sliderBrightness, 0, 100);
	lv_slider_set_value(ui->scrSettings_sliderBrightness, 80, LV_ANIM_OFF);
	lv_obj_set_pos(ui->scrSettings_sliderBrightness, 40, 200);
	lv_obj_set_size(ui->scrSettings_sliderBrightness, 400, 25);

	//Write style for scrSettings_sliderBrightness, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_sliderBrightness, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_sliderBrightness, lv_color_hex(0xe3f2fd), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_sliderBrightness, 50, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_width(ui->scrSettings_sliderBrightness, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_sliderBrightness, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrSettings_sliderBrightness, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_sliderBrightness, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_sliderBrightness, lv_color_hex(0x2196f3), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_sliderBrightness, LV_GRAD_DIR_HOR, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_color(ui->scrSettings_sliderBrightness, lv_color_hex(0x1976d2), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_sliderBrightness, 50, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for scrSettings_sliderBrightness, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrSettings_sliderBrightness, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_sliderBrightness, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_sliderBrightness, 3, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrSettings_sliderBrightness, lv_color_hex(0x2196f3), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_sliderBrightness, 50, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_sliderBrightness, 8, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrSettings_sliderBrightness, lv_color_hex(0x2196f3), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrSettings_sliderBrightness, 120, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrSettings_sliderBrightness, 0, LV_PART_KNOB|LV_STATE_DEFAULT);

	/* ========================================
	 * RIGHT CONTENT - WiFi Panel (简洁布局)
	 * ========================================*/
	ui->scrSettings_contWifiPanel = lv_obj_create(ui->scrSettings_contRight);
	lv_obj_set_pos(ui->scrSettings_contWifiPanel, 0, 0);
	lv_obj_set_size(ui->scrSettings_contWifiPanel, 480, 320);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contWifiPanel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrSettings_contWifiPanel, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(ui->scrSettings_contWifiPanel, LV_OBJ_FLAG_HIDDEN);

	lv_obj_set_style_border_width(ui->scrSettings_contWifiPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contWifiPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contWifiPanel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_contWifiPanel, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrSettings_contWifiPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* === WiFi列表标题 === */
	ui->scrSettings_labelWifiTitle = lv_label_create(ui->scrSettings_contWifiPanel);
	lv_label_set_text(ui->scrSettings_labelWifiTitle, "Available Networks");
	lv_obj_set_pos(ui->scrSettings_labelWifiTitle, 10, 5);
	lv_obj_set_size(ui->scrSettings_labelWifiTitle, 300, 30);
	lv_obj_set_style_text_color(ui->scrSettings_labelWifiTitle, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelWifiTitle, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_labelWifiTitle, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* === WiFi列表(扩大区域) === */
	ui->scrSettings_listWifi = lv_list_create(ui->scrSettings_contWifiPanel);
	lv_obj_set_pos(ui->scrSettings_listWifi, 0, 35);
	lv_obj_set_size(ui->scrSettings_listWifi, 480, 220);
	lv_obj_set_style_bg_opa(ui->scrSettings_listWifi, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_listWifi, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_listWifi, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrSettings_listWifi, lv_color_hex(0xe0e0e0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_listWifi, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrSettings_listWifi, 5, LV_PART_MAIN|LV_STATE_DEFAULT);

	lv_obj_t * btn = lv_list_add_btn(ui->scrSettings_listWifi, LV_SYMBOL_WIFI, "Tap Scan to discover networks");
	lv_obj_set_style_text_color(lv_obj_get_child(btn, 1), lv_color_hex(0x95a5a6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(lv_obj_get_child(btn, 1), &lv_font_ShanHaiZhongXiaYeWuYuW_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* === 操作按钮区域 === */
	lv_obj_t * btn_container = lv_obj_create(ui->scrSettings_contWifiPanel);
	lv_obj_set_pos(btn_container, 0, 262);
	lv_obj_set_size(btn_container, 480, 58);
	lv_obj_set_scrollbar_mode(btn_container, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(btn_container, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(btn_container, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(btn_container, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(btn_container, 5, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Scan按钮
	ui->scrSettings_btnWifiScan = lv_btn_create(btn_container);
	ui->scrSettings_btnWifiScan_label = lv_label_create(ui->scrSettings_btnWifiScan);
	lv_label_set_text(ui->scrSettings_btnWifiScan_label, LV_SYMBOL_REFRESH " Scan");
	lv_obj_align(ui->scrSettings_btnWifiScan_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_width(ui->scrSettings_btnWifiScan_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrSettings_btnWifiScan, 5, 3);
	lv_obj_set_size(ui->scrSettings_btnWifiScan, 145, 48);
	lv_obj_set_style_bg_opa(ui->scrSettings_btnWifiScan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnWifiScan, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnWifiScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnWifiScan, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnWifiScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnWifiScan_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnWifiScan_label, &lv_font_ShanHaiZhongXiaYeWuYuW_22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_btnWifiScan_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Manual按钮
	ui->scrSettings_btnWifiCustom = lv_btn_create(btn_container);
	ui->scrSettings_btnWifiCustom_label = lv_label_create(ui->scrSettings_btnWifiCustom);
	lv_label_set_text(ui->scrSettings_btnWifiCustom_label, LV_SYMBOL_EDIT " Manual");
	lv_obj_align(ui->scrSettings_btnWifiCustom_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_width(ui->scrSettings_btnWifiCustom_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrSettings_btnWifiCustom, 165, 3);
	lv_obj_set_size(ui->scrSettings_btnWifiCustom, 145, 48);
	lv_obj_set_style_bg_opa(ui->scrSettings_btnWifiCustom, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnWifiCustom, lv_color_hex(0x9b59b6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnWifiCustom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnWifiCustom, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnWifiCustom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnWifiCustom_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnWifiCustom_label, &lv_font_ShanHaiZhongXiaYeWuYuW_22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_btnWifiCustom_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Disconnect按钮 (保存到ui结构体以便事件处理)
	ui->scrSettings_btnWifiDisconnect = lv_btn_create(btn_container);
	ui->scrSettings_btnWifiDisconnect_label = lv_label_create(ui->scrSettings_btnWifiDisconnect);
	lv_label_set_text(ui->scrSettings_btnWifiDisconnect_label, LV_SYMBOL_CLOSE " Disconnect");
	lv_obj_align(ui->scrSettings_btnWifiDisconnect_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_width(ui->scrSettings_btnWifiDisconnect_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrSettings_btnWifiDisconnect, 325, 3);
	lv_obj_set_size(ui->scrSettings_btnWifiDisconnect, 145, 48);
	lv_obj_set_style_bg_opa(ui->scrSettings_btnWifiDisconnect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnWifiDisconnect, lv_color_hex(0xe74c3c), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnWifiDisconnect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnWifiDisconnect, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnWifiDisconnect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnWifiDisconnect_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnWifiDisconnect_label, &lv_font_ShanHaiZhongXiaYeWuYuW_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_btnWifiDisconnect_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Spinner for scanning
	ui->scrSettings_spinnerWifiScan = lv_spinner_create(ui->scrSettings_contWifiPanel, 1000, 60);
	lv_obj_set_pos(ui->scrSettings_spinnerWifiScan, 215, 120);
	lv_obj_set_size(ui->scrSettings_spinnerWifiScan, 50, 50);
	lv_obj_set_style_arc_color(ui->scrSettings_spinnerWifiScan, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->scrSettings_spinnerWifiScan, lv_color_hex(0xbdc3c7), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->scrSettings_spinnerWifiScan, LV_OBJ_FLAG_HIDDEN);

	// 隐藏的状态标签(用于代码兼容性)
	ui->scrSettings_labelWifiStatus = lv_label_create(ui->scrSettings_contWifiPanel);
	lv_label_set_text(ui->scrSettings_labelWifiStatus, "");
	lv_obj_add_flag(ui->scrSettings_labelWifiStatus, LV_OBJ_FLAG_HIDDEN);

	/* ========================================
	 * RIGHT CONTENT - About Panel
	 * ========================================*/
	ui->scrSettings_contAboutPanel = lv_obj_create(ui->scrSettings_contRight);
	lv_obj_set_pos(ui->scrSettings_contAboutPanel, 0, 0);
	lv_obj_set_size(ui->scrSettings_contAboutPanel, 480, 320);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contAboutPanel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrSettings_contAboutPanel, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(ui->scrSettings_contAboutPanel, LV_OBJ_FLAG_HIDDEN);  // Hidden by default

	//Write style for scrSettings_contAboutPanel
	lv_obj_set_style_border_width(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_contAboutPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrSettings_labelAboutTitle = lv_label_create(ui->scrSettings_contAboutPanel);
	lv_label_set_text(ui->scrSettings_labelAboutTitle, "About");
	lv_obj_set_pos(ui->scrSettings_labelAboutTitle, 0, 10);
	lv_obj_set_size(ui->scrSettings_labelAboutTitle, 300, 32);
	lv_obj_set_style_text_color(ui->scrSettings_labelAboutTitle, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelAboutTitle, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrSettings_labelAboutTitle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_labelAboutTitle, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrSettings_labelAboutContent = lv_label_create(ui->scrSettings_contAboutPanel);
	lv_label_set_text(ui->scrSettings_labelAboutContent, "This section is reserved for future development.\n\nDevice Information, Version, etc.\nwill be displayed here.");
	lv_label_set_long_mode(ui->scrSettings_labelAboutContent, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->scrSettings_labelAboutContent, 20, 80);
	lv_obj_set_size(ui->scrSettings_labelAboutContent, 440, 200);
	lv_obj_set_style_text_color(ui->scrSettings_labelAboutContent, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelAboutContent, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrSettings_labelAboutContent, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_labelAboutContent, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrSettings_labelAboutContent, 10, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * WiFi Custom Connection Dialog (Initially Hidden)
	 * ========================================*/
	ui->scrSettings_contWifiDialog = lv_obj_create(ui->scrSettings);
	lv_obj_set_size(ui->scrSettings_contWifiDialog, 450, 320);
	lv_obj_center(ui->scrSettings_contWifiDialog);
	lv_obj_add_flag(ui->scrSettings_contWifiDialog, LV_OBJ_FLAG_HIDDEN);
	
	//Write style for WiFi dialog - Clean solid style without shadow
	lv_obj_set_style_bg_opa(ui->scrSettings_contWifiDialog, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_contWifiDialog, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_contWifiDialog, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrSettings_contWifiDialog, lv_color_hex(0x3498db), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->scrSettings_contWifiDialog, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contWifiDialog, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_contWifiDialog, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrSettings_contWifiDialog, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
	
	// Dialog title
	lv_obj_t *dialog_title = lv_label_create(ui->scrSettings_contWifiDialog);
	lv_label_set_text(dialog_title, "Connect to WiFi");
	lv_obj_set_pos(dialog_title, 0, 0);
	lv_obj_set_size(dialog_title, 400, 35);
	lv_obj_set_style_text_color(dialog_title, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dialog_title, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(dialog_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	
	// SSID label
	lv_obj_t *ssid_label = lv_label_create(ui->scrSettings_contWifiDialog);
	lv_label_set_text(ssid_label, "WiFi Name (SSID):");
	lv_obj_set_pos(ssid_label, 0, 50);
	lv_obj_set_style_text_color(ssid_label, lv_color_hex(0x546e7a), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ssid_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	
	// SSID textarea
	ui->scrSettings_textareaSSID = lv_textarea_create(ui->scrSettings_contWifiDialog);
	lv_obj_set_pos(ui->scrSettings_textareaSSID, 0, 75);
	lv_obj_set_size(ui->scrSettings_textareaSSID, 400, 45);
	lv_textarea_set_text(ui->scrSettings_textareaSSID, "");
	lv_textarea_set_placeholder_text(ui->scrSettings_textareaSSID, "Enter WiFi name");
	lv_textarea_set_one_line(ui->scrSettings_textareaSSID, true);
	lv_obj_set_style_border_width(ui->scrSettings_textareaSSID, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrSettings_textareaSSID, lv_color_hex(0xb0bec5), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrSettings_textareaSSID, lv_color_hex(0x4caf50), LV_PART_MAIN|LV_STATE_FOCUSED);
	lv_obj_set_style_radius(ui->scrSettings_textareaSSID, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_textareaSSID, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	
	// Password label
	lv_obj_t *pwd_label = lv_label_create(ui->scrSettings_contWifiDialog);
	lv_label_set_text(pwd_label, "Password:");
	lv_obj_set_pos(pwd_label, 0, 135);
	lv_obj_set_style_text_color(pwd_label, lv_color_hex(0x546e7a), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(pwd_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	
	// Password textarea
	ui->scrSettings_textareaPassword = lv_textarea_create(ui->scrSettings_contWifiDialog);
	lv_obj_set_pos(ui->scrSettings_textareaPassword, 0, 160);
	lv_obj_set_size(ui->scrSettings_textareaPassword, 400, 45);
	lv_textarea_set_text(ui->scrSettings_textareaPassword, "");
	lv_textarea_set_placeholder_text(ui->scrSettings_textareaPassword, "Enter password");
	lv_textarea_set_one_line(ui->scrSettings_textareaPassword, true);
	lv_textarea_set_password_mode(ui->scrSettings_textareaPassword, true);
	lv_obj_set_style_border_width(ui->scrSettings_textareaPassword, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrSettings_textareaPassword, lv_color_hex(0xb0bec5), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrSettings_textareaPassword, lv_color_hex(0x4caf50), LV_PART_MAIN|LV_STATE_FOCUSED);
	lv_obj_set_style_radius(ui->scrSettings_textareaPassword, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_textareaPassword, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	
	// Connect button - Soft Mint Green (柔和薄荷绿)
	ui->scrSettings_btnWifiConnect = lv_btn_create(ui->scrSettings_contWifiDialog);
	ui->scrSettings_btnWifiConnect_label = lv_label_create(ui->scrSettings_btnWifiConnect);
	lv_label_set_text(ui->scrSettings_btnWifiConnect_label, LV_SYMBOL_WIFI "Connect");
	lv_obj_align(ui->scrSettings_btnWifiConnect_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrSettings_btnWifiConnect, 210, 230);
	lv_obj_set_size(ui->scrSettings_btnWifiConnect, 180, 50);

	lv_obj_set_style_bg_opa(ui->scrSettings_btnWifiConnect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnWifiConnect, lv_color_hex(0xB2DFDB), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnWifiConnect, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnWifiConnect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnWifiConnect, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnWifiConnect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnWifiConnect_label, lv_color_hex(0x00695C), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnWifiConnect_label, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_btnWifiConnect_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	
	// Cancel button - Soft Rose (柔和玫瑰色)
	ui->scrSettings_btnWifiCancel = lv_btn_create(ui->scrSettings_contWifiDialog);
	ui->scrSettings_btnWifiCancel_label = lv_label_create(ui->scrSettings_btnWifiCancel);
	lv_label_set_text(ui->scrSettings_btnWifiCancel_label, "Cancel");
	lv_obj_align(ui->scrSettings_btnWifiCancel_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrSettings_btnWifiCancel, 10, 230);
	lv_obj_set_size(ui->scrSettings_btnWifiCancel, 180, 50);

	lv_obj_set_style_bg_opa(ui->scrSettings_btnWifiCancel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnWifiCancel, lv_color_hex(0xF8BBD0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrSettings_btnWifiCancel, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnWifiCancel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnWifiCancel, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnWifiCancel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnWifiCancel_label, lv_color_hex(0x880E4F), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnWifiCancel_label, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_btnWifiCancel_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * RIGHT CONTENT - SD Card Panel (SD卡管理面板)
	 * SD Card Manager UI will be created dynamically in events_init.c
	 * ========================================*/
	ui->scrSettings_contGalleryPanel = lv_obj_create(ui->scrSettings_contRight);
	lv_obj_set_pos(ui->scrSettings_contGalleryPanel, 0, 0);
	lv_obj_set_size(ui->scrSettings_contGalleryPanel, 480, 320);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contGalleryPanel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(ui->scrSettings_contGalleryPanel, LV_OBJ_FLAG_HIDDEN);  // Hidden by default

	lv_obj_set_style_border_width(ui->scrSettings_contGalleryPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contGalleryPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contGalleryPanel, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_contGalleryPanel, lv_color_hex(0x1a1a2e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrSettings_contGalleryPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * RIGHT CONTENT - Cloud Platform Panel (云平台管理面板)
	 * Cloud Manager UI will be created dynamically
	 * ========================================*/
	ui->scrSettings_contCloudPanel = lv_obj_create(ui->scrSettings_contRight);
	lv_obj_set_pos(ui->scrSettings_contCloudPanel, 0, 0);
	lv_obj_set_size(ui->scrSettings_contCloudPanel, 480, 320);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contCloudPanel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(ui->scrSettings_contCloudPanel, LV_OBJ_FLAG_HIDDEN);  // Hidden by default

	lv_obj_set_style_border_width(ui->scrSettings_contCloudPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contCloudPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_contCloudPanel, LV_OPA_TRANSP, LV_PART_MAIN|LV_STATE_DEFAULT);  /* 透明背景 */
	lv_obj_set_style_pad_all(ui->scrSettings_contCloudPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* Note: The following elements are kept for backward compatibility but will be
	 * replaced by SD Card Manager UI when the panel is shown */

	// SD Card title (placeholder - will be replaced)
	ui->scrSettings_labelGalleryTitle = lv_label_create(ui->scrSettings_contGalleryPanel);
	lv_label_set_text(ui->scrSettings_labelGalleryTitle, LV_SYMBOL_SD_CARD " SD Card Manager");
	lv_obj_set_pos(ui->scrSettings_labelGalleryTitle, 0, 0);
	lv_obj_set_size(ui->scrSettings_labelGalleryTitle, 200, 40);
	lv_obj_set_style_text_color(ui->scrSettings_labelGalleryTitle, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelGalleryTitle, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_labelGalleryTitle, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Capacity label (placeholder - will be replaced)
	ui->scrSettings_labelGalleryCount = lv_label_create(ui->scrSettings_contGalleryPanel);
	lv_label_set_text(ui->scrSettings_labelGalleryCount, "Loading...");
	lv_obj_set_pos(ui->scrSettings_labelGalleryCount, 210, 8);
	lv_obj_set_size(ui->scrSettings_labelGalleryCount, 100, 24);
	lv_obj_set_style_text_color(ui->scrSettings_labelGalleryCount, lv_color_hex(0x888888), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelGalleryCount, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Hide USB switch (not needed for SD Card Manager)
	ui->scrSettings_switchUSB = lv_switch_create(ui->scrSettings_contGalleryPanel);
	lv_obj_set_pos(ui->scrSettings_switchUSB, 400, 3);
	lv_obj_set_size(ui->scrSettings_switchUSB, 60, 30);
	lv_obj_add_flag(ui->scrSettings_switchUSB, LV_OBJ_FLAG_HIDDEN);  // Hidden - not needed

	// Switch background (OFF state) - Light gray
	lv_obj_set_style_bg_color(ui->scrSettings_switchUSB, lv_color_hex(0xd0d0d0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrSettings_switchUSB, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_switchUSB, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_switchUSB, 15, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Switch indicator (ON state) - Vibrant blue
	lv_obj_set_style_bg_color(ui->scrSettings_switchUSB, lv_color_hex(0x00BCD4), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_opa(ui->scrSettings_switchUSB, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);

	// Switch knob - White with shadow
	lv_obj_set_style_bg_color(ui->scrSettings_switchUSB, lv_color_hex(0xFFFFFF), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_switchUSB, 6, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrSettings_switchUSB, lv_color_hex(0x000000), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrSettings_switchUSB, 60, LV_PART_KNOB|LV_STATE_DEFAULT);

	/* Note: All legacy Gallery elements below are hidden.
	 * The SD Card Manager UI will be created dynamically when panel is shown.
	 * These are kept for backward compatibility with gui_guider.h struct. */

	// USB Switch Label - Hidden (not needed)
	ui->scrSettings_labelUSB = lv_label_create(ui->scrSettings_contGalleryPanel);
	lv_label_set_text(ui->scrSettings_labelUSB, "");
	lv_obj_add_flag(ui->scrSettings_labelUSB, LV_OBJ_FLAG_HIDDEN);

	// Legacy filter buttons - Hidden (not needed)
	lv_obj_t *filter_cont = lv_obj_create(ui->scrSettings_contGalleryPanel);
	lv_obj_add_flag(filter_cont, LV_OBJ_FLAG_HIDDEN);

	ui->scrSettings_btnGalleryAll = lv_btn_create(filter_cont);
	lv_obj_add_flag(ui->scrSettings_btnGalleryAll, LV_OBJ_FLAG_HIDDEN);

	ui->scrSettings_btnGalleryFlash = lv_btn_create(filter_cont);
	lv_obj_add_flag(ui->scrSettings_btnGalleryFlash, LV_OBJ_FLAG_HIDDEN);

	ui->scrSettings_btnGalleryPsram = lv_btn_create(filter_cont);
	lv_obj_add_flag(ui->scrSettings_btnGalleryPsram, LV_OBJ_FLAG_HIDDEN);

	// Legacy Gallery list - Hidden (not needed)
	ui->scrSettings_listGallery = lv_list_create(ui->scrSettings_contGalleryPanel);
	lv_obj_add_flag(ui->scrSettings_listGallery, LV_OBJ_FLAG_HIDDEN);

	// Legacy empty state label - Hidden (not needed)
	ui->scrSettings_labelGalleryEmpty = lv_label_create(ui->scrSettings_contGalleryPanel);
	lv_obj_add_flag(ui->scrSettings_labelGalleryEmpty, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_text_color(ui->scrSettings_labelGalleryEmpty, lv_color_hex(0x9e9e9e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_labelGalleryEmpty, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrSettings_labelGalleryEmpty, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_flag(ui->scrSettings_labelGalleryEmpty, LV_OBJ_FLAG_HIDDEN);  // Hidden by default

	/* ========================================
	 * Image Viewer Dialog (全屏图片查看器)
	 * ========================================*/
	ui->scrSettings_contImageViewer = lv_obj_create(ui->scrSettings);
	lv_obj_set_pos(ui->scrSettings_contImageViewer, 0, 0);
	lv_obj_set_size(ui->scrSettings_contImageViewer, 800, 480);
	lv_obj_add_flag(ui->scrSettings_contImageViewer, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_scrollbar_mode(ui->scrSettings_contImageViewer, LV_SCROLLBAR_MODE_OFF);

	// Semi-transparent black background
	lv_obj_set_style_bg_opa(ui->scrSettings_contImageViewer, 240, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_contImageViewer, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_contImageViewer, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_contImageViewer, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrSettings_contImageViewer, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Image display
	ui->scrSettings_imgViewer = lv_img_create(ui->scrSettings_contImageViewer);
	lv_obj_center(ui->scrSettings_imgViewer);
	lv_obj_set_size(ui->scrSettings_imgViewer, 800, 480);
	lv_obj_set_style_img_opa(ui->scrSettings_imgViewer, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Delete button
	ui->scrSettings_btnImageDelete = lv_btn_create(ui->scrSettings_contImageViewer);
	ui->scrSettings_btnImageDelete_label = lv_label_create(ui->scrSettings_btnImageDelete);
	lv_label_set_text(ui->scrSettings_btnImageDelete_label, LV_SYMBOL_TRASH " Delete");
	lv_obj_align(ui->scrSettings_btnImageDelete_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrSettings_btnImageDelete, 550, 400);
	lv_obj_set_size(ui->scrSettings_btnImageDelete, 120, 50);

	lv_obj_set_style_bg_opa(ui->scrSettings_btnImageDelete, 200, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnImageDelete, lv_color_hex(0xf44336), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnImageDelete, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnImageDelete, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnImageDelete, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnImageDelete_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnImageDelete_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Close button
	ui->scrSettings_btnImageClose = lv_btn_create(ui->scrSettings_contImageViewer);
	ui->scrSettings_btnImageClose_label = lv_label_create(ui->scrSettings_btnImageClose);
	lv_label_set_text(ui->scrSettings_btnImageClose_label, LV_SYMBOL_CLOSE);
	lv_obj_align(ui->scrSettings_btnImageClose_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrSettings_btnImageClose, 700, 20);
	lv_obj_set_size(ui->scrSettings_btnImageClose, 70, 50);

	lv_obj_set_style_bg_opa(ui->scrSettings_btnImageClose, 200, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnImageClose, lv_color_hex(0x607d8b), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnImageClose, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnImageClose, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnImageClose, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnImageClose_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnImageClose_label, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);

	// 图片信息标签 (显示存储位置和索引)
	ui->scrSettings_labelImageInfo = lv_label_create(ui->scrSettings_contImageViewer);
	lv_label_set_text(ui->scrSettings_labelImageInfo, "FLASH | 1/5");
	lv_obj_set_pos(ui->scrSettings_labelImageInfo, 20, 20);
	lv_obj_set_size(ui->scrSettings_labelImageInfo, 200, 30);
	lv_obj_set_style_bg_opa(ui->scrSettings_labelImageInfo, 180, LV_PART_MAIN);
	lv_obj_set_style_bg_color(ui->scrSettings_labelImageInfo, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_labelImageInfo, 8, LV_PART_MAIN);
	lv_obj_set_style_pad_all(ui->scrSettings_labelImageInfo, 5, LV_PART_MAIN);
	lv_obj_set_style_text_color(ui->scrSettings_labelImageInfo, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(ui->scrSettings_labelImageInfo, &lv_font_montserratMedium_16, LV_PART_MAIN);

	// 上一张按钮
	ui->scrSettings_btnImagePrev = lv_btn_create(ui->scrSettings_contImageViewer);
	lv_obj_set_pos(ui->scrSettings_btnImagePrev, 20, 200);
	lv_obj_set_size(ui->scrSettings_btnImagePrev, 60, 80);
	lv_obj_set_style_bg_opa(ui->scrSettings_btnImagePrev, 150, LV_PART_MAIN);
	lv_obj_set_style_bg_color(ui->scrSettings_btnImagePrev, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_btnImagePrev, 10, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_btnImagePrev, 0, LV_PART_MAIN);
	lv_obj_t *lbl_prev = lv_label_create(ui->scrSettings_btnImagePrev);
	lv_label_set_text(lbl_prev, LV_SYMBOL_LEFT);
	lv_obj_center(lbl_prev);
	lv_obj_set_style_text_color(lbl_prev, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(lbl_prev, &lv_font_montserratMedium_26, LV_PART_MAIN);

	// 下一张按钮
	ui->scrSettings_btnImageNext = lv_btn_create(ui->scrSettings_contImageViewer);
	lv_obj_set_pos(ui->scrSettings_btnImageNext, 720, 200);
	lv_obj_set_size(ui->scrSettings_btnImageNext, 60, 80);
	lv_obj_set_style_bg_opa(ui->scrSettings_btnImageNext, 150, LV_PART_MAIN);
	lv_obj_set_style_bg_color(ui->scrSettings_btnImageNext, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_btnImageNext, 10, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_btnImageNext, 0, LV_PART_MAIN);
	lv_obj_t *lbl_next = lv_label_create(ui->scrSettings_btnImageNext);
	lv_label_set_text(lbl_next, LV_SYMBOL_RIGHT);
	lv_obj_center(lbl_next);
	lv_obj_set_style_text_color(lbl_next, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(lbl_next, &lv_font_montserratMedium_26, LV_PART_MAIN);

	// 迁移存储位置按钮 (Move to FLASH/PSRAM)
	ui->scrSettings_btnImageMove = lv_btn_create(ui->scrSettings_contImageViewer);
	ui->scrSettings_btnImageMove_label = lv_label_create(ui->scrSettings_btnImageMove);
	lv_label_set_text(ui->scrSettings_btnImageMove_label, LV_SYMBOL_DOWNLOAD " Move to FLASH");
	lv_obj_align(ui->scrSettings_btnImageMove_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_pos(ui->scrSettings_btnImageMove, 130, 400);
	lv_obj_set_size(ui->scrSettings_btnImageMove, 180, 50);

	lv_obj_set_style_bg_opa(ui->scrSettings_btnImageMove, 200, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrSettings_btnImageMove, lv_color_hex(0x2196f3), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrSettings_btnImageMove, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrSettings_btnImageMove, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrSettings_btnImageMove, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrSettings_btnImageMove_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrSettings_btnImageMove_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* ========================================
	 * Loading Overlay (加载动画遮罩层)
	 * ========================================*/
	ui->scrSettings_contLoadingOverlay = lv_obj_create(ui->scrSettings);
	lv_obj_set_size(ui->scrSettings_contLoadingOverlay, LV_PCT(100), LV_PCT(100));
	lv_obj_set_pos(ui->scrSettings_contLoadingOverlay, 0, 0);
	lv_obj_set_style_bg_color(ui->scrSettings_contLoadingOverlay, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ui->scrSettings_contLoadingOverlay, 180, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_contLoadingOverlay, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_contLoadingOverlay, 0, LV_PART_MAIN);
	lv_obj_clear_flag(ui->scrSettings_contLoadingOverlay, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(ui->scrSettings_contLoadingOverlay, LV_OBJ_FLAG_HIDDEN);

	// 加载动画旋转器 (Spinner) - 35x35尺寸更美观
	ui->scrSettings_spinnerLoading = lv_spinner_create(ui->scrSettings_contLoadingOverlay, 800, 60);
	lv_obj_set_size(ui->scrSettings_spinnerLoading, 35, 35);
	lv_obj_align(ui->scrSettings_spinnerLoading, LV_ALIGN_CENTER, 0, -10);
	lv_obj_set_style_arc_color(ui->scrSettings_spinnerLoading, lv_color_hex(0x2196f3), LV_PART_INDICATOR);
	lv_obj_set_style_arc_color(ui->scrSettings_spinnerLoading, lv_color_hex(0x404040), LV_PART_MAIN);
	lv_obj_set_style_arc_width(ui->scrSettings_spinnerLoading, 4, LV_PART_INDICATOR);
	lv_obj_set_style_arc_width(ui->scrSettings_spinnerLoading, 4, LV_PART_MAIN);

	// 加载提示文字
	ui->scrSettings_labelLoadingText = lv_label_create(ui->scrSettings_contLoadingOverlay);
	lv_label_set_text(ui->scrSettings_labelLoadingText, "Processing...");
	lv_obj_align(ui->scrSettings_labelLoadingText, LV_ALIGN_CENTER, 0, 35);
	lv_obj_set_style_text_color(ui->scrSettings_labelLoadingText, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(ui->scrSettings_labelLoadingText, &lv_font_montserratMedium_20, LV_PART_MAIN);

	/* ========================================
	 * Storage Selection Dialog (存储选择弹窗)
	 * ========================================*/
	ui->scrSettings_contStorageDialog = lv_obj_create(ui->scrSettings);
	lv_obj_set_size(ui->scrSettings_contStorageDialog, 380, 280);
	lv_obj_center(ui->scrSettings_contStorageDialog);
	lv_obj_add_flag(ui->scrSettings_contStorageDialog, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_bg_color(ui->scrSettings_contStorageDialog, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ui->scrSettings_contStorageDialog, 255, LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_contStorageDialog, 16, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_contStorageDialog, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(ui->scrSettings_contStorageDialog, lv_color_hex(0x3498db), LV_PART_MAIN);
	lv_obj_set_style_shadow_width(ui->scrSettings_contStorageDialog, 20, LV_PART_MAIN);
	lv_obj_set_style_shadow_color(ui->scrSettings_contStorageDialog, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_shadow_opa(ui->scrSettings_contStorageDialog, 60, LV_PART_MAIN);
	lv_obj_set_style_pad_all(ui->scrSettings_contStorageDialog, 20, LV_PART_MAIN);

	// 弹窗标题
	ui->scrSettings_labelStorageTitle = lv_label_create(ui->scrSettings_contStorageDialog);
	lv_label_set_text(ui->scrSettings_labelStorageTitle, LV_SYMBOL_SETTINGS " Default Storage");
	lv_obj_set_pos(ui->scrSettings_labelStorageTitle, 0, 0);
	lv_obj_set_size(ui->scrSettings_labelStorageTitle, 340, 30);
	lv_obj_set_style_text_color(ui->scrSettings_labelStorageTitle, lv_color_hex(0x2c3e50), LV_PART_MAIN);
	lv_obj_set_style_text_font(ui->scrSettings_labelStorageTitle, &lv_font_montserratMedium_22, LV_PART_MAIN);
	lv_obj_set_style_text_align(ui->scrSettings_labelStorageTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

	// 存储信息
	ui->scrSettings_labelStorageInfo = lv_label_create(ui->scrSettings_contStorageDialog);
	lv_label_set_text(ui->scrSettings_labelStorageInfo, "FLASH: 0/5 (Persistent)\nPSRAM: 0/15 (Temporary)");
	lv_obj_set_pos(ui->scrSettings_labelStorageInfo, 0, 40);
	lv_obj_set_size(ui->scrSettings_labelStorageInfo, 340, 50);
	lv_obj_set_style_text_color(ui->scrSettings_labelStorageInfo, lv_color_hex(0x7f8c8d), LV_PART_MAIN);
	lv_obj_set_style_text_font(ui->scrSettings_labelStorageInfo, &lv_font_montserratMedium_16, LV_PART_MAIN);
	lv_obj_set_style_text_align(ui->scrSettings_labelStorageInfo, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

	// FLASH选项按钮
	ui->scrSettings_btnStorageFlash = lv_btn_create(ui->scrSettings_contStorageDialog);
	lv_obj_set_pos(ui->scrSettings_btnStorageFlash, 0, 100);
	lv_obj_set_size(ui->scrSettings_btnStorageFlash, 340, 50);
	lv_obj_set_style_bg_color(ui->scrSettings_btnStorageFlash, lv_color_hex(0xe8f5e9), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_btnStorageFlash, 12, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_btnStorageFlash, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(ui->scrSettings_btnStorageFlash, lv_color_hex(0x4caf50), LV_PART_MAIN);
	lv_obj_t *lbl_st_flash = lv_label_create(ui->scrSettings_btnStorageFlash);
	lv_label_set_text(lbl_st_flash, LV_SYMBOL_SAVE " FLASH (Persistent)");
	lv_obj_center(lbl_st_flash);
	lv_obj_set_style_text_color(lbl_st_flash, lv_color_hex(0x2e7d32), LV_PART_MAIN);
	lv_obj_set_style_text_font(lbl_st_flash, &lv_font_montserratMedium_19, LV_PART_MAIN);

	// PSRAM选项按钮
	ui->scrSettings_btnStoragePsram = lv_btn_create(ui->scrSettings_contStorageDialog);
	lv_obj_set_pos(ui->scrSettings_btnStoragePsram, 0, 160);
	lv_obj_set_size(ui->scrSettings_btnStoragePsram, 340, 50);
	lv_obj_set_style_bg_color(ui->scrSettings_btnStoragePsram, lv_color_hex(0xfff3e0), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_btnStoragePsram, 12, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_btnStoragePsram, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(ui->scrSettings_btnStoragePsram, lv_color_hex(0xff9800), LV_PART_MAIN);
	lv_obj_t *lbl_st_psram = lv_label_create(ui->scrSettings_btnStoragePsram);
	lv_label_set_text(lbl_st_psram, LV_SYMBOL_CHARGE " PSRAM (Temporary)");
	lv_obj_center(lbl_st_psram);
	lv_obj_set_style_text_color(lbl_st_psram, lv_color_hex(0xe65100), LV_PART_MAIN);
	lv_obj_set_style_text_font(lbl_st_psram, &lv_font_montserratMedium_19, LV_PART_MAIN);

	// 关闭按钮
	ui->scrSettings_btnStorageClose = lv_btn_create(ui->scrSettings_contStorageDialog);
	lv_obj_set_pos(ui->scrSettings_btnStorageClose, 290, -15);
	lv_obj_set_size(ui->scrSettings_btnStorageClose, 50, 40);
	lv_obj_set_style_bg_opa(ui->scrSettings_btnStorageClose, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_btnStorageClose, 0, LV_PART_MAIN);
	lv_obj_t *lbl_st_close = lv_label_create(ui->scrSettings_btnStorageClose);
	lv_label_set_text(lbl_st_close, LV_SYMBOL_CLOSE);
	lv_obj_center(lbl_st_close);
	lv_obj_set_style_text_color(lbl_st_close, lv_color_hex(0x95a5a6), LV_PART_MAIN);
	lv_obj_set_style_text_font(lbl_st_close, &lv_font_montserratMedium_20, LV_PART_MAIN);

	/* ========================================
	 * USB Mode Switch Dialog (USB模式切换弹窗)
	 * ========================================*/
	ui->scrSettings_contUSBModeDialog = lv_obj_create(ui->scrSettings);
	lv_obj_set_size(ui->scrSettings_contUSBModeDialog, 420, 320);
	lv_obj_center(ui->scrSettings_contUSBModeDialog);
	lv_obj_add_flag(ui->scrSettings_contUSBModeDialog, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_bg_color(ui->scrSettings_contUSBModeDialog, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ui->scrSettings_contUSBModeDialog, 255, LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_contUSBModeDialog, 16, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_contUSBModeDialog, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(ui->scrSettings_contUSBModeDialog, lv_color_hex(0x9c27b0), LV_PART_MAIN);
	lv_obj_set_style_shadow_width(ui->scrSettings_contUSBModeDialog, 25, LV_PART_MAIN);
	lv_obj_set_style_shadow_color(ui->scrSettings_contUSBModeDialog, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_shadow_opa(ui->scrSettings_contUSBModeDialog, 80, LV_PART_MAIN);
	lv_obj_set_style_pad_all(ui->scrSettings_contUSBModeDialog, 20, LV_PART_MAIN);

	// USB模式标题
	ui->scrSettings_labelUSBModeTitle = lv_label_create(ui->scrSettings_contUSBModeDialog);
	lv_label_set_text(ui->scrSettings_labelUSBModeTitle, LV_SYMBOL_USB " USB Mode");
	lv_obj_set_pos(ui->scrSettings_labelUSBModeTitle, 0, 0);
	lv_obj_set_size(ui->scrSettings_labelUSBModeTitle, 380, 35);
	lv_obj_set_style_text_color(ui->scrSettings_labelUSBModeTitle, lv_color_hex(0x2c3e50), LV_PART_MAIN);
	lv_obj_set_style_text_font(ui->scrSettings_labelUSBModeTitle, &lv_font_montserratMedium_24, LV_PART_MAIN);
	lv_obj_set_style_text_align(ui->scrSettings_labelUSBModeTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

	// 描述文字
	ui->scrSettings_labelUSBModeDesc = lv_label_create(ui->scrSettings_contUSBModeDialog);
	lv_label_set_text(ui->scrSettings_labelUSBModeDesc, "Select USB mode. System will restart\nto apply changes.");
	lv_obj_set_pos(ui->scrSettings_labelUSBModeDesc, 0, 40);
	lv_obj_set_size(ui->scrSettings_labelUSBModeDesc, 380, 45);
	lv_obj_set_style_text_color(ui->scrSettings_labelUSBModeDesc, lv_color_hex(0x7f8c8d), LV_PART_MAIN);
	lv_obj_set_style_text_font(ui->scrSettings_labelUSBModeDesc, &lv_font_montserratMedium_16, LV_PART_MAIN);
	lv_obj_set_style_text_align(ui->scrSettings_labelUSBModeDesc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

	// U盘模式按钮
	ui->scrSettings_btnUSBModeMSC = lv_btn_create(ui->scrSettings_contUSBModeDialog);
	lv_obj_set_pos(ui->scrSettings_btnUSBModeMSC, 0, 120);
	lv_obj_set_size(ui->scrSettings_btnUSBModeMSC, 380, 55);
	lv_obj_set_style_bg_color(ui->scrSettings_btnUSBModeMSC, lv_color_hex(0xe3f2fd), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_btnUSBModeMSC, 12, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_btnUSBModeMSC, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(ui->scrSettings_btnUSBModeMSC, lv_color_hex(0x2196f3), LV_PART_MAIN);
	lv_obj_t *lbl_usb_msc = lv_label_create(ui->scrSettings_btnUSBModeMSC);
	lv_label_set_text(lbl_usb_msc, LV_SYMBOL_DRIVE " USB Disk Mode");
	lv_obj_center(lbl_usb_msc);
	lv_obj_set_style_text_color(lbl_usb_msc, lv_color_hex(0x1565c0), LV_PART_MAIN);
	lv_obj_set_style_text_font(lbl_usb_msc, &lv_font_montserratMedium_19, LV_PART_MAIN);

	// 取消按钮
	ui->scrSettings_btnUSBModeCancel = lv_btn_create(ui->scrSettings_contUSBModeDialog);
	lv_obj_set_pos(ui->scrSettings_btnUSBModeCancel, 130, 250);
	lv_obj_set_size(ui->scrSettings_btnUSBModeCancel, 120, 40);
	lv_obj_set_style_bg_color(ui->scrSettings_btnUSBModeCancel, lv_color_hex(0xeceff1), LV_PART_MAIN);
	lv_obj_set_style_radius(ui->scrSettings_btnUSBModeCancel, 10, LV_PART_MAIN);
	lv_obj_set_style_border_width(ui->scrSettings_btnUSBModeCancel, 0, LV_PART_MAIN);
	lv_obj_t *lbl_usb_cancel = lv_label_create(ui->scrSettings_btnUSBModeCancel);
	lv_label_set_text(lbl_usb_cancel, "Cancel");
	lv_obj_center(lbl_usb_cancel);
	lv_obj_set_style_text_color(lbl_usb_cancel, lv_color_hex(0x546e7a), LV_PART_MAIN);
	lv_obj_set_style_text_font(lbl_usb_cancel, &lv_font_montserratMedium_16, LV_PART_MAIN);

	//Update current screen layout.
	lv_obj_update_layout(ui->scrSettings);

	//Init events for screen.
	events_init_scrSettings(ui);
}

