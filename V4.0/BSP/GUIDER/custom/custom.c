/*
* Copyright 2023 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "lvgl.h"
#include "custom.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_onenet.h"
#include "wifi_manager.h"
#include "weather_api.h"
#include "screenshot.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdcard_manager.h"
#include "cloud_manager.h"
#include "esp_netif.h"

/* wifi_onenet.h already includes all necessary type definitions */
/* No need to redefine types here */

/*********************
 *      DEFINES
 *********************/

/**********************
 *  FONT DECLARATIONS
 **********************/
LV_FONT_DECLARE(lv_font_ShanHaiZhongXiaYeWuYuW_22);
LV_FONT_DECLARE(myFont);  // 中文字库（从Flash分区加载）

/**********************
 *  IMAGE DECLARATIONS
 **********************/
LV_IMG_DECLARE(_cloud_alpha_123x90);   // AI头像图标（云朵）
LV_IMG_DECLARE(_cosbb_alpha_30x30);    // 用户头像图标（替代）
LV_IMG_DECLARE(_wifi_alpha_39x34);     // WiFi connected icon
LV_IMG_DECLARE(_internet_alpha_66x66); // Internet connected icon (green)
LV_IMG_DECLARE(_no_internet_alpha_66x65); // No internet icon (for loading/disconnected)
LV_IMG_DECLARE(_load_alpha_40x40);     // Loading圆圈气泡图标

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_timer_t * time_update_timer = NULL;
static bool wifi_connected_status = false;  /* Track WiFi connection status */
static bool g_onenet_http_online = false;   /* Track OneNet HTTP online status */
static lv_ui * cached_ui = NULL;            /* Cache UI pointer for screen events */
/* static lv_obj_t * label_runtime = NULL; */     /* Dynamic runtime/update time label - 未使用，已注释 */
/* static lv_timer_t * runtime_update_timer = NULL; */ /* Runtime update timer - 未使用，已注释 */
/* static uint32_t system_start_time = 0; */  /* System start timestamp - 未使用，已注释 */
static uint32_t last_data_update_time = 0;  /* Last data update timestamp */

/* Carousel effect parameters */
#define CARD_SIZE_SIDE   100    /* Card size at sides (100x100) */
#define CARD_SIZE_CENTER 180    /* Card size at center (160x160) */
#define CAROUSEL_SPACING 100    /* Distance for full transition */
#define CARD_GAP         25     /* Fixed gap between cards (edge to edge) */

/* Menu items information with specific icon sizes */
typedef struct {
    lv_obj_t *obj;
    lv_obj_t *icon;
    lv_obj_t *label;
    lv_coord_t base_x;            /* Base X position (for spacing) */
    lv_coord_t orig_icon_width;   /* Original icon width */
    lv_coord_t orig_icon_height;  /* Original icon height */
    lv_coord_t icon_size_side;    /* Icon size at sides */
    lv_coord_t icon_size_center;  /* Icon size at center */
} menu_item_info_t;

static menu_item_info_t menu_items_info[6];
static lv_timer_t * carousel_init_timer = NULL;

/* OneNet integration variables */
static const char *ONENET_TAG = "ONENET_UI";
static bool g_onenet_enabled = false;
/* g_onenet_http_online is already defined above */
static lv_timer_t *g_onenet_process_timer = NULL;
static lv_obj_t *g_location_label = NULL;
static location_info_t g_last_location = {0};

/* Weather display variables */
static const char *WEATHER_TAG = "WEATHER_UI";
static lv_obj_t *g_weather_label = NULL;  /* 天气信息显示标签 */
static lv_obj_t *g_weather_icon_cc = NULL;  /* 摄氏度图标 */
static weather_info_t g_last_weather = {0};  /* 最后一次天气数据 */

/**
 * @brief Reset weather display pointers (called when scrHome is reloaded)
 *
 * This function should be called when scrHome screen is loaded to ensure
 * that the weather display pointers are reset, preventing access to
 * invalid objects that may have been deleted when the screen was unloaded.
 */
void reset_weather_display_pointers(void)
{
    ESP_LOGI(WEATHER_TAG, "Resetting weather display pointers");
    g_weather_label = NULL;
    g_weather_icon_cc = NULL;
}

/**
 * Create a demo application
 */

void configure_scrHome_scroll(lv_ui *ui)
{
    /* Configure scroll direction for scrHome and contMain */
    if (ui->scrHome != NULL) {
        /* Disable scrolling on scrHome (no horizontal or vertical scrolling) */
        lv_obj_clear_flag(ui->scrHome, LV_OBJ_FLAG_SCROLLABLE);
    }
    
    if (ui->scrHome_contMain != NULL) {
        /* Enable horizontal scrolling only on contMain */
        lv_obj_set_scroll_dir(ui->scrHome_contMain, LV_DIR_HOR);
        lv_obj_add_flag(ui->scrHome_contMain, LV_OBJ_FLAG_SCROLLABLE);
        
        /* Add right padding to contMain to create spacing after cont_2 */
        lv_obj_set_style_pad_right(ui->scrHome_contMain, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    
    /* Disable scrolling on cont_2 (Settings card) */
    if (ui->scrHome_cont_2 != NULL) {
        lv_obj_clear_flag(ui->scrHome_cont_2, LV_OBJ_FLAG_SCROLLABLE);
    }
}

/**
 * Update WiFi location display
 * NOTE: Disabled - weather display is used instead
 */
void update_runtime_display(lv_timer_t * timer)
{
    (void)timer; /* Unused parameter */

    /* 已禁用 - 改为显示天气信息 */
    /* Disabled - weather display is used instead */
}

/**
 * Create WiFi location display in status bar (left side, aligned with time on right)
 * NOTE: Disabled to make room for weather display
 */
void create_runtime_display(lv_ui *ui)
{
    /* 已禁用位置显示，改为显示天气信息 */
    /* Location display has been disabled to show weather information instead */
    (void)ui;  /* Suppress unused parameter warning */

    /* No longer creating location label - weather display is shown instead */
    return;
}

/**
 * Update data update timestamp (call this when new data is received via WiFi)
 * 
 * ESP32 Integration Example:
 * 
 * 1. Auto update on WiFi connection:
 *    void wifi_event_handler(void* arg, esp_event_base_t event_base,
 *                            int32_t event_id, void* event_data) {
 *        if (event_id == WIFI_EVENT_STA_CONNECTED) {
 *            update_wifi_status(&guider_ui, true);  // Auto calls update_data_timestamp()
 *        }
 *    }
 * 
 * 2. Update on data received (e.g., from OneNET cloud):
 *    void onenet_data_callback(const char* data) {
 *        process_sensor_data(data);
 *        update_data_timestamp();  // Display shows "Update: Xs ago"
 *    }
 * 
 * 3. Update on UART/STM32 communication:
 *    void uart_data_received(uint8_t* data, size_t len) {
 *        parse_stm32_data(data, len);
 *        update_data_timestamp();  // Record data receive time
 *    }
 * 
 * Display Logic:
 * - WiFi disconnected: Shows "Runtime: HH:MM:SS" (RED)
 * - WiFi connected with data: Shows "Update: Xs ago" or "Update: Xm Ys ago" (GREEN)
 */
void update_data_timestamp(void)
{
    last_data_update_time = lv_tick_get() / 1000;
}

/**
 * Set WiFi connection status flag
 * This is used by event handlers to update the status before screen transitions
 */
void set_wifi_connected_status(bool is_connected)
{
    wifi_connected_status = is_connected;
}

/* WiFi连接状态标志（由WiFi事件回调设置，由LVGL定时器读取） */
static volatile bool g_wifi_connection_pending = false;
static volatile bool g_wifi_connected_state = false;

/**
 * @brief 延迟初始化HTTP的定时器回调
 */
static void delayed_http_init_timer_cb(lv_timer_t * timer)
{
    if (g_onenet_enabled) {
        ESP_LOGI(ONENET_TAG, "Initializing OneNet HTTP (delayed)...");
        /* HTTP模式不需要显式启动连接，只需要初始化 */
        esp_err_t ret = onenet_http_init();
        if (ret != ESP_OK) {
            ESP_LOGE(ONENET_TAG, "Failed to init HTTP: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(ONENET_TAG, "HTTP initialized successfully");
            /* WiFi定位上报将在设备激活后通过HTTP上报 */
        }
    }

    /* 删除一次性定时器 */
    lv_timer_del(timer);
}

/**
 * @brief WiFi状态轮询定时器（在LVGL任务中安全执行）
 */
static void wifi_status_poll_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    /* 检查是否有待处理的WiFi状态变化 */
    if (g_wifi_connection_pending) {
        g_wifi_connection_pending = false;

        bool connected = g_wifi_connected_state;
        ESP_LOGI(ONENET_TAG, "Processing WiFi status change: %s",
                 connected ? "Connected" : "Disconnected");

        wifi_connected_status = connected;

        /* 更新状态标志 */
        set_wifi_connected_status(connected);

        /* 更新UI显示（将加载图标改为WiFi图标） */
        if (cached_ui != NULL) {
            ESP_LOGI(ONENET_TAG, "Updating WiFi status UI...");
            update_wifi_status(cached_ui, connected);
        } else {
            ESP_LOGW(ONENET_TAG, "Cached UI is NULL, cannot update WiFi status display");
        }

        /* WiFi连接后延迟初始化HTTP（给ESP32-C6足够时间稳定） */
        if (connected && g_onenet_enabled) {
            ESP_LOGI(ONENET_TAG, "Scheduling OneNet HTTP init in 3 seconds...");
            ESP_LOGI(ONENET_TAG, "Waiting for ESP32-C6 WiFi to stabilize...");
            /* 创建一次性定时器，3秒后初始化HTTP */
            lv_timer_t *http_timer = lv_timer_create(delayed_http_init_timer_cb, 3000, NULL);
            lv_timer_set_repeat_count(http_timer, 1);
        }

        /* WiFi连接成功后启动天气自动更新 */
        if (connected) {
            ESP_LOGI(WEATHER_TAG, "WiFi connected, starting weather auto-update...");
            weather_api_start_auto_update();
            
            /* 获取并更新 IP 地址显示 */
            extern void wireless_serial_update_ip_display(const char *, uint16_t);
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_ip_info_t ip_info;
                if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                    char ip_str[16];
                    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
                    ESP_LOGI("WIRELESS_SERIAL", "WiFi IP: %s", ip_str);
                    wireless_serial_update_ip_display(ip_str, 8888);
                }
            }
        } else {
            /* WiFi断开时停止天气更新 */
            ESP_LOGI(WEATHER_TAG, "WiFi disconnected, stopping weather auto-update");
            weather_api_stop_auto_update();
            
            /* 更新 IP 显示为断开状态 */
            extern void wireless_serial_update_ip_display(const char *, uint16_t);
            wireless_serial_update_ip_display(NULL, 0);
            
            /* WiFi断开时设置设备离线状态（本地） */
            if (g_onenet_enabled && cloud_manager_is_activated()) {
                ESP_LOGI(ONENET_TAG, "Setting device OFFLINE due to WiFi disconnect...");
                onenet_device_set_offline_local();
            }
        }
    }
}

/**
 * @brief OneNet WiFi状态回调（符合wifi_manager的回调签名）
 * @param connected WiFi连接状态
 * @param ssid 连接的SSID（可能为NULL）
 *
 * @note 这个函数在WiFi事件处理器（中断上下文）中被调用
 * @warning 不要在这里调用任何LVGL函数！只设置标志位
 */
void onenet_wifi_status_callback(bool connected, const char *ssid)
{
    ESP_LOGI(ONENET_TAG, "WiFi status callback: %s (SSID: %s)",
             connected ? "Connected" : "Disconnected",
             ssid ? ssid : "N/A");

    /* 只设置标志位，实际处理由LVGL定时器完成 */
    g_wifi_connected_state = connected;
    g_wifi_connection_pending = true;
}

/**
 * @brief OneNet命令回调
 */
static void onenet_cmd_callback(const char* topic, const char* data, int data_len)
{
    ESP_LOGI(ONENET_TAG, "Received command: %.*s", data_len, data);
}

/**
 * @brief WiFi定位结果回调
 */
static void location_callback(const location_info_t *location)
{
    if (location == NULL || !location->valid) {
        return;
    }

    memcpy(&g_last_location, location, sizeof(location_info_t));

    ESP_LOGI(ONENET_TAG, "Location: %s", location->address);
    ESP_LOGI(ONENET_TAG, "   Coordinates: %.6f, %.6f", location->longitude, location->latitude);
    ESP_LOGI(ONENET_TAG, "   Accuracy: %.1f meters", location->radius);

    /* 更新UI显示 */
    if (g_location_label != NULL && lv_obj_is_valid(g_location_label)) {
        char loc_str[128];
        snprintf(loc_str, sizeof(loc_str), "Loc: %.6f, %.6f",
                 location->longitude, location->latitude);
        lv_label_set_text(g_location_label, loc_str);
    }
}

/**
 * @brief 天气数据回调函数
 */
static void weather_callback(const weather_info_t *weather)
{
    if (weather == NULL || !weather->valid) {
        ESP_LOGW(WEATHER_TAG, "Invalid weather data received");
        return;
    }

    /* 保存天气数据 */
    memcpy(&g_last_weather, weather, sizeof(weather_info_t));

    ESP_LOGI(WEATHER_TAG, "Weather updated: %s, %s, %d°C",
             weather->location_name, weather->text, weather->temperature);

    /* 更新UI显示 - 精简格式：城市 天气 温度（不含°C，用图标代替） */
    if (g_weather_label != NULL && lv_obj_is_valid(g_weather_label)) {
        char weather_str[64];
        snprintf(weather_str, sizeof(weather_str), "%s  %s  %d",
                 weather->location_name, weather->text,
                 weather->temperature);
        lv_label_set_text(g_weather_label, weather_str);
        ESP_LOGI(WEATHER_TAG, "Weather label updated with: %s", weather_str);

        /* 显示摄氏度图标（只有获取到天气数据后才显示） */
        if (g_weather_icon_cc != NULL && lv_obj_is_valid(g_weather_icon_cc)) {
            lv_obj_clear_flag(g_weather_icon_cc, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(WEATHER_TAG, "CC icon shown");
        }
    } else {
        ESP_LOGW(WEATHER_TAG, "Weather label is NULL or invalid! g_weather_label=%p, valid=%d",
                 g_weather_label, g_weather_label ? lv_obj_is_valid(g_weather_label) : 0);
    }
}

/**
 * @brief 创建天气显示标签（左上角）
 */
static void create_weather_display(lv_ui *ui)
{
    ESP_LOGI(WEATHER_TAG, "create_weather_display called, ui=%p", ui);

    if (ui == NULL || ui->scrHome_contBG == NULL) {
        ESP_LOGW(WEATHER_TAG, "Cannot create weather display: UI not ready (ui=%p, contBG=%p)",
                 ui, ui ? ui->scrHome_contBG : NULL);
        return;
    }

    /* 检查scrHome_contBG对象是否有效 */
    if (!lv_obj_is_valid(ui->scrHome_contBG)) {
        ESP_LOGE(WEATHER_TAG, "scrHome_contBG is invalid, cannot create weather display");
        return;
    }

    /* 检查当前加载的屏幕是否是scrHome */
    lv_obj_t *current_screen = lv_scr_act();
    if (current_screen != ui->scrHome) {
        ESP_LOGW(WEATHER_TAG, "Current screen is not scrHome, skipping weather display creation");
        return;
    }

    ESP_LOGI(WEATHER_TAG, "UI is ready, scrHome_contBG=%p", ui->scrHome_contBG);

    /* 删除旧标签 - 使用更安全的检查 */
    if (g_weather_label != NULL) {
        /* Check if object is still valid before deleting */
        if (lv_obj_is_valid(g_weather_label)) {
            /* Check if parent is still valid */
            lv_obj_t *parent = lv_obj_get_parent(g_weather_label);
            if (parent != NULL && lv_obj_is_valid(parent)) {
                ESP_LOGI(WEATHER_TAG, "Deleting old weather label");
                lv_obj_del(g_weather_label);
            } else {
                ESP_LOGW(WEATHER_TAG, "Weather label parent is invalid, skipping delete");
            }
        } else {
            ESP_LOGW(WEATHER_TAG, "Weather label is invalid, skipping delete");
        }
        g_weather_label = NULL;
    }

    /* 删除旧图标 - 使用更安全的检查 */
    if (g_weather_icon_cc != NULL) {
        /* Check if object is still valid before deleting */
        if (lv_obj_is_valid(g_weather_icon_cc)) {
            /* Check if parent is still valid */
            lv_obj_t *parent = lv_obj_get_parent(g_weather_icon_cc);
            if (parent != NULL && lv_obj_is_valid(parent)) {
                ESP_LOGI(WEATHER_TAG, "Deleting old CC icon");
                lv_obj_del(g_weather_icon_cc);
            } else {
                ESP_LOGW(WEATHER_TAG, "CC icon parent is invalid, skipping delete");
            }
        } else {
            ESP_LOGW(WEATHER_TAG, "CC icon is invalid, skipping delete");
        }
        g_weather_icon_cc = NULL;
    }

    /* 再次检查父容器是否有效（防止在删除旧对象时父容器被删除） */
    if (!lv_obj_is_valid(ui->scrHome_contBG)) {
        ESP_LOGE(WEATHER_TAG, "scrHome_contBG became invalid after cleanup, aborting");
        return;
    }

    /* 创建天气标签 */
    ESP_LOGI(WEATHER_TAG, "Creating weather label in scrHome_contBG...");
    g_weather_label = lv_label_create(ui->scrHome_contBG);
    if (g_weather_label == NULL) {
        ESP_LOGE(WEATHER_TAG, "Failed to create weather label");
        return;
    }
    ESP_LOGI(WEATHER_TAG, "Weather label created successfully: %p", g_weather_label);

    /* 设置初始文本 */
    lv_label_set_text(g_weather_label, "Loading...");
    ESP_LOGI(WEATHER_TAG, "Initial text set to 'Loading...'");

    /* 设置位置（左上角，WiFi图标右侧，与时间y坐标一致） */
    lv_obj_set_pos(g_weather_label, 75, 15);  /* WiFi图标在x=25，宽40，留10px间距；y=15与时间一致 */

    /* 设置文本样式（白色，30pt字体，与时间样式一致） */
    lv_obj_set_style_text_color(g_weather_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(g_weather_label, &lv_font_ShanHaiZhongXiaYeWuYuW_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(g_weather_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* 再次检查父容器是否有效（防止在创建标签时发生屏幕切换） */
    if (!lv_obj_is_valid(ui->scrHome_contBG)) {
        ESP_LOGE(WEATHER_TAG, "scrHome_contBG became invalid after creating label, aborting");
        return;
    }

    /* 创建摄氏度图标（在温度数字右侧，向右移动100个单位） */
    ESP_LOGI(WEATHER_TAG, "Creating CC icon in scrHome_contBG...");
    g_weather_icon_cc = lv_img_create(ui->scrHome_contBG);
    if (g_weather_icon_cc == NULL) {
        ESP_LOGE(WEATHER_TAG, "Failed to create CC icon");
    } else {
        /* CC icon removed - using text label instead */
        // lv_img_set_src(g_weather_icon_cc, &_CC_alpha_47x39);
        /* 位置：温度文本约在x=75+150=225，向右移动100单位 = 325 */
        /* y坐标：从7向下移动3个像素到10 */
        lv_obj_set_pos(g_weather_icon_cc, 345, 10);  /* y=10，向下移动3个像素 */
        lv_obj_set_size(g_weather_icon_cc, 38, 38);
        lv_obj_set_style_img_opa(g_weather_icon_cc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
        /* 设置图标颜色为白色 */
        lv_obj_set_style_img_recolor(g_weather_icon_cc, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_img_recolor_opa(g_weather_icon_cc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
        /* 初始隐藏图标，只有获取到天气数据后才显示 */
        lv_obj_add_flag(g_weather_icon_cc, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(WEATHER_TAG, "CC icon created successfully (hidden): %p", g_weather_icon_cc);
    }

    ESP_LOGI(WEATHER_TAG, "Weather display created");

    /* 如果已有天气数据，立即更新显示 */
    if (g_last_weather.valid) {
        weather_callback(&g_last_weather);
    }
}

/* 位置更新定时器相关变量 */
static uint32_t s_last_location_update_tick = 0;  // 上次位置更新的tick
static const uint32_t LOCATION_UPDATE_INTERVAL_MS = 300000;  // 5分钟 = 300000ms

/**
 * @brief OneNet处理定时器
 */
static void onenet_process_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    /* 调用OneNet处理函数 */
    onenet_process();
    
    /* 注意: cloud_manager_process() 不在这里调用 */
    /* 上传任务处理在 cloud_manager_ui.c 中通过独立机制完成 */

    /* 更新HTTP在线状态 */
    onenet_state_t http_state = onenet_http_get_state();
    bool http_online = (http_state == ONENET_STATE_ONLINE);

    if (http_online != g_onenet_http_online) {
        g_onenet_http_online = http_online;
        ESP_LOGI(ONENET_TAG, "HTTP state: %s", http_online ? "Online" : "Offline");
    }

    /* 每5分钟触发一次位置更新 */
    if (wifi_connected_status && cloud_manager_is_activated()) {
        uint32_t current_tick = lv_tick_get();
        if (s_last_location_update_tick == 0 ||
            (current_tick - s_last_location_update_tick) >= LOCATION_UPDATE_INTERVAL_MS) {
            ESP_LOGI(ONENET_TAG, "Triggering periodic location update (5 min interval)");
            onenet_ui_trigger_location_request();
            s_last_location_update_tick = current_tick;
        }
    }
}

/**
 * @brief 创建位置显示标签（左上角）- 已禁用
 */
/*
static void create_location_display(lv_ui *ui)
{
    if (ui == NULL || ui->scrHome == NULL) {
        return;
    }

    // 删除旧标签
    if (g_location_label != NULL && lv_obj_is_valid(g_location_label)) {
        lv_obj_del(g_location_label);
        g_location_label = NULL;
    }

    // 创建位置标签
    g_location_label = lv_label_create(ui->scrHome);
    lv_obj_set_pos(g_location_label, 10, 10);
    lv_obj_set_style_text_color(g_location_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_location_label, &lv_font_montserrat_16, LV_PART_MAIN);

    // 未连接WiFi时不显示任何文字
    lv_label_set_text(g_location_label, "");
}
*/


void custom_init(lv_ui *ui)
{
    /* Add your codes here */

    /* Cache UI pointer */
    cached_ui = ui;


    /* 尝试从NVS恢复上次保存的时间 - 必须在创建定时器之前执行 */
    ESP_LOGI(ONENET_TAG, "Attempting to restore time from NVS...");
    esp_err_t time_ret = onenet_restore_time_from_nvs();
    if (time_ret == ESP_OK) {
        ESP_LOGI(ONENET_TAG, "✅ Time restored successfully from NVS");
    } else {
        ESP_LOGI(ONENET_TAG, "⚠️  No saved time found, will sync from SNTP after WiFi connection");
    }

    /* Create a timer to update time display every second */
    if (time_update_timer == NULL) {
        time_update_timer = lv_timer_create(update_time_display, 1000, NULL);
    }

    /* Initialize WiFi status display with loading animation */
    update_wifi_status(ui, false);

    /* Create runtime/data update time display */
    create_runtime_display(ui);

    /* Apply scrHome scroll configuration */
    configure_scrHome_scroll(ui);

    /* Setup carousel effect for contMain menu */
    setup_contMain_carousel_effect(ui);

    /* Initialize WiFi manager first (required for OneNet) */
    ESP_LOGI(ONENET_TAG, "Initializing WiFi manager...");

    esp_err_t wifi_ret = wifi_manager_init();
    if (wifi_ret != ESP_OK) {
        ESP_LOGE(ONENET_TAG, "Failed to initialize WiFi manager: %s", esp_err_to_name(wifi_ret));
        ESP_LOGW(ONENET_TAG, "Continuing without WiFi...");
        return;
    }

    /* 创建WiFi状态轮询定时器（每100ms检查一次） */
    static lv_timer_t *wifi_poll_timer = NULL;
    if (wifi_poll_timer == NULL) {
        wifi_poll_timer = lv_timer_create(wifi_status_poll_timer_cb, 100, NULL);
        ESP_LOGI(ONENET_TAG, "WiFi status poll timer created");
    }

    /* Try to auto-connect to saved WiFi */
    ESP_LOGI(ONENET_TAG, "Attempting to auto-connect to saved WiFi...");
    esp_err_t auto_connect_ret = wifi_manager_auto_connect(onenet_wifi_status_callback);
    if (auto_connect_ret == ESP_OK) {
        ESP_LOGI(ONENET_TAG, "Auto-connecting to saved WiFi network...");
    } else if (auto_connect_ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(ONENET_TAG, "No saved WiFi credentials found, please connect manually from Settings");
    } else {
        ESP_LOGW(ONENET_TAG, "Failed to auto-connect: %s", esp_err_to_name(auto_connect_ret));
    }

    /* Note: WiFi status callback will be set when connecting via wifi_manager_connect() */
    /* The callback is passed as a parameter to wifi_manager_connect() in events_init.c */

    /* Initialize OneNet HTTP (WiFi is managed by wifi_manager) */
    ESP_LOGI(ONENET_TAG, "Initializing OneNet HTTP...");
    esp_err_t ret = onenet_http_init();
    if (ret == ESP_OK) {
        g_onenet_enabled = true;

        /* 设置定位回调 */
        wifi_location_set_callback(location_callback);

        /* 创建OneNet处理定时器（每100ms） */
        if (g_onenet_process_timer == NULL) {
            g_onenet_process_timer = lv_timer_create(onenet_process_timer_cb, 100, NULL);
        }

        /* 创建位置显示 - 已禁用，不显示定位文字 */
        /* create_location_display(ui); */

        ESP_LOGI(ONENET_TAG, "OneNet HTTP integration initialized successfully");
    } else {
        ESP_LOGE(ONENET_TAG, "Failed to initialize OneNet HTTP");
    }

    /* 初始化天气API模块 */
    ESP_LOGI(WEATHER_TAG, "Initializing Weather API...");
    esp_err_t weather_ret = weather_api_init(weather_callback);
    if (weather_ret == ESP_OK) {
        /* 创建天气显示UI */
        create_weather_display(ui);

        /* 注意：不在这里启动自动更新，等WiFi连接成功后再启动 */
        ESP_LOGI(WEATHER_TAG, "Weather API initialized successfully");
        ESP_LOGI(WEATHER_TAG, "Weather auto-update will start after WiFi connection");
    } else {
        ESP_LOGE(WEATHER_TAG, "Failed to initialize Weather API");
    }

    /* 初始化 SD 卡管理模块 */
    ESP_LOGI("SDCARD", "Initializing SD Card Manager...");
    esp_err_t sdcard_ret = sdcard_manager_init();
    if (sdcard_ret == ESP_OK) {
        ESP_LOGI("SDCARD", "SD Card Manager initialized successfully");
    } else {
        ESP_LOGW("SDCARD", "SD Card Manager init failed: %s", esp_err_to_name(sdcard_ret));
    }

    /* 初始化云平台管理模块 */
    ESP_LOGI("CLOUD", "Initializing Cloud Manager...");
    esp_err_t cloud_ret = cloud_manager_init();
    if (cloud_ret == ESP_OK) {
        ESP_LOGI("CLOUD", "Cloud Manager initialized successfully");
        
        /* 注意: cloud_manager_process() 不在这里调用，避免链接问题 */
        /* 上传进度更新由 cloud_manager_ui.c 中的定时器处理 */
        
        /* 清除激活状态，使设备显示二维码激活界面 */
        ESP_LOGI("CLOUD", "Clearing activation state for QR code display...");
        cloud_manager_clear_activation();
        
        if (cloud_manager_is_activated()) {
            ESP_LOGI("CLOUD", "Device is already activated");
        } else {
            ESP_LOGI("CLOUD", "Device not activated yet");
        }
    } else {
        ESP_LOGW("CLOUD", "Cloud Manager init failed: %s", esp_err_to_name(cloud_ret));
    }

    /* 截屏功能在 main.c 中初始化，这里不再重复初始化 */
    /* screenshot_init() is now called in main.c before custom_init() */

    /* 初始化无线串口模块 */
    ESP_LOGI("WIRELESS_SERIAL", "Initializing Wireless Serial...");
    extern esp_err_t wireless_serial_init(void*, void*);
    extern esp_err_t wireless_serial_start_server(void);
    esp_err_t ws_ret = wireless_serial_init(NULL, NULL);
    if (ws_ret == ESP_OK) {
        ESP_LOGI("WIRELESS_SERIAL", "Wireless Serial initialized successfully");
        
        /* 自动启动 TCP Server（用于快速测试） */
        ESP_LOGI("WIRELESS_SERIAL", "Auto-starting TCP Server on port 8888...");
        esp_err_t server_ret = wireless_serial_start_server();
        if (server_ret == ESP_OK) {
            ESP_LOGI("WIRELESS_SERIAL", "TCP Server started successfully");
        } else {
            ESP_LOGW("WIRELESS_SERIAL", "Failed to start TCP Server: %s", esp_err_to_name(server_ret));
        }
    } else {
        ESP_LOGW("WIRELESS_SERIAL", "Wireless Serial init failed: %s", esp_err_to_name(ws_ret));
    }
}


void slider_adjust_img_cb(lv_obj_t * img, int32_t brightValue, int16_t hueValue)
{
    static lv_color_t recolor;

    recolor = lv_color_hsv_to_rgb(hueValue, 100, brightValue);

    lv_obj_set_style_img_recolor(img, recolor, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(img, 50, LV_PART_MAIN|LV_STATE_DEFAULT);
}

void update_time_display(lv_timer_t * timer)
{
    (void)timer; /* Unused parameter */

    static char time_str[32];
    static char date_str[32];
    bool time_valid = false;
    static uint32_t log_counter = 0;  /* 用于减少日志输出频率 */
    static bool function_called_logged = false;

    /* 第一次调用时打印日志，确认函数被执行 */
    if (!function_called_logged) {
        ESP_LOGI(ONENET_TAG, "✅ update_time_display() function is being called");
        function_called_logged = true;
    }

    /* 直接获取系统时间（已通过SNTP同步） */
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    /* 每10秒打印一次调试信息 */
    if (log_counter % 10 == 0) {
        ESP_LOGI(ONENET_TAG, "🕐 System time check:");
        ESP_LOGI(ONENET_TAG, "   Timestamp: %ld", (long)now);
        ESP_LOGI(ONENET_TAG, "   Year: %d (valid if >= 2020)", timeinfo.tm_year + 1900);
        ESP_LOGI(ONENET_TAG, "   Time: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
    log_counter++;

    /* 检查时间是否有效（年份 >= 2020） */
    bool time_is_valid = (timeinfo.tm_year + 1900 >= 2020);

    if (time_is_valid) {
        /* 系统时间有效，直接使用（SNTP会自动同步） */
        /* 使用逗号分隔的格式：1,Aug,2025,12:00 */
        strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);  /* 只显示时:分，不显示秒 */
        /* 手动构建日期字符串，使用逗号分隔 */
        char month_str[16];
        strftime(month_str, sizeof(month_str), "%b", &timeinfo);  /* 月份缩写 */
        snprintf(date_str, sizeof(date_str), "%d,%s,%d",
                 timeinfo.tm_mday, month_str, timeinfo.tm_year + 1900);
        time_valid = true;

        /* 第一次同步成功时打印日志 */
        static bool first_sync_logged = false;
        if (!first_sync_logged) {
            ESP_LOGI(ONENET_TAG, "========================================");
            ESP_LOGI(ONENET_TAG, "✅ Time display updated successfully!");
            ESP_LOGI(ONENET_TAG, "   Date: %s", date_str);
            ESP_LOGI(ONENET_TAG, "   Time: %s", time_str);
            ESP_LOGI(ONENET_TAG, "========================================");
            first_sync_logged = true;
        }

        /* 每60秒（60次调用）保存一次时间到NVS，用于断电恢复 */
        if (log_counter % 60 == 0) {
            onenet_save_time_to_nvs();
        }
    } else {
        /* 时间未同步 - 显示占位符，使用逗号分隔格式 */
        snprintf(time_str, sizeof(time_str), "--:--");
        snprintf(date_str, sizeof(date_str), "--,---,----");

        /* 打印等待同步的日志 */
        if (log_counter % 10 == 0) {
            ESP_LOGW(ONENET_TAG, "⏳ Waiting for SNTP time synchronization...");
        }
    }

    /* Update the time display if scrHome is active and spangroup exists */
    if (guider_ui.scrHome != NULL && lv_obj_is_valid(guider_ui.scrHome)) {
        lv_obj_t * act_scr = lv_scr_act();
        if (act_scr == guider_ui.scrHome && guider_ui.scrHome_spangroup_1 != NULL &&
            lv_obj_is_valid(guider_ui.scrHome_spangroup_1)) {

            /* Clear existing spans safely */
            lv_span_t *first_span = lv_spangroup_get_child(guider_ui.scrHome_spangroup_1, 0);
            if (first_span != NULL) {
                lv_spangroup_del_span(guider_ui.scrHome_spangroup_1, first_span);
            }

            /* Create new span with updated time */
            lv_span_t *span = lv_spangroup_new_span(guider_ui.scrHome_spangroup_1);
            if (span == NULL) {
                ESP_LOGW(ONENET_TAG, "Failed to create new span for time display");
                return;
            }

            /* Combine date and time with comma separator */
            static char full_time_str[64];
            snprintf(full_time_str, sizeof(full_time_str), "%s,%s", date_str, time_str);
            lv_span_set_text(span, full_time_str);

            /* Set color based on WiFi connection and time validity */
            if (wifi_connected_status && time_valid) {
                /* Connected and time valid - White */
                lv_style_set_text_color(&span->style, lv_color_hex(0xffffff));
            } else {
                /* Disconnected or invalid - Red */
                lv_style_set_text_color(&span->style, lv_color_hex(0xff4444));
            }

            lv_style_set_text_decor(&span->style, LV_TEXT_DECOR_NONE);
            /* 无论WiFi是否连接，都使用lv_font_ShanHaiZhongXiaYeWuYuW_30字体 */
            lv_style_set_text_font(&span->style, &lv_font_ShanHaiZhongXiaYeWuYuW_30);

            /* Refresh the spangroup */
            lv_spangroup_refr_mode(guider_ui.scrHome_spangroup_1);
        }
    }
}

/**
 * Update menu item based on interpolation factor (0.0 = side, 1.0 = center)
 */
static void update_menu_item(int index, float t)
{
    if (index < 0 || index >= 6) return;
    
    menu_item_info_t *info = &menu_items_info[index];
    if (info->obj == NULL) return;
    
    /* Get parent container to calculate vertical center */
    lv_obj_t *parent = lv_obj_get_parent(info->obj);
    lv_coord_t cont_height = lv_obj_get_height(parent);
    
    /* Interpolate card size */
    lv_coord_t card_size = CARD_SIZE_SIDE + (lv_coord_t)((CARD_SIZE_CENTER - CARD_SIZE_SIDE) * t);
    lv_obj_set_size(info->obj, card_size, card_size);
    
    /* Position card using base_x (calculated in scroll event with fixed gap) */
    lv_coord_t x_pos = info->base_x;
    
    /* Keep card vertically centered as size changes (上下等距) */
    lv_coord_t y_pos = (cont_height - card_size) / 2;
    
    lv_obj_set_pos(info->obj, x_pos, y_pos);
    
    /* Interpolate icon zoom based on target sizes */
    if (info->icon != NULL && info->orig_icon_width > 0) {
        /* Calculate target icon size */
        lv_coord_t target_size = info->icon_size_side + 
                                (lv_coord_t)((info->icon_size_center - info->icon_size_side) * t);
        
        /* Calculate zoom factor: target_size / original_size * 256 */
        uint16_t zoom = (uint16_t)((target_size * 256) / info->orig_icon_width);
        
        /* Apply zoom to icon */
        lv_img_set_zoom(info->icon, zoom);
        
        /* Center icon in card - use center function for better accuracy */
        lv_obj_center(info->icon);
    }
    
    /* Show/hide label based on proximity to center */
    if (info->label != NULL) {
        if (t > 0.7f) {  /* Show label only when very close to center */
            lv_obj_clear_flag(info->label, LV_OBJ_FLAG_HIDDEN);
            /* Position label: center of text is 80px below card center */
            lv_obj_align(info->label, LV_ALIGN_CENTER, 0, 65);
        } else {  /* Hide label when at sides */
            lv_obj_add_flag(info->label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * Timer callback to initialize carousel effect after UI is fully loaded
 */
static void carousel_init_timer_cb(lv_timer_t * timer)
{
    lv_ui *ui = (lv_ui *)timer->user_data;

    /* Safety check: ensure scrHome is still the active screen and objects are valid */
    if (ui->scrHome_contMain != NULL && lv_obj_is_valid(ui->scrHome_contMain) &&
        ui->scrHome_contPrint != NULL && lv_obj_is_valid(ui->scrHome_contPrint) &&
        lv_scr_act() == ui->scrHome) {
        /* Scroll to contPrint to center it */
        lv_obj_scroll_to_view(ui->scrHome_contPrint, LV_ANIM_ON);

        /* Trigger scroll event to update scales */
        lv_event_send(ui->scrHome_contMain, LV_EVENT_SCROLL, NULL);
    }

    /* Delete the one-shot timer */
    if (carousel_init_timer != NULL) {
        lv_timer_del(carousel_init_timer);
        carousel_init_timer = NULL;
    }
}

/**
 * Scroll event callback for carousel effect
 */
void contMain_scroll_event_cb(lv_event_t * e)
{
    lv_obj_t * cont = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code != LV_EVENT_SCROLL) return;
    
    lv_area_t cont_area, item_area;
    int32_t cont_center_x, item_center_x, diff_x;
    
    /* Get container center */
    lv_obj_get_coords(cont, &cont_area);
    cont_center_x = cont_area.x1 + lv_area_get_width(&cont_area) / 2;
    
    /* First pass: calculate t values and card sizes */
    float t_values[6];
    lv_coord_t card_sizes[6];
    
    for (int i = 0; i < 6; i++) {
        if (menu_items_info[i].obj == NULL) continue;
        
        lv_obj_get_coords(menu_items_info[i].obj, &item_area);
        
        /* Calculate distance from center */
        item_center_x = item_area.x1 + lv_area_get_width(&item_area) / 2;
        diff_x = item_center_x - cont_center_x;
        diff_x = LV_ABS(diff_x);
        
        /* Calculate interpolation factor t (0.0 = side, 1.0 = center) */
        if (diff_x >= CAROUSEL_SPACING) {
            t_values[i] = 0.0f;
        } else {
            t_values[i] = 1.0f - ((float)diff_x / (float)CAROUSEL_SPACING);
        }
        
        /* Calculate card size based on t */
        card_sizes[i] = CARD_SIZE_SIDE + (lv_coord_t)((CARD_SIZE_CENTER - CARD_SIZE_SIDE) * t_values[i]);
    }
    
    /* Second pass: position cards with fixed gap */
    lv_coord_t x_pos = 0;
    for (int i = 0; i < 6; i++) {
        if (menu_items_info[i].obj == NULL) continue;
        
        /* Save X position for this card */
        menu_items_info[i].base_x = x_pos;
        
        /* Update card with new size and position */
        update_menu_item(i, t_values[i]);
        
        /* Calculate next X position: current_x + current_card_size + gap */
        x_pos += card_sizes[i] + CARD_GAP;
    }
}

/**
 * Setup carousel effect for contMain
 * Initializes scroll position to center on contPrint (3rd item)
 */
void setup_contMain_carousel_effect(lv_ui *ui)
{
    if (ui->scrHome_contMain == NULL) return;
    
    /* Enable smooth scrolling with snap */
    lv_obj_set_scroll_snap_x(ui->scrHome_contMain, LV_SCROLL_SNAP_CENTER);
    lv_obj_update_snap(ui->scrHome_contMain, LV_ANIM_ON);
    
    /* Initialize menu items array in display order */
    lv_obj_t * menu_items[] = {
        ui->scrHome_contCopy,   /* 1st card */
        ui->scrHome_contScan,   /* 2nd card */
        ui->scrHome_contPrint,  /* 3rd card */
        ui->scrHome_cont_5,     /* 4th card */
        ui->scrHome_cont_1,     /* 5th card */
        ui->scrHome_cont_2      /* 6th card */
    };
    
    /* Icon sizes for each menu item: [side_size, center_size] */
    const lv_coord_t icon_sizes[][2] = {
        {54, 75},  /* contCopy - imgIconCopy */
        {60, 90},  /* contScan - imgIconScan */
        {50, 75},  /* contPrint - imgIconPrint */
        {55, 75},  /* cont_5 - img_4 */
        {55, 75},  /* cont_1 - img_1 */
        {55, 75}   /* cont_2 - img_2 */
    };
    
    /* Initialize cards with fixed gap between them */
    lv_coord_t x_pos = 0;
    
    for (uint32_t i = 0; i < 6; i++) {
        if (menu_items[i] == NULL) continue;
        
        /* Store menu item reference */
        menu_items_info[i].obj = menu_items[i];
        
        /* Get icon and label children */
        menu_items_info[i].icon = lv_obj_get_child(menu_items[i], 0);
        menu_items_info[i].label = lv_obj_get_child(menu_items[i], 1);
        
        /* Save original icon size */
        if (menu_items_info[i].icon != NULL) {
            menu_items_info[i].orig_icon_width = lv_obj_get_width(menu_items_info[i].icon);
            menu_items_info[i].orig_icon_height = lv_obj_get_height(menu_items_info[i].icon);
        }
        
        /* Set specific icon sizes for this menu item */
        menu_items_info[i].icon_size_side = icon_sizes[i][0];
        menu_items_info[i].icon_size_center = icon_sizes[i][1];
        
        /* Initialize label as hidden */
        if (menu_items_info[i].label != NULL) {
            lv_obj_add_flag(menu_items_info[i].label, LV_OBJ_FLAG_HIDDEN);
        }
        
        /* Save base X position with fixed gap */
        menu_items_info[i].base_x = x_pos;
        
        /* Set initial card size to side size */
        lv_obj_set_size(menu_items[i], CARD_SIZE_SIDE, CARD_SIZE_SIDE);
        
        /* Position card: horizontally with fixed gap, vertically centered */
        lv_coord_t cont_height = lv_obj_get_height(ui->scrHome_contMain);
        lv_coord_t y_pos = (cont_height - CARD_SIZE_SIDE) / 2;  /* 上下等距居中 */
        lv_obj_set_pos(menu_items[i], x_pos, y_pos);
        
        /* Initialize item at side state (t=0.0) */
        update_menu_item(i, 0.0f);
        
        /* Ensure icon is centered after all sizing (important for small cards) */
        if (menu_items_info[i].icon != NULL) {
            lv_obj_center(menu_items_info[i].icon);
        }
        
        /* Calculate next X position with fixed gap */
        x_pos += CARD_SIZE_SIDE + CARD_GAP;
    }
    
    /* Add scroll event for dynamic scaling */
    lv_obj_add_event_cb(ui->scrHome_contMain, contMain_scroll_event_cb, 
                        LV_EVENT_SCROLL, NULL);
    
    /* Use a one-shot timer to initialize carousel after UI is fully loaded */
    if (carousel_init_timer == NULL) {
        carousel_init_timer = lv_timer_create(carousel_init_timer_cb, 100, ui);
        lv_timer_set_repeat_count(carousel_init_timer, 1);
    }
}

/**
 * Update WiFi connection status display
 * @param ui Pointer to the UI structure
 * @param is_connected true if WiFi is connected, false if loading/disconnected
 * 
 * Usage example for ESP32:
 * // In your ESP32 WiFi event handler:
 * extern lv_ui guider_ui;
 * 
 * void wifi_event_handler(void* arg, esp_event_base_t event_base,
 *                         int32_t event_id, void* event_data)
 * {
 *     if (event_id == WIFI_EVENT_STA_CONNECTED) {
 *         update_wifi_status(&guider_ui, true);
 *     }
 *     else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
 *         update_wifi_status(&guider_ui, false);
 *     }
 * }
 */
/**
 * Rotate animation callback for loading icon
 */
static void wifi_loading_anim_cb(void * var, int32_t v)
{
    lv_img_set_angle(var, v);
}

void update_wifi_status(lv_ui *ui, bool is_connected)
{
    if (ui == NULL || ui->scrHome_imgWifiStatus == NULL || ui->scrHome_labelWifiStatus == NULL) {
        ESP_LOGW("WIFI_STATUS", "UI objects not ready, skipping update");
        return;
    }

    /* 检查对象是否有效 */
    if (!lv_obj_is_valid(ui->scrHome_imgWifiStatus) || !lv_obj_is_valid(ui->scrHome_labelWifiStatus)) {
        ESP_LOGW("WIFI_STATUS", "UI objects invalid, skipping update");
        return;
    }

    /* Save WiFi status for screen reload */
    wifi_connected_status = is_connected;
    cached_ui = ui;

    ESP_LOGI("WIFI_STATUS", "Updating WiFi status: %s", is_connected ? "Connected" : "Disconnected");

    if (is_connected) {
        /* 再次检查对象有效性（防止在保存状态后对象被删除） */
        if (!lv_obj_is_valid(ui->scrHome_imgWifiStatus) || !lv_obj_is_valid(ui->scrHome_labelWifiStatus)) {
            ESP_LOGW("WIFI_STATUS", "Objects became invalid, aborting update");
            return;
        }

        /* Stop any loading animation */
        lv_anim_del(ui->scrHome_imgWifiStatus, NULL);
        lv_img_set_angle(ui->scrHome_imgWifiStatus, 0);

        /* WiFi Connected - 使用绿色internet图标 */
        ESP_LOGI("WIFI_STATUS", "Setting connected icon...");
        lv_img_set_src(ui->scrHome_imgWifiStatus, &_internet_alpha_66x66);
        ESP_LOGI("WIFI_STATUS", "Setting connected label...");
        lv_label_set_text(ui->scrHome_labelWifiStatus, "CONNECTED");

        /* 设置图标为绿色 */
        lv_obj_set_style_img_recolor(ui->scrHome_imgWifiStatus, lv_color_hex(0x00ff88), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_img_recolor_opa(ui->scrHome_imgWifiStatus, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

        /* Green color with enhanced visual effects */
        lv_obj_set_style_text_color(ui->scrHome_labelWifiStatus,
                                     lv_color_hex(0x00ff88), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(ui->scrHome_labelWifiStatus, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(ui->scrHome_labelWifiStatus,
                                       lv_color_hex(0x00ff00), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_spread(ui->scrHome_labelWifiStatus, 3, LV_PART_MAIN|LV_STATE_DEFAULT);

        /* Adjust icon position and size for 40x40 display */
        lv_obj_set_pos(ui->scrHome_imgWifiStatus, 25, 6);
        lv_obj_set_size(ui->scrHome_imgWifiStatus, 40, 40);

        /* Update data timestamp when WiFi connects */
        update_data_timestamp();

    } else {
        /* 再次检查对象有效性（防止在保存状态后对象被删除） */
        if (!lv_obj_is_valid(ui->scrHome_imgWifiStatus) || !lv_obj_is_valid(ui->scrHome_labelWifiStatus)) {
            ESP_LOGW("WIFI_STATUS", "Objects became invalid, aborting update");
            return;
        }

        /* WiFi Loading/Disconnected - 使用圆圈气泡图标并旋转 */
        lv_img_set_src(ui->scrHome_imgWifiStatus, &_load_alpha_40x40);
        lv_label_set_text(ui->scrHome_labelWifiStatus, "LOADING...");

        /* 设置图标为红色 */
        lv_obj_set_style_img_recolor(ui->scrHome_imgWifiStatus, lv_color_hex(0xff4444), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_img_recolor_opa(ui->scrHome_imgWifiStatus, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

        /* Red color with pulsing glow effect */
        lv_obj_set_style_text_color(ui->scrHome_labelWifiStatus,
                                     lv_color_hex(0xff4444), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(ui->scrHome_labelWifiStatus, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(ui->scrHome_labelWifiStatus,
                                       lv_color_hex(0xff0000), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_spread(ui->scrHome_labelWifiStatus, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

        /* Adjust icon position and size for 40x40 display */
        lv_obj_set_pos(ui->scrHome_imgWifiStatus, 25, 6);
        lv_obj_set_size(ui->scrHome_imgWifiStatus, 40, 40);

        /* Add rotation animation to loading icon */
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, ui->scrHome_imgWifiStatus);
        lv_anim_set_exec_cb(&anim, wifi_loading_anim_cb);
        lv_anim_set_values(&anim, 0, 3600);  /* 0 to 360 degrees (x10) */
        lv_anim_set_time(&anim, 2000);       /* 2 seconds per rotation */
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&anim);
    }
}

/* SNTP removed - using OneNet cloud time instead */

/**
 * @brief 延迟创建天气显示的定时器回调
 *
 * 这个回调在屏幕完全加载后延迟执行，避免在屏幕切换动画期间创建对象
 */
static void delayed_weather_display_create_cb(lv_timer_t * timer)
{
    lv_ui *ui = (lv_ui *)timer->user_data;

    if (ui == NULL) {
        ESP_LOGW(WEATHER_TAG, "UI is NULL in delayed callback");
        lv_timer_del(timer);
        return;
    }

    /* 检查scrHome是否仍然是当前屏幕 */
    if (lv_scr_act() != ui->scrHome) {
        ESP_LOGW(WEATHER_TAG, "Screen changed before delayed creation, aborting");
        lv_timer_del(timer);
        return;
    }

    /* 检查scrHome_contBG是否有效 */
    if (ui->scrHome_contBG == NULL || !lv_obj_is_valid(ui->scrHome_contBG)) {
        ESP_LOGW(WEATHER_TAG, "scrHome_contBG is invalid in delayed callback");
        lv_timer_del(timer);
        return;
    }

    ESP_LOGI(WEATHER_TAG, "Delayed weather display creation starting...");

    /* Recreate runtime display (已禁用) */
    create_runtime_display(ui);

    /* Recreate weather display */
    create_weather_display(ui);

    /* 如果已有天气数据，立即更新显示 */
    if (g_last_weather.valid) {
        weather_callback(&g_last_weather);
    }

    ESP_LOGI(WEATHER_TAG, "Delayed weather display creation completed");

    /* 删除一次性定时器 */
    lv_timer_del(timer);
}

/**
 * Restore WiFi status and runtime display when screen is reloaded
 * Call this in screen load event
 */
void restore_wifi_status_on_screen_load(lv_ui *ui)
{
    if (ui == NULL || ui->scrHome_imgWifiStatus == NULL) {
        ESP_LOGW(WEATHER_TAG, "UI or WiFi status image is NULL");
        return;
    }

    ESP_LOGI(WEATHER_TAG, "Restoring WiFi status on screen load...");

    /* Restore WiFi status with saved state */
    update_wifi_status(ui, wifi_connected_status);

    /* 使用延迟定时器创建天气显示，避免在屏幕切换动画期间创建对象 */
    /* 延迟300ms，等待屏幕切换动画完成 */
    lv_timer_t *delayed_timer = lv_timer_create(delayed_weather_display_create_cb, 300, ui);
    lv_timer_set_repeat_count(delayed_timer, 1);  /* 只执行一次 */

    ESP_LOGI(WEATHER_TAG, "Scheduled delayed weather display creation (300ms)");

    /* Time synchronization is handled by OneNet cloud platform */
}

/**********************
 *  SIGNAL GENERATOR
 **********************/
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Signal generator state */
static int current_wave_type = 0;  // 0=sine, 1=triangle, 2=square
static uint32_t current_frequency = 1000;  // in Hz (1kHz default)
static uint16_t current_amplitude = 165;   // in units (0.01V) - 1.65V default
static lv_chart_series_t * wave_series = NULL;

/**
 * Update waveform display based on parameters
 * @param chart The chart object to update
 * @param wave_type 0=sine, 1=triangle, 2=square
 * @param frequency Frequency in Hz (0-2000000)
 * @param amplitude Amplitude in units representing 0.01V (0-300 = 0-3V)
 */
void update_waveform_display(lv_obj_t * chart, int wave_type, uint32_t frequency, uint16_t amplitude)
{
    if (chart == NULL || wave_series == NULL) {
        ESP_LOGW("WAVEFORM", "Chart or series is NULL, skipping update");
        return;
    }

    /* Validate chart object */
    if (!lv_obj_is_valid(chart)) {
        ESP_LOGW("WAVEFORM", "Chart object is invalid, resetting series");
        wave_series = NULL;
        return;
    }

    /* Update state */
    current_wave_type = wave_type;
    current_frequency = frequency;
    current_amplitude = amplitude;

    /* Center value for 0-3V range (1.5V = 150 units) */
    const int32_t center = 150;
    const int32_t max_range = 300;

    /* amplitude is 0-300 (0-3V), but it represents peak-to-peak
     * For waveform display, we need half of it (peak amplitude)
     * and scale it to fit in the chart (max peak = 150 to stay in 0-300 range) */
    int32_t peak_amplitude = amplitude / 2;

    /* Calculate number of cycles to display based on frequency for smooth visual effect */
    float cycles;
    if (frequency == 0) {
        cycles = 0.5f;  // DC signal, show flat line
    } else if (frequency <= 100) {
        /* 0-100 Hz: 0.5 to 1 cycle */
        cycles = 0.5f + (frequency / 100.0f) * 0.5f;
    } else if (frequency <= 1000) {
        /* 100-1000 Hz: 1 to 3 cycles */
        cycles = 1.0f + ((frequency - 100.0f) / 900.0f) * 2.0f;
    } else if (frequency <= 10000) {
        /* 1k-10k Hz: 3 to 6 cycles - reduced to keep more points per cycle */
        cycles = 3.0f + ((frequency - 1000.0f) / 9000.0f) * 3.0f;
    } else if (frequency <= 100000) {
        /* 10k-100k Hz: 6 to 10 cycles */
        cycles = 6.0f + ((frequency - 10000.0f) / 90000.0f) * 4.0f;
    } else if (frequency <= 500000) {
        /* 100k-500k Hz: 10 to 14 cycles */
        cycles = 10.0f + ((frequency - 100000.0f) / 400000.0f) * 4.0f;
    } else {
        /* 500k-2M Hz: 14 to 18 cycles */
        float factor = (frequency - 500000.0f) / 1500000.0f;
        if (factor > 1.0f) factor = 1.0f;
        cycles = 14.0f + factor * 4.0f;
    }

    /* Calculate optimal point count for smooth display
     * For sine and triangle waves, we need many points per cycle for smoothness
     * Square wave doesn't need as many points */
    uint16_t min_points_per_cycle = (wave_type == 2) ? 16 : 80;  /* Square wave needs fewer points */
    uint16_t max_total_points = 2000;  /* Increased max for smoother display */

    /* Calculate point count ensuring minimum points per cycle */
    uint16_t point_count = (uint16_t)(cycles * min_points_per_cycle);

    /* For smooth curves (sine/triangle), ensure adequate total points */
    if (wave_type != 2) {
        /* Ensure at least 400 points for smooth display */
        if (point_count < 400) point_count = 400;
    } else {
        /* Square wave needs fewer points */
        if (point_count < 100) point_count = 100;
    }

    /* Cap at maximum for performance */
    if (point_count > max_total_points) point_count = max_total_points;

    /* Update chart point count dynamically */
    lv_chart_set_point_count(chart, point_count);

    for (uint16_t i = 0; i < point_count; i++) {
        float phase = (float)i / (float)point_count * cycles * 2.0f * M_PI;
        int32_t value = 0;

        if (frequency == 0) {
            /* DC signal - flat line at center */
            value = center;
        } else {
            switch (wave_type) {
                case 0:  // Sine wave
                    value = (int32_t)(sinf(phase) * peak_amplitude + center);
                    break;

                case 1:  // Triangle wave
                    {
                        float cycle_phase = fmodf(phase, 2.0f * M_PI) / (2.0f * M_PI);

                        if (cycle_phase < 0.25f) {
                            value = (int32_t)(center + 4.0f * cycle_phase * peak_amplitude);
                        } else if (cycle_phase < 0.75f) {
                            value = (int32_t)(center + peak_amplitude - 4.0f * (cycle_phase - 0.25f) * peak_amplitude);
                        } else {
                            value = (int32_t)(center - peak_amplitude + 4.0f * (cycle_phase - 0.75f) * peak_amplitude);
                        }
                    }
                    break;

                case 2:  // Square wave
                    {
                        float cycle_phase = fmodf(phase, 2.0f * M_PI);
                        value = (cycle_phase < M_PI) ? (center + peak_amplitude) : (center - peak_amplitude);
                    }
                    break;
            }
        }

        /* Clamp value to valid range (0-300 = 0-3V) */
        if (value < 0) value = 0;
        if (value > max_range) value = max_range;

        /* Update chart point */
        lv_chart_set_next_value(chart, wave_series, value);
    }

    /* Refresh chart display */
    lv_chart_refresh(chart);
}

/**
 * Initialize signal generator with default waveform
 * @param ui Pointer to the UI structure
 */
void init_signal_generator(lv_ui *ui)
{
    if (ui == NULL || ui->scrCopy_imgScanned == NULL) {
        return;
    }

    /* Always recreate series to avoid stale pointer issues */
    wave_series = NULL;
    wave_series = lv_chart_add_series(ui->scrCopy_imgScanned,
                                     lv_color_hex(0x2196F3),
                                     LV_CHART_AXIS_PRIMARY_Y);

    /* Initialize with default waveform (sine, 10kHz, 1.5V) */
    /* Default slider value 100 = 10kHz, amplitude 150 = 1.5V */
    update_waveform_display(ui->scrCopy_imgScanned, 0, 10000, 150);
}

/**
 * Deinitialize signal generator and clean up resources
 * Called when leaving scrCopy screen
 */
void deinit_signal_generator(void)
{
    /* Clear the series pointer to prevent use-after-free */
    wave_series = NULL;

    /* Reset state variables */
    current_wave_type = 0;
    current_frequency = 10000;
    current_amplitude = 150;
}

/*********************
 * DIGITAL MULTIMETER FUNCTIONS
 *********************/

/* DMM timer and state variables */
static lv_timer_t * dmm_update_timer = NULL;
static lv_ui * dmm_ui = NULL;

/* DMM measurement simulation structure */
typedef struct {
    float voltage_base;
    float current_base;
    float resistance_base;
    uint8_t update_counter;
} dmm_state_t;

static dmm_state_t dmm_state = {
    .voltage_base = 3.14f,
    .current_base = 0.235f,
    .resistance_base = 4700.0f,
    .update_counter = 0
};

/**
 * Timer callback to update DMM measurements with realistic fluctuations
 * @param timer Pointer to the timer
 */
static void dmm_measurement_update_cb(lv_timer_t * timer)
{
    if (dmm_ui == NULL || !lv_obj_is_valid(dmm_ui->scrPrintMenu)) {
        return;
    }
    
    dmm_state.update_counter++;
    
    /* Check which mode button is active */
    bool voltage_active = lv_obj_has_state(dmm_ui->scrPrintMenu_contUSB, LV_STATE_CHECKED);
    bool current_active = lv_obj_has_state(dmm_ui->scrPrintMenu_contMobile, LV_STATE_CHECKED);
    bool resistance_active = lv_obj_has_state(dmm_ui->scrPrintMenu_contInternet, LV_STATE_CHECKED);
    
    char buf[32];
    int32_t arc_val = 0;
    
    if (voltage_active) {
        /* Simulate voltage measurement with small fluctuations */
        float voltage = dmm_state.voltage_base + (lv_rand(0, 100) - 50) / 500.0f;
        if (voltage < 0) voltage = 0;
        if (voltage > 5.0f) voltage = 5.0f;
        
        snprintf(buf, sizeof(buf), "%.3f V", voltage);
        lv_label_set_text(dmm_ui->scrPrintMenu_labelUSB, buf);
        
        arc_val = (int32_t)((voltage / 5.0f) * 100);
        lv_arc_set_value(dmm_ui->scrPrintMenu_imgUSB, arc_val);
        
        /* Update percentage label */
        snprintf(buf, sizeof(buf), "%d%%", arc_val);
        lv_label_set_text(dmm_ui->scrPrintMenu_labelnternet, buf);
        
        /* 固定橘黄色，不变色 */
        lv_obj_set_style_arc_color(dmm_ui->scrPrintMenu_imgUSB, 
            lv_color_hex(0xFF9800), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    }
    else if (current_active) {
        /* Simulate current measurement */
        float current = dmm_state.current_base + (lv_rand(0, 100) - 50) / 5000.0f;
        if (current < 0) current = 0;
        if (current > 1.0f) current = 1.0f;
        
        snprintf(buf, sizeof(buf), "%.3f A", current);
        lv_label_set_text(dmm_ui->scrPrintMenu_labelUSB, buf);
        
        arc_val = (int32_t)((current / 1.0f) * 100);
        lv_arc_set_value(dmm_ui->scrPrintMenu_imgUSB, arc_val);
        
        snprintf(buf, sizeof(buf), "%d%%", arc_val);
        lv_label_set_text(dmm_ui->scrPrintMenu_labelnternet, buf);
        
        /* 固定橘黄色，不变色 */
        lv_obj_set_style_arc_color(dmm_ui->scrPrintMenu_imgUSB, 
            lv_color_hex(0xFF9800), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    }
    else if (resistance_active) {
        /* Simulate resistance measurement */
        float resistance = dmm_state.resistance_base + (lv_rand(0, 100) - 50) * 2.0f;
        if (resistance < 0) resistance = 0;
        if (resistance > 10000.0f) resistance = 10000.0f;
        
        if (resistance > 1000) {
            snprintf(buf, sizeof(buf), "%.2f kΩ", resistance / 1000.0f);
        } else {
            snprintf(buf, sizeof(buf), "%.1f Ω", resistance);
        }
        lv_label_set_text(dmm_ui->scrPrintMenu_labelUSB, buf);
        
        arc_val = (int32_t)((resistance / 10000.0f) * 100);
        lv_arc_set_value(dmm_ui->scrPrintMenu_imgUSB, arc_val);
        
        snprintf(buf, sizeof(buf), "%d%%", arc_val);
        lv_label_set_text(dmm_ui->scrPrintMenu_labelnternet, buf);
        
        /* 固定橘黄色，不变色 */
        lv_obj_set_style_arc_color(dmm_ui->scrPrintMenu_imgUSB, 
            lv_color_hex(0xFF9800), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    }
}

/**
 * Initialize digital multimeter with dynamic updates
 * @param ui Pointer to the UI structure
 */
void init_digital_multimeter(lv_ui *ui)
{
    if (ui == NULL) {
        return;
    }
    
    dmm_ui = ui;
    
    /* Start timer for continuous measurement updates (100ms interval) */
    if (dmm_update_timer == NULL) {
        dmm_update_timer = lv_timer_create(dmm_measurement_update_cb, 100, NULL);
    } else {
        lv_timer_reset(dmm_update_timer);
    }
}

/**
 * Stop digital multimeter updates
 */
void stop_digital_multimeter(void)
{
    if (dmm_update_timer != NULL) {
        lv_timer_del(dmm_update_timer);
        dmm_update_timer = NULL;
    }
    dmm_ui = NULL;
}

/*********************
 * AI CHAT FUNCTIONS
 *********************/

/* AI Chat state variables */
static lv_ui * ai_chat_ui = NULL;
static lv_obj_t * loading_label = NULL;
static lv_timer_t * loading_timer = NULL;
static int loading_dots = 0;
static int message_y_pos = 20;  // Track Y position for messages

/**
 * Get greeting message based on current time
 * @return Greeting string
 */
const char* ai_chat_get_greeting_by_time(void)
{
    // 固定开头语：你好, 我是小智!（使用英文标点符号）
    // Fixed greeting: Hello, I'm Xiaozhi! (using English punctuation)
    return "你好, 我是小智!";
}

/**
 * Loading animation timer callback
 * @param timer Pointer to the timer
 */
static void loading_anim_cb(lv_timer_t * timer)
{
    if (loading_label == NULL || !lv_obj_is_valid(loading_label)) {
        if (loading_timer != NULL) {
            lv_timer_del(loading_timer);
            loading_timer = NULL;
        }
        return;
    }
    
    // Get the text label stored in user_data
    lv_obj_t * text_label = (lv_obj_t *)lv_obj_get_user_data(loading_label);
    if (text_label == NULL || !lv_obj_is_valid(text_label)) {
        return;
    }
    
    loading_dots = (loading_dots + 1) % 4;
    
    static char loading_text[32];
    // 中文思考动画
    const char* dots[] = {"", ".", "..", "..."};
    snprintf(loading_text, sizeof(loading_text), "思考中%s", dots[loading_dots]);
    lv_label_set_text(text_label, loading_text);
}

/**
 * Create a chat bubble (message container)
 * @param parent Parent container
 * @param message Message text
 * @param is_ai true for AI message (left), false for user message (right)
 * @param y_pos Y position for the bubble
 * @return Created bubble object
 */
static lv_obj_t * create_chat_bubble(lv_obj_t * parent, const char* message, bool is_ai, lv_coord_t y_pos)
{
    if (parent == NULL || message == NULL) return NULL;

    // 创建头像 - Avatar icon (30x30 圆形)
    lv_obj_t * avatar = lv_img_create(parent);
    if (is_ai) {
        // AI头像：使用云朵图标（缩放显示）
        lv_img_set_src(avatar, &_cloud_alpha_123x90);
        lv_obj_set_size(avatar, 30, 30);
        // AI头像在左侧
        lv_obj_set_pos(avatar, 5, y_pos);
    } else {
        // 用户头像：使用替代图标
        lv_img_set_src(avatar, &_cosbb_alpha_30x30);
        lv_obj_set_size(avatar, 30, 30);
        // 用户头像在右侧
        lv_obj_set_pos(avatar, 405, y_pos);  // 右侧对齐（440卡片宽 - 30头像宽 - 5边距）
    }
    // 头像圆形背景
    lv_obj_set_style_radius(avatar, LV_RADIUS_CIRCLE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(avatar, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(avatar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(avatar, lv_color_hex(0xE0E0E0), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(avatar, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(avatar, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(avatar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    // 创建聊天气泡容器 - 避免重叠（聊天卡片宽度为460px）
    // Create bubble container - proper width to avoid overlap (chat card is 460px wide)
    lv_obj_t * bubble = lv_obj_create(parent);
    // 【大小设置】聊天气泡尺寸 - 动态调整
    // 宽度和高度将根据文本内容动态设置
    lv_obj_set_size(bubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);  // 自动适应内容大小
    lv_obj_set_scrollbar_mode(bubble, LV_SCROLLBAR_MODE_OFF);

    // 根据发送者设置样式 - 明显的颜色区分
    // Style based on sender - clear color distinction
    if (is_ai) {
        // AI消息在左侧，白色背景黑字 - 气泡样式
        // AI message on the left with white background and black text - bubble style
        lv_obj_set_style_bg_opa(bubble, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);  // 白色背景
        lv_obj_set_style_bg_grad_dir(bubble, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);  // 不使用渐变
    } else {
        // 用户消息在右侧，深绿色背景白字 - 微信风格气泡
        // User message on the right with deep green background and white text - WeChat style bubble
        lv_obj_set_style_bg_opa(bubble, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x95EC69), LV_PART_MAIN|LV_STATE_DEFAULT);  // 微信绿色背景
        lv_obj_set_style_bg_grad_dir(bubble, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);  // 不使用渐变
    }
    
    // 气泡样式 - 真实的聊天气泡效果（微信风格）
    // Real chat bubble styling - WeChat/iMessage style
    lv_obj_set_style_border_width(bubble, 0, LV_PART_MAIN|LV_STATE_DEFAULT);  // 无边框，更像真实气泡
    lv_obj_set_style_border_opa(bubble, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    
    // 圆角设计 - 使用outline模拟气泡尾巴效果
    lv_obj_set_style_radius(bubble, 15, LV_PART_MAIN|LV_STATE_DEFAULT);  // 统一圆角
    
    // 使用outline创建气泡尾巴效果
    if (is_ai) {
        // AI气泡：添加左侧指向效果
        lv_obj_set_style_outline_width(bubble, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_outline_color(bubble, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_outline_pad(bubble, -12, LV_PART_MAIN|LV_STATE_DEFAULT);  // 负值让outline向内
        lv_obj_set_style_outline_opa(bubble, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    } else {
        // 用户气泡：添加右侧指向效果
        lv_obj_set_style_outline_width(bubble, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_outline_color(bubble, lv_color_hex(0x95EC69), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_outline_pad(bubble, -12, LV_PART_MAIN|LV_STATE_DEFAULT);  // 负值让outline向内
        lv_obj_set_style_outline_opa(bubble, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    
    // 气泡内边距 - 确保文字上下左右都有足够空间，实现居中效果
    lv_obj_set_style_pad_top(bubble, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(bubble, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(bubble, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(bubble, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    
    // 阴影效果 - 所有气泡都有柔和阴影，增强立体感
    // Shadow effect - all bubbles have soft shadow for depth
    lv_obj_set_style_shadow_width(bubble, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bubble, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bubble, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bubble, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    if (is_ai) {
        lv_obj_set_style_shadow_ofs_x(bubble, -1, LV_PART_MAIN|LV_STATE_DEFAULT);  // 左侧气泡阴影偏左
    } else {
        lv_obj_set_style_shadow_ofs_x(bubble, 1, LV_PART_MAIN|LV_STATE_DEFAULT);   // 右侧气泡阴影偏右
    }
    lv_obj_set_style_shadow_ofs_y(bubble, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

    // 不显示发送者标签（AI助手/我），只显示消息内容
    // No sender label, only show message content

    // 消息标签 - 中文字体，左对齐
    // Message label - Chinese font, left alignment
    lv_obj_t * msg_label = lv_label_create(bubble);
    lv_label_set_text(msg_label, message);
    lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);  // 自动换行

    // 【宽度设置】消息文本宽度 - 根据内容自动调整，设置最大宽度限制
    // 最大宽度：AI消息最大300px，用户消息最大300px
    int max_width = 300;
    lv_obj_set_width(msg_label, LV_SIZE_CONTENT);  // 先设置为自动宽度
    lv_obj_update_layout(msg_label);  // 更新布局获取实际宽度

    // 如果文本宽度超过最大宽度，则限制并允许换行
    lv_coord_t actual_width = lv_obj_get_width(msg_label);
    if (actual_width > max_width) {
        lv_obj_set_width(msg_label, max_width);
    }

    // 根据发送者设置文字颜色和字体 - 确保高对比度可读性
    if (is_ai) {
        lv_obj_set_style_text_color(msg_label, lv_color_hex(0x1A1A1A), LV_PART_MAIN|LV_STATE_DEFAULT);  // 深黑色文字
        // AI消息使用myFont中文字库（从Flash分区加载）
        lv_obj_set_style_text_font(msg_label, &myFont, LV_PART_MAIN|LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_color(msg_label, lv_color_hex(0x1A1A1A), LV_PART_MAIN|LV_STATE_DEFAULT);  // 深黑色文字（绿色背景用黑字更清晰）
        // 用户消息使用myFont中文字库
        lv_obj_set_style_text_font(msg_label, &myFont, LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    lv_obj_set_style_text_opa(msg_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(msg_label, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    // 【文字对齐】水平居中对齐
    lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);  // 文字水平居中

    // 【标签位置】标签在气泡中完全居中（上下左右都居中）
    lv_obj_center(msg_label);  // 使用center函数确保完全居中

    // 强制更新布局以获取实际尺寸
    lv_obj_update_layout(bubble);

    // 【位置设置】根据气泡实际宽度设置位置
    // 聊天卡片宽度为440px，头像宽度为30px
    lv_coord_t bubble_width = lv_obj_get_width(bubble);

    if (is_ai) {
        // AI消息在左侧，头像右侧
        // x=45: 距离聊天卡片左边缘45像素（头像5 + 宽度30 + 间距10）
        lv_obj_set_pos(bubble, 45, y_pos);
    } else {
        // 用户消息在右侧，头像左侧
        // 右对齐：卡片宽度440 - 气泡宽度 - 头像30 - 间距10 - 右边距5
        lv_coord_t x_pos = 440 - bubble_width - 30 - 10 - 5;
        lv_obj_set_pos(bubble, x_pos, y_pos);
    }

    return bubble;
}

/**
 * Initialize AI chat screen with greeting message
 * @param ui Pointer to the UI structure
 * 
 * Usage:
 * Called when AI Chat screen is loaded to show initial greeting
 */
void init_ai_chat(lv_ui *ui)
{
    if (ui == NULL || ui->scrAIChat_contChatArea == NULL) {
        return;
    }
    
    ai_chat_ui = ui;
    message_y_pos = 20;
    
    // Add greeting message based on time
    const char* greeting = ai_chat_get_greeting_by_time();
    ai_chat_add_message(ui, greeting, true);
}

/**
 * Add a message to the chat area
 * @param ui Pointer to the UI structure
 * @param message Message text to add
 * @param is_ai true for AI message, false for user message
 * 
 * ESP32 Integration Example:
 * 
 * 1. Add user voice-to-text result:
 *    void voice_recognition_callback(const char* text) {
 *        ai_chat_add_message(&guider_ui, text, false);  // User message
 *        ai_chat_add_loading_animation(&guider_ui);     // Show thinking
 *        send_to_ai_backend(text);  // Send to AI processing
 *    }
 * 
 * 2. Add AI response:
 *    void ai_response_callback(const char* response) {
 *        ai_chat_remove_loading_animation(&guider_ui);  // Remove thinking
 *        ai_chat_add_message(&guider_ui, response, true);  // AI response
 *    }
 */
void ai_chat_add_message(lv_ui *ui, const char* message, bool is_ai)
{
    if (ui == NULL || ui->scrAIChat_contChatArea == NULL || message == NULL) {
        return;
    }
    
    // 创建聊天气泡（固定高度70px）
    // Create chat bubble with fixed height (70px)
    lv_obj_t * bubble = create_chat_bubble(ui->scrAIChat_contChatArea, message, is_ai, message_y_pos);
    
    if (bubble != NULL) {
        // 更新下一条消息的Y位置（70px高度 + 10px间距）
        // Update Y position for next message (70px height + 10px gap)
        message_y_pos += 80;
        
        // Smooth scroll to bottom to show new message
        lv_obj_scroll_to_y(ui->scrAIChat_contChatArea, message_y_pos, LV_ANIM_ON);
    }
}

/**
 * Add loading animation (thinking indicator)
 * @param ui Pointer to the UI structure
 * 
 * Shows "Thinking..." with animated dots while AI is processing
 */
void ai_chat_add_loading_animation(lv_ui *ui)
{
    if (ui == NULL || ui->scrAIChat_contChatArea == NULL) {
        return;
    }
    
    // Remove existing loading animation if any
    ai_chat_remove_loading_animation(ui);
    
    // Create loading bubble - 白色背景黑字，与AI消息一致
    loading_label = lv_obj_create(ui->scrAIChat_contChatArea);
    lv_obj_set_size(loading_label, 140, 45);  // 精致小尺寸
    lv_obj_set_pos(loading_label, 10, message_y_pos);  // 左对齐，和AI消息一致
    lv_obj_set_scrollbar_mode(loading_label, LV_SCROLLBAR_MODE_OFF);
    
    // Loading bubble style - 白色背景黑字，与AI消息样式完全一致
    lv_obj_set_style_bg_opa(loading_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(loading_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);  // 白色背景
    lv_obj_set_style_border_width(loading_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);  // 无边框
    lv_obj_set_style_border_opa(loading_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(loading_label, 15, LV_PART_MAIN|LV_STATE_DEFAULT);  // 与气泡一致的圆角
    lv_obj_set_style_pad_all(loading_label, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    
    // 添加outline效果 - 与AI气泡一致
    lv_obj_set_style_outline_width(loading_label, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(loading_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(loading_label, -12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(loading_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    
    // 柔和阴影效果 - 与气泡一致
    lv_obj_set_style_shadow_width(loading_label, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(loading_label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(loading_label, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(loading_label, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(loading_label, -1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(loading_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    
    // Create text label - 中文思考提示（使用英文标点符号）
    lv_obj_t * text = lv_label_create(loading_label);
    lv_label_set_text(text, "思考中...");  // 使用英文省略号
    lv_obj_set_style_text_color(text, lv_color_hex(0x888888), LV_PART_MAIN|LV_STATE_DEFAULT);  // 灰色文字
    lv_obj_set_style_text_font(text, &myFont, LV_PART_MAIN|LV_STATE_DEFAULT);  // 使用myFont中文字库
    lv_obj_center(text);
    
    // Store text label for animation
    lv_obj_set_user_data(loading_label, text);
    
    // Create animation timer
    loading_dots = 0;
    if (loading_timer == NULL) {
        loading_timer = lv_timer_create(loading_anim_cb, 500, NULL);
    }
    
    // Scroll to show loading
    lv_obj_scroll_to_y(ui->scrAIChat_contChatArea, message_y_pos, LV_ANIM_ON);
}

/**
 * Remove loading animation
 * @param ui Pointer to the UI structure
 */
void ai_chat_remove_loading_animation(lv_ui *ui)
{
    if (loading_label != NULL && lv_obj_is_valid(loading_label)) {
        lv_obj_del(loading_label);
        loading_label = NULL;
    }
    
    if (loading_timer != NULL) {
        lv_timer_del(loading_timer);
        loading_timer = NULL;
    }
}

/**
 * Start voice input (called when button is pressed)
 * @param ui Pointer to the UI structure
 * 
 * ESP32 should implement actual voice recording when this is called
 */
void ai_chat_start_voice_input(lv_ui *ui)
{
    if (ui == NULL || ui->scrAIChat_labelStatus == NULL) {
        return;
    }
    
    // Update status label - 隐藏状态文字
    lv_label_set_text(ui->scrAIChat_labelStatus, "");
    
    // Change button text
    if (ui->scrAIChat_btnVoiceInput_label != NULL) {
        lv_label_set_text(ui->scrAIChat_btnVoiceInput_label, "MIC");
    }
    
    /* ESP32 TODO: Start voice recording here
     * Example:
     * i2s_start_recording();
     * voice_buffer_reset();
     */
}

/**
 * Stop voice input (called when button is released)
 * @param ui Pointer to the UI structure
 * 
 * ESP32 should stop recording and process the voice data
 */
void ai_chat_stop_voice_input(lv_ui *ui)
{
    if (ui == NULL || ui->scrAIChat_labelStatus == NULL) {
        return;
    }
    
    // Update status label - 隐藏状态文字
    lv_label_set_text(ui->scrAIChat_labelStatus, "");
    
    // Restore button text
    if (ui->scrAIChat_btnVoiceInput_label != NULL) {
        lv_label_set_text(ui->scrAIChat_btnVoiceInput_label, "MIC");
    }
    
    /* ESP32 TODO: Stop recording and process voice
     * Example:
     * i2s_stop_recording();
     * voice_data = get_recorded_data();
     * send_to_speech_recognition(voice_data);
     * 
     * After speech recognition:
     * ai_chat_add_message(&guider_ui, recognized_text, false);
     * ai_chat_add_loading_animation(&guider_ui);
     * send_to_ai_backend(recognized_text);
     */
    
    /* DEMO 演示：模拟语音输入进行测试
     * Demo: Simulate voice input for testing
     * 在真实实现中，这里会等待语音识别结果
     * In real implementation, wait for voice recognition result
     * 
     * 仅在需要测试时取消注释下面的代码
     * Uncomment below code only for testing
     */
    
    /* 
    // 模拟添加用户消息（实际中来自语音识别）
    ai_chat_add_message(ui, "Hello, how are you?", false);
    
    // 显示AI正在思考
    ai_chat_add_loading_animation(ui);
    */
    
    // 真实场景：等待语音识别结果后再调用上述函数
    // Real scenario: Wait for voice recognition result, then call above functions
}

/**
 * @brief 更新位置显示（公共函数）
 */
void onenet_ui_update_location_display(void)
{
    if (g_location_label != NULL && lv_obj_is_valid(g_location_label)) {
        if (g_last_location.valid) {
            char loc_str[128];
            snprintf(loc_str, sizeof(loc_str), "Loc: %.6f, %.6f (%.0fm)",
                     g_last_location.longitude,
                     g_last_location.latitude,
                     g_last_location.radius);
            lv_label_set_text(g_location_label, loc_str);
        } else {
            /* 未连接WiFi或没有位置信息时不显示任何文字 */
            lv_label_set_text(g_location_label, "");
        }
    }
}

/**
 * @brief 手动触发定位请求（公共函数）
 */
void onenet_ui_trigger_location_request(void)
{
    if (!g_onenet_enabled) {
        ESP_LOGW(ONENET_TAG, "OneNet not enabled");
        return;
    }

    if (!wifi_connected_status) {
        ESP_LOGW(ONENET_TAG, "WiFi not connected");
        return;
    }

    if (!cloud_manager_is_activated()) {
        ESP_LOGW(ONENET_TAG, "Device not activated");
        return;
    }

    ESP_LOGI(ONENET_TAG, "Triggering WiFi location request (HTTP)...");
    /* 使用异步任务方式触发定位，避免阻塞UI */
    wifi_location_trigger_async();
}
