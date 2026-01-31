/**
 * @file cloud_manager_ui.h
 * @brief Cloud Platform Management UI Module
 */

#ifndef CLOUD_MANAGER_UI_H
#define CLOUD_MANAGER_UI_H

#include "lvgl.h"
#include "gui_guider.h"
#include "cloud_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CLOUD_UI_STATE_QR_CODE = 0,
    CLOUD_UI_STATE_ACTIVATING,
    CLOUD_UI_STATE_ACTIVATED,
    CLOUD_UI_STATE_FILE_BROWSER,
    CLOUD_UI_STATE_UPLOADING
} cloud_ui_state_t;

esp_err_t cloud_manager_ui_create(lv_obj_t *parent);
void cloud_manager_ui_destroy(void);
void cloud_manager_ui_show(void);
void cloud_manager_ui_hide(void);
void cloud_manager_ui_refresh(void);
void cloud_manager_ui_set_state(cloud_ui_state_t state);
cloud_ui_state_t cloud_manager_ui_get_state(void);
void cloud_manager_ui_show_qr_code(void);
void cloud_manager_ui_show_activating(void);
void cloud_manager_ui_show_activated(const char *device_code);
void cloud_manager_ui_update_sync_status(const cloud_sync_status_t *status);
void cloud_manager_ui_update_today_stats(const cloud_today_stats_t *stats);
void cloud_manager_ui_show_file_browser(void);
void cloud_manager_ui_hide_file_browser(void);
void cloud_manager_ui_refresh_file_list(void);
void cloud_manager_ui_show_upload_progress(const char *file_name);
void cloud_manager_ui_update_upload_progress(uint32_t progress, uint32_t current_points, uint32_t total_points, const char *status);
void cloud_manager_ui_hide_upload_progress(void);
void cloud_manager_ui_show_upload_result(bool success, const char *message);
void cloud_manager_ui_set_auto_upload(bool enabled);
void cloud_manager_ui_set_interval(uint32_t minutes);
void cloud_manager_ui_show_error(int error_type, const char *message);
void cloud_manager_ui_activation_callback(cloud_activation_state_t state, const char *device_code);
void cloud_manager_ui_upload_progress_callback(uint32_t task_id, uint32_t progress, uint32_t uploaded_bytes);
void cloud_manager_ui_upload_complete_callback(uint32_t task_id, bool success, const char *error_msg);

/**
 * @brief 更新定位信息显示
 * @note 从 OneNET 获取最新的 WiFi 定位结果并更新 UI
 */
void cloud_manager_ui_update_location(void);

#ifdef __cplusplus
}
#endif

#endif

