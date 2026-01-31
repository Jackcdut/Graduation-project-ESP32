/*
 * WiFi Manager Implementation
 * ESP32-P4 + ESP32-C6 using ESP-Hosted
 */

/**
 * @file wifi_manager.c
 * @brief WiFi Manager Implementation
 * 
 * ESP32-P4 + ESP32-C6 using ESP-Hosted
 * 
 * @note This file is part of the network module (main/network/)
 */

#include "wifi_manager.h"
#include "esp_wifi_remote.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "wifi_manager";

/* NVS namespace for WiFi credentials */
#define NVS_NAMESPACE "wifi_config"
#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASSWORD "password"

/* WiFi event group */
static EventGroupHandle_t s_wifi_event_group = NULL;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_SCAN_DONE_BIT BIT2

/* Connection retry settings */
#define WIFI_MAX_RETRY 5
static int s_retry_num = 0;

/* WiFi status */
static bool s_is_connected = false;
static char s_connected_ssid[33] = {0};
static int8_t s_rssi = 0;

/* Callbacks */
static wifi_scan_done_cb_t s_scan_done_cb = NULL;
static wifi_status_cb_t s_status_cb = NULL;

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started (ready for connection)");
        /* Don't auto-connect here - wait for user to connect from Settings screen */
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_is_connected = false;
        s_rssi = 0;
        
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry connecting to AP (%d/%d)", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGW(TAG, "Failed to connect to AP");
            
            /* Notify callback */
            if (s_status_cb) {
                s_status_cb(false, NULL);
            }
            
            /* Also notify global callback if set (for UI updates) */
            extern void wifi_status_update_callback(bool, const char *) __attribute__((weak));
            if (wifi_status_update_callback) {
                wifi_status_update_callback(false, NULL);
            }
        }
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        s_retry_num = 0;
        s_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        /* Get connected AP info */
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            memcpy(s_connected_ssid, ap_info.ssid, sizeof(s_connected_ssid) - 1);
            s_rssi = ap_info.rssi;
            ESP_LOGI(TAG, "Connected to SSID: %s, RSSI: %d", s_connected_ssid, s_rssi);

            /* Save WiFi credentials to NVS for auto-connect */
            wifi_config_t wifi_config;
            if (esp_wifi_get_config(WIFI_IF_STA, &wifi_config) == ESP_OK) {
                esp_err_t save_ret = wifi_manager_save_credentials(
                    (char *)wifi_config.sta.ssid,
                    (char *)wifi_config.sta.password
                );
                if (save_ret == ESP_OK) {
                    ESP_LOGI(TAG, "WiFi credentials saved for auto-connect");
                } else {
                    ESP_LOGW(TAG, "Failed to save WiFi credentials: %s", esp_err_to_name(save_ret));
                }
            }
        }

        /* Initialize SNTP time synchronization after WiFi connected */
        extern void wifi_manager_init_sntp(void) __attribute__((weak));
        if (wifi_manager_init_sntp) {
            wifi_manager_init_sntp();
        }

        /* Notify callback */
        if (s_status_cb) {
            s_status_cb(true, s_connected_ssid);
        }

        /* Also notify global callback if set (for UI updates) */
        extern void wifi_status_update_callback(bool, const char *) __attribute__((weak));
        if (wifi_status_update_callback) {
            wifi_status_update_callback(true, s_connected_ssid);
        }
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "WiFi scan done");
        xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);
        
        /* Get scan result count */
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        ESP_LOGI(TAG, "Found %d APs", ap_count);
        
        /* Notify callback */
        if (s_scan_done_cb) {
            s_scan_done_cb(ap_count);
        }
    }
}

esp_err_t wifi_manager_init(void)
{
    if (s_wifi_event_group != NULL) {
        ESP_LOGW(TAG, "WiFi manager already initialized");
        return ESP_OK;
    }
    
    /* Create event group */
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }
    
    /* Initialize TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_init());
    
    /* Create default event loop if not exists */
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Create default WiFi STA netif */
    esp_netif_create_default_wifi_sta();
    
    /* Initialize WiFi with default config */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                               &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                               &wifi_event_handler, NULL));
    
    /* Set WiFi mode to STA */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    /* Start WiFi */
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi manager initialized successfully");
    return ESP_OK;
}

esp_err_t wifi_manager_scan_start(wifi_scan_done_cb_t cb)
{
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    s_scan_done_cb = cb;
    
    /* Clear scan done bit */
    xEventGroupClearBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);
    
    /* Configure scan */
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };
    
    /* Start scan */
    esp_err_t ret = esp_wifi_scan_start(&scan_config, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start scan: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "WiFi scan started");
    return ESP_OK;
}

esp_err_t wifi_manager_get_scan_results(wifi_ap_record_t *ap_records, uint16_t *ap_count)
{
    if (s_wifi_event_group == NULL || ap_records == NULL || ap_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Check if scan is done (non-blocking) - CRITICAL: Do not block in LVGL context! */
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    
    if (!(bits & WIFI_SCAN_DONE_BIT)) {
        /* Scan not completed yet */
        return ESP_ERR_NOT_FINISHED;
    }
    
    /* Get scan results */
    esp_err_t ret = esp_wifi_scan_get_ap_records(ap_count, ap_records);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get scan results: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password, wifi_status_cb_t status_cb)
{
    if (s_wifi_event_group == NULL || ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* Disconnect if already connected or attempting to connect */
    if (s_is_connected) {
        ESP_LOGI(TAG, "Disconnecting from current network before new connection");
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(100)); /* Wait for disconnect to complete */
    }
    
    /* Reset connection state */
    s_status_cb = status_cb;
    s_retry_num = 0;
    s_is_connected = false;
    s_rssi = 0;
    memset(s_connected_ssid, 0, sizeof(s_connected_ssid));
    
    /* Clear event bits */
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    
    /* Configure WiFi */
    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (password != NULL) {
        strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }
    wifi_config.sta.threshold.authmode = (password != NULL && strlen(password) > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);
    
    /* Set WiFi configuration */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    /* Connect */
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(ret));
        s_retry_num = WIFI_MAX_RETRY; /* Prevent auto-retry on immediate failure */
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t wifi_manager_disconnect(void)
{
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    s_is_connected = false;
    s_rssi = 0;
    memset(s_connected_ssid, 0, sizeof(s_connected_ssid));
    
    return esp_wifi_disconnect();
}

bool wifi_manager_is_connected(void)
{
    return s_is_connected;
}

esp_err_t wifi_manager_get_ssid(char *ssid, size_t max_len)
{
    if (ssid == NULL || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_is_connected) {
        return ESP_ERR_WIFI_NOT_CONNECT;
    }
    
    strlcpy(ssid, s_connected_ssid, max_len);
    return ESP_OK;
}

int8_t wifi_manager_get_rssi(void)
{
    return s_rssi;
}

esp_err_t wifi_manager_save_credentials(const char *ssid, const char *password)
{
    if (ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err;

    /* Open NVS */
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    /* Save SSID */
    err = nvs_set_str(nvs_handle, NVS_KEY_SSID, ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    /* Save password (can be empty string for open networks) */
    const char *pwd = (password != NULL) ? password : "";
    err = nvs_set_str(nvs_handle, NVS_KEY_PASSWORD, pwd);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    /* Commit changes */
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "WiFi credentials saved: SSID=%s", ssid);
    return err;
}

esp_err_t wifi_manager_load_credentials(char *ssid, size_t ssid_len, char *password, size_t password_len)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err;

    /* Open NVS */
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved WiFi credentials found");
        } else {
            ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        }
        return err;
    }

    /* Load SSID */
    size_t required_size = ssid_len;
    err = nvs_get_str(nvs_handle, NVS_KEY_SSID, ssid, &required_size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved SSID found");
        } else {
            ESP_LOGE(TAG, "Failed to load SSID: %s", esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
        return err;
    }

    /* Load password */
    required_size = password_len;
    err = nvs_get_str(nvs_handle, NVS_KEY_PASSWORD, password, &required_size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved password found, using empty password");
            password[0] = '\0';
            err = ESP_OK;
        } else {
            ESP_LOGE(TAG, "Failed to load password: %s", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return err;
        }
    }

    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "WiFi credentials loaded: SSID=%s", ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_clear_credentials(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    /* Open NVS */
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    /* Erase all keys in namespace */
    err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase credentials: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    /* Commit changes */
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "WiFi credentials cleared");
    return err;
}

esp_err_t wifi_manager_auto_connect(wifi_status_cb_t status_cb)
{
    char ssid[33] = {0};
    char password[64] = {0};

    /* Load saved credentials */
    esp_err_t err = wifi_manager_load_credentials(ssid, sizeof(ssid), password, sizeof(password));
    if (err != ESP_OK) {
        return err;
    }

    /* Connect using saved credentials */
    ESP_LOGI(TAG, "Auto-connecting to saved WiFi: %s", ssid);
    return wifi_manager_connect(ssid, password, status_cb);
}

