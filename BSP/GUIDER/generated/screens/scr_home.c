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



void setup_scr_scrHome(lv_ui *ui)
{
	//Write codes scrHome
	ui->scrHome = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrHome, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrHome, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome, lv_color_hex(0xF3F8FE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_contBG
	ui->scrHome_contBG = lv_obj_create(ui->scrHome);
	lv_obj_set_pos(ui->scrHome_contBG, 0, 0);
	lv_obj_set_size(ui->scrHome_contBG, 800, 56);
	lv_obj_set_scrollbar_mode(ui->scrHome_contBG, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_contBG, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_contBG, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_contBG, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_contBG, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_imgWifiStatus
	ui->scrHome_imgWifiStatus = lv_img_create(ui->scrHome_contBG);
	lv_obj_add_flag(ui->scrHome_imgWifiStatus, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrHome_imgWifiStatus, &_load_alpha_40x40);
	lv_img_set_pivot(ui->scrHome_imgWifiStatus, 50, 50);
	lv_img_set_angle(ui->scrHome_imgWifiStatus, 0);
	lv_obj_set_pos(ui->scrHome_imgWifiStatus, 25, 8);
	lv_obj_set_size(ui->scrHome_imgWifiStatus, 40, 40);

	//Write style for scrHome_imgWifiStatus, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrHome_imgWifiStatus, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_labelWifiStatus (hidden - only icon visible)
	ui->scrHome_labelWifiStatus = lv_label_create(ui->scrHome_contBG);
	lv_label_set_text(ui->scrHome_labelWifiStatus, "");
	lv_obj_add_flag(ui->scrHome_labelWifiStatus, LV_OBJ_FLAG_HIDDEN);

	//Write codes scrHome_contMain
	ui->scrHome_contMain = lv_obj_create(ui->scrHome);
	lv_obj_set_pos(ui->scrHome_contMain, 59, 83);
	lv_obj_set_size(ui->scrHome_contMain, 683, 247);
	lv_obj_set_scrollbar_mode(ui->scrHome_contMain, LV_SCROLLBAR_MODE_AUTO);

	//Write style for scrHome_contMain, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_contMain, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_contMain, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_contMain, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_contMain, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_contMain, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_contMain, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_contMain, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_contMain, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_contMain, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_contMain, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_contPrint
	ui->scrHome_contPrint = lv_obj_create(ui->scrHome_contMain);
	lv_obj_set_pos(ui->scrHome_contPrint, 366, 33);
	lv_obj_set_size(ui->scrHome_contPrint, 158, 184);
	lv_obj_set_scrollbar_mode(ui->scrHome_contPrint, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_contPrint, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_contPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_contPrint, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_contPrint, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_contPrint, lv_color_hex(0x46b146), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_contPrint, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_contPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_contPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_contPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_contPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_contPrint, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrHome_contPrint, lv_color_hex(0x2a7a2a), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrHome_contPrint, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrHome_contPrint, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrHome_contPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrHome_contPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_imgIconPrint
	ui->scrHome_imgIconPrint = lv_img_create(ui->scrHome_contPrint);
	lv_obj_add_flag(ui->scrHome_imgIconPrint, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrHome_imgIconPrint, &_vao_alpha_70x70);
	lv_img_set_pivot(ui->scrHome_imgIconPrint, 50,50);
	lv_img_set_angle(ui->scrHome_imgIconPrint, 0);
	lv_obj_center(ui->scrHome_imgIconPrint);
	lv_obj_add_flag(ui->scrHome_imgIconPrint, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_imgIconPrint, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrHome_imgIconPrint, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_labelPrint
	ui->scrHome_labelPrint = lv_label_create(ui->scrHome_contPrint);
	lv_label_set_text(ui->scrHome_labelPrint, "Voltmeter");
	lv_label_set_long_mode(ui->scrHome_labelPrint, LV_LABEL_LONG_WRAP);
	lv_obj_add_flag(ui->scrHome_labelPrint, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_labelPrint, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrHome_labelPrint, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrHome_labelPrint, &lv_font_ShanHaiZhongXiaYeWuYuW_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrHome_labelPrint, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrHome_labelPrint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_labelPrint, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_contCopy
	ui->scrHome_contCopy = lv_obj_create(ui->scrHome_contMain);
	lv_obj_set_pos(ui->scrHome_contCopy, 20, 33);
	lv_obj_set_size(ui->scrHome_contCopy, 158, 184);
	lv_obj_set_scrollbar_mode(ui->scrHome_contCopy, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_contCopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_contCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_contCopy, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_contCopy, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_contCopy, lv_color_hex(0x342e3c), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_contCopy, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_contCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_contCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_contCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_contCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_contCopy, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrHome_contCopy, lv_color_hex(0x1a1520), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrHome_contCopy, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrHome_contCopy, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrHome_contCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrHome_contCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_imgIconCopy
	ui->scrHome_imgIconCopy = lv_img_create(ui->scrHome_contCopy);
	lv_obj_add_flag(ui->scrHome_imgIconCopy, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrHome_imgIconCopy, &_sina_alpha_75x75);
	lv_img_set_pivot(ui->scrHome_imgIconCopy, 50,50);
	lv_img_set_angle(ui->scrHome_imgIconCopy, 0);
	lv_obj_center(ui->scrHome_imgIconCopy);
	lv_obj_add_flag(ui->scrHome_imgIconCopy, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_imgIconCopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrHome_imgIconCopy, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_labelCopy
	ui->scrHome_labelCopy = lv_label_create(ui->scrHome_contCopy);
	lv_label_set_text(ui->scrHome_labelCopy, "Signal-Gen");
	lv_label_set_long_mode(ui->scrHome_labelCopy, LV_LABEL_LONG_WRAP);
	lv_obj_add_flag(ui->scrHome_labelCopy, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_labelCopy, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrHome_labelCopy, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrHome_labelCopy, &lv_font_ShanHaiZhongXiaYeWuYuW_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrHome_labelCopy, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrHome_labelCopy, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_labelCopy, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_contScan
	ui->scrHome_contScan = lv_obj_create(ui->scrHome_contMain);
	lv_obj_set_pos(ui->scrHome_contScan, 193, 33);
	lv_obj_set_size(ui->scrHome_contScan, 158, 184);
	lv_obj_set_scrollbar_mode(ui->scrHome_contScan, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_contScan, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_contScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_contScan, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_contScan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_contScan, lv_color_hex(0x4c55c4), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_contScan, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_contScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_contScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_contScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_contScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_contScan, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrHome_contScan, lv_color_hex(0x3040a0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrHome_contScan, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrHome_contScan, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrHome_contScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrHome_contScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_imgIconScan
	ui->scrHome_imgIconScan = lv_img_create(ui->scrHome_contScan);
	lv_obj_add_flag(ui->scrHome_imgIconScan, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrHome_imgIconScan, &_666_alpha_70x70);
	lv_img_set_pivot(ui->scrHome_imgIconScan, 50,50);
	lv_img_set_angle(ui->scrHome_imgIconScan, 0);
	lv_obj_center(ui->scrHome_imgIconScan);
	lv_obj_add_flag(ui->scrHome_imgIconScan, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_imgIconScan, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrHome_imgIconScan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_labelScan
	ui->scrHome_labelScan = lv_label_create(ui->scrHome_contScan);
	lv_label_set_text(ui->scrHome_labelScan, "Oscilloscope");
	lv_label_set_long_mode(ui->scrHome_labelScan, LV_LABEL_LONG_WRAP);
	lv_obj_add_flag(ui->scrHome_labelScan, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_labelScan, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrHome_labelScan, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrHome_labelScan, &lv_font_ShanHaiZhongXiaYeWuYuW_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrHome_labelScan, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrHome_labelScan, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_labelScan, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_cont_1
	ui->scrHome_cont_1 = lv_obj_create(ui->scrHome_contMain);
	lv_obj_set_pos(ui->scrHome_cont_1, 712, 33);
	lv_obj_set_size(ui->scrHome_cont_1, 158, 184);
	lv_obj_set_scrollbar_mode(ui->scrHome_cont_1, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_cont_1, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_cont_1, lv_color_hex(0x7a7e63), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_cont_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrHome_cont_1, lv_color_hex(0x4a4e33), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrHome_cont_1, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrHome_cont_1, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrHome_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrHome_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_img_1
	ui->scrHome_img_1 = lv_img_create(ui->scrHome_cont_1);
	lv_obj_add_flag(ui->scrHome_img_1, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrHome_img_1, &_COM001_alpha_89x70);
	lv_img_set_pivot(ui->scrHome_img_1, 50,50);
	lv_img_set_angle(ui->scrHome_img_1, 0);
	lv_obj_center(ui->scrHome_img_1);
	lv_obj_add_flag(ui->scrHome_img_1, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrHome_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_label_1
	ui->scrHome_label_1 = lv_label_create(ui->scrHome_cont_1);
	lv_label_set_text(ui->scrHome_label_1, "Serial-Port ");
	lv_label_set_long_mode(ui->scrHome_label_1, LV_LABEL_LONG_WRAP);
	lv_obj_add_flag(ui->scrHome_label_1, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrHome_label_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrHome_label_1, &lv_font_ShanHaiZhongXiaYeWuYuW_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrHome_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrHome_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_cont_2
	ui->scrHome_cont_2 = lv_obj_create(ui->scrHome_contMain);
	lv_obj_set_pos(ui->scrHome_cont_2, 885, 32);
	lv_obj_set_size(ui->scrHome_cont_2, 158, 184);
	lv_obj_set_scrollbar_mode(ui->scrHome_cont_2, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_cont_2, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_cont_2, lv_color_hex(0x496073), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_cont_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_cont_2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrHome_cont_2, lv_color_hex(0x2a3540), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrHome_cont_2, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrHome_cont_2, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrHome_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrHome_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_img_2
	ui->scrHome_img_2 = lv_img_create(ui->scrHome_cont_2);
	lv_obj_add_flag(ui->scrHome_img_2, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrHome_img_2, &_set_alpha_68x70);
	lv_img_set_pivot(ui->scrHome_img_2, 50,50);
	lv_img_set_angle(ui->scrHome_img_2, 0);
	lv_obj_center(ui->scrHome_img_2);
	lv_obj_add_flag(ui->scrHome_img_2, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrHome_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_label_2
	ui->scrHome_label_2 = lv_label_create(ui->scrHome_cont_2);
	lv_label_set_text(ui->scrHome_label_2, "Settings");
	lv_label_set_long_mode(ui->scrHome_label_2, LV_LABEL_LONG_WRAP);
	lv_obj_add_flag(ui->scrHome_label_2, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrHome_label_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrHome_label_2, &lv_font_ShanHaiZhongXiaYeWuYuW_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrHome_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrHome_label_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_cont_5
	ui->scrHome_cont_5 = lv_obj_create(ui->scrHome_contMain);
	lv_obj_set_pos(ui->scrHome_cont_5, 539, 33);
	lv_obj_set_size(ui->scrHome_cont_5, 158, 184);
	lv_obj_set_scrollbar_mode(ui->scrHome_cont_5, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_cont_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_cont_5, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_cont_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_cont_5, lv_color_hex(0xe12e2e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_cont_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_cont_5, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrHome_cont_5, lv_color_hex(0xa01818), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrHome_cont_5, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrHome_cont_5, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrHome_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrHome_cont_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_img_4
	ui->scrHome_img_4 = lv_img_create(ui->scrHome_cont_5);
	lv_obj_add_flag(ui->scrHome_img_4, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrHome_img_4, &_33_alpha_76x75);
	lv_img_set_pivot(ui->scrHome_img_4, 50,50);
	lv_img_set_angle(ui->scrHome_img_4, 0);
	lv_obj_center(ui->scrHome_img_4);
	lv_obj_add_flag(ui->scrHome_img_4, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_img_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrHome_img_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_label_5
	ui->scrHome_label_5 = lv_label_create(ui->scrHome_cont_5);
	lv_label_set_text(ui->scrHome_label_5, "Power");
	lv_label_set_long_mode(ui->scrHome_label_5, LV_LABEL_LONG_WRAP);
	lv_obj_add_flag(ui->scrHome_label_5, LV_OBJ_FLAG_EVENT_BUBBLE);

	//Write style for scrHome_label_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrHome_label_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrHome_label_5, &lv_font_ShanHaiZhongXiaYeWuYuW_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrHome_label_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrHome_label_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_contTop
	ui->scrHome_contTop = lv_obj_create(ui->scrHome);
	lv_obj_set_pos(ui->scrHome_contTop, -48, 0);
	lv_obj_set_size(ui->scrHome_contTop, 880, 107);
	lv_obj_set_scrollbar_mode(ui->scrHome_contTop, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_contTop, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_contTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* imgIconCall removed - not needed */

	//Write codes scrHome_spangroup_1
	ui->scrHome_spangroup_1 = lv_spangroup_create(ui->scrHome_contTop);
	lv_spangroup_set_align(ui->scrHome_spangroup_1, LV_TEXT_ALIGN_LEFT);
	lv_spangroup_set_overflow(ui->scrHome_spangroup_1, LV_SPAN_OVERFLOW_CLIP);
	lv_spangroup_set_mode(ui->scrHome_spangroup_1, LV_SPAN_MODE_BREAK);
	//create spans
	lv_span_t *scrHome_spangroup_1_span;
	scrHome_spangroup_1_span = lv_spangroup_new_span(ui->scrHome_spangroup_1);
	lv_span_set_text(scrHome_spangroup_1_span, "1,Aug,2025,12:00");
	lv_style_set_text_color(&scrHome_spangroup_1_span->style, lv_color_hex(0xffffff));
	lv_style_set_text_decor(&scrHome_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
	lv_style_set_text_font(&scrHome_spangroup_1_span->style, &lv_font_ShanHaiZhongXiaYeWuYuW_30);
	lv_obj_set_pos(ui->scrHome_spangroup_1,580 , 15);  /* 从600向左移动到550 */
	lv_obj_set_size(ui->scrHome_spangroup_1, 265, 38);  /* 增加宽度以容纳更长的时间文本 */

	//Write style state: LV_STATE_DEFAULT for &style_scrHome_spangroup_1_main_main_default
	static lv_style_t style_scrHome_spangroup_1_main_main_default;
	ui_init_style(&style_scrHome_spangroup_1_main_main_default);
	
	lv_style_set_border_width(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_style_set_radius(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_style_set_bg_opa(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_style_set_pad_top(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_style_set_pad_right(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_style_set_pad_bottom(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_style_set_pad_left(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_style_set_shadow_width(&style_scrHome_spangroup_1_main_main_default, 0);
	lv_obj_add_style(ui->scrHome_spangroup_1, &style_scrHome_spangroup_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_spangroup_refr_mode(ui->scrHome_spangroup_1);

	//Write codes scrHome_contColorInk
	ui->scrHome_contColorInk = lv_obj_create(ui->scrHome);
	lv_obj_set_pos(ui->scrHome_contColorInk, 507, 357);
	lv_obj_set_size(ui->scrHome_contColorInk, 235, 91);
	lv_obj_set_scrollbar_mode(ui->scrHome_contColorInk, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrHome_contColorInk, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrHome_contColorInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_contColorInk, 14, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrHome_contColorInk, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_contColorInk, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_contColorInk, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrHome_contColorInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrHome_contColorInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrHome_contColorInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrHome_contColorInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_contColorInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrHome_barBlueInk
	ui->scrHome_barBlueInk = lv_bar_create(ui->scrHome_contColorInk);
	lv_obj_set_style_anim_time(ui->scrHome_barBlueInk, 1000, 0);
	lv_bar_set_mode(ui->scrHome_barBlueInk, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->scrHome_barBlueInk, 0, 100);
	lv_bar_set_value(ui->scrHome_barBlueInk, 80, LV_ANIM_ON);
	lv_obj_set_pos(ui->scrHome_barBlueInk, 16, 0);
	lv_obj_set_size(ui->scrHome_barBlueInk, 40, 88);

	//Write style for scrHome_barBlueInk, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barBlueInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barBlueInk, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_barBlueInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrHome_barBlueInk, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barBlueInk, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_barBlueInk, lv_color_hex(0x2ad3ff), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_barBlueInk, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barBlueInk, 17, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes scrHome_barRedInk
	ui->scrHome_barRedInk = lv_bar_create(ui->scrHome_contColorInk);
	lv_obj_set_style_anim_time(ui->scrHome_barRedInk, 1000, 0);
	lv_bar_set_mode(ui->scrHome_barRedInk, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->scrHome_barRedInk, 0, 100);
	lv_bar_set_value(ui->scrHome_barRedInk, 25, LV_ANIM_ON);
	lv_obj_set_pos(ui->scrHome_barRedInk, 68, 0);
	lv_obj_set_size(ui->scrHome_barRedInk, 35, 88);

	//Write style for scrHome_barRedInk, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barRedInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barRedInk, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_barRedInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrHome_barRedInk, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barRedInk, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_barRedInk, lv_color_hex(0xef1382), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_barRedInk, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barRedInk, 17, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes scrHome_barYellowInk
	ui->scrHome_barYellowInk = lv_bar_create(ui->scrHome_contColorInk);
	lv_obj_set_style_anim_time(ui->scrHome_barYellowInk, 1000, 0);
	lv_bar_set_mode(ui->scrHome_barYellowInk, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->scrHome_barYellowInk, 0, 100);
	lv_bar_set_value(ui->scrHome_barYellowInk, 70, LV_ANIM_ON);
	lv_obj_set_pos(ui->scrHome_barYellowInk, 115, 0);
	lv_obj_set_size(ui->scrHome_barYellowInk, 40, 88);

	//Write style for scrHome_barYellowInk, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barYellowInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barYellowInk, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_barYellowInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrHome_barYellowInk, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barYellowInk, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_barYellowInk, lv_color_hex(0xe4ea09), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_barYellowInk, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barYellowInk, 17, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes scrHome_barBlackInk
	ui->scrHome_barBlackInk = lv_bar_create(ui->scrHome_contColorInk);
	lv_obj_set_style_anim_time(ui->scrHome_barBlackInk, 1000, 0);
	lv_bar_set_mode(ui->scrHome_barBlackInk, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->scrHome_barBlackInk, 0, 100);
	lv_bar_set_value(ui->scrHome_barBlackInk, 55, LV_ANIM_ON);
	lv_obj_set_pos(ui->scrHome_barBlackInk, 171, -3);
	lv_obj_set_size(ui->scrHome_barBlackInk, 40, 88);

	//Write style for scrHome_barBlackInk, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barBlackInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barBlackInk, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_barBlackInk, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrHome_barBlackInk, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_barBlackInk, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_barBlackInk, lv_color_hex(0x000000), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_barBlackInk, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_barBlackInk, 17, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes scrHome_btn_1
	ui->scrHome_btn_1 = lv_btn_create(ui->scrHome);
	ui->scrHome_btn_1_label = lv_label_create(ui->scrHome_btn_1);
	lv_label_set_text(ui->scrHome_btn_1_label, "Hi! What do you want to do today?");
	lv_label_set_long_mode(ui->scrHome_btn_1_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrHome_btn_1_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrHome_btn_1, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrHome_btn_1_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrHome_btn_1, 59, 373);
	lv_obj_set_size(ui->scrHome_btn_1, 430, 58);

	//Write style for scrHome_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrHome_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrHome_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrHome_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrHome_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->scrHome_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrHome_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(ui->scrHome_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrHome_btn_1, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrHome_btn_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrHome_btn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrHome_btn_1, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrHome_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrHome_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrHome_btn_1, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrHome_btn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrHome_btn_1, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrHome_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrHome_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrHome_btn_1, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	lv_obj_set_style_bg_opa(ui->scrHome_btn_1, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_bg_color(ui->scrHome_btn_1, lv_color_hex(0x5B9BD5), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_border_color(ui->scrHome_btn_1, lv_color_hex(0x5B9BD5), LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_opa(ui->scrHome_btn_1, 60, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_color(ui->scrHome_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_PRESSED);

	//The custom code of scrHome.
	configure_scrHome_scroll(ui);

	//Update current screen layout.
	lv_obj_update_layout(ui->scrHome);

	//Init events for screen.
	events_init_scrHome(ui);
}
