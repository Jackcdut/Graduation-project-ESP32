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



void setup_scr_scrWirelessSerial(lv_ui *ui)
{
	//Write codes scrWirelessSerial
	ui->scrWirelessSerial = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrWirelessSerial, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrWirelessSerial, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrWirelessSerial, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial, lv_color_hex(0xe8ecf0), LV_PART_MAIN|LV_STATE_DEFAULT);  // Light gray background
	lv_obj_set_style_bg_grad_dir(ui->scrWirelessSerial, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrWirelessSerial_labelTitle - directly on gray background
	ui->scrWirelessSerial_labelTitle = lv_label_create(ui->scrWirelessSerial);
	lv_label_set_text(ui->scrWirelessSerial_labelTitle, "UART-Debug");
	lv_label_set_long_mode(ui->scrWirelessSerial_labelTitle, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->scrWirelessSerial_labelTitle, 275, 15);
	lv_obj_set_size(ui->scrWirelessSerial_labelTitle, 250, 32);

	//Write style for scrWirelessSerial_labelTitle
	lv_obj_set_style_border_width(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_labelTitle, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_labelTitle, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrWirelessSerial_labelTitle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrWirelessSerial_labelTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrWirelessSerial_btnBack - using Settings page style
	ui->scrWirelessSerial_btnBack = lv_btn_create(ui->scrWirelessSerial);
	ui->scrWirelessSerial_btnBack_label = lv_label_create(ui->scrWirelessSerial_btnBack);
	lv_label_set_text(ui->scrWirelessSerial_btnBack_label, "<");
	lv_label_set_long_mode(ui->scrWirelessSerial_btnBack_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrWirelessSerial_btnBack_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrWirelessSerial_btnBack, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrWirelessSerial_btnBack_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrWirelessSerial_btnBack, 20, 12);
	lv_obj_set_size(ui->scrWirelessSerial_btnBack, 50, 50);

	//Write style for scrWirelessSerial_btnBack - same as Settings page but with dark gray color
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_btnBack, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_btnBack, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);  // Dark gray
	lv_obj_set_style_text_font(ui->scrWirelessSerial_btnBack, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrWirelessSerial_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrWirelessSerial_btnBack, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrWirelessSerial_textareaReceive (Left white card - Receive area)
	ui->scrWirelessSerial_textareaReceive = lv_textarea_create(ui->scrWirelessSerial);
	lv_textarea_set_text(ui->scrWirelessSerial_textareaReceive,
		"=== Wi-Fi AT Commands ===\n"
		"AT+CWINIT - Init/Deinit Wi-Fi driver\n"
		"AT+CWMODE - Set Wi-Fi mode (STA/AP/STA+AP)\n"
		"AT+CWJAP - Connect to AP\n"
		"AT+CWLAP - Scan available APs\n"
		"AT+CWQAP - Disconnect from AP\n"
		"AT+CWSAP - Configure SoftAP\n"
		"AT+CWDHCP - Enable/Disable DHCP\n"
		"AT+CIPSTA - Set Station IP address\n"
		"AT+CIPAP - Set SoftAP IP address\n"
		"AT+CIPSTAMAC - Set Station MAC\n"
		"AT+CIPAPMAC - Set SoftAP MAC\n"
		"AT+CWHOSTNAME - Set hostname\n"
		"AT+CWCOUNTRY - Set country code\n\n");
	lv_textarea_set_placeholder_text(ui->scrWirelessSerial_textareaReceive, "");
	lv_obj_set_pos(ui->scrWirelessSerial_textareaReceive, 15, 60);
	lv_obj_set_size(ui->scrWirelessSerial_textareaReceive, 390, 345);  // Height increased by 35 (310+20+15)

	// Enable scrolling for the textarea content only (vertical scrolling)
	lv_obj_set_scrollbar_mode(ui->scrWirelessSerial_textareaReceive, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_scroll_dir(ui->scrWirelessSerial_textareaReceive, LV_DIR_VER);

	// Ensure content starts from top and scrolls properly
	lv_obj_scroll_to_y(ui->scrWirelessSerial_textareaReceive, 0, LV_ANIM_OFF);

	//Write style for scrWirelessSerial_textareaReceive - white card, no border
	lv_obj_set_style_text_color(ui->scrWirelessSerial_textareaReceive, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_textareaReceive, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);  // Larger font
	lv_obj_set_style_text_opa(ui->scrWirelessSerial_textareaReceive, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrWirelessSerial_textareaReceive, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrWirelessSerial_textareaReceive, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_textareaReceive, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_textareaReceive, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);  // Pure white
	lv_obj_set_style_border_width(ui->scrWirelessSerial_textareaReceive, 0, LV_PART_MAIN|LV_STATE_DEFAULT);  // No border
	lv_obj_set_style_radius(ui->scrWirelessSerial_textareaReceive, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrWirelessSerial_textareaReceive, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrWirelessSerial_textareaReceive, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrWirelessSerial_textareaReceive, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrWirelessSerial_textareaReceive, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_textareaReceive, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Scrollbar style
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_textareaReceive, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_textareaReceive, lv_color_hex(0xc0c0c0), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_textareaReceive, 3, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Write codes scrWirelessSerial_btnSend (Right white card - Configuration panel only)
	ui->scrWirelessSerial_btnSend = lv_obj_create(ui->scrWirelessSerial);
	lv_obj_set_pos(ui->scrWirelessSerial_btnSend, 415, 60);  // Aligned with left card
	lv_obj_set_size(ui->scrWirelessSerial_btnSend, 370, 240);  // Configuration panel only (not including checkboxes area)
	lv_obj_set_scrollbar_mode(ui->scrWirelessSerial_btnSend, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrWirelessSerial_btnSend, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling

	//Write style for scrWirelessSerial_btnSend - white card, no border
	lv_obj_set_style_border_width(ui->scrWirelessSerial_btnSend, 0, LV_PART_MAIN|LV_STATE_DEFAULT);  // No border
	lv_obj_set_style_radius(ui->scrWirelessSerial_btnSend, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_btnSend, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_btnSend, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);  // Pure white
	lv_obj_set_style_bg_grad_dir(ui->scrWirelessSerial_btnSend, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrWirelessSerial_btnSend, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrWirelessSerial_btnSend, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrWirelessSerial_btnSend, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrWirelessSerial_btnSend, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_btnSend, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Add configuration labels and dropdowns to right panel - centered layout

	// BaudRate label and dropdown
	ui->scrWirelessSerial_labelBaudRate = lv_label_create(ui->scrWirelessSerial_btnSend);
	lv_label_set_text(ui->scrWirelessSerial_labelBaudRate, "BaudRate");
	lv_obj_set_pos(ui->scrWirelessSerial_labelBaudRate, 15, 20);
	lv_obj_set_size(ui->scrWirelessSerial_labelBaudRate, 100, 28);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_labelBaudRate, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_labelBaudRate, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrWirelessSerial_dropdownBaudRate = lv_dropdown_create(ui->scrWirelessSerial_btnSend);
	lv_dropdown_set_options(ui->scrWirelessSerial_dropdownBaudRate, "115200\n57600\n38400\n19200\n9600\n4800\n2400\n1200");
	lv_obj_set_pos(ui->scrWirelessSerial_dropdownBaudRate, 185, 17);
	lv_obj_set_size(ui->scrWirelessSerial_dropdownBaudRate, 160, 35);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_dropdownBaudRate, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_dropdownBaudRate, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrWirelessSerial_dropdownBaudRate, lv_color_hex(0xc0c0c0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_dropdownBaudRate, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_dropdownBaudRate, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// StopBits label and dropdown
	ui->scrWirelessSerial_labelStopBits = lv_label_create(ui->scrWirelessSerial_btnSend);
	lv_label_set_text(ui->scrWirelessSerial_labelStopBits, "StopBits");
	lv_obj_set_pos(ui->scrWirelessSerial_labelStopBits, 15, 65);
	lv_obj_set_size(ui->scrWirelessSerial_labelStopBits, 100, 28);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_labelStopBits, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_labelStopBits, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrWirelessSerial_dropdownStopBits = lv_dropdown_create(ui->scrWirelessSerial_btnSend);
	lv_dropdown_set_options(ui->scrWirelessSerial_dropdownStopBits, "1\n1.5\n2");
	lv_obj_set_pos(ui->scrWirelessSerial_dropdownStopBits, 185, 62);
	lv_obj_set_size(ui->scrWirelessSerial_dropdownStopBits, 160, 35);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_dropdownStopBits, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_dropdownStopBits, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrWirelessSerial_dropdownStopBits, lv_color_hex(0xc0c0c0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_dropdownStopBits, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_dropdownStopBits, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Length label and dropdown
	ui->scrWirelessSerial_labelLength = lv_label_create(ui->scrWirelessSerial_btnSend);
	lv_label_set_text(ui->scrWirelessSerial_labelLength, "Length");
	lv_obj_set_pos(ui->scrWirelessSerial_labelLength, 15, 110);
	lv_obj_set_size(ui->scrWirelessSerial_labelLength, 100, 28);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_labelLength, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_labelLength, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrWirelessSerial_dropdownLength = lv_dropdown_create(ui->scrWirelessSerial_btnSend);
	lv_dropdown_set_options(ui->scrWirelessSerial_dropdownLength, "8\n7\n6\n5");
	lv_obj_set_pos(ui->scrWirelessSerial_dropdownLength, 185, 107);
	lv_obj_set_size(ui->scrWirelessSerial_dropdownLength, 160, 35);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_dropdownLength, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_dropdownLength, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrWirelessSerial_dropdownLength, lv_color_hex(0xc0c0c0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_dropdownLength, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_dropdownLength, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Parity label and dropdown
	ui->scrWirelessSerial_labelParity = lv_label_create(ui->scrWirelessSerial_btnSend);
	lv_label_set_text(ui->scrWirelessSerial_labelParity, "Parity");
	lv_obj_set_pos(ui->scrWirelessSerial_labelParity, 15, 155);
	lv_obj_set_size(ui->scrWirelessSerial_labelParity, 100, 28);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_labelParity, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_labelParity, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrWirelessSerial_dropdownParity = lv_dropdown_create(ui->scrWirelessSerial_btnSend);
	lv_dropdown_set_options(ui->scrWirelessSerial_dropdownParity, "None\nOdd\nEven");
	lv_obj_set_pos(ui->scrWirelessSerial_dropdownParity, 185, 152);
	lv_obj_set_size(ui->scrWirelessSerial_dropdownParity, 160, 35);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_dropdownParity, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_dropdownParity, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrWirelessSerial_dropdownParity, lv_color_hex(0xc0c0c0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_dropdownParity, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_dropdownParity, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Checkboxes - arranged in a more balanced layout
	ui->scrWirelessSerial_checkboxHexSend = lv_checkbox_create(ui->scrWirelessSerial_btnSend);
	lv_checkbox_set_text(ui->scrWirelessSerial_checkboxHexSend, "Hex-Send");
	lv_obj_set_pos(ui->scrWirelessSerial_checkboxHexSend, 15, 200);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_checkboxHexSend, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_checkboxHexSend, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);

	ui->scrWirelessSerial_checkboxHexReceive = lv_checkbox_create(ui->scrWirelessSerial_btnSend);
	lv_checkbox_set_text(ui->scrWirelessSerial_checkboxHexReceive, "Hex-Receive");
	lv_obj_set_pos(ui->scrWirelessSerial_checkboxHexReceive, 185, 200);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_checkboxHexReceive, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_checkboxHexReceive, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);

	// Right white card container for Send-NewLine and Clear button (below config panel)
	ui->scrWirelessSerial_contClearCard = lv_obj_create(ui->scrWirelessSerial);
	lv_obj_set_pos(ui->scrWirelessSerial_contClearCard, 415, 310);  // Below config panel (60+240+10)
	lv_obj_set_size(ui->scrWirelessSerial_contClearCard, 180, 95);  // Half width of config panel
	lv_obj_set_scrollbar_mode(ui->scrWirelessSerial_contClearCard, LV_SCROLLBAR_MODE_OFF);

	// Style for clear card container
	lv_obj_set_style_border_width(ui->scrWirelessSerial_contClearCard, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_contClearCard, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_contClearCard, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_contClearCard, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrWirelessSerial_contClearCard, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrWirelessSerial_contClearCard, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrWirelessSerial_contClearCard, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrWirelessSerial_contClearCard, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrWirelessSerial_contClearCard, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_contClearCard, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Send-NewLine checkbox - top half
	ui->scrWirelessSerial_checkboxSendNewLine = lv_checkbox_create(ui->scrWirelessSerial_contClearCard);
	lv_checkbox_set_text(ui->scrWirelessSerial_checkboxSendNewLine, "Send-NewLine");
	lv_obj_set_pos(ui->scrWirelessSerial_checkboxSendNewLine, 5, 5);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_checkboxSendNewLine, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_checkboxSendNewLine, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_state(ui->scrWirelessSerial_checkboxSendNewLine, LV_STATE_CHECKED);  // Default checked

	// Clear button - bottom half
	ui->scrWirelessSerial_btnClearReceive = lv_btn_create(ui->scrWirelessSerial_contClearCard);
	ui->scrWirelessSerial_btnClearReceive_label = lv_label_create(ui->scrWirelessSerial_btnClearReceive);
	lv_label_set_text(ui->scrWirelessSerial_btnClearReceive_label, "Clear");
	lv_label_set_long_mode(ui->scrWirelessSerial_btnClearReceive_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrWirelessSerial_btnClearReceive_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrWirelessSerial_btnClearReceive, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrWirelessSerial_btnClearReceive_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrWirelessSerial_btnClearReceive, 5, 40);  // Below checkbox
	lv_obj_set_size(ui->scrWirelessSerial_btnClearReceive, 150, 35);  // Full width
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_btnClearReceive, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_btnClearReceive, lv_color_hex(0xe8ecf0), LV_PART_MAIN|LV_STATE_DEFAULT);  // Light gray background
	lv_obj_set_style_bg_grad_dir(ui->scrWirelessSerial_btnClearReceive, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_btnClearReceive, 0, LV_PART_MAIN|LV_STATE_DEFAULT);  // No border
	lv_obj_set_style_radius(ui->scrWirelessSerial_btnClearReceive, 8, LV_PART_MAIN|LV_STATE_DEFAULT);  // Rounded corners
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_btnClearReceive, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_btnClearReceive, lv_color_hex(0x606060), LV_PART_MAIN|LV_STATE_DEFAULT);  // Dark gray text
	lv_obj_set_style_text_font(ui->scrWirelessSerial_btnClearReceive, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrWirelessSerial_btnClearReceive, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrWirelessSerial_btnClearReceive, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Right white card container for AT command type selector (right side of clear card)
	ui->scrWirelessSerial_contATSelector = lv_obj_create(ui->scrWirelessSerial);
	lv_obj_set_pos(ui->scrWirelessSerial_contATSelector, 605, 310);  // Right side of clear card (415+180+10)
	lv_obj_set_size(ui->scrWirelessSerial_contATSelector, 180, 95);  // Half width of config panel
	lv_obj_set_scrollbar_mode(ui->scrWirelessSerial_contATSelector, LV_SCROLLBAR_MODE_OFF);

	// Style for AT selector card container
	lv_obj_set_style_border_width(ui->scrWirelessSerial_contATSelector, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_contATSelector, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_contATSelector, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_contATSelector, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrWirelessSerial_contATSelector, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrWirelessSerial_contATSelector, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrWirelessSerial_contATSelector, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrWirelessSerial_contATSelector, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrWirelessSerial_contATSelector, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_contATSelector, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Label for AT command selector
	ui->scrWirelessSerial_labelATType = lv_label_create(ui->scrWirelessSerial_contATSelector);
	lv_label_set_text(ui->scrWirelessSerial_labelATType, "AT Type");
	lv_obj_set_pos(ui->scrWirelessSerial_labelATType, 5, 8);
	lv_obj_set_size(ui->scrWirelessSerial_labelATType, 150, 25);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_labelATType, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_labelATType, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Dropdown for AT command type selection
	ui->scrWirelessSerial_dropdownATType = lv_dropdown_create(ui->scrWirelessSerial_contATSelector);
	lv_dropdown_set_options(ui->scrWirelessSerial_dropdownATType, "Basic\nWi-Fi\nTCP/IP\nMQTT\nHTTP\nUser");
	lv_obj_set_pos(ui->scrWirelessSerial_dropdownATType, 5, 40);
	lv_obj_set_size(ui->scrWirelessSerial_dropdownATType, 150, 35);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_dropdownATType, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_dropdownATType, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrWirelessSerial_dropdownATType, lv_color_hex(0xc0c0c0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_dropdownATType, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_dropdownATType, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_dropdown_set_selected(ui->scrWirelessSerial_dropdownATType, 0);  // Default to "Basic"

	// White card container for Send area (textarea + button) - spans full width
	ui->scrWirelessSerial_contSendCard = lv_obj_create(ui->scrWirelessSerial);
	lv_obj_set_pos(ui->scrWirelessSerial_contSendCard, 15, 415);  // Position adjusted: moved down 15px (60+345+10)
	lv_obj_set_size(ui->scrWirelessSerial_contSendCard, 770, 60);  // Container spans full width (15 to 785)
	lv_obj_set_scrollbar_mode(ui->scrWirelessSerial_contSendCard, LV_SCROLLBAR_MODE_OFF);

	// Style for send area white card container
	lv_obj_set_style_border_width(ui->scrWirelessSerial_contSendCard, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_contSendCard, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_contSendCard, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_contSendCard, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrWirelessSerial_contSendCard, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrWirelessSerial_contSendCard, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrWirelessSerial_contSendCard, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrWirelessSerial_contSendCard, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrWirelessSerial_contSendCard, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_contSendCard, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrWirelessSerial_textareaSend (Send data input - inside white card)
	ui->scrWirelessSerial_textareaSend = lv_textarea_create(ui->scrWirelessSerial_contSendCard);
	lv_textarea_set_text(ui->scrWirelessSerial_textareaSend, "AT+CWLAP");
	lv_textarea_set_placeholder_text(ui->scrWirelessSerial_textareaSend, "");
	lv_textarea_set_one_line(ui->scrWirelessSerial_textareaSend, true);
	lv_obj_set_pos(ui->scrWirelessSerial_textareaSend, 5, 10);  // Position relative to container
	lv_obj_set_size(ui->scrWirelessSerial_textareaSend, 650, 40);  // Wider to span most of the container

	//Write style for scrWirelessSerial_textareaSend - light gray background, larger font
	lv_obj_set_style_text_color(ui->scrWirelessSerial_textareaSend, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrWirelessSerial_textareaSend, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);  // Larger font
	lv_obj_set_style_text_opa(ui->scrWirelessSerial_textareaSend, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrWirelessSerial_textareaSend, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrWirelessSerial_textareaSend, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_textareaSend, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_textareaSend, lv_color_hex(0xf5f5f5), LV_PART_MAIN|LV_STATE_DEFAULT);  // Light gray
	lv_obj_set_style_border_width(ui->scrWirelessSerial_textareaSend, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrWirelessSerial_textareaSend, lv_color_hex(0xe0e0e0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrWirelessSerial_textareaSend, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrWirelessSerial_textareaSend, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrWirelessSerial_textareaSend, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrWirelessSerial_textareaSend, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrWirelessSerial_textareaSend, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_textareaSend, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrWirelessSerial_btnClear (Send button - inside white card)
	ui->scrWirelessSerial_btnClear = lv_btn_create(ui->scrWirelessSerial_contSendCard);
	ui->scrWirelessSerial_btnClear_label = lv_label_create(ui->scrWirelessSerial_btnClear);
	lv_label_set_text(ui->scrWirelessSerial_btnClear_label, "Send");
	lv_label_set_long_mode(ui->scrWirelessSerial_btnClear_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrWirelessSerial_btnClear_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrWirelessSerial_btnClear, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrWirelessSerial_btnClear_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrWirelessSerial_btnClear, 665, 10);  // Position relative to container, next to textarea
	lv_obj_set_size(ui->scrWirelessSerial_btnClear, 95, 40);

	//Write style for scrWirelessSerial_btnClear (Send button) - light gray style
	lv_obj_set_style_bg_opa(ui->scrWirelessSerial_btnClear, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrWirelessSerial_btnClear, lv_color_hex(0xe8ecf0), LV_PART_MAIN|LV_STATE_DEFAULT);  // Light gray background
	lv_obj_set_style_bg_grad_dir(ui->scrWirelessSerial_btnClear, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrWirelessSerial_btnClear, 0, LV_PART_MAIN|LV_STATE_DEFAULT);  // No border
	lv_obj_set_style_radius(ui->scrWirelessSerial_btnClear, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrWirelessSerial_btnClear, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrWirelessSerial_btnClear, lv_color_hex(0x606060), LV_PART_MAIN|LV_STATE_DEFAULT);  // Dark gray text
	lv_obj_set_style_text_font(ui->scrWirelessSerial_btnClear, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrWirelessSerial_btnClear, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrWirelessSerial_btnClear, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->scrWirelessSerial);

	//Init events for screen.
	events_init_scrWirelessSerial(ui);
}

