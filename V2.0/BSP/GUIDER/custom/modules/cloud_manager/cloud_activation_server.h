/**
 * @file cloud_activation_server.h
 * @brief ESP32 AP Hotspot and HTTP Server for Device Activation
 * 
 * This module provides:
 * 1. SoftAP hotspot for device configuration
 * 2. HTTP server with activation webpage
 * 3. OneNet device registration via HTTP API
 */

#ifndef CLOUD_ACTIVATION_SERVER_H
#define CLOUD_ACTIVATION_SERVER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AP Configuration */
#define ACTIVATION_AP_SSID          "ExDebug_Setup"
#define ACTIVATION_AP_PASSWORD      "12345678"
#define ACTIVATION_AP_CHANNEL       1
#define ACTIVATION_AP_MAX_CONN      4

/* Aliases for UI access */
#define CLOUD_ACTIVATION_AP_SSID    ACTIVATION_AP_SSID
#define CLOUD_ACTIVATION_AP_PASS    ACTIVATION_AP_PASSWORD

/* HTTP Server Configuration */
#define ACTIVATION_HTTP_PORT        80

/* OneNet API Configuration */
#define ONENET_API_HOST             "iot-api.heclouds.com"
#define ONENET_API_PORT             443
#define ONENET_API_CREATE_DEVICE    "/device/create"

/* Activation status */
typedef enum {
    ACTIVATION_STATUS_IDLE = 0,
    ACTIVATION_STATUS_AP_STARTED,
    ACTIVATION_STATUS_CLIENT_CONNECTED,
    ACTIVATION_STATUS_WIFI_CONFIGURING,
    ACTIVATION_STATUS_WIFI_CONNECTED,
    ACTIVATION_STATUS_REGISTERING,
    ACTIVATION_STATUS_SUCCESS,
    ACTIVATION_STATUS_FAILED
} activation_status_t;

/* Activation result */
typedef struct {
    bool success;
    char device_code[20];
    char device_name[48];
    char device_id[24];       /* 设备ID (did) - 每个设备唯一 */
    char product_id[24];      /* 产品ID (pid) - 产品标识符 */
    char sec_key[64];
    char error_msg[64];
} activation_result_t;

/* Activation status callback */
typedef void (*activation_status_cb_t)(activation_status_t status, const activation_result_t *result);

/**
 * @brief Start activation server (AP + HTTP)
 * 
 * @param device_code Pre-generated device code
 * @param status_cb Status callback function
 * @return ESP_OK on success
 */
esp_err_t cloud_activation_server_start(const char *device_code, activation_status_cb_t status_cb);

/**
 * @brief Stop activation server
 * 
 * @return ESP_OK on success
 */
esp_err_t cloud_activation_server_stop(void);

/**
 * @brief Check if activation server is running
 * 
 * @return true if running
 */
bool cloud_activation_server_is_running(void);

/**
 * @brief Get current activation status
 * 
 * @return Current status
 */
activation_status_t cloud_activation_server_get_status(void);

/**
 * @brief Get AP IP address string
 * 
 * @return IP address string (e.g., "192.168.4.1")
 */
const char* cloud_activation_server_get_ip(void);

/**
 * @brief Generate OneNet API authorization token
 * 
 * @param product_id Product ID
 * @param access_key Access key
 * @param token_buf Buffer to store token
 * @param buf_size Buffer size
 * @return ESP_OK on success
 */
esp_err_t cloud_activation_generate_token(const char *product_id, const char *access_key,
                                          char *token_buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* CLOUD_ACTIVATION_SERVER_H */

