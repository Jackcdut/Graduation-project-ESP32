/**
 * @file cloud_activation_server.c
 * @brief ESP32 AP Hotspot and HTTP Server for Device Activation
 */

#include "cloud_activation_server.h"
#include "wifi_onenet.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "mbedtls/md.h"
#include "mbedtls/base64.h"
#include "esp_crt_bundle.h"  /* For certificate bundle verification */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "ACTIVATION_SERVER";

/* Use ONENET_HTTP_PRODUCT_ID and ONENET_ACCESS_KEY from wifi_onenet.h */

/* Static variables */
static bool s_server_running = false;
static bool s_operation_in_progress = false;  /* Prevent concurrent start/stop operations */
static SemaphoreHandle_t s_server_mutex = NULL;  /* Mutex to protect server state */
static httpd_handle_t s_http_server = NULL;
static activation_status_t s_status = ACTIVATION_STATUS_IDLE;
static activation_status_cb_t s_status_cb = NULL;
static char s_device_code[20] = {0};
static char s_ap_ip[16] = "Not Connected";
static activation_result_t s_result = {0};

/* Static buffer for HTTP response data collection */
static char *s_http_response_buffer = NULL;
static int s_http_response_length = 0;
static int s_http_response_capacity = 0;

/* HTTP event handler to collect response data */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                /* Not chunked - collect data */
                int data_len = evt->data_len;
                if (s_http_response_buffer == NULL) {
                    /* Allocate buffer */
                    s_http_response_capacity = data_len + 1;
                    s_http_response_buffer = malloc(s_http_response_capacity);
                    if (s_http_response_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate response buffer");
                        return ESP_FAIL;
                    }
                    s_http_response_length = 0;
                } else if (s_http_response_length + data_len + 1 > s_http_response_capacity) {
                    /* Reallocate buffer */
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

/* Forward declarations */
static esp_err_t start_softap(void);
static esp_err_t stop_softap(void);
static esp_err_t start_http_server(void);
static esp_err_t stop_http_server(void);
static esp_err_t register_device_onenet(const char *device_name, const char *desc,
                                        const char *lon, const char *lat);

/* HTML page for activation - 美化设计，浅蓝色背景，显示产品ID和设备ID */
static const char ACTIVATION_HTML[] = 
"<!DOCTYPE html>"
"<html><head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">"
"<title>ExDebugTool Activation</title>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:linear-gradient(135deg,#e0f7fa 0%%,#b2ebf2 100%%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}"
".card{background:#fff;border-radius:16px;padding:25px;max-width:360px;width:100%%;box-shadow:0 8px 32px rgba(0,150,200,0.15)}"
".logo{text-align:center;margin-bottom:15px}"
".logo h1{color:#0288d1;font-size:22px;font-weight:700}"
".logo p{color:#666;font-size:12px;margin-top:5px}"
".info-box{background:#f5f9fc;border-radius:10px;padding:12px;margin-bottom:15px}"
".info-row{display:flex;justify-content:space-between;padding:6px 0;font-size:13px}"
".info-row label{color:#666}"
".info-row span{color:#333;font-weight:600;font-family:monospace}"
".btn{width:100%%;padding:14px;background:linear-gradient(135deg,#0288d1,#03a9f4);color:#fff;border:none;border-radius:10px;font-size:15px;font-weight:600;cursor:pointer;transition:all 0.3s}"
".btn:hover{transform:translateY(-2px);box-shadow:0 4px 12px rgba(2,136,209,0.4)}"
".btn:disabled{background:#ccc;transform:none;box-shadow:none;cursor:not-allowed}"
".status{margin-top:12px;padding:10px;border-radius:8px;text-align:center;font-size:13px;display:none}"
".status.info{display:block;background:#e3f2fd;color:#1565c0}"
".status.success{display:block;background:#e8f5e9;color:#2e7d32}"
".status.error{display:block;background:#ffebee;color:#c62828}"
".result{margin-top:12px;display:none;background:#f0f9ff;border-radius:10px;padding:12px;border:1px solid #b3e5fc}"
".result.show{display:block}"
".result-item{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #e0e0e0;font-size:13px}"
".result-item:last-child{border-bottom:none}"
".result-item label{color:#666}"
".result-item span{color:#0288d1;font-weight:600;font-family:monospace}"
"</style>"
"</head>"
"<body>"
"<div class=\"card\">"
"<div class=\"logo\">"
"<h1>ExDebugTool</h1>"
"<p>Device Activation</p>"
"</div>"
"<div class=\"info-box\">"
"<div class=\"info-row\"><label>Product ID</label><span>" ONENET_HTTP_PRODUCT_ID "</span></div>"
"<div class=\"info-row\"><label>Device Name</label><span>%s</span></div>"
"</div>"
"<button class=\"btn\" id=\"btn\" onclick=\"activate()\">Activate Device</button>"
"<div class=\"status\" id=\"status\"></div>"
"<div class=\"result\" id=\"result\">"
"<div class=\"result-item\"><label>Product ID</label><span id=\"rpid\">-</span></div>"
"<div class=\"result-item\"><label>Device ID</label><span id=\"rid\">-</span></div>"
"<div class=\"result-item\"><label>Device Name</label><span id=\"rname\">-</span></div>"
"<div class=\"result-item\"><label>Status</label><span id=\"rstat\" style=\"color:#4caf50\">-</span></div>"
"</div>"
"</div>"
"<script>"
"var btn=document.getElementById('btn');"
"var st=document.getElementById('status');"
"function show(m,t){st.className='status '+t;st.innerHTML=m;}"
"async function activate(){"
"btn.disabled=true;btn.innerHTML='Activating...';"
"show('Registering device on OneNet...','info');"
"try{"
"var r=await fetch('/activate',{method:'POST',headers:{'Content-Type':'application/json'},body:'{}'});"
"var d=await r.json();"
"if(d.success){"
"show('Device activated successfully!','success');"
"document.getElementById('rpid').textContent=d.product_id||'" ONENET_HTTP_PRODUCT_ID "';"
"document.getElementById('rid').textContent=d.device_id;"
"document.getElementById('rname').textContent=d.device_name;"
"document.getElementById('rstat').textContent='Online';"
"document.getElementById('result').classList.add('show');"
"btn.innerHTML='Activated!';"
"}else{show('Error: '+d.error,'error');btn.disabled=false;btn.innerHTML='Retry';}"
"}catch(e){show('Network Error: '+e.message,'error');btn.disabled=false;btn.innerHTML='Retry';}"
"}"
"</script>"
"</body>"
"</html>";

/* Update status and notify callback */
static void update_status(activation_status_t new_status)
{
    s_status = new_status;
    if (s_status_cb) {
        s_status_cb(new_status, &s_result);
    }
}

/* HTTP GET handler for root page - optimized to avoid header length issues */
static esp_err_t http_get_handler(httpd_req_t *req)
{
    size_t html_len = strlen(ACTIVATION_HTML) + strlen(s_device_code) + 1;
    char *html_buf = malloc(html_len);
    if (html_buf == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    snprintf(html_buf, html_len, ACTIVATION_HTML, s_device_code);
    
    size_t html_size = strlen(html_buf);
    ESP_LOGI(TAG, "Sending HTML response, length: %d", html_size);
    
    /* Use absolute minimal response headers to avoid "Header fields too long" error */
    /* Only set Content-Type, let ESP-IDF handle other headers automatically */
    httpd_resp_set_type(req, "text/html");
    
    /* Do NOT set any additional headers (Connection, Cache-Control, etc.) */
    /* This minimizes response header size */
    
    /* Use chunked transfer encoding to send response in small pieces */
    /* This avoids any buffer size limitations */
    esp_err_t ret = ESP_OK;
    
    /* Send in small chunks to avoid any buffer overflow */
    const size_t chunk_size = 256;
    size_t remaining = html_size;
    const char *pos = html_buf;
    
    while (remaining > 0 && ret == ESP_OK) {
        size_t to_send = (remaining > chunk_size) ? chunk_size : remaining;
        ret = httpd_resp_send_chunk(req, pos, to_send);
        if (ret == ESP_OK) {
            pos += to_send;
            remaining -= to_send;
        } else {
            ESP_LOGE(TAG, "Failed to send chunk at offset %d: %s", 
                     (int)(pos - html_buf), esp_err_to_name(ret));
            break;
        }
    }
    
    /* Send final empty chunk to indicate end of response */
    if (ret == ESP_OK) {
        ret = httpd_resp_send_chunk(req, NULL, 0);
    }
    
    free(html_buf);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "HTML response sent successfully");
    return ESP_OK;
}

/* HTTP POST handler for activation - with proper error handling */
static esp_err_t http_activate_handler(httpd_req_t *req)
{
    /* Read request body (can be empty) */
    char buf[128] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret < 0 && ret != HTTPD_SOCK_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Failed to receive request body: %d", ret);
    }
    
    ESP_LOGI(TAG, "Activation request received");
    
    update_status(ACTIVATION_STATUS_REGISTERING);
    
    /* Note: In APSTA mode, the device can have both AP and STA active.
     * The STA interface should be connected to a router with internet access
     * for OneNet API calls to work. We don't block here, but let the HTTP
     * client attempt the connection and report the actual error.
     */
    
    /* Generate unique device name with incrementing counter (ED001, ED002, ...) */
    static const char *NVS_NAMESPACE = "cloud_mgr";
    static const char *NVS_KEY_DEVICE_COUNTER = "dev_counter";
    
    nvs_handle_t nvs_handle;
    uint32_t device_counter = 1;  /* Start from 1 */
    
    esp_err_t nvs_ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (nvs_ret == ESP_OK) {
        /* Try to read existing counter */
        nvs_get_u32(nvs_handle, NVS_KEY_DEVICE_COUNTER, &device_counter);
        /* Increment counter */
        device_counter++;
        /* Save incremented counter */
        nvs_set_u32(nvs_handle, NVS_KEY_DEVICE_COUNTER, device_counter);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    } else {
        /* If NVS open failed, create new namespace and start from 1 */
        nvs_ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (nvs_ret == ESP_OK) {
            nvs_set_u32(nvs_handle, NVS_KEY_DEVICE_COUNTER, device_counter);
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
        }
    }
    
    char device_name[48];
    snprintf(device_name, sizeof(device_name), "ExDebugTool_ED%03d", device_counter);
    
    /* Register device on OneNet */
    ESP_LOGI(TAG, "Registering on OneNet: %s (counter: %lu)", device_name, (unsigned long)device_counter);
    
    esp_err_t reg_ret = register_device_onenet(device_name, "ESP32-P4 Debug Tool", "116.397", "39.916");
    
    char response[512];
    esp_err_t http_ret = ESP_OK;
    
    if (reg_ret == ESP_OK) {
        s_result.success = true;
        /* Device code should be generated from OneNet device ID, not pre-generated */
        /* Use device_id as device_code (or format it if needed) */
        if (strlen(s_result.device_id) > 0) {
            snprintf(s_result.device_code, sizeof(s_result.device_code), "%s", s_result.device_id);
        } else {
            /* Fallback: use pre-generated code if device_id is empty */
            snprintf(s_result.device_code, sizeof(s_result.device_code), "%s", s_device_code);
        }
        
        update_status(ACTIVATION_STATUS_SUCCESS);
        
        snprintf(response, sizeof(response),
                 "{\"success\":true,\"product_id\":\"%s\",\"device_id\":\"%s\",\"device_name\":\"%s\"}",
                 s_result.product_id, s_result.device_id, s_result.device_name);
        
        ESP_LOGI(TAG, "Activation successful! ProductID=%s, DeviceID=%s, Name=%s", 
                 s_result.product_id, s_result.device_id, s_result.device_name);
    } else {
        s_result.success = false;
        update_status(ACTIVATION_STATUS_FAILED);
        
        /* Get detailed error message */
        const char *err_msg = s_result.error_msg[0] ? s_result.error_msg : "Registration failed";
        
        /* Escape JSON special characters */
        char safe_error[256] = {0};
        const char *p = err_msg;
        char *q = safe_error;
        size_t remaining = sizeof(safe_error) - 1;
        
        while (*p && remaining > 1) {
            if (*p == '"') {
                if (remaining > 2) { *q++ = '\\'; *q++ = '"'; remaining -= 2; }
            } else if (*p == '\\') {
                if (remaining > 2) { *q++ = '\\'; *q++ = '\\'; remaining -= 2; }
            } else if (*p == '\n') {
                if (remaining > 2) { *q++ = '\\'; *q++ = 'n'; remaining -= 2; }
            } else {
                *q++ = *p;
                remaining--;
            }
            p++;
        }
        *q = '\0';
        
        snprintf(response, sizeof(response),
                 "{\"success\":false,\"error\":\"%s\"}", safe_error);
        
        ESP_LOGE(TAG, "Activation failed: %s", err_msg);
    }
    
    /* Set minimal response headers */
    httpd_resp_set_type(req, "application/json");
    
    /* Send response - use sendstr which handles string properly */
    http_ret = httpd_resp_sendstr(req, response);
    
    if (http_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send activation response: %s", esp_err_to_name(http_ret));
        return http_ret;
    }
    
    ESP_LOGI(TAG, "Activation response sent successfully");
    return ESP_OK;
}

/* Generate OneNet API Token */
esp_err_t cloud_activation_generate_token(const char *product_id, const char *access_key,
                                          char *token_buf, size_t buf_size)
{
    /* Token format: version=2018-10-31&res=products/{pid}&et={expire_time}&method=md5&sign={signature} */
    time_t now;
    time(&now);
    time_t expire_time = now + 365 * 24 * 3600; /* 1 year */
    
    /* Build string to sign */
    char string_to_sign[256];
    snprintf(string_to_sign, sizeof(string_to_sign),
             "%lld\nmd5\nproducts/%s\n2018-10-31",
             (long long)expire_time, product_id);
    
    /* Calculate HMAC-MD5 */
    uint8_t hmac[16];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), 1);
    
    /* Decode base64 access key first */
    uint8_t key_decoded[64];
    size_t key_decoded_len = 0;
    mbedtls_base64_decode(key_decoded, sizeof(key_decoded), &key_decoded_len,
                          (const uint8_t *)access_key, strlen(access_key));
    
    mbedtls_md_hmac_starts(&ctx, key_decoded, key_decoded_len);
    mbedtls_md_hmac_update(&ctx, (const uint8_t *)string_to_sign, strlen(string_to_sign));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);
    
    /* Base64 encode signature */
    char sign_base64[64];
    size_t sign_len = 0;
    mbedtls_base64_encode((uint8_t *)sign_base64, sizeof(sign_base64), &sign_len, hmac, 16);
    sign_base64[sign_len] = '\0';
    
    /* URL encode signature */
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
    
    /* Build final token */
    snprintf(token_buf, buf_size,
             "version=2018-10-31&res=products%%2F%s&et=%lld&method=md5&sign=%s",
             product_id, (long long)expire_time, sign_encoded);
    
    return ESP_OK;
}

/* Register device on OneNet via HTTP API */
static esp_err_t register_device_onenet(const char *device_name, const char *desc,
                                        const char *lon, const char *lat)
{
    /* Build request body */
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "product_id", ONENET_HTTP_PRODUCT_ID);
    cJSON_AddStringToObject(body, "device_name", device_name);
    cJSON_AddStringToObject(body, "desc", desc);
    cJSON_AddStringToObject(body, "lon", lon);
    cJSON_AddStringToObject(body, "lat", lat);
    
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    
    if (body_str == NULL) {
        snprintf(s_result.error_msg, sizeof(s_result.error_msg), "JSON creation failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Request body: %s", body_str);
    
    /* Generate authorization token using HTTP product credentials */
    char auth_token[256];
    cloud_activation_generate_token(ONENET_HTTP_PRODUCT_ID, ONENET_ACCESS_KEY, auth_token, sizeof(auth_token));
    
    /* Configure HTTP client with TLS certificate verification and event handler */
    esp_http_client_config_t config = {
        .host = ONENET_API_HOST,
        .port = ONENET_API_PORT,
        .path = ONENET_API_CREATE_DEVICE,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,  /* Increased timeout for SSL handshake */
        .crt_bundle_attach = esp_crt_bundle_attach,  /* Enable certificate bundle verification */
        .event_handler = http_event_handler,  /* Use event handler to collect response */
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        free(body_str);
        snprintf(s_result.error_msg, sizeof(s_result.error_msg), "HTTP client init failed");
        return ESP_FAIL;
    }
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "authorization", auth_token);
    esp_http_client_set_post_field(client, body_str, strlen(body_str));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err != ESP_OK) {
        int error_code = esp_http_client_get_errno(client);
        ESP_LOGE(TAG, "HTTP request failed: %s (errno: %d)", esp_err_to_name(err), error_code);
        
        /* Provide more detailed error message */
        char detailed_error[128];
        if (err == ESP_ERR_HTTP_CONNECT) {
            snprintf(detailed_error, sizeof(detailed_error), "Connection failed. Check internet connection.");
        } else if (err == ESP_ERR_HTTP_FETCH_HEADER) {
            snprintf(detailed_error, sizeof(detailed_error), "Failed to fetch response header.");
        } else if (error_code == 113) {  /* EHOSTUNREACH */
            snprintf(detailed_error, sizeof(detailed_error), "Host unreachable. Check network connection.");
        } else if (error_code == 110) {  /* ETIMEDOUT */
            snprintf(detailed_error, sizeof(detailed_error), "Request timeout. Server may be unreachable.");
        } else {
            snprintf(detailed_error, sizeof(detailed_error), "HTTP request failed: %s (errno: %d)", 
                     esp_err_to_name(err), error_code);
        }
        
        snprintf(s_result.error_msg, sizeof(s_result.error_msg), "%s", detailed_error);
        free(body_str);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    int status_code = esp_http_client_get_status_code(client);
    int content_length = esp_http_client_get_content_length(client);
    
    ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d", status_code, content_length);
    
    free(body_str);
    esp_http_client_cleanup(client);
    
    /* Check if we collected response data via event handler */
    if (s_http_response_buffer == NULL || s_http_response_length == 0) {
        ESP_LOGE(TAG, "No response data collected");
        if (s_http_response_buffer) {
            free(s_http_response_buffer);
            s_http_response_buffer = NULL;
        }
        snprintf(s_result.error_msg, sizeof(s_result.error_msg), "No response data received");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Response length: %d bytes", s_http_response_length);
    ESP_LOGI(TAG, "Response: %s", s_http_response_buffer);
    
    /* Parse response */
    cJSON *resp_json = cJSON_Parse(s_http_response_buffer);
    
    /* Debug: Print parsed JSON structure */
    if (resp_json != NULL) {
        char *debug_json = cJSON_Print(resp_json);
        if (debug_json != NULL) {
            ESP_LOGI(TAG, "Parsed JSON structure: %s", debug_json);
            free(debug_json);
        }
    }
    if (resp_json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON. Response: %s", s_http_response_buffer);
        /* Try to get cJSON error if available */
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "JSON parse error before: %s", error_ptr);
        }
        free(s_http_response_buffer);
        s_http_response_buffer = NULL;
        snprintf(s_result.error_msg, sizeof(s_result.error_msg), "Response parse failed");
        return ESP_FAIL;
    }
    
    /* Free response buffer after parsing */
    free(s_http_response_buffer);
    s_http_response_buffer = NULL;
    
    cJSON *code = cJSON_GetObjectItem(resp_json, "code");
    if (!cJSON_IsNumber(code) || code->valueint != 0) {
        cJSON *msg = cJSON_GetObjectItem(resp_json, "msg");
        if (cJSON_IsString(msg)) {
            snprintf(s_result.error_msg, sizeof(s_result.error_msg), "API error: %s", msg->valuestring);
        } else {
            snprintf(s_result.error_msg, sizeof(s_result.error_msg), "API error");
        }
        cJSON_Delete(resp_json);
        return ESP_FAIL;
    }
    
    /* Extract device info from response */
    cJSON *data = cJSON_GetObjectItem(resp_json, "data");
    if (data) {
        cJSON *did = cJSON_GetObjectItem(data, "did");
        cJSON *pid = cJSON_GetObjectItem(data, "pid");  /* 产品ID */
        cJSON *name = cJSON_GetObjectItem(data, "name");
        cJSON *sec_key = cJSON_GetObjectItem(data, "sec_key");
        
        /* 解析产品ID (pid) - 字符串格式 */
        if (pid != NULL && cJSON_IsString(pid) && pid->valuestring != NULL) {
            snprintf(s_result.product_id, sizeof(s_result.product_id), "%s", pid->valuestring);
            ESP_LOGI(TAG, "Product ID (pid): %s", s_result.product_id);
        } else {
            /* 如果API没有返回pid，使用配置的产品ID */
            snprintf(s_result.product_id, sizeof(s_result.product_id), "%s", ONENET_HTTP_PRODUCT_ID);
            ESP_LOGW(TAG, "Product ID not in response, using configured: %s", s_result.product_id);
        }
        
        /* 解析设备ID (did) - 可能是字符串或数字格式 */
        /* did = 设备唯一标识符，每个设备不同 */
        if (did != NULL) {
            ESP_LOGI(TAG, "did field type: %d (string=%d, number=%d)", 
                     did->type, cJSON_String, cJSON_Number);
            
            if (cJSON_IsString(did) && did->valuestring != NULL) {
                /* 字符串格式 - MQTTS/LwM2M 老产品 */
                snprintf(s_result.device_id, sizeof(s_result.device_id), "%s", did->valuestring);
                ESP_LOGI(TAG, "Device ID (string): %s", s_result.device_id);
            } else if (cJSON_IsNumber(did)) {
                /* 数字格式 - 新产品，使用 cJSON_PrintUnformatted 保持精度 */
                char *did_str = cJSON_PrintUnformatted(did);
                if (did_str != NULL) {
                    snprintf(s_result.device_id, sizeof(s_result.device_id), "%s", did_str);
                    ESP_LOGI(TAG, "Device ID (number): %s", s_result.device_id);
                    cJSON_free(did_str);
                }
            }
        } else {
            ESP_LOGE(TAG, "Device ID (did) not found in response");
        }
        
        if (cJSON_IsString(name)) {
            snprintf(s_result.device_name, sizeof(s_result.device_name), "%s", name->valuestring);
        }
        
        if (cJSON_IsString(sec_key) && sec_key->valuestring != NULL) {
            snprintf(s_result.sec_key, sizeof(s_result.sec_key), "%s", sec_key->valuestring);
            /* 打印完整的 sec_key 用于调试（生产环境应该移除） */
            ESP_LOGI(TAG, "SecKey received: %s (len=%d)", 
                     s_result.sec_key, (int)strlen(s_result.sec_key));
        } else {
            ESP_LOGW(TAG, "SecKey not found in response! Device may already exist.");
            ESP_LOGW(TAG, "If device exists, you need to delete it from OneNET and re-create.");
        }
        
        ESP_LOGI(TAG, "Parsed: DeviceID=%s, ProductID=%s, Name=%s, SecKey=%s", 
                 s_result.device_id, s_result.product_id, s_result.device_name,
                 strlen(s_result.sec_key) > 0 ? "present" : "MISSING");
    }
    
    cJSON_Delete(resp_json);
    return ESP_OK;
}

/* 
 * ESP32-P4 + ESP32-C6 (ESP-Hosted via SDIO)
 * ESP32-C6 supports APSTA mode (simultaneous AP + STA)
 * We start AP mode for activation, keeping any existing STA connection
 */

static esp_netif_t *s_ap_netif_handle = NULL;
static bool s_ap_mode_started = false;

/* Start SoftAP mode on ESP32-C6 via ESP-Hosted */
static esp_err_t start_softap(void)
{
    ESP_LOGI(TAG, "========== Starting SoftAP Mode (ESP32-C6 via SDIO) ==========");
    
    /* Check if AP already started */
    if (s_ap_mode_started) {
        ESP_LOGW(TAG, "SoftAP already running");
        return ESP_OK;
    }
    
    /* Create AP netif if not exists */
    if (s_ap_netif_handle == NULL) {
        s_ap_netif_handle = esp_netif_create_default_wifi_ap();
        if (s_ap_netif_handle == NULL) {
            ESP_LOGE(TAG, "Failed to create AP netif");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "  ✓ AP netif created");
    }
    
    /* Get current WiFi mode */
    wifi_mode_t current_mode;
    esp_err_t ret = esp_wifi_get_mode(&current_mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Set to APSTA mode (keep STA if connected, add AP) */
    wifi_mode_t new_mode = WIFI_MODE_APSTA;
    if (current_mode == WIFI_MODE_NULL) {
        new_mode = WIFI_MODE_AP;
    }
    
    ESP_LOGI(TAG, "  Setting WiFi mode: %d -> %d", current_mode, new_mode);
    ret = esp_wifi_set_mode(new_mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Configure AP parameters */
    wifi_config_t ap_config = {
        .ap = {
            .ssid = ACTIVATION_AP_SSID,
            .ssid_len = strlen(ACTIVATION_AP_SSID),
            .password = ACTIVATION_AP_PASSWORD,
            .channel = ACTIVATION_AP_CHANNEL,
            .max_connection = ACTIVATION_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    
    /* For open network (no password) */
    if (strlen(ACTIVATION_AP_PASSWORD) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_LOGI(TAG, "  AP Config: SSID=%s, Channel=%d", 
             ACTIVATION_AP_SSID, ACTIVATION_AP_CHANNEL);
    
    ret = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP config: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "  ✓ AP config applied");
    
    /* Get AP IP address (default: 192.168.4.1) */
    esp_netif_ip_info_t ip_info;
    ret = esp_netif_get_ip_info(s_ap_netif_handle, &ip_info);
    if (ret == ESP_OK && ip_info.ip.addr != 0) {
        snprintf(s_ap_ip, sizeof(s_ap_ip), IPSTR, IP2STR(&ip_info.ip));
    } else {
        /* Default AP gateway IP */
        snprintf(s_ap_ip, sizeof(s_ap_ip), "192.168.4.1");
    }
    
    s_ap_mode_started = true;
    
    ESP_LOGI(TAG, "  ✓ SoftAP started successfully");
    ESP_LOGI(TAG, "  AP IP: %s", s_ap_ip);
    ESP_LOGI(TAG, "  SSID: %s, Password: %s", ACTIVATION_AP_SSID, ACTIVATION_AP_PASSWORD);
    ESP_LOGI(TAG, "==========================================================");
    
    return ESP_OK;
}

/* Stop SoftAP mode, restore to STA only */
static esp_err_t stop_softap(void)
{
    ESP_LOGI(TAG, "Stopping SoftAP mode");
    
    if (!s_ap_mode_started) {
        return ESP_OK;
    }
    
    /* Restore to STA-only mode */
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to restore STA mode: %s", esp_err_to_name(ret));
    }
    
    s_ap_mode_started = false;
    snprintf(s_ap_ip, sizeof(s_ap_ip), "Not Connected");
    
    ESP_LOGI(TAG, "SoftAP stopped, restored to STA mode");
    return ESP_OK;
}

/* HTTP server URI handlers */
static const httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = http_get_handler,
    .user_ctx = NULL
};

static const httpd_uri_t uri_activate = {
    .uri = "/activate",
    .method = HTTP_POST,
    .handler = http_activate_handler,
    .user_ctx = NULL
};

/* Start HTTP server */
static esp_err_t start_http_server(void)
{
    if (s_http_server != NULL) {
        ESP_LOGW(TAG, "HTTP server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = ACTIVATION_HTTP_PORT;
    config.stack_size = 16384;  /* Increased stack size */
    config.lru_purge_enable = true;
    config.max_uri_handlers = 4;
    /* Increase max request header length to handle larger requests */
    /* Note: This is set in sdkconfig, but we document it here */
    /* CONFIG_HTTPD_MAX_REQ_HDR_LEN should be at least 1024 */
    
    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    esp_err_t ret = httpd_start(&s_http_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    httpd_register_uri_handler(s_http_server, &uri_get);
    httpd_register_uri_handler(s_http_server, &uri_activate);
    
    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}

/* Stop HTTP server - with extended delays to prevent TLSP callback conflicts */
static esp_err_t stop_http_server(void)
{
    if (s_http_server) {
        ESP_LOGI(TAG, "Stopping HTTP server...");
        
        /* Save handle before stopping */
        httpd_handle_t server_handle = s_http_server;
        
        /* Clear handle first to prevent race conditions */
        s_http_server = NULL;
        
        /* First, stop accepting new connections by unregistering URI handlers */
        /* This prevents new requests from being processed */
        ESP_LOGI(TAG, "Unregistering URI handlers...");
        httpd_unregister_uri(server_handle, "/");
        httpd_unregister_uri(server_handle, "/activate");
        
        /* Wait for any in-flight requests to complete */
        /* This gives time for active HTTP requests to finish processing */
        ESP_LOGI(TAG, "Waiting for active requests to complete...");
        vTaskDelay(pdMS_TO_TICKS(1000));  /* Increased from 500ms to 1000ms */
        
        /* Now stop the server */
        /* This will close all connections and stop all server threads */
        ESP_LOGI(TAG, "Calling httpd_stop...");
        esp_err_t ret = httpd_stop(server_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_stop failed: %s", esp_err_to_name(ret));
        }
        
        /* Extended delay to ensure all HTTP server threads and resources are fully cleaned up */
        /* This is critical to avoid TLSP deletion callback conflicts */
        /* Wait longer to ensure all pthread cleanup callbacks complete */
        /* HTTP server uses pthread internally, and cleanup callbacks need time to execute */
        ESP_LOGI(TAG, "Waiting for HTTP server thread cleanup (pthread cleanup callbacks)...");
        vTaskDelay(pdMS_TO_TICKS(4000));  /* Increased from 3000ms to 4000ms for pthread cleanup */
        
        ESP_LOGI(TAG, "HTTP server stopped and cleaned up");
    }
    return ESP_OK;
}

/* Public API implementations */

esp_err_t cloud_activation_server_start(const char *device_code, activation_status_cb_t status_cb)
{
    /* Create mutex if not exists (thread-safe lazy initialization) */
    if (s_server_mutex == NULL) {
        s_server_mutex = xSemaphoreCreateMutex();
        if (s_server_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create server mutex");
            return ESP_ERR_NO_MEM;
        }
    }
    
    /* Acquire mutex to prevent concurrent operations */
    if (xSemaphoreTake(s_server_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire server mutex (timeout)");
        return ESP_ERR_TIMEOUT;
    }
    
    /* Check if operation is already in progress */
    if (s_operation_in_progress) {
        xSemaphoreGive(s_server_mutex);
        ESP_LOGW(TAG, "Server operation already in progress");
        return ESP_ERR_INVALID_STATE;
    }
    
    /* Check if server is already running */
    if (s_server_running) {
        xSemaphoreGive(s_server_mutex);
        ESP_LOGW(TAG, "Activation server already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    /* Mark operation as in progress */
    s_operation_in_progress = true;
    
    /* Device code will be set after successful activation from OneNet device_id */
    /* Clear any pre-existing device code */
    s_device_code[0] = '\0';
    
    s_status_cb = status_cb;
    
    memset(&s_result, 0, sizeof(s_result));
    
    /* Start SoftAP */
    esp_err_t ret = start_softap();
    if (ret != ESP_OK) {
        s_operation_in_progress = false;
        xSemaphoreGive(s_server_mutex);
        return ret;
    }
    
    /* Start HTTP server */
    ret = start_http_server();
    if (ret != ESP_OK) {
        stop_softap();
        s_operation_in_progress = false;
        xSemaphoreGive(s_server_mutex);
        return ret;
    }
    
    s_server_running = true;
    s_operation_in_progress = false;
    update_status(ACTIVATION_STATUS_AP_STARTED);
    
    /* Release mutex before returning */
    xSemaphoreGive(s_server_mutex);
    
    ESP_LOGI(TAG, "Activation server started successfully");
    ESP_LOGI(TAG, "1. Connect to WiFi: %s (password: %s)", ACTIVATION_AP_SSID, ACTIVATION_AP_PASSWORD);
    ESP_LOGI(TAG, "2. Open browser: http://%s", s_ap_ip);
    
    return ESP_OK;
}

esp_err_t cloud_activation_server_stop(void)
{
    /* Create mutex if not exists (thread-safe lazy initialization) */
    if (s_server_mutex == NULL) {
        s_server_mutex = xSemaphoreCreateMutex();
        if (s_server_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create server mutex");
            return ESP_ERR_NO_MEM;
        }
    }
    
    /* Acquire mutex to prevent concurrent operations */
    if (xSemaphoreTake(s_server_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire server mutex (timeout)");
        return ESP_ERR_TIMEOUT;
    }
    
    /* Check if operation is already in progress */
    if (s_operation_in_progress) {
        xSemaphoreGive(s_server_mutex);
        ESP_LOGW(TAG, "Server operation already in progress, cannot stop now");
        return ESP_ERR_INVALID_STATE;
    }
    
    /* Check if server is not running */
    if (!s_server_running) {
        xSemaphoreGive(s_server_mutex);
        return ESP_OK;
    }
    
    /* Mark operation as in progress */
    s_operation_in_progress = true;
    
    ESP_LOGI(TAG, "Stopping activation server...");
    
    /* Stop HTTP server first - this includes extended delays internally */
    stop_http_server();
    
    /* Additional delay after HTTP server stop to ensure complete cleanup */
    /* Wait for all pthread cleanup callbacks to complete */
    /* This is critical - pthread cleanup happens asynchronously */
    ESP_LOGI(TAG, "Waiting for complete resource cleanup (pthread/TLSP cleanup)...");
    vTaskDelay(pdMS_TO_TICKS(3000));  /* Increased from 2000ms to 3000ms */
    
    /* Then stop SoftAP */
    stop_softap();
    
    /* Final delay to ensure WiFi mode change completes and all resources released */
    /* This is critical to prevent TLSP deletion callback conflicts */
    /* Additional delay for WiFi mode change and netif cleanup */
    /* WiFi mode change may also trigger thread cleanup */
    ESP_LOGI(TAG, "Waiting for WiFi/netif cleanup...");
    vTaskDelay(pdMS_TO_TICKS(2500));  /* Increased from 2000ms to 2500ms */
    
    s_server_running = false;
    s_operation_in_progress = false;
    s_status = ACTIVATION_STATUS_IDLE;
    
    /* Release mutex before returning */
    xSemaphoreGive(s_server_mutex);
    
    ESP_LOGI(TAG, "Activation server stopped and all resources released");
    return ESP_OK;
}

bool cloud_activation_server_is_running(void)
{
    /* Thread-safe check: acquire mutex if exists */
    if (s_server_mutex != NULL) {
        if (xSemaphoreTake(s_server_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool running = s_server_running;
            xSemaphoreGive(s_server_mutex);
            return running;
        }
    }
    /* Fallback if mutex not created yet */
    return s_server_running;
}

activation_status_t cloud_activation_server_get_status(void)
{
    return s_status;
}

const char* cloud_activation_server_get_ip(void)
{
    return s_ap_ip;
}

