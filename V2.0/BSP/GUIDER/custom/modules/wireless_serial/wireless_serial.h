/**
 * @file wireless_serial.h
 * @brief Wireless Serial Port Module Header
 * 
 * ESP32-P4 + ESP32-C6 WiFi Serial Communication via TCP Socket.
 * Provides the backend logic for scrWirelessSerial UI page.
 * 
 * Features:
 * - TCP Server mode (ESP32-P4 listens on port 8888)
 * - TCP Client mode (connect to remote server)
 * - Bidirectional data transfer
 * - UI integration with LVGL textarea
 * 
 * Usage:
 * 1. Call wireless_serial_init() to initialize the module
 * 2. Call wireless_serial_start_server() to start listening
 * 3. Use wireless_serial_send() to send data
 * 4. Received data is automatically displayed in UI
 * 
 * @note This module is tightly coupled with the scrWirelessSerial UI page.
 */

#ifndef WIRELESS_SERIAL_H
#define WIRELESS_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

/* ==================== Configuration ==================== */

/** TCP port for wireless serial communication */
#define WIRELESS_SERIAL_PORT        8888

/** Maximum number of simultaneous client connections */
#define WIRELESS_SERIAL_MAX_CLIENTS 4

/** Size of receive/transmit buffer in bytes */
#define WIRELESS_SERIAL_BUFFER_SIZE 1024

/* ==================== Type Definitions ==================== */

/**
 * @brief Connection status enumeration
 */
typedef enum {
    WS_STATUS_DISCONNECTED = 0,  /**< Not connected */
    WS_STATUS_CONNECTING,        /**< Connection in progress */
    WS_STATUS_CONNECTED,         /**< Connected and ready */
    WS_STATUS_ERROR              /**< Error state */
} wireless_serial_status_t;

/**
 * @brief Data received callback function type
 * @param data Pointer to received data
 * @param len Length of received data in bytes
 */
typedef void (*wireless_serial_data_cb_t)(const uint8_t *data, size_t len);

/**
 * @brief Status change callback function type
 * @param status New connection status
 */
typedef void (*wireless_serial_status_cb_t)(wireless_serial_status_t status);

/* ==================== Core API Functions ==================== */

/**
 * @brief Initialize wireless serial module
 * 
 * @param data_callback Callback function for received data (can be NULL)
 * @param status_callback Callback function for status changes (can be NULL)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_init(wireless_serial_data_cb_t data_callback, 
                               wireless_serial_status_cb_t status_callback);

/**
 * @brief Deinitialize wireless serial module
 * 
 * Stops server/client and releases resources.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_deinit(void);

/**
 * @brief Start wireless serial server (ESP32-P4 as server)
 * 
 * Starts listening on WIRELESS_SERIAL_PORT (8888) for incoming connections.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_start_server(void);

/**
 * @brief Stop wireless serial server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_stop_server(void);

/**
 * @brief Connect to wireless serial server (as client)
 * 
 * @param server_ip Server IP address (e.g., "192.168.1.100")
 * @param port Server port (typically WIRELESS_SERIAL_PORT)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_connect(const char *server_ip, uint16_t port);

/**
 * @brief Disconnect from wireless serial server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_disconnect(void);

/**
 * @brief Send data via wireless serial
 * 
 * @param data Data buffer to send
 * @param len Length of data in bytes
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_send(const uint8_t *data, size_t len);

/**
 * @brief Get current connection status
 * 
 * @return wireless_serial_status_t Current status
 */
wireless_serial_status_t wireless_serial_get_status(void);

/**
 * @brief Check if connected
 * 
 * @return true if connected, false otherwise
 */
bool wireless_serial_is_connected(void);

/**
 * @brief Enable UART passthrough for external devices (e.g., STM32)
 * 
 * When enabled, data received from UART will be forwarded to WiFi,
 * and data received from WiFi will be forwarded to UART.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_enable_uart_passthrough(void);

/**
 * @brief Disable UART passthrough
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wireless_serial_disable_uart_passthrough(void);

/* ==================== UI Interface Functions ==================== */
/* These functions are called from events_init.c for UI interaction */

/**
 * @brief Toggle connection state (connect/disconnect)
 * 
 * Called when user clicks the connect/disconnect button in UI.
 */
void wireless_serial_toggle_connection(void);

/**
 * @brief Send data from UI input
 * 
 * @param data Data string to send
 * @param len Length of data
 */
void wireless_serial_send_data(const char *data, size_t len);

/**
 * @brief Update UI with received data
 * 
 * Called internally when data is received. Updates the receive textarea.
 * 
 * @param data Received data
 * @param len Length of data
 */
void wireless_serial_update_ui_receive(const uint8_t *data, size_t len);

/**
 * @brief Update UI connection status display
 * 
 * Called internally when connection status changes.
 * 
 * @param status New connection status
 */
void wireless_serial_update_ui_status(wireless_serial_status_t status);

/**
 * @brief Update IP address display in wireless serial UI
 * 
 * Called when WiFi connects or IP changes.
 * 
 * @param ip_address IP address string (e.g., "192.168.1.100")
 * @param port TCP port number
 */
void wireless_serial_update_ip_display(const char *ip_address, uint16_t port);

/**
 * @brief Update IP display when wireless serial screen is loaded
 * 
 * Called automatically when entering the wireless serial screen.
 * Checks current WiFi status and updates IP display accordingly.
 */
void wireless_serial_update_ip_on_screen_load(void);

#ifdef __cplusplus
}
#endif

#endif /* WIRELESS_SERIAL_H */
