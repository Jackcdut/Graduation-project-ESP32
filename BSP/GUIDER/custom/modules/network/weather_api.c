/**
 * @file weather_api.c
 * @brief 心知天气API接口实现
 */

/**
 * @file weather_api.c
 * @brief 心知天气API接口实现
 * 
 * @note This file is part of the network module (main/network/)
 */

#include "weather_api.h"
#include "wifi_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_crt_bundle.h"  /* 用于HTTPS证书验证 */
#include <string.h>

static const char *TAG = "WEATHER_API";

/* 全局变量 */
static weather_info_t s_weather_data = {0};
static weather_callback_t s_weather_callback = NULL;
static TimerHandle_t s_weather_timer = NULL;
static bool s_initialized = false;

/* HTTP事件处理器 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief 解析心知天气JSON响应
 */
static esp_err_t parse_weather_response(const char *json_data, weather_info_t *weather)
{
    if (json_data == NULL || weather == NULL) {
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Parsing weather JSON: %s", json_data);

    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    /* 初始化天气数据 */
    memset(weather, 0, sizeof(weather_info_t));
    weather->valid = false;

    /* 获取results数组 */
    cJSON *results = cJSON_GetObjectItem(root, "results");
    if (results == NULL || !cJSON_IsArray(results)) {
        ESP_LOGE(TAG, "No results array in JSON");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    /* 获取第一个结果 */
    cJSON *result = cJSON_GetArrayItem(results, 0);
    if (result == NULL) {
        ESP_LOGE(TAG, "Empty results array");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    /* 解析location信息 */
    cJSON *location = cJSON_GetObjectItem(result, "location");
    if (location) {
        cJSON *name = cJSON_GetObjectItem(location, "name");
        cJSON *path = cJSON_GetObjectItem(location, "path");
        
        if (name && cJSON_IsString(name)) {
            strncpy(weather->location_name, name->valuestring, sizeof(weather->location_name) - 1);
        }
        if (path && cJSON_IsString(path)) {
            strncpy(weather->location_path, path->valuestring, sizeof(weather->location_path) - 1);
        }
    }

    /* 解析now天气数据 */
    cJSON *now = cJSON_GetObjectItem(result, "now");
    if (now == NULL) {
        ESP_LOGE(TAG, "No 'now' object in result");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    /* 提取天气字段 */
    cJSON *text = cJSON_GetObjectItem(now, "text");
    cJSON *code = cJSON_GetObjectItem(now, "code");
    cJSON *temperature = cJSON_GetObjectItem(now, "temperature");
    cJSON *feels_like = cJSON_GetObjectItem(now, "feels_like");
    cJSON *humidity = cJSON_GetObjectItem(now, "humidity");
    cJSON *wind_direction = cJSON_GetObjectItem(now, "wind_direction");
    cJSON *wind_scale = cJSON_GetObjectItem(now, "wind_scale");

    if (text && cJSON_IsString(text)) {
        strncpy(weather->text, text->valuestring, sizeof(weather->text) - 1);
    }
    if (code && cJSON_IsString(code)) {
        strncpy(weather->code, code->valuestring, sizeof(weather->code) - 1);
    }
    if (temperature && cJSON_IsString(temperature)) {
        weather->temperature = atoi(temperature->valuestring);
    }
    if (feels_like && cJSON_IsString(feels_like)) {
        weather->feels_like = atoi(feels_like->valuestring);
    }
    if (humidity && cJSON_IsString(humidity)) {
        weather->humidity = atoi(humidity->valuestring);
    }
    if (wind_direction && cJSON_IsString(wind_direction)) {
        strncpy(weather->wind_direction, wind_direction->valuestring, sizeof(weather->wind_direction) - 1);
    }
    if (wind_scale && cJSON_IsString(wind_scale)) {
        weather->wind_scale = atoi(wind_scale->valuestring);
    }

    /* 获取last_update */
    cJSON *last_update = cJSON_GetObjectItem(result, "last_update");
    if (last_update && cJSON_IsString(last_update)) {
        strncpy(weather->last_update, last_update->valuestring, sizeof(weather->last_update) - 1);
    }

    weather->valid = true;
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Weather parsed: %s, %s, %d°C",
             weather->location_name, weather->text, weather->temperature);

    return ESP_OK;
}

/**
 * @brief 获取天气数据（HTTP请求）
 */
static esp_err_t fetch_weather_data(void)
{
    char url[256];
    char output_buffer[WEATHER_HTTP_BUFFER_SIZE] = {0};

    /* 构建请求URL */
    snprintf(url, sizeof(url),
             "https://api.seniverse.com/v3/weather/now.json?key=%s&location=%s&language=%s&unit=%s",
             SENIVERSE_API_KEY, SENIVERSE_LOCATION, SENIVERSE_LANGUAGE, SENIVERSE_UNIT);

    ESP_LOGI(TAG, "Fetching weather from: %s", url);

    /* 配置HTTP客户端 - 使用HTTPS并启用证书包验证 */
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,  /* 使用ESP32证书包进行验证 */
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    /* 设置GET方法 */
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    /* 打开连接 */
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    /* 获取响应头 */
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "HTTP client fetch headers failed");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    /* 读取响应数据 */
    int data_read = esp_http_client_read_response(client, output_buffer, WEATHER_HTTP_BUFFER_SIZE - 1);
    if (data_read >= 0) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", status_code, content_length);

        if (status_code == 200) {
            /* 解析JSON数据 */
            weather_info_t new_weather;
            if (parse_weather_response(output_buffer, &new_weather) == ESP_OK) {
                /* 更新全局天气数据 */
                memcpy(&s_weather_data, &new_weather, sizeof(weather_info_t));

                /* 调用回调函数 */
                if (s_weather_callback) {
                    s_weather_callback(&s_weather_data);
                }

                err = ESP_OK;
            } else {
                ESP_LOGE(TAG, "Failed to parse weather response");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "Failed to read response");
        err = ESP_FAIL;
    }

    /* 关闭连接 */
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return err;
}

/**
 * @brief 天气更新定时器回调
 */
static void weather_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Weather update timer triggered");
    weather_api_fetch_now();
}

/* ==================== 公共API实现 ==================== */

esp_err_t weather_api_init(weather_callback_t callback)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Weather API already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing Weather API...");

    s_weather_callback = callback;
    memset(&s_weather_data, 0, sizeof(weather_info_t));
    s_initialized = true;

    ESP_LOGI(TAG, "Weather API initialized successfully");
    return ESP_OK;
}

esp_err_t weather_api_start_auto_update(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Weather API not initialized");
        return ESP_FAIL;
    }

    /* 创建定时器（30分钟） */
    if (s_weather_timer == NULL) {
        s_weather_timer = xTimerCreate(
            "weather_timer",
            pdMS_TO_TICKS(WEATHER_UPDATE_INTERVAL_MS),
            pdTRUE,  // 自动重载
            NULL,
            weather_timer_callback
        );

        if (s_weather_timer == NULL) {
            ESP_LOGE(TAG, "Failed to create weather timer");
            return ESP_FAIL;
        }
    }

    /* 立即获取一次天气 */
    weather_api_fetch_now();

    /* 启动定时器 */
    if (xTimerStart(s_weather_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start weather timer");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Weather auto-update started (interval: %d minutes)",
             WEATHER_UPDATE_INTERVAL_MS / 60000);
    return ESP_OK;
}

void weather_api_stop_auto_update(void)
{
    if (s_weather_timer != NULL) {
        xTimerStop(s_weather_timer, 0);
        ESP_LOGI(TAG, "Weather auto-update stopped");
    }
}

esp_err_t weather_api_fetch_now(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Weather API not initialized");
        return ESP_FAIL;
    }

    /* 检查WiFi连接 */
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot fetch weather");
        return ESP_FAIL;
    }

    return fetch_weather_data();
}

esp_err_t weather_api_get_last_data(weather_info_t *weather)
{
    if (weather == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_weather_data.valid) {
        return ESP_FAIL;
    }

    memcpy(weather, &s_weather_data, sizeof(weather_info_t));
    return ESP_OK;
}

bool weather_api_is_wifi_connected(void)
{
    return wifi_manager_is_connected();
}

