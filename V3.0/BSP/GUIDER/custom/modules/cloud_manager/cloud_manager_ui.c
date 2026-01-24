/**
 * @file cloud_manager_ui.c
 * @brief Cloud Platform Management UI Implementation
 */

#include "cloud_manager_ui.h"
#include "cloud_manager.h"
#include "cloud_activation_server.h"
#include "wifi_onenet.h"  /* For HTTP connection control */
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp32_p4_function_ev_board.h"  /* For bsp_display_lock/unlock */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

static const char *TAG = "CLOUD_UI";

/* Font declarations - using ShanHaiZhongXiaYeWuYuW font family */
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_18)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_20)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_22)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_24)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_28)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_30)
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_45)

/* Font defines for easy usage */
#define FONT_TITLE_LARGE   &lv_font_ShanHaiZhongXiaYeWuYuW_30
#define FONT_TITLE         &lv_font_ShanHaiZhongXiaYeWuYuW_28
#define FONT_SUBTITLE      &lv_font_ShanHaiZhongXiaYeWuYuW_24
#define FONT_LARGE         &lv_font_ShanHaiZhongXiaYeWuYuW_22
#define FONT_NORMAL        &lv_font_ShanHaiZhongXiaYeWuYuW_20
#define FONT_SMALL         &lv_font_ShanHaiZhongXiaYeWuYuW_18

/* Colors - Light theme matching sdcard_manager style */
#define COLOR_BG            lv_color_hex(0xF5F5F5)
#define COLOR_CARD          lv_color_hex(0xFFFFFF)
#define COLOR_PRIMARY       lv_color_hex(0x2196F3)
#define COLOR_SUCCESS       lv_color_hex(0x4CAF50)
#define COLOR_WARNING       lv_color_hex(0xFF9800)
#define COLOR_ERROR         lv_color_hex(0xF44336)
#define COLOR_TEXT          lv_color_hex(0x212121)
#define COLOR_TEXT_SEC      lv_color_hex(0x757575)
#define COLOR_BORDER        lv_color_hex(0xE0E0E0)
#define COLOR_DIVIDER       lv_color_hex(0xE0E0E0)

/* Layout constants - 右侧面板尺寸约 520x370 */
#define QR_CODE_SIZE        110   /* 二维码容器尺寸 */
#define QR_CODE_SPACING     15    /* 二维码之间的间距 */
#define BUTTON_HEIGHT       40    /* 按钮高度 */
#define PANEL_PADDING       15    /* 面板内边距 */

/* UI Objects */
static lv_obj_t *s_parent = NULL;
static lv_obj_t *s_main_cont = NULL;
static cloud_ui_state_t s_ui_state = CLOUD_UI_STATE_QR_CODE;

/* QR Code / Activation panel */
static lv_obj_t *s_qr_panel = NULL;
static lv_obj_t *s_qr_canvas = NULL;  /* WiFi QR code canvas */
static lv_obj_t *s_qr_url_canvas = NULL;  /* HTTP URL QR code canvas */
static lv_obj_t *s_qr_code_canvas = NULL;  /* Device Code QR code canvas */
static lv_obj_t *s_qr_label = NULL;
static lv_obj_t *s_qr_hint = NULL;
static lv_obj_t *s_qr_url_hint = NULL;
static lv_obj_t *s_qr_code_hint = NULL;
static lv_obj_t *s_btn_start_activation = NULL;
static lv_obj_t *s_btn_help = NULL;  /* Help/Instructions button */
static lv_obj_t *s_activation_status_label = NULL;
static lv_obj_t *s_help_popup = NULL;  /* Help popup window */
static bool s_activation_server_started = false;

/* Activated panel */
static lv_obj_t *s_activated_panel = NULL;
static lv_obj_t *s_congrats_label = NULL;
static lv_obj_t *s_device_code_label = NULL;
static lv_obj_t *s_guide_label = NULL;
static lv_obj_t *s_sync_status_cont = NULL;
static lv_obj_t *s_sync_onenet_label = NULL;
static lv_obj_t *s_sync_uploaded_label = NULL;
static lv_obj_t *s_sync_sensor_label = NULL;
static lv_obj_t *s_sync_device_label = NULL;
static lv_obj_t *s_sync_location_label = NULL;

/* Action buttons */
static lv_obj_t *s_btn_manual_upload = NULL;
static lv_obj_t *s_btn_back_to_qr = NULL;      /* 返回扫码页面按钮 */
static lv_obj_t *s_btn_wifi_location = NULL;   /* WiFi定位上报按钮 */
static lv_obj_t *s_btn_device_online = NULL;   /* 设备上线/下线按钮 */
static lv_obj_t *s_btn_refresh_sync = NULL;    /* 手动刷新同步状态按钮 */

/* Today stats panel */
static lv_obj_t *s_stats_panel = NULL;
static lv_obj_t *s_stats_count_label = NULL;
static lv_obj_t *s_stats_bytes_label = NULL;

/* Auto settings */
static lv_obj_t *s_auto_switch = NULL;
static lv_obj_t *s_interval_dropdown = NULL;

/* File browser popup */
static lv_obj_t *s_file_browser_popup = NULL;
static lv_obj_t *s_file_list = NULL;
static lv_obj_t *s_selected_file_label = NULL;
static lv_obj_t *s_btn_upload_file = NULL;
static lv_obj_t *s_btn_cancel_browse = NULL;

/* Upload progress popup */
static lv_obj_t *s_upload_popup = NULL;
static lv_obj_t *s_upload_file_label = NULL;
static lv_obj_t *s_upload_bar = NULL;
static lv_obj_t *s_upload_points_label = NULL;
static lv_obj_t *s_upload_status_label = NULL;
static lv_obj_t *s_upload_percent_label = NULL;  /* 进度百分比标签 */
static lv_obj_t *s_btn_cancel_upload = NULL;

/* Refresh button animation */
static lv_obj_t *s_refresh_icon_label = NULL;  /* 刷新图标标签，用于旋转动画 */
static lv_anim_t s_refresh_anim;               /* 刷新动画 */
static bool s_refresh_anim_running = false;    /* 动画是否正在运行 */

/* Error popup */
static lv_obj_t *s_error_popup = NULL;

/* Real-time clock display */
static lv_obj_t *s_time_label = NULL;
static lv_timer_t *s_time_update_timer = NULL;

/* Auto sync timer (30 seconds interval) */
static lv_timer_t *s_auto_sync_timer = NULL;
#define AUTO_SYNC_INTERVAL_MS  30000  /* 30秒自动同步间隔 */

/* Selected file info */
static char s_selected_file_path[64] = {0};
static uint32_t s_current_upload_task_id = 0;

/* Async activation result for thread-safe UI update */
static activation_result_t s_pending_activation_result;
static bool s_activation_success_pending = false;
static bool s_activation_fail_pending = false;

/* Forward declarations */
static void create_qr_panel(lv_obj_t *parent);
static void create_activated_panel(lv_obj_t *parent);
static void create_file_browser_popup(void);
static void create_upload_progress_popup(void);
static void draw_qr_code(const char *data);
static void draw_url_qr_code(const char *url);
static void draw_code_qr_code(const char *code);
static void auto_sync_timer_cb(lv_timer_t *timer);
static void scan_sd_files(void);
static void create_help_popup(void);
static void btn_help_cb(lv_event_t *e);

/* Event handlers */
static void btn_start_activation_cb(lv_event_t *e);
static void async_update_ui_after_stop(void *param);
static void btn_manual_upload_cb(lv_event_t *e);
static void btn_back_to_qr_cb(lv_event_t *e);       /* 返回扫码页面 */
static void btn_wifi_location_cb(lv_event_t *e);    /* WiFi定位上报 */
static void btn_device_online_cb(lv_event_t *e);    /* 设备上线/下线 */
static void btn_refresh_sync_cb(lv_event_t *e);     /* 手动刷新同步状态 */
static void auto_switch_cb(lv_event_t *e);
static void interval_dropdown_cb(lv_event_t *e);
static void file_item_cb(lv_event_t *e);
static void btn_upload_file_cb(lv_event_t *e);
static void btn_cancel_browse_cb(lv_event_t *e);
static void btn_cancel_upload_cb(lv_event_t *e);
static void btn_close_error_cb(lv_event_t *e);

/* Activation server status callback */
static void activation_status_callback(activation_status_t status, const activation_result_t *result);

/* 刷新图标旋转动画回调 */
static void refresh_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    if (obj != NULL) {
        lv_obj_set_style_transform_angle(obj, v * 10, 0);  /* v * 10 = 0~3600 (0~360度) */
    }
}

/* 动画完成回调 */
static void refresh_anim_ready_cb(lv_anim_t *a)
{
    (void)a;
    s_refresh_anim_running = false;
    /* 重置旋转角度 */
    if (s_refresh_icon_label != NULL) {
        lv_obj_set_style_transform_angle(s_refresh_icon_label, 0, 0);
    }
}

/* 启动刷新图标旋转动画 */
static void start_refresh_animation(void)
{
    if (s_refresh_icon_label == NULL || s_refresh_anim_running) return;
    
    s_refresh_anim_running = true;
    
    lv_anim_init(&s_refresh_anim);
    lv_anim_set_var(&s_refresh_anim, s_refresh_icon_label);
    lv_anim_set_exec_cb(&s_refresh_anim, refresh_anim_cb);
    lv_anim_set_values(&s_refresh_anim, 0, 360);  /* 0~360度 */
    lv_anim_set_time(&s_refresh_anim, 800);       /* 800ms 完成一圈 */
    lv_anim_set_repeat_count(&s_refresh_anim, 2); /* 旋转2圈 */
    lv_anim_set_ready_cb(&s_refresh_anim, refresh_anim_ready_cb);
    lv_anim_set_path_cb(&s_refresh_anim, lv_anim_path_ease_in_out);  /* 缓入缓出效果 */
    lv_anim_start(&s_refresh_anim);
}

/* 实时时间更新定时器回调（每秒更新） */
static void time_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    if (s_time_label == NULL || !lv_obj_is_valid(s_time_label)) {
        return;
    }
    
    /* 获取系统时间 */
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    /* 检查时间是否有效（年份 >= 2020） */
    bool time_valid = (timeinfo.tm_year + 1900 >= 2020);
    
    /* 格式化日期和时间，与主界面样式一致 */
    char date_str[32];
    char time_str[32];
    
    if (time_valid) {
        /* 日期格式: D,MMM,YYYY (例如: 19,Jan,2026) */
        const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        snprintf(date_str, sizeof(date_str), "%d,%s,%d",
                 timeinfo.tm_mday,
                 months[timeinfo.tm_mon],
                 timeinfo.tm_year + 1900);
        
        /* 时间格式: HH:MM */
        snprintf(time_str, sizeof(time_str), "%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min);
    } else {
        snprintf(date_str, sizeof(date_str), "--,---,----");
        snprintf(time_str, sizeof(time_str), "--:--");
    }
    
    /* 组合日期和时间，用逗号分隔 */
    char full_time_str[64];
    snprintf(full_time_str, sizeof(full_time_str), "%s,%s", date_str, time_str);
    
    lv_label_set_text(s_time_label, full_time_str);
    
    /* 根据时间有效性设置颜色：黑色（已同步）或红色（未同步） */
    if (time_valid) {
        lv_obj_set_style_text_color(s_time_label, lv_color_hex(0x000000), 0);  /* 黑色 */
    } else {
        lv_obj_set_style_text_color(s_time_label, lv_color_hex(0xff4444), 0);  /* 红色 */
    }
}

/* 自动同步定时器回调（30秒间隔） */
static void auto_sync_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    /* 仅在已激活状态下执行自动同步 */
    if (s_ui_state != CLOUD_UI_STATE_ACTIVATED) {
        return;
    }
    
    /* 检查activated_panel是否有效（屏幕切换时可能被删除） */
    if (s_activated_panel == NULL || !lv_obj_is_valid(s_activated_panel)) {
        ESP_LOGW(TAG, "Auto sync skipped: activated_panel invalid");
        return;
    }
    
    /* 更新同步状态显示 - 先检查对象有效性 */
    if (s_sync_onenet_label != NULL && lv_obj_is_valid(s_sync_onenet_label)) {
        cloud_sync_status_t sync_status;
        if (cloud_manager_get_sync_status(&sync_status) == ESP_OK) {
            cloud_manager_ui_update_sync_status(&sync_status);
        }
    }
    
    /* 更新统计信息显示 - 先检查对象有效性 */
    cloud_today_stats_t stats;
    if (cloud_manager_get_today_stats(&stats) == ESP_OK) {
        if (s_stats_count_label != NULL && lv_obj_is_valid(s_stats_count_label)) {
            char count_str[32];
            snprintf(count_str, sizeof(count_str), "%lu files", (unsigned long)stats.upload_count);
            lv_label_set_text(s_stats_count_label, count_str);
        }
        if (s_stats_bytes_label != NULL && lv_obj_is_valid(s_stats_bytes_label)) {
            char bytes_str[32];
            cloud_manager_format_bytes(stats.bytes_uploaded, bytes_str, sizeof(bytes_str));
            lv_label_set_text(s_stats_bytes_label, bytes_str);
        }
    }
    
    ESP_LOGI(TAG, "Auto sync timer triggered");
}

/* ==================== Public Functions ==================== */

esp_err_t cloud_manager_ui_create(lv_obj_t *parent)
{
    if (parent == NULL) return ESP_ERR_INVALID_ARG;
    
    s_parent = parent;
    
    /* 直接使用parent作为容器，不创建额外的中间容器 */
    /* s_main_cont 指向 parent，用于兼容现有代码 */
    s_main_cont = parent;
    
    /* 始终创建两个面板，根据状态显示/隐藏 */
    create_qr_panel(parent);
    create_activated_panel(parent);
    
    /* 根据激活状态显示对应面板 */
    if (cloud_manager_is_activated()) {
        s_ui_state = CLOUD_UI_STATE_ACTIVATED;
        lv_obj_add_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        s_ui_state = CLOUD_UI_STATE_QR_CODE;
        lv_obj_clear_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
    }
    
    /* Create popups (hidden by default) */
    create_file_browser_popup();
    create_upload_progress_popup();
    
    /* 创建自动同步定时器（30秒间隔），仅在已激活状态下启动 */
    if (cloud_manager_is_activated() && s_auto_sync_timer == NULL) {
        s_auto_sync_timer = lv_timer_create(auto_sync_timer_cb, AUTO_SYNC_INTERVAL_MS, NULL);
        ESP_LOGI(TAG, "Auto sync timer created (interval: %d ms)", AUTO_SYNC_INTERVAL_MS);
    }
    
    /* 创建实时时间更新定时器（每秒更新），仅在已激活状态下启动 */
    if (cloud_manager_is_activated() && s_time_update_timer == NULL) {
        s_time_update_timer = lv_timer_create(time_update_timer_cb, 1000, NULL);
        ESP_LOGI(TAG, "Time update timer created (interval: 1000 ms)");
    }
    
    ESP_LOGI(TAG, "Cloud Manager UI created");
    return ESP_OK;
}

void cloud_manager_ui_destroy(void)
{
    /* 停止实时时间更新定时器 */
    if (s_time_update_timer != NULL) {
        lv_timer_del(s_time_update_timer);
        s_time_update_timer = NULL;
    }
    
    /* 停止自动同步定时器 */
    if (s_auto_sync_timer != NULL) {
        lv_timer_del(s_auto_sync_timer);
        s_auto_sync_timer = NULL;
    }
    
    /* 停止刷新动画并清理 */
    if (s_refresh_anim_running) {
        lv_anim_del(s_refresh_icon_label, refresh_anim_cb);
        s_refresh_anim_running = false;
    }
    s_refresh_icon_label = NULL;
    s_time_label = NULL;
    
    /* 删除两个面板（不删除s_main_cont，因为它指向parent） */
    if (s_qr_panel != NULL) {
        lv_obj_del(s_qr_panel);
        s_qr_panel = NULL;
    }
    if (s_activated_panel != NULL) {
        lv_obj_del(s_activated_panel);
        s_activated_panel = NULL;
    }
    s_main_cont = NULL;
    if (s_file_browser_popup != NULL) {
        lv_obj_del(s_file_browser_popup);
        s_file_browser_popup = NULL;
    }
    if (s_upload_popup != NULL) {
        lv_obj_del(s_upload_popup);
        s_upload_popup = NULL;
    }
    if (s_error_popup != NULL) {
        lv_obj_del(s_error_popup);
        s_error_popup = NULL;
    }
    /* 销毁帮助弹窗 */
    if (s_help_popup != NULL) {
        lv_obj_del(s_help_popup);
        s_help_popup = NULL;
    }
    
    /* Reset all panel pointers */
    s_qr_panel = NULL;
    s_qr_canvas = NULL;
    s_qr_url_canvas = NULL;
    s_qr_code_canvas = NULL;
    s_qr_label = NULL;
    s_qr_hint = NULL;
    s_qr_url_hint = NULL;
    s_qr_code_hint = NULL;
    s_btn_help = NULL;
    s_help_popup = NULL;
    s_activated_panel = NULL;
    s_congrats_label = NULL;
    s_device_code_label = NULL;
    s_guide_label = NULL;
    s_sync_status_cont = NULL;
    s_sync_onenet_label = NULL;
    s_sync_uploaded_label = NULL;
    s_sync_sensor_label = NULL;
    s_sync_device_label = NULL;
    s_sync_location_label = NULL;
    s_btn_manual_upload = NULL;
    s_btn_back_to_qr = NULL;
    s_btn_wifi_location = NULL;
    s_btn_device_online = NULL;
    s_btn_refresh_sync = NULL;
    s_stats_panel = NULL;
    s_stats_count_label = NULL;
    s_stats_bytes_label = NULL;
    s_auto_switch = NULL;
    s_interval_dropdown = NULL;
    s_file_list = NULL;
    s_selected_file_label = NULL;
    s_btn_upload_file = NULL;
    s_btn_cancel_browse = NULL;
    s_upload_file_label = NULL;
    s_upload_bar = NULL;
    s_upload_points_label = NULL;
    s_upload_status_label = NULL;
    s_upload_percent_label = NULL;
    s_btn_cancel_upload = NULL;
    
    s_parent = NULL;
    ESP_LOGI(TAG, "Cloud Manager UI destroyed");
}

void cloud_manager_ui_show(void)
{
    /* 显示当前状态对应的面板 */
    if (s_ui_state == CLOUD_UI_STATE_ACTIVATED) {
        if (s_activated_panel != NULL) {
            lv_obj_clear_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_qr_panel != NULL) {
            lv_obj_add_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (s_qr_panel != NULL) {
            lv_obj_clear_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_activated_panel != NULL) {
            lv_obj_add_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void cloud_manager_ui_hide(void)
{
    /* 隐藏两个面板 */
    if (s_qr_panel != NULL) {
        lv_obj_add_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_activated_panel != NULL) {
        lv_obj_add_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void cloud_manager_ui_refresh(void)
{
    if (s_ui_state == CLOUD_UI_STATE_ACTIVATED) {
        /* 检查activated_panel是否有效（屏幕切换时可能被删除） */
        if (s_activated_panel == NULL || !lv_obj_is_valid(s_activated_panel)) {
            return;
        }
        
        /* 获取真实的OneNET在线状态 */
        bool is_online = onenet_is_device_online();
        
        /* 更新设备信息卡片中的在线状态显示 */
        if (s_congrats_label != NULL && lv_obj_is_valid(s_congrats_label)) {
            if (is_online) {
                lv_label_set_text(s_congrats_label, LV_SYMBOL_OK " Online");
                lv_obj_set_style_text_color(s_congrats_label, COLOR_SUCCESS, 0);
            } else {
                lv_label_set_text(s_congrats_label, LV_SYMBOL_CLOSE " Offline");
                lv_obj_set_style_text_color(s_congrats_label, COLOR_WARNING, 0);
            }
        }
        
        /* 更新同步状态卡片中的OneNet连接状态 */
        if (s_sync_onenet_label != NULL && lv_obj_is_valid(s_sync_onenet_label)) {
            if (is_online) {
                lv_label_set_text(s_sync_onenet_label, "OneNet: Connected");
                lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_SUCCESS, 0);
            } else {
                lv_label_set_text(s_sync_onenet_label, "OneNet: Disconnected");
                lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
            }
        }
        
        /* 更新WiFi定位按钮状态 */
        if (s_btn_wifi_location != NULL && lv_obj_is_valid(s_btn_wifi_location)) {
            lv_obj_t *label = lv_obj_get_child(s_btn_wifi_location, 0);
            if (label != NULL && lv_obj_is_valid(label)) {
                if (wifi_location_is_reporting()) {
                    lv_label_set_text(label, LV_SYMBOL_CLOSE " Stop");
                    lv_obj_set_style_bg_color(s_btn_wifi_location, COLOR_WARNING, 0);
                } else {
                    lv_label_set_text(label, LV_SYMBOL_GPS " Locate");
                    lv_obj_set_style_bg_color(s_btn_wifi_location, COLOR_PRIMARY, 0);
                }
            }
        }
        
        /* 更新设备上线/下线按钮状态 */
        if (s_btn_device_online != NULL && lv_obj_is_valid(s_btn_device_online)) {
            lv_obj_t *label = lv_obj_get_child(s_btn_device_online, 0);
            if (label != NULL && lv_obj_is_valid(label)) {
                if (is_online) {
                    lv_label_set_text(label, LV_SYMBOL_OK " Online");
                    lv_obj_set_style_bg_color(s_btn_device_online, COLOR_SUCCESS, 0);
                } else {
                    lv_label_set_text(label, LV_SYMBOL_WIFI " Connect");
                    lv_obj_set_style_bg_color(s_btn_device_online, COLOR_PRIMARY, 0);
                }
            }
        }
        
        /* 更新同步状态 */
        cloud_sync_status_t status;
        cloud_manager_get_sync_status(&status);
        cloud_manager_ui_update_sync_status(&status);

        /* 更新今日统计 */
        cloud_today_stats_t stats;
        cloud_manager_get_today_stats(&stats);
        cloud_manager_ui_update_today_stats(&stats);
        
        /* 更新定位信息显示 */
        cloud_manager_ui_update_location();
    }
}

/* 定位信息更新 - 使用缓存的定位结果 */
void cloud_manager_ui_update_location(void)
{
    if (s_ui_state != CLOUD_UI_STATE_ACTIVATED) {
        return;
    }
    
    /* 检查activated_panel是否有效 */
    if (s_activated_panel == NULL || !lv_obj_is_valid(s_activated_panel)) {
        return;
    }
    
    /* 从缓存获取最后一次定位结果 */
    location_info_t location = {0};
    esp_err_t ret = wifi_location_get_last_result(&location);
    
    if (ret == ESP_OK && location.valid) {
        /* 更新定位显示 */
        if (s_sync_location_label != NULL && lv_obj_is_valid(s_sync_location_label)) {
            char text[80];
            snprintf(text, sizeof(text), "Loc: %.4f, %.4f", 
                     location.longitude, location.latitude);
            lv_label_set_text(s_sync_location_label, text);
            lv_obj_set_style_text_color(s_sync_location_label, COLOR_SUCCESS, 0);
        }
    } else {
        /* 定位失败或无数据 */
        if (s_sync_location_label != NULL && lv_obj_is_valid(s_sync_location_label)) {
            /* 检查是否正在上报 */
            if (wifi_location_is_reporting()) {
                lv_label_set_text(s_sync_location_label, "Location: Reporting...");
                lv_obj_set_style_text_color(s_sync_location_label, COLOR_PRIMARY, 0);
            } else {
                lv_label_set_text(s_sync_location_label, "Location: --");
                lv_obj_set_style_text_color(s_sync_location_label, COLOR_TEXT_SEC, 0);
            }
        }
    }
}

void cloud_manager_ui_set_state(cloud_ui_state_t state)
{
    s_ui_state = state;
}

cloud_ui_state_t cloud_manager_ui_get_state(void)
{
    return s_ui_state;
}

/* ==================== QR Code Display ==================== */

void cloud_manager_ui_show_qr_code(void)
{
    if (s_qr_panel != NULL) {
        lv_obj_clear_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_activated_panel != NULL) {
        lv_obj_add_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
    }
    
    /* Generate and draw QR code */
    char qr_data[256];
    cloud_manager_generate_qr_data(qr_data, sizeof(qr_data));
    draw_qr_code(qr_data);
    
    s_ui_state = CLOUD_UI_STATE_QR_CODE;
}

void cloud_manager_ui_show_activating(void)
{
    if (s_qr_hint != NULL) {
        lv_label_set_text(s_qr_hint, "Activating device...");
        lv_obj_set_style_text_color(s_qr_hint, COLOR_PRIMARY, 0);
    }
    s_ui_state = CLOUD_UI_STATE_ACTIVATING;
}

/* ==================== Activated Status ==================== */

void cloud_manager_ui_show_activated(const char *device_code)
{
    ESP_LOGI(TAG, "Switching to activated panel, device_code=%s", 
             device_code ? device_code : "(null)");
    
    /* Hide QR panel */
    if (s_qr_panel != NULL) {
        lv_obj_add_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
    }
    
    /* Create activated panel if not exists */
    if (s_activated_panel == NULL && s_parent != NULL) {
        create_activated_panel(s_parent);
    }
    
    /* Show activated panel */
    if (s_activated_panel != NULL) {
        lv_obj_clear_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
    }
    
    /* 检查activated_panel是否有效 */
    if (s_activated_panel == NULL || !lv_obj_is_valid(s_activated_panel)) {
        ESP_LOGW(TAG, "show_activated: activated_panel invalid");
        return;
    }
    
    /* 获取最新的设备信息 */
    cloud_device_info_t info;
    cloud_manager_get_device_info(&info);
    
    /* 更新设备名称显示 */
    if (s_device_code_label != NULL && lv_obj_is_valid(s_device_code_label)) {
        const char *name = (device_code != NULL && strlen(device_code) > 0) ? device_code :
                          (strlen(info.device_name) > 0 ? info.device_name : "ExDebugTool");
        char code_text[80];
        snprintf(code_text, sizeof(code_text), LV_SYMBOL_OK " %s", name);
        lv_label_set_text(s_device_code_label, code_text);
        ESP_LOGI(TAG, "Device name label updated: %s", code_text);
    }
    
    /* 更新在线状态显示 - 使用真实的OneNET在线状态 */
    bool is_online = onenet_is_device_online();
    if (s_congrats_label != NULL && lv_obj_is_valid(s_congrats_label)) {
        if (is_online) {
            lv_label_set_text(s_congrats_label, LV_SYMBOL_OK " Online");
            lv_obj_set_style_text_color(s_congrats_label, COLOR_SUCCESS, 0);
        } else {
            lv_label_set_text(s_congrats_label, LV_SYMBOL_CLOSE " Offline");
            lv_obj_set_style_text_color(s_congrats_label, COLOR_WARNING, 0);
        }
    }
    
    /* 更新OneNet连接状态标签 */
    if (s_sync_onenet_label != NULL && lv_obj_is_valid(s_sync_onenet_label)) {
        if (is_online) {
            lv_label_set_text(s_sync_onenet_label, "OneNet: Connected");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_SUCCESS, 0);
        } else {
            lv_label_set_text(s_sync_onenet_label, "OneNet: Disconnected");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
        }
    }
    
    /* 更新设备ID显示 */
    if (s_guide_label != NULL && lv_obj_is_valid(s_guide_label)) {
        char did_txt[48];
        snprintf(did_txt, sizeof(did_txt), "DID: %s", 
                 strlen(info.device_id) > 0 ? info.device_id : "--");
        lv_label_set_text(s_guide_label, did_txt);
    }
    
    ESP_LOGI(TAG, "Product ID: %s, Device ID: %s", info.product_id, info.device_id);
    
    /* 更新WiFi定位按钮状态 */
    if (s_btn_wifi_location != NULL) {
        lv_obj_t *label = lv_obj_get_child(s_btn_wifi_location, 0);
        if (label != NULL) {
            if (wifi_location_is_reporting()) {
                lv_label_set_text(label, LV_SYMBOL_CLOSE " Stop");
                lv_obj_set_style_bg_color(s_btn_wifi_location, COLOR_WARNING, 0);
            } else {
                lv_label_set_text(label, LV_SYMBOL_GPS " Locate");
                lv_obj_set_style_bg_color(s_btn_wifi_location, COLOR_PRIMARY, 0);
            }
        }
    }
    
    /* 更新设备上线/下线按钮状态 */
    if (s_btn_device_online != NULL) {
        lv_obj_t *label = lv_obj_get_child(s_btn_device_online, 0);
        if (label != NULL) {
            if (is_online) {
                lv_label_set_text(label, LV_SYMBOL_OK " Online");
                lv_obj_set_style_bg_color(s_btn_device_online, COLOR_SUCCESS, 0);
            } else {
                lv_label_set_text(label, LV_SYMBOL_WIFI " Connect");
                lv_obj_set_style_bg_color(s_btn_device_online, COLOR_PRIMARY, 0);
            }
        }
    }
    
    s_ui_state = CLOUD_UI_STATE_ACTIVATED;
    cloud_manager_ui_refresh();
    
    ESP_LOGI(TAG, "Activated panel is now visible");
}

void cloud_manager_ui_update_sync_status(const cloud_sync_status_t *status)
{
    if (status == NULL) return;
    
    /* 检查activated_panel是否有效（屏幕切换时可能被删除） */
    if (s_activated_panel == NULL || !lv_obj_is_valid(s_activated_panel)) {
        return;
    }
    
    /* Update OneNet connection status - 使用真实的OneNET在线状态 */
    if (s_sync_onenet_label != NULL && lv_obj_is_valid(s_sync_onenet_label)) {
        bool is_online = onenet_is_device_online();
        if (is_online) {
            lv_label_set_text(s_sync_onenet_label, "OneNet: Connected");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_SUCCESS, 0);
        } else {
            lv_label_set_text(s_sync_onenet_label, "OneNet: Disconnected");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
        }
    }
    
    /* Format last sync times */
    char time_str[32];
    
    /* Oscilloscope data */
    if (s_sync_sensor_label != NULL && lv_obj_is_valid(s_sync_sensor_label)) {
        cloud_manager_format_last_sync(SYNC_TYPE_OSCILLOSCOPE, time_str, sizeof(time_str));
        char text[64];
        snprintf(text, sizeof(text), "Oscilloscope: %s", time_str);
        lv_label_set_text(s_sync_sensor_label, text);
    }
    
    /* Multimeter data */
    if (s_sync_uploaded_label != NULL && lv_obj_is_valid(s_sync_uploaded_label)) {
        cloud_manager_format_last_sync(SYNC_TYPE_SENSOR, time_str, sizeof(time_str));
        char text[64];
        snprintf(text, sizeof(text), "Multimeter: %s", time_str);
        lv_label_set_text(s_sync_uploaded_label, text);
    }
    
    /* Power data */
    if (s_sync_device_label != NULL && lv_obj_is_valid(s_sync_device_label)) {
        cloud_manager_format_last_sync(SYNC_TYPE_DEVICE_STATUS, time_str, sizeof(time_str));
        char text[64];
        snprintf(text, sizeof(text), "Power: %s", time_str);
        lv_label_set_text(s_sync_device_label, text);
    }
    
    /* Location info */
    if (s_sync_location_label != NULL && lv_obj_is_valid(s_sync_location_label)) {
        cloud_manager_format_last_sync(SYNC_TYPE_LOCATION, time_str, sizeof(time_str));
        char text[64];
        snprintf(text, sizeof(text), "Location: %s", time_str);
        lv_label_set_text(s_sync_location_label, text);
    }
}

void cloud_manager_ui_update_today_stats(const cloud_today_stats_t *stats)
{
    if (stats == NULL) return;
    
    /* 检查activated_panel是否有效 */
    if (s_activated_panel == NULL || !lv_obj_is_valid(s_activated_panel)) {
        return;
    }
    
    if (s_stats_count_label != NULL && lv_obj_is_valid(s_stats_count_label)) {
        char text[64];
        snprintf(text, sizeof(text), "Today: %lu",
                 (unsigned long)stats->upload_count);
        lv_label_set_text(s_stats_count_label, text);
    }
    
    if (s_stats_bytes_label != NULL && lv_obj_is_valid(s_stats_bytes_label)) {
        char bytes_str[16];
        cloud_manager_format_bytes(stats->bytes_uploaded, bytes_str, sizeof(bytes_str));
        lv_label_set_text(s_stats_bytes_label, bytes_str);
    }
}

/* ==================== File Browser ==================== */

void cloud_manager_ui_show_file_browser(void)
{
    if (s_file_browser_popup == NULL) {
        create_file_browser_popup();
    }
    
    scan_sd_files();
    
    lv_obj_clear_flag(s_file_browser_popup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_file_browser_popup);
    
    s_ui_state = CLOUD_UI_STATE_FILE_BROWSER;
}

void cloud_manager_ui_hide_file_browser(void)
{
    if (s_file_browser_popup != NULL) {
        lv_obj_add_flag(s_file_browser_popup, LV_OBJ_FLAG_HIDDEN);
    }
    s_ui_state = CLOUD_UI_STATE_ACTIVATED;
}

void cloud_manager_ui_refresh_file_list(void)
{
    scan_sd_files();
}

/* ==================== Upload Progress ==================== */

void cloud_manager_ui_show_upload_progress(const char *file_name)
{
    if (s_upload_popup == NULL) {
        create_upload_progress_popup();
    }
    
    if (s_upload_file_label != NULL && file_name != NULL) {
        char text[128];
        snprintf(text, sizeof(text), "File: %s", file_name);
        lv_label_set_text(s_upload_file_label, text);
    }
    
    if (s_upload_bar != NULL) {
        lv_bar_set_value(s_upload_bar, 0, LV_ANIM_OFF);
    }
    
    if (s_upload_points_label != NULL) {
        lv_label_set_text(s_upload_points_label, "Data: 0/0");
    }
    
    if (s_upload_status_label != NULL) {
        lv_label_set_text(s_upload_status_label, "Status: Preparing...");
    }
    
    lv_obj_clear_flag(s_upload_popup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_upload_popup);
    
    s_ui_state = CLOUD_UI_STATE_UPLOADING;
}

void cloud_manager_ui_update_upload_progress(uint32_t progress, 
                                              uint32_t current_points,
                                              uint32_t total_points,
                                              const char *status)
{
    ESP_LOGI(TAG, "Updating UI: progress=%lu%%, bar=%p, percent=%p", 
             (unsigned long)progress, s_upload_bar, s_upload_percent_label);
    
    if (s_upload_bar != NULL) {
        lv_bar_set_value(s_upload_bar, progress, LV_ANIM_ON);
        
        /* 更新进度条颜色 - 根据进度变化 */
        if (progress < 30) {
            lv_obj_set_style_bg_color(s_upload_bar, COLOR_WARNING, LV_PART_INDICATOR);
        } else if (progress < 70) {
            lv_obj_set_style_bg_color(s_upload_bar, COLOR_PRIMARY, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(s_upload_bar, COLOR_SUCCESS, LV_PART_INDICATOR);
        }
    }
    
    /* 更新百分比标签 */
    if (s_upload_percent_label != NULL) {
        char percent_text[16];
        snprintf(percent_text, sizeof(percent_text), "%lu%%", (unsigned long)progress);
        lv_label_set_text(s_upload_percent_label, percent_text);
    }
    
    if (s_upload_points_label != NULL) {
        char text[64];
        snprintf(text, sizeof(text), "Data points: %lu / %lu",
                 (unsigned long)current_points, (unsigned long)total_points);
        lv_label_set_text(s_upload_points_label, text);
    }
    
    if (s_upload_status_label != NULL && status != NULL) {
        lv_label_set_text(s_upload_status_label, status);
    }
}

void cloud_manager_ui_hide_upload_progress(void)
{
    if (s_upload_popup != NULL) {
        lv_obj_add_flag(s_upload_popup, LV_OBJ_FLAG_HIDDEN);
    }
    s_ui_state = CLOUD_UI_STATE_ACTIVATED;
}

void cloud_manager_ui_show_upload_result(bool success, const char *message)
{
    ESP_LOGI(TAG, "show_upload_result: success=%d, msg=%s", success, message ? message : "null");
    cloud_manager_ui_hide_upload_progress();
    ESP_LOGI(TAG, "Progress popup hidden, showing result popup...");
    
    if (success) {
        cloud_manager_ui_show_error(0, message);
    } else {
        cloud_manager_ui_show_error(5, message);
    }
    ESP_LOGI(TAG, "Result popup shown");
}

/* ==================== Settings ==================== */

void cloud_manager_ui_set_auto_upload(bool enabled)
{
    if (s_auto_switch != NULL) {
        if (enabled) {
            lv_obj_add_state(s_auto_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(s_auto_switch, LV_STATE_CHECKED);
        }
    }
}

void cloud_manager_ui_set_interval(uint32_t minutes)
{
    if (s_interval_dropdown != NULL) {
        /* 选项: 1min, 5min, 10min, 30min, 1hour */
        uint16_t idx = 0;  /* 默认1分钟 */
        if (minutes <= 1) idx = 0;        /* 1min */
        else if (minutes <= 5) idx = 1;   /* 5min */
        else if (minutes <= 10) idx = 2;  /* 10min */
        else if (minutes <= 30) idx = 3;  /* 30min */
        else idx = 4;                     /* 1hour */
        lv_dropdown_set_selected(s_interval_dropdown, idx);
    }
}

/* ==================== Error Handling ==================== */

void cloud_manager_ui_show_error(int error_type, const char *message)
{
    ESP_LOGI(TAG, "show_error: type=%d, msg=%s, popup=%p", error_type, message ? message : "null", s_error_popup);
    
    /* Create error popup if needed */
    if (s_error_popup == NULL) {
        s_error_popup = lv_obj_create(lv_layer_top());
        lv_obj_set_size(s_error_popup, 360, 180);
        lv_obj_center(s_error_popup);
        lv_obj_set_scrollbar_mode(s_error_popup, LV_SCROLLBAR_MODE_OFF);  /* 禁用滚动条 */
        lv_obj_clear_flag(s_error_popup, LV_OBJ_FLAG_SCROLLABLE);  /* 禁止滚动 */
        lv_obj_set_style_bg_color(s_error_popup, COLOR_CARD, 0);
        lv_obj_set_style_border_width(s_error_popup, 2, 0);
        lv_obj_set_style_border_color(s_error_popup, error_type == 0 ? COLOR_SUCCESS : COLOR_ERROR, 0);
        lv_obj_set_style_radius(s_error_popup, 12, 0);
        lv_obj_set_style_shadow_width(s_error_popup, 20, 0);
        lv_obj_set_style_shadow_opa(s_error_popup, LV_OPA_30, 0);
        
        lv_obj_t *icon = lv_label_create(s_error_popup);
        lv_obj_set_pos(icon, 20, 15);
        lv_obj_set_style_text_font(icon, FONT_TITLE_LARGE, 0);
        
        lv_obj_t *msg = lv_label_create(s_error_popup);
        lv_obj_set_pos(msg, 70, 20);
        lv_obj_set_width(msg, 270);
        lv_obj_set_style_text_font(msg, FONT_NORMAL, 0);
        lv_obj_set_style_text_color(msg, COLOR_TEXT, 0);  /* 使用深色文字，在白色背景上可见 */
        lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
        
        lv_obj_t *btn = lv_btn_create(s_error_popup);
        lv_obj_set_size(btn, 100, 40);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -15);
        lv_obj_set_style_bg_color(btn, COLOR_PRIMARY, 0);
        lv_obj_add_event_cb(btn, btn_close_error_cb, LV_EVENT_CLICKED, NULL);
        
        lv_obj_t *btn_label = lv_label_create(btn);
        lv_label_set_text(btn_label, "OK");
        lv_obj_center(btn_label);
        lv_obj_set_style_text_font(btn_label, FONT_NORMAL, 0);
        lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    }
    
    /* Update icon based on error type */
    lv_obj_t *icon = lv_obj_get_child(s_error_popup, 0);
    lv_obj_t *msg = lv_obj_get_child(s_error_popup, 1);
    
    const char *icon_text = "X";
    lv_color_t border_color = COLOR_ERROR;
    
    switch (error_type) {
        case 0:  /* Success */
            icon_text = "OK";
            border_color = COLOR_SUCCESS;
            lv_obj_set_style_text_color(icon, COLOR_SUCCESS, 0);
            break;
        case 1:  /* SD card not inserted */
            icon_text = "SD";
            lv_obj_set_style_text_color(icon, COLOR_WARNING, 0);
            break;
        case 2:  /* File read error */
            icon_text = "!";
            lv_obj_set_style_text_color(icon, COLOR_ERROR, 0);
            break;
        case 3:  /* Format not supported */
            icon_text = "?";
            lv_obj_set_style_text_color(icon, COLOR_WARNING, 0);
            break;
        case 4:  /* Network disconnected */
            icon_text = "NET";
            lv_obj_set_style_text_color(icon, COLOR_ERROR, 0);
            break;
        case 5:  /* Upload failed */
        default:
            icon_text = "X";
            lv_obj_set_style_text_color(icon, COLOR_ERROR, 0);
            break;
    }
    
    lv_label_set_text(icon, icon_text);
    lv_obj_set_style_border_color(s_error_popup, border_color, 0);
    
    if (message != NULL) {
        lv_label_set_text(msg, message);
    }
    
    lv_obj_clear_flag(s_error_popup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_error_popup);
    ESP_LOGI(TAG, "Error popup displayed: type=%d", error_type);
}

/* ==================== Callbacks ==================== */

void cloud_manager_ui_activation_callback(cloud_activation_state_t state, const char *device_code)
{
    switch (state) {
        case CLOUD_STATE_ACTIVATING:
            cloud_manager_ui_show_activating();
            break;
        case CLOUD_STATE_ACTIVATED:
            cloud_manager_ui_show_activated(device_code);
            break;
        case CLOUD_STATE_ERROR:
            cloud_manager_ui_show_error(5, "Activation failed, please retry");
            break;
        default:
            break;
    }
}

/* 异步UI更新数据结构 */
typedef struct {
    uint32_t task_id;
    uint32_t progress;
    uint32_t uploaded_bytes;
    uint32_t current_points;
    uint32_t total_points;
    char status[32];
} async_progress_data_t;

typedef struct {
    uint32_t task_id;
    bool success;
    char error_msg[64];
    uint32_t data_points;
} async_complete_data_t;

static async_progress_data_t s_async_progress = {0};
static async_complete_data_t s_async_complete = {0};

/* 异步进度更新回调 - 在LVGL主线程中执行 */
static void async_update_progress(void *param)
{
    (void)param;
    ESP_LOGI(TAG, "async_update_progress called: progress=%lu%%, status=%s", 
             (unsigned long)s_async_progress.progress, s_async_progress.status);
    cloud_manager_ui_update_upload_progress(
        s_async_progress.progress,
        s_async_progress.current_points,
        s_async_progress.total_points,
        s_async_progress.status
    );
}

/* 异步完成更新回调 - 在LVGL主线程中执行 */
static void async_update_complete(void *param)
{
    (void)param;
    s_current_upload_task_id = 0;
    
    if (s_async_complete.success) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Upload complete! %lu data items",
                 (unsigned long)s_async_complete.data_points);
        cloud_manager_ui_show_upload_result(true, msg);
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "Upload failed: %s",
                 strlen(s_async_complete.error_msg) > 0 ? s_async_complete.error_msg : "Unknown error");
        cloud_manager_ui_show_upload_result(false, msg);
    }
    
    /* Refresh status */
    cloud_manager_ui_refresh();
}

void cloud_manager_ui_upload_progress_callback(uint32_t task_id, uint32_t progress, uint32_t uploaded_bytes)
{
    ESP_LOGI(TAG, "Progress callback: task_id=%lu, progress=%lu%%, bytes=%lu, current_task=%lu", 
             (unsigned long)task_id, (unsigned long)progress, (unsigned long)uploaded_bytes,
             (unsigned long)s_current_upload_task_id);
    
    if (task_id == s_current_upload_task_id) {
        /* 确定状态文本 */
        const char *status_text;
        if (progress < 20) {
            status_text = "Reading file...";
        } else if (progress < 50) {
            status_text = "Preparing...";
        } else if (progress < 80) {
            status_text = "Uploading...";
        } else {
            status_text = "Finishing...";
        }
        
        /* 直接使用bsp_display_lock保护LVGL调用，确保线程安全 */
        if (bsp_display_lock(100)) {
            ESP_LOGI(TAG, "Updating UI directly: progress=%lu%%, bar=%p", 
                     (unsigned long)progress, s_upload_bar);
            cloud_manager_ui_update_upload_progress(progress, 0, 0, status_text);
            bsp_display_unlock();
        } else {
            ESP_LOGW(TAG, "Failed to acquire display lock for progress update");
        }
    } else {
        ESP_LOGW(TAG, "Task ID mismatch: expected %lu, got %lu", 
                 (unsigned long)s_current_upload_task_id, (unsigned long)task_id);
    }
}

void cloud_manager_ui_upload_complete_callback(uint32_t task_id, bool success, const char *error_msg)
{
    ESP_LOGI(TAG, "Complete callback: task_id=%lu, success=%d, error=%s", 
             (unsigned long)task_id, success, error_msg ? error_msg : "none");
    
    if (task_id == s_current_upload_task_id) {
        ESP_LOGI(TAG, "Task ID matched, acquiring display lock...");
        
        /* 注意：不要在这里调用 cloud_manager_get_upload_task()，因为会导致死锁！
         * 完成回调是在 cloud_manager_execute_pending_upload() 持有 s_upload_mutex 时调用的，
         * 而 cloud_manager_get_upload_task() 也需要获取同一个互斥锁。
         */
        
        /* 直接使用bsp_display_lock保护LVGL调用 */
        if (bsp_display_lock(2000)) {
            ESP_LOGI(TAG, "Display lock acquired, showing result...");
            s_current_upload_task_id = 0;
            
            if (success) {
                const char *msg = "Upload complete!";
                ESP_LOGI(TAG, "Showing success result");
                cloud_manager_ui_show_upload_result(true, msg);
            } else {
                char msg[128];
                snprintf(msg, sizeof(msg), "Upload failed: %s",
                         (error_msg && strlen(error_msg) > 0) ? error_msg : "Unknown error");
                ESP_LOGI(TAG, "Showing error result: %s", msg);
                cloud_manager_ui_show_upload_result(false, msg);
            }
            
            /* Refresh status */
            cloud_manager_ui_refresh();
            bsp_display_unlock();
            ESP_LOGI(TAG, "Display lock released");
        } else {
            ESP_LOGW(TAG, "Failed to acquire display lock for complete update (timeout 2000ms)");
        }
    } else {
        ESP_LOGW(TAG, "Task ID mismatch in complete callback: expected %lu, got %lu", 
                 (unsigned long)s_current_upload_task_id, (unsigned long)task_id);
    }
}

/* ==================== Internal Functions ==================== */

/*
 * ================================================================
 *         云平台激活界面 (Cloud Activation Panel) - 重新设计
 * ================================================================
 *
 *  布局设计 (右侧面板约 520x370):
 *  +--------------------------------------------------+
 *  |              Cloud Activation                    |  <- 标题
 *  |           Device Code: ESP32P4_XXXX              |  <- 设备码
 *  +--------------------------------------------------+
 *  |  +--------+  15px  +--------+  15px  +--------+  |
 *  |  | WiFi   |        |  URL   |        | Code   |  |  <- 三个二维码
 *  |  |  QR    |        |   QR   |        |  QR    |  |     110x110
 *  |  +--------+        +--------+        +--------+  |
 *  |  1.Connect         2.Browser         3.Code      |  <- 标签
 *  +--------------------------------------------------+
 *  |  [====== Start Server ======] [Instructions]     |  <- 按钮行
 *  +--------------------------------------------------+
 *  |        Click button to start server              |  <- 状态
 *  +--------------------------------------------------+
 */
static void create_qr_panel(lv_obj_t *parent)
{
    /* 计算布局参数 */
    const lv_coord_t qr_size = QR_CODE_SIZE;           /* 110 */
    const lv_coord_t qr_spacing = QR_CODE_SPACING;     /* 15 */
    const lv_coord_t total_qr_width = qr_size * 3 + qr_spacing * 2;  /* 360 */
    
    s_qr_panel = lv_obj_create(parent);
    lv_obj_set_size(s_qr_panel, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(s_qr_panel, 0, 0);
    lv_obj_set_style_bg_opa(s_qr_panel, LV_OPA_TRANSP, 0);  /* 透明背景，去掉灰色 */
    lv_obj_set_style_border_width(s_qr_panel, 0, 0);
    lv_obj_set_style_pad_all(s_qr_panel, PANEL_PADDING, 0);
    lv_obj_set_scrollbar_mode(s_qr_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_qr_panel, LV_OBJ_FLAG_SCROLLABLE);  /* 禁止滚动 */
    
    /* ===== Row 1: 标题 (居中) ===== */
    s_qr_label = lv_label_create(s_qr_panel);
    lv_label_set_text(s_qr_label, "Cloud Activation");
    lv_obj_set_style_text_font(s_qr_label, FONT_SUBTITLE, 0);
    lv_obj_set_style_text_color(s_qr_label, COLOR_TEXT, 0);
    lv_obj_align(s_qr_label, LV_ALIGN_TOP_MID, 0, 0);
    
    /* ===== Row 2: 设备码 (居中) - will be updated after activation ===== */
    lv_obj_t *code_label = lv_label_create(s_qr_panel);
    lv_label_set_text(code_label, "Device Code: (After Activation)");
    lv_obj_set_style_text_font(code_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(code_label, COLOR_TEXT_SEC, 0);
    lv_obj_align_to(code_label, s_qr_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    
    /* ===== Row 3: 三个二维码容器 (水平居中，等间距) ===== */
    /* 创建二维码行容器 */
    lv_obj_t *qr_row = lv_obj_create(s_qr_panel);
    lv_obj_set_size(qr_row, total_qr_width, qr_size);
    lv_obj_align_to(qr_row, code_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_opa(qr_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(qr_row, 0, 0);
    lv_obj_set_style_pad_all(qr_row, 0, 0);
    lv_obj_set_scrollbar_mode(qr_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(qr_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(qr_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    /* WiFi QR Code (左) */
    s_qr_canvas = lv_obj_create(qr_row);
    lv_obj_set_size(s_qr_canvas, qr_size, qr_size);
    lv_obj_set_style_bg_color(s_qr_canvas, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_qr_canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_qr_canvas, 2, 0);
    lv_obj_set_style_border_color(s_qr_canvas, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(s_qr_canvas, 8, 0);
    lv_obj_set_style_pad_all(s_qr_canvas, 5, 0);
    lv_obj_set_scrollbar_mode(s_qr_canvas, LV_SCROLLBAR_MODE_OFF);
    
    s_qr_hint = lv_label_create(s_qr_canvas);
    lv_label_set_text(s_qr_hint, "WiFi\nQR");
    lv_obj_center(s_qr_hint);
    lv_obj_set_style_text_font(s_qr_hint, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_qr_hint, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_align(s_qr_hint, LV_TEXT_ALIGN_CENTER, 0);
    
    /* HTTP URL QR Code (中) */
    s_qr_url_canvas = lv_obj_create(qr_row);
    lv_obj_set_size(s_qr_url_canvas, qr_size, qr_size);
    lv_obj_set_style_bg_color(s_qr_url_canvas, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_qr_url_canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_qr_url_canvas, 2, 0);
    lv_obj_set_style_border_color(s_qr_url_canvas, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(s_qr_url_canvas, 8, 0);
    lv_obj_set_style_pad_all(s_qr_url_canvas, 5, 0);
    lv_obj_set_scrollbar_mode(s_qr_url_canvas, LV_SCROLLBAR_MODE_OFF);
    
    s_qr_url_hint = lv_label_create(s_qr_url_canvas);
    lv_label_set_text(s_qr_url_hint, "URL\nQR");
    lv_obj_center(s_qr_url_hint);
    lv_obj_set_style_text_font(s_qr_url_hint, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_qr_url_hint, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_align(s_qr_url_hint, LV_TEXT_ALIGN_CENTER, 0);
    
    /* Device Code QR Code (右) */
    s_qr_code_canvas = lv_obj_create(qr_row);
    lv_obj_set_size(s_qr_code_canvas, qr_size, qr_size);
    lv_obj_set_style_bg_color(s_qr_code_canvas, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_qr_code_canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_qr_code_canvas, 2, 0);
    lv_obj_set_style_border_color(s_qr_code_canvas, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(s_qr_code_canvas, 8, 0);
    lv_obj_set_style_pad_all(s_qr_code_canvas, 5, 0);
    lv_obj_set_scrollbar_mode(s_qr_code_canvas, LV_SCROLLBAR_MODE_OFF);
    
    s_qr_code_hint = lv_label_create(s_qr_code_canvas);
    lv_label_set_text(s_qr_code_hint, "Code\nQR");
    lv_obj_center(s_qr_code_hint);
    lv_obj_set_style_text_font(s_qr_code_hint, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_qr_code_hint, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_align(s_qr_code_hint, LV_TEXT_ALIGN_CENTER, 0);
    
    /* ===== Row 4: 二维码标签行 (与二维码对齐) ===== */
    lv_obj_t *label_row = lv_obj_create(s_qr_panel);
    lv_obj_set_size(label_row, total_qr_width, 20);
    lv_obj_align_to(label_row, qr_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_opa(label_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(label_row, 0, 0);
    lv_obj_set_style_pad_all(label_row, 0, 0);
    lv_obj_set_scrollbar_mode(label_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(label_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(label_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    /* 三个标签 */
    lv_obj_t *wifi_label = lv_label_create(label_row);
    lv_obj_set_width(wifi_label, qr_size);
    lv_label_set_text(wifi_label, "1. WiFi");
    lv_obj_set_style_text_font(wifi_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(wifi_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_align(wifi_label, LV_TEXT_ALIGN_CENTER, 0);
    
    lv_obj_t *url_label = lv_label_create(label_row);
    lv_obj_set_width(url_label, qr_size);
    lv_label_set_text(url_label, "2. Browser");
    lv_obj_set_style_text_font(url_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(url_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_align(url_label, LV_TEXT_ALIGN_CENTER, 0);
    
    lv_obj_t *code_qr_label = lv_label_create(label_row);
    lv_obj_set_width(code_qr_label, qr_size);
    lv_label_set_text(code_qr_label, "3. Code");
    lv_obj_set_style_text_font(code_qr_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(code_qr_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_align(code_qr_label, LV_TEXT_ALIGN_CENTER, 0);
    
    /* ===== Row 5: 按钮行 (与二维码宽度对齐) ===== */
    lv_obj_t *btn_row = lv_obj_create(s_qr_panel);
    lv_obj_set_size(btn_row, total_qr_width, BUTTON_HEIGHT);
    lv_obj_align_to(btn_row, label_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_scrollbar_mode(btn_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    /* Start/Stop Button - 占据左边两个二维码的宽度 */
    s_btn_start_activation = lv_btn_create(btn_row);
    lv_obj_set_size(s_btn_start_activation, qr_size * 2 + qr_spacing, BUTTON_HEIGHT);
    lv_obj_set_style_bg_color(s_btn_start_activation, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_color(s_btn_start_activation, lv_color_hex(0x1565C0), LV_STATE_PRESSED);
    lv_obj_set_style_radius(s_btn_start_activation, 8, 0);
    lv_obj_set_style_shadow_width(s_btn_start_activation, 4, 0);
    lv_obj_set_style_shadow_opa(s_btn_start_activation, LV_OPA_30, 0);
    lv_obj_add_event_cb(s_btn_start_activation, btn_start_activation_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_label = lv_label_create(s_btn_start_activation);
    lv_label_set_text(btn_label, "Start Server");
    lv_obj_center(btn_label);
    lv_obj_set_style_text_font(btn_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    
    /* Help/Instructions Button - 占据右边一个二维码的宽度 */
    s_btn_help = lv_btn_create(btn_row);
    lv_obj_set_size(s_btn_help, qr_size, BUTTON_HEIGHT);
    lv_obj_set_style_bg_color(s_btn_help, COLOR_CARD, 0);
    lv_obj_set_style_bg_color(s_btn_help, lv_color_hex(0xE0E0E0), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_btn_help, 1, 0);
    lv_obj_set_style_border_color(s_btn_help, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(s_btn_help, 8, 0);
    lv_obj_add_event_cb(s_btn_help, btn_help_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *help_label = lv_label_create(s_btn_help);
    lv_label_set_text(help_label, "Help");
    lv_obj_center(help_label);
    lv_obj_set_style_text_font(help_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(help_label, COLOR_PRIMARY, 0);
    
    /* ===== Row 6: 状态标签 (居中) ===== */
    s_activation_status_label = lv_label_create(s_qr_panel);
    lv_label_set_text(s_activation_status_label, "Click button to start activation server");
    lv_obj_align_to(s_activation_status_label, btn_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_text_font(s_activation_status_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_activation_status_label, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_align(s_activation_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_activation_status_label, total_qr_width);
}

/*
 * ================================================================
 *      激活成功界面 - 重新设计 (约 500x370)
 * ================================================================
 *  +--------------------------------------------------+
 *  | [< Back]    Device Activated    [Connect]        |  32px
 *  +--------------------------------------------------+
 *  | ✓ ExDebugTool_ED008                    Online    |  Row1
 *  | PID: FCwDzD6VU0    DID: 2512999902              |  Row2  70px
 *  +--------------------------------------------------+
 *  | Last Sync:                      OneNet: Online   |
 *  |   Oscilloscope: --              Multimeter: --   |  90px
 *  |   Power: --                     Location: --     |
 *  +--------------------------------------------------+
 *  | Manual Upload    |    Auto Upload               |  90px
 *  +--------------------------------------------------+
 */
static void create_activated_panel(lv_obj_t *parent)
{
    /*
     * ================================================================
     *      激活成功界面布局 (Activated Panel Layout)
     * ================================================================
     *  面板尺寸: 约 500x370 (右侧面板)
     *
     *  +------------------------------------------------------------------+
     *  | [< Back]                           [Locate] [Connect]            |  32px 顶部栏
     *  +------------------------------------------------------------------+
     *  | +------------------------------------------------------------+  |
     *  | | ✓ ExDebugTool_ED013                          ● Online     |  |  设备信息卡片
     *  | | PID: FCwDzD6VU0                    DID: 2512999902        |  |  70px
     *  | +------------------------------------------------------------+  |
     *  +------------------------------------------------------------------+
     *  | +------------------------------------------------------------+  |
     *  | | ⟳ Last Sync [click to refresh]      OneNet: Connected     |  |  同步状态卡片
     *  | |   Oscilloscope: --                  Multimeter: --        |  |  90px
     *  | |   Power: --                         Location: 113.2, 23.1 |  |
     *  | +------------------------------------------------------------+  |
     *  +------------------------------------------------------------------+
     *  | +---------------------------+ +---------------------------+     |
     *  | | ↑ Manual Upload           | | ⟳ Auto Upload             |     |  上传操作区域
     *  | | [Select File]  Today: 0   | | [Switch]  Interval: 5min  |     |  95px
     *  | |                0 B        | |                           |     |
     *  | +---------------------------+ +---------------------------+     |
     *  +------------------------------------------------------------------+
     *
     *  按钮说明:
     *  - [< Back]: 返回扫码激活页面
     *  - [Locate]: 启动/停止WiFi定位上报
     *  - [Connect]: 设备上线/下线 (调用OneNET API)
     *  - "⟳ Last Sync" 标题可点击刷新同步状态（带旋转动画）
     *
     *  状态显示:
     *  - ● Online/Offline: 真实显示OneNET设备在线状态 (基于API返回)
     *  - OneNet: Connected/Disconnected: 显示当前连接状态
     * ================================================================
     */
    
    s_activated_panel = lv_obj_create(parent);
    lv_obj_set_size(s_activated_panel, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(s_activated_panel, 0, 0);
    lv_obj_set_style_bg_opa(s_activated_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_activated_panel, 0, 0);
    lv_obj_set_style_pad_all(s_activated_panel, 0, 0);  /* 去掉内边距，使用外层容器的padding */
    lv_obj_set_scrollbar_mode(s_activated_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_activated_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    cloud_device_info_t info;
    cloud_manager_get_device_info(&info);
    
    /* ===== 顶部栏 (32px): [Back] [Title] [Locate] [Online] - 整体下移5px后为-10 ===== */
    lv_obj_t *top_bar = lv_obj_create(s_activated_panel);
    lv_obj_set_size(top_bar, lv_pct(100), 32);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, -2);  /* 整体再下移3px */
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 实时时间显示（替代原来的返回按钮位置），使用与主界面相同的样式 */
    s_time_label = lv_label_create(top_bar);
    lv_label_set_text(s_time_label, "--,---,----,--:--");
    lv_obj_align(s_time_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(s_time_label, &lv_font_ShanHaiZhongXiaYeWuYuW_30, 0);  /* 使用与主界面相同的字体 */
    lv_obj_set_style_text_color(s_time_label, lv_color_hex(0xff4444), 0);  /* 初始为红色（未同步） */
    
    s_btn_back_to_qr = NULL;  /* 不创建返回按钮 */
    
    /* WiFi定位按钮 - 默认未启动状态 */
    s_btn_wifi_location = lv_btn_create(top_bar);
    lv_obj_set_size(s_btn_wifi_location, 90, 28);  /* 增加宽度 */
    lv_obj_align(s_btn_wifi_location, LV_ALIGN_RIGHT_MID, -100, 0);  /* 调整位置 */
    lv_obj_set_style_bg_color(s_btn_wifi_location, COLOR_PRIMARY, 0);  /* 默认蓝色 */
    lv_obj_set_style_radius(s_btn_wifi_location, 6, 0);
    lv_obj_add_event_cb(s_btn_wifi_location, btn_wifi_location_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *loc_lbl = lv_label_create(s_btn_wifi_location);
    lv_label_set_text(loc_lbl, LV_SYMBOL_GPS " Locate");  /* 默认显示Locate */
    lv_obj_center(loc_lbl);
    lv_obj_set_style_text_font(loc_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(loc_lbl, lv_color_white(), 0);
    
    /* 设备上线/下线按钮 */
    s_btn_device_online = lv_btn_create(top_bar);
    lv_obj_set_size(s_btn_device_online, 95, 28);  /* 增加宽度 */
    lv_obj_align(s_btn_device_online, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(s_btn_device_online, 6, 0);
    lv_obj_add_event_cb(s_btn_device_online, btn_device_online_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *online_lbl = lv_label_create(s_btn_device_online);
    if (onenet_is_device_online()) {
        lv_label_set_text(online_lbl, LV_SYMBOL_OK " Online");
        lv_obj_set_style_bg_color(s_btn_device_online, COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(online_lbl, LV_SYMBOL_WIFI " Connect");
        lv_obj_set_style_bg_color(s_btn_device_online, COLOR_PRIMARY, 0);
    }
    lv_obj_center(online_lbl);
    lv_obj_set_style_text_font(online_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(online_lbl, lv_color_white(), 0);
    
    /* ===== 设备信息卡片 (75px) - 与顶部栏间隔8px ===== */
    lv_obj_t *dev_card = lv_obj_create(s_activated_panel);
    lv_obj_set_size(dev_card, lv_pct(100), 75);
    lv_obj_align_to(dev_card, top_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);  /* 间隔8px */
    lv_obj_set_style_bg_color(dev_card, COLOR_CARD, 0);
    lv_obj_set_style_radius(dev_card, 10, 0);
    lv_obj_set_style_border_width(dev_card, 0, 0);  /* 去掉边框 */
    lv_obj_set_style_pad_all(dev_card, 10, 0);
    lv_obj_clear_flag(dev_card, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Row 1: 设备名称 (左) + 在线状态 (右) */
    s_device_code_label = lv_label_create(dev_card);
    char name_txt[80];
    snprintf(name_txt, sizeof(name_txt), LV_SYMBOL_OK " %s", 
             strlen(info.device_name) > 0 ? info.device_name : "ExDebugTool");
    lv_label_set_text(s_device_code_label, name_txt);
    lv_obj_align(s_device_code_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(s_device_code_label, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(s_device_code_label, COLOR_SUCCESS, 0);
    
    /* 在线/离线状态标签 - 右上角 (显示真实OneNET状态) */
    s_congrats_label = lv_label_create(dev_card);
    /* 根据真实的OneNET在线状态设置初始显示 */
    if (onenet_is_device_online()) {
        lv_label_set_text(s_congrats_label, LV_SYMBOL_OK " Online");
        lv_obj_set_style_text_color(s_congrats_label, COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(s_congrats_label, LV_SYMBOL_CLOSE " Offline");
        lv_obj_set_style_text_color(s_congrats_label, COLOR_WARNING, 0);
    }
    lv_obj_align(s_congrats_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_font(s_congrats_label, FONT_SMALL, 0);
    
    /* Row 2: 产品ID + 设备ID */
    lv_obj_t *pid_label = lv_label_create(dev_card);
    char pid_txt[48];
    const char *pid = (strlen(info.product_id) > 0) ? info.product_id : ONENET_HTTP_PRODUCT_ID;
    snprintf(pid_txt, sizeof(pid_txt), "PID: %s", pid);
    lv_label_set_text(pid_label, pid_txt);
    lv_obj_align(pid_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(pid_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(pid_label, COLOR_TEXT_SEC, 0);
    
    /* s_guide_label 用于显示设备ID */
    s_guide_label = lv_label_create(dev_card);
    char did_txt[48];
    snprintf(did_txt, sizeof(did_txt), "DID: %s", 
             strlen(info.device_id) > 0 ? info.device_id : "--");
    lv_label_set_text(s_guide_label, did_txt);
    lv_obj_align(s_guide_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_text_font(s_guide_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_guide_label, COLOR_TEXT_SEC, 0);
    
    /* ===== 最后同步状态卡片 (110px) - 与设备信息卡片间隔8px ===== */
    s_sync_status_cont = lv_obj_create(s_activated_panel);
    lv_obj_set_size(s_sync_status_cont, lv_pct(100), 110);  /* 高度110px */
    lv_obj_align_to(s_sync_status_cont, dev_card, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);  /* 间隔8px */
    lv_obj_set_style_bg_color(s_sync_status_cont, COLOR_CARD, 0);
    lv_obj_set_style_radius(s_sync_status_cont, 10, 0);
    lv_obj_set_style_border_width(s_sync_status_cont, 0, 0);  /* 去掉边框 */
    lv_obj_set_style_pad_all(s_sync_status_cont, 10, 0);
    lv_obj_clear_flag(s_sync_status_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Title row: Last Sync (clickable as refresh button) + Connection status */
    /* 使用按钮作为 "Last Sync" 标题，点击可刷新 */
    s_btn_refresh_sync = lv_btn_create(s_sync_status_cont);
    lv_obj_set_size(s_btn_refresh_sync, 115, 22);
    lv_obj_align(s_btn_refresh_sync, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(s_btn_refresh_sync, LV_OPA_TRANSP, 0);  /* 透明背景 */
    lv_obj_set_style_bg_opa(s_btn_refresh_sync, LV_OPA_20, LV_STATE_PRESSED);  /* 按下时有反馈 */
    lv_obj_set_style_bg_color(s_btn_refresh_sync, COLOR_PRIMARY, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_btn_refresh_sync, 0, 0);
    lv_obj_set_style_shadow_width(s_btn_refresh_sync, 0, 0);
    lv_obj_set_style_pad_all(s_btn_refresh_sync, 0, 0);
    lv_obj_add_event_cb(s_btn_refresh_sync, btn_refresh_sync_cb, LV_EVENT_CLICKED, NULL);
    
    /* 刷新图标 - 单独创建以支持旋转动画 */
    s_refresh_icon_label = lv_label_create(s_btn_refresh_sync);
    lv_label_set_text(s_refresh_icon_label, LV_SYMBOL_REFRESH);
    lv_obj_align(s_refresh_icon_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(s_refresh_icon_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_refresh_icon_label, COLOR_PRIMARY, 0);
    /* 设置旋转中心点 */
    lv_obj_set_style_transform_pivot_x(s_refresh_icon_label, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(s_refresh_icon_label, LV_PCT(50), 0);
    
    /* Last Sync 文字 */
    lv_obj_t *sync_title = lv_label_create(s_btn_refresh_sync);
    lv_label_set_text(sync_title, "Last Sync");
    lv_obj_align(sync_title, LV_ALIGN_LEFT_MID, 18, 0);  /* 图标右侧 */
    lv_obj_set_style_text_font(sync_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(sync_title, COLOR_PRIMARY, 0);
    
    /* OneNet连接状态标签 - 显示真实状态 */
    s_sync_onenet_label = lv_label_create(s_sync_status_cont);
    /* 根据真实的OneNET在线状态设置初始文本 */
    if (onenet_is_device_online()) {
        lv_label_set_text(s_sync_onenet_label, "OneNet: Connected");
        lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(s_sync_onenet_label, "OneNet: Disconnected");
        lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
    }
    lv_obj_align(s_sync_onenet_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_font(s_sync_onenet_label, FONT_SMALL, 0);
    
    /* Row 1: Oscilloscope + Multimeter data - 下移8px */
    s_sync_sensor_label = lv_label_create(s_sync_status_cont);
    lv_label_set_text(s_sync_sensor_label, "Oscilloscope: --");
    lv_obj_align(s_sync_sensor_label, LV_ALIGN_TOP_LEFT, 0, 38);  /* 30+8=38 */
    lv_obj_set_style_text_font(s_sync_sensor_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_sync_sensor_label, COLOR_TEXT, 0);
    
    s_sync_uploaded_label = lv_label_create(s_sync_status_cont);
    lv_label_set_text(s_sync_uploaded_label, "Multimeter: --");
    lv_obj_align(s_sync_uploaded_label, LV_ALIGN_TOP_RIGHT, 0, 38);  /* 30+8=38 */
    lv_obj_set_style_text_font(s_sync_uploaded_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_sync_uploaded_label, COLOR_TEXT, 0);
    
    /* Row 2: Power data + Location info - 下移8px */
    s_sync_device_label = lv_label_create(s_sync_status_cont);
    lv_label_set_text(s_sync_device_label, "Power: --");
    lv_obj_align(s_sync_device_label, LV_ALIGN_TOP_LEFT, 0, 64);  /* 56+8=64 */
    lv_obj_set_style_text_font(s_sync_device_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_sync_device_label, COLOR_TEXT, 0);
    
    s_sync_location_label = lv_label_create(s_sync_status_cont);
    lv_label_set_text(s_sync_location_label, "Location: --");
    lv_obj_align(s_sync_location_label, LV_ALIGN_TOP_RIGHT, 0, 64);  /* 56+8=64 */
    lv_obj_set_style_text_font(s_sync_location_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_sync_location_label, COLOR_TEXT, 0);
    
    /* ===== 上传操作区域 - 左右两个白色卡片 (105px) - 与同步卡片间隔8px ===== */
    lv_obj_t *upload_row = lv_obj_create(s_activated_panel);
    lv_obj_set_size(upload_row, lv_pct(100), 105);  /* 减小5px高度: 110-5=105 */
    lv_obj_align_to(upload_row, s_sync_status_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);  /* 间隔8px */
    lv_obj_set_style_bg_opa(upload_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(upload_row, 0, 0);
    lv_obj_set_style_pad_all(upload_row, 0, 0);
    lv_obj_clear_flag(upload_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(upload_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(upload_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    /* ===== 左侧卡片: 手动上传 ===== */
    lv_obj_t *manual_card = lv_obj_create(upload_row);
    lv_obj_set_size(manual_card, lv_pct(48), 100);  /* 减小5px高度: 105-5=100 */
    lv_obj_set_style_bg_color(manual_card, COLOR_CARD, 0);
    lv_obj_set_style_radius(manual_card, 10, 0);
    lv_obj_set_style_border_width(manual_card, 0, 0);  /* 去掉边框 */
    lv_obj_set_style_pad_all(manual_card, 10, 0);
    lv_obj_clear_flag(manual_card, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Manual upload title with icon */
    lv_obj_t *manual_title = lv_label_create(manual_card);
    lv_label_set_text(manual_title, LV_SYMBOL_UPLOAD " Manual Upload");
    lv_obj_align(manual_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(manual_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(manual_title, COLOR_PRIMARY, 0);
    
    /* Upload button - 更大更明显，上移10px */
    s_btn_manual_upload = lv_btn_create(manual_card);
    lv_obj_set_size(s_btn_manual_upload, 100, 32);
    lv_obj_align(s_btn_manual_upload, LV_ALIGN_BOTTOM_LEFT, 0, -10);  /* 上移10px */
    lv_obj_set_style_bg_color(s_btn_manual_upload, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(s_btn_manual_upload, 6, 0);
    lv_obj_add_event_cb(s_btn_manual_upload, btn_manual_upload_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *up_lbl = lv_label_create(s_btn_manual_upload);
    lv_label_set_text(up_lbl, "Select File");
    lv_obj_center(up_lbl);
    lv_obj_set_style_text_font(up_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(up_lbl, lv_color_white(), 0);
    
    /* Today stats - 右侧垂直排列 */
    s_stats_panel = NULL;
    s_stats_count_label = lv_label_create(manual_card);
    lv_label_set_text(s_stats_count_label, "Today: 0");
    lv_obj_align(s_stats_count_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_font(s_stats_count_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_stats_count_label, COLOR_TEXT, 0);
    
    s_stats_bytes_label = lv_label_create(manual_card);
    lv_label_set_text(s_stats_bytes_label, "0 B");
    lv_obj_align(s_stats_bytes_label, LV_ALIGN_BOTTOM_RIGHT, 0, -10);  /* 上移10px */
    lv_obj_set_style_text_font(s_stats_bytes_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_stats_bytes_label, COLOR_TEXT_SEC, 0);
    
    /* ===== 右侧卡片: 自动上传设置 ===== */
    lv_obj_t *auto_card = lv_obj_create(upload_row);
    lv_obj_set_size(auto_card, lv_pct(48), 100);  /* 减小5px高度: 105-5=100 */
    lv_obj_set_style_bg_color(auto_card, COLOR_CARD, 0);
    lv_obj_set_style_radius(auto_card, 10, 0);
    lv_obj_set_style_border_width(auto_card, 0, 0);  /* 去掉边框 */
    lv_obj_set_style_pad_all(auto_card, 10, 0);
    lv_obj_clear_flag(auto_card, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Auto upload title with icon */
    lv_obj_t *auto_title = lv_label_create(auto_card);
    lv_label_set_text(auto_title, LV_SYMBOL_LOOP " Auto Upload");
    lv_obj_align(auto_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(auto_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(auto_title, COLOR_PRIMARY, 0);
    
    /* Auto upload switch - 右上角 */
    s_auto_switch = lv_switch_create(auto_card);
    lv_obj_set_size(s_auto_switch, 44, 22);
    lv_obj_align(s_auto_switch, LV_ALIGN_TOP_RIGHT, 0, -2);
    lv_obj_set_style_bg_color(s_auto_switch, lv_color_hex(0xBDBDBD), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_auto_switch, COLOR_SUCCESS, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(s_auto_switch, auto_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    /* Interval setting row - 底部，上移10px */
    lv_obj_t *interval_lbl = lv_label_create(auto_card);
    lv_label_set_text(interval_lbl, "Interval:");
    lv_obj_align(interval_lbl, LV_ALIGN_BOTTOM_LEFT, 0, -10);  /* 上移10px */
    lv_obj_set_style_text_font(interval_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(interval_lbl, COLOR_TEXT, 0);
    
    /* Interval dropdown - 分钟级到小时级间隔（用于批量数据上报），上移10px */
    s_interval_dropdown = lv_dropdown_create(auto_card);
    lv_dropdown_set_options(s_interval_dropdown, "1min\n5min\n10min\n30min\n1hour");
    lv_obj_set_size(s_interval_dropdown, 85, 28);
    lv_obj_align(s_interval_dropdown, LV_ALIGN_BOTTOM_RIGHT, 0, -10);  /* 上移10px */
    lv_obj_set_style_text_font(s_interval_dropdown, FONT_SMALL, 0);
    lv_obj_set_style_pad_all(s_interval_dropdown, 3, LV_PART_MAIN);
    lv_obj_add_event_cb(s_interval_dropdown, interval_dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    auto_upload_settings_t settings;
    cloud_manager_get_auto_settings(&settings);
    if (settings.enabled) {
        lv_obj_add_state(s_auto_switch, LV_STATE_CHECKED);
    }
    cloud_manager_ui_set_interval(settings.interval_minutes);
    
    /* 产品信息行已删除 - 产品ID显示在设备信息卡片中 */
}

static void create_file_browser_popup(void)
{
    /* 半透明背景遮罩 - 参考 SD 卡管理样式 */
    s_file_browser_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_file_browser_popup, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(s_file_browser_popup, 0, 0);
    lv_obj_set_style_bg_color(s_file_browser_popup, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_file_browser_popup, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_file_browser_popup, 0, 0);
    lv_obj_clear_flag(s_file_browser_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_file_browser_popup, LV_OBJ_FLAG_HIDDEN);
    
    /* 弹窗卡片 */
    lv_obj_t *card = lv_obj_create(s_file_browser_popup);
    lv_obj_set_size(card, 400, 320);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, COLOR_DIVIDER, 0);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 标题 */
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "Select File to Upload");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(title, FONT_LARGE, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    
    /* 红色关闭按钮 */
    lv_obj_t *close_btn = lv_btn_create(card);
    lv_obj_set_size(close_btn, 32, 32);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 5, -5);
    lv_obj_set_style_bg_color(close_btn, COLOR_ERROR, 0);
    lv_obj_set_style_radius(close_btn, 16, 0);
    lv_obj_add_event_cb(close_btn, btn_cancel_browse_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *x_icon = lv_label_create(close_btn);
    lv_label_set_text(x_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x_icon, lv_color_white(), 0);
    lv_obj_center(x_icon);
    
    /* 文件列表容器 */
    s_file_list = lv_obj_create(card);
    lv_obj_set_size(s_file_list, 370, 180);
    lv_obj_align(s_file_list, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_color(s_file_list, lv_color_hex(0xF8F8F8), 0);
    lv_obj_set_style_radius(s_file_list, 8, 0);
    lv_obj_set_style_border_width(s_file_list, 1, 0);
    lv_obj_set_style_border_color(s_file_list, COLOR_DIVIDER, 0);
    lv_obj_set_style_pad_all(s_file_list, 8, 0);
    lv_obj_set_flex_flow(s_file_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_file_list, 5, 0);
    lv_obj_set_scrollbar_mode(s_file_list, LV_SCROLLBAR_MODE_AUTO);
    
    /* 选中文件标签 */
    s_selected_file_label = lv_label_create(card);
    lv_label_set_text(s_selected_file_label, "Selected: (none)");
    lv_obj_align(s_selected_file_label, LV_ALIGN_BOTTOM_LEFT, 0, -45);
    lv_obj_set_style_text_font(s_selected_file_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_selected_file_label, COLOR_PRIMARY, 0);
    
    /* 上传按钮 */
    s_btn_upload_file = lv_btn_create(card);
    lv_obj_set_size(s_btn_upload_file, 140, 36);
    lv_obj_align(s_btn_upload_file, LV_ALIGN_BOTTOM_LEFT, 50, 0);
    lv_obj_set_style_bg_color(s_btn_upload_file, COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(s_btn_upload_file, 8, 0);
    lv_obj_add_event_cb(s_btn_upload_file, btn_upload_file_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *upload_lbl = lv_label_create(s_btn_upload_file);
    lv_label_set_text(upload_lbl, LV_SYMBOL_UPLOAD " Upload");
    lv_obj_center(upload_lbl);
    lv_obj_set_style_text_font(upload_lbl, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(upload_lbl, lv_color_white(), 0);
    
    /* 取消按钮 */
    s_btn_cancel_browse = lv_btn_create(card);
    lv_obj_set_size(s_btn_cancel_browse, 100, 36);
    lv_obj_align(s_btn_cancel_browse, LV_ALIGN_BOTTOM_RIGHT, -50, 0);
    lv_obj_set_style_bg_color(s_btn_cancel_browse, COLOR_CARD, 0);
    lv_obj_set_style_border_width(s_btn_cancel_browse, 1, 0);
    lv_obj_set_style_border_color(s_btn_cancel_browse, COLOR_BORDER, 0);
    lv_obj_set_style_radius(s_btn_cancel_browse, 8, 0);
    lv_obj_add_event_cb(s_btn_cancel_browse, btn_cancel_browse_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *cancel_lbl = lv_label_create(s_btn_cancel_browse);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_center(cancel_lbl);
    lv_obj_set_style_text_font(cancel_lbl, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(cancel_lbl, COLOR_TEXT, 0);
}

static void create_upload_progress_popup(void)
{
    /* 半透明背景遮罩 */
    s_upload_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_upload_popup, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(s_upload_popup, 0, 0);
    lv_obj_set_style_bg_color(s_upload_popup, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_upload_popup, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_upload_popup, 0, 0);
    lv_obj_clear_flag(s_upload_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_upload_popup, LV_OBJ_FLAG_HIDDEN);
    
    /* 弹窗卡片 */
    lv_obj_t *card = lv_obj_create(s_upload_popup);
    lv_obj_set_size(card, 420, 280);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 30, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 标题区域 - 带图标 */
    lv_obj_t *title_cont = lv_obj_create(card);
    lv_obj_set_size(title_cont, 380, 40);
    lv_obj_align(title_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_hex(0xE3F2FD), 0);
    lv_obj_set_style_radius(title_cont, 8, 0);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 8, 0);
    lv_obj_clear_flag(title_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *title_icon = lv_label_create(title_cont);
    lv_label_set_text(title_icon, LV_SYMBOL_UPLOAD);
    lv_obj_align(title_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(title_icon, FONT_SUBTITLE, 0);
    lv_obj_set_style_text_color(title_icon, COLOR_PRIMARY, 0);
    
    lv_obj_t *title = lv_label_create(title_cont);
    lv_label_set_text(title, "Uploading File to OneNET");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 35, 0);
    lv_obj_set_style_text_font(title, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    
    /* 文件名 */
    s_upload_file_label = lv_label_create(card);
    lv_label_set_text(s_upload_file_label, "File: ");
    lv_obj_align(s_upload_file_label, LV_ALIGN_TOP_LEFT, 0, 50);
    lv_obj_set_style_text_font(s_upload_file_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_upload_file_label, COLOR_TEXT, 0);
    
    /* 进度条容器 */
    lv_obj_t *progress_cont = lv_obj_create(card);
    lv_obj_set_size(progress_cont, 380, 60);
    lv_obj_align(progress_cont, LV_ALIGN_TOP_MID, 0, 75);
    lv_obj_set_style_bg_opa(progress_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(progress_cont, 0, 0);
    lv_obj_set_style_pad_all(progress_cont, 0, 0);
    lv_obj_clear_flag(progress_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 进度条 - 渐变色 */
    s_upload_bar = lv_bar_create(progress_cont);
    lv_obj_set_size(s_upload_bar, 380, 28);
    lv_obj_align(s_upload_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_bar_set_range(s_upload_bar, 0, 100);
    lv_bar_set_value(s_upload_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_upload_bar, lv_color_hex(0xE0E0E0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_upload_bar, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_upload_bar, 14, LV_PART_MAIN);
    lv_obj_set_style_radius(s_upload_bar, 14, LV_PART_INDICATOR);
    lv_obj_set_style_anim_time(s_upload_bar, 300, 0);
    
    /* 进度百分比标签 */
    s_upload_percent_label = lv_label_create(progress_cont);
    lv_label_set_text(s_upload_percent_label, "0%");
    lv_obj_align(s_upload_percent_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_text_font(s_upload_percent_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_upload_percent_label, COLOR_TEXT_SEC, 0);
    
    /* 数据点统计 */
    s_upload_points_label = lv_label_create(progress_cont);
    lv_label_set_text(s_upload_points_label, "Data points: 0 / 0");
    lv_obj_align(s_upload_points_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(s_upload_points_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_upload_points_label, COLOR_TEXT_SEC, 0);
    
    /* 状态信息卡片 - 使用浅蓝色背景替代黄色 */
    lv_obj_t *status_card = lv_obj_create(card);
    lv_obj_set_size(status_card, 380, 50);
    lv_obj_align(status_card, LV_ALIGN_TOP_MID, 0, 145);
    lv_obj_set_style_bg_color(status_card, lv_color_hex(0xE3F2FD), 0);  /* 浅蓝色 */
    lv_obj_set_style_radius(status_card, 8, 0);
    lv_obj_set_style_border_width(status_card, 0, 0);
    lv_obj_set_style_pad_all(status_card, 10, 0);
    lv_obj_clear_flag(status_card, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *status_icon = lv_label_create(status_card);
    lv_label_set_text(status_icon, LV_SYMBOL_REFRESH);
    lv_obj_align(status_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(status_icon, COLOR_PRIMARY, 0);  /* 蓝色图标 */
    
    s_upload_status_label = lv_label_create(status_card);
    lv_label_set_text(s_upload_status_label, "Preparing upload...");
    lv_obj_align(s_upload_status_label, LV_ALIGN_LEFT_MID, 25, 0);
    lv_obj_set_style_text_font(s_upload_status_label, FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_upload_status_label, COLOR_TEXT, 0);
    
    /* 取消按钮 */
    s_btn_cancel_upload = lv_btn_create(card);
    lv_obj_set_size(s_btn_cancel_upload, 140, 40);
    lv_obj_align(s_btn_cancel_upload, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_btn_cancel_upload, COLOR_ERROR, 0);
    lv_obj_set_style_radius(s_btn_cancel_upload, 8, 0);
    lv_obj_set_style_shadow_width(s_btn_cancel_upload, 4, 0);
    lv_obj_set_style_shadow_opa(s_btn_cancel_upload, LV_OPA_20, 0);
    lv_obj_add_event_cb(s_btn_cancel_upload, btn_cancel_upload_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *cancel_lbl = lv_label_create(s_btn_cancel_upload);
    lv_label_set_text(cancel_lbl, LV_SYMBOL_CLOSE " Cancel");
    lv_obj_center(cancel_lbl);
    lv_obj_set_style_text_font(cancel_lbl, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(cancel_lbl, lv_color_white(), 0);
}

static void draw_qr_code(const char *data)
{
    ESP_LOGI(TAG, "Generating QR Code for: %s", data);
    
    if (s_qr_canvas == NULL) {
        ESP_LOGE(TAG, "QR canvas is NULL!");
        return;
    }
    
    /* Clear all children (including placeholder text) */
    lv_obj_clean(s_qr_canvas);
    
    /* Set canvas background to gray */
    lv_obj_set_style_bg_color(s_qr_canvas, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_qr_canvas, LV_OPA_COVER, 0);
    
    /* Calculate QR code size based on canvas size */
    lv_coord_t canvas_w = lv_obj_get_width(s_qr_canvas);
    lv_coord_t canvas_h = lv_obj_get_height(s_qr_canvas);
    lv_coord_t qr_size = (canvas_w < canvas_h ? canvas_w : canvas_h) - 16; /* Leave padding */
    if (qr_size < 100) qr_size = 100;
    if (qr_size > 160) qr_size = 160;
    
    ESP_LOGI(TAG, "QR canvas: %dx%d, QR code size: %d, data length: %d", 
             canvas_w, canvas_h, qr_size, strlen(data));
    
#if LV_USE_QRCODE
    /* Create real QR code: black on white background */
    lv_obj_t *qr = lv_qrcode_create(s_qr_canvas, qr_size, lv_color_black(), lv_color_white());
    if (qr != NULL) {
        ESP_LOGI(TAG, "QR code object created, updating with data...");
        lv_res_t qr_ret = lv_qrcode_update(qr, data, strlen(data));
        if (qr_ret == LV_RES_OK) {
            lv_obj_center(qr);
            lv_obj_set_style_border_width(qr, 0, 0);
            ESP_LOGI(TAG, "QR code created and updated successfully!");
        } else {
            ESP_LOGE(TAG, "Failed to update QR code data: %d (LV_RES_INV=%d)", qr_ret, LV_RES_INV);
            lv_obj_del(qr);
            /* Show error text */
            lv_obj_t *err = lv_label_create(s_qr_canvas);
            lv_label_set_text(err, "QR Update\nFailed");
            lv_obj_center(err);
            lv_obj_set_style_text_color(err, COLOR_ERROR, 0);
            lv_obj_set_style_text_align(err, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_font(err, FONT_SMALL, 0);
        }
    } else {
        ESP_LOGE(TAG, "Failed to create QR code object - LV_USE_QRCODE enabled but creation failed");
        /* Show error text */
        lv_obj_t *err = lv_label_create(s_qr_canvas);
        lv_label_set_text(err, "QR Create\nFailed");
        lv_obj_center(err);
        lv_obj_set_style_text_color(err, COLOR_ERROR, 0);
        lv_obj_set_style_text_align(err, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(err, FONT_SMALL, 0);
    }
#else
    /* Fallback: show URL text if QR code component not enabled */
    lv_obj_t *text = lv_label_create(s_qr_canvas);
    lv_label_set_text(text, data);
    lv_obj_center(text);
    lv_obj_set_style_text_font(text, FONT_NORMAL, 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(text, lv_color_black(), 0);
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(text, qr_size);
    ESP_LOGW(TAG, "LV_USE_QRCODE not enabled, showing URL text");
#endif
}

/* Draw QR code in URL canvas */
static void draw_url_qr_code(const char *url)
{
    ESP_LOGI(TAG, "Generating URL QR Code for: %s", url);
    
    if (s_qr_url_canvas == NULL) {
        ESP_LOGE(TAG, "URL QR canvas is NULL!");
        return;
    }
    
    /* Clear all children (including placeholder text) */
    lv_obj_clean(s_qr_url_canvas);
    
    /* Set canvas background to gray */
    lv_obj_set_style_bg_color(s_qr_url_canvas, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_qr_url_canvas, LV_OPA_COVER, 0);
    
    /* Calculate QR code size based on canvas size */
    lv_coord_t canvas_w = lv_obj_get_width(s_qr_url_canvas);
    lv_coord_t canvas_h = lv_obj_get_height(s_qr_url_canvas);
    lv_coord_t qr_size = (canvas_w < canvas_h ? canvas_w : canvas_h) - 16; /* Leave padding */
    if (qr_size < 100) qr_size = 100;
    if (qr_size > 160) qr_size = 160;
    
    ESP_LOGI(TAG, "URL QR canvas: %dx%d, QR code size: %d, data length: %d", 
             canvas_w, canvas_h, qr_size, strlen(url));
    
#if LV_USE_QRCODE
    /* Create real QR code: black on white background */
    lv_obj_t *qr = lv_qrcode_create(s_qr_url_canvas, qr_size, lv_color_black(), lv_color_white());
    if (qr != NULL) {
        ESP_LOGI(TAG, "URL QR code object created, updating with data...");
        lv_res_t qr_ret = lv_qrcode_update(qr, url, strlen(url));
        if (qr_ret == LV_RES_OK) {
            lv_obj_center(qr);
            lv_obj_set_style_border_width(qr, 0, 0);
            ESP_LOGI(TAG, "URL QR code created and updated successfully!");
        } else {
            ESP_LOGE(TAG, "Failed to update URL QR code data: %d", qr_ret);
            lv_obj_del(qr);
            /* Show error text */
            lv_obj_t *err = lv_label_create(s_qr_url_canvas);
            lv_label_set_text(err, "QR Update\nFailed");
            lv_obj_center(err);
            lv_obj_set_style_text_color(err, COLOR_ERROR, 0);
            lv_obj_set_style_text_align(err, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_font(err, FONT_SMALL, 0);
        }
    } else {
        ESP_LOGE(TAG, "Failed to create URL QR code object");
        /* Show error text */
        lv_obj_t *err = lv_label_create(s_qr_url_canvas);
        lv_label_set_text(err, "QR Create\nFailed");
        lv_obj_center(err);
        lv_obj_set_style_text_color(err, COLOR_ERROR, 0);
        lv_obj_set_style_text_align(err, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(err, FONT_SMALL, 0);
    }
#else
    /* Fallback: show URL text if QR code component not enabled */
    lv_obj_t *text = lv_label_create(s_qr_url_canvas);
    lv_label_set_text(text, url);
    lv_obj_center(text);
    lv_obj_set_style_text_font(text, FONT_NORMAL, 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(text, lv_color_black(), 0);
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(text, qr_size);
    ESP_LOGW(TAG, "LV_USE_QRCODE not enabled, showing URL text");
#endif
}

/* Draw QR code in Code canvas */
static void draw_code_qr_code(const char *code)
{
    ESP_LOGI(TAG, "Generating Code QR Code for: %s", code);
    
    if (s_qr_code_canvas == NULL) {
        ESP_LOGE(TAG, "Code QR canvas is NULL!");
        return;
    }
    
    /* Clear all children (including placeholder text) */
    lv_obj_clean(s_qr_code_canvas);
    
    /* Set canvas background to gray */
    lv_obj_set_style_bg_color(s_qr_code_canvas, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_qr_code_canvas, LV_OPA_COVER, 0);
    
    /* Calculate QR code size based on canvas size */
    lv_coord_t canvas_w = lv_obj_get_width(s_qr_code_canvas);
    lv_coord_t canvas_h = lv_obj_get_height(s_qr_code_canvas);
    lv_coord_t qr_size = (canvas_w < canvas_h ? canvas_w : canvas_h) - 12; /* Leave padding */
    if (qr_size < 80) qr_size = 80;
    if (qr_size > 110) qr_size = 110;
    
    ESP_LOGI(TAG, "Code QR canvas: %dx%d, QR code size: %d, data length: %d", 
             canvas_w, canvas_h, qr_size, strlen(code));
    
#if LV_USE_QRCODE
    /* Create real QR code: black on white background */
    lv_obj_t *qr = lv_qrcode_create(s_qr_code_canvas, qr_size, lv_color_black(), lv_color_white());
    if (qr != NULL) {
        ESP_LOGI(TAG, "Code QR code object created, updating with data...");
        lv_res_t qr_ret = lv_qrcode_update(qr, code, strlen(code));
        if (qr_ret == LV_RES_OK) {
            lv_obj_center(qr);
            lv_obj_set_style_border_width(qr, 0, 0);
            ESP_LOGI(TAG, "Code QR code created and updated successfully!");
        } else {
            ESP_LOGE(TAG, "Failed to update Code QR code data: %d", qr_ret);
            lv_obj_del(qr);
            /* Show error text */
            lv_obj_t *err = lv_label_create(s_qr_code_canvas);
            lv_label_set_text(err, "QR Update\nFailed");
            lv_obj_center(err);
            lv_obj_set_style_text_color(err, COLOR_ERROR, 0);
            lv_obj_set_style_text_align(err, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_font(err, FONT_SMALL, 0);
        }
    } else {
        ESP_LOGE(TAG, "Failed to create Code QR code object");
        /* Show error text */
        lv_obj_t *err = lv_label_create(s_qr_code_canvas);
        lv_label_set_text(err, "QR Create\nFailed");
        lv_obj_center(err);
        lv_obj_set_style_text_color(err, COLOR_ERROR, 0);
        lv_obj_set_style_text_align(err, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(err, FONT_SMALL, 0);
    }
#else
    /* Fallback: show code text if QR code component not enabled */
    lv_obj_t *text = lv_label_create(s_qr_code_canvas);
    lv_label_set_text(text, code);
    lv_obj_center(text);
    lv_obj_set_style_text_font(text, FONT_NORMAL, 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(text, lv_color_black(), 0);
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(text, qr_size);
    ESP_LOGW(TAG, "LV_USE_QRCODE not enabled, showing code text");
#endif
}

/* Create help/instructions popup */
/* 帮助弹窗关闭按钮回调 */
static void help_popup_close_cb(lv_event_t *e)
{
    (void)e;
    if (s_help_popup != NULL) {
        lv_obj_add_flag(s_help_popup, LV_OBJ_FLAG_HIDDEN);
    }
}

/*
 * 创建帮助弹窗 - 紧凑美观设计，使用图标和彩色标签
 */
static void create_help_popup(void)
{
    if (s_help_popup != NULL) {
        lv_obj_clear_flag(s_help_popup, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_help_popup);
        return;
    }
    
    /* 半透明背景遮罩 */
    s_help_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_help_popup, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(s_help_popup, 0, 0);
    lv_obj_set_style_bg_color(s_help_popup, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_help_popup, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_help_popup, 0, 0);
    lv_obj_clear_flag(s_help_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_help_popup, help_popup_close_cb, LV_EVENT_CLICKED, NULL);
    
    /* 弹窗卡片 - 紧凑尺寸（调整高度以适配减少的内容） */
    lv_obj_t *card = lv_obj_create(s_help_popup);
    lv_obj_set_size(card, 360, 345);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, COLOR_PRIMARY, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_shadow_width(card, 15, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
    
    /* 标题行 - 带图标 */
    lv_obj_t *title_row = lv_obj_create(card);
    lv_obj_set_size(title_row, 328, 32);
    lv_obj_align(title_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(title_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_row, 0, 0);
    lv_obj_set_style_pad_all(title_row, 0, 0);
    
    lv_obj_t *title = lv_label_create(title_row);
    lv_label_set_text(title, "Activation Guide");
    lv_obj_set_style_text_font(title, FONT_SUBTITLE, 0);
    lv_obj_set_style_text_color(title, COLOR_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);
    
    /* 关闭按钮 - 右上角，使用LV_SYMBOL_CLOSE */
    lv_obj_t *close_btn = lv_btn_create(title_row);
    lv_obj_set_size(close_btn, 32, 32);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(close_btn, COLOR_ERROR, 0);
    lv_obj_set_style_radius(close_btn, 16, 0);
    lv_obj_add_event_cb(close_btn, help_popup_close_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *close_icon = lv_label_create(close_btn);
    lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(close_icon, FONT_NORMAL, 0);
    lv_obj_center(close_icon);
    
    /* 内容区域 - 使用Flex布局，自动分配空间，避免空白 */
    lv_obj_t *content = lv_obj_create(card);
    lv_obj_set_size(content, 328, 265);
    lv_obj_align_to(content, title_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);  /* 禁用滚动 */
    /* 使用Flex布局，垂直排列，均匀分布 */
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(content, 10, 0);  /* 卡片之间的间距 */
    
    /* Function Section - 蓝色卡片，自适应高度 */
    lv_obj_t *func_card = lv_obj_create(content);
    lv_obj_set_width(func_card, 328);
    lv_obj_set_style_bg_color(func_card, lv_color_hex(0xE3F2FD), 0);
    lv_obj_set_style_border_width(func_card, 0, 0);
    lv_obj_set_style_radius(func_card, 8, 0);
    lv_obj_set_style_pad_all(func_card, 10, 0);
    lv_obj_clear_flag(func_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(func_card, 0);  /* 不自动扩展 */
    
    lv_obj_t *func_title = lv_label_create(func_card);
    lv_label_set_text(func_title, "[Function]");
    lv_obj_set_style_text_font(func_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(func_title, lv_color_hex(0x1565C0), 0);
    lv_obj_align(func_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *func_desc = lv_label_create(func_card);
    lv_label_set_text(func_desc, "Register device on OneNet");
    lv_obj_set_style_text_font(func_desc, FONT_SMALL, 0);
    lv_obj_set_style_text_color(func_desc, COLOR_TEXT, 0);
    lv_obj_align_to(func_desc, func_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
    
    /* Steps Section - 绿色卡片，自适应高度 */
    lv_obj_t *steps_card = lv_obj_create(content);
    lv_obj_set_width(steps_card, 328);
    lv_obj_set_style_bg_color(steps_card, lv_color_hex(0xE8F5E9), 0);
    lv_obj_set_style_border_width(steps_card, 0, 0);
    lv_obj_set_style_radius(steps_card, 8, 0);
    lv_obj_set_style_pad_all(steps_card, 10, 0);
    lv_obj_clear_flag(steps_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(steps_card, 0);
    
    lv_obj_t *steps_title = lv_label_create(steps_card);
    lv_label_set_text(steps_title, "[Steps]");
    lv_obj_set_style_text_font(steps_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(steps_title, lv_color_hex(0x2E7D32), 0);
    lv_obj_align(steps_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *steps_list = lv_label_create(steps_card);
    lv_label_set_text(steps_list,
        "1. Click 'Start Server'\n"
        "2. Scan WiFi QR to connect\n"
        "3. Scan URL QR to open browser"
    );
    lv_obj_set_style_text_font(steps_list, FONT_SMALL, 0);
    lv_obj_set_style_text_color(steps_list, COLOR_TEXT, 0);
    lv_obj_align_to(steps_list, steps_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
    lv_obj_set_width(steps_list, 308);
    lv_label_set_long_mode(steps_list, LV_LABEL_LONG_WRAP);
    
    /* Notes Section - 浅灰色卡片，自适应高度，占据剩余空间 */
    lv_obj_t *notes_card = lv_obj_create(content);
    lv_obj_set_width(notes_card, 328);
    lv_obj_set_style_bg_color(notes_card, lv_color_hex(0xF5F5F5), 0);  /* 浅灰色 */
    lv_obj_set_style_border_width(notes_card, 0, 0);
    lv_obj_set_style_radius(notes_card, 8, 0);
    lv_obj_set_style_pad_all(notes_card, 10, 0);
    lv_obj_clear_flag(notes_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(notes_card, 1);  /* 占据剩余空间 */
    
    lv_obj_t *notes_title = lv_label_create(notes_card);
    lv_label_set_text(notes_title, "[Notes]");
    lv_obj_set_style_text_font(notes_title, FONT_SMALL, 0);
    lv_obj_set_style_text_color(notes_title, lv_color_hex(0xE65100), 0);
    lv_obj_align(notes_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *notes_list = lv_label_create(notes_card);
    lv_label_set_text(notes_list,
        "- Device code is unique\n"
        "- Keep device powered\n"
        "- Same WiFi network required\n"
        "- One-time activation"
    );
    lv_obj_set_style_text_font(notes_list, FONT_SMALL, 0);
    lv_obj_set_style_text_color(notes_list, COLOR_TEXT, 0);
    lv_obj_align_to(notes_list, notes_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
    lv_obj_set_width(notes_list, 308);
    lv_label_set_long_mode(notes_list, LV_LABEL_LONG_WRAP);
}

/* 帮助按钮回调 - 显示帮助弹窗 */
static void btn_help_cb(lv_event_t *e)
{
    (void)e;
    
    if (s_help_popup == NULL) {
        create_help_popup();
    } else {
        lv_obj_clear_flag(s_help_popup, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_help_popup);
    }
}

/**
 * @brief 检查文件扩展名是否为OneNET支持的格式
 * @note OneNET文件上传支持: .jpg .jpeg .png .bmp .gif .webp .tiff .txt
 *       CSV文件会被重命名为.txt上传
 */
static bool is_supported_file_ext(const char *ext)
{
    if (ext == NULL) return false;
    
    /* 图片格式 */
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 ||
        strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0 ||
        strcasecmp(ext, ".gif") == 0 || strcasecmp(ext, ".webp") == 0 ||
        strcasecmp(ext, ".tiff") == 0) {
        return true;
    }
    
    /* 文本格式 */
    if (strcasecmp(ext, ".txt") == 0 || strcasecmp(ext, ".csv") == 0) {
        return true;
    }
    
    return false;
}

/**
 * @brief 获取文件类型图标
 */
static const char* get_file_icon(const char *ext)
{
    if (ext == NULL) return LV_SYMBOL_FILE;
    
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 ||
        strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0 ||
        strcasecmp(ext, ".gif") == 0 || strcasecmp(ext, ".webp") == 0 ||
        strcasecmp(ext, ".tiff") == 0) {
        return LV_SYMBOL_IMAGE;
    }
    
    if (strcasecmp(ext, ".csv") == 0) {
        return LV_SYMBOL_LIST;  /* CSV数据文件 */
    }
    
    return LV_SYMBOL_FILE;
}

static void scan_sd_files(void)
{
    if (s_file_list == NULL) return;
    
    /* Clear current list */
    lv_obj_clean(s_file_list);
    s_selected_file_path[0] = '\0';
    
    if (s_selected_file_label != NULL) {
        lv_label_set_text(s_selected_file_label, "Selected: (none)");
    }
    
    /* Try to open SD card directory - 扫描截图和示波器数据目录 */
    const char *paths[] = {
        "/sdcard/screenshots",   /* 截图目录 */
        "/sdcard/oscilloscope",  /* 示波器数据目录 */
        "/sdcard/data",          /* 通用数据目录 */
        "/sdcard"                /* 根目录 */
    };
    
    int file_count = 0;
    
    for (int p = 0; p < 4; p++) {
        DIR *dir = opendir(paths[p]);
        if (dir == NULL) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && file_count < 30) {
            /* Skip directories and hidden files */
            if (entry->d_type == DT_DIR) continue;
            if (entry->d_name[0] == '.') continue;
            
            /* 检查是否为OneNET支持的文件格式 */
            const char *ext = strrchr(entry->d_name, '.');
            if (!is_supported_file_ext(ext)) continue;
            
            /* Get file info */
            char full_path[128];
            snprintf(full_path, sizeof(full_path), "%s/%s", paths[p], entry->d_name);
            
            struct stat st;
            if (stat(full_path, &st) != 0) continue;
            
            /* 检查文件大小限制 (20MB) */
            if (st.st_size > 20 * 1024 * 1024) {
                ESP_LOGW(TAG, "File too large, skipping: %s (%ld bytes)", 
                         entry->d_name, (long)st.st_size);
                continue;
            }
            
            /* Check if already uploaded */
            bool uploaded = cloud_manager_is_file_uploaded(full_path);
            
            /* Create file item */
            lv_obj_t *item = lv_obj_create(s_file_list);
            lv_obj_set_size(item, lv_pct(100), 55);
            lv_obj_set_style_bg_color(item, uploaded ? lv_color_hex(0xE8F5E9) : COLOR_CARD, 0);
            lv_obj_set_style_radius(item, 6, 0);
            lv_obj_set_style_border_width(item, 0, 0);
            lv_obj_set_style_pad_all(item, 10, 0);
            lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
            
            /* Store path in user data */
            char *path_copy = lv_mem_alloc(strlen(full_path) + 1);
            if (path_copy) {
                strcpy(path_copy, full_path);
                lv_obj_set_user_data(item, path_copy);
            }
            lv_obj_add_event_cb(item, file_item_cb, LV_EVENT_CLICKED, NULL);
            
            /* 文件图标 */
            lv_obj_t *icon_lbl = lv_label_create(item);
            lv_label_set_text(icon_lbl, get_file_icon(ext));
            lv_obj_align(icon_lbl, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_20, 0);
            lv_obj_set_style_text_color(icon_lbl, COLOR_PRIMARY, 0);
            
            /* File name */
            lv_obj_t *name_lbl = lv_label_create(item);
            lv_label_set_text(name_lbl, entry->d_name);
            lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 30, 0);
            lv_obj_set_style_text_font(name_lbl, FONT_NORMAL, 0);
            lv_obj_set_style_text_color(name_lbl, uploaded ? COLOR_SUCCESS : COLOR_TEXT, 0);
            lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
            lv_obj_set_width(name_lbl, 280);
            
            /* File size and date */
            char info_text[80];
            char size_str[16];
            cloud_manager_format_bytes(st.st_size, size_str, sizeof(size_str));
            
            struct tm *tm_info = localtime(&st.st_mtime);
            snprintf(info_text, sizeof(info_text), "%s  %02d-%02d %02d:%02d%s",
                     size_str,
                     tm_info->tm_mon + 1, tm_info->tm_mday,
                     tm_info->tm_hour, tm_info->tm_min,
                     uploaded ? "  " LV_SYMBOL_OK : "");
            
            lv_obj_t *info_lbl = lv_label_create(item);
            lv_label_set_text(info_lbl, info_text);
            lv_obj_align(info_lbl, LV_ALIGN_BOTTOM_LEFT, 30, 0);
            lv_obj_set_style_text_font(info_lbl, FONT_SMALL, 0);
            lv_obj_set_style_text_color(info_lbl, uploaded ? COLOR_SUCCESS : COLOR_TEXT_SEC, 0);
            
            file_count++;
        }
        
        closedir(dir);
    }
    
    if (file_count == 0) {
        /* Show empty message */
        lv_obj_t *empty_lbl = lv_label_create(s_file_list);
        lv_label_set_text(empty_lbl, "No uploadable files found\n\n"
                                     "Supported formats:\n"
                                     "Images: .jpg .png .bmp .gif\n"
                                     "Data: .csv .txt");
        lv_obj_center(empty_lbl);
        lv_obj_set_style_text_font(empty_lbl, FONT_NORMAL, 0);
        lv_obj_set_style_text_color(empty_lbl, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_align(empty_lbl, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    ESP_LOGI(TAG, "Found %d uploadable files", file_count);
}

/* ==================== Event Handlers ==================== */

/* Task to stop server in background - prevents crash */
static void stop_server_task(void *param)
{
    (void)param;
    
    ESP_LOGI(TAG, "Stopping server in background task");
    
    /* Stop server with delays to ensure clean shutdown */
    /* The stop function now has mutex protection and extended delays */
    esp_err_t ret = cloud_activation_server_stop();
    if (ret == ESP_OK) {
        /* Sync UI state with actual server state */
        s_activation_server_started = false;
        ESP_LOGI(TAG, "Server stopped successfully");
    } else {
        ESP_LOGE(TAG, "Failed to stop server: %s", esp_err_to_name(ret));
        /* Still update UI state based on actual server state */
        s_activation_server_started = cloud_activation_server_is_running();
    }
    
    /* Wait a bit more to ensure everything is cleaned up */
    /* Additional delay to ensure all threads and resources are fully released */
    /* This ensures all pthread cleanup callbacks complete before task deletion */
    ESP_LOGI(TAG, "Waiting for final cleanup before UI update...");
    vTaskDelay(pdMS_TO_TICKS(1000));  /* Increased from 500ms to 1000ms */
    
    /* Update UI in LVGL thread using async call */
    lv_async_call(async_update_ui_after_stop, NULL);
    
    /* Additional delay before task deletion to ensure async call is processed */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    vTaskDelete(NULL);
}

/* Async handler for updating UI after server stop */
static void async_update_ui_after_stop(void *param)
{
    (void)param;
    
    ESP_LOGI(TAG, "Updating UI after server stop");
    
    /* Double-check server state before updating UI */
    bool server_running = cloud_activation_server_is_running();
    if (server_running) {
        ESP_LOGW(TAG, "Server still running, aborting UI update");
        /* Re-enable button so user can try again */
        if (s_btn_start_activation) {
            lv_obj_clear_state(s_btn_start_activation, LV_STATE_DISABLED);
        }
        return;
    }
    
    /* Sync UI state */
    s_activation_server_started = false;
    
    /* Update UI - clear both QR codes */
    if (s_qr_canvas) {
        lv_obj_clean(s_qr_canvas);
        /* Restore placeholder text */
        s_qr_hint = lv_label_create(s_qr_canvas);
        lv_label_set_text(s_qr_hint, "WiFi\nQR");
        lv_obj_center(s_qr_hint);
        lv_obj_set_style_text_font(s_qr_hint, FONT_SMALL, 0);
        lv_obj_set_style_text_color(s_qr_hint, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_align(s_qr_hint, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    if (s_qr_url_canvas) {
        lv_obj_clean(s_qr_url_canvas);
        /* Restore placeholder text */
        s_qr_url_hint = lv_label_create(s_qr_url_canvas);
        lv_label_set_text(s_qr_url_hint, "URL\nQR");
        lv_obj_center(s_qr_url_hint);
        lv_obj_set_style_text_font(s_qr_url_hint, FONT_SMALL, 0);
        lv_obj_set_style_text_color(s_qr_url_hint, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_align(s_qr_url_hint, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    if (s_qr_code_canvas) {
        lv_obj_clean(s_qr_code_canvas);
        /* Restore placeholder text */
        s_qr_code_hint = lv_label_create(s_qr_code_canvas);
        lv_label_set_text(s_qr_code_hint, "Code\nQR");
        lv_obj_center(s_qr_code_hint);
        lv_obj_set_style_text_font(s_qr_code_hint, FONT_SMALL, 0);
        lv_obj_set_style_text_color(s_qr_code_hint, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_align(s_qr_code_hint, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    if (s_btn_start_activation) {
        lv_obj_t *btn_label = lv_obj_get_child(s_btn_start_activation, 0);
        if (btn_label) {
            lv_label_set_text(btn_label, "Start Server");
        }
        lv_obj_set_style_bg_color(s_btn_start_activation, COLOR_PRIMARY, 0);
        /* Re-enable button after server is fully stopped */
        lv_obj_clear_state(s_btn_start_activation, LV_STATE_DISABLED);
    }
    
    if (s_activation_status_label) {
        lv_label_set_text(s_activation_status_label, "Click button to start activation server");
        lv_obj_set_style_text_color(s_activation_status_label, COLOR_TEXT_SEC, 0);
    }
    
    ESP_LOGI(TAG, "UI updated after server stop");
}

static void btn_start_activation_cb(lv_event_t *e)
{
    (void)e;
    
    /* Double-check: if button is disabled, ignore click (shouldn't happen but safety check) */
    if (s_btn_start_activation && lv_obj_has_state(s_btn_start_activation, LV_STATE_DISABLED)) {
        ESP_LOGW(TAG, "Button click ignored - button is disabled");
        return;
    }
    
    /* Check server state before proceeding */
    bool server_running = cloud_activation_server_is_running();
    if (s_activation_server_started != server_running) {
        /* State mismatch - sync UI state with actual server state */
        ESP_LOGW(TAG, "State mismatch: UI=%d, Server=%d, syncing...", 
                 s_activation_server_started, server_running);
        s_activation_server_started = server_running;
    }
    
    /* Prevent multiple clicks - disable button immediately */
    if (s_btn_start_activation) {
        lv_obj_add_state(s_btn_start_activation, LV_STATE_DISABLED);
    }
    
    if (s_activation_server_started || server_running) {
        /* Server is running - stop it */
        ESP_LOGI(TAG, "Stopping server (UI state: %d, Server state: %d)", 
                 s_activation_server_started, server_running);
        /* Create a task to stop server in background to avoid crash */
        xTaskCreate(stop_server_task, "stop_server", 4096, NULL, 5, NULL);
        /* Button will be re-enabled in async_update_ui_after_stop */
        return;
    } else {
        /* Start server - check again to prevent race condition */
        if (cloud_activation_server_is_running()) {
            ESP_LOGW(TAG, "Server started by another thread, aborting");
            if (s_btn_start_activation) {
                lv_obj_clear_state(s_btn_start_activation, LV_STATE_DISABLED);
            }
            return;
        }
        
        /* Start server - device code will be set after successful activation */
        ESP_LOGI(TAG, "Starting activation server...");
        esp_err_t ret = cloud_activation_server_start(NULL, activation_status_callback);
        
        if (ret == ESP_OK) {
            s_activation_server_started = true;
            
            /* Get server IP address (AP mode: 192.168.4.1) */
            const char *ip_addr = cloud_activation_server_get_ip();
            
            /* Update button and status first */
            if (s_btn_start_activation) {
                lv_obj_t *btn_label = lv_obj_get_child(s_btn_start_activation, 0);
                if (btn_label) {
                    lv_label_set_text(btn_label, "Stop Server");
                }
                lv_obj_set_style_bg_color(s_btn_start_activation, COLOR_WARNING, 0);
                /* Re-enable button after server started */
                lv_obj_clear_state(s_btn_start_activation, LV_STATE_DISABLED);
            }
            
            if (s_activation_status_label) {
                char status_msg[128];
                snprintf(status_msg, sizeof(status_msg), 
                         "Server running at http://%s", ip_addr);
                lv_label_set_text(s_activation_status_label, status_msg);
                lv_obj_set_style_text_color(s_activation_status_label, COLOR_SUCCESS, 0);
            }
            
            /* Generate WiFi QR code */
            if (s_qr_canvas != NULL) {
                /* Hide placeholder first */
                if (s_qr_hint) {
                    lv_obj_add_flag(s_qr_hint, LV_OBJ_FLAG_HIDDEN);
                }
                
                /* Generate WiFi QR code */
                char qr_wifi[128];
                snprintf(qr_wifi, sizeof(qr_wifi), 
                         "WIFI:S:%s;T:WPA;P:%s;;", 
                         CLOUD_ACTIVATION_AP_SSID, 
                         CLOUD_ACTIVATION_AP_PASS);
                
                ESP_LOGI(TAG, "Generating WiFi QR code: %s", CLOUD_ACTIVATION_AP_SSID);
                draw_qr_code(qr_wifi);
            } else {
                ESP_LOGE(TAG, "WiFi QR canvas is NULL!");
            }
            
            /* Generate URL QR code */
            if (s_qr_url_canvas != NULL) {
                /* Hide placeholder first */
                if (s_qr_url_hint) {
                    lv_obj_add_flag(s_qr_url_hint, LV_OBJ_FLAG_HIDDEN);
                }
                
                /* Generate URL QR code */
                char qr_url[64];
                snprintf(qr_url, sizeof(qr_url), "http://%s", ip_addr);
                
                ESP_LOGI(TAG, "Generating URL QR code: %s", qr_url);
                draw_url_qr_code(qr_url);
            } else {
                ESP_LOGE(TAG, "URL QR canvas is NULL!");
            }
            
            /* Device Code QR code should only be shown after successful activation */
            /* Hide it initially - will be shown after activation succeeds */
            if (s_qr_code_canvas != NULL && s_qr_code_hint != NULL) {
                lv_obj_clear_flag(s_qr_code_hint, LV_OBJ_FLAG_HIDDEN);
            }
            
            ESP_LOGI(TAG, "AP started: SSID=%s, Server at http://%s", 
                     CLOUD_ACTIVATION_AP_SSID, ip_addr);
        } else {
            if (s_activation_status_label) {
                lv_label_set_text(s_activation_status_label, "Failed to start AP! Check WiFi module.");
                lv_obj_set_style_text_color(s_activation_status_label, COLOR_ERROR, 0);
            }
            ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
            /* Re-enable button on error */
            if (s_btn_start_activation) {
                lv_obj_clear_state(s_btn_start_activation, LV_STATE_DISABLED);
            }
        }
    }
}

/* Async handler for activation success - called in LVGL main thread */
static void async_activation_success_handler(void *param)
{
    (void)param;
    
    if (!s_activation_success_pending) return;
    s_activation_success_pending = false;
    
    ESP_LOGI(TAG, "Processing activation success in UI thread");
    
    /* Update device info in cloud manager */
    cloud_manager_set_activated(s_pending_activation_result.device_id,
                                 s_pending_activation_result.product_id,
                                 s_pending_activation_result.device_name, 
                                 s_pending_activation_result.sec_key);
    
    /* Stop server */
    cloud_activation_server_stop();
    s_activation_server_started = false;
    
    /* Update button state */
    if (s_btn_start_activation) {
        lv_obj_t *btn_label = lv_obj_get_child(s_btn_start_activation, 0);
        if (btn_label) {
            lv_label_set_text(btn_label, "Start Activation");
        }
        lv_obj_set_style_bg_color(s_btn_start_activation, COLOR_PRIMARY, 0);
    }
    
    /* Show device code QR code after successful activation */
    if (s_qr_code_canvas != NULL && strlen(s_pending_activation_result.device_code) > 0) {
        /* Hide placeholder */
        if (s_qr_code_hint) {
            lv_obj_add_flag(s_qr_code_hint, LV_OBJ_FLAG_HIDDEN);
        }
        /* Generate Device Code QR code */
        ESP_LOGI(TAG, "Generating Device Code QR code after activation: %s", 
                 s_pending_activation_result.device_code);
        draw_code_qr_code(s_pending_activation_result.device_code);
    }
    
    /* Show activated panel with device code */
    cloud_manager_ui_show_activated(s_pending_activation_result.device_code);
    
    ESP_LOGI(TAG, "Device activated: Code=%s, ID=%s, Name=%s", 
             s_pending_activation_result.device_code,
             s_pending_activation_result.device_id, 
             s_pending_activation_result.device_name);
}

/* Async handler for activation failure - called in LVGL main thread */
static void async_activation_fail_handler(void *param)
{
    (void)param;
    
    if (!s_activation_fail_pending) return;
    s_activation_fail_pending = false;
    
    if (s_activation_status_label) {
        char msg[80];
        snprintf(msg, sizeof(msg), "Failed: %s", s_pending_activation_result.error_msg);
        lv_label_set_text(s_activation_status_label, msg);
        lv_obj_set_style_text_color(s_activation_status_label, COLOR_ERROR, 0);
    }
}

static void activation_status_callback(activation_status_t status, const activation_result_t *result)
{
    /* Note: This callback may be called from HTTP server task, not LVGL thread!
     * For simple status updates, we update directly (risky but usually works for labels)
     * For complex UI changes (panel switching), we use lv_async_call for thread safety */
    
    switch (status) {
        case ACTIVATION_STATUS_AP_STARTED:
            if (s_activation_status_label) {
                lv_label_set_text(s_activation_status_label, "AP ready! Connect your phone...");
                lv_obj_set_style_text_color(s_activation_status_label, COLOR_PRIMARY, 0);
            }
            break;
            
        case ACTIVATION_STATUS_CLIENT_CONNECTED:
            if (s_activation_status_label) {
                lv_label_set_text(s_activation_status_label, "Phone connected!");
                lv_obj_set_style_text_color(s_activation_status_label, COLOR_SUCCESS, 0);
            }
            break;
            
        case ACTIVATION_STATUS_REGISTERING:
            if (s_activation_status_label) {
                lv_label_set_text(s_activation_status_label, "Registering on OneNet...");
                lv_obj_set_style_text_color(s_activation_status_label, COLOR_WARNING, 0);
            }
            break;
            
        case ACTIVATION_STATUS_SUCCESS:
            if (result && result->success) {
                /* Copy result and schedule async UI update */
                memcpy(&s_pending_activation_result, result, sizeof(activation_result_t));
                s_activation_success_pending = true;
                
                /* Schedule UI update in LVGL main thread */
                lv_async_call(async_activation_success_handler, NULL);
                
                /* Show immediate feedback */
                if (s_activation_status_label) {
                    lv_label_set_text(s_activation_status_label, "Activation Success! Switching...");
                    lv_obj_set_style_text_color(s_activation_status_label, COLOR_SUCCESS, 0);
                }
            }
            break;
            
        case ACTIVATION_STATUS_FAILED:
            if (result) {
                memcpy(&s_pending_activation_result, result, sizeof(activation_result_t));
            } else {
                snprintf(s_pending_activation_result.error_msg, 
                         sizeof(s_pending_activation_result.error_msg), 
                         "Unknown error");
            }
            s_activation_fail_pending = true;
            lv_async_call(async_activation_fail_handler, NULL);
            break;
            
        default:
            break;
    }
}

/* 返回扫码页面按钮回调 */
static void btn_back_to_qr_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Back to QR code page - clearing activation state");
    
    /* 停止定位上报（如果正在运行） */
    if (wifi_location_is_reporting()) {
        wifi_location_stop_periodic_report();
    }
    
    /* 清除激活状态，这样用户可以重新激活设备获取新的 sec_key */
    cloud_manager_clear_activation();
    
    /* 隐藏激活后页面 */
    if (s_activated_panel != NULL) {
        lv_obj_add_flag(s_activated_panel, LV_OBJ_FLAG_HIDDEN);
    }
    
    /* 显示扫码页面 */
    if (s_qr_panel != NULL) {
        lv_obj_clear_flag(s_qr_panel, LV_OBJ_FLAG_HIDDEN);
    }
    
    /* 更新 QR 提示文本 */
    if (s_qr_hint != NULL) {
        lv_label_set_text(s_qr_hint, "Activation cleared. Please re-activate.");
        lv_obj_set_style_text_color(s_qr_hint, COLOR_WARNING, 0);
    }
    
    s_ui_state = CLOUD_UI_STATE_QR_CODE;
    
    ESP_LOGI(TAG, "Activation cleared. Device needs to be re-activated.");
}

/* WiFi定位上报按钮回调 - 启动/停止定位上报 */
static void btn_wifi_location_cb(lv_event_t *e)
{
    (void)e;
    
    ESP_LOGI(TAG, "WiFi location button clicked");
    
    /* 检查当前是否正在上报 */
    if (wifi_location_is_reporting()) {
        /* 正在上报，点击则停止 */
        ESP_LOGI(TAG, "Stopping periodic location report...");
        wifi_location_stop_periodic_report();
        
        /* 更新按钮显示 */
        lv_obj_t *label = lv_obj_get_child(s_btn_wifi_location, 0);
        if (label) {
            lv_label_set_text(label, LV_SYMBOL_GPS " Locate");
        }
        lv_obj_set_style_bg_color(s_btn_wifi_location, COLOR_PRIMARY, 0);
        
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "OneNet: Location stopped");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_TEXT_SEC, 0);
        }
        return;
    }
    
    /* 检查设备是否已激活 */
    if (!cloud_manager_is_activated()) {
        ESP_LOGW(TAG, "Device not activated");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "Device not activated!");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
        }
        return;
    }
    
    /* 检查WiFi连接 */
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "WiFi not connected!");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
        }
        return;
    }
    
    /* 获取当前下拉框选择的间隔，用于定位上报 */
    uint16_t idx = lv_dropdown_get_selected(s_interval_dropdown);
    /* 选项: 1min, 5min, 10min, 30min, 1hour */
    uint32_t intervals_min[] = {1, 5, 10, 30, 60};
    uint32_t interval_min = (idx < 5) ? intervals_min[idx] : 1;  /* 默认1分钟 */
    uint32_t interval_ms = interval_min * 60 * 1000;
    
    if (s_sync_onenet_label) {
        lv_label_set_text(s_sync_onenet_label, "Starting location report...");
        lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
    }
    
    ESP_LOGI(TAG, "Starting periodic location report (interval: %lu min)...", interval_min);
    
    esp_err_t ret = wifi_location_start_periodic_report(interval_ms);
    
    if (ret == ESP_OK) {
        /* 更新按钮显示为"停止" */
        lv_obj_t *label = lv_obj_get_child(s_btn_wifi_location, 0);
        if (label) {
            lv_label_set_text(label, LV_SYMBOL_CLOSE " Stop");
        }
        lv_obj_set_style_bg_color(s_btn_wifi_location, COLOR_WARNING, 0);
        
        if (s_sync_onenet_label) {
            char msg[64];
            snprintf(msg, sizeof(msg), "OneNet: Reporting every %lu min", interval_min);
            lv_label_set_text(s_sync_onenet_label, msg);
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_SUCCESS, 0);
        }
    } else {
        ESP_LOGE(TAG, "Failed to start periodic location report");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "Failed to start report!");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
        }
    }
}

/* 设备上线/下线按钮回调 */
static void btn_device_online_cb(lv_event_t *e)
{
    (void)e;
    
    ESP_LOGI(TAG, "Device online button clicked");
    
    /* 检查设备是否已激活 */
    if (!cloud_manager_is_activated()) {
        ESP_LOGW(TAG, "Device not activated");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "Device not activated!");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
        }
        return;
    }
    
    /* 检查WiFi连接 */
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "WiFi not connected!");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
        }
        return;
    }
    
    /* 检查当前在线状态 */
    if (onenet_is_device_online()) {
        /* 当前在线，点击则下线 */
        ESP_LOGI(TAG, "Setting device OFFLINE...");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "Going offline...");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
        }
        
        esp_err_t ret = onenet_device_offline();
        
        if (ret == ESP_OK) {
            /* 更新按钮显示 */
            lv_obj_t *label = lv_obj_get_child(s_btn_device_online, 0);
            if (label) {
                lv_label_set_text(label, LV_SYMBOL_WIFI " Connect");
            }
            lv_obj_set_style_bg_color(s_btn_device_online, COLOR_PRIMARY, 0);
            
            /* 更新在线状态标签 */
            if (s_congrats_label) {
                lv_label_set_text(s_congrats_label, "Offline");
                lv_obj_set_style_text_color(s_congrats_label, COLOR_WARNING, 0);
            }
            
            if (s_sync_onenet_label) {
                lv_label_set_text(s_sync_onenet_label, "OneNet: Offline");
                lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_TEXT_SEC, 0);
            }
            ESP_LOGI(TAG, "Device is now OFFLINE");
        } else {
            ESP_LOGE(TAG, "Failed to set device offline");
            if (s_sync_onenet_label) {
                lv_label_set_text(s_sync_onenet_label, "Offline failed!");
                lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
            }
        }
    } else {
        /* 当前离线，点击则上线 */
        ESP_LOGI(TAG, "Setting device ONLINE...");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "Going online...");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
        }
        
        esp_err_t ret = onenet_device_online();
        
        if (ret == ESP_OK) {
            /* 更新按钮显示 */
            lv_obj_t *label = lv_obj_get_child(s_btn_device_online, 0);
            if (label) {
                lv_label_set_text(label, LV_SYMBOL_OK " Online");
            }
            lv_obj_set_style_bg_color(s_btn_device_online, COLOR_SUCCESS, 0);
            
            /* 更新在线状态标签 */
            if (s_congrats_label) {
                lv_label_set_text(s_congrats_label, "Online");
                lv_obj_set_style_text_color(s_congrats_label, COLOR_SUCCESS, 0);
            }
            
            if (s_sync_onenet_label) {
                lv_label_set_text(s_sync_onenet_label, "OneNet: Online");
                lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_SUCCESS, 0);
            }
            ESP_LOGI(TAG, "Device is now ONLINE");
            
            /* WiFi定位需要手动点击Location按钮启动，不在这里自动启动 */
            
            /* 上线后自动刷新同步状态 */
            cloud_sync_status_t sync_status;
            if (cloud_manager_get_sync_status(&sync_status) == ESP_OK) {
                cloud_manager_ui_update_sync_status(&sync_status);
            }
        } else {
            ESP_LOGE(TAG, "Failed to set device online");
            if (s_sync_onenet_label) {
                lv_label_set_text(s_sync_onenet_label, "Online failed!");
                lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
            }
        }
    }
}

/* 手动刷新同步状态按钮回调 */
static void btn_refresh_sync_cb(lv_event_t *e)
{
    (void)e;
    
    ESP_LOGI(TAG, "Refresh sync button clicked");
    
    /* 启动刷新图标旋转动画 */
    start_refresh_animation();
    
    /* 检查WiFi连接 */
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "WiFi not connected!");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
        }
        return;
    }
    
    /* 检查设备是否已激活 */
    if (!cloud_manager_is_activated()) {
        ESP_LOGW(TAG, "Device not activated");
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "Device not activated!");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_ERROR, 0);
        }
        return;
    }
    
    /* 显示刷新中状态 */
    if (s_sync_onenet_label) {
        lv_label_set_text(s_sync_onenet_label, "Refreshing...");
        lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_WARNING, 0);
    }
    
    /* 从 OneNET 获取最新的定位结果 */
    location_info_t location = {0};
    esp_err_t ret = onenet_get_wifi_location(&location);
    
    if (ret == ESP_OK && location.valid) {
        /* 更新定位显示 */
        if (s_sync_location_label) {
            char text[80];
            snprintf(text, sizeof(text), "Loc: %.4f, %.4f", 
                     location.longitude, location.latitude);
            lv_label_set_text(s_sync_location_label, text);
            lv_obj_set_style_text_color(s_sync_location_label, COLOR_SUCCESS, 0);
        }
        
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "OneNet: Location updated");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_SUCCESS, 0);
        }
        ESP_LOGI(TAG, "Location refreshed: %.4f, %.4f", location.longitude, location.latitude);
    } else {
        if (s_sync_location_label) {
            lv_label_set_text(s_sync_location_label, "Location: No data");
            lv_obj_set_style_text_color(s_sync_location_label, COLOR_TEXT_SEC, 0);
        }
        
        if (s_sync_onenet_label) {
            lv_label_set_text(s_sync_onenet_label, "OneNet: No location data");
            lv_obj_set_style_text_color(s_sync_onenet_label, COLOR_TEXT_SEC, 0);
        }
        ESP_LOGW(TAG, "No location data available");
    }
    
    /* 刷新其他同步状态 */
    cloud_manager_ui_refresh();
}

static void btn_manual_upload_cb(lv_event_t *e)
{
    (void)e;
    cloud_manager_ui_show_file_browser();
}

static void auto_switch_cb(lv_event_t *e)
{
    (void)e;
    bool checked = lv_obj_has_state(s_auto_switch, LV_STATE_CHECKED);
    cloud_manager_set_auto_upload_enabled(checked);
    ESP_LOGI(TAG, "Auto upload %s", checked ? "enabled" : "disabled");
}

static void interval_dropdown_cb(lv_event_t *e)
{
    (void)e;
    uint16_t idx = lv_dropdown_get_selected(s_interval_dropdown);
    /* 选项: 1min, 5min, 10min, 30min, 1hour */
    uint32_t minutes[] = {1, 5, 10, 30, 60};
    if (idx < 5) {
        cloud_manager_set_auto_interval(minutes[idx]);
        ESP_LOGI(TAG, "Upload interval set to %lu minutes", (unsigned long)minutes[idx]);
        
        /* 如果定位上报正在运行，同步更新定位间隔 */
        if (wifi_location_is_reporting()) {
            uint32_t interval_ms = minutes[idx] * 60 * 1000;
            wifi_location_set_report_interval(interval_ms);
            ESP_LOGI(TAG, "Location report interval synced to %lu min", (unsigned long)minutes[idx]);
        }
    }
}

static void file_item_cb(lv_event_t *e)
{
    lv_obj_t *item = lv_event_get_target(e);
    char *path = (char *)lv_obj_get_user_data(item);
    
    if (path != NULL) {
        snprintf(s_selected_file_path, sizeof(s_selected_file_path), "%s", path);
        
        /* Extract filename for display */
        const char *fname = strrchr(path, '/');
        if (fname) fname++; else fname = path;
        
        if (s_selected_file_label != NULL) {
            char text[128];
            snprintf(text, sizeof(text), "Selected: %s", fname);
            lv_label_set_text(s_selected_file_label, text);
        }
        
        /* Highlight selected item */
        uint32_t count = lv_obj_get_child_cnt(s_file_list);
        for (uint32_t i = 0; i < count; i++) {
            lv_obj_t *child = lv_obj_get_child(s_file_list, i);
            if (child == item) {
                lv_obj_set_style_border_width(child, 2, 0);
                lv_obj_set_style_border_color(child, COLOR_PRIMARY, 0);
            } else {
                lv_obj_set_style_border_width(child, 0, 0);
            }
        }
        
        ESP_LOGI(TAG, "Selected file: %s", path);
    }
}

/* 异步上传任务参数 */
static uint32_t s_async_upload_task_id = 0;
static TaskHandle_t s_upload_task_handle = NULL;
static volatile bool s_upload_task_busy = false;  /* 标记任务是否正在执行上传 */

/* 异步上传任务函数 - 在独立线程中执行 */
static void async_upload_task(void *param)
{
    uint32_t task_id = (uint32_t)(uintptr_t)param;
    ESP_LOGI(TAG, "Async upload task started for task_id=%lu", (unsigned long)task_id);
    
    s_upload_task_busy = true;
    
    /* 执行上传 */
    esp_err_t ret = cloud_manager_execute_pending_upload(task_id);
    
    if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Upload execution returned: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "Async upload task completed");
    s_upload_task_busy = false;
    s_upload_task_handle = NULL;
    
    /* 不要使用 vTaskDelete(NULL)，因为 HTTP/SSL 库使用了 TLSP，
     * 删除任务会导致 TLSP 回调崩溃。
     * 让任务进入空闲状态，下次上传会创建新任务。
     */
    ESP_LOGI(TAG, "Upload task entering idle state");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));  /* 每10秒唤醒一次，避免看门狗问题 */
    }
}

static void btn_upload_file_cb(lv_event_t *e)
{
    (void)e;
    if (strlen(s_selected_file_path) == 0) {
        cloud_manager_ui_show_error(2, "Please select a file first");
        return;
    }
    
    /* Check if file already uploaded */
    if (cloud_manager_is_file_uploaded(s_selected_file_path)) {
        cloud_manager_ui_show_error(3, "This file has already been uploaded");
        return;
    }
    
    /* Check if upload is already in progress */
    if (s_upload_task_busy) {
        cloud_manager_ui_show_error(5, "Upload already in progress");
        return;
    }
    
    /* Hide file browser */
    cloud_manager_ui_hide_file_browser();
    
    /* Extract filename */
    const char *fname = strrchr(s_selected_file_path, '/');
    if (fname) fname++; else fname = s_selected_file_path;
    
    /* Show upload progress */
    cloud_manager_ui_show_upload_progress(fname);
    
    /* Create upload task */
    uint32_t task_id;
    esp_err_t ret = cloud_manager_upload_file(s_selected_file_path, &task_id);
    
    if (ret == ESP_OK) {
        s_current_upload_task_id = task_id;
        s_async_upload_task_id = task_id;
        ESP_LOGI(TAG, "Upload task created: %lu", (unsigned long)task_id);
        
        /* Register callbacks */
        cloud_manager_set_progress_callback(cloud_manager_ui_upload_progress_callback);
        cloud_manager_set_complete_callback(cloud_manager_ui_upload_complete_callback);
        
        /* 创建异步上传任务 - 在独立线程中执行，不阻塞UI */
        ESP_LOGI(TAG, "Starting async upload task...");
        BaseType_t xret = xTaskCreate(
            async_upload_task,
            "upload_task",
            8192,  /* 8KB栈空间 */
            (void *)(uintptr_t)task_id,
            5,     /* 优先级 */
            &s_upload_task_handle
        );
        
        if (xret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create upload task");
            cloud_manager_ui_hide_upload_progress();
            cloud_manager_ui_show_error(5, "Failed to start upload");
            s_upload_task_handle = NULL;
        }
    } else {
        cloud_manager_ui_hide_upload_progress();
        
        const char *err_msg = "Failed to create upload task";
        if (ret == ESP_ERR_NOT_FOUND) err_msg = "File not found or cannot be read";
        else if (ret == ESP_ERR_NO_MEM) err_msg = "Upload queue is full";
        
        cloud_manager_ui_show_error(5, err_msg);
    }
}

static void btn_cancel_browse_cb(lv_event_t *e)
{
    (void)e;
    cloud_manager_ui_hide_file_browser();
}

static void btn_cancel_upload_cb(lv_event_t *e)
{
    (void)e;
    if (s_current_upload_task_id > 0) {
        cloud_manager_cancel_upload(s_current_upload_task_id);
        s_current_upload_task_id = 0;
    }
    cloud_manager_ui_hide_upload_progress();
}

static void btn_close_error_cb(lv_event_t *e)
{
    (void)e;
    if (s_error_popup != NULL) {
        lv_obj_add_flag(s_error_popup, LV_OBJ_FLAG_HIDDEN);
    }
}
