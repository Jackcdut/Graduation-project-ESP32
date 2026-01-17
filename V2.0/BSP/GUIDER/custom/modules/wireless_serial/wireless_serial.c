/**
 * @file wireless_serial.c
 * @brief Wireless Serial Port Module with UART Passthrough
 * 
 * ESP32-P4 + ESP32-C6 WiFi Serial Communication via TCP Socket.
 * Supports UART passthrough for external devices (e.g., STM32).
 * 
 * Features:
 * - TCP Server mode (ESP32-P4 listens on port 8888)
 * - TCP Client mode (connect to remote server)
 * - UART passthrough (bridge UART to WiFi)
 * - Bidirectional data transfer
 * - UI integration with LVGL textarea
 * 
 * @note This module is tightly coupled with the scrWirelessSerial UI page.
 */

#include "wireless_serial.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>

/* LVGL includes for UI updates */
#include "lvgl.h"
#include "gui_guider.h"

static const char *TAG = "WirelessSerial";

/* UART Configuration for external device passthrough */
#define UART_PORT_NUM      UART_NUM_1
#define UART_TX_PIN        GPIO_NUM_51
#define UART_RX_PIN        GPIO_NUM_52
#define UART_BUF_SIZE      (1024)

/* ==================== State Variables ==================== */
static wireless_serial_status_t s_status = WS_STATUS_DISCONNECTED;
static wireless_serial_data_cb_t s_data_callback = NULL;
static wireless_serial_status_cb_t s_status_callback = NULL;
static bool s_uart_passthrough_enabled = false;

/* ==================== Socket Variables ==================== */
static int s_server_socket = -1;
static int s_client_socket = -1;
static TaskHandle_t s_server_task = NULL;
static TaskHandle_t s_client_task = NULL;
static TaskHandle_t s_uart_rx_task = NULL;
static bool s_running = false;

/* ==================== Forward Declarations ==================== */
static void server_task(void *pvParameters);
static void client_task(void *pvParameters);
static void uart_rx_task(void *pvParameters);
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
 * @brief UART RX task - receives data from external device and forwards to WiFi
 */
static void uart_rx_task(void *pvParameters)
{
    uint8_t *data = (uint8_t *) malloc(UART_BUF_SIZE);
    
    ESP_LOGI(TAG, "UART RX task started");
    
    while (s_uart_passthrough_enabled) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            ESP_LOGI(TAG, "UART received %d bytes", len);
            
            /* Forward to WiFi if connected */
            if (s_client_socket >= 0) {
                wireless_serial_send(data, len);
            }
            
            /* Also update UI */
            wireless_serial_update_ui_receive(data, len);
        }
    }
    
    free(data);
    ESP_LOGI(TAG, "UART RX task stopped");
    vTaskDelete(NULL);
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
                
                /* Forward to UART if passthrough enabled */
                if (s_uart_passthrough_enabled) {
                    uart_write_bytes(UART_PORT_NUM, (const char *)rx_buffer, len);
                }
                
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
    typedef struct {
        char ip[64];
        uint16_t port;
    } client_params_t;
    
    client_params_t *params = (client_params_t *)pvParameters;
    struct sockaddr_in server_addr;

    ESP_LOGI(TAG, "Client task started, connecting to %s:%d", params->ip, params->port);
    update_status(WS_STATUS_CONNECTING);

    /* Create socket */
    s_client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_client_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        update_status(WS_STATUS_ERROR);
        free(params);
        vTaskDelete(NULL);
        return;
    }

    /* Configure server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->port);
    inet_pton(AF_INET, params->ip, &server_addr.sin_addr);

    /* Connect to server */
    if (connect(s_client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to connect: errno %d", errno);
        close(s_client_socket);
        s_client_socket = -1;
        update_status(WS_STATUS_ERROR);
        free(params);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Connected to server");
    update_status(WS_STATUS_CONNECTED);
    free(params);

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

            /* Forward to UART if passthrough enabled */
            if (s_uart_passthrough_enabled) {
                uart_write_bytes(UART_PORT_NUM, (const char *)rx_buffer, len);
            }

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

    /* Initialize UART for passthrough */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized on TX:%d RX:%d", UART_TX_PIN, UART_RX_PIN);

    return ESP_OK;
}

esp_err_t wireless_serial_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing wireless serial");

    wireless_serial_stop_server();
    wireless_serial_disconnect();
    wireless_serial_disable_uart_passthrough();

    s_data_callback = NULL;
    s_status_callback = NULL;
    
    uart_driver_delete(UART_PORT_NUM);

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

    /* Create parameter structure */
    typedef struct {
        char ip[64];
        uint16_t port;
    } client_params_t;
    
    client_params_t *params = malloc(sizeof(client_params_t));
    if (params == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    strncpy(params->ip, server_ip, sizeof(params->ip) - 1);
    params->ip[sizeof(params->ip) - 1] = '\0';
    params->port = port;
    
    xTaskCreate(client_task, "ws_client", 4096, params, 5, &s_client_task);

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

esp_err_t wireless_serial_enable_uart_passthrough(void)
{
    if (s_uart_passthrough_enabled) {
        ESP_LOGW(TAG, "UART passthrough already enabled");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Enabling UART passthrough");
    s_uart_passthrough_enabled = true;

    /* Start UART RX task */
    xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 5, &s_uart_rx_task);

    return ESP_OK;
}

esp_err_t wireless_serial_disable_uart_passthrough(void)
{
    if (!s_uart_passthrough_enabled) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Disabling UART passthrough");
    s_uart_passthrough_enabled = false;

    /* Wait for UART task to finish */
    if (s_uart_rx_task) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_uart_rx_task = NULL;
    }

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

void wireless_serial_update_ip_display(const char *ip_address, uint16_t port)
{
    /* Get UI reference */
    extern lv_ui guider_ui;

    if (guider_ui.scrWirelessSerial_labelIPAddress && lv_obj_is_valid(guider_ui.scrWirelessSerial_labelIPAddress)) {
        char ip_text[64];
        if (ip_address && strlen(ip_address) > 0) {
            snprintf(ip_text, sizeof(ip_text), "IP: %s:%u", ip_address, port);
        } else {
            snprintf(ip_text, sizeof(ip_text), "IP: Disconnected");
        }
        lv_label_set_text(guider_ui.scrWirelessSerial_labelIPAddress, ip_text);
        ESP_LOGI(TAG, "IP display updated: %s", ip_text);
    }
}

void wireless_serial_update_ip_on_screen_load(void)
{
    /* Check WiFi connection and update IP display */
    ESP_LOGI(TAG, "Updating IP display on screen load...");
    
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            char ip_str[16];
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI(TAG, "WiFi connected, IP: %s", ip_str);
            wireless_serial_update_ip_display(ip_str, 8888);
        } else {
            ESP_LOGI(TAG, "WiFi not connected or no IP");
            wireless_serial_update_ip_display(NULL, 0);
        }
    } else {
        ESP_LOGI(TAG, "Network interface not found");
        wireless_serial_update_ip_display(NULL, 0);
    }
}
