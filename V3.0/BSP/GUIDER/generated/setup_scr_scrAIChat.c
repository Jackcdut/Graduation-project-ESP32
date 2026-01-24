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

// 声明myFont中文字库
LV_FONT_DECLARE(myFont);

void setup_scr_scrAIChat(lv_ui *ui)
{
	//Write codes scrAIChat - AI Chat Interface
	ui->scrAIChat = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrAIChat, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrAIChat, LV_SCROLLBAR_MODE_OFF);  // 禁用整体滑动
	lv_obj_clear_flag(ui->scrAIChat, LV_OBJ_FLAG_SCROLLABLE);  // 完全禁用滑动

	//Write style for scrAIChat, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrAIChat, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat, lv_color_hex(0xF3F8FE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrAIChat_contBG - Top bar with smooth transition support
	ui->scrAIChat_contBG = lv_obj_create(ui->scrAIChat);
	lv_obj_set_pos(ui->scrAIChat_contBG, 0, 0);
	lv_obj_set_size(ui->scrAIChat_contBG, 800, 105);
	lv_obj_set_scrollbar_mode(ui->scrAIChat_contBG, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrAIChat_contBG, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrAIChat_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrAIChat_contBG, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_contBG, lv_color_hex(0x2f3243), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_contBG, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrAIChat_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrAIChat_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrAIChat_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrAIChat_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrAIChat_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrAIChat_labelTitle - AI Chat Title
	ui->scrAIChat_labelTitle = lv_label_create(ui->scrAIChat);
	lv_label_set_text(ui->scrAIChat_labelTitle, "AI ASSISTANT");
	lv_label_set_long_mode(ui->scrAIChat_labelTitle, LV_LABEL_LONG_CLIP);
	lv_obj_set_pos(ui->scrAIChat_labelTitle, 150, 30);
	lv_obj_set_size(ui->scrAIChat_labelTitle, 500, 40);

	//Write style for scrAIChat_labelTitle, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrAIChat_labelTitle, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrAIChat_labelTitle, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrAIChat_labelTitle, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrAIChat_labelTitle, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrAIChat_labelTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrAIChat_labelTitle, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrAIChat_btnBack - Back button
	ui->scrAIChat_btnBack = lv_btn_create(ui->scrAIChat);
	ui->scrAIChat_btnBack_label = lv_label_create(ui->scrAIChat_btnBack);
	lv_label_set_text(ui->scrAIChat_btnBack_label, "<");
	lv_label_set_long_mode(ui->scrAIChat_btnBack_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrAIChat_btnBack_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrAIChat_btnBack, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrAIChat_btnBack_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrAIChat_btnBack, 20, 25);
	lv_obj_set_size(ui->scrAIChat_btnBack, 50, 50);

	//Write style for scrAIChat_btnBack, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrAIChat_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrAIChat_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_btnBack, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrAIChat_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrAIChat_btnBack, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrAIChat_btnBack, &lv_font_montserratMedium_41, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrAIChat_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrAIChat_btnBack, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrAIChat_contChatArea - Scrollable chat message area (LEFT CARD)
	// 【左侧卡片】聊天对话区域 - 白色背景的对话框，保留上下滑动
	ui->scrAIChat_contChatArea = lv_obj_create(ui->scrAIChat);
	// 【位置设置】左侧聊天卡片位置 - 居中对齐
	// x=20: 距离屏幕左边缘20像素
	// y=110: 距离屏幕顶部110像素（标题下方）
	lv_obj_set_pos(ui->scrAIChat_contChatArea, 20, 80);
	// 【大小设置】左侧聊天卡片尺寸
	// 宽度=460px: 聊天区域宽度
	// 高度=350px: 聊天区域高度
	lv_obj_set_size(ui->scrAIChat_contChatArea, 460, 380);
	lv_obj_set_scrollbar_mode(ui->scrAIChat_contChatArea, LV_SCROLLBAR_MODE_AUTO);  // 保留滚动条
	lv_obj_set_scroll_dir(ui->scrAIChat_contChatArea, LV_DIR_VER);  // 只允许垂直滑动

	//Write style for scrAIChat_contChatArea, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrAIChat_contChatArea, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrAIChat_contChatArea, lv_color_hex(0xdfe6e9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->scrAIChat_contChatArea, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_contChatArea, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrAIChat_contChatArea, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_contChatArea, lv_color_hex(0xF5F5F5), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅灰色背景
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_contChatArea, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrAIChat_contChatArea, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrAIChat_contChatArea, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrAIChat_contChatArea, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrAIChat_contChatArea, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrAIChat_contChatArea, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrAIChat_contChatArea, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_contChatArea, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrAIChat_contChatArea, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrAIChat_contChatArea, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_contChatArea, 3, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Scrollbar style
	lv_obj_set_style_bg_opa(ui->scrAIChat_contChatArea, 100, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_contChatArea, lv_color_hex(0xbdc3c7), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_contChatArea, 5, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Write codes scrAIChat_contInputCard - Input control card (RIGHT CARD)
	// 【右侧卡片】输入控制区域 - 白色背景卡片，禁用滑动
	ui->scrAIChat_contInputCard = lv_obj_create(ui->scrAIChat);
	// 【位置设置】右侧输入卡片位置 - 居中对齐
	// x=490: 距离屏幕左边缘490像素（左侧卡片20+460+10间距）
	// y=110: 距离屏幕顶部110像素（与左侧卡片对齐）
	lv_obj_set_pos(ui->scrAIChat_contInputCard, 490, 80);
	// 【大小设置】右侧输入卡片尺寸
	// 宽度=290px, 高度=350px: 与左侧卡片等高
	lv_obj_set_size(ui->scrAIChat_contInputCard, 290, 380);
	lv_obj_set_scrollbar_mode(ui->scrAIChat_contInputCard, LV_SCROLLBAR_MODE_OFF);  // 禁用滚动条
	lv_obj_clear_flag(ui->scrAIChat_contInputCard, LV_OBJ_FLAG_SCROLLABLE);  // 完全禁用滑动
	
	//Write style for scrAIChat_contInputCard, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrAIChat_contInputCard, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrAIChat_contInputCard, lv_color_hex(0xdfe6e9), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->scrAIChat_contInputCard, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_contInputCard, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrAIChat_contInputCard, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_contInputCard, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_contInputCard, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrAIChat_contInputCard, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrAIChat_contInputCard, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrAIChat_contInputCard, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrAIChat_contInputCard, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrAIChat_contInputCard, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrAIChat_contInputCard, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_contInputCard, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrAIChat_contInputCard, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrAIChat_contInputCard, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_contInputCard, 3, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrAIChat_textAreaInput - Text input area (INSIDE RIGHT CARD)
	// 【文本输入框】位于右侧卡片内，支持语音转文字或直接打字输入
	ui->scrAIChat_textAreaInput = lv_textarea_create(ui->scrAIChat_contInputCard);
	lv_textarea_set_placeholder_text(ui->scrAIChat_textAreaInput, "Type or speak...");  // 占位符文本
	lv_textarea_set_one_line(ui->scrAIChat_textAreaInput, false);  // 多行输入
	lv_textarea_set_max_length(ui->scrAIChat_textAreaInput, 500);  // 最大输入500个字符
	// 【位置设置】输入框位置（相对于右侧卡片）
	// x=0: 距离卡片左边缘0像素（由padding控制）
	// y=0: 距离卡片顶部0像素（由padding控制）
	lv_obj_set_pos(ui->scrAIChat_textAreaInput, 0, 10);
	// 【大小设置】输入框大小（增大高度，减少底部空白）
	// 宽度=260px: 卡片宽度(290) - padding左右(15*2)
	// 高度=280px: 增大输入框高度
	lv_obj_set_size(ui->scrAIChat_textAreaInput, 260, 280);

	//Write style for scrAIChat_textAreaInput, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrAIChat_textAreaInput, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_textAreaInput, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrAIChat_textAreaInput, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrAIChat_textAreaInput, lv_color_hex(0xbdc3c7), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->scrAIChat_textAreaInput, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_textAreaInput, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrAIChat_textAreaInput, lv_color_hex(0x2c3e50), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrAIChat_textAreaInput, &myFont, LV_PART_MAIN|LV_STATE_DEFAULT);  // 使用myFont中文字库
	lv_obj_set_style_text_opa(ui->scrAIChat_textAreaInput, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrAIChat_textAreaInput, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrAIChat_textAreaInput, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrAIChat_textAreaInput, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrAIChat_textAreaInput, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrAIChat_textAreaInput, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrAIChat_btnVoiceInput - Voice input button (INSIDE RIGHT CARD, BOTTOM LEFT)
	// 【语音输入按钮】位于右侧卡片底部，按住说话，松开停止
	ui->scrAIChat_btnVoiceInput = lv_btn_create(ui->scrAIChat_contInputCard);
	ui->scrAIChat_btnVoiceInput_label = lv_label_create(ui->scrAIChat_btnVoiceInput);
	lv_label_set_text(ui->scrAIChat_btnVoiceInput_label, "MIC");
	lv_label_set_long_mode(ui->scrAIChat_btnVoiceInput_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrAIChat_btnVoiceInput_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrAIChat_btnVoiceInput, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrAIChat_btnVoiceInput_label, LV_PCT(100));
	// 【位置设置】语音按钮位置（向下移动，减少底部空白）
	// x=5: 距离卡片左边缘5像素
	// y=305: 向下移动到305像素（原265 + 40）
	lv_obj_set_pos(ui->scrAIChat_btnVoiceInput, 5, 305);
	// 【大小设置】语音按钮大小
	// 宽度=85px, 高度=50px: 按钮尺寸稍微增大
	lv_obj_set_size(ui->scrAIChat_btnVoiceInput, 85, 50);

	//Write style for scrAIChat_btnVoiceInput, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	// 语音按钮配色：浅绿色 - 清新自然，表示可用状态
	lv_obj_set_style_bg_opa(ui->scrAIChat_btnVoiceInput, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_btnVoiceInput, lv_color_hex(0x81C784), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅绿色
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_btnVoiceInput, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);  // 无渐变
	lv_obj_set_style_border_width(ui->scrAIChat_btnVoiceInput, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_btnVoiceInput, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	// 添加阴影效果 - 增强立体感
	lv_obj_set_style_shadow_width(ui->scrAIChat_btnVoiceInput, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrAIChat_btnVoiceInput, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_btnVoiceInput, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrAIChat_btnVoiceInput, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrAIChat_btnVoiceInput, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_btnVoiceInput, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrAIChat_btnVoiceInput, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrAIChat_btnVoiceInput, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrAIChat_btnVoiceInput, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrAIChat_btnVoiceInput, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrAIChat_btnVoiceInput, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	// 按下时变为鲜艳的红色，模拟录音状态，提供强烈的视觉反馈
	lv_obj_set_style_bg_opa(ui->scrAIChat_btnVoiceInput, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_bg_color(ui->scrAIChat_btnVoiceInput, lv_color_hex(0xEF5350), LV_PART_MAIN|LV_STATE_PRESSED);  // 鲜艳红色（录音中）
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_btnVoiceInput, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_PRESSED);  // 无渐变
	// 按下时阴影减弱，模拟按下效果
	lv_obj_set_style_shadow_width(ui->scrAIChat_btnVoiceInput, 3, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_btnVoiceInput, 40, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_btnVoiceInput, 1, LV_PART_MAIN|LV_STATE_PRESSED);
	// 按下时稍微缩小，模拟按下效果
	lv_obj_set_style_transform_width(ui->scrAIChat_btnVoiceInput, -3, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_transform_height(ui->scrAIChat_btnVoiceInput, -3, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write codes scrAIChat_btnDelete - Delete button (INSIDE RIGHT CARD, BOTTOM MIDDLE)
	// 【删除按钮】位于右侧卡片底部中间，每按一下删除一个字符
	ui->scrAIChat_btnDelete = lv_btn_create(ui->scrAIChat_contInputCard);
	ui->scrAIChat_btnDelete_label = lv_label_create(ui->scrAIChat_btnDelete);
	lv_label_set_text(ui->scrAIChat_btnDelete_label, "DEL");
	lv_label_set_long_mode(ui->scrAIChat_btnDelete_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrAIChat_btnDelete_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrAIChat_btnDelete, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrAIChat_btnDelete_label, LV_PCT(100));
	// 【位置设置】删除按钮位置（向下移动）
	// x=98: 距离卡片左边缘98像素（MIC按钮右侧）
	// y=305: 向下移动到305像素（原265 + 40）
	lv_obj_set_pos(ui->scrAIChat_btnDelete, 98, 305);
	// 【大小设置】删除按钮大小
	// 宽度=80px, 高度=50px: 按钮尺寸稍微增大
	lv_obj_set_size(ui->scrAIChat_btnDelete, 80, 50);

	//Write style for scrAIChat_btnDelete, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	// 删除按钮配色：浅珊瑚橙 - 温和警示
	lv_obj_set_style_bg_opa(ui->scrAIChat_btnDelete, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_btnDelete, lv_color_hex(0xFFAB91), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅珊瑚橙
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_btnDelete, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);  // 无渐变
	lv_obj_set_style_border_width(ui->scrAIChat_btnDelete, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_btnDelete, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	// 添加阴影效果 - 增强立体感
	lv_obj_set_style_shadow_width(ui->scrAIChat_btnDelete, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrAIChat_btnDelete, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_btnDelete, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrAIChat_btnDelete, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrAIChat_btnDelete, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_btnDelete, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrAIChat_btnDelete, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrAIChat_btnDelete, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrAIChat_btnDelete, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrAIChat_btnDelete, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrAIChat_btnDelete, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	// 按下时变为深珊瑚橙
	lv_obj_set_style_bg_opa(ui->scrAIChat_btnDelete, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_bg_color(ui->scrAIChat_btnDelete, lv_color_hex(0xFF8A65), LV_PART_MAIN|LV_STATE_PRESSED);  // 深珊瑚橙
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_btnDelete, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_PRESSED);  // 无渐变
	// 按下时阴影减弱，模拟按下效果
	lv_obj_set_style_shadow_width(ui->scrAIChat_btnDelete, 3, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_btnDelete, 40, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_btnDelete, 1, LV_PART_MAIN|LV_STATE_PRESSED);
	// 按下时稍微缩小，模拟按下效果
	lv_obj_set_style_transform_width(ui->scrAIChat_btnDelete, -3, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_transform_height(ui->scrAIChat_btnDelete, -3, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write codes scrAIChat_btnSend - Send button (INSIDE RIGHT CARD, BOTTOM RIGHT)
	// 【发送按钮】位于右侧卡片底部右侧，点击发送消息
	ui->scrAIChat_btnSend = lv_btn_create(ui->scrAIChat_contInputCard);
	ui->scrAIChat_btnSend_label = lv_label_create(ui->scrAIChat_btnSend);
	lv_label_set_text(ui->scrAIChat_btnSend_label, "SEND");
	lv_label_set_long_mode(ui->scrAIChat_btnSend_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrAIChat_btnSend_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrAIChat_btnSend, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrAIChat_btnSend_label, LV_PCT(100));
	// 【位置设置】发送按钮位置（向下移动）
	// x=186: 距离卡片左边缘186像素（DEL按钮右侧）
	// y=305: 向下移动到305像素（原265 + 40）
	lv_obj_set_pos(ui->scrAIChat_btnSend, 186, 305);
	// 【大小设置】发送按钮大小
	// 宽度=75px, 高度=50px: 按钮尺寸稍微增大
	lv_obj_set_size(ui->scrAIChat_btnSend, 75, 50);

	//Write style for scrAIChat_btnSend, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	// 发送按钮配色：浅天蓝色 - 清新活力
	lv_obj_set_style_bg_opa(ui->scrAIChat_btnSend, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrAIChat_btnSend, lv_color_hex(0x81D4FA), LV_PART_MAIN|LV_STATE_DEFAULT);  // 浅天蓝色
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_btnSend, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);  // 无渐变
	lv_obj_set_style_border_width(ui->scrAIChat_btnSend, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_btnSend, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	// 添加阴影效果 - 增强立体感
	lv_obj_set_style_shadow_width(ui->scrAIChat_btnSend, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui->scrAIChat_btnSend, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_btnSend, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui->scrAIChat_btnSend, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(ui->scrAIChat_btnSend, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_btnSend, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrAIChat_btnSend, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrAIChat_btnSend, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrAIChat_btnSend, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrAIChat_btnSend, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for scrAIChat_btnSend, Part: LV_PART_MAIN, State: LV_STATE_PRESSED.
	// 按下时变为深天蓝色
	lv_obj_set_style_bg_opa(ui->scrAIChat_btnSend, 255, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_bg_color(ui->scrAIChat_btnSend, lv_color_hex(0x4FC3F7), LV_PART_MAIN|LV_STATE_PRESSED);  // 深天蓝色
	lv_obj_set_style_bg_grad_dir(ui->scrAIChat_btnSend, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_PRESSED);  // 无渐变
	// 按下时阴影减弱，模拟按下效果
	lv_obj_set_style_shadow_width(ui->scrAIChat_btnSend, 3, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_opa(ui->scrAIChat_btnSend, 40, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_shadow_ofs_y(ui->scrAIChat_btnSend, 1, LV_PART_MAIN|LV_STATE_PRESSED);
	// 按下时稍微缩小，模拟按下效果
	lv_obj_set_style_transform_width(ui->scrAIChat_btnSend, -3, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_transform_height(ui->scrAIChat_btnSend, -3, LV_PART_MAIN|LV_STATE_PRESSED);

	//Write codes scrAIChat_labelStatus - Status label (hidden initially)
	ui->scrAIChat_labelStatus = lv_label_create(ui->scrAIChat);
	lv_label_set_text(ui->scrAIChat_labelStatus, "");
	lv_label_set_long_mode(ui->scrAIChat_labelStatus, LV_LABEL_LONG_CLIP);
	lv_obj_set_pos(ui->scrAIChat_labelStatus, 300, 467);
	lv_obj_set_size(ui->scrAIChat_labelStatus, 200, 30);

	//Write style for scrAIChat_labelStatus, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrAIChat_labelStatus, lv_color_hex(0x7f8c8d), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrAIChat_labelStatus, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrAIChat_labelStatus, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrAIChat_labelStatus, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrAIChat_labelStatus, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->scrAIChat);

	//Init events for screen.
	events_init_scrAIChat(ui);
}

