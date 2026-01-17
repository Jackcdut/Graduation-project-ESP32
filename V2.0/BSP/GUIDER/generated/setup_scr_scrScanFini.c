
#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_scrScanFini(lv_ui *ui)
{
	//Write codes scrScanFini
	ui->scrScanFini = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrScanFini, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrScanFini, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrScanFini, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrScanFini, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrScanFini, lv_color_hex(0xF3F8FE), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrScanFini, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrScanFini_contBG
	ui->scrScanFini_contBG = lv_obj_create(ui->scrScanFini);
	lv_obj_set_pos(ui->scrScanFini_contBG, 0, 0);
	lv_obj_set_size(ui->scrScanFini_contBG, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrScanFini_contBG, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrScanFini_contBG, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrScanFini_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrScanFini_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrScanFini_contBG, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrScanFini_contBG, lv_color_hex(0x28b620), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrScanFini_contBG, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrScanFini_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrScanFini_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrScanFini_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrScanFini_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrScanFini_contBG, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrScanFini_imgIconOk
	ui->scrScanFini_imgIconOk = lv_img_create(ui->scrScanFini);
	lv_obj_add_flag(ui->scrScanFini_imgIconOk, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->scrScanFini_imgIconOk, &_ready_alpha_183x183);
	lv_img_set_pivot(ui->scrScanFini_imgIconOk, 55,55);
	lv_img_set_angle(ui->scrScanFini_imgIconOk, 0);
	lv_obj_set_pos(ui->scrScanFini_imgIconOk, 308, 63);
	lv_obj_set_size(ui->scrScanFini_imgIconOk, 183, 183);

	//Write style for scrScanFini_imgIconOk, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->scrScanFini_imgIconOk, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrScanFini_labelPrompt
	ui->scrScanFini_labelPrompt = lv_label_create(ui->scrScanFini);
	lv_label_set_text(ui->scrScanFini_labelPrompt, "WiFi Connected Successfully");
	lv_label_set_long_mode(ui->scrScanFini_labelPrompt, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->scrScanFini_labelPrompt, 121, 296);
	lv_obj_set_size(ui->scrScanFini_labelPrompt, 558, 56);

	//Write style for scrScanFini_labelPrompt, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrScanFini_labelPrompt, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrScanFini_labelPrompt, &lv_font_montserratMedium_33, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrScanFini_labelPrompt, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->scrScanFini_labelPrompt, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrScanFini_labelPrompt, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrScanFini_labelPrompt, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrScanFini_btnNxet
	ui->scrScanFini_btnNxet = lv_btn_create(ui->scrScanFini);
	ui->scrScanFini_btnNxet_label = lv_label_create(ui->scrScanFini_btnNxet);
	lv_label_set_text(ui->scrScanFini_btnNxet_label, "CONTINUE");
	lv_label_set_long_mode(ui->scrScanFini_btnNxet_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrScanFini_btnNxet_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrScanFini_btnNxet, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrScanFini_btnNxet_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrScanFini_btnNxet, 270, 372);
	lv_obj_set_size(ui->scrScanFini_btnNxet, 260, 68);

	//Write style for scrScanFini_btnNxet, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->scrScanFini_btnNxet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrScanFini_btnNxet, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->scrScanFini_btnNxet, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrScanFini_btnNxet, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(ui->scrScanFini_btnNxet, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrScanFini_btnNxet, 33, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrScanFini_btnNxet, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrScanFini_btnNxet, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrScanFini_btnNxet, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrScanFini_btnNxet, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrScanFini_btnNxet, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//The custom code of scrScanFini.
	

	//Update current screen layout.
	lv_obj_update_layout(ui->scrScanFini);

	//Init events for screen.
	events_init_scrScanFini(ui);
}
