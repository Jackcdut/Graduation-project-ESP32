/**
 * @file sdcard_manager_ui.h
 * @brief SD Card Manager Full-Screen LVGL UI Module
 * 
 * Provides a full-screen UI for SD Card management with:
 * - Header with back button
 * - Status card showing SD card capacity
 * - 2x2 Grid Layout:
 *   - Top-left: Storage usage legend
 *   - Top-right: File manager buttons
 *   - Bottom-left: Recent files list
 *   - Bottom-right: Statistics
 * - File browser overlay
 */

#ifndef SDCARD_MANAGER_UI_H
#define SDCARD_MANAGER_UI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and show full-screen SD card manager UI
 * @param parent Ignored - UI is created as full-screen overlay on lv_scr_act()
 * @return Created panel object, or NULL on failure
 */
lv_obj_t* sdcard_manager_ui_create(lv_obj_t *parent);

/**
 * @brief Destroy SD card manager UI and free resources
 */
void sdcard_manager_ui_destroy(void);

/**
 * @brief Update all UI elements with current data
 */
void sdcard_manager_ui_update(void);

/**
 * @brief Show file browser for a specific file type
 * @param type File type to browse (0=all folders, 1=screenshots, 2=oscilloscope, 3=config)
 */
void sdcard_manager_ui_show_browser(int type);

/**
 * @brief Check if UI is currently visible
 * @return true if visible
 */
bool sdcard_manager_ui_is_visible(void);

#ifdef __cplusplus
}
#endif

#endif /* SDCARD_MANAGER_UI_H */
