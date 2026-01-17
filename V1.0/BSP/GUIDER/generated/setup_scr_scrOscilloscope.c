/*
* Copyright 2025 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/*
* ============================================================================
* OSCILLOSCOPE SCREEN - Professional Single Channel Digital Oscilloscope
* ============================================================================
*
* LAYOUT: 800x480 screen with top toolbar (20px), waveform area (720x410px),
*         bottom measurements (20px), and right control panel (65px width)
*
* FEATURES:
* - Single channel (CHA) yellow waveform display
* - Grid lines, center lines, coordinate axes, trigger level indicator
* - RUN/STOP, PAN gesture, Waveform preview, FFT analysis, Data export
* - 5 measurements: Freq, Vmax, Vmin, Vp-p, Vrms (or FFT mode)
* - 8 controls: Channel, Time scale (1us-1s), Voltage (10mV-12V),
*   X/Y position, Trigger voltage, Coupling (AC/DC), Trigger mode
* ============================================================================
*/

#include "lvgl.h"
#include <stdio.h>
#include <math.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

// Helper function to create measurement display (bottom measurement - height 20px, larger font)
static void create_measurement_display(lv_ui *ui, lv_obj_t *parent, lv_obj_t **cont, lv_obj_t **label_title, lv_obj_t **label_value,
                                 int x, const char *title, const char *value, uint32_t color) {
	*cont = lv_obj_create(parent);
	lv_obj_set_pos(*cont, x, 0);
	lv_obj_set_size(*cont, 140, 20);  // Fixed height 20px
	lv_obj_set_scrollbar_mode(*cont, LV_SCROLLBAR_MODE_OFF);

	// Container style - Colored background with small rounded corners
	lv_obj_set_style_border_width(*cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(*cont, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(*cont, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(*cont, lv_color_hex(color), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(*cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(*cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Create a single label for combined title and value (centered)
	*label_title = lv_label_create(*cont);
	char combined_text[64];
	snprintf(combined_text, sizeof(combined_text), "%s%s", title, value);  // No space between
	lv_label_set_text(*label_title, combined_text);
	lv_obj_set_size(*label_title, 140, 20);
	lv_obj_set_style_text_color(*label_title, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(*label_title, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(*label_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(*label_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(*label_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_align(*label_title, LV_ALIGN_CENTER, 0, 0);

	// Store label_title pointer in label_value as well for compatibility
	*label_value = *label_title;
}

// Helper function to create control panel item (right side controls - width 65px, height 50px, larger font)
static void create_control_panel(lv_ui *ui, lv_obj_t *parent, lv_obj_t **cont, lv_obj_t **label_title, lv_obj_t **label_value,
                          int y_offset, const char *title, const char *value, uint32_t color) {
	*cont = lv_obj_create(parent);
	lv_obj_set_pos(*cont, 0, y_offset);
	lv_obj_set_size(*cont, 65, 50);  // Width 65px, Height 50px (increased from 40)
	lv_obj_set_scrollbar_mode(*cont, LV_SCROLLBAR_MODE_OFF);

	// Container style - Black background, colored border, rounded corners
	lv_obj_set_style_border_width(*cont, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(*cont, lv_color_hex(color), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(*cont, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(*cont, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(*cont, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(*cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(*cont, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Title label (top part) - Colored background, height 25px, larger font, only top corners rounded
	*label_title = lv_label_create(*cont);
	lv_label_set_text(*label_title, title);
	lv_obj_set_pos(*label_title, 0, 0);
	lv_obj_set_size(*label_title, 65, 25);  // Increased from 20 to 25
	lv_obj_set_style_text_color(*label_title, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(*label_title, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(*label_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(*label_title, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(*label_title, lv_color_hex(color), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(*label_title, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(*label_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(*label_title, 3, LV_PART_MAIN|LV_STATE_DEFAULT);  // Center vertically
	lv_obj_align(*label_title, LV_ALIGN_TOP_MID, 0, 0);

	// Value label (bottom part) - White text on black background, height 25px, larger font
	*label_value = lv_label_create(*cont);
	lv_label_set_text(*label_value, value);
	lv_obj_set_pos(*label_value, 0, 25);  // Position after title (25px down)
	lv_obj_set_size(*label_value, 65, 25);  // Increased from 20 to 25
	lv_obj_set_style_text_color(*label_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(*label_value, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(*label_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(*label_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(*label_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(*label_value, 5, LV_PART_MAIN|LV_STATE_DEFAULT);  // Center vertically (increased from 3 to 5, +2 units)
	lv_obj_align(*label_value, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void setup_scr_scrOscilloscope(lv_ui *ui)
{
	//Write codes scrOscilloscope - Professional Oscilloscope Interface
	ui->scrOscilloscope = lv_obj_create(NULL);
	lv_obj_set_size(ui->scrOscilloscope, 800, 480);
	lv_obj_set_scrollbar_mode(ui->scrOscilloscope, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrOscilloscope - Black background
	lv_obj_set_style_bg_opa(ui->scrOscilloscope, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrOscilloscope, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_btnBack - Back button (same style as other pages, moved down 10px from -15)
	ui->scrOscilloscope_btnBack = lv_btn_create(ui->scrOscilloscope);
	ui->scrOscilloscope_btnBack_label = lv_label_create(ui->scrOscilloscope_btnBack);
	lv_label_set_text(ui->scrOscilloscope_btnBack_label, "<");
	lv_label_set_long_mode(ui->scrOscilloscope_btnBack_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrOscilloscope_btnBack_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_btnBack, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrOscilloscope_btnBack_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrOscilloscope_btnBack, 10, -5);  // Moved down 10px (from -15 to -5)
	lv_obj_set_size(ui->scrOscilloscope_btnBack, 40, 40);

	//Write style for scrOscilloscope_btnBack - transparent background, white text, no border
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_btnBack, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnBack, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrOscilloscope_btnBack, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrOscilloscope_btnBack, &lv_font_montserratMedium_26, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrOscilloscope_btnBack, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrOscilloscope_btnBack, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_btnAuto - AUTO button (aligned with EXPORT button top)
	ui->scrOscilloscope_btnAuto = lv_btn_create(ui->scrOscilloscope);
	ui->scrOscilloscope_btnAuto_label = lv_label_create(ui->scrOscilloscope_btnAuto);
	lv_label_set_text(ui->scrOscilloscope_btnAuto_label, "AUTO");
	lv_label_set_long_mode(ui->scrOscilloscope_btnAuto_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(ui->scrOscilloscope_btnAuto_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_btnAuto, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrOscilloscope_btnAuto_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrOscilloscope_btnAuto, 735, 0);  // X=735 (aligned with CH), Y=0 (aligned with EXPORT top)
	lv_obj_set_size(ui->scrOscilloscope_btnAuto, 65, 40);  // Height 40px

	//Write style for scrOscilloscope_btnAuto
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_btnAuto, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_btnAuto, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);  // Cyan color
	lv_obj_set_style_border_width(ui->scrOscilloscope_btnAuto, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrOscilloscope_btnAuto, lv_color_hex(0x00FFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_btnAuto, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnAuto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrOscilloscope_btnAuto, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrOscilloscope_btnAuto, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrOscilloscope_btnAuto, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrOscilloscope_btnAuto, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === TOP TOOLBAR (Height 30px) ===
	// Contains: RUN/STOP button, PAN button, Waveform position preview, FFT button, EXPORT button

	// Create horizontal scrollable container for top buttons
	lv_obj_t *contTopButtons = lv_obj_create(ui->scrOscilloscope);
	lv_obj_set_pos(contTopButtons, 55, 0);  // Start after back button (50 + 5 spacing)
	lv_obj_set_size(contTopButtons, 665, 30);  // Width: 720 - 55 = 665, Height: 30px
	lv_obj_set_scrollbar_mode(contTopButtons, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_scroll_dir(contTopButtons, LV_DIR_HOR);

	// Container style - transparent background
	lv_obj_set_style_border_width(contTopButtons, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(contTopButtons, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(contTopButtons, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(contTopButtons, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(contTopButtons, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_btnStartStop - Start/Stop button (Green)
	// Function: Toggle between RUN and STOP states to control waveform acquisition
	ui->scrOscilloscope_btnStartStop = lv_btn_create(contTopButtons);
	ui->scrOscilloscope_btnStartStop_label = lv_label_create(ui->scrOscilloscope_btnStartStop);
	lv_label_set_text(ui->scrOscilloscope_btnStartStop_label, "RUN");
	lv_label_set_long_mode(ui->scrOscilloscope_btnStartStop_label, LV_LABEL_LONG_CLIP);
	lv_obj_align(ui->scrOscilloscope_btnStartStop_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_btnStartStop, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrOscilloscope_btnStartStop_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrOscilloscope_btnStartStop, 0, 0);
	lv_obj_set_size(ui->scrOscilloscope_btnStartStop, 70, 30);

	//Write style for scrOscilloscope_btnStartStop
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_btnStartStop, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_btnStartStop, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_btnStartStop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_btnStartStop, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnStartStop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrOscilloscope_btnStartStop, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrOscilloscope_btnStartStop, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrOscilloscope_btnStartStop, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrOscilloscope_btnStartStop, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_btnPanZoom - GRID button (Blue)
	// Function: Toggle grid display on/off
	ui->scrOscilloscope_btnPanZoom = lv_btn_create(contTopButtons);
	ui->scrOscilloscope_btnPanZoom_label = lv_label_create(ui->scrOscilloscope_btnPanZoom);
	lv_label_set_text(ui->scrOscilloscope_btnPanZoom_label, "GRID");
	lv_label_set_long_mode(ui->scrOscilloscope_btnPanZoom_label, LV_LABEL_LONG_CLIP);
	lv_obj_align(ui->scrOscilloscope_btnPanZoom_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_btnPanZoom, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrOscilloscope_btnPanZoom_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrOscilloscope_btnPanZoom, 75, 0);
	lv_obj_set_size(ui->scrOscilloscope_btnPanZoom, 70, 30);

	//Write style for scrOscilloscope_btnPanZoom - Bright blue when grid is ON
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_btnPanZoom, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_btnPanZoom, lv_color_hex(0x0080FF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_btnPanZoom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_btnPanZoom, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnPanZoom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrOscilloscope_btnPanZoom, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrOscilloscope_btnPanZoom, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrOscilloscope_btnPanZoom, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrOscilloscope_btnPanZoom, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_sliderWavePos - Container for waveform position preview
	// Function: Shows full waveform with blue mask indicating currently visible portion on screen
	// The blue overlay represents which part of the total waveform data is displayed
	ui->scrOscilloscope_sliderWavePos = lv_obj_create(contTopButtons);
	lv_obj_set_pos(ui->scrOscilloscope_sliderWavePos, 150, 0);
	lv_obj_set_size(ui->scrOscilloscope_sliderWavePos, 300, 30);
	lv_obj_set_scrollbar_mode(ui->scrOscilloscope_sliderWavePos, LV_SCROLLBAR_MODE_OFF);

	//Write style for waveform preview container - Dark background
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_sliderWavePos, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_sliderWavePos, lv_color_hex(0x202020), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_sliderWavePos, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrOscilloscope_sliderWavePos, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_sliderWavePos, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_sliderWavePos, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_sliderWavePos, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Add mini chart to show full waveform preview
	lv_obj_t *chartPreview = lv_chart_create(ui->scrOscilloscope_sliderWavePos);
	lv_chart_set_type(chartPreview, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(chartPreview, 0, 0);  // No grid lines
	lv_chart_set_point_count(chartPreview, 100);
	lv_obj_set_pos(chartPreview, 0, 0);
	lv_obj_set_size(chartPreview, 296, 26);  // Increased height to match container
	lv_obj_set_scrollbar_mode(chartPreview, LV_SCROLLBAR_MODE_OFF);

	// Style for preview chart
	lv_obj_set_style_bg_opa(chartPreview, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(chartPreview, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(chartPreview, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(chartPreview, 1, LV_PART_ITEMS|LV_STATE_DEFAULT);
	lv_obj_set_style_size(chartPreview, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(chartPreview, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Add waveform series to preview - increased frequency for more visible cycles
	lv_chart_series_t * serPreview = lv_chart_add_series(chartPreview, lv_color_hex(0xFFFF00), LV_CHART_AXIS_PRIMARY_Y);
	for(int i = 0; i < 100; i++) {
		serPreview->y_points[i] = 50 + (int)(40 * sin(i * 0.4));  // Higher frequency - more cycles
	}
	lv_chart_refresh(chartPreview);

	// Add blue mask overlay to show visible portion (30-70% of waveform)
	ui->scrOscilloscope_sliderWaveMask = lv_obj_create(ui->scrOscilloscope_sliderWavePos);
	lv_obj_set_pos(ui->scrOscilloscope_sliderWaveMask, 90, 0);  // Start at 30% position
	lv_obj_set_size(ui->scrOscilloscope_sliderWaveMask, 120, 26);  // 40% width, match height
	lv_obj_set_scrollbar_mode(ui->scrOscilloscope_sliderWaveMask, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_sliderWaveMask, 100, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_sliderWaveMask, lv_color_hex(0x0000FF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_sliderWaveMask, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->scrOscilloscope_sliderWaveMask, lv_color_hex(0x0080FF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_sliderWaveMask, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_sliderWaveMask, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_btnFFT - FFT button (Yellow)
	// Function: Toggle FFT analysis mode
	// When enabled: Waveform shows frequency spectrum, measurements change to:
	//   Freq, Fundamental Freq, Harmonics (1-9), THD (Total Harmonic Distortion)
	ui->scrOscilloscope_btnFFT = lv_btn_create(contTopButtons);
	ui->scrOscilloscope_btnFFT_label = lv_label_create(ui->scrOscilloscope_btnFFT);
	lv_label_set_text(ui->scrOscilloscope_btnFFT_label, "FFT");
	lv_label_set_long_mode(ui->scrOscilloscope_btnFFT_label, LV_LABEL_LONG_CLIP);
	lv_obj_align(ui->scrOscilloscope_btnFFT_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_btnFFT, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrOscilloscope_btnFFT_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrOscilloscope_btnFFT, 455, 0);
	lv_obj_set_size(ui->scrOscilloscope_btnFFT, 70, 30);

	//Write style for scrOscilloscope_btnFFT
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_btnFFT, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_btnFFT, lv_color_hex(0xFFFF00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_btnFFT, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_btnFFT, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnFFT, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrOscilloscope_btnFFT, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrOscilloscope_btnFFT, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrOscilloscope_btnFFT, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrOscilloscope_btnFFT, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_btnExport - Export button (Orange)
	// Function: Enable USB mass storage mode to export waveform data as .tst file
	// Exports all waveform data, and FFT data if FFT mode is enabled
	ui->scrOscilloscope_btnExport = lv_btn_create(contTopButtons);
	ui->scrOscilloscope_btnExport_label = lv_label_create(ui->scrOscilloscope_btnExport);
	lv_label_set_text(ui->scrOscilloscope_btnExport_label, "EXPORT");
	lv_label_set_long_mode(ui->scrOscilloscope_btnExport_label, LV_LABEL_LONG_CLIP);
	lv_obj_align(ui->scrOscilloscope_btnExport_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_btnExport, 0, LV_STATE_DEFAULT);
	lv_obj_set_width(ui->scrOscilloscope_btnExport_label, LV_PCT(100));
	lv_obj_set_pos(ui->scrOscilloscope_btnExport, 530, 0);
	lv_obj_set_size(ui->scrOscilloscope_btnExport, 135, 30);  // Extend to right edge (665-530=135)

	//Write style for scrOscilloscope_btnExport
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_btnExport, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_btnExport, lv_color_hex(0xFF8000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_btnExport, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_btnExport, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnExport, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->scrOscilloscope_btnExport, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->scrOscilloscope_btnExport, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->scrOscilloscope_btnExport, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->scrOscilloscope_btnExport, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === WAVEFORM DISPLAY AREA (Fixed size: 720x400px) ===

	//Write codes scrOscilloscope_contWaveform - Waveform container (fixed, non-scrollable)
	ui->scrOscilloscope_contWaveform = lv_obj_create(ui->scrOscilloscope);
	lv_obj_set_pos(ui->scrOscilloscope_contWaveform, 0, 45);  // 30 + 15 = 45
	lv_obj_set_size(ui->scrOscilloscope_contWaveform, 720, 400);  // Fixed display size
	lv_obj_set_scrollbar_mode(ui->scrOscilloscope_contWaveform, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(ui->scrOscilloscope_contWaveform, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling

	//Write style for scrOscilloscope_contWaveform - Black background, square corners
	lv_obj_set_style_border_width(ui->scrOscilloscope_contWaveform, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_contWaveform, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_contWaveform, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_contWaveform, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->scrOscilloscope_contWaveform, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_contWaveform, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->scrOscilloscope_contWaveform, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes scrOscilloscope_chartWaveform - Chart for waveform display (Single channel)
	// Fixed size chart matching container - professional oscilloscope style
	// Grid: 18 horizontal divisions x 10 vertical divisions
	// Each division: 40x40 pixels (perfect square grid)
	// Center axes are positioned ON grid lines (at division 9 horizontal, division 5 vertical)
	ui->scrOscilloscope_chartWaveform = lv_chart_create(ui->scrOscilloscope_contWaveform);
	lv_chart_set_type(ui->scrOscilloscope_chartWaveform, LV_CHART_TYPE_LINE);
	// lv_chart_set_div_line_count(hdiv, vdiv): hdiv=horizontal lines, vdiv=vertical lines
	// For 720x400 with 40x40 grid: 18 horizontal divisions (need 10 horizontal lines), 10 vertical divisions (need 18 vertical lines)
	lv_chart_set_div_line_count(ui->scrOscilloscope_chartWaveform, 10, 18);
	lv_chart_set_point_count(ui->scrOscilloscope_chartWaveform, 720);  // One point per horizontal pixel
	lv_obj_set_pos(ui->scrOscilloscope_chartWaveform, 0, 0);
	lv_obj_set_size(ui->scrOscilloscope_chartWaveform, 720, 400);  // Fixed size matching container
	lv_obj_set_scrollbar_mode(ui->scrOscilloscope_chartWaveform, LV_SCROLLBAR_MODE_OFF);

	//Write style for scrOscilloscope_chartWaveform - Black background
	lv_obj_set_style_bg_opa(ui->scrOscilloscope_chartWaveform, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->scrOscilloscope_chartWaveform, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->scrOscilloscope_chartWaveform, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->scrOscilloscope_chartWaveform, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui->scrOscilloscope_chartWaveform, 3, LV_PART_ITEMS|LV_STATE_DEFAULT);  // Waveform line width: 3 pixels
	lv_obj_set_style_size(ui->scrOscilloscope_chartWaveform, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->scrOscilloscope_chartWaveform, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Grid lines style - Gray dashed lines (will be controlled by GRID button)
	lv_obj_set_style_line_dash_width(ui->scrOscilloscope_chartWaveform, 3, LV_PART_MAIN|LV_STATE_DEFAULT);  // Dashed lines
	lv_obj_set_style_line_dash_gap(ui->scrOscilloscope_chartWaveform, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_color(ui->scrOscilloscope_chartWaveform, lv_color_hex(0x404040), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui->scrOscilloscope_chartWaveform, 1, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Set chart range: Y axis 0-1000 (center at 500, matching 10 divisions * 100 units/div)
	// This ensures center line is exactly on a grid line
	lv_chart_set_range(ui->scrOscilloscope_chartWaveform, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);

	// Add series for single channel yellow waveform
	lv_chart_series_t * ser1 = lv_chart_add_series(ui->scrOscilloscope_chartWaveform, lv_color_hex(0xFFFF00), LV_CHART_AXIS_PRIMARY_Y);

	// Generate sample sine wave data - fixed size dataset
	// Y range: 0-1000, center at 500 (0V reference)
	// Amplitude: 300 units (±3 divisions from center)
	for(int i = 0; i < 720; i++) {
		ser1->y_points[i] = 500 + (int)(300 * sin(i * 0.05));  // Center at 500, amplitude 300
	}

	lv_chart_refresh(ui->scrOscilloscope_chartWaveform);

	// === Add coordinate axes and center lines (for fixed area) ===
	// Note: Center lines are positioned ON grid lines for easy reading
	// Horizontal center: at division 5 (out of 10) = 200 pixels from top
	// Vertical center: at division 9 (out of 18) = 360 pixels from left

	// Horizontal center line (thick dashed white line at Y center - 0V reference)
	lv_obj_t *lineCenterH = lv_line_create(ui->scrOscilloscope_contWaveform);
	static lv_point_t line_points_h[] = {{0, 200}, {720, 200}};  // Y center = 400/2 = 200
	lv_line_set_points(lineCenterH, line_points_h, 2);
	lv_obj_set_style_line_color(lineCenterH, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(lineCenterH, 3, LV_PART_MAIN|LV_STATE_DEFAULT);  // Thicker line
	lv_obj_set_style_line_dash_width(lineCenterH, 5, LV_PART_MAIN|LV_STATE_DEFAULT);  // Dashed
	lv_obj_set_style_line_dash_gap(lineCenterH, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_opa(lineCenterH, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Vertical center line (thick dashed white line at X center - time reference)
	lv_obj_t *lineCenterV = lv_line_create(ui->scrOscilloscope_contWaveform);
	static lv_point_t line_points_v[] = {{360, 0}, {360, 400}};  // X center = 720/2 = 360
	lv_line_set_points(lineCenterV, line_points_v, 2);
	lv_obj_set_style_line_color(lineCenterV, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(lineCenterV, 3, LV_PART_MAIN|LV_STATE_DEFAULT);  // Thicker line
	lv_obj_set_style_line_dash_width(lineCenterV, 5, LV_PART_MAIN|LV_STATE_DEFAULT);  // Dashed
	lv_obj_set_style_line_dash_gap(lineCenterV, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_opa(lineCenterV, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	// === BOTTOM MEASUREMENT DISPLAYS (5 buttons: Freq, Vmax, Vmin, Vp-p, Vrms) ===
	// Normal mode: Freq, Vmax, Vmin, Vp-p, Vrms
	// FFT mode: Freq, Fundamental Freq, Harmonics (clickable for 1-9), THD

	// Create horizontal scrollable container for measurements
	lv_obj_t *contMeasurements = lv_obj_create(ui->scrOscilloscope);
	lv_obj_set_pos(contMeasurements, 0, 460);  // 45 + 400 + 15 = 460
	lv_obj_set_size(contMeasurements, 720, 20);  // Fixed height 20px
	lv_obj_set_scrollbar_mode(contMeasurements, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_scroll_dir(contMeasurements, LV_DIR_HOR);  // Horizontal scroll only

	// Container style - transparent background
	lv_obj_set_style_border_width(contMeasurements, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(contMeasurements, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(contMeasurements, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(contMeasurements, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(contMeasurements, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Create 5 measurement displays - Freq, Vmax, Vmin, Vp-p, Vrms
	create_measurement_display(ui, contMeasurements, &ui->scrOscilloscope_contFreq, &ui->scrOscilloscope_labelFreqTitle, &ui->scrOscilloscope_labelFreqValue,
	                           0, "Freq:", "50MHz", 0xFFFF00);  // Yellow

	create_measurement_display(ui, contMeasurements, &ui->scrOscilloscope_contVmax, &ui->scrOscilloscope_labelVmaxTitle, &ui->scrOscilloscope_labelVmaxValue,
	                           145, "Vmax:", "3.30V", 0xFFFF00);  // Yellow

	create_measurement_display(ui, contMeasurements, &ui->scrOscilloscope_contVmin, &ui->scrOscilloscope_labelVminTitle, &ui->scrOscilloscope_labelVminValue,
	                           290, "Vmin:", "0.10V", 0x00FF00);  // Green

	create_measurement_display(ui, contMeasurements, &ui->scrOscilloscope_contVpp, &ui->scrOscilloscope_labelVppTitle, &ui->scrOscilloscope_labelVppValue,
	                           435, "Vp-p:", "3.20V", 0x00FF00);  // Green

	create_measurement_display(ui, contMeasurements, &ui->scrOscilloscope_contVrms, &ui->scrOscilloscope_labelVrmsTitle, &ui->scrOscilloscope_labelVrmsValue,
	                           580, "Vrms:", "1.13V", 0xFF8000);  // Orange

	// === RIGHT SIDE CONTROL PANEL (English labels, larger font) ===
	// Controls (top to bottom):
	// 1. CH: Channel selection (fixed to CHA for single channel)
	// 2. Time: Time scale (1us ~ 1s, each horizontal grid represents one time division)
	// 3. Volt: Voltage scale (10mV ~ 12V)
	// 4. X-Pos: X-axis offset (updated by two-finger horizontal pan gesture)
	// 5. Y-Pos: Y-axis offset (updated by two-finger vertical pan gesture)
	// 6. Trig-V: Trigger voltage level
	// 7. Coupl: Coupling mode (AC/DC)
	// 8. Trig-M: Trigger mode selection

	// Create scrollable container for right side controls (positioned after AUTO button)
	lv_obj_t *contRightPanel = lv_obj_create(ui->scrOscilloscope);
	lv_obj_set_pos(contRightPanel, 735, 45);  // Y=45 (0+40+5, after AUTO button with 5px spacing)
	lv_obj_set_size(contRightPanel, 65, 435);  // Height adjusted to fill remaining space
	lv_obj_set_scrollbar_mode(contRightPanel, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_scroll_dir(contRightPanel, LV_DIR_VER);

	// Container style - transparent background
	lv_obj_set_style_border_width(contRightPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(contRightPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(contRightPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(contRightPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(contRightPanel, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Create control panels - English labels, alternating green and purple, increased spacing
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contChannel, &ui->scrOscilloscope_labelChannelTitle, &ui->scrOscilloscope_labelChannelValue,
	                     0, "CH", "CHA", 0x00FF00);  // Green - Channel selection
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contTimeScale, &ui->scrOscilloscope_labelTimeScaleTitle, &ui->scrOscilloscope_labelTimeScaleValue,
	                     55, "Time", "20us", 0xFF00FF);  // Purple - Time scale (50+5 spacing)
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contVoltScale, &ui->scrOscilloscope_labelVoltScaleTitle, &ui->scrOscilloscope_labelVoltScaleValue,
	                     110, "Volt", "1V", 0x00FF00);  // Green - Voltage scale (55+50+5)
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contXOffset, &ui->scrOscilloscope_labelXOffsetTitle, &ui->scrOscilloscope_labelXOffsetValue,
	                     165, "X-Pos", "0", 0xFF00FF);  // Purple - X axis offset (110+50+5)
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contYOffset, &ui->scrOscilloscope_labelYOffsetTitle, &ui->scrOscilloscope_labelYOffsetValue,
	                     220, "Y-Pos", "0", 0x00FF00);  // Green - Y axis offset (165+50+5)
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contTrigger, &ui->scrOscilloscope_labelTriggerTitle, &ui->scrOscilloscope_labelTriggerValue,
	                     275, "Trig-V", "0.5V", 0xFF00FF);  // Purple - Trigger voltage (220+50+5)
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contCoupling, &ui->scrOscilloscope_labelCouplingTitle, &ui->scrOscilloscope_labelCouplingValue,
	                     330, "Coupl", "DC", 0x00FF00);  // Green - Coupling mode (275+50+5)
	create_control_panel(ui, contRightPanel, &ui->scrOscilloscope_contTriggerMode, &ui->scrOscilloscope_labelTriggerModeTitle, &ui->scrOscilloscope_labelTriggerModeValue,
	                     385, "Trig-M", "RISE", 0xFF00FF);  // Purple - Trigger mode (330+50+5)

	//Update current screen layout.
	lv_obj_update_layout(ui->scrOscilloscope);

	//Init events for screen.
	events_init_scrOscilloscope(ui);
}
