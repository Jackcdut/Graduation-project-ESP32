/**
 * @file weather_api.h
 * @brief 心知天气API接口头文件
 * 
 * 本文件定义了获取心知天气实况数据的接口
 */

#ifndef WEATHER_API_H
#define WEATHER_API_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 心知天气配置 */
#define SENIVERSE_API_KEY       "SJ-R69u0cq-9v19ew"  // 心知天气私钥
#define SENIVERSE_LOCATION      "chengdu"            // 成都市（支持：城市拼音、中文名、"四川成华"等）
#define SENIVERSE_LANGUAGE      "en"                 // 英文 (en=English, zh-Hans=简体中文)
#define SENIVERSE_UNIT          "c"                  // 摄氏度

/* 天气更新间隔（毫秒） */
#define WEATHER_UPDATE_INTERVAL_MS  (30 * 60 * 1000)  // 30分钟

/* HTTP响应缓冲区大小 */
#define WEATHER_HTTP_BUFFER_SIZE    2048

/**
 * @brief 天气信息结构体
 */
typedef struct {
    bool valid;                 // 数据是否有效
    char location_name[64];     // 位置名称（如：成都）
    char location_path[128];    // 完整路径（如：成都,四川,中国）
    char text[32];              // 天气现象文字（如：晴、多云）
    char code[8];               // 天气现象代码
    int temperature;            // 温度（摄氏度）
    int feels_like;             // 体感温度
    int humidity;               // 相对湿度（0-100%）
    char wind_direction[16];    // 风向文字
    int wind_scale;             // 风力等级
    char last_update[32];       // 最后更新时间
} weather_info_t;

/**
 * @brief 天气数据回调函数类型
 * @param weather 天气信息指针
 */
typedef void (*weather_callback_t)(const weather_info_t *weather);

/**
 * @brief 初始化天气API模块
 * @param callback 天气数据更新回调函数（可选，传NULL表示不使用回调）
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t weather_api_init(weather_callback_t callback);

/**
 * @brief 启动天气自动更新定时器
 * @note 会立即获取一次天气，然后每30分钟自动更新
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t weather_api_start_auto_update(void);

/**
 * @brief 停止天气自动更新定时器
 */
void weather_api_stop_auto_update(void);

/**
 * @brief 手动获取一次天气数据
 * @return esp_err_t ESP_OK表示成功
 */
esp_err_t weather_api_fetch_now(void);

/**
 * @brief 获取最后一次的天气数据
 * @param weather 输出参数，存储天气信息
 * @return esp_err_t ESP_OK表示成功，ESP_FAIL表示无有效数据
 */
esp_err_t weather_api_get_last_data(weather_info_t *weather);

/**
 * @brief 检查WiFi是否已连接
 * @return true表示已连接，false表示未连接
 */
bool weather_api_is_wifi_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_API_H
