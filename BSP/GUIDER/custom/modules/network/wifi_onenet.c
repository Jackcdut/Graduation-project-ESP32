/**
 * @file wifi_onenet.c
 * @brief ESP32 WiFi连接和OneNET云平台HTTP通信模块实现文件
 * @version 2.0
 * @date 2025-01-08
 *
 * @details
 * 本文件实现了ESP32连接WiFi网络并与中国移动OneNET云平台进行HTTP通信的所有功能。
 *
 * 版本2.0变更：
 * - 移除MQTT协议支持，改用HTTP协议
 * - 使用设备激活后获取的device_id和sec_key进行认证
 * - 在线状态基于HTTP请求时间判断
 *
 * 主要功能模块：
 * 1. WiFi网络连接管理
 * 2. OneNET HTTP数据上报
 * 3. WiFi定位服务
 * 4. 时间同步服务（SNTP）
 */

#include "wifi_onenet.h"
#include "wifi_manager.h"
#include <time.h>
#include <sys/time.h>
#include "cJSON.h"
#include "esp_mac.h"
#include "esp_sntp.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "mbedtls/md.h"
#include "mbedtls/base64.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ==================== 日志标签定义 ==================== */
static const char *TAG = "WIFI_ONENET";

/* ==================== NVS配置键名 ==================== */
#define NVS_NAMESPACE       "wifi_config"
#define NVS_SSID_KEY        "ssid"
#define NVS_PASSWORD_KEY    "password"
#define NVS_TIME_KEY        "last_time"
#define NVS_BOOT_KEY        "boot_time"

/* ==================== WiFi相关全局变量 ==================== */
static int s_retry_num = 0;
static wifi_state_t s_wifi_state = WIFI_STATE_DISCONNECTED;
static wifi_status_callback_t s_wifi_callback = NULL;
static bool s_wifi_state_changed = false;

/* ==================== OneNET HTTP相关全局变量 ==================== */
static onenet_state_t s_onenet_state = ONENET_STATE_UNKNOWN;
static bool s_device_online = false;  /* 设备在线状态（通过 API 设置） */
static bool s_http_initialized = false;

/* HTTP响应缓冲区 */
static char *s_http_response_buffer = NULL;
static int s_http_response_length = 0;
static int s_http_response_capacity = 0;

/* ==================== WIFI定位相关变量 ==================== */
static uint32_t s_wifi_location_report_interval = 0;
static int64_t s_last_wifi_location_report_time = 0;
static location_callback_t s_location_callback = NULL;
static location_info_t s_last_location = {0};
static TaskHandle_t s_wifi_location_task_handle = NULL;
static bool s_wifi_location_task_running = false;

/* ==================== 时间同步相关变量 ==================== */
static bool s_time_synced = false;
static time_t s_last_synced_time = 0;
static int64_t s_last_sync_tick = 0;
static bool s_sntp_initialized = false;

/* ==================== 外部函数声明（从cloud_manager获取设备凭证）==================== */
/* 这些函数在cloud_manager.c中实现 */
extern esp_err_t cloud_manager_get_device_credentials(char *product_id, size_t pid_len,
                                                       char *device_name, size_t name_len,
                                                       char *sec_key, size_t key_len);
extern bool cloud_manager_is_activated(void);

/* ==================== 内部函数声明 ==================== */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifi_location_report_task(void *arg);
static void process_wifi_state_change(void);
static void process_wifi_location_auto_report(void);
static esp_err_t parse_location_response(const char *data, location_info_t *location);
static void initialize_sntp(void);
static esp_err_t nvs_read_wifi_config(char *ssid, char *password);
static esp_err_t nvs_save_wifi_config(const char *ssid, const char *password);
static esp_err_t nvs_save_time(void);
static esp_err_t nvs_restore_time(void);
static esp_err_t generate_device_token(const char *product_id, const char *device_name, 
                                       const char *sec_key, char *token_buf, size_t buf_size);
static esp_err_t http_event_handler(esp_http_client_event_t *evt);

/* ==================== HTTP事件处理函数 ==================== */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                int data_len = evt->data_len;
                if (s_http_response_buffer == NULL) {
                    s_http_response_capacity = data_len + 1;
                    s_http_response_buffer = malloc(s_http_response_capacity);
                    if (s_http_response_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate response buffer");
                        return ESP_FAIL;
                    }
                    s_http_response_length = 0;
                } else if (s_http_response_length + data_len + 1 > s_http_response_capacity) {
                    s_http_response_capacity = s_http_response_length + data_len + 1;
                    char *new_buf = realloc(s_http_response_buffer, s_http_response_capacity);
                    if (new_buf == NULL) {
                        ESP_LOGE(TAG, "Failed to reallocate response buffer");
                        return ESP_FAIL;
                    }
                    s_http_response_buffer = new_buf;
                }
                memcpy(s_http_response_buffer + s_http_response_length, evt->data, data_len);
                s_http_response_length += data_len;
                s_http_response_buffer[s_http_response_length] = '\0';
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

/* ==================== WiFi事件处理函数实现 ==================== */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi STA started, connecting to AP...");
        s_wifi_state = WIFI_STATE_CONNECTING;
        s_wifi_state_changed = true;
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (attempt %d/%d)", s_retry_num, WIFI_MAXIMUM_RETRY);
            s_wifi_state = WIFI_STATE_CONNECTING;
        } else {
            ESP_LOGE(TAG, "Failed to connect to AP after %d attempts", WIFI_MAXIMUM_RETRY);
            s_wifi_state = WIFI_STATE_FAILED;
        }
        s_wifi_state_changed = true;
        ESP_LOGW(TAG, "WiFi disconnected");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected! Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_wifi_state = WIFI_STATE_CONNECTED;
        s_wifi_state_changed = true;
        initialize_sntp();
    }
}

/* ==================== 设备Token生成函数 ==================== */
/**
 * @brief 生成设备级别的OneNet API Token
 * @note 使用设备的sec_key生成，用于HTTP API认证
 */
static esp_err_t generate_device_token(const char *product_id, const char *device_name,
                                       const char *sec_key, char *token_buf, size_t buf_size)
{
    if (product_id == NULL || device_name == NULL || sec_key == NULL || token_buf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    time_t now;
    time(&now);
    time_t expire_time = now + 365 * 24 * 3600; /* 1年有效期 */

    /* 构建签名字符串: et\nmethod\nres\nversion 
     * 根据 OneNET Studio 文档，格式为：
     * {expire_time}\n{method}\n{res}\n{version}
     */
    char string_to_sign[256];
    snprintf(string_to_sign, sizeof(string_to_sign),
             "%lld\nsha256\nproducts/%s/devices/%s\n2018-10-31",
             (long long)expire_time, product_id, device_name);

    ESP_LOGI(TAG, "Token string_to_sign: %s", string_to_sign);

    /* 计算HMAC-SHA256签名 */
    uint8_t hmac[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);

    /* 
     * OneNET Studio 设备 sec_key 是 Base64 编码的密钥
     * 需要先 Base64 解码，然后用解码后的密钥进行 HMAC-SHA256 签名
     */
    uint8_t key_decoded[64];
    size_t key_decoded_len = 0;
    size_t sec_key_len = strlen(sec_key);
    
    int ret = mbedtls_base64_decode(key_decoded, sizeof(key_decoded), &key_decoded_len,
                                    (const uint8_t *)sec_key, sec_key_len);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to decode sec_key: %d (sec_key_len=%d)", ret, (int)sec_key_len);
        ESP_LOGE(TAG, "sec_key may be corrupted or invalid. Please re-activate the device.");
        mbedtls_md_free(&ctx);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "sec_key decoded: input_len=%d, decoded_len=%d", (int)sec_key_len, (int)key_decoded_len);

    mbedtls_md_hmac_starts(&ctx, key_decoded, key_decoded_len);
    mbedtls_md_hmac_update(&ctx, (const uint8_t *)string_to_sign, strlen(string_to_sign));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    /* Base64编码签名 */
    char sign_base64[64];
    size_t sign_len = 0;
    mbedtls_base64_encode((uint8_t *)sign_base64, sizeof(sign_base64), &sign_len, hmac, 32);
    sign_base64[sign_len] = '\0';

    /* URL编码签名 */
    char sign_encoded[128];
    char *p = sign_encoded;
    for (size_t i = 0; i < sign_len; i++) {
        if (sign_base64[i] == '+') {
            *p++ = '%'; *p++ = '2'; *p++ = 'B';
        } else if (sign_base64[i] == '/') {
            *p++ = '%'; *p++ = '2'; *p++ = 'F';
        } else if (sign_base64[i] == '=') {
            *p++ = '%'; *p++ = '3'; *p++ = 'D';
        } else {
            *p++ = sign_base64[i];
        }
    }
    *p = '\0';

    /* 构建最终Token */
    snprintf(token_buf, buf_size,
             "version=2018-10-31&res=products%%2F%s%%2Fdevices%%2F%s&et=%lld&method=sha256&sign=%s",
             product_id, device_name, (long long)expire_time, sign_encoded);

    ESP_LOGI(TAG, "Generated token: version=2018-10-31&res=products/%s/devices/%s&et=%lld&method=sha256&sign=...",
             product_id, device_name, (long long)expire_time);
    
    return ESP_OK;
}

/* ==================== OneNET HTTP功能函数实现 ==================== */

esp_err_t onenet_http_init(void)
{
    if (s_http_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing OneNET HTTP client...");
    ESP_LOGI(TAG, "HTTP API Host: %s:%d", ONENET_HTTP_API_HOST, ONENET_HTTP_API_PORT);

    s_onenet_state = ONENET_STATE_INIT;
    s_http_initialized = true;

    ESP_LOGI(TAG, "OneNET HTTP client initialized successfully");
    return ESP_OK;
}

onenet_state_t onenet_http_get_state(void)
{
    if (!s_http_initialized) {
        return ONENET_STATE_UNKNOWN;
    }

    if (!cloud_manager_is_activated()) {
        return ONENET_STATE_OFFLINE;
    }

    /* 基于设备上下线 API 的实际状态 */
    return s_device_online ? ONENET_STATE_ONLINE : ONENET_STATE_OFFLINE;
}

bool onenet_is_device_online(void)
{
    return s_device_online && s_http_initialized && cloud_manager_is_activated();
}

/* ==================== 设备上下线 API 实现 ==================== */
esp_err_t onenet_device_set_online(bool online)
{
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot set device %s", online ? "online" : "offline");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }

    /* 获取设备凭证 */
    char product_id[32] = {0};
    char device_name[64] = {0};
    char sec_key[64] = {0};

    esp_err_t ret = cloud_manager_get_device_credentials(product_id, sizeof(product_id),
                                                          device_name, sizeof(device_name),
                                                          sec_key, sizeof(sec_key));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device credentials");
        return ret;
    }

    if (strlen(sec_key) == 0) {
        ESP_LOGE(TAG, "sec_key is empty");
        return ESP_ERR_INVALID_STATE;
    }

    /* 生成Token */
    char auth_token[512];
    ret = generate_device_token(product_id, device_name, sec_key, auth_token, sizeof(auth_token));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate token");
        return ret;
    }

    /* 获取设备 MAC 地址 */
    uint8_t mac[6];
    char mac_str[18];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* 
     * 构造请求体
     * POST /device/online
     * {
     *   "pid": "xxx",
     *   "devName": "xxx",
     *   "status": 1,  // 1-在线 0-离线
     *   "protocol": "HTTP",
     *   "network": 0,  // 0-非蜂窝
     *   "mac": "xx:xx:xx:xx:xx:xx"
     * }
     */
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "pid", product_id);
    cJSON_AddStringToObject(root, "devName", device_name);
    cJSON_AddNumberToObject(root, "status", online ? 1 : 0);
    cJSON_AddStringToObject(root, "protocol", "HTTP");
    cJSON_AddNumberToObject(root, "network", 0);  /* 非蜂窝 */
    cJSON_AddStringToObject(root, "mac", mac_str);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (body == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Device %s request: %s", online ? "ONLINE" : "OFFLINE", body);

    /* 清空响应缓冲区 */
    if (s_http_response_buffer != NULL) {
        free(s_http_response_buffer);
        s_http_response_buffer = NULL;
    }
    s_http_response_length = 0;

    /* 配置HTTP客户端 - 使用 open.iot.10086.cn */
    esp_http_client_config_t config = {
        .host = ONENET_HTTP_API_HOST,
        .port = ONENET_HTTP_API_PORT,
        .path = ONENET_HTTP_DEVICE_ONLINE,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        free(body);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "token", auth_token);
    esp_http_client_set_post_field(client, body, strlen(body));

    ESP_LOGI(TAG, "Sending device online request to: %s%s", ONENET_HTTP_API_HOST, ONENET_HTTP_DEVICE_ONLINE);

    ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Device online API Status = %d", status_code);

        if (s_http_response_buffer != NULL && s_http_response_length > 0) {
            ESP_LOGI(TAG, "Response: %s", s_http_response_buffer);

            cJSON *resp = cJSON_Parse(s_http_response_buffer);
            if (resp != NULL) {
                cJSON *errno_item = cJSON_GetObjectItem(resp, "errno");
                cJSON *error_item = cJSON_GetObjectItem(resp, "error");
                
                bool success = false;
                if (cJSON_IsNumber(errno_item) && errno_item->valueint == 0) {
                    success = true;
                }
                if (cJSON_IsString(error_item) && strcmp(error_item->valuestring, "succ") == 0) {
                    success = true;
                }

                if (success) {
                    s_device_online = online;
                    s_onenet_state = online ? ONENET_STATE_ONLINE : ONENET_STATE_OFFLINE;
                    ESP_LOGI(TAG, "Device %s successfully!", online ? "ONLINE" : "OFFLINE");
                    ret = ESP_OK;
                } else {
                    const char *err_msg = cJSON_IsString(error_item) ? error_item->valuestring : "unknown";
                    ESP_LOGE(TAG, "Device online API failed: %s", err_msg);
                    ret = ESP_FAIL;
                }
                cJSON_Delete(resp);
            }

            free(s_http_response_buffer);
            s_http_response_buffer = NULL;
            s_http_response_length = 0;
        } else {
            /* 没有响应体，检查状态码 */
            if (status_code >= 200 && status_code < 300) {
                s_device_online = online;
                s_onenet_state = online ? ONENET_STATE_ONLINE : ONENET_STATE_OFFLINE;
                ESP_LOGI(TAG, "Device %s (no response body)", online ? "ONLINE" : "OFFLINE");
                ret = ESP_OK;
            } else {
                ret = ESP_FAIL;
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    free(body);

    return ret;
}

esp_err_t onenet_device_online(void)
{
    return onenet_device_set_online(true);
}

esp_err_t onenet_device_offline(void)
{
    return onenet_device_set_online(false);
}

void onenet_device_set_offline_local(void)
{
    /* 强制设置本地离线状态（不发送 HTTP 请求） */
    s_device_online = false;
    s_onenet_state = ONENET_STATE_OFFLINE;
    ESP_LOGI(TAG, "Device set to OFFLINE (local only)");
}

esp_err_t onenet_http_test_connection(void)
{
    /* 
     * 使用 OneNET 设备上下线 API 来设置设备在线状态
     * POST /device/online
     */
    ESP_LOGI(TAG, "Testing OneNET connection via device online API...");
    
    /* 调用设备上线 API */
    esp_err_t ret = onenet_device_online();
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Device is now ONLINE on OneNET!");
    } else {
        ESP_LOGW(TAG, "Device online API failed, ret=%d", ret);
    }
    
    return ret;
}

esp_err_t onenet_http_report_property(const char *property_name, const char *value)
{
    if (property_name == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 
     * 构造 OneJSON 格式的 params
     * 格式: "property_name": {"value": xxx}
     * 注意: value 直接嵌入，调用者需要确保格式正确（数字不带引号，字符串带引号）
     */
    char params_json[256];
    snprintf(params_json, sizeof(params_json),
             "\"%s\":{\"value\":%s}", property_name, value);

    return onenet_http_report_properties(params_json);
}

esp_err_t onenet_http_report_properties(const char *params_json)
{
    if (params_json == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot report properties");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }

    /* 获取设备凭证 */
    char product_id[32] = {0};
    char device_name[64] = {0};
    char sec_key[64] = {0};

    esp_err_t ret = cloud_manager_get_device_credentials(product_id, sizeof(product_id),
                                                          device_name, sizeof(device_name),
                                                          sec_key, sizeof(sec_key));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device credentials");
        return ret;
    }

    ESP_LOGI(TAG, "Property upload: PID=%s, Name=%s", product_id, device_name);
    if (strlen(sec_key) == 0) {
        ESP_LOGE(TAG, "ERROR: sec_key is EMPTY!");
        return ESP_ERR_INVALID_STATE;
    }

    /* 生成Token */
    char auth_token[512];
    ret = generate_device_token(product_id, device_name, sec_key, auth_token, sizeof(auth_token));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate token");
        return ret;
    }

    /* 
     * 根据 OneNET HTTP 接入文档:
     * - URL: https://open.iot.10086.cn/fuse/http/device/thing/property/post
     * - Header: token (鉴权), Content-Type: application/json
     * - Request Parameters: topic, protocol (作为URL查询参数)
     * - Body: OneJSON格式报文体
     * 
     * OneJSON 格式:
     * {
     *   "id": "123",
     *   "version": "1.0",
     *   "params": {
     *     "property_name": {"value": xxx, "time": 1234567890123}
     *   }
     * }
     */
    
    /* 构造 topic (URL编码) */
    char topic[128];
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", product_id, device_name);

    /* 构造 OneJSON 请求体 */
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    char msg_id[16];
    snprintf(msg_id, sizeof(msg_id), "%lu", (unsigned long)(esp_timer_get_time() / 1000));
    cJSON_AddStringToObject(root, "id", msg_id);
    cJSON_AddStringToObject(root, "version", "1.0");

    /* 解析 params_json */
    char full_params[1024];
    snprintf(full_params, sizeof(full_params), "{%s}", params_json);
    cJSON *params = cJSON_Parse(full_params);
    if (params != NULL) {
        cJSON_AddItemToObject(root, "params", params);
    } else {
        ESP_LOGE(TAG, "Failed to parse params_json: %s", params_json);
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (body == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "OneJSON Body: %s", body);

    /* 清空响应缓冲区 */
    if (s_http_response_buffer != NULL) {
        free(s_http_response_buffer);
        s_http_response_buffer = NULL;
    }
    s_http_response_length = 0;

    /* 
     * 构造完整URL，包含查询参数 topic 和 protocol
     * URL编码 topic 中的特殊字符: $ -> %24, / -> %2F
     */
    char topic_encoded[256];
    char *p = topic_encoded;
    for (const char *s = topic; *s && (p - topic_encoded) < (int)sizeof(topic_encoded) - 4; s++) {
        if (*s == '$') {
            *p++ = '%'; *p++ = '2'; *p++ = '4';
        } else if (*s == '/') {
            *p++ = '%'; *p++ = '2'; *p++ = 'F';
        } else {
            *p++ = *s;
        }
    }
    *p = '\0';

    char url_path[512];
    snprintf(url_path, sizeof(url_path), "%s?topic=%s&protocol=HTTP",
             ONENET_HTTP_PROPERTY_UPLOAD, topic_encoded);
    
    ESP_LOGI(TAG, "URL Path: %s", url_path);

    /* 配置HTTP客户端 */
    esp_http_client_config_t config = {
        .host = ONENET_HTTP_API_HOST,
        .port = ONENET_HTTP_API_PORT,
        .path = url_path,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        free(body);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "token", auth_token);
    esp_http_client_set_post_field(client, body, strlen(body));

    ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, Content-Length = %d", status_code, content_length);

        /* 解析响应 */
        if (s_http_response_buffer != NULL && s_http_response_length > 0) {
            ESP_LOGI(TAG, "Response (%d bytes): %s", s_http_response_length, s_http_response_buffer);

            cJSON *resp = cJSON_Parse(s_http_response_buffer);
            if (resp != NULL) {
                /* OneNET HTTP 接入返回码可能是字符串 "succ" 或数字 0 */
                cJSON *code = cJSON_GetObjectItem(resp, "code");
                bool success = false;
                
                if (cJSON_IsString(code) && strcmp(code->valuestring, "succ") == 0) {
                    success = true;
                } else if (cJSON_IsNumber(code) && code->valueint == 0) {
                    success = true;
                }
                
                if (success) {
                    ESP_LOGI(TAG, "Property uploaded successfully!");
                    ret = ESP_OK;
                } else {
                    const char *error_code_str = "unknown";
                    if (cJSON_IsString(code)) {
                        error_code_str = code->valuestring;
                    } else if (cJSON_IsNumber(code)) {
                        static char code_buf[16];
                        snprintf(code_buf, sizeof(code_buf), "%d", code->valueint);
                        error_code_str = code_buf;
                    }
                    
                    ESP_LOGW(TAG, "API response code: %s", error_code_str);
                    
                    /* 根据 OneNET HTTP 接入返回码处理 */
                    if (cJSON_IsString(code)) {
                        if (strcmp(code->valuestring, "authPermissionDeny") == 0) {
                            ESP_LOGE(TAG, "Authentication failed!");
                            ret = ESP_ERR_INVALID_STATE;
                        } else if (strcmp(code->valuestring, "invalidParameter") == 0) {
                            ESP_LOGE(TAG, "Invalid parameter!");
                            ret = ESP_ERR_INVALID_ARG;
                        } else {
                            ret = ESP_FAIL;
                        }
                    } else {
                        ret = ESP_FAIL;
                    }
                }
                cJSON_Delete(resp);
            }

            free(s_http_response_buffer);
            s_http_response_buffer = NULL;
            s_http_response_length = 0;
        } else {
            ESP_LOGW(TAG, "No response body received (buffer=%p, len=%d)", 
                     (void*)s_http_response_buffer, s_http_response_length);
            /* HTTP 200 但没有响应体，可能也是成功 */
            if (status_code >= 200 && status_code < 300) {
                ret = ESP_OK;
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    free(body);

    return ret;
}

/* ==================== WiFi定位HTTP上报实现 ==================== */
esp_err_t onenet_http_report_wifi_location(const wifi_location_data_t *location_data)
{
    if (location_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot report location");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }

    /* 
     * 根据 OneNET 官方文档，WiFi 定位数据通过物模型上报到 $OneNET_LBS_WIFI 系统功能点
     * 
     * 请求格式：
     * {
     *   "id": "xxx",
     *   "version": "1.0",
     *   "params": {
     *     "$OneNET_LBS_WIFI": {
     *       "value": {
     *         "macs": "mac1,rssi1|mac2,rssi2|...",  // 必填，至少2个AP
     *         "mmac": "connected_mac,rssi,ssid",    // 选填，已连接热点
     *         "imsi": "352315052834187",            // 选填
     *         "smac": "device_mac",                 // 选填
     *         "idfa": "device_id",                  // 选填
     *         "serverip": "gateway_ip"              // 选填
     *       }
     *     }
     *   }
     * }
     */
    
    ESP_LOGI(TAG, "Uploading WiFi location data to $OneNET_LBS_WIFI...");
    ESP_LOGI(TAG, "  macs: %s", location_data->macs);
    ESP_LOGI(TAG, "  mmac: %s", location_data->mmac);
    ESP_LOGI(TAG, "  smac: %s", location_data->smac);

    /* 构造 $OneNET_LBS_WIFI 的 value 对象 */
    cJSON *value = cJSON_CreateObject();
    if (value == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /* macs 是必填字段 */
    if (strlen(location_data->macs) > 0) {
        cJSON_AddStringToObject(value, "macs", location_data->macs);
    } else {
        ESP_LOGE(TAG, "macs is required but empty!");
        cJSON_Delete(value);
        return ESP_ERR_INVALID_ARG;
    }

    /* 添加可选字段 */
    if (strlen(location_data->mmac) > 0) {
        cJSON_AddStringToObject(value, "mmac", location_data->mmac);
    }
    if (strlen(location_data->smac) > 0) {
        cJSON_AddStringToObject(value, "smac", location_data->smac);
    }
    if (strlen(location_data->imsi) > 0) {
        cJSON_AddStringToObject(value, "imsi", location_data->imsi);
    }
    if (strlen(location_data->idfa) > 0) {
        cJSON_AddStringToObject(value, "idfa", location_data->idfa);
    }
    if (strlen(location_data->serverip) > 0) {
        cJSON_AddStringToObject(value, "serverip", location_data->serverip);
    }

    /* 构造完整的 params JSON 字符串 */
    char *value_str = cJSON_PrintUnformatted(value);
    cJSON_Delete(value);
    
    if (value_str == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /* 格式: "$OneNET_LBS_WIFI":{"value":{...}} */
    char params_json[1536];
    snprintf(params_json, sizeof(params_json),
             "\"$OneNET_LBS_WIFI\":{\"value\":%s}", value_str);
    free(value_str);

    ESP_LOGI(TAG, "WiFi LBS params: %s", params_json);

    /* 使用通用属性上报函数 */
    esp_err_t ret = onenet_http_report_properties(params_json);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi location data uploaded successfully!");
    } else {
        ESP_LOGE(TAG, "WiFi location upload failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/* ==================== 获取 WiFi 定位结果 ==================== */
esp_err_t onenet_get_wifi_location(location_info_t *location)
{
    if (location == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot get location");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }

    /* 获取设备凭证 */
    char product_id[32] = {0};
    char device_name[64] = {0};
    char sec_key[64] = {0};

    esp_err_t ret = cloud_manager_get_device_credentials(product_id, sizeof(product_id),
                                                          device_name, sizeof(device_name),
                                                          sec_key, sizeof(sec_key));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device credentials");
        return ret;
    }

    if (strlen(sec_key) == 0) {
        ESP_LOGE(TAG, "sec_key is empty");
        return ESP_ERR_INVALID_STATE;
    }

    /* 生成Token */
    char auth_token[512];
    ret = generate_device_token(product_id, device_name, sec_key, auth_token, sizeof(auth_token));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate token");
        return ret;
    }

    /* 清空响应缓冲区 */
    if (s_http_response_buffer != NULL) {
        free(s_http_response_buffer);
        s_http_response_buffer = NULL;
    }
    s_http_response_length = 0;

    /* 
     * OneNET WiFi 定位获取 API
     * GET https://iot-api.heclouds.com/fuse-lbs/latest-wifi-location?product_id=xxx&device_name=xxx
     */
    char url_path[256];
    snprintf(url_path, sizeof(url_path), 
             "/fuse-lbs/latest-wifi-location?product_id=%s&device_name=%s",
             product_id, device_name);

    ESP_LOGI(TAG, "Getting WiFi location from: %s", url_path);

    /* 配置HTTP客户端 - 使用 iot-api.heclouds.com */
    esp_http_client_config_t config = {
        .host = "iot-api.heclouds.com",
        .port = 443,
        .path = url_path,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 15000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "authorization", auth_token);

    ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "GET Location Status = %d", status_code);

        if (s_http_response_buffer != NULL && s_http_response_length > 0) {
            ESP_LOGI(TAG, "Location Response: %s", s_http_response_buffer);

            /* 解析响应 JSON */
            cJSON *resp = cJSON_Parse(s_http_response_buffer);
            if (resp != NULL) {
                cJSON *code = cJSON_GetObjectItem(resp, "code");
                bool success = false;
                
                if (cJSON_IsNumber(code) && code->valueint == 0) {
                    success = true;
                } else if (cJSON_IsString(code) && strcmp(code->valuestring, "succ") == 0) {
                    success = true;
                }

                if (success) {
                    cJSON *data = cJSON_GetObjectItem(resp, "data");
                    if (data != NULL) {
                        cJSON *lon = cJSON_GetObjectItem(data, "lon");
                        cJSON *lat = cJSON_GetObjectItem(data, "lat");
                        cJSON *at = cJSON_GetObjectItem(data, "at");

                        if (cJSON_IsNumber(lon) && cJSON_IsNumber(lat)) {
                            location->longitude = (float)lon->valuedouble;
                            location->latitude = (float)lat->valuedouble;
                            location->valid = true;
                            location->radius = 0;  /* API 不返回精度 */
                            
                            if (cJSON_IsString(at)) {
                                snprintf(location->address, sizeof(location->address), 
                                         "Updated: %s", at->valuestring);
                            } else {
                                location->address[0] = '\0';
                            }

                            /* 更新缓存的定位结果 */
                            memcpy(&s_last_location, location, sizeof(location_info_t));

                            ESP_LOGI(TAG, "Location: lon=%.6f, lat=%.6f", 
                                     location->longitude, location->latitude);
                            ret = ESP_OK;
                        } else {
                            ESP_LOGW(TAG, "Location data incomplete");
                            ret = ESP_ERR_NOT_FOUND;
                        }
                    } else {
                        ESP_LOGW(TAG, "No data in response");
                        ret = ESP_ERR_NOT_FOUND;
                    }
                } else {
                    cJSON *msg = cJSON_GetObjectItem(resp, "msg");
                    ESP_LOGW(TAG, "API error: %s", 
                             cJSON_IsString(msg) ? msg->valuestring : "unknown");
                    ret = ESP_FAIL;
                }
                cJSON_Delete(resp);
            } else {
                ESP_LOGE(TAG, "Failed to parse response JSON");
                ret = ESP_FAIL;
            }

            free(s_http_response_buffer);
            s_http_response_buffer = NULL;
            s_http_response_length = 0;
        } else {
            ESP_LOGW(TAG, "No response body");
            ret = ESP_ERR_NOT_FOUND;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    return ret;
}


/* ==================== NVS操作函数实现 ==================== */

static esp_err_t nvs_read_wifi_config(char *ssid, char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    size_t ssid_len = 32;
    size_t pass_len = 64;

    err = nvs_get_str(nvs_handle, NVS_SSID_KEY, ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_get_str(nvs_handle, NVS_PASSWORD_KEY, password, &pass_len);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "WiFi config read from NVS: SSID=%s", ssid);
    return ESP_OK;
}

static esp_err_t nvs_save_wifi_config(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_SSID_KEY, ssid);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_PASSWORD_KEY, password);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "WiFi config saved to NVS: SSID=%s", ssid);
    return err;
}

static esp_err_t nvs_save_time(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    time_t now = time(NULL);
    uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000);

    nvs_set_i64(nvs_handle, NVS_TIME_KEY, (int64_t)now);
    nvs_set_u32(nvs_handle, NVS_BOOT_KEY, uptime_sec);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    return ESP_OK;
}

static esp_err_t nvs_restore_time(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    int64_t saved_time = 0;
    err = nvs_get_i64(nvs_handle, NVS_TIME_KEY, &saved_time);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);

    struct timeval tv = {
        .tv_sec = (time_t)saved_time,
        .tv_usec = 0
    };
    settimeofday(&tv, NULL);

    setenv("TZ", "CST-8", 1);
    tzset();

    s_time_synced = true;
    return ESP_OK;
}

/* ==================== 位置信息解析 ==================== */
static esp_err_t parse_location_response(const char *data, location_info_t *location)
{
    if (data == NULL || location == NULL) {
        return ESP_FAIL;
    }

    memset(location, 0, sizeof(location_info_t));
    location->valid = false;

    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        return ESP_FAIL;
    }

    cJSON *params = cJSON_GetObjectItem(root, "params");
    if (params == NULL) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *lbs_wifi = cJSON_GetObjectItem(params, "$OneNET_LBS_WIFI");
    if (lbs_wifi == NULL) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *value = cJSON_GetObjectItem(lbs_wifi, "value");
    if (value == NULL) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *lon = cJSON_GetObjectItem(value, "lon");
    cJSON *lat = cJSON_GetObjectItem(value, "lat");
    cJSON *radius = cJSON_GetObjectItem(value, "radius");
    cJSON *address = cJSON_GetObjectItem(value, "address");

    if (lon && lat) {
        location->longitude = lon->valuedouble;
        location->latitude = lat->valuedouble;
        location->valid = true;

        if (radius) {
            location->radius = radius->valuedouble;
        }

        if (address && address->valuestring) {
            snprintf(location->address, sizeof(location->address), "%s", address->valuestring);
        }
    }

    cJSON_Delete(root);
    return location->valid ? ESP_OK : ESP_FAIL;
}

/* ==================== WiFi功能函数实现 ==================== */

esp_err_t onenet_wifi_init(wifi_status_callback_t callback)
{
    ESP_LOGW(TAG, "onenet_wifi_init() - WiFi event handling delegated to wifi_manager");
    s_wifi_callback = callback;
    s_wifi_state = WIFI_STATE_DISCONNECTED;
    s_wifi_state_changed = false;
    s_retry_num = 0;
    return ESP_OK;
}

esp_err_t wifi_connect_with_credentials(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Connecting to WiFi: SSID=%s", ssid);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    /* 使用 snprintf 代替 strncpy 避免截断警告 */
    snprintf((char*)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", ssid);
    snprintf((char*)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", password);

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        s_wifi_state = WIFI_STATE_FAILED;
        s_wifi_state_changed = true;
        return ret;
    }

    nvs_save_wifi_config(ssid, password);

    s_retry_num = 0;
    s_wifi_state = WIFI_STATE_CONNECTING;
    s_wifi_state_changed = true;

    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi connection: %s", esp_err_to_name(ret));
        s_wifi_state = WIFI_STATE_FAILED;
        s_wifi_state_changed = true;
        return ret;
    }

    return ESP_OK;
}

esp_err_t onenet_wifi_disconnect(void)
{
    ESP_LOGI(TAG, "Disconnecting from WiFi...");
    return esp_wifi_disconnect();
}

bool wifi_is_connected(void)
{
    /* 使用 wifi_manager 的连接状态，因为 WiFi 事件由 wifi_manager 处理 */
    return wifi_manager_is_connected();
}

wifi_state_t wifi_get_state(void)
{
    return s_wifi_state;
}

/* ==================== WiFi定位功能实现 ==================== */

/* WiFi定位上报任务 - 支持循环上报 */
static void wifi_location_report_task(void *arg)
{
    uint32_t initial_interval_ms = (uint32_t)(uintptr_t)arg;  /* 初始上报间隔，0表示只上报一次 */
    bool single_mode = (initial_interval_ms == 0);
    
    /* 设置初始间隔到全局变量 */
    if (!single_mode && initial_interval_ms > 0) {
        s_wifi_location_report_interval = initial_interval_ms;
    }
    
    s_wifi_location_task_running = true;
    ESP_LOGI(TAG, "WiFi location report task started, interval=%lu ms, single_mode=%d", 
             initial_interval_ms, single_mode);
    
    while (s_wifi_location_task_running) {
        /* 检查WiFi连接 */
        if (!wifi_is_connected()) {
            ESP_LOGW(TAG, "WiFi not connected, waiting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        /* 检查设备是否已激活 */
        if (!cloud_manager_is_activated()) {
            ESP_LOGW(TAG, "Device not activated, waiting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        ESP_LOGI(TAG, "Executing WiFi location report...");
        
        /* 收集WiFi定位数据 */
        wifi_location_data_t *location_data = (wifi_location_data_t *)malloc(sizeof(wifi_location_data_t));
        if (location_data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for location data");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        esp_err_t ret = wifi_collect_location_data(location_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to collect WiFi location data");
            free(location_data);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        /* 使用HTTP上报 */
        ret = onenet_http_report_wifi_location(location_data);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "WiFi location report completed successfully");
            s_last_wifi_location_report_time = esp_timer_get_time() / 1000;
            
            /* 上报成功后，延迟一小段时间再获取定位结果（等待平台处理） */
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            /* 从 OneNET 获取定位结果 */
            location_info_t loc_result = {0};
            esp_err_t loc_ret = onenet_get_wifi_location(&loc_result);
            if (loc_ret == ESP_OK && loc_result.valid) {
                ESP_LOGI(TAG, "Got location: lon=%.6f, lat=%.6f", 
                         loc_result.longitude, loc_result.latitude);
                /* 更新缓存的定位结果 */
                memcpy(&s_last_location, &loc_result, sizeof(location_info_t));
                /* 调用回调通知 UI */
                if (s_location_callback != NULL) {
                    s_location_callback(&loc_result);
                }
            } else {
                ESP_LOGW(TAG, "Failed to get location result (may need more time)");
            }
        } else {
            ESP_LOGE(TAG, "WiFi location report failed");
        }
        
        /* 释放 location_data */
        free(location_data);
        
        /* 如果是单次模式，上报一次后进入休眠 */
        if (single_mode) {
            ESP_LOGI(TAG, "Single report mode, task entering idle");
            break;
        }
        
        /* 动态读取当前间隔（支持运行时调整，不限制最小间隔） */
        uint32_t current_interval = s_wifi_location_report_interval;
        if (current_interval == 0) current_interval = 1;  /* 防止0间隔 */
        
        /* 等待下一次上报 */
        if (current_interval < 1000) {
            ESP_LOGI(TAG, "Next report in %lu ms", current_interval);
        } else {
            ESP_LOGI(TAG, "Next report in %lu sec", current_interval / 1000);
        }
        vTaskDelay(pdMS_TO_TICKS(current_interval));
    }
    
    ESP_LOGI(TAG, "WiFi location report task stopped");
    s_wifi_location_task_running = false;
    s_wifi_location_task_handle = NULL;
    
    /* 
     * 重要：HTTP 客户端使用 mbedTLS，它会在线程本地存储(TLS)中保存状态。
     * 直接使用 vTaskDelete(NULL) 会导致 TLSP 清理回调失败。
     * 
     * 解决方案：任务进入无限休眠，不删除自己。
     * 这样 TLS 资源会一直保留，直到系统重启。
     * 虽然会占用一些内存，但避免了崩溃。
     */
    ESP_LOGI(TAG, "Task entering idle state (not deleting to avoid TLSP crash)");
    
    /* 任务进入无限休眠 */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000));  /* 每分钟唤醒一次，检查是否需要退出 */
    }
}

int wifi_scan_access_points(wifi_ap_info_t *ap_list, int max_ap, uint32_t timeout_ms)
{
    if (ap_list == NULL || max_ap <= 0) {
        return -1;
    }

    ESP_LOGI(TAG, "Starting WiFi scan...");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300
            }
        }
    };

    esp_err_t ret = esp_wifi_scan_start(&scan_config, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(ret));
        return -1;
    }

    uint32_t elapsed_ms = 0;
    const uint32_t poll_interval_ms = 100;
    bool scan_done = false;

    while (elapsed_ms < timeout_ms) {
        vTaskDelay(pdMS_TO_TICKS(poll_interval_ms));
        elapsed_ms += poll_interval_ms;

        uint16_t temp_count = 0;
        ret = esp_wifi_scan_get_ap_num(&temp_count);
        if (ret == ESP_OK) {
            scan_done = true;
            break;
        }
    }

    if (!scan_done) {
        ESP_LOGW(TAG, "Scan timeout");
        return -1;
    }

    uint16_t scan_count = 0;
    esp_wifi_scan_get_ap_num(&scan_count);

    if (scan_count == 0) {
        return 0;
    }

    if (scan_count > max_ap) {
        scan_count = max_ap;
    }

    wifi_ap_record_t *ap_records = malloc(scan_count * sizeof(wifi_ap_record_t));
    if (ap_records == NULL) {
        return -1;
    }

    esp_wifi_scan_get_ap_records(&scan_count, ap_records);

    for (int i = 0; i < scan_count; i++) {
        /* 使用 snprintf 代替 strncpy 避免截断警告 */
        snprintf(ap_list[i].ssid, sizeof(ap_list[i].ssid), "%s", (char*)ap_records[i].ssid);
        snprintf(ap_list[i].bssid, sizeof(ap_list[i].bssid),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            ap_records[i].bssid[0], ap_records[i].bssid[1], ap_records[i].bssid[2],
            ap_records[i].bssid[3], ap_records[i].bssid[4], ap_records[i].bssid[5]);
        ap_list[i].rssi = ap_records[i].rssi;
        ap_list[i].channel = ap_records[i].primary;
        ap_list[i].authmode = ap_records[i].authmode;
    }

    free(ap_records);
    ESP_LOGI(TAG, "WiFi scan completed, found %d APs", scan_count);
    return scan_count;
}

esp_err_t wifi_get_connected_ap_info(wifi_ap_info_t *connected_ap)
{
    if (connected_ap == NULL) {
        return ESP_FAIL;
    }

    if (!wifi_is_connected()) {
        return ESP_FAIL;
    }

    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret != ESP_OK) {
        return ESP_FAIL;
    }

    /* 使用 snprintf 代替 strncpy 避免截断警告 */
    snprintf(connected_ap->ssid, sizeof(connected_ap->ssid), "%s", (char*)ap_info.ssid);
    snprintf(connected_ap->bssid, sizeof(connected_ap->bssid),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
        ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
    connected_ap->rssi = ap_info.rssi;
    connected_ap->channel = ap_info.primary;
    connected_ap->authmode = ap_info.authmode;

    return ESP_OK;
}

esp_err_t wifi_collect_location_data(wifi_location_data_t *location_data)
{
    if (location_data == NULL) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Collecting WiFi location data...");
    memset(location_data, 0, sizeof(wifi_location_data_t));

    /* 获取设备MAC地址 */
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        snprintf(location_data->smac, sizeof(location_data->smac),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        strcpy(location_data->smac, "00:00:00:00:00:00");
    }

    /* 获取网关IP */
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif != NULL) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            snprintf(location_data->serverip, sizeof(location_data->serverip),
                IPSTR, IP2STR(&ip_info.gw));
        } else {
            strcpy(location_data->serverip, "0.0.0.0");
        }
    } else {
        strcpy(location_data->serverip, "0.0.0.0");
    }

    /* 生成IMSI */
    uint8_t base_mac[6];
    uint64_t chip_id = 0;
    if (esp_wifi_get_mac(WIFI_IF_STA, base_mac) == ESP_OK) {
        for (int i = 0; i < 6; i++) {
            chip_id = (chip_id << 8) | base_mac[i];
        }
    } else {
        chip_id = esp_random();
    }
    snprintf(location_data->imsi, sizeof(location_data->imsi), "460%012llu", chip_id % 1000000000000ULL);

    /* 生成IDFA */
    uint32_t random1 = esp_random();
    uint32_t random2 = esp_random();
    snprintf(location_data->idfa, sizeof(location_data->idfa),
        "%08lX-%04X-%04X-%04X-%08lX%04X",
        (uint32_t)(chip_id >> 32),
        (uint16_t)(chip_id >> 16),
        (uint16_t)chip_id,
        (uint16_t)(random1 >> 16),
        random2,
        (uint16_t)random1);

    /* 获取当前连接的热点信息 */
    wifi_ap_info_t connected_ap;
    if (wifi_get_connected_ap_info(&connected_ap) == ESP_OK) {
        snprintf(location_data->mmac, sizeof(location_data->mmac),
            "%s,%d", connected_ap.bssid, connected_ap.rssi);
    }

    /* 扫描周围热点 */
    int ap_count = 0;
    wifi_ap_info_t *scan_results = (wifi_ap_info_t *)malloc(WIFI_SCAN_MAX_AP * sizeof(wifi_ap_info_t));
    if (scan_results != NULL) {
        int scan_count = wifi_scan_access_points(scan_results, WIFI_SCAN_MAX_AP, WIFI_SCAN_TIMEOUT_MS);
        if (scan_count > 0) {
            char *macs_ptr = location_data->macs;
            size_t remaining = sizeof(location_data->macs) - 1;

            for (int i = 0; i < scan_count && remaining > 0; i++) {
                int written = snprintf(macs_ptr, remaining, "%s%s,%d",
                    (i > 0) ? "|" : "", scan_results[i].bssid, scan_results[i].rssi);
                if (written > 0 && (size_t)written < remaining) {
                    macs_ptr += written;
                    remaining -= written;
                    ap_count++;
                } else {
                    break;
                }
            }
        }
        free(scan_results);
    }

    /* OneNET 要求至少2个AP才能定位 */
    if (ap_count < 2) {
        ESP_LOGW(TAG, "Only %d AP(s) found, need at least 2 for WiFi location", ap_count);
    }

    ESP_LOGI(TAG, "WiFi location data collected: %d APs", ap_count);
    return ESP_OK;
}

esp_err_t wifi_location_report_once(void)
{
    ESP_LOGI(TAG, "Starting WiFi location report...");

    wifi_location_data_t location_data;
    esp_err_t ret = wifi_collect_location_data(&location_data);
    if (ret != ESP_OK) {
        return ret;
    }

    return onenet_http_report_wifi_location(&location_data);
}

esp_err_t wifi_location_set_auto_report_interval(uint32_t interval_ms)
{
    s_wifi_location_report_interval = interval_ms;
    s_last_wifi_location_report_time = esp_timer_get_time() / 1000;
    ESP_LOGI(TAG, "WiFi location auto report interval: %lu ms", interval_ms);
    return ESP_OK;
}

void wifi_location_set_callback(location_callback_t callback)
{
    s_location_callback = callback;
}

esp_err_t wifi_location_get_last_result(location_info_t *location)
{
    if (location == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *location = s_last_location;
    return s_last_location.valid ? ESP_OK : ESP_ERR_NOT_FOUND;
}

void wifi_location_trigger_async(void)
{
    if (s_wifi_location_task_running) {
        ESP_LOGW(TAG, "WiFi location task already running");
        return;
    }

    /* 
     * 注意：不删除旧的休眠任务，因为删除会导致 TLSP 崩溃。
     * 每次调用都创建新任务，旧任务会一直休眠直到系统重启。
     * 这会占用一些内存，但避免了崩溃。
     */
    
    /* 创建单次上报任务（interval=0） */
    TaskHandle_t new_task = NULL;
    BaseType_t ret = xTaskCreate(wifi_location_report_task, "wifi_loc", 12288, (void *)0, 5, &new_task);
    if (ret == pdPASS) {
        s_wifi_location_task_handle = new_task;
    } else {
        ESP_LOGE(TAG, "Failed to create WiFi location task");
    }
}

esp_err_t wifi_location_start_periodic_report(uint32_t interval_ms)
{
    if (s_wifi_location_task_running) {
        ESP_LOGW(TAG, "WiFi location task already running");
        return ESP_OK;  /* 已经在运行，直接返回成功 */
    }
    
    if (interval_ms < 1000) {
        interval_ms = 5000;  /* 最小间隔1秒，默认5秒 */
    }
    
    ESP_LOGI(TAG, "Starting periodic WiFi location report, interval=%lu ms", interval_ms);
    
    /* 
     * 注意：不删除旧的休眠任务，因为删除会导致 TLSP 崩溃。
     * 每次调用都创建新任务。
     */
    TaskHandle_t new_task = NULL;
    BaseType_t ret = xTaskCreate(
        wifi_location_report_task, 
        "wifi_loc_periodic", 
        12288, 
        (void *)(uintptr_t)interval_ms,  /* 传递间隔时间 */
        5, 
        &new_task
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create periodic location report task");
        return ESP_FAIL;
    }
    
    s_wifi_location_task_handle = new_task;
    s_wifi_location_report_interval = interval_ms;
    return ESP_OK;
}

void wifi_location_stop_periodic_report(void)
{
    if (!s_wifi_location_task_running) {
        ESP_LOGI(TAG, "WiFi location task not running");
        return;
    }
    
    ESP_LOGI(TAG, "Stopping periodic WiFi location report...");
    s_wifi_location_task_running = false;  /* 设置标志，任务会自行退出 */
    s_wifi_location_report_interval = 0;
}

bool wifi_location_is_reporting(void)
{
    return s_wifi_location_task_running;
}

esp_err_t wifi_location_set_report_interval(uint32_t interval_ms)
{
    /* 不限制最小间隔，支持毫秒级上报 */
    s_wifi_location_report_interval = interval_ms;
    if (interval_ms < 1000) {
        ESP_LOGI(TAG, "WiFi location report interval set to %lu ms", interval_ms);
    } else {
        ESP_LOGI(TAG, "WiFi location report interval set to %lu sec", interval_ms / 1000);
    }
    
    return ESP_OK;
}

uint32_t wifi_location_get_report_interval(void)
{
    return s_wifi_location_report_interval;
}

/* ==================== 示波器数据批量上报 ==================== */

#define OSCILLOSCOPE_BUFFER_SIZE 1024
static float s_oscilloscope_buffer[OSCILLOSCOPE_BUFFER_SIZE];
static uint32_t s_oscilloscope_count = 0;
static int64_t s_oscilloscope_timestamps[OSCILLOSCOPE_BUFFER_SIZE];

esp_err_t onenet_report_oscilloscope_data(float voltage)
{
    /* 限制电压范围 -50.00 ~ 50.00 */
    if (voltage < -50.0f) voltage = -50.0f;
    if (voltage > 50.0f) voltage = 50.0f;
    
    /* 存储到缓冲区 */
    s_oscilloscope_buffer[s_oscilloscope_count] = voltage;
    s_oscilloscope_timestamps[s_oscilloscope_count] = (int64_t)time(NULL) * 1000;  /* 毫秒时间戳 */
    s_oscilloscope_count++;
    
    ESP_LOGD(TAG, "Oscilloscope data buffered: %.2f V (%lu/%d)", 
             voltage, s_oscilloscope_count, OSCILLOSCOPE_BUFFER_SIZE);
    
    /* 缓冲区满时自动上报 */
    if (s_oscilloscope_count >= OSCILLOSCOPE_BUFFER_SIZE) {
        return onenet_report_oscilloscope_batch();
    }
    
    return ESP_OK;
}

uint32_t onenet_get_oscilloscope_buffer_count(void)
{
    return s_oscilloscope_count;
}

esp_err_t onenet_report_oscilloscope_batch(void)
{
    if (s_oscilloscope_count == 0) {
        ESP_LOGW(TAG, "No oscilloscope data to report");
        return ESP_OK;
    }
    
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot report batch data");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }
    
    /* 获取设备凭证 */
    char product_id[32] = {0};
    char device_name[64] = {0};
    char sec_key[64] = {0};
    
    esp_err_t ret = cloud_manager_get_device_credentials(product_id, sizeof(product_id),
                                                          device_name, sizeof(device_name),
                                                          sec_key, sizeof(sec_key));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device credentials");
        return ret;
    }
    
    if (strlen(sec_key) == 0) {
        ESP_LOGE(TAG, "sec_key is empty");
        return ESP_ERR_INVALID_STATE;
    }
    
    /* 生成Token */
    char auth_token[512];
    ret = generate_device_token(product_id, device_name, sec_key, auth_token, sizeof(auth_token));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate token");
        return ret;
    }
    
    ESP_LOGI(TAG, "Reporting %lu oscilloscope data points in batch...", s_oscilloscope_count);
    
    /* 
     * 构造批量上报 OneJSON 格式
     * POST /device/thing/pack/post?topic=$sys/{pid}/{device-name}/thing/pack/post&protocol=http
     * {
     *   "id": "123",
     *   "version": "1.0",
     *   "params": {
     *     "properties": {
     *       "Oscilloscope_data": {
     *         "value": [v1, v2, ...],  // 或者多次上报
     *         "time": timestamp
     *       }
     *     }
     *   }
     * }
     * 
     * 注意: OneNET 单个属性不支持数组，需要逐个上报或使用历史数据接口
     * 这里使用最后一个值作为当前值上报，同时记录数据点数量
     */
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    char msg_id[16];
    snprintf(msg_id, sizeof(msg_id), "%lu", (unsigned long)(esp_timer_get_time() / 1000));
    cJSON_AddStringToObject(root, "id", msg_id);
    cJSON_AddStringToObject(root, "version", "1.0");
    
    cJSON *params = cJSON_CreateObject();
    cJSON *properties = cJSON_CreateObject();
    
    /* 使用最后一个数据点的值和时间戳 */
    cJSON *osc_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(osc_data, "value", s_oscilloscope_buffer[s_oscilloscope_count - 1]);
    cJSON_AddNumberToObject(osc_data, "time", (double)s_oscilloscope_timestamps[s_oscilloscope_count - 1]);
    
    cJSON_AddItemToObject(properties, "Oscilloscope_data", osc_data);
    cJSON_AddItemToObject(params, "properties", properties);
    cJSON_AddItemToObject(root, "params", params);
    
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (body == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Batch report body: %s", body);
    
    /* 清空响应缓冲区 */
    if (s_http_response_buffer != NULL) {
        free(s_http_response_buffer);
        s_http_response_buffer = NULL;
    }
    s_http_response_length = 0;
    
    /* 构造 topic (URL编码) */
    char topic_encoded[256];
    char *p = topic_encoded;
    char topic[128];
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/pack/post", product_id, device_name);
    
    for (const char *s = topic; *s && (p - topic_encoded) < (int)sizeof(topic_encoded) - 4; s++) {
        if (*s == '$') {
            *p++ = '%'; *p++ = '2'; *p++ = '4';
        } else if (*s == '/') {
            *p++ = '%'; *p++ = '2'; *p++ = 'F';
        } else {
            *p++ = *s;
        }
    }
    *p = '\0';
    
    /* 构造完整URL */
    char url[512];
    snprintf(url, sizeof(url), 
             "https://%s%s?topic=%s&protocol=HTTP",
             ONENET_HTTP_API_HOST, "/fuse/http/device/thing/pack/post", topic_encoded);
    
    ESP_LOGI(TAG, "Batch report URL: %s", url);
    
    /* 配置HTTP客户端 */
    esp_http_client_config_t config = {
        .url = url,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        free(body);
        return ESP_FAIL;
    }
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "token", auth_token);
    esp_http_client_set_post_field(client, body, strlen(body));
    
    ret = esp_http_client_perform(client);
    
    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Batch report HTTP Status = %d", status_code);
        
        if (s_http_response_buffer != NULL && s_http_response_length > 0) {
            ESP_LOGI(TAG, "Response: %s", s_http_response_buffer);
            
            cJSON *resp = cJSON_Parse(s_http_response_buffer);
            if (resp != NULL) {
                cJSON *errno_item = cJSON_GetObjectItem(resp, "errno");
                cJSON *error_item = cJSON_GetObjectItem(resp, "error");
                
                bool success = false;
                if (cJSON_IsNumber(errno_item) && errno_item->valueint == 0) {
                    success = true;
                }
                if (cJSON_IsString(error_item) && strcmp(error_item->valuestring, "succ") == 0) {
                    success = true;
                }
                
                if (success) {
                    ESP_LOGI(TAG, "Batch report success! %lu data points uploaded", s_oscilloscope_count);
                    s_oscilloscope_count = 0;  /* 清空缓冲区 */
                    ret = ESP_OK;
                } else {
                    const char *err_msg = cJSON_IsString(error_item) ? error_item->valuestring : "unknown";
                    ESP_LOGE(TAG, "Batch report failed: %s", err_msg);
                    ret = ESP_FAIL;
                }
                cJSON_Delete(resp);
            }
            
            free(s_http_response_buffer);
            s_http_response_buffer = NULL;
            s_http_response_length = 0;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(ret));
    }
    
    esp_http_client_cleanup(client);
    free(body);
    
    return ret;
}

/* ==================== 时间同步功能实现 ==================== */

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "SNTP time synchronized!");

    setenv("TZ", "CST-8", 1);
    tzset();

    s_time_synced = true;
    s_last_synced_time = tv->tv_sec;
    s_last_sync_tick = esp_timer_get_time() / 1000000;

    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", strftime_buf);

    nvs_save_time();
}

static void initialize_sntp(void)
{
    if (s_sntp_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Initializing SNTP...");

    setenv("TZ", "CST-8", 1);
    tzset();

    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "ntp1.aliyun.com");
    esp_sntp_setservername(2, "pool.ntp.org");
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    esp_sntp_init();

    s_sntp_initialized = true;
    ESP_LOGI(TAG, "SNTP initialized");
}

esp_err_t onenet_sync_time(void)
{
    if (s_time_synced) {
        return ESP_OK;
    }

    if (!s_sntp_initialized) {
        initialize_sntp();
    }

    return ESP_OK;
}

void wifi_manager_init_sntp(void)
{
    ESP_LOGI(TAG, "WiFi manager triggered SNTP initialization");

    if (s_sntp_initialized) {
        sntp_restart();
    } else {
        initialize_sntp();
    }
}

esp_err_t onenet_save_time_to_nvs(void)
{
    return nvs_save_time();
}

esp_err_t onenet_restore_time_from_nvs(void)
{
    return nvs_restore_time();
}

esp_err_t onenet_get_time_string(char *time_str, size_t buf_size, const char *format)
{
    if (time_str == NULL || buf_size == 0 || format == NULL) {
        return ESP_FAIL;
    }

    time_t now;
    struct tm timeinfo;

    if (s_time_synced) {
        int64_t current_tick = esp_timer_get_time() / 1000000;
        int64_t elapsed = current_tick - s_last_sync_tick;
        now = s_last_synced_time + elapsed;
    } else {
        now = time(NULL);
        if (now < 1640995200) {
            struct tm default_time = {
                .tm_year = 2025 - 1900,
                .tm_mon = 0,
                .tm_mday = 1,
                .tm_hour = 0,
                .tm_min = 0,
                .tm_sec = 0
            };
            now = mktime(&default_time) + (esp_timer_get_time() / 1000000);
        }
    }

    localtime_r(&now, &timeinfo);
    strftime(time_str, buf_size, format, &timeinfo);

    return ESP_OK;
}

bool onenet_is_time_synced(void)
{
    return s_time_synced;
}

esp_err_t onenet_generate_device_token(char *token_buf, size_t buf_size)
{
    if (token_buf == NULL || buf_size < 256) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* 从 cloud_manager 获取设备凭证 */
    char product_id[32] = {0};
    char device_name[64] = {0};
    char sec_key[128] = {0};
    
    esp_err_t ret = cloud_manager_get_device_credentials(product_id, sizeof(product_id),
                                                          device_name, sizeof(device_name),
                                                          sec_key, sizeof(sec_key));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device credentials");
        return ret;
    }
    
    /* 使用设备凭证生成 token */
    return generate_device_token(product_id, device_name, sec_key, token_buf, buf_size);
}

/* ==================== 处理函数 ==================== */

static void process_wifi_state_change(void)
{
    if (s_wifi_state_changed && s_wifi_callback) {
        s_wifi_state_changed = false;
        bool connected = (s_wifi_state == WIFI_STATE_CONNECTED);
        s_wifi_callback(connected);
    }
}

static void process_wifi_location_auto_report(void)
{
    /* 如果周期性任务正在运行，不使用主循环触发 */
    if (s_wifi_location_task_running) {
        return;
    }
    
    if (s_wifi_location_report_interval == 0) {
        return;
    }

    /* 使用HTTP状态判断 */
    if (onenet_http_get_state() != ONENET_STATE_ONLINE) {
        return;
    }

    if (!wifi_is_connected()) {
        return;
    }

    int64_t current_time = esp_timer_get_time() / 1000;
    if ((current_time - s_last_wifi_location_report_time) >= s_wifi_location_report_interval) {
        ESP_LOGI(TAG, "Auto WiFi location report triggered (main loop)");
        wifi_location_report_once();
        s_last_wifi_location_report_time = current_time;
    }
}

void onenet_process(void)
{
    process_wifi_state_change();
    process_wifi_location_auto_report();
}

/* ==================== 便捷函数实现 ==================== */

esp_err_t wifi_onenet_init(wifi_status_callback_t wifi_callback)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Starting WiFi and OneNET HTTP initialization...");

    /* 尝试从NVS恢复时间 */
    ret = nvs_restore_time();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Time restored from NVS");
    }

    ret = onenet_wifi_init(wifi_callback);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = onenet_http_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "WiFi and OneNET HTTP initialization completed");
    return ESP_OK;
}

void wifi_onenet_status_info(void)
{
    ESP_LOGI(TAG, "==================== WiFi & OneNET Status ====================");

    ESP_LOGI(TAG, "WiFi Status:");
    ESP_LOGI(TAG, "  - Connected: %s", wifi_is_connected() ? "YES" : "NO");
    ESP_LOGI(TAG, "  - Retry Count: %d/%d", s_retry_num, WIFI_MAXIMUM_RETRY);

    const char* state_str;
    onenet_state_t state = onenet_http_get_state();
    switch (state) {
        case ONENET_STATE_UNKNOWN: state_str = "UNKNOWN"; break;
        case ONENET_STATE_INIT: state_str = "INITIALIZED"; break;
        case ONENET_STATE_ONLINE: state_str = "ONLINE"; break;
        case ONENET_STATE_OFFLINE: state_str = "OFFLINE"; break;
        case ONENET_STATE_ERROR: state_str = "ERROR"; break;
        default: state_str = "INVALID"; break;
    }

    ESP_LOGI(TAG, "OneNET HTTP Status:");
    ESP_LOGI(TAG, "  - State: %s", state_str);
    ESP_LOGI(TAG, "  - API Host: %s:%d", ONENET_HTTP_API_HOST, ONENET_HTTP_API_PORT);
    ESP_LOGI(TAG, "  - Product ID: %s", ONENET_HTTP_PRODUCT_ID);

    ESP_LOGI(TAG, "Features:");
    ESP_LOGI(TAG, "  - WiFi Location: Enabled (HTTP)");
    ESP_LOGI(TAG, "  - Time Sync: Enabled (SNTP)");
    ESP_LOGI(TAG, "  - Protocol: HTTP (MQTT removed)");

    ESP_LOGI(TAG, "System Info:");
    ESP_LOGI(TAG, "  - Free Heap: %u bytes", (unsigned int)esp_get_free_heap_size());
    ESP_LOGI(TAG, "  - Uptime: %lld ms", esp_timer_get_time() / 1000);

    ESP_LOGI(TAG, "============================================================");
}
