/**
 * @file sdcard_manager_ui.c
 * @brief SD Card Manager - 2x2 Grid Layout with Status Bar
 * 
 * =============================================================================
 *                              LAYOUT DESIGN (800 x 480)
 * =============================================================================
 * 
 * +--------------------------------------------------------------------------+
 * |                         STATUS BAR (50px height)                         |
 * | +------+                                                                 |
 * | | Back |   [SD]   [====== Capacity Bar (400px) ======]    2.5GB/16GB    |
 * | +------+                                                                 |
 * +--------------------------------------------------------------------------+
 * |                                                                          |
 * |   +---------------------------+     +---------------------------+        |
 * |   |      STORAGE (Top-Left)   |     |  FILE MANAGER (Top-Right) |        |
 * |   |                           |     |                           |        |
 * |   |  [====] Screenshots 25%   |     |     +---------------+     |        |
 * |   |  [==] Oscilloscope 10%    |     |     |               |     |        |
 * |   |  [=] Config 5%            |     |     |  [FOLDER]     |     |        |
 * |   |  [==========] Free 60%    |     |     |  Open Files   |     |        |
 * |   |                           |     |     |               |     |        |
 * |   +---------------------------+     |     +---------------+     |        |
 * |                                     +---------------------------+        |
 * |   +---------------------------+     +---------------------------+        |
 * |   |  RECENT FILES (Bot-Left)  |     |   STATISTICS (Bot-Right)  |        |
 * |   |                           |     |                           |        |
 * |   |  [IMG] screenshot1.bmp 2M |     |  Screenshots: 125 (1.2GB) |        |
 * |   |  [CSV] data001.csv   50K  |     |  Oscilloscope: 48 (850MB) |        |
 * |   |  [IMG] screenshot2.bmp 2M |     |  Config: 15 (50KB)        |        |
 * |   |  [CFG] config.json   1K   |     |  -----------------------  |        |
 * |   |  [CSV] data002.csv   45K  |     |  Today Write: 50MB        |        |
 * |   |  ...                      |     |  Avg Size: 1.5MB          |        |
 * |   +---------------------------+     |  Total: 188 files         |        |
 * |                                     +---------------------------+        |
 * +--------------------------------------------------------------------------+
 * 
 * =============================================================================
 *                          FOLDER POPUP (Center, 350x280)
 * =============================================================================
 * 
 *                      +-----------------------------+
 *                      |   Select Folder         [X] |
 *                      |                             |
 *                      | +-------------------------+ |
 *                      | |  [IMG]  Screenshots     | |
 *                      | +-------------------------+ |
 *                      |                             |
 *                      | +-------------------------+ |
 *                      | |  [FILE] Oscilloscope    | |
 *                      | +-------------------------+ |
 *                      |                             |
 *                      | +-------------------------+ |
 *                      | |  [CFG]  Config          | |
 *                      | +-------------------------+ |
 *                      +-----------------------------+
 * 
 * =============================================================================
 *                      FILE BROWSER (Full Screen 800x480)
 * =============================================================================
 * 
 * +--------------------------------------------------------------------------+
 * |  [Back]              Screenshots               [Edit]  [Delete]          |
 * +--------------------------------------------------------------------------+
 * |                                                                          |
 * |  +--------------------------------------------------------------------+  |
 * |  | [ ] [IMG] screenshot_001.bmp          2.1 MB            [DELETE]   |  |
 * |  +--------------------------------------------------------------------+  |
 * |  | [ ] [IMG] screenshot_002.bmp          1.8 MB            [DELETE]   |  |
 * |  +--------------------------------------------------------------------+  |
 * |  | [ ] [IMG] screenshot_003.bmp          2.3 MB            [DELETE]   |  |
 * |  +--------------------------------------------------------------------+  |
 * |  | [ ] [IMG] screenshot_004.bmp          1.9 MB            [DELETE]   |  |
 * |  +--------------------------------------------------------------------+  |
 * |  |                          ... more files ...                        |  |
 * |  +--------------------------------------------------------------------+  |
 * |                                                                          |
 * +--------------------------------------------------------------------------+
 * 
 * Edit Mode: Show checkboxes [ ], hide individual [DELETE] buttons
 * Normal Mode: Hide checkboxes, show individual [DELETE] buttons
 * 
 * =============================================================================
 */

#include "sdcard_manager_ui.h"
#include "sdcard_manager.h"
#include "media_player.h"
#include "gui_guider.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *TAG = "SDCARD_UI";

/* Fonts */
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_18)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_20)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_22)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_24)

#define FONT_TITLE   &lv_font_ShanHaiZhongXiaYeWuYuW_24
#define FONT_LARGE   &lv_font_ShanHaiZhongXiaYeWuYuW_22
#define FONT_NORMAL  &lv_font_ShanHaiZhongXiaYeWuYuW_20
#define FONT_SMALL   &lv_font_ShanHaiZhongXiaYeWuYuW_18

/* Colors */
#define COLOR_BG            lv_color_hex(0xF5F5F5)
#define COLOR_CARD          lv_color_hex(0xFFFFFF)
#define COLOR_PRIMARY       lv_color_hex(0x2196F3)
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)
#define COLOR_WARNING       lv_color_hex(0xFF9800)
#define COLOR_ERROR         lv_color_hex(0xF44336)
#define COLOR_TEXT          lv_color_hex(0x212121)
#define COLOR_TEXT_SEC      lv_color_hex(0x757575)
#define COLOR_DIVIDER       lv_color_hex(0xE0E0E0)
#define COLOR_BORDER        lv_color_hex(0xE0E0E0)

#define COLOR_SCREENSHOT    lv_color_hex(0x4CAF50)
#define COLOR_OSCILLOSCOPE  lv_color_hex(0x2196F3)
#define COLOR_CONFIG        lv_color_hex(0xFF9800)
#define COLOR_MEDIA         lv_color_hex(0x9C27B0)  /* Purple for media files */
#define COLOR_FREE          lv_color_hex(0xBDBDBD)
#define COLOR_USED          lv_color_hex(0x3F51B5)

#define FONT_MEDIUM         FONT_NORMAL

/* UI Components */
static lv_obj_t *g_screen = NULL;
static lv_obj_t *g_capacity_label = NULL;
static lv_obj_t *g_sd_status_label = NULL;
static lv_obj_t *g_sd_status_cont = NULL;
static lv_obj_t *g_sd_status_icon = NULL;

/* Grid panels */
static lv_obj_t *g_pie_panel = NULL;       /* Top-left */
static lv_obj_t *g_files_panel = NULL;     /* Top-right */
static lv_obj_t *g_recent_panel = NULL;    /* Bottom-left */
static lv_obj_t *g_stats_panel = NULL;     /* Bottom-right */

/* Popup & Browser */
static lv_obj_t *g_folder_popup = NULL;
static lv_obj_t *g_file_browser = NULL;
static lv_obj_t *g_file_list = NULL;
static int g_browse_type = 0;
static bool g_edit_mode = false;
static lv_obj_t *g_edit_btn = NULL;
static lv_obj_t *g_delete_btn = NULL;

/* File buffer */
#define MAX_FILES 20
static sdcard_file_info_t *g_files = NULL;
static uint32_t g_file_count = 0;
static bool *g_selected = NULL;

/* Timer */
static lv_timer_t *g_timer = NULL;

extern lv_ui guider_ui;

/* Forward declarations */
static void create_status_bar(void);
static void create_pie_panel(lv_obj_t *parent);
static void create_files_panel(lv_obj_t *parent);
static void create_recent_panel(lv_obj_t *parent);
static void create_stats_panel(lv_obj_t *parent);
static void update_all(void);
static void show_folder_popup(void);
static void show_file_browser(int type);
static void close_popup(void);
static void close_browser(void);

/* Events */
static void back_clicked(lv_event_t *e);
static void files_btn_clicked(lv_event_t *e);
static void folder_btn_clicked(lv_event_t *e);
static void popup_close_clicked(lv_event_t *e);
static void browser_back_clicked(lv_event_t *e);
static void edit_btn_clicked(lv_event_t *e);
static void delete_selected_clicked(lv_event_t *e);
static void file_checkbox_changed(lv_event_t *e);
static void file_delete_clicked(lv_event_t *e);
static void file_item_clicked(lv_event_t *e);
static void file_detail_close_clicked(lv_event_t *e);

/* File detail popup */
static lv_obj_t *g_file_detail_popup = NULL;
static lv_color_t *g_image_buffer = NULL;  /* For BMP image preview */

/* ============== Helper ============== */

static lv_obj_t* create_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/* ============== Main ============== */

lv_obj_t* sdcard_manager_ui_create(lv_obj_t *parent)
{
    (void)parent;
    if (g_screen) return g_screen;
    
    ESP_LOGI(TAG, "Creating SD Card UI...");
    
    /* Alloc buffers */
    if (!g_files) {
        g_files = heap_caps_malloc(sizeof(sdcard_file_info_t) * MAX_FILES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!g_files) g_files = malloc(sizeof(sdcard_file_info_t) * MAX_FILES);
    }
    if (!g_selected) {
        g_selected = calloc(MAX_FILES, sizeof(bool));
    }
    if (!g_files || !g_selected) {
        ESP_LOGE(TAG, "Memory alloc failed");
        return NULL;
    }
    
    sdcard_manager_init();
    
    /* Full screen - create as child of active screen */
    g_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(g_screen, 0, 0);  /* Use set_pos for exact positioning */
    lv_obj_set_style_bg_color(g_screen, COLOR_BG, 0);
    lv_obj_set_style_border_width(g_screen, 0, 0);
    lv_obj_set_style_radius(g_screen, 0, 0);
    lv_obj_set_style_pad_all(g_screen, 0, 0);
    lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(g_screen);  /* Ensure it's on top of all other objects */
    
    /* Status bar - 50px */
    create_status_bar();
    
    /* Grid area */
    lv_coord_t grid_x = 10;
    lv_coord_t grid_y = 55;
    lv_coord_t grid_w = LV_HOR_RES - 20;
    lv_coord_t grid_h = LV_VER_RES - 60;
    lv_coord_t cell_w = (grid_w - 10) / 2;
    lv_coord_t cell_h = (grid_h - 10) / 2;
    
    /* 2x2 Grid */
    g_pie_panel = create_card(g_screen, grid_x, grid_y, cell_w, cell_h);
    g_files_panel = create_card(g_screen, grid_x + cell_w + 10, grid_y, cell_w, cell_h);
    g_recent_panel = create_card(g_screen, grid_x, grid_y + cell_h + 10, cell_w, cell_h);
    g_stats_panel = create_card(g_screen, grid_x + cell_w + 10, grid_y + cell_h + 10, cell_w, cell_h);
    
    create_pie_panel(g_pie_panel);
    create_files_panel(g_files_panel);
    create_recent_panel(g_recent_panel);
    create_stats_panel(g_stats_panel);
    
    g_timer = lv_timer_create((lv_timer_cb_t)update_all, 2000, NULL);
    update_all();
    
    ESP_LOGI(TAG, "UI created");
    return g_screen;
}

void sdcard_manager_ui_destroy(void)
{
    if (g_timer) { lv_timer_del(g_timer); g_timer = NULL; }
    if (g_folder_popup && lv_obj_is_valid(g_folder_popup)) lv_obj_del(g_folder_popup);
    if (g_file_browser && lv_obj_is_valid(g_file_browser)) lv_obj_del(g_file_browser);
    if (g_screen && lv_obj_is_valid(g_screen)) lv_obj_del(g_screen);
    
    g_folder_popup = NULL;
    g_file_browser = NULL;
    g_screen = NULL;
    g_pie_panel = NULL;
    g_files_panel = NULL;
    g_recent_panel = NULL;
    g_stats_panel = NULL;
    g_file_list = NULL;
    
    if (g_files) { free(g_files); g_files = NULL; }
    if (g_selected) { free(g_selected); g_selected = NULL; }
    
    ESP_LOGI(TAG, "UI destroyed");
}

void sdcard_manager_ui_update(void) { if (g_screen) update_all(); }
bool sdcard_manager_ui_is_visible(void) { return g_screen != NULL; }
void sdcard_manager_ui_show_browser(int type) { show_file_browser(type); }

/* ============== Status Bar ============== */

static void create_status_bar(void)
{
    /* Status bar - transparent background */
    lv_obj_t *bar = lv_obj_create(g_screen);
    lv_obj_set_size(bar, LV_HOR_RES - 20, 50);
    lv_obj_set_pos(bar, 10, 5);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Back button */
    lv_obj_t *back = lv_btn_create(bar);
    lv_obj_set_size(back, 80, 34);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(back, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(back, 17, 0);
    lv_obj_add_event_cb(back, back_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_lbl = lv_label_create(back);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(back_lbl, FONT_SMALL, 0);
    lv_obj_center(back_lbl);
    
    /* SD Card status indicator */
    g_sd_status_cont = lv_obj_create(bar);
    lv_obj_set_size(g_sd_status_cont, 100, 34);
    lv_obj_align(g_sd_status_cont, LV_ALIGN_LEFT_MID, 90, 0);
    lv_obj_set_style_bg_color(g_sd_status_cont, lv_color_hex(0xE8F5E9), 0);
    lv_obj_set_style_radius(g_sd_status_cont, 8, 0);
    lv_obj_set_style_border_width(g_sd_status_cont, 0, 0);
    lv_obj_set_style_pad_all(g_sd_status_cont, 0, 0);
    lv_obj_clear_flag(g_sd_status_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    g_sd_status_icon = lv_label_create(g_sd_status_cont);
    lv_label_set_text(g_sd_status_icon, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(g_sd_status_icon, COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(g_sd_status_icon, &lv_font_montserrat_18, 0);
    lv_obj_align(g_sd_status_icon, LV_ALIGN_LEFT_MID, 8, 0);
    
    g_sd_status_label = lv_label_create(g_sd_status_cont);
    lv_label_set_text(g_sd_status_label, "Ready");
    lv_obj_set_style_text_color(g_sd_status_label, COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(g_sd_status_label, FONT_SMALL, 0);
    lv_obj_align(g_sd_status_label, LV_ALIGN_LEFT_MID, 32, 0);
    
    /* Capacity label only - no bar */
    g_capacity_label = lv_label_create(bar);
    lv_label_set_text(g_capacity_label, "-- / --");
    lv_obj_set_style_text_color(g_capacity_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(g_capacity_label, FONT_NORMAL, 0);
    lv_obj_align(g_capacity_label, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void update_status_bar(void)
{
    bool is_mounted = sdcard_manager_is_mounted();
    sdcard_stats_t stats;
    bool has_stats = (sdcard_manager_get_stats(&stats) == ESP_OK);
    
    /* Update SD card status indicator */
    if (g_sd_status_label && g_sd_status_cont && g_sd_status_icon) {
        if (is_mounted && has_stats) {
            lv_label_set_text(g_sd_status_label, "Ready");
            lv_obj_set_style_text_color(g_sd_status_label, COLOR_SUCCESS, 0);
            lv_obj_set_style_text_color(g_sd_status_icon, COLOR_SUCCESS, 0);
            lv_obj_set_style_bg_color(g_sd_status_cont, lv_color_hex(0xE8F5E9), 0);
        } else {
            lv_label_set_text(g_sd_status_label, "No Card");
            lv_obj_set_style_text_color(g_sd_status_label, COLOR_ERROR, 0);
            lv_obj_set_style_text_color(g_sd_status_icon, COLOR_ERROR, 0);
            lv_obj_set_style_bg_color(g_sd_status_cont, lv_color_hex(0xFFEBEE), 0);
        }
    }
    
    /* Update capacity label */
    if (g_capacity_label) {
        if (!is_mounted || !has_stats) {
            lv_label_set_text(g_capacity_label, "No SD Card");
        } else {
            char used[16], total[16], txt[48];
            sdcard_manager_format_size(stats.used_bytes, used, sizeof(used));
            sdcard_manager_format_size(stats.total_bytes, total, sizeof(total));
            snprintf(txt, sizeof(txt), "%s / %s", used, total);
            lv_label_set_text(g_capacity_label, txt);
        }
    }
}

/* ============== Storage Panel (Top-Left) - Arc/Ring Chart ============== */

/* Storage for arc and legend pointers */
static lv_obj_t *g_storage_arc = NULL;  /* Main arc for total usage */
static lv_obj_t *g_storage_arcs[4] = {NULL, NULL, NULL, NULL};  /* Individual arcs for each file type */
static lv_obj_t *g_storage_pct_label = NULL;
static lv_obj_t *g_storage_legend = NULL;

static void create_pie_panel(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Storage Usage");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, FONT_MEDIUM, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* Background arc - gray background */
    g_storage_arc = lv_arc_create(parent);
    lv_obj_set_size(g_storage_arc, 100, 100);
    lv_obj_align(g_storage_arc, LV_ALIGN_LEFT_MID, 10, 10);
    lv_arc_set_rotation(g_storage_arc, 270);
    lv_arc_set_bg_angles(g_storage_arc, 0, 360);
    lv_arc_set_range(g_storage_arc, 0, 100);
    lv_arc_set_value(g_storage_arc, 100);  /* Full circle as background */
    lv_obj_remove_style(g_storage_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(g_storage_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(g_storage_arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(g_storage_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(g_storage_arc, lv_color_hex(0xE0E0E0), LV_PART_MAIN);
    lv_obj_set_style_arc_color(g_storage_arc, lv_color_hex(0xE0E0E0), LV_PART_INDICATOR);
    
    /* Create individual arcs for each file type (will be positioned on top) */
    for (int i = 0; i < 4; i++) {
        g_storage_arcs[i] = lv_arc_create(parent);
        lv_obj_set_size(g_storage_arcs[i], 100, 100);
        lv_obj_align(g_storage_arcs[i], LV_ALIGN_LEFT_MID, 10, 10);
        lv_arc_set_rotation(g_storage_arcs[i], 270);
        lv_arc_set_bg_angles(g_storage_arcs[i], 0, 0);  /* Will be set in update */
        lv_arc_set_range(g_storage_arcs[i], 0, 100);
        lv_arc_set_value(g_storage_arcs[i], 0);
        lv_obj_remove_style(g_storage_arcs[i], NULL, LV_PART_KNOB);
        lv_obj_clear_flag(g_storage_arcs[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_width(g_storage_arcs[i], 12, LV_PART_MAIN);
        lv_obj_set_style_arc_width(g_storage_arcs[i], 12, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(g_storage_arcs[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(g_storage_arcs[i], LV_OPA_TRANSP, LV_PART_INDICATOR);
    }
    
    /* Percentage in center */
    g_storage_pct_label = lv_label_create(parent);
    lv_label_set_text(g_storage_pct_label, "0%");
    lv_obj_set_style_text_color(g_storage_pct_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(g_storage_pct_label, FONT_LARGE, 0);
    lv_obj_align_to(g_storage_pct_label, g_storage_arc, LV_ALIGN_CENTER, 0, 0);
    
    /* Legend area - right side */
    g_storage_legend = lv_obj_create(parent);
    lv_obj_set_size(g_storage_legend, 160, 130);
    lv_obj_align(g_storage_legend, LV_ALIGN_RIGHT_MID, -5, 10);
    lv_obj_set_style_bg_opa(g_storage_legend, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_storage_legend, 0, 0);
    lv_obj_set_style_pad_all(g_storage_legend, 0, 0);
    lv_obj_set_flex_flow(g_storage_legend, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(g_storage_legend, 4, 0);
    lv_obj_clear_flag(g_storage_legend, LV_OBJ_FLAG_SCROLLABLE);
}

static void update_pie_panel(void)
{
    if (!g_pie_panel || !g_storage_legend) return;
    lv_obj_clean(g_storage_legend);
    
    sdcard_stats_t stats;
    bool has_stats = (sdcard_manager_get_stats(&stats) == ESP_OK);
    
    if (!has_stats) {
        lv_arc_set_value(g_storage_arc, 0);
        lv_label_set_text(g_storage_pct_label, "--");
        lv_obj_t *msg = lv_label_create(g_storage_legend);
        lv_label_set_text(msg, "No SD Card");
        lv_obj_set_style_text_color(msg, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(msg, FONT_SMALL, 0);
        return;
    }
    
    /* Calculate percentages for each file type */
    uint64_t total = stats.used_bytes + stats.free_bytes;
    int used_pct = total > 0 ? (int)((stats.used_bytes * 100) / total) : 0;
    
    /* File type data with colors */
    struct { const char *name; lv_color_t color; uint64_t bytes; } items[] = {
        {"Screenshots", COLOR_SCREENSHOT, stats.screenshot_bytes},
        {"Oscilloscope", COLOR_OSCILLOSCOPE, stats.oscilloscope_bytes},
        {"Media", COLOR_MEDIA, stats.media_bytes},
        {"Free", COLOR_FREE, stats.free_bytes}
    };
    
    /* Calculate percentages and cumulative start angles */
    int percentages[4];
    int start_angles[4] = {0, 0, 0, 0};
    int current_angle = 0;
    
    for (int i = 0; i < 4; i++) {
        percentages[i] = total > 0 ? (int)((items[i].bytes * 100) / total) : 0;
        start_angles[i] = current_angle;
        current_angle += percentages[i];
    }
    
    /* Update individual arcs for each file type */
    for (int i = 0; i < 4; i++) {
        if (g_storage_arcs[i]) {
            if (percentages[i] > 0) {
                /* Calculate start and end angles in degrees (0-360) */
                int start_deg = start_angles[i] * 360 / 100;
                int end_deg = (start_angles[i] + percentages[i]) * 360 / 100;
                
                /* Set arc angles directly (rotation is already set to 270 for top start) */
                lv_arc_set_rotation(g_storage_arcs[i], 270);
                lv_arc_set_bg_angles(g_storage_arcs[i], start_deg, end_deg);
                lv_obj_set_style_arc_color(g_storage_arcs[i], items[i].color, LV_PART_MAIN);
                lv_obj_set_style_arc_opa(g_storage_arcs[i], LV_OPA_COVER, LV_PART_MAIN);
                /* Hide indicator part */
                lv_obj_set_style_arc_opa(g_storage_arcs[i], LV_OPA_TRANSP, LV_PART_INDICATOR);
            } else {
                /* Hide arc if percentage is 0 */
                lv_obj_set_style_arc_opa(g_storage_arcs[i], LV_OPA_TRANSP, LV_PART_MAIN);
                lv_obj_set_style_arc_opa(g_storage_arcs[i], LV_OPA_TRANSP, LV_PART_INDICATOR);
            }
        }
    }
    
    /* Update percentage label */
    char pct_txt[8];
    snprintf(pct_txt, sizeof(pct_txt), "%d%%", used_pct);
    lv_label_set_text(g_storage_pct_label, pct_txt);
    lv_obj_align_to(g_storage_pct_label, g_storage_arc, LV_ALIGN_CENTER, 0, 0);
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = lv_obj_create(g_storage_legend);
        lv_obj_set_size(row, lv_pct(100), 28);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        
        /* Color dot */
        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(dot, items[i].color, 0);
        lv_obj_set_style_radius(dot, 5, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        
        /* Name and size */
        char txt[32], sz[12];
        sdcard_manager_format_size(items[i].bytes, sz, sizeof(sz));
        snprintf(txt, sizeof(txt), "%s", items[i].name);
        
        lv_obj_t *name_lbl = lv_label_create(row);
        lv_label_set_text(name_lbl, txt);
        lv_obj_set_style_text_color(name_lbl, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(name_lbl, FONT_SMALL, 0);
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 14, 0);
        
        lv_obj_t *size_lbl = lv_label_create(row);
        lv_label_set_text(size_lbl, sz);
        lv_obj_set_style_text_color(size_lbl, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(size_lbl, FONT_SMALL, 0);
        lv_obj_align(size_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
    }
}

/* ============== Files Panel (Top-Right) ============== */

static void create_files_panel(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "File Manager");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, FONT_LARGE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* Big button - use percentage */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, lv_pct(90), lv_pct(75));
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(btn, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_add_event_cb(btn, files_btn_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, LV_SYMBOL_DIRECTORY);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -15);
    
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "Open Files");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, FONT_NORMAL, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 25);
}

/* ============== Recent Panel (Bottom-Left) ============== */

static void create_recent_panel(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Recent Files");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, FONT_LARGE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(80));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 5, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_user_data(parent, list);
}

static void update_recent_panel(void)
{
    if (!g_recent_panel) return;
    lv_obj_t *list = lv_obj_get_user_data(g_recent_panel);
    if (!list) return;
    lv_obj_clean(list);
    
    sdcard_file_info_t recent[6];
    uint32_t count = 0;
    sdcard_manager_get_recent_files(recent, 6, &count);
    
    if (count == 0) {
        lv_obj_t *empty = lv_label_create(list);
        lv_label_set_text(empty, "No recent files");
        lv_obj_set_style_text_color(empty, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(empty, FONT_SMALL, 0);
        return;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_size(row, lv_pct(100), 26);
        lv_obj_set_style_bg_color(row, COLOR_BG, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_hor(row, 6, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_color_t ic = (recent[i].type == SDCARD_FILE_TYPE_SCREENSHOT) ? COLOR_SCREENSHOT :
                       (recent[i].type == SDCARD_FILE_TYPE_OSCILLOSCOPE) ? COLOR_OSCILLOSCOPE :
                       (recent[i].type == SDCARD_FILE_TYPE_MEDIA) ? COLOR_MEDIA : COLOR_TEXT_SEC;
        const char *icon = (recent[i].type == SDCARD_FILE_TYPE_SCREENSHOT) ? LV_SYMBOL_IMAGE :
                          (recent[i].type == SDCARD_FILE_TYPE_OSCILLOSCOPE) ? LV_SYMBOL_FILE :
                          (recent[i].type == SDCARD_FILE_TYPE_MEDIA) ? LV_SYMBOL_VIDEO : LV_SYMBOL_FILE;
        
        lv_obj_t *ic_lbl = lv_label_create(row);
        lv_label_set_text(ic_lbl, icon);
        lv_obj_set_style_text_color(ic_lbl, ic, 0);
        lv_obj_align(ic_lbl, LV_ALIGN_LEFT_MID, 0, 0);
        
        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, recent[i].name);
        lv_obj_set_style_text_color(name, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(name, FONT_SMALL, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 20, 0);
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name, 180);
        
        char sz[16];
        sdcard_manager_format_size(recent[i].size, sz, sizeof(sz));
        lv_obj_t *size_lbl = lv_label_create(row);
        lv_label_set_text(size_lbl, sz);
        lv_obj_set_style_text_color(size_lbl, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(size_lbl, FONT_SMALL, 0);
        lv_obj_align(size_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
    }
}

/* ============== Stats Panel (Bottom-Right) - Bar Chart Style ============== */

static void create_stats_panel(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Storage Usage");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, FONT_MEDIUM, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(88));
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(parent, cont);
}

static void update_stats_panel(void)
{
    if (!g_stats_panel) return;
    lv_obj_t *cont = lv_obj_get_user_data(g_stats_panel);
    if (!cont) return;
    lv_obj_clean(cont);
    
    sdcard_stats_t stats;
    if (sdcard_manager_get_stats(&stats) != ESP_OK) {
        lv_obj_t *msg = lv_label_create(cont);
        lv_label_set_text(msg, "No SD Card");
        lv_obj_set_style_text_color(msg, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(msg, FONT_NORMAL, 0);
        lv_obj_center(msg);
        return;
    }
    
    /* File type stats with colored background cards */
    struct { const char *name; lv_color_t color; uint32_t count; uint64_t bytes; const char *icon; } types[] = {
        {"Screenshot", COLOR_SCREENSHOT, stats.screenshot_count, stats.screenshot_bytes, LV_SYMBOL_IMAGE},
        {"Oscilloscope", COLOR_OSCILLOSCOPE, stats.oscilloscope_count, stats.oscilloscope_bytes, LV_SYMBOL_FILE},
        {"Media", COLOR_MEDIA, stats.media_count, stats.media_bytes, LV_SYMBOL_VIDEO}
    };
    
    /* Find max bytes for proportional card width */
    uint64_t max_bytes = 1;
    for (int i = 0; i < 3; i++) {
        if (types[i].bytes > max_bytes) max_bytes = types[i].bytes;
    }
    
    /* Get container width for calculation */
    lv_coord_t cont_width = lv_obj_get_width(cont);
    const int card_max_width = cont_width - 10;  /* Max card width (with margin) */
    const int card_min_width = 80;  /* Minimum card width even if 0 */
    
    for (int i = 0; i < 3; i++) {
        /* Calculate card width based on memory size proportion */
        int card_width = (int)((types[i].bytes * card_max_width) / max_bytes);
        if (card_width < card_min_width) card_width = card_min_width;  /* Always show minimum width */
        
        /* Colored background card with proportional width */
        lv_obj_t *card = lv_obj_create(cont);
        lv_obj_set_size(card, card_width, 42);
        lv_obj_set_pos(card, 0, i * 46);
        lv_obj_set_style_bg_color(card, types[i].color, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_20, 0);  /* 20% opacity */
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        
        /* Icon */
        lv_obj_t *icon = lv_label_create(card);
        lv_label_set_text(icon, types[i].icon);
        lv_obj_set_style_text_color(icon, types[i].color, 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);
        
        /* Name and count */
        char name_txt[64];
        snprintf(name_txt, sizeof(name_txt), "%s: %lu", types[i].name, (unsigned long)types[i].count);
        lv_obj_t *name = lv_label_create(card);
        lv_label_set_text(name, name_txt);
        lv_obj_set_style_text_color(name, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(name, FONT_SMALL, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 30, -8);
        
        /* Size info */
        char sz[16];
        sdcard_manager_format_size(types[i].bytes, sz, sizeof(sz));
        lv_obj_t *size_lbl = lv_label_create(card);
        lv_label_set_text(size_lbl, sz);
        lv_obj_set_style_text_color(size_lbl, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(size_lbl, FONT_SMALL, 0);
        lv_obj_align(size_lbl, LV_ALIGN_LEFT_MID, 30, 8);
    }
    
    /* Summary at bottom */
    uint32_t total_files = stats.screenshot_count + stats.oscilloscope_count + stats.media_count;
    
    lv_obj_t *total_row = lv_obj_create(cont);
    lv_obj_set_size(total_row, lv_pct(100), 28);
    lv_obj_set_pos(total_row, 0, 142);
    lv_obj_set_style_bg_color(total_row, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_radius(total_row, 6, 0);
    lv_obj_set_style_border_width(total_row, 0, 0);
    lv_obj_set_style_pad_hor(total_row, 8, 0);
    lv_obj_clear_flag(total_row, LV_OBJ_FLAG_SCROLLABLE);
    
    char txt[32];
    snprintf(txt, sizeof(txt), "Total: %lu files", (unsigned long)total_files);
    lv_obj_t *total_lbl = lv_label_create(total_row);
    lv_label_set_text(total_lbl, txt);
    lv_obj_set_style_text_color(total_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(total_lbl, FONT_SMALL, 0);
    lv_obj_align(total_lbl, LV_ALIGN_LEFT_MID, 0, 0);
    
    char sz[16];
    sdcard_manager_format_size(stats.today_write_bytes, sz, sizeof(sz));
    snprintf(txt, sizeof(txt), "Today: %s", sz);
    lv_obj_t *write_lbl = lv_label_create(total_row);
    lv_label_set_text(write_lbl, txt);
    lv_obj_set_style_text_color(write_lbl, COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(write_lbl, FONT_SMALL, 0);
    lv_obj_align(write_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
}

/* ============== Update All ============== */

static void update_all(void)
{
    if (!g_screen) return;
    sdcard_manager_refresh_stats();
    update_status_bar();
    update_pie_panel();
    update_recent_panel();
    update_stats_panel();
}

/* ============== Media File Scanner ============== */

static bool is_media_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext) return false;
    
    /* Check for supported media extensions: AVI video and images */
    if (strcasecmp(ext, ".avi") == 0 ||
        strcasecmp(ext, ".png") == 0 ||
        strcasecmp(ext, ".bmp") == 0 ||
        strcasecmp(ext, ".jpg") == 0 ||
        strcasecmp(ext, ".jpeg") == 0) {
        return true;
    }
    return false;
}

static void scan_media_files_recursive(const char *path, int depth)
{
    if (depth > 3 || g_file_count >= MAX_FILES) return;  /* Limit recursion depth */
    
    /* Skip screenshots directory - those are handled separately */
    if (strstr(path, "/screenshots") != NULL || strstr(path, "/Screenshots") != NULL) {
        return;
    }
    
    DIR *dir = opendir(path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && g_file_count < MAX_FILES) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        
        if (S_ISDIR(st.st_mode)) {
            /* Skip screenshots subdirectory */
            if (strcasecmp(entry->d_name, "screenshots") == 0) {
                continue;
            }
            /* Recurse into subdirectory */
            scan_media_files_recursive(full_path, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_media_file(entry->d_name)) {
            /* Add media file to list - use snprintf to avoid truncation warnings */
            snprintf(g_files[g_file_count].name, sizeof(g_files[g_file_count].name), "%s", entry->d_name);
            snprintf(g_files[g_file_count].path, sizeof(g_files[g_file_count].path), "%s", full_path);
            g_files[g_file_count].size = st.st_size;
            g_files[g_file_count].created_time = st.st_mtime;
            g_file_count++;
        }
    }
    
    closedir(dir);
}

static void scan_media_files(void)
{
    g_file_count = 0;
    
    /* Only scan /sdcard/Media directory for media files */
    const char *media_path = "/sdcard/Media";
    
    struct stat st;
    if (stat(media_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        scan_media_files_recursive(media_path, 0);
    } else {
        ESP_LOGW(TAG, "Media directory not found: %s", media_path);
    }
    
    ESP_LOGI(TAG, "Found %lu media files (AVI + non-screenshot images)", (unsigned long)g_file_count);
}

/* ============== Folder Popup ============== */

static void show_folder_popup(void)
{
    if (g_folder_popup) return;
    
    /* Dim background - no radius for full screen mask */
    g_folder_popup = lv_obj_create(g_screen);
    lv_obj_set_size(g_folder_popup, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(g_folder_popup, 0, 0);
    lv_obj_set_style_bg_color(g_folder_popup, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_folder_popup, LV_OPA_50, 0);
    lv_obj_set_style_border_width(g_folder_popup, 0, 0);
    lv_obj_set_style_radius(g_folder_popup, 0, 0);  /* No radius for mask */
    lv_obj_set_style_pad_all(g_folder_popup, 0, 0);
    lv_obj_clear_flag(g_folder_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_folder_popup, popup_close_clicked, LV_EVENT_CLICKED, NULL);
    
    /* Popup card */
    lv_obj_t *card = lv_obj_create(g_folder_popup);
    lv_obj_set_size(card, 350, 280);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, COLOR_DIVIDER, 0);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
    
    /* Title */
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "Select Folder");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, FONT_LARGE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    /* Close button */
    lv_obj_t *close_btn = lv_btn_create(card);
    lv_obj_set_size(close_btn, 32, 32);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 5, -5);
    lv_obj_set_style_bg_color(close_btn, COLOR_ERROR, 0);
    lv_obj_set_style_radius(close_btn, 16, 0);
    lv_obj_add_event_cb(close_btn, popup_close_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *x = lv_label_create(close_btn);
    lv_label_set_text(x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x, lv_color_white(), 0);
    lv_obj_center(x);
    
    /* Folder buttons - 3 buttons: Screenshots, Oscilloscope, Media (replaces Config) */
    struct { const char *icon; const char *name; lv_color_t color; int type; } folders[] = {
        {LV_SYMBOL_IMAGE, "Screenshots", COLOR_SCREENSHOT, 1},
        {LV_SYMBOL_FILE, "Oscilloscope", COLOR_OSCILLOSCOPE, 2},
        {LV_SYMBOL_VIDEO, "Media", COLOR_MEDIA, 3}  /* type=3 now shows media files */
    };
    
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(card);
        lv_obj_set_size(btn, 300, 55);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 50 + i * 65);
        lv_obj_set_style_bg_color(btn, folders[i].color, 0);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_user_data(btn, (void*)(intptr_t)folders[i].type);
        lv_obj_add_event_cb(btn, folder_btn_clicked, LV_EVENT_CLICKED, NULL);
        
        char txt[32];
        snprintf(txt, sizeof(txt), "%s  %s", folders[i].icon, folders[i].name);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, txt);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, FONT_NORMAL, 0);
        lv_obj_center(lbl);
    }
}

static void close_popup(void)
{
    if (g_folder_popup && lv_obj_is_valid(g_folder_popup)) {
        lv_obj_del(g_folder_popup);
    }
    g_folder_popup = NULL;
}

/* ============== File Browser (Popup Window - Same style as Select Folder) ============== */

static void refresh_file_list(void);

static void show_file_browser(int type)
{
    close_popup();
    g_browse_type = type;
    g_edit_mode = false;
    memset(g_selected, 0, MAX_FILES * sizeof(bool));
    
    /* Dim background - no radius for full screen mask */
    g_file_browser = lv_obj_create(g_screen);
    lv_obj_set_size(g_file_browser, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(g_file_browser, 0, 0);
    lv_obj_set_style_bg_color(g_file_browser, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_file_browser, LV_OPA_50, 0);
    lv_obj_set_style_border_width(g_file_browser, 0, 0);
    lv_obj_set_style_radius(g_file_browser, 0, 0);  /* No radius for mask */
    lv_obj_set_style_pad_all(g_file_browser, 0, 0);
    lv_obj_clear_flag(g_file_browser, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Popup card - same style as Select Folder */
    lv_obj_t *card = lv_obj_create(g_file_browser);
    lv_obj_set_size(card, 700, 420);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, COLOR_DIVIDER, 0);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Title */
    const char *ttxt = (type == 1) ? "Screenshots" : 
                       (type == 2) ? "Oscilloscope Data" : "Media Files";
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, ttxt);
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, FONT_LARGE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    /* Close button - red, top right */
    lv_obj_t *close_btn = lv_btn_create(card);
    lv_obj_set_size(close_btn, 32, 32);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 5, -5);
    lv_obj_set_style_bg_color(close_btn, COLOR_ERROR, 0);
    lv_obj_set_style_radius(close_btn, 16, 0);
    lv_obj_add_event_cb(close_btn, browser_back_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *x = lv_label_create(close_btn);
    lv_label_set_text(x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x, lv_color_white(), 0);
    lv_obj_center(x);
    
    /* Edit button - top left area */
    g_edit_btn = lv_btn_create(card);
    lv_obj_set_size(g_edit_btn, 70, 30);
    lv_obj_align(g_edit_btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(g_edit_btn, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(g_edit_btn, 15, 0);
    lv_obj_add_event_cb(g_edit_btn, edit_btn_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *edit_lbl = lv_label_create(g_edit_btn);
    lv_label_set_text(edit_lbl, "Edit");
    lv_obj_set_style_text_color(edit_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(edit_lbl, FONT_SMALL, 0);
    lv_obj_center(edit_lbl);
    
    /* Delete selected button (hidden initially) - next to edit */
    g_delete_btn = lv_btn_create(card);
    lv_obj_set_size(g_delete_btn, 70, 30);
    lv_obj_align(g_delete_btn, LV_ALIGN_TOP_LEFT, 80, 0);
    lv_obj_set_style_bg_color(g_delete_btn, COLOR_ERROR, 0);
    lv_obj_set_style_radius(g_delete_btn, 15, 0);
    lv_obj_add_event_cb(g_delete_btn, delete_selected_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_delete_btn, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t *del_lbl = lv_label_create(g_delete_btn);
    lv_label_set_text(del_lbl, LV_SYMBOL_TRASH " Del");
    lv_obj_set_style_text_color(del_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(del_lbl, FONT_SMALL, 0);
    lv_obj_center(del_lbl);
    
    /* File list area */
    g_file_list = lv_obj_create(card);
    lv_obj_set_size(g_file_list, 670, 350);
    lv_obj_align(g_file_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(g_file_list, COLOR_BG, 0);
    lv_obj_set_style_radius(g_file_list, 8, 0);
    lv_obj_set_style_border_width(g_file_list, 0, 0);
    lv_obj_set_style_pad_all(g_file_list, 8, 0);
    lv_obj_set_flex_flow(g_file_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(g_file_list, 5, 0);
    lv_obj_add_flag(g_file_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(g_file_list, LV_DIR_VER);
    
    refresh_file_list();
}

static void refresh_file_list(void)
{
    if (!g_file_list) return;
    lv_obj_clean(g_file_list);
    
    g_file_count = 0;
    
    if (g_browse_type == 3) {
        /* Media files - scan SD card for PNG, BMP and AVI files */
        scan_media_files();
    } else {
        sdcard_file_type_t ft = (g_browse_type == 1) ? SDCARD_FILE_TYPE_SCREENSHOT : SDCARD_FILE_TYPE_OSCILLOSCOPE;
        sdcard_manager_list_files(ft, g_files, MAX_FILES, &g_file_count);
    }
    
    if (g_file_count == 0) {
        lv_obj_t *empty = lv_label_create(g_file_list);
        lv_label_set_text(empty, "No files found");
        lv_obj_set_style_text_color(empty, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(empty, FONT_NORMAL, 0);
        return;
    }
    
    lv_color_t ic = (g_browse_type == 1) ? COLOR_SCREENSHOT : 
                   (g_browse_type == 2) ? COLOR_OSCILLOSCOPE : COLOR_MEDIA;
    const char *icon = (g_browse_type == 1) ? LV_SYMBOL_IMAGE : 
                      (g_browse_type == 2) ? LV_SYMBOL_FILE : LV_SYMBOL_VIDEO;
    
    for (uint32_t i = 0; i < g_file_count; i++) {
        lv_obj_t *item = lv_obj_create(g_file_list);
        lv_obj_set_size(item, lv_pct(100), 55);
        lv_obj_set_style_bg_color(item, COLOR_BG, 0);
        lv_obj_set_style_radius(item, 10, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_pad_hor(item, 10, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(item, (void*)(intptr_t)i);
        lv_obj_add_event_cb(item, file_item_clicked, LV_EVENT_CLICKED, NULL);
        
        /* Checkbox (only visible in edit mode) */
        lv_obj_t *cb = lv_checkbox_create(item);
        lv_checkbox_set_text(cb, "");
        lv_obj_align(cb, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_user_data(cb, (void*)(intptr_t)i);
        lv_obj_add_event_cb(cb, file_checkbox_changed, LV_EVENT_VALUE_CHANGED, NULL);
        if (!g_edit_mode) lv_obj_add_flag(cb, LV_OBJ_FLAG_HIDDEN);
        
        /* Icon - for media files, show different icon based on extension */
        const char *file_icon = icon;
        lv_color_t file_ic = ic;
        if (g_browse_type == 3) {
            const char *ext = strrchr(g_files[i].name, '.');
            if (ext) {
                if (strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0 || 
                    strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
                    file_icon = LV_SYMBOL_IMAGE;
                    file_ic = COLOR_SCREENSHOT;
                } else if (strcasecmp(ext, ".avi") == 0) {
                    file_icon = LV_SYMBOL_VIDEO;
                    file_ic = COLOR_MEDIA;
                }
            }
        }
        
        lv_obj_t *ic_lbl = lv_label_create(item);
        lv_label_set_text(ic_lbl, file_icon);
        lv_obj_set_style_text_color(ic_lbl, file_ic, 0);
        lv_obj_set_style_text_font(ic_lbl, &lv_font_montserrat_22, 0);
        lv_obj_align(ic_lbl, LV_ALIGN_LEFT_MID, g_edit_mode ? 35 : 0, 0);
        
        /* Filename */
        lv_obj_t *name = lv_label_create(item);
        lv_label_set_text(name, g_files[i].name);
        lv_obj_set_style_text_color(name, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(name, FONT_SMALL, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, g_edit_mode ? 65 : 30, -10);
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name, 450);
        
        /* Size */
        char sz[16];
        sdcard_manager_format_size(g_files[i].size, sz, sizeof(sz));
        lv_obj_t *size_lbl = lv_label_create(item);
        lv_label_set_text(size_lbl, sz);
        lv_obj_set_style_text_color(size_lbl, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(size_lbl, FONT_SMALL, 0);
        lv_obj_align(size_lbl, LV_ALIGN_LEFT_MID, g_edit_mode ? 65 : 30, 12);
        
        /* Delete button (only visible in non-edit mode) */
        lv_obj_t *del = lv_btn_create(item);
        lv_obj_set_size(del, 42, 42);
        lv_obj_align(del, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_bg_color(del, COLOR_ERROR, 0);
        lv_obj_set_style_radius(del, 21, 0);
        lv_obj_set_user_data(del, (void*)(intptr_t)i);
        lv_obj_add_event_cb(del, file_delete_clicked, LV_EVENT_CLICKED, NULL);
        if (g_edit_mode) lv_obj_add_flag(del, LV_OBJ_FLAG_HIDDEN);
        
        lv_obj_t *del_ic = lv_label_create(del);
        lv_label_set_text(del_ic, LV_SYMBOL_TRASH);
        lv_obj_set_style_text_color(del_ic, lv_color_white(), 0);
        lv_obj_center(del_ic);
    }
}

static void close_browser(void)
{
    if (g_file_browser && lv_obj_is_valid(g_file_browser)) {
        lv_obj_del(g_file_browser);
    }
    g_file_browser = NULL;
    g_file_list = NULL;
    g_edit_btn = NULL;
    g_delete_btn = NULL;
    update_all();
}

/* ============== Event Handlers ============== */

static void back_clicked(lv_event_t *e) { (void)e; sdcard_manager_ui_destroy(); }

static void files_btn_clicked(lv_event_t *e) { (void)e; show_folder_popup(); }

static void folder_btn_clicked(lv_event_t *e)
{
    int type = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    show_file_browser(type);
}

static void popup_close_clicked(lv_event_t *e) { (void)e; close_popup(); }

static void browser_back_clicked(lv_event_t *e) { (void)e; close_browser(); }

static void edit_btn_clicked(lv_event_t *e)
{
    (void)e;
    g_edit_mode = !g_edit_mode;
    memset(g_selected, 0, MAX_FILES * sizeof(bool));
    
    /* Update edit button text */
    lv_obj_t *lbl = lv_obj_get_child(g_edit_btn, 0);
    lv_label_set_text(lbl, g_edit_mode ? "Done" : "Edit");
    
    /* Show/hide delete button */
    if (g_edit_mode) {
        lv_obj_clear_flag(g_delete_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_delete_btn, LV_OBJ_FLAG_HIDDEN);
    }
    
    refresh_file_list();
}

static void file_checkbox_changed(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(cb);
    if (idx >= 0 && idx < MAX_FILES) {
        g_selected[idx] = lv_obj_has_state(cb, LV_STATE_CHECKED);
    }
}

static void delete_selected_clicked(lv_event_t *e)
{
    (void)e;
    for (uint32_t i = 0; i < g_file_count; i++) {
        if (g_selected[i]) {
            sdcard_manager_delete_file(g_files[i].path);
            ESP_LOGI(TAG, "Deleted: %s", g_files[i].name);
        }
    }
    memset(g_selected, 0, MAX_FILES * sizeof(bool));
    refresh_file_list();
}

static void file_delete_clicked(lv_event_t *e)
{
    lv_event_stop_bubbling(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (idx >= 0 && idx < (int)g_file_count) {
        sdcard_manager_delete_file(g_files[idx].path);
        ESP_LOGI(TAG, "Deleted: %s", g_files[idx].name);
        refresh_file_list();
    }
}

static void file_detail_close_clicked(lv_event_t *e)
{
    (void)e;
    if (g_file_detail_popup && lv_obj_is_valid(g_file_detail_popup)) {
        lv_obj_del(g_file_detail_popup);
    }
    g_file_detail_popup = NULL;
    
    /* Free image buffer when closing */
    if (g_image_buffer != NULL) {
        heap_caps_free(g_image_buffer);
        g_image_buffer = NULL;
    }
}

static void show_image_preview(int idx)
{
    if (idx < 0 || idx >= (int)g_file_count) return;
    
    ESP_LOGI(TAG, "Loading image: %s", g_files[idx].path);
    
    /* Open BMP file */
    FILE *f = fopen(g_files[idx].path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", g_files[idx].path);
        return;
    }
    
    /* Read BMP header */
    uint8_t header[54];
    if (fread(header, 1, 54, f) != 54) {
        ESP_LOGE(TAG, "Failed to read BMP header");
        fclose(f);
        return;
    }
    
    /* Parse dimensions */
    int width = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
    int height = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
    if (height < 0) height = -height;  /* Handle top-down BMPs */
    
    ESP_LOGI(TAG, "Image size: %dx%d", width, height);
    
    /* Allocate buffer for image */
    size_t buffer_size = width * height * sizeof(lv_color_t);
    if (g_image_buffer != NULL) {
        heap_caps_free(g_image_buffer);
    }
    g_image_buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (g_image_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate image buffer");
        fclose(f);
        return;
    }
    
    /* Read and convert BMP data (bottom-up, BGR) to LVGL format */
    int row_size = ((width * 3 + 3) / 4) * 4;
    uint8_t *row_buffer = malloc(row_size);
    if (row_buffer == NULL) {
        heap_caps_free(g_image_buffer);
        g_image_buffer = NULL;
        fclose(f);
        return;
    }
    
    for (int y = height - 1; y >= 0; y--) {
        if (fread(row_buffer, 1, row_size, f) != row_size) {
            ESP_LOGE(TAG, "Failed to read BMP row");
            break;
        }
        
        for (int x = 0; x < width; x++) {
            uint8_t b = row_buffer[x * 3 + 0];
            uint8_t g = row_buffer[x * 3 + 1];
            uint8_t r = row_buffer[x * 3 + 2];
            g_image_buffer[y * width + x] = lv_color_make(r, g, b);
        }
    }
    
    free(row_buffer);
    fclose(f);
    
    /* Create fullscreen popup - create directly on screen, not on browser */
    g_file_detail_popup = lv_obj_create(g_screen);
    lv_obj_set_size(g_file_detail_popup, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(g_file_detail_popup, 0, 0);
    lv_obj_set_style_bg_color(g_file_detail_popup, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(g_file_detail_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_file_detail_popup, 0, 0);
    lv_obj_set_style_pad_all(g_file_detail_popup, 0, 0);
    lv_obj_set_style_radius(g_file_detail_popup, 0, 0);
    lv_obj_clear_flag(g_file_detail_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(g_file_detail_popup);  /* Bring to front */
    
    /* Image container - full screen, no padding */
    lv_obj_t *img_cont = lv_obj_create(g_file_detail_popup);
    lv_coord_t cont_w = LV_HOR_RES;      /* Full width */
    lv_coord_t cont_h = LV_VER_RES;      /* Full height */
    lv_obj_set_size(img_cont, cont_w, cont_h);
    lv_obj_set_pos(img_cont, 0, 0);  /* Start from top-left corner */
    lv_obj_set_style_bg_color(img_cont, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(img_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(img_cont, 0, 0);
    lv_obj_set_style_pad_all(img_cont, 0, 0);
    lv_obj_set_style_radius(img_cont, 0, 0);
    lv_obj_clear_flag(img_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Record access for recent list */
    sdcard_manager_record_access(g_files[idx].path);
    update_recent_panel();
    
    /* Create image descriptor and display */
    static lv_img_dsc_t img_dsc;
    img_dsc.header.always_zero = 0;
    img_dsc.header.w = width;
    img_dsc.header.h = height;
    img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    img_dsc.data_size = buffer_size;
    img_dsc.data = (const uint8_t *)g_image_buffer;
    
    lv_obj_t *img = lv_img_create(img_cont);
    lv_img_set_src(img, &img_dsc);
    
    /*
     * LVGL zoom: 256 = 1.0x
     * Fill strategy: use larger scale so the image fully covers the container (may crop).
     */
    int32_t scale_x = (int32_t)cont_w * 256 / width;
    int32_t scale_y = (int32_t)cont_h * 256 / height;
    int32_t scale = (scale_x > scale_y) ? scale_x : scale_y; /* fill */
    if (scale < 64) scale = 64;       /* min 0.25x */
    if (scale > 1536) scale = 1536;   /* max 6x */
    
    /* Set pivot to image center before zoom */
    lv_img_set_pivot(img, width / 2, height / 2);
    lv_img_set_zoom(img, scale);
    
    /* Set image position to center of container */
    lv_obj_set_pos(img, cont_w / 2 - (width * scale / 256) / 2, 
                   cont_h / 2 - (height * scale / 256) / 2);
    
    /* Filename label */
    lv_obj_t *name_lbl = lv_label_create(g_file_detail_popup);
    lv_label_set_text(name_lbl, g_files[idx].name);
    lv_obj_set_style_text_color(name_lbl, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(name_lbl, FONT_NORMAL, 0);
    lv_obj_align(name_lbl, LV_ALIGN_BOTTOM_MID, 0, -50);
    
    /* Size label */
    char sz[32];
    sdcard_manager_format_size(g_files[idx].size, sz, sizeof(sz));
    lv_obj_t *size_lbl = lv_label_create(g_file_detail_popup);
    lv_label_set_text(size_lbl, sz);
    lv_obj_set_style_text_color(size_lbl, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(size_lbl, FONT_SMALL, 0);
    lv_obj_align(size_lbl, LV_ALIGN_BOTTOM_MID, 0, -25);
    
    /* Close button - moved up */
    lv_obj_t *close_btn = lv_btn_create(g_file_detail_popup);
    lv_obj_set_size(close_btn, 50, 50);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);  /* Moved up from 20 to 10 */
    lv_obj_set_style_bg_color(close_btn, COLOR_ERROR, 0);
    lv_obj_set_style_radius(close_btn, 25, 0);
    lv_obj_add_event_cb(close_btn, file_detail_close_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *x = lv_label_create(close_btn);
    lv_label_set_text(x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x, lv_color_white(), 0);
    lv_obj_set_style_text_font(x, &lv_font_montserrat_20, 0);
    lv_obj_center(x);
    
    /* Click anywhere to close */
    lv_obj_add_flag(g_file_detail_popup, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_file_detail_popup, file_detail_close_clicked, LV_EVENT_CLICKED, NULL);
}

static void show_file_info(int idx)
{
    if (idx < 0 || idx >= (int)g_file_count) return;
    
    /* Record access for recent list */
    sdcard_manager_record_access(g_files[idx].path);
    update_recent_panel();
    
    /* Create popup - no radius for mask */
    g_file_detail_popup = lv_obj_create(g_file_browser);
    lv_obj_set_size(g_file_detail_popup, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(g_file_detail_popup, 0, 0);
    lv_obj_set_style_bg_color(g_file_detail_popup, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_file_detail_popup, LV_OPA_50, 0);
    lv_obj_set_style_border_width(g_file_detail_popup, 0, 0);
    lv_obj_set_style_radius(g_file_detail_popup, 0, 0);  /* No radius for mask */
    lv_obj_set_style_pad_all(g_file_detail_popup, 0, 0);
    lv_obj_clear_flag(g_file_detail_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_file_detail_popup, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_file_detail_popup, file_detail_close_clicked, LV_EVENT_CLICKED, NULL);
    
    /* Info card */
    lv_obj_t *card = lv_obj_create(g_file_detail_popup);
    lv_obj_set_size(card, 400, 280);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, COLOR_DIVIDER, 0);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
    
    /* Title */
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "File Information");
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, FONT_LARGE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    /* Close button */
    lv_obj_t *close_btn = lv_btn_create(card);
    lv_obj_set_size(close_btn, 32, 32);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 5, -5);
    lv_obj_set_style_bg_color(close_btn, COLOR_ERROR, 0);
    lv_obj_set_style_radius(close_btn, 16, 0);
    lv_obj_add_event_cb(close_btn, file_detail_close_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *x = lv_label_create(close_btn);
    lv_label_set_text(x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x, lv_color_white(), 0);
    lv_obj_center(x);
    
    /* File icon */
    lv_obj_t *icon = lv_label_create(card);
    lv_label_set_text(icon, (g_browse_type == 2) ? LV_SYMBOL_FILE : LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(icon, (g_browse_type == 2) ? COLOR_OSCILLOSCOPE : COLOR_CONFIG, 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, -20);
    
    /* Filename */
    lv_obj_t *name_lbl = lv_label_create(card);
    lv_label_set_text(name_lbl, g_files[idx].name);
    lv_obj_set_style_text_color(name_lbl, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(name_lbl, FONT_NORMAL, 0);
    lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 60, -40);
    lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(name_lbl, 280);
    
    /* File size */
    char sz[32];
    sdcard_manager_format_size(g_files[idx].size, sz, sizeof(sz));
    char size_txt[48];
    snprintf(size_txt, sizeof(size_txt), "Size: %s", sz);
    lv_obj_t *size_lbl = lv_label_create(card);
    lv_label_set_text(size_lbl, size_txt);
    lv_obj_set_style_text_color(size_lbl, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(size_lbl, FONT_SMALL, 0);
    lv_obj_align(size_lbl, LV_ALIGN_LEFT_MID, 60, -10);
    
    /* File type */
    const char *type_str = (g_browse_type == 2) ? "Oscilloscope CSV Data" : "Configuration File";
    lv_obj_t *type_lbl = lv_label_create(card);
    lv_label_set_text(type_lbl, type_str);
    lv_obj_set_style_text_color(type_lbl, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(type_lbl, FONT_SMALL, 0);
    lv_obj_align(type_lbl, LV_ALIGN_LEFT_MID, 60, 15);
    
    /* Created time */
    char time_txt[48];
    sdcard_manager_format_time(g_files[idx].created_time, time_txt, sizeof(time_txt));
    char created_txt[64];
    snprintf(created_txt, sizeof(created_txt), "Created: %s", time_txt);
    lv_obj_t *time_lbl = lv_label_create(card);
    lv_label_set_text(time_lbl, created_txt);
    lv_obj_set_style_text_color(time_lbl, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(time_lbl, FONT_SMALL, 0);
    lv_obj_align(time_lbl, LV_ALIGN_LEFT_MID, 60, 40);
    
    /* Path */
    lv_obj_t *path_lbl = lv_label_create(card);
    lv_label_set_text(path_lbl, g_files[idx].path);
    lv_obj_set_style_text_color(path_lbl, lv_color_hex(0x9E9E9E), 0);
    lv_obj_set_style_text_font(path_lbl, FONT_SMALL, 0);
    lv_obj_align(path_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_label_set_long_mode(path_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(path_lbl, 360);
}

static void file_item_clicked(lv_event_t *e)
{
    /* Don't process if in edit mode */
    if (g_edit_mode) return;
    
    /* Check if the click was on a button (delete) */
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *current = lv_event_get_current_target(e);
    if (target != current) return;  /* Click was on a child object */
    
    int idx = (int)(intptr_t)lv_obj_get_user_data(current);
    if (idx < 0 || idx >= (int)g_file_count) return;
    
    if (g_browse_type == 3) {
        /* Media file - play with media player */
        /* Save path before any UI changes */
        char filepath[256];
        strncpy(filepath, g_files[idx].path, sizeof(filepath) - 1);
        filepath[sizeof(filepath) - 1] = '\0';
        
        /* Check if format is supported before playing */
        media_type_t mtype = media_player_get_type(filepath);
        if (mtype != MEDIA_TYPE_BMP && mtype != MEDIA_TYPE_AVI) {
            /* Show unsupported format message */
            ESP_LOGW(TAG, "Unsupported format: %s", filepath);
            
            /* Create a simple message box */
            static const char *btns[] = {"OK", ""};
            lv_obj_t *msgbox = lv_msgbox_create(NULL, "Unsupported", 
                                       "Only BMP and AVI supported", 
                                       btns, true);
            lv_obj_center(msgbox);
            return;
        }
        
        /* Record file access for recent files list */
        sdcard_manager_record_access(filepath);
        
        /* Initialize media player if needed */
        media_player_init();
        
        /* Play the media file - player UI will overlay on top */
        esp_err_t ret = media_player_play(filepath);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to play: %s", filepath);
            
            /* Show error message */
            static const char *err_btns[] = {"OK", ""};
            lv_obj_t *errmsg = lv_msgbox_create(NULL, "Error", 
                                       "Failed to open file", 
                                       err_btns, true);
            lv_obj_center(errmsg);
        }
    } else if (g_browse_type == 1) {
        /* Screenshot - show image preview */
        show_image_preview(idx);
    } else {
        /* Oscilloscope or Config - show file info */
        show_file_info(idx);
    }
}
