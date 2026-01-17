/**
 * @file wireless_serial.c
 * @brief Wireless Serial Port Module
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
 * @note This module is tightly coupled with the scrWirelessSerial UI page.
 */

#include "wireless_serial.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

/* LVGL includes for UI updates */
#include "lvgl.h"
#include "gui_guider.h"

static const char *TAG = "WirelessSerial";

/* ==================== State Variables ==================== */
static wireless_serial_status_t s_status = WS_STATUS_DISCONNECTED;
static wireless_serial_data_cb_t s_data_callback = NULL;
static wireless_serial_status_cb_t s_status_callback = NULL;

/* ==================== Socket Variables ==================== */
static int s_server_socket = -1;
static int s_client_socket = -1;
static TaskHandle_t s_server_task = NULL;
static TaskHandle_t s_client_task = NULL;
static bool s_running = false;

/* ==================== Forward Declarations ==================== */
static void server_task(void *pvParameters);
static void client_task(void *pvParameters);
static void update_status(wireless_serial_status_t new_status);

/* ==================== Internal Functions ==================== */

/**
 * @brief Update connection status and notify callback
 */
static void update_status(wireless_serial_status_t new_status)
{
    if (s_status != new_status) {
        s_status = new_status;
        ESP_LOGI(TAG, "Status changed to: %d", new_status);
        
        if (s_status_callback) {
            s_status_callback(new_status);
        }
        
        /* Update UI */
        wireless_serial_update_ui_status(new_status);
    }
}

/**
 * @brief Server task - accepts connections and receives data
 * 
 * This task runs as a TCP server, listening on WIRELESS_SERIAL_PORT (8888).
 * When a client connects, it enters a receive loop to handle incoming data.
 */
static void server_task(void *pvParameters)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    ESP_LOGI(TAG, "Server task started");
    
    /* Create socket */
    s_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_server_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        update_status(WS_STATUS_ERROR);
        vTaskDelete(NULL);
        return;
    }
    
    /* Set socket options - allow address reuse */
    int opt = 1;
    setsockopt(s_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Bind socket to any address on specified port */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(WIRELESS_SERIAL_PORT);
    
    if (bind(s_server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: errno %d", errno);
        close(s_server_socket);
        s_server_socket = -1;
        update_status(WS_STATUS_ERROR);
        vTaskDelete(NULL);
        return;
    }
    
    /* Start listening for connections */
    if (listen(s_server_socket, WIRELESS_SERIAL_MAX_CLIENTS) < 0) {
        ESP_LOGE(TAG, "Failed to listen: errno %d", errno);
        close(s_server_socket);
        s_server_socket = -1;
        update_status(WS_STATUS_ERROR);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Server listening on port %d", WIRELESS_SERIAL_PORT);
    update_status(WS_STATUS_CONNECTING);
    
    /* Main server loop */
    while (s_running) {
        /* Accept incoming connection */
        s_client_socket = accept(s_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (s_client_socket < 0) {
            if (s_running) {
                ESP_LOGE(TAG, "Failed to accept connection: errno %d", errno);
            }
            continue;
        }
        
        ESP_LOGI(TAG, "Client connected from %s:%d", 
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        update_status(WS_STATUS_CONNECTED);
        
        /* Receive data loop */
        uint8_t rx_buffer[WIRELESS_SERIAL_BUFFER_SIZE];
        while (s_running) {
            int len = recv(s_client_socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Receive failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGI(TAG, "Client disconnected");
                break;
            } else {
                rx_buffer[len] = 0; /* Null terminate for string operations */
                ESP_LOGI(TAG, "Received %d bytes", len);
                
                /* Call user data callback if registered */
                if (s_data_callback) {
                    s_data_callback(rx_buffer, len);
                }
                
                /* Update UI with received data */
                wireless_serial_update_ui_receive(rx_buffer, len);
            }
        }
        
        /* Close client socket and wait for next connection */
        close(s_client_socket);
        s_client_socket = -1;
        update_status(WS_STATUS_CONNECTING);
    }
    
    /* Cleanup */
    if (s_server_socket >= 0) {
        close(s_server_socket);
        s_server_socket = -1;
    }
    
    ESP_LOGI(TAG, "Server task stopped");
    vTaskDelete(NULL);
}

/**
 * @brief Client task - connects to server and receives data
 * 
 * This task runs as a TCP client, connecting to a remote server.
 */
static void client_task(void *pvParameters)
{
    char *server_ip = (char *)pvParameters;
    struct sockaddr_in server_addr;

    ESP_LOGI(TAG, "Client task started, connecting to %s:%d", server_ip, WIRELESS_SERIAL_PORT);
    update_status(WS_STATUS_CONNECTING);

    /* Create socket */
    s_client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_client_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        update_status(WS_STATUS_ERROR);
        free(server_ip);
        vTaskDelete(NULL);
        return;
    }

    /* Configure server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(WIRELESS_SERIAL_PORT);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    /* Connect to server */
    if (connect(s_client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to connect: errno %d", errno);
        close(s_client_socket);
        s_client_socket = -1;
        update_status(WS_STATUS_ERROR);
        free(server_ip);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Connected to server");
    update_status(WS_STATUS_CONNECTED);
    free(server_ip);

    /* Receive data loop */
    uint8_t rx_buffer[WIRELESS_SERIAL_BUFFER_SIZE];
    while (s_running) {
        int len = recv(s_client_socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Receive failed: errno %d", errno);
            break;
        } else if (len == 0) {
            ESP_LOGI(TAG, "Server disconnected");
            break;
        } else {
            rx_buffer[len] = 0; /* Null terminate */
            ESP_LOGI(TAG, "Received %d bytes", len);

            /* Call user data callback if registered */
            if (s_data_callback) {
                s_data_callback(rx_buffer, len);
            }

            /* Update UI with received data */
            wireless_serial_update_ui_receive(rx_buffer, len);
        }
    }

    /* Cleanup */
    close(s_client_socket);
    s_client_socket = -1;
    update_status(WS_STATUS_DISCONNECTED);

    ESP_LOGI(TAG, "Client task stopped");
    vTaskDelete(NULL);
}

/* ==================== Public API Functions ==================== */

esp_err_t wireless_serial_init(wireless_serial_data_cb_t data_callback,
                               wireless_serial_status_cb_t status_callback)
{
    ESP_LOGI(TAG, "Initializing wireless serial");

    s_data_callback = data_callback;
    s_status_callback = status_callback;
    s_status = WS_STATUS_DISCONNECTED;

    return ESP_OK;
}

esp_err_t wireless_serial_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing wireless serial");

    wireless_serial_stop_server();
    wireless_serial_disconnect();

    s_data_callback = NULL;
    s_status_callback = NULL;

    return ESP_OK;
}

esp_err_t wireless_serial_start_server(void)
{
    if (s_running) {
        ESP_LOGW(TAG, "Server already running");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting wireless serial server on port %d", WIRELESS_SERIAL_PORT);
    s_running = true;

    xTaskCreate(server_task, "ws_server", 4096, NULL, 5, &s_server_task);

    return ESP_OK;
}

esp_err_t wireless_serial_stop_server(void)
{
    if (!s_running) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping wireless serial server");
    s_running = false;

    /* Close sockets to unblock tasks */
    if (s_client_socket >= 0) {
        close(s_client_socket);
        s_client_socket = -1;
    }

    if (s_server_socket >= 0) {
        close(s_server_socket);
        s_server_socket = -1;
    }

    /* Wait for task to finish */
    if (s_server_task) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_server_task = NULL;
    }

    update_status(WS_STATUS_DISCONNECTED);

    return ESP_OK;
}

esp_err_t wireless_serial_connect(const char *server_ip, uint16_t port)
{
    if (s_running) {
        ESP_LOGW(TAG, "Already connected or connecting");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Connecting to %s:%d", server_ip, port);
    s_running = true;

    char *ip_copy = strdup(server_ip);
    xTaskCreate(client_task, "ws_client", 4096, ip_copy, 5, &s_client_task);

    return ESP_OK;
}

esp_err_t wireless_serial_disconnect(void)
{
    if (!s_running) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Disconnecting");
    s_running = false;

    /* Close socket to unblock task */
    if (s_client_socket >= 0) {
        close(s_client_socket);
        s_client_socket = -1;
    }

    /* Wait for task to finish */
    if (s_client_task) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_client_task = NULL;
    }

    update_status(WS_STATUS_DISCONNECTED);

    return ESP_OK;
}

esp_err_t wireless_serial_send(const uint8_t *data, size_t len)
{
    if (s_client_socket < 0) {
        ESP_LOGW(TAG, "Not connected, cannot send");
        return ESP_ERR_INVALID_STATE;
    }

    int sent = send(s_client_socket, data, len, 0);
    if (sent < 0) {
        ESP_LOGE(TAG, "Send failed: errno %d", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sent %d bytes", sent);
    return ESP_OK;
}

wireless_serial_status_t wireless_serial_get_status(void)
{
    return s_status;
}

bool wireless_serial_is_connected(void)
{
    return (s_status == WS_STATUS_CONNECTED);
}

/* ==================== UI Interface Functions ==================== */

void wireless_serial_toggle_connection(void)
{
    if (wireless_serial_is_connected() || s_status == WS_STATUS_CONNECTING) {
        /* Disconnect */
        wireless_serial_stop_server();
        wireless_serial_disconnect();
    } else {
        /* Start server (ESP32-P4 acts as server) */
        wireless_serial_start_server();
    }
}

void wireless_serial_send_data(const char *data, size_t len)
{
    if (data && len > 0) {
        wireless_serial_send((const uint8_t *)data, len);
    }
}

void wireless_serial_update_ui_receive(const uint8_t *data, size_t len)
{
    /* Get UI reference */
    extern lv_ui guider_ui;

    if (guider_ui.scrWirelessSerial_textareaReceive) {
        /* Get current text */
        const char *current_text = lv_textarea_get_text(guider_ui.scrWirelessSerial_textareaReceive);

        /* Create new text with received data appended */
        char new_text[WIRELESS_SERIAL_BUFFER_SIZE + 256];
        snprintf(new_text, sizeof(new_text), "%s%.*s",
                 current_text ? current_text : "", (int)len, data);

        /* Update textarea */
        lv_textarea_set_text(guider_ui.scrWirelessSerial_textareaReceive, new_text);

        /* Scroll to bottom to show latest data */
        lv_textarea_set_cursor_pos(guider_ui.scrWirelessSerial_textareaReceive, LV_TEXTAREA_CURSOR_LAST);
    }
}

void wireless_serial_update_ui_status(wireless_serial_status_t status)
{
    /* 
     * UI elements for status display have been removed in the new design.
     * Status is now shown through the receive textarea or connection button state.
     */
    (void)status;  /* Suppress unused parameter warning */
}
