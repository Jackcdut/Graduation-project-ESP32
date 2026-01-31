/*
 * WiFi Manager for ESP32-P4 with ESP32-C6
 * Uses ESP-Hosted solution for WiFi connectivity
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi scan result callback */
typedef void (*wifi_scan_done_cb_t)(uint16_t ap_count);

/* WiFi connection status callback */
typedef void (*wifi_status_cb_t)(bool connected, const char *ssid);

/**
 * @brief Initialize WiFi manager
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Start WiFi scan
 * 
 * @param cb Callback function when scan completes
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_scan_start(wifi_scan_done_cb_t cb);

/**
 * @brief Get scan results
 * 
 * @param ap_records Array to store AP records
 * @param ap_count Pointer to AP count (input: max count, output: actual count)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_scan_results(wifi_ap_record_t *ap_records, uint16_t *ap_count);

/**
 * @brief Connect to WiFi AP
 * 
 * @param ssid SSID of the AP
 * @param password Password (can be NULL for open networks)
 * @param status_cb Status callback function
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password, wifi_status_cb_t status_cb);

/**
 * @brief Disconnect from WiFi
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get connected SSID
 * 
 * @param ssid Buffer to store SSID
 * @param max_len Maximum buffer length
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_ssid(char *ssid, size_t max_len);

/**
 * @brief Get WiFi RSSI
 *
 * @return RSSI value in dBm, 0 if not connected
 */
int8_t wifi_manager_get_rssi(void);

/**
 * @brief Save WiFi credentials to NVS
 *
 * @param ssid SSID to save
 * @param password Password to save
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_save_credentials(const char *ssid, const char *password);

/**
 * @brief Load WiFi credentials from NVS
 *
 * @param ssid Buffer to store SSID
 * @param ssid_len Maximum SSID buffer length
 * @param password Buffer to store password
 * @param password_len Maximum password buffer length
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if no credentials saved
 */
esp_err_t wifi_manager_load_credentials(char *ssid, size_t ssid_len, char *password, size_t password_len);

/**
 * @brief Clear saved WiFi credentials from NVS
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_clear_credentials(void);

/**
 * @brief Auto-connect to saved WiFi if available
 *
 * @param status_cb Status callback function
 * @return ESP_OK if auto-connect started, ESP_ERR_NVS_NOT_FOUND if no saved credentials
 */
esp_err_t wifi_manager_auto_connect(wifi_status_cb_t status_cb);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */
