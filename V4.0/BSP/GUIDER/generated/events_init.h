/*
* Copyright 2025 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


#ifndef EVENTS_INIT_H_
#define EVENTS_INIT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"

void events_init(lv_ui *ui);

void events_init_scrHome(lv_ui *ui);
void events_init_scrCopy(lv_ui *ui);
void events_init_scrScan(lv_ui *ui);
void events_init_scrPrintMenu(lv_ui *ui);
void events_init_scrPrintInternet(lv_ui *ui);
void events_init_scrScanFini(lv_ui *ui);
void events_init_scrPowerSupply(lv_ui *ui);
void events_init_scrAIChat(lv_ui *ui);
void events_init_scrSettings(lv_ui *ui);
void events_init_scrWirelessSerial(lv_ui *ui);
void events_init_scrOscilloscope(lv_ui *ui);

/* ========================================
 * ESP32 Signal Generator Interface Functions
 * ========================================*/
// Set amplitude slider reference for safe access in event handlers
void scrCopy_set_amplitude_slider(lv_obj_t *slider);
// Clear static references when screen is unloaded
void scrCopy_clear_static_refs(void);

/* ========================================
 * ESP32 Power Supply Interface Functions
 * ========================================*/
// Update functions (called by ESP32 to update measurements)
void ps_update_voltage_actual(float voltage_actual);
void ps_update_current_actual(float current_actual);
void ps_update_power_actual(float power_actual);
void ps_update_mode(bool is_cc_mode);
void ps_update_measurements(float voltage_actual, float current_actual, float power_actual, bool is_cc_mode);

// Get functions (ESP32 reads UI settings)
float ps_get_voltage_set(void);
float ps_get_current_set(void);
bool ps_get_output_enabled(void);
bool ps_get_mode(void);  // true = CC mode, false = CV mode

/* ========================================
 * ESP32 Settings WiFi Interface Functions
 * ========================================*/
// WiFi scan callback (called by ESP32 after scan completes)
void wifi_scan_result_callback(const char *wifi_networks[], int signal_strengths[], int count);

// WiFi status callback (called by ESP32 when connection status changes)
void wifi_status_update_callback(bool connected, const char *ssid);

// Get brightness setting (ESP32 reads brightness value)
int settings_get_brightness(void);

/* ========================================
 * ESP32 Wireless Serial Interface Functions
 * ========================================*/
// Update received data in the receive textarea (called by ESP32 when data is received)
void wireless_serial_update_receive_data(const char *data);

// Get current serial configuration (ESP32 reads configuration)
void wireless_serial_get_config(uint32_t *baudrate, uint8_t *databits, uint8_t *stopbits, uint8_t *parity);

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
