/**
 * @file stm32_comm.c
 * @brief ESP32-P4与STM32通信模块实现
 */

#include "stm32_comm.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "STM32_COMM";

/*============================================================================
 * 硬件配置
 *============================================================================*/
#define STM32_UART_NUM          UART_NUM_2
#define STM32_UART_TX_PIN       GPIO_NUM_49
#define STM32_UART_RX_PIN       GPIO_NUM_50
#define STM32_UART_BAUD         921600
#define STM32_UART_BUF_SIZE     (COMM_MAX_DATA_LEN + 64)

/*============================================================================
 * 私有变量
 *============================================================================*/
static bool s_initialized = false;
static TaskHandle_t s_rx_task = NULL;
static SemaphoreHandle_t s_tx_mutex = NULL;
static stm32_comm_callbacks_t s_callbacks = {0};

/* 接收缓冲区 */
static uint8_t *s_rx_buffer = NULL;
static uint16_t s_rx_len = 0;

/* 发送缓冲区 */
static uint8_t *s_tx_buffer = NULL;

/* 连接状态 */
static volatile bool s_connected = false;
static volatile uint32_t s_last_heartbeat = 0;
#define HEARTBEAT_TIMEOUT_MS    3000

/*============================================================================
 * 协议处理函数
 *============================================================================*/

static uint8_t calc_checksum(const uint8_t *data, uint16_t len)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

static uint16_t build_frame(uint8_t *buffer, stm32_func_code_t func_code,
                            const void *data, uint16_t data_len)
{
    uint16_t index = 0;
    
    buffer[index++] = COMM_FRAME_HEADER;
    buffer[index++] = (uint8_t)func_code;
    buffer[index++] = (uint8_t)(data_len & 0xFF);
    buffer[index++] = (uint8_t)((data_len >> 8) & 0xFF);
    
    if (data && data_len > 0) {
        memcpy(&buffer[index], data, data_len);
        index += data_len;
    }
    
    uint8_t checksum = calc_checksum(&buffer[1], 3 + data_len);
    buffer[index++] = checksum;
    
    return index;
}

static int parse_frame(const uint8_t *buffer, uint16_t len,
                       stm32_func_code_t *func_code, uint8_t **data, uint16_t *data_len)
{
    if (len < COMM_HEADER_SIZE + COMM_CHECKSUM_SIZE) return -1;
    if (buffer[0] != COMM_FRAME_HEADER) return -2;
    
    *func_code = (stm32_func_code_t)buffer[1];
    *data_len = buffer[2] | ((uint16_t)buffer[3] << 8);
    
    if (*data_len > COMM_MAX_DATA_LEN) return -4;
    
    uint16_t frame_len = COMM_HEADER_SIZE + *data_len + COMM_CHECKSUM_SIZE;
    if (len < frame_len) return -1;
    
    uint8_t calc_cs = calc_checksum(&buffer[1], 3 + *data_len);
    uint8_t recv_cs = buffer[COMM_HEADER_SIZE + *data_len];
    if (calc_cs != recv_cs) return -3;
    
    *data = (*data_len > 0) ? (uint8_t *)&buffer[COMM_HEADER_SIZE] : NULL;
    return 0;
}

/*============================================================================
 * 数据处理
 *============================================================================*/

static void process_received_frame(stm32_func_code_t func_code, 
                                   const uint8_t *data, uint16_t data_len)
{
    switch (func_code) {
        case FUNC_OSC_WAVEFORM:
            if (s_callbacks.osc_waveform && data_len >= sizeof(stm32_osc_waveform_t)) {
                s_callbacks.osc_waveform((const stm32_osc_waveform_t *)data, data_len);
            }
            break;
            
        case FUNC_OSC_MEASUREMENT:
            if (s_callbacks.osc_measurement && data_len >= sizeof(stm32_osc_measurement_t)) {
                s_callbacks.osc_measurement((const stm32_osc_measurement_t *)data);
            }
            break;
            
        case FUNC_METER_DATA:
            if (s_callbacks.meter_data && data_len >= sizeof(stm32_meter_data_t)) {
                s_callbacks.meter_data((const stm32_meter_data_t *)data);
            }
            break;
            
        case FUNC_SIGGEN_STATUS:
            if (s_callbacks.siggen_status && data_len >= sizeof(stm32_siggen_status_t)) {
                s_callbacks.siggen_status((const stm32_siggen_status_t *)data);
            }
            break;
            
        case FUNC_POWER_DATA:
            if (s_callbacks.power_data && data_len >= sizeof(stm32_power_data_t)) {
                s_callbacks.power_data((const stm32_power_data_t *)data);
            }
            break;
            
        case FUNC_HEARTBEAT:
            s_last_heartbeat = (uint32_t)(esp_timer_get_time() / 1000);
            s_connected = true;
            if (s_callbacks.heartbeat && data_len >= sizeof(stm32_heartbeat_t)) {
                s_callbacks.heartbeat((const stm32_heartbeat_t *)data);
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown func_code: 0x%02X", func_code);
            break;
    }
}

static void process_rx_buffer(void)
{
    uint16_t start_idx = 0;
    
    while (start_idx < s_rx_len) {
        /* 查找帧头 */
        if (s_rx_buffer[start_idx] != COMM_FRAME_HEADER) {
            start_idx++;
            continue;
        }
        
        uint16_t remaining = s_rx_len - start_idx;
        if (remaining < COMM_HEADER_SIZE + COMM_CHECKSUM_SIZE) break;
        
        uint16_t data_len = s_rx_buffer[start_idx + 2] | 
                           ((uint16_t)s_rx_buffer[start_idx + 3] << 8);
        
        if (data_len > COMM_MAX_DATA_LEN) {
            start_idx++;
            continue;
        }
        
        uint16_t frame_len = COMM_HEADER_SIZE + data_len + COMM_CHECKSUM_SIZE;
        if (remaining < frame_len) break;
        
        stm32_func_code_t func_code;
        uint8_t *frame_data;
        uint16_t frame_data_len;
        
        int result = parse_frame(&s_rx_buffer[start_idx], frame_len,
                                 &func_code, &frame_data, &frame_data_len);
        
        if (result == 0) {
            process_received_frame(func_code, frame_data, frame_data_len);
            start_idx += frame_len;
        } else {
            start_idx++;
        }
    }
    
    /* 移除已处理数据 */
    if (start_idx > 0) {
        if (start_idx < s_rx_len) {
            memmove(s_rx_buffer, &s_rx_buffer[start_idx], s_rx_len - start_idx);
            s_rx_len -= start_idx;
        } else {
            s_rx_len = 0;
        }
    }
}

/*============================================================================
 * 接收任务
 *============================================================================*/

static void uart_rx_task(void *pvParameters)
{
    uint8_t *temp_buf = heap_caps_malloc(256, MALLOC_CAP_8BIT);
    if (!temp_buf) {
        ESP_LOGE(TAG, "Failed to allocate temp buffer");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "UART RX task started");
    
    while (s_initialized) {
        int len = uart_read_bytes(STM32_UART_NUM, temp_buf, 256, pdMS_TO_TICKS(10));
        
        if (len > 0) {
            /* 追加到接收缓冲区 */
            if (s_rx_len + len <= STM32_UART_BUF_SIZE) {
                memcpy(&s_rx_buffer[s_rx_len], temp_buf, len);
                s_rx_len += len;
            } else {
                /* 缓冲区溢出，丢弃旧数据 */
                s_rx_len = 0;
                memcpy(s_rx_buffer, temp_buf, len);
                s_rx_len = len;
                ESP_LOGW(TAG, "RX buffer overflow, reset");
            }
            
            /* 处理接收缓冲区 */
            process_rx_buffer();
        }
        
        /* 检查心跳超时 */
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        if (s_connected && (now - s_last_heartbeat > HEARTBEAT_TIMEOUT_MS)) {
            s_connected = false;
            ESP_LOGW(TAG, "STM32 heartbeat timeout");
        }
    }
    
    free(temp_buf);
    ESP_LOGI(TAG, "UART RX task stopped");
    vTaskDelete(NULL);
}

/*============================================================================
 * 发送函数
 *============================================================================*/

static esp_err_t send_frame(stm32_func_code_t func_code, const void *data, uint16_t data_len)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    
    if (xSemaphoreTake(s_tx_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    uint16_t frame_len = build_frame(s_tx_buffer, func_code, data, data_len);
    int sent = uart_write_bytes(STM32_UART_NUM, s_tx_buffer, frame_len);
    
    xSemaphoreGive(s_tx_mutex);
    
    if (sent != frame_len) {
        ESP_LOGE(TAG, "UART send failed: %d/%d", sent, frame_len);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/*============================================================================
 * 公共API实现
 *============================================================================*/

esp_err_t stm32_comm_init(const stm32_comm_callbacks_t *callbacks)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing STM32 communication...");
    
    /* 分配缓冲区 */
    s_rx_buffer = heap_caps_malloc(STM32_UART_BUF_SIZE, MALLOC_CAP_8BIT);
    s_tx_buffer = heap_caps_malloc(STM32_UART_BUF_SIZE, MALLOC_CAP_8BIT);
    
    if (!s_rx_buffer || !s_tx_buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffers");
        if (s_rx_buffer) free(s_rx_buffer);
        if (s_tx_buffer) free(s_tx_buffer);
        return ESP_ERR_NO_MEM;
    }
    
    /* 创建互斥锁 */
    s_tx_mutex = xSemaphoreCreateMutex();
    if (!s_tx_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(s_rx_buffer);
        free(s_tx_buffer);
        return ESP_ERR_NO_MEM;
    }
    
    /* 配置UART */
    uart_config_t uart_config = {
        .baud_rate = STM32_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    esp_err_t ret = uart_driver_install(STM32_UART_NUM, STM32_UART_BUF_SIZE, 
                                        STM32_UART_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    
    ret = uart_param_config(STM32_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        uart_driver_delete(STM32_UART_NUM);
        goto cleanup;
    }
    
    ret = uart_set_pin(STM32_UART_NUM, STM32_UART_TX_PIN, STM32_UART_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        uart_driver_delete(STM32_UART_NUM);
        goto cleanup;
    }
    
    /* 注册回调 */
    if (callbacks) {
        memcpy(&s_callbacks, callbacks, sizeof(stm32_comm_callbacks_t));
    }
    
    s_initialized = true;
    s_rx_len = 0;
    s_connected = false;
    
    /* 创建接收任务 */
    xTaskCreate(uart_rx_task, "stm32_rx", 4096, NULL, 10, &s_rx_task);
    
    ESP_LOGI(TAG, "STM32 communication initialized (TX:%d, RX:%d, %d bps)",
             STM32_UART_TX_PIN, STM32_UART_RX_PIN, STM32_UART_BAUD);
    
    return ESP_OK;

cleanup:
    if (s_tx_mutex) vSemaphoreDelete(s_tx_mutex);
    if (s_rx_buffer) free(s_rx_buffer);
    if (s_tx_buffer) free(s_tx_buffer);
    s_tx_mutex = NULL;
    s_rx_buffer = NULL;
    s_tx_buffer = NULL;
    return ret;
}

void stm32_comm_deinit(void)
{
    if (!s_initialized) return;
    
    ESP_LOGI(TAG, "Deinitializing STM32 communication...");
    
    s_initialized = false;
    
    /* 等待任务结束 */
    if (s_rx_task) {
        vTaskDelay(pdMS_TO_TICKS(50));
        s_rx_task = NULL;
    }
    
    uart_driver_delete(STM32_UART_NUM);
    
    if (s_tx_mutex) {
        vSemaphoreDelete(s_tx_mutex);
        s_tx_mutex = NULL;
    }
    
    if (s_rx_buffer) {
        free(s_rx_buffer);
        s_rx_buffer = NULL;
    }
    
    if (s_tx_buffer) {
        free(s_tx_buffer);
        s_tx_buffer = NULL;
    }
    
    memset(&s_callbacks, 0, sizeof(s_callbacks));
}

void stm32_comm_register_callbacks(const stm32_comm_callbacks_t *callbacks)
{
    if (callbacks) {
        /* 合并回调而不是覆盖，只更新非NULL的回调 */
        if (callbacks->osc_waveform) {
            s_callbacks.osc_waveform = callbacks->osc_waveform;
        }
        if (callbacks->osc_measurement) {
            s_callbacks.osc_measurement = callbacks->osc_measurement;
        }
        if (callbacks->meter_data) {
            s_callbacks.meter_data = callbacks->meter_data;
        }
        if (callbacks->siggen_status) {
            s_callbacks.siggen_status = callbacks->siggen_status;
        }
        if (callbacks->power_data) {
            s_callbacks.power_data = callbacks->power_data;
        }
        if (callbacks->heartbeat) {
            s_callbacks.heartbeat = callbacks->heartbeat;
        }
    }
}

bool stm32_comm_is_connected(void)
{
    return s_connected;
}

/*============================================================================
 * 信号发生器控制
 *============================================================================*/

esp_err_t stm32_siggen_set_all(uint8_t wave_type, float frequency, float amplitude)
{
    stm32_siggen_control_t cmd = {
        .cmd_type = SIGGEN_CMD_SET_ALL,
        .wave_type = wave_type,
        .enabled = 1,
        .reserved = 0,
        .frequency = frequency,
        .amplitude = amplitude
    };
    return send_frame(FUNC_SIGGEN_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_siggen_set_waveform(uint8_t wave_type)
{
    stm32_siggen_control_t cmd = {
        .cmd_type = SIGGEN_CMD_SET_WAVE,
        .wave_type = wave_type,
        .enabled = 0,
        .reserved = 0,
        .frequency = 0,
        .amplitude = 0
    };
    return send_frame(FUNC_SIGGEN_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_siggen_set_frequency(float frequency)
{
    stm32_siggen_control_t cmd = {
        .cmd_type = SIGGEN_CMD_SET_FREQ,
        .wave_type = 0,
        .enabled = 0,
        .reserved = 0,
        .frequency = frequency,
        .amplitude = 0
    };
    return send_frame(FUNC_SIGGEN_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_siggen_set_amplitude(float amplitude)
{
    stm32_siggen_control_t cmd = {
        .cmd_type = SIGGEN_CMD_SET_AMP,
        .wave_type = 0,
        .enabled = 0,
        .reserved = 0,
        .frequency = 0,
        .amplitude = amplitude
    };
    return send_frame(FUNC_SIGGEN_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_siggen_enable(bool enable)
{
    stm32_siggen_control_t cmd = {
        .cmd_type = enable ? SIGGEN_CMD_ENABLE : SIGGEN_CMD_DISABLE,
        .wave_type = 0,
        .enabled = enable ? 1 : 0,
        .reserved = 0,
        .frequency = 0,
        .amplitude = 0
    };
    return send_frame(FUNC_SIGGEN_CONTROL, &cmd, sizeof(cmd));
}

/*============================================================================
 * 示波器控制
 *============================================================================*/

esp_err_t stm32_osc_start(void)
{
    stm32_osc_control_t cmd = {
        .cmd_type = OSC_CMD_START,
        .gain = 0,
        .coupling = 0,
        .auto_range = 0,
        .sample_rate = 0
    };
    return send_frame(FUNC_OSC_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_osc_stop(void)
{
    stm32_osc_control_t cmd = {
        .cmd_type = OSC_CMD_STOP,
        .gain = 0,
        .coupling = 0,
        .auto_range = 0,
        .sample_rate = 0
    };
    return send_frame(FUNC_OSC_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_osc_single(void)
{
    stm32_osc_control_t cmd = {
        .cmd_type = OSC_CMD_SINGLE,
        .gain = 0,
        .coupling = 0,
        .auto_range = 0,
        .sample_rate = 0
    };
    return send_frame(FUNC_OSC_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_osc_set_gain(uint8_t gain)
{
    stm32_osc_control_t cmd = {
        .cmd_type = OSC_CMD_SET_GAIN,
        .gain = gain,
        .coupling = 0,
        .auto_range = 0,
        .sample_rate = 0
    };
    return send_frame(FUNC_OSC_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_osc_set_coupling(uint8_t coupling)
{
    stm32_osc_control_t cmd = {
        .cmd_type = OSC_CMD_SET_COUPLING,
        .gain = 0,
        .coupling = coupling,
        .auto_range = 0,
        .sample_rate = 0
    };
    return send_frame(FUNC_OSC_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_osc_set_samplerate(uint32_t sample_rate)
{
    stm32_osc_control_t cmd = {
        .cmd_type = OSC_CMD_SET_SAMPLERATE,
        .gain = 0,
        .coupling = 0,
        .auto_range = 0,
        .sample_rate = sample_rate
    };
    return send_frame(FUNC_OSC_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_osc_set_autorange(bool enable)
{
    stm32_osc_control_t cmd = {
        .cmd_type = OSC_CMD_SET_AUTORANGE,
        .gain = 0,
        .coupling = 0,
        .auto_range = enable ? 1 : 0,
        .sample_rate = 0
    };
    return send_frame(FUNC_OSC_CONTROL, &cmd, sizeof(cmd));
}

/*============================================================================
 * 万用表控制
 *============================================================================*/

esp_err_t stm32_meter_set_mode(uint8_t mode)
{
    stm32_meter_control_t cmd = {
        .cmd_type = METER_CMD_SET_MODE,
        .mode = mode,
        .range = 0,
        .auto_range = 0
    };
    return send_frame(FUNC_METER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_meter_set_range(uint8_t range)
{
    stm32_meter_control_t cmd = {
        .cmd_type = METER_CMD_SET_RANGE,
        .mode = 0,
        .range = range,
        .auto_range = 0
    };
    return send_frame(FUNC_METER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_meter_set_autorange(bool enable)
{
    stm32_meter_control_t cmd = {
        .cmd_type = METER_CMD_AUTO_RANGE,
        .mode = 0,
        .range = 0,
        .auto_range = enable ? 1 : 0
    };
    return send_frame(FUNC_METER_CONTROL, &cmd, sizeof(cmd));
}

/*============================================================================
 * 心跳
 *============================================================================*/

esp_err_t stm32_comm_send_heartbeat(void)
{
    stm32_heartbeat_t hb = {
        .timestamp = (uint32_t)(esp_timer_get_time() / 1000),
        .device_status = 0,
        .current_mode = 0,
        .reserved = {0, 0}
    };
    return send_frame(FUNC_HEARTBEAT, &hb, sizeof(hb));
}

/*============================================================================
 * 数控电源控制
 *============================================================================*/

esp_err_t stm32_power_enable(bool enable)
{
    stm32_power_control_t cmd = {
        .cmd_type = enable ? POWER_CMD_ENABLE : POWER_CMD_DISABLE,
        .output_enable = enable ? 1 : 0,
        .mode = 0,
        .pd_voltage = 0,
        .voltage_set = 0,
        .current_set = 0
    };
    return send_frame(FUNC_POWER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_power_set_voltage(float voltage)
{
    stm32_power_control_t cmd = {
        .cmd_type = POWER_CMD_SET_VOLTAGE,
        .output_enable = 0,
        .mode = 0,
        .pd_voltage = 0,
        .voltage_set = voltage,
        .current_set = 0
    };
    return send_frame(FUNC_POWER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_power_set_current(float current)
{
    stm32_power_control_t cmd = {
        .cmd_type = POWER_CMD_SET_CURRENT,
        .output_enable = 0,
        .mode = 0,
        .pd_voltage = 0,
        .voltage_set = 0,
        .current_set = current
    };
    return send_frame(FUNC_POWER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_power_set_mode(uint8_t mode)
{
    stm32_power_control_t cmd = {
        .cmd_type = POWER_CMD_SET_MODE,
        .output_enable = 0,
        .mode = mode,
        .pd_voltage = 0,
        .voltage_set = 0,
        .current_set = 0
    };
    return send_frame(FUNC_POWER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_power_set_pd_voltage(uint8_t pd_voltage)
{
    stm32_power_control_t cmd = {
        .cmd_type = POWER_CMD_SET_PD,
        .output_enable = 0,
        .mode = 0,
        .pd_voltage = pd_voltage,
        .voltage_set = 0,
        .current_set = 0
    };
    return send_frame(FUNC_POWER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_power_set_all(bool enable, float voltage, float current, 
                              uint8_t mode, uint8_t pd_voltage)
{
    stm32_power_control_t cmd = {
        .cmd_type = POWER_CMD_SET_ALL,
        .output_enable = enable ? 1 : 0,
        .mode = mode,
        .pd_voltage = pd_voltage,
        .voltage_set = voltage,
        .current_set = current
    };
    return send_frame(FUNC_POWER_CONTROL, &cmd, sizeof(cmd));
}

esp_err_t stm32_power_clear_protection(void)
{
    stm32_power_control_t cmd = {
        .cmd_type = POWER_CMD_CLEAR_PROT,
        .output_enable = 0,
        .mode = 0,
        .pd_voltage = 0,
        .voltage_set = 0,
        .current_set = 0
    };
    return send_frame(FUNC_POWER_CONTROL, &cmd, sizeof(cmd));
}

/*============================================================================
 * 工作模式切换
 *============================================================================*/

esp_err_t stm32_set_mode(uint8_t mode)
{
    stm32_mode_control_t cmd = {
        .mode = mode,
        .reserved = {0, 0, 0}
    };
    return send_frame(FUNC_MODE_CONTROL, &cmd, sizeof(cmd));
}
