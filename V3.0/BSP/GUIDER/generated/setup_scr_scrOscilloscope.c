/*
* Copyright 2025 NXP
* OSCILLOSCOPE SCREEN - Professional Single Channel Digital Oscilloscope V6.0
* FONT: All text uses lv_font_ShanHaiZhongXiaYeWuYuW family
* Updated: Full-width optimized layout with 15-column grid and wider control panel
*/

#include "lvgl.h"
#include <stdio.h>
#include <math.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"

#define SCREEN_WIDTH            800
#define SCREEN_HEIGHT           480
#define TOP_BAR_HEIGHT          40
#define TOP_BTN_RADIUS          8

// 波形显示区 - 使用正方形方格 (43x43像素)，扩展到16列充分利用空间
#define GRID_SIZE               43
#define GRID_COLS               16
#define GRID_ROWS               9
#define WAVEFORM_X              4
#define WAVEFORM_Y              45
#define WAVEFORM_WIDTH          (GRID_SIZE * GRID_COLS + 4)  // 692 (含边框)
#define WAVEFORM_HEIGHT         (GRID_SIZE * GRID_ROWS + 4 - 6)  // 385 (含边框，上下各减3)

// 右侧面板位置 - 靠右对齐，保持原有控制按钮宽度
#define RIGHT_PANEL_GAP         4
#define RIGHT_PANEL_WIDTH       90
#define RIGHT_PANEL_X           (SCREEN_WIDTH - RIGHT_PANEL_WIDTH - 4)  // 706
#define RIGHT_PANEL_TOP_Y       5  // 向下移动5像素
#define RIGHT_PANEL_BOTTOM_Y    476
#define CTRL_ITEM_COUNT         7
#define CTRL_ITEM_SPACING       4
#define CTRL_ITEM_HEIGHT        64
#define CTRL_ITEM_WIDTH         86
#define CTRL_OUTER_RADIUS       8

#define BOTTOM_BAR_HEIGHT       40
#define BOTTOM_BAR_Y            436
#define BOTTOM_BTN_RADIUS       8
#define PREVIEW_WIDTH           200
#define PREVIEW_HEIGHT          36

// 中心线坐标 (基于14列9行的方格)
#define CENTER_H_Y              (GRID_SIZE * GRID_ROWS / 2 + 2)  // 195
#define CENTER_V_X              (GRID_SIZE * GRID_COLS / 2 + 2)  // 303

#define COLOR_BG_BLACK          0x000000
#define COLOR_GRID_LINE         0x404040
#define COLOR_CENTER_LINE       0xFFFFFF
#define COLOR_BORDER            0x606060
#define COLOR_WAVEFORM_YELLOW   0xFFFF00
#define COLOR_TRIGGER_MAGENTA   0xFF00FF
#define COLOR_BTN_GREEN         0x00C853
#define COLOR_BTN_BLUE          0x2979FF
#define COLOR_BTN_YELLOW        0xFFD600
#define COLOR_BTN_ORANGE        0xFF6D00
#define COLOR_BTN_CYAN          0x00E5FF
#define COLOR_CTRL_GREEN        0x00E676
#define COLOR_CTRL_PURPLE       0xE040FB

static void create_measurement_button(lv_ui *ui, lv_obj_t *parent, lv_obj_t **cont, 
    lv_obj_t **label_title, lv_obj_t **label_value, int x, int width, 
    const char *title, const char *value, uint32_t color) {
    *cont = lv_btn_create(parent);
    lv_obj_set_pos(*cont, x, 0);
    lv_obj_set_size(*cont, width, BOTTOM_BAR_HEIGHT);
    lv_obj_set_style_bg_color(*cont, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(*cont, 0, 0);
    lv_obj_set_style_radius(*cont, BOTTOM_BTN_RADIUS, 0);
    lv_obj_set_style_shadow_width(*cont, 0, 0);
    *label_title = lv_label_create(*cont);
    char txt[64];
    snprintf(txt, sizeof(txt), "%s %s", title, value);
    lv_label_set_text(*label_title, txt);
    lv_obj_set_style_text_color(*label_title, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(*label_title, &lv_font_ShanHaiZhongXiaYeWuYuW_20, 0);
    lv_obj_align(*label_title, LV_ALIGN_CENTER, 0, 0);
    *label_value = *label_title;
}

static void create_control_item(lv_ui *ui, lv_obj_t *parent, lv_obj_t **cont, 
    lv_obj_t **label_title, lv_obj_t **label_value, int y, 
    const char *title, const char *value, uint32_t color) {
    *cont = lv_obj_create(parent);
    lv_obj_set_pos(*cont, 0, y);
    lv_obj_set_size(*cont, CTRL_ITEM_WIDTH, CTRL_ITEM_HEIGHT);
    lv_obj_set_scrollbar_mode(*cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(*cont, lv_color_hex(COLOR_BG_BLACK), 0);
    lv_obj_set_style_border_width(*cont, 2, 0);
    lv_obj_set_style_border_color(*cont, lv_color_hex(color), 0);
    lv_obj_set_style_radius(*cont, CTRL_OUTER_RADIUS, 0);
    lv_obj_set_style_pad_all(*cont, 0, 0);
    lv_obj_set_style_shadow_width(*cont, 0, 0);
    
    // 创建内部填充矩形 - 上边圆角，下边直角
    // 方法：使用两个矩形叠加，上部圆角矩形 + 下部直角矩形
    lv_obj_t *fill_top = lv_obj_create(*cont);
    lv_obj_set_pos(fill_top, 0, 0);
    lv_obj_set_size(fill_top, CTRL_ITEM_WIDTH - 4, 16);  // 上半部分
    lv_obj_set_scrollbar_mode(fill_top, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(fill_top, 255, 0);
    lv_obj_set_style_bg_color(fill_top, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(fill_top, 0, 0);
    lv_obj_set_style_radius(fill_top, CTRL_OUTER_RADIUS - 2, 0);  // 圆角
    lv_obj_set_style_pad_all(fill_top, 0, 0);
    lv_obj_align(fill_top, LV_ALIGN_TOP_MID, 0, 0);
    
    // 下半部分直角矩形，覆盖圆角矩形的下边圆角
    lv_obj_t *fill_bottom = lv_obj_create(*cont);
    lv_obj_set_pos(fill_bottom, 0, 10);  // 从圆角半径位置开始
    lv_obj_set_size(fill_bottom, CTRL_ITEM_WIDTH - 4, 16);  // 下半部分
    lv_obj_set_scrollbar_mode(fill_bottom, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(fill_bottom, 255, 0);
    lv_obj_set_style_bg_color(fill_bottom, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(fill_bottom, 0, 0);
    lv_obj_set_style_radius(fill_bottom, 0, 0);  // 直角
    lv_obj_set_style_pad_all(fill_bottom, 0, 0);
    lv_obj_align(fill_bottom, LV_ALIGN_TOP_MID, 0, 10);
    
    // 标题标签放在填充区域上
    *label_title = lv_label_create(*cont);
    lv_label_set_text(*label_title, title);
    lv_obj_set_style_text_color(*label_title, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(*label_title, &lv_font_ShanHaiZhongXiaYeWuYuW_18, 0);
    lv_obj_set_style_text_align(*label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(*label_title, LV_ALIGN_TOP_MID, 0, 4);
    
    *label_value = lv_label_create(*cont);
    lv_label_set_text(*label_value, value);
    lv_obj_set_size(*label_value, CTRL_ITEM_WIDTH - 4, 28);
    lv_obj_set_style_text_color(*label_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(*label_value, &lv_font_ShanHaiZhongXiaYeWuYuW_20, 0);
    lv_obj_set_style_text_align(*label_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(*label_value, 4, 0);
    lv_obj_align(*label_value, LV_ALIGN_BOTTOM_MID, 0, -2);
}

void setup_scr_scrOscilloscope(lv_ui *ui)
{
    ui->scrOscilloscope = lv_obj_create(NULL);
    lv_obj_set_size(ui->scrOscilloscope, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_scrollbar_mode(ui->scrOscilloscope, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->scrOscilloscope, lv_color_hex(COLOR_BG_BLACK), 0);

    // 顶部栏 - 左右两边都与波形区对齐
    #define TOP_BAR_X           WAVEFORM_X  // 与波形区左边对齐
    #define TOP_BAR_WIDTH       WAVEFORM_WIDTH  // 与波形区等宽
    
    // 返回按钮 - 简洁的"<"图标，无背景
    ui->scrOscilloscope_btnBack = lv_btn_create(ui->scrOscilloscope);
    ui->scrOscilloscope_btnBack_label = lv_label_create(ui->scrOscilloscope_btnBack);
    lv_label_set_text(ui->scrOscilloscope_btnBack_label, "<");
    lv_obj_align(ui->scrOscilloscope_btnBack_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(ui->scrOscilloscope_btnBack, 0, 0);  // 放在屏幕最左边
    lv_obj_set_size(ui->scrOscilloscope_btnBack, 60, TOP_BAR_HEIGHT);  // 60x40，容易点击
    
    // 无背景样式
    lv_obj_set_style_bg_opa(ui->scrOscilloscope_btnBack, 0, 0);  // 完全透明背景
    lv_obj_set_style_border_width(ui->scrOscilloscope_btnBack, 0, 0);  // 无边框
    lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnBack, 0, 0);  // 无阴影
    lv_obj_set_style_radius(ui->scrOscilloscope_btnBack, 0, 0);  // 无圆角
    
    // 只显示白色的"<"图标
    lv_obj_set_style_text_color(ui->scrOscilloscope_btnBack_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ui->scrOscilloscope_btnBack_label, &lv_font_ShanHaiZhongXiaYeWuYuW_30, 0);  // 30号字体
    
    // 确保返回按钮在最上层，不被遮挡
    lv_obj_move_foreground(ui->scrOscilloscope_btnBack);

    lv_obj_t *contTopBar = lv_obj_create(ui->scrOscilloscope);
    lv_obj_set_pos(contTopBar, TOP_BAR_X, 0);
    lv_obj_set_size(contTopBar, TOP_BAR_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_scrollbar_mode(contTopBar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(contTopBar, 0, 0);
    lv_obj_set_style_border_width(contTopBar, 0, 0);
    lv_obj_set_style_pad_all(contTopBar, 0, 0);

    // 顶部栏布局: < | RUN | FFT | 波形预览 | EXPORT | AUTO
    // 按钮从返回按钮后开始，AUTO按钮右边与顶部栏右边对齐
    // 返回按钮宽40，间距4，剩余空间动态分配
    #define BACK_BTN_WIDTH      40
    #define TOP_BTN_GAP         4
    #define RUN_BTN_WIDTH       90
    #define FFT_BTN_WIDTH       90
    #define EXPORT_BTN_WIDTH    90
    #define AUTO_BTN_WIDTH      90
    // 预览区宽度 = 总宽度 - 返回按钮 - RUN - FFT - EXPORT - AUTO - 5个间距
    #define CALC_PREVIEW_WIDTH  (TOP_BAR_WIDTH - BACK_BTN_WIDTH - RUN_BTN_WIDTH - FFT_BTN_WIDTH - EXPORT_BTN_WIDTH - AUTO_BTN_WIDTH - 5 * TOP_BTN_GAP)
    
    int top_x = BACK_BTN_WIDTH + TOP_BTN_GAP;  // 从返回按钮后开始
    
    ui->scrOscilloscope_btnStartStop = lv_btn_create(contTopBar);
    ui->scrOscilloscope_btnStartStop_label = lv_label_create(ui->scrOscilloscope_btnStartStop);
    lv_label_set_text(ui->scrOscilloscope_btnStartStop_label, "RUN");
    lv_obj_align(ui->scrOscilloscope_btnStartStop_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(ui->scrOscilloscope_btnStartStop, top_x, 0);
    lv_obj_set_size(ui->scrOscilloscope_btnStartStop, RUN_BTN_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(ui->scrOscilloscope_btnStartStop, lv_color_hex(COLOR_BTN_GREEN), 0);
    lv_obj_set_style_border_width(ui->scrOscilloscope_btnStartStop, 0, 0);
    lv_obj_set_style_radius(ui->scrOscilloscope_btnStartStop, TOP_BTN_RADIUS, 0);
    lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnStartStop, 0, 0);
    lv_obj_set_style_text_color(ui->scrOscilloscope_btnStartStop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(ui->scrOscilloscope_btnStartStop, &lv_font_ShanHaiZhongXiaYeWuYuW_20, 0);
    top_x += RUN_BTN_WIDTH + TOP_BTN_GAP;

    // FFT按钮 - 紧挨RUN按钮
    ui->scrOscilloscope_btnFFT = lv_btn_create(contTopBar);
    ui->scrOscilloscope_btnFFT_label = lv_label_create(ui->scrOscilloscope_btnFFT);
    lv_label_set_text(ui->scrOscilloscope_btnFFT_label, "FFT");
    lv_obj_align(ui->scrOscilloscope_btnFFT_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(ui->scrOscilloscope_btnFFT, top_x, 0);
    lv_obj_set_size(ui->scrOscilloscope_btnFFT, FFT_BTN_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(ui->scrOscilloscope_btnFFT, lv_color_hex(COLOR_BTN_YELLOW), 0);
    lv_obj_set_style_border_width(ui->scrOscilloscope_btnFFT, 0, 0);
    lv_obj_set_style_radius(ui->scrOscilloscope_btnFFT, TOP_BTN_RADIUS, 0);
    lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnFFT, 0, 0);
    lv_obj_set_style_text_color(ui->scrOscilloscope_btnFFT, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(ui->scrOscilloscope_btnFFT, &lv_font_ShanHaiZhongXiaYeWuYuW_20, 0);
    top_x += FFT_BTN_WIDTH + TOP_BTN_GAP;

    // 波形预览区域 - 细白色圆角边框，显示采样数据概览
    ui->scrOscilloscope_sliderWavePos = lv_obj_create(contTopBar);
    lv_obj_set_pos(ui->scrOscilloscope_sliderWavePos, top_x, 2);
    lv_obj_set_size(ui->scrOscilloscope_sliderWavePos, CALC_PREVIEW_WIDTH, PREVIEW_HEIGHT);
    lv_obj_set_scrollbar_mode(ui->scrOscilloscope_sliderWavePos, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->scrOscilloscope_sliderWavePos, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(ui->scrOscilloscope_sliderWavePos, 1, 0);  // 细边框
    lv_obj_set_style_border_color(ui->scrOscilloscope_sliderWavePos, lv_color_hex(0xFFFFFF), 0);  // 白色边框
    lv_obj_set_style_border_opa(ui->scrOscilloscope_sliderWavePos, 150, 0);  // 降低边框不透明度
    lv_obj_set_style_radius(ui->scrOscilloscope_sliderWavePos, 4, 0);  // 小圆角
    lv_obj_set_style_pad_all(ui->scrOscilloscope_sliderWavePos, 2, 0);

    int preview_w = CALC_PREVIEW_WIDTH;  // 动态计算的预览区宽度
    int preview_h = PREVIEW_HEIGHT - 4;  // 内部高度
    
    // 波形预览图表 - 显示完整采样数据，大幅值正弦波
    lv_obj_t *chartPreview = lv_chart_create(ui->scrOscilloscope_sliderWavePos);
    lv_chart_set_type(chartPreview, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(chartPreview, 0, 0);
    lv_chart_set_range(chartPreview, LV_CHART_AXIS_PRIMARY_Y, 0, 100);  // 设置Y轴范围
    lv_chart_set_point_count(chartPreview, preview_w - 4);
    lv_obj_set_size(chartPreview, preview_w - 4, preview_h);
    lv_obj_set_style_bg_opa(chartPreview, 0, 0);
    lv_obj_set_style_border_width(chartPreview, 0, 0);
    lv_obj_set_style_line_width(chartPreview, 2, LV_PART_ITEMS);  // 线宽2px
    lv_obj_set_style_size(chartPreview, 0, LV_PART_INDICATOR);
    lv_chart_series_t *serPreview = lv_chart_add_series(chartPreview, lv_color_hex(COLOR_WAVEFORM_YELLOW), LV_CHART_AXIS_PRIMARY_Y);
    // 大幅值正弦波，占满整个高度，显示更多周期
    for(int i = 0; i < preview_w - 4; i++) {
        serPreview->y_points[i] = 50 + (int)(80 * sin(i * 0.3));  // 幅值98%，频率系数0.3（显示更多完整周期）
    }
    lv_chart_refresh(chartPreview);

    // 左侧蓝色遮罩 - 表示不在波形区域显示的数据（隐藏数据）
    lv_obj_t *maskLeft = lv_obj_create(ui->scrOscilloscope_sliderWavePos);
    lv_obj_set_pos(maskLeft, 0, 0);
    lv_obj_set_size(maskLeft, 60, preview_h);
    lv_obj_set_scrollbar_mode(maskLeft, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(maskLeft, 180, 0);  // 半透明
    lv_obj_set_style_bg_color(maskLeft, lv_color_hex(0x0066FF), 0);  // 亮蓝色
    lv_obj_set_style_border_width(maskLeft, 0, 0);
    lv_obj_set_style_radius(maskLeft, 3, 0);  // 小圆角

    // 右侧蓝色遮罩 - 表示不在波形区域显示的数据（隐藏数据）
    lv_obj_t *maskRight = lv_obj_create(ui->scrOscilloscope_sliderWavePos);
    lv_obj_set_pos(maskRight, preview_w - 64, 0);
    lv_obj_set_size(maskRight, 60, preview_h);
    lv_obj_set_scrollbar_mode(maskRight, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(maskRight, 180, 0);  // 半透明
    lv_obj_set_style_bg_color(maskRight, lv_color_hex(0x0066FF), 0);  // 亮蓝色
    lv_obj_set_style_border_width(maskRight, 0, 0);
    lv_obj_set_style_radius(maskRight, 3, 0);  // 小圆角
    ui->scrOscilloscope_sliderWaveMask = maskLeft;  // 保存左侧遮罩引用
    ui->scrOscilloscope_sliderWaveMaskRight = maskRight;  // 保存右侧遮罩引用

    // 触发位置标记 - 精致的T形图标
    // 垂直线
    static lv_point_t triggerLinePoints[2];
    triggerLinePoints[0].x = preview_w / 2;
    triggerLinePoints[0].y = 6;  // 从T字下方开始
    triggerLinePoints[1].x = preview_w / 2;
    triggerLinePoints[1].y = preview_h - 2;
    lv_obj_t *triggerLine = lv_line_create(ui->scrOscilloscope_sliderWavePos);
    lv_line_set_points(triggerLine, triggerLinePoints, 2);
    lv_obj_set_style_line_color(triggerLine, lv_color_hex(COLOR_TRIGGER_MAGENTA), 0);
    lv_obj_set_style_line_width(triggerLine, 2, 0);  // 细线
    ui->scrOscilloscope_sliderWaveTrigger = triggerLine;  // 保存触发位置标记引用

    // T形图标顶部横线
    static lv_point_t triggerTopLinePoints[2];
    triggerTopLinePoints[0].x = preview_w / 2 - 6;
    triggerTopLinePoints[0].y = 4;
    triggerTopLinePoints[1].x = preview_w / 2 + 6;
    triggerTopLinePoints[1].y = 4;
    lv_obj_t *triggerTopLine = lv_line_create(ui->scrOscilloscope_sliderWavePos);
    lv_line_set_points(triggerTopLine, triggerTopLinePoints, 2);
    lv_obj_set_style_line_color(triggerTopLine, lv_color_hex(COLOR_TRIGGER_MAGENTA), 0);
    lv_obj_set_style_line_width(triggerTopLine, 2, 0);  // 细线
    lv_obj_set_style_line_rounded(triggerTopLine, true, 0);  // 圆角端点

    top_x += CALC_PREVIEW_WIDTH + TOP_BTN_GAP;

    // EXPORT按钮
    ui->scrOscilloscope_btnExport = lv_btn_create(contTopBar);
    ui->scrOscilloscope_btnExport_label = lv_label_create(ui->scrOscilloscope_btnExport);
    lv_label_set_text(ui->scrOscilloscope_btnExport_label, "EXPORT");
    lv_obj_align(ui->scrOscilloscope_btnExport_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(ui->scrOscilloscope_btnExport, top_x, 0);
    lv_obj_set_size(ui->scrOscilloscope_btnExport, EXPORT_BTN_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(ui->scrOscilloscope_btnExport, lv_color_hex(COLOR_BTN_ORANGE), 0);
    lv_obj_set_style_border_width(ui->scrOscilloscope_btnExport, 0, 0);
    lv_obj_set_style_radius(ui->scrOscilloscope_btnExport, TOP_BTN_RADIUS, 0);
    lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnExport, 0, 0);
    lv_obj_set_style_text_color(ui->scrOscilloscope_btnExport, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ui->scrOscilloscope_btnExport, &lv_font_ShanHaiZhongXiaYeWuYuW_18, 0);
    top_x += EXPORT_BTN_WIDTH + TOP_BTN_GAP;

    // AUTO 按钮 - 右边与顶部栏右边对齐
    ui->scrOscilloscope_btnAuto = lv_btn_create(contTopBar);
    ui->scrOscilloscope_btnAuto_label = lv_label_create(ui->scrOscilloscope_btnAuto);
    lv_label_set_text(ui->scrOscilloscope_btnAuto_label, "AUTO");
    lv_obj_align(ui->scrOscilloscope_btnAuto_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(ui->scrOscilloscope_btnAuto, TOP_BAR_WIDTH - AUTO_BTN_WIDTH, 0);  // 右对齐
    lv_obj_set_size(ui->scrOscilloscope_btnAuto, AUTO_BTN_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(ui->scrOscilloscope_btnAuto, lv_color_hex(COLOR_BTN_CYAN), 0);
    lv_obj_set_style_border_width(ui->scrOscilloscope_btnAuto, 0, 0);
    lv_obj_set_style_radius(ui->scrOscilloscope_btnAuto, TOP_BTN_RADIUS, 0);
    lv_obj_set_style_shadow_width(ui->scrOscilloscope_btnAuto, 0, 0);
    lv_obj_set_style_text_color(ui->scrOscilloscope_btnAuto, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(ui->scrOscilloscope_btnAuto, &lv_font_ShanHaiZhongXiaYeWuYuW_20, 0);

    ui->scrOscilloscope_contWaveform = lv_obj_create(ui->scrOscilloscope);
    lv_obj_set_pos(ui->scrOscilloscope_contWaveform, WAVEFORM_X, WAVEFORM_Y);
    lv_obj_set_size(ui->scrOscilloscope_contWaveform, WAVEFORM_WIDTH, WAVEFORM_HEIGHT);
    lv_obj_set_scrollbar_mode(ui->scrOscilloscope_contWaveform, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->scrOscilloscope_contWaveform, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->scrOscilloscope_contWaveform, lv_color_hex(COLOR_BG_BLACK), 0);
    lv_obj_set_style_border_width(ui->scrOscilloscope_contWaveform, 2, 0);
    lv_obj_set_style_border_color(ui->scrOscilloscope_contWaveform, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_radius(ui->scrOscilloscope_contWaveform, 0, 0);
    lv_obj_set_style_pad_all(ui->scrOscilloscope_contWaveform, 0, 0);
    lv_obj_set_style_shadow_width(ui->scrOscilloscope_contWaveform, 0, 0);

    ui->scrOscilloscope_chartWaveform = lv_chart_create(ui->scrOscilloscope_contWaveform);
    lv_chart_set_type(ui->scrOscilloscope_chartWaveform, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(ui->scrOscilloscope_chartWaveform, GRID_ROWS, GRID_COLS);
    lv_chart_set_point_count(ui->scrOscilloscope_chartWaveform, WAVEFORM_WIDTH - 4);
    lv_obj_set_pos(ui->scrOscilloscope_chartWaveform, 2, 2);
    lv_obj_set_size(ui->scrOscilloscope_chartWaveform, WAVEFORM_WIDTH - 4, WAVEFORM_HEIGHT - 4);
    lv_obj_set_scrollbar_mode(ui->scrOscilloscope_chartWaveform, LV_SCROLLBAR_MODE_OFF);
    
    // 关键修复：让图表不拦截触摸事件，触摸事件会传递到父容器（contWaveform）
    lv_obj_clear_flag(ui->scrOscilloscope_chartWaveform, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui->scrOscilloscope_chartWaveform, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    lv_obj_set_style_bg_color(ui->scrOscilloscope_chartWaveform, lv_color_hex(COLOR_BG_BLACK), 0);
    lv_obj_set_style_border_width(ui->scrOscilloscope_chartWaveform, 0, 0);
    lv_obj_set_style_radius(ui->scrOscilloscope_chartWaveform, 0, 0);
    lv_obj_set_style_line_width(ui->scrOscilloscope_chartWaveform, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(ui->scrOscilloscope_chartWaveform, 0, LV_PART_INDICATOR);
    // 精致的点状网格 - 小点，暗色
    lv_obj_set_style_line_dash_width(ui->scrOscilloscope_chartWaveform, 1, LV_PART_MAIN);  // 点宽1px
    lv_obj_set_style_line_dash_gap(ui->scrOscilloscope_chartWaveform, 8, LV_PART_MAIN);    // 间隔8px
    lv_obj_set_style_line_color(ui->scrOscilloscope_chartWaveform, lv_color_hex(0x303030), LV_PART_MAIN);  // 更暗的灰色
    lv_obj_set_style_line_width(ui->scrOscilloscope_chartWaveform, 1, LV_PART_MAIN);       // 细线
    lv_obj_set_style_line_opa(ui->scrOscilloscope_chartWaveform, 180, LV_PART_MAIN);       // 半透明
    lv_chart_set_range(ui->scrOscilloscope_chartWaveform, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);
    lv_chart_series_t *ser1 = lv_chart_add_series(ui->scrOscilloscope_chartWaveform, lv_color_hex(COLOR_WAVEFORM_YELLOW), LV_CHART_AXIS_PRIMARY_Y);
    // Initialize with flat line at center (500) - real ADC data will be displayed by drawing functions
    for(int i = 0; i < WAVEFORM_WIDTH - 4; i++) ser1->y_points[i] = 500;
    lv_chart_refresh(ui->scrOscilloscope_chartWaveform);

    // 中心坐标轴 - 实线 + 密集刻度标记（专业示波器风格）
    // 水平中心线（X轴）- 实线
    lv_obj_t *lineCenterH = lv_line_create(ui->scrOscilloscope_contWaveform);
    static lv_point_t line_h[] = {{2, CENTER_H_Y}, {WAVEFORM_WIDTH - 2, CENTER_H_Y}};
    lv_line_set_points(lineCenterH, line_h, 2);
    lv_obj_set_style_line_color(lineCenterH, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_line_width(lineCenterH, 1, 0);
    lv_obj_set_style_line_opa(lineCenterH, 180, 0);  // 与网格一致的透明度

    // 垂直中心线（Y轴）- 实线
    lv_obj_t *lineCenterV = lv_line_create(ui->scrOscilloscope_contWaveform);
    static lv_point_t line_v[] = {{CENTER_V_X, 2}, {CENTER_V_X, WAVEFORM_HEIGHT - 2}};
    lv_line_set_points(lineCenterV, line_v, 2);
    lv_obj_set_style_line_color(lineCenterV, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_line_width(lineCenterV, 1, 0);
    lv_obj_set_style_line_opa(lineCenterV, 180, 0);  // 与网格一致的透明度

    // X轴刻度标记 - 沿水平线密集分布的小竖线
    #define X_TICK_SPACING 8  // 每8px一个刻度
    #define X_TICK_HEIGHT 3   // 刻度高度
    static lv_point_t x_ticks[100][2];
    int x_tick_idx = 0;
    for(int x = 2; x < WAVEFORM_WIDTH - 2; x += X_TICK_SPACING) {
        if(x_tick_idx < 100) {
            x_ticks[x_tick_idx][0].x = x;
            x_ticks[x_tick_idx][0].y = CENTER_H_Y - X_TICK_HEIGHT;
            x_ticks[x_tick_idx][1].x = x;
            x_ticks[x_tick_idx][1].y = CENTER_H_Y + X_TICK_HEIGHT;
            
            lv_obj_t *tick = lv_line_create(ui->scrOscilloscope_contWaveform);
            lv_line_set_points(tick, x_ticks[x_tick_idx], 2);
            lv_obj_set_style_line_color(tick, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_line_width(tick, 1, 0);
            lv_obj_set_style_line_opa(tick, 180, 0);  // 与网格一致的透明度
            x_tick_idx++;
        }
    }

    // Y轴刻度标记 - 沿垂直线密集分布的小横线
    #define Y_TICK_SPACING 8  // 每8px一个刻度
    #define Y_TICK_WIDTH 3    // 刻度宽度
    static lv_point_t y_ticks[60][2];
    int y_tick_idx = 0;
    for(int y = 2; y < WAVEFORM_HEIGHT - 2; y += Y_TICK_SPACING) {
        if(y_tick_idx < 60) {
            y_ticks[y_tick_idx][0].x = CENTER_V_X - Y_TICK_WIDTH;
            y_ticks[y_tick_idx][0].y = y;
            y_ticks[y_tick_idx][1].x = CENTER_V_X + Y_TICK_WIDTH;
            y_ticks[y_tick_idx][1].y = y;
            
            lv_obj_t *tick = lv_line_create(ui->scrOscilloscope_contWaveform);
            lv_line_set_points(tick, y_ticks[y_tick_idx], 2);
            lv_obj_set_style_line_color(tick, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_line_width(tick, 1, 0);
            lv_obj_set_style_line_opa(tick, 180, 0);  // 与网格一致的透明度
            y_tick_idx++;
        }
    }

    lv_obj_t *contBottom = lv_obj_create(ui->scrOscilloscope);
    lv_obj_set_pos(contBottom, WAVEFORM_X, BOTTOM_BAR_Y);
    lv_obj_set_size(contBottom, WAVEFORM_WIDTH, BOTTOM_BAR_HEIGHT);
    lv_obj_set_scrollbar_mode(contBottom, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(contBottom, 0, 0);
    lv_obj_set_style_border_width(contBottom, 0, 0);
    lv_obj_set_style_pad_all(contBottom, 0, 0);

    int btn_w = (WAVEFORM_WIDTH - 4 * 4) / 5;
    create_measurement_button(ui, contBottom, &ui->scrOscilloscope_contFreq,
        &ui->scrOscilloscope_labelFreqTitle, &ui->scrOscilloscope_labelFreqValue,
        0, btn_w, "Freq:", "1.00kHz", COLOR_BTN_YELLOW);
    create_measurement_button(ui, contBottom, &ui->scrOscilloscope_contVmax,
        &ui->scrOscilloscope_labelVmaxTitle, &ui->scrOscilloscope_labelVmaxValue,
        btn_w + 4, btn_w, "Vmax:", "0.53V", COLOR_BTN_YELLOW);
    create_measurement_button(ui, contBottom, &ui->scrOscilloscope_contVmin,
        &ui->scrOscilloscope_labelVminTitle, &ui->scrOscilloscope_labelVminValue,
        (btn_w + 4) * 2, btn_w, "Vmin:", "-1.50V", COLOR_CTRL_GREEN);
    create_measurement_button(ui, contBottom, &ui->scrOscilloscope_contVpp,
        &ui->scrOscilloscope_labelVppTitle, &ui->scrOscilloscope_labelVppValue,
        (btn_w + 4) * 3, btn_w, "Vp-p:", "2.03V", COLOR_CTRL_GREEN);
    create_measurement_button(ui, contBottom, &ui->scrOscilloscope_contVrms,
        &ui->scrOscilloscope_labelVrmsTitle, &ui->scrOscilloscope_labelVrmsValue,
        (btn_w + 4) * 4, btn_w, "Vrms:", "1.06V", COLOR_BTN_ORANGE);

    lv_obj_t *contRightPanel = lv_obj_create(ui->scrOscilloscope);
    lv_obj_set_pos(contRightPanel, RIGHT_PANEL_X, RIGHT_PANEL_TOP_Y);
    lv_obj_set_size(contRightPanel, RIGHT_PANEL_WIDTH, RIGHT_PANEL_BOTTOM_Y - RIGHT_PANEL_TOP_Y);
    lv_obj_set_scrollbar_mode(contRightPanel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(contRightPanel, 0, 0);
    lv_obj_set_style_border_width(contRightPanel, 0, 0);
    lv_obj_set_style_pad_all(contRightPanel, 0, 0);

    // 计算控制项间距，使顶部与顶部按钮对齐，底部与底部按钮对齐
    // 面板高度 = 476, 7个控制项每个60高度, 总420, 剩余56, 6个间距, 每个约9
    int panel_height = RIGHT_PANEL_BOTTOM_Y - RIGHT_PANEL_TOP_Y;
    int total_items_height = CTRL_ITEM_COUNT * CTRL_ITEM_HEIGHT;
    int spacing = (panel_height - total_items_height) / (CTRL_ITEM_COUNT - 1);
    
    int y = 0;
    create_control_item(ui, contRightPanel, &ui->scrOscilloscope_contChannel,
        &ui->scrOscilloscope_labelChannelTitle, &ui->scrOscilloscope_labelChannelValue,
        y, "CH", "CHA", COLOR_CTRL_GREEN);
    y += CTRL_ITEM_HEIGHT + spacing;
    create_control_item(ui, contRightPanel, &ui->scrOscilloscope_contTimeScale,
        &ui->scrOscilloscope_labelTimeScaleTitle, &ui->scrOscilloscope_labelTimeScaleValue,
        y, "Time", "1ms", COLOR_CTRL_PURPLE);
    y += CTRL_ITEM_HEIGHT + spacing;
    create_control_item(ui, contRightPanel, &ui->scrOscilloscope_contVoltScale,
        &ui->scrOscilloscope_labelVoltScaleTitle, &ui->scrOscilloscope_labelVoltScaleValue,
        y, "Volt", "2V", COLOR_CTRL_GREEN);
    y += CTRL_ITEM_HEIGHT + spacing;
    create_control_item(ui, contRightPanel, &ui->scrOscilloscope_contXOffset,
        &ui->scrOscilloscope_labelXOffsetTitle, &ui->scrOscilloscope_labelXOffsetValue,
        y, "X-Pos", "0us", COLOR_CTRL_PURPLE);
    lv_obj_set_scroll_dir(ui->scrOscilloscope_contXOffset, LV_DIR_VER);  // 只允许垂直滑动
    y += CTRL_ITEM_HEIGHT + spacing;
    create_control_item(ui, contRightPanel, &ui->scrOscilloscope_contYOffset,
        &ui->scrOscilloscope_labelYOffsetTitle, &ui->scrOscilloscope_labelYOffsetValue,
        y, "Y-Pos", "0V", COLOR_CTRL_GREEN);
    lv_obj_set_scroll_dir(ui->scrOscilloscope_contYOffset, LV_DIR_VER);  // 只允许垂直滑动
    y += CTRL_ITEM_HEIGHT + spacing;
    create_control_item(ui, contRightPanel, &ui->scrOscilloscope_contTrigger,
        &ui->scrOscilloscope_labelTriggerTitle, &ui->scrOscilloscope_labelTriggerValue,
        y, "Survey", "OFF", COLOR_CTRL_PURPLE);
    y += CTRL_ITEM_HEIGHT + spacing;
    create_control_item(ui, contRightPanel, &ui->scrOscilloscope_contCoupling,
        &ui->scrOscilloscope_labelCouplingTitle, &ui->scrOscilloscope_labelCouplingValue,
        y, "Coup", "DC", COLOR_CTRL_GREEN);

    lv_obj_update_layout(ui->scrOscilloscope);
    events_init_scrOscilloscope(ui);
}
