/**
 * @file wifi_onenet.h
 * @brief ESP32 WiFi连接和OneNET云平台HTTP通信模块头文件
 * @version 2.0
 * @date 2025-01-08
 *
 * @details
 * 本文件定义了ESP32连接WiFi网络并与中国移动OneNET云平台进行HTTP通信的所有接口。
 * 
 * 版本2.0变更：
 * - 移除MQTT协议支持，改用HTTP协议
 * - 使用设备激活后获取的device_id和sec_key进行认证
 * - 在线状态基于HTTP请求时间判断（10秒内有请求则在线）
 *
 * 主要功能模块：
 * 1. WiFi网络连接管理 - 自动连接、重连、状态监控
 * 2. OneNET HTTP通信 - 属性上报、数据上传
 * 3. WiFi定位服务 - 收集热点信息、上报定位数据、解析位置
 * 4. 时间同步服务 - SNTP网络时间同步
 *
 * @author ESP32开发团队
 */

#ifndef WIFI_ONENET_H
#define WIFI_ONENET_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_random.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== OneNET HTTP API 配置参数 ==================== */

/**
 * @brief OneNET HTTP产品ID (用于创建新设备)
 * @note 从OneNET控制台获取，Ex_Debug产品
 */
#define ONENET_HTTP_PRODUCT_ID  "FCwDzD6VU0"

/**
 * @brief OneNET HTTP产品Access Key
 * @note 从OneNET控制台产品详情页获取，用于HTTP API授权
 */
#define ONENET_ACCESS_KEY       "74ttu7Ofi9n3Z7dfT6dfkctJR8E3xyVxFXdX71/Hs4k="

/**
 * @brief OneNET HTTP 接入服务器地址
 * @note 使用 HTTP 协议接入 OneNET 物联网平台
 */
#define ONENET_HTTP_API_HOST    "open.iot.10086.cn"

/**
 * @brief OneNET HTTP API 服务器端口 (HTTPS)
 */
#define ONENET_HTTP_API_PORT    443

/**
 * @brief OneNET HTTP 接入基础路径
 */
#define ONENET_HTTP_BASE_PATH   "/fuse/http"

/**
 * @brief OneNET 设备属性上报 API 路径 (设备 -> 云端)
 * @note 完整路径: /fuse/http/device/thing/property/post
 */
#define ONENET_HTTP_PROPERTY_UPLOAD "/fuse/http/device/thing/property/post"

/**
 * @brief OneNET 设备事件上报 API 路径
 */
#define ONENET_HTTP_EVENT_POST      "/fuse/http/device/thing/event/post"

/**
 * @brief OneNET 批量数据上报 API 路径
 */
#define ONENET_HTTP_PACK_POST       "/fuse/http/device/thing/pack/post"

/**
 * @brief OneNET 历史数据上报 API 路径
 */
#define ONENET_HTTP_HISTORY_POST    "/fuse/http/device/thing/history/post"

/**
 * @brief OneNET 设备上下线 API 路径
 * @note POST /fuse/http/device/online 设置设备在线/离线状态
 */
#define ONENET_HTTP_DEVICE_ONLINE   "/fuse/http/device/online"

/* ==================== WiFi配置参数 ==================== */

/**
 * @brief WiFi最大重连次数
 * @note 达到最大次数后停止重连，需要手动重试
 */
#define WIFI_MAXIMUM_RETRY      10

/* ==================== WiFi定位配置参数 ==================== */

/**
 * @brief WiFi扫描最大热点数量
 */
#define WIFI_SCAN_MAX_AP        20

/**
 * @brief WiFi扫描超时时间（毫秒）
 */
#define WIFI_SCAN_TIMEOUT_MS    5000

/* ==================== 数据结构定义 ==================== */

/**
 * @brief WiFi连接状态枚举
 */
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,    /**< 未连接 */
    WIFI_STATE_CONNECTING,          /**< 正在连接 */
    WIFI_STATE_CONNECTED,           /**< 已连接 */
    WIFI_STATE_FAILED              /**< 连接失败 */
} wifi_state_t;

/**
 * @brief OneNET连接状态枚举
 * @note 基于HTTP请求时间判断在线状态
 */
typedef enum {
    ONENET_STATE_UNKNOWN = 0,       /**< 未知状态 */
    ONENET_STATE_INIT,              /**< 已初始化 */
    ONENET_STATE_ONLINE,            /**< 在线（10秒内有HTTP请求） */
    ONENET_STATE_OFFLINE,           /**< 离线 */
    ONENET_STATE_ERROR              /**< 错误状态 */
} onenet_state_t;

/* 为了兼容旧代码，保留mqtt_client_state_t别名 */
typedef onenet_state_t mqtt_client_state_t;
#define MQTT_STATE_UNKNOWN      ONENET_STATE_UNKNOWN
#define MQTT_STATE_INIT         ONENET_STATE_INIT
#define MQTT_STATE_CONNECTED    ONENET_STATE_ONLINE
#define MQTT_STATE_DISCONNECTED ONENET_STATE_OFFLINE
#define MQTT_STATE_ERROR        ONENET_STATE_ERROR

/**
 * @brief WiFi热点信息结构体
 */
typedef struct {
    char ssid[33];                  /**< SSID名称 */
    char bssid[18];                 /**< BSSID (MAC地址) */
    int8_t rssi;                    /**< 信号强度 (dBm) */
    uint8_t channel;                /**< 信道号 */
    wifi_auth_mode_t authmode;      /**< 认证模式 */
} wifi_ap_info_t;

/**
 * @brief WiFi定位数据结构体（OneNET格式）
 */
typedef struct {
    char imsi[16];                  /**< 设备IMSI（使用芯片ID生成） */
    char serverip[16];              /**< 网关IP地址 */
    char macs[1024];                /**< 扫描到的热点列表 */
    char mmac[64];                  /**< 当前连接的热点 */
    char smac[18];                  /**< 设备MAC地址 */
    char idfa[48];                  /**< 设备唯一标识 */
} wifi_location_data_t;

/**
 * @brief 位置信息结构体
 */
typedef struct {
    float longitude;                /**< 经度 */
    float latitude;                 /**< 纬度 */
    float radius;                   /**< 定位精度半径（米） */
    char address[256];              /**< 地址描述 */
    bool valid;                     /**< 数据是否有效 */
} location_info_t;

/* ==================== 回调函数类型定义 ==================== */

/**
 * @brief WiFi状态变化回调函数类型
 */
typedef void (*wifi_status_callback_t)(bool connected);

/**
 * @brief WiFi定位结果回调函数类型
 */
typedef void (*location_callback_t)(const location_info_t *location);

/* ==================== WiFi功能函数 ==================== */

/**
 * @brief WiFi网络初始化
 * @param callback WiFi状态变化回调函数
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_wifi_init(wifi_status_callback_t callback);

/**
 * @brief 连接到WiFi网络
 * @param ssid WiFi SSID
 * @param password WiFi密码
 * @return esp_err_t ESP_OK表示启动成功
 */
esp_err_t wifi_connect_with_credentials(const char *ssid, const char *password);

/**
 * @brief 断开WiFi网络连接
 * @return esp_err_t ESP_OK表示断开成功
 */
esp_err_t onenet_wifi_disconnect(void);

/**
 * @brief 获取WiFi连接状态
 * @return true WiFi已连接
 */
bool wifi_is_connected(void);

/**
 * @brief 获取WiFi状态（详细状态）
 * @return wifi_state_t 当前WiFi状态
 */
wifi_state_t wifi_get_state(void);

/* ==================== OneNET HTTP功能函数 ==================== */

/**
 * @brief OneNET HTTP客户端初始化
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_http_init(void);

/**
 * @brief 测试OneNET HTTP连接
 * @return esp_err_t ESP_OK表示连接成功
 * @note 此函数会发送一个测试请求来验证连接
 */
esp_err_t onenet_http_test_connection(void);

/**
 * @brief 获取OneNET连接状态
 * @return onenet_state_t 当前连接状态
 * @note 基于设备上下线 API 的实际状态
 */
onenet_state_t onenet_http_get_state(void);

/**
 * @brief 设置设备在线状态
 * @param online true=上线, false=下线
 * @return esp_err_t ESP_OK表示成功
 * @note 调用 OneNET API: POST /device/online
 */
esp_err_t onenet_device_set_online(bool online);

/**
 * @brief 设备上线（简化接口）
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_device_online(void);

/**
 * @brief 设备下线（简化接口）
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_device_offline(void);

/**
 * @brief 检查设备是否在线
 * @return true 设备在线
 */
bool onenet_is_device_online(void);

/**
 * @brief 强制设置本地离线状态（不发送 HTTP 请求）
 * @note 用于 WiFi 断开时快速更新本地状态
 */
void onenet_device_set_offline_local(void);

/**
 * @brief 上报单个属性到OneNET
 * @param property_name 属性名称
 * @param value 属性值（字符串格式）
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_http_report_property(const char *property_name, const char *value);

/**
 * @brief 上报多个属性到OneNET（JSON格式）
 * @param params_json 属性JSON字符串
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_http_report_properties(const char *params_json);

/* 为了兼容旧代码，保留函数别名 */
#define onenet_mqtt_get_state()     onenet_http_get_state()
#define onenet_mqtt_init(cb)        onenet_http_init()
#define onenet_mqtt_start()         onenet_device_online()
#define onenet_mqtt_stop()          onenet_device_offline()

/* ==================== WiFi定位功能函数 ==================== */

/**
 * @brief 执行WIFI热点扫描
 * @param ap_list 输出参数，存储扫描到的热点信息
 * @param max_ap 最大扫描热点数量
 * @param timeout_ms 扫描超时时间（毫秒）
 * @return int 实际扫描到的热点数量，失败时返回-1
 */
int wifi_scan_access_points(wifi_ap_info_t *ap_list, int max_ap, uint32_t timeout_ms);

/**
 * @brief 获取当前连接的WIFI热点信息
 * @param connected_ap 输出参数
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t wifi_get_connected_ap_info(wifi_ap_info_t *connected_ap);

/**
 * @brief 收集WIFI定位数据
 * @param location_data 输出参数
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t wifi_collect_location_data(wifi_location_data_t *location_data);

/**
 * @brief 上报WIFI定位数据到OneNET平台（HTTP方式）
 * @param location_data WIFI定位数据结构指针
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_http_report_wifi_location(const wifi_location_data_t *location_data);

/**
 * @brief 执行完整的WIFI定位流程
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t wifi_location_report_once(void);

/**
 * @brief 异步触发WiFi定位上报（单次）
 */
void wifi_location_trigger_async(void);

/**
 * @brief 启动周期性WiFi定位上报
 * @param interval_ms 上报间隔（毫秒），最小1000ms，默认5000ms
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t wifi_location_start_periodic_report(uint32_t interval_ms);

/**
 * @brief 停止周期性WiFi定位上报
 */
void wifi_location_stop_periodic_report(void);

/**
 * @brief 检查WiFi定位上报是否正在运行
 * @return true 正在运行
 */
bool wifi_location_is_reporting(void);

/**
 * @brief 设置WIFI定位自动上报间隔（旧接口，建议使用 wifi_location_start_periodic_report）
 * @param interval_ms 间隔时间（毫秒），0表示禁用
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t wifi_location_set_auto_report_interval(uint32_t interval_ms);

/**
 * @brief WiFi管理器调用此函数来初始化SNTP
 */
void wifi_manager_init_sntp(void);

/**
 * @brief 设置位置信息回调函数
 * @param callback 位置信息回调函数
 */
void wifi_location_set_callback(location_callback_t callback);

/**
 * @brief 获取最后一次定位结果
 * @param location 输出参数
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t wifi_location_get_last_result(location_info_t *location);

/**
 * @brief 从 OneNET 平台获取最新的 WiFi 定位结果
 * @param location 输出参数，存储定位结果（经纬度）
 * @return esp_err_t ESP_OK表示成功
 * @note 调用 OneNET API: GET /fuse-lbs/latest-wifi-location
 */
esp_err_t onenet_get_wifi_location(location_info_t *location);

/**
 * @brief 存储示波器数据到缓冲区（满1024个时自动批量上报）
 * @param voltage 电压值 (float, 范围 -50.00 ~ 50.00)
 * @return esp_err_t ESP_OK表示成功
 * @note 属性标识符: Oscilloscope_data，缓冲区满时自动调用批量上报
 */
esp_err_t onenet_report_oscilloscope_data(float voltage);

/**
 * @brief 批量上报示波器数据到 OneNET
 * @return esp_err_t ESP_OK表示成功
 * @note 使用 /device/thing/pack/post 接口批量上报
 */
esp_err_t onenet_report_oscilloscope_batch(void);

/**
 * @brief 获取当前示波器数据缓冲区中的数据点数量
 * @return uint32_t 数据点数量
 */
uint32_t onenet_get_oscilloscope_buffer_count(void);

/**
 * @brief 设置WiFi定位上报间隔（动态调整）
 * @param interval_ms 新的上报间隔（毫秒），最小1000ms
 * @return esp_err_t ESP_OK表示成功
 * @note 如果定位上报正在运行，会立即生效
 */
esp_err_t wifi_location_set_report_interval(uint32_t interval_ms);

/**
 * @brief 获取当前WiFi定位上报间隔
 * @return uint32_t 当前间隔（毫秒），0表示未启用
 */
uint32_t wifi_location_get_report_interval(void);

/* ==================== 时间同步功能函数 ==================== */

/**
 * @brief 同步网络时间
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_sync_time(void);

/**
 * @brief 保存当前时间到NVS
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_save_time_to_nvs(void);

/**
 * @brief 从NVS恢复时间
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_restore_time_from_nvs(void);

/**
 * @brief 获取当前时间（字符串格式）
 * @param time_str 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param format 时间格式字符串
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t onenet_get_time_string(char *time_str, size_t buf_size, const char *format);

/**
 * @brief 检查时间是否已同步
 * @return true 时间已同步
 */
bool onenet_is_time_synced(void);

/**
 * @brief 生成设备级别的OneNet API Token
 * @param token_buf 输出token缓冲区
 * @param buf_size 缓冲区大小（建议至少256字节）
 * @return esp_err_t ESP_OK表示成功
 * @note 使用设备的sec_key生成，用于HTTP API认证（如文件上传）
 */
esp_err_t onenet_generate_device_token(char *token_buf, size_t buf_size);

/* ==================== 处理函数 ==================== */

/**
 * @brief OneNET处理函数（需要在主循环中定期调用）
 */
void onenet_process(void);

/* ==================== 便捷函数 ==================== */

/**
 * @brief WiFi和OneNET完整初始化
 * @param wifi_callback WiFi状态变化回调函数
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t wifi_onenet_init(wifi_status_callback_t wifi_callback);

/**
 * @brief 获取WiFi和OneNET状态信息（调试用）
 */
void wifi_onenet_status_info(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_ONENET_H */
