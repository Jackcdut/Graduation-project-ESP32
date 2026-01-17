/**
 * @file screenshot.h
 * @brief Screenshot Module - SD Card Only
 * 
 * Takes screenshots and saves them directly to SD card.
 * Triggered by three-finger swipe gesture.
 * Naming: Screenshot_001.bmp, Screenshot_002.bmp, etc.
 */

#ifndef __SCREENSHOT_H_
#define __SCREENSHOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize screenshot module
 */
esp_err_t screenshot_init(void);

/**
 * @brief Deinitialize screenshot module
 */
esp_err_t screenshot_deinit(void);

/**
 * @brief Take a screenshot of the current display
 * @param screen Screen to capture (NULL for active screen with all layers)
 * @note This captures the entire display including:
 *       - Active screen (lv_scr_act) and all its children
 *       - Top layer (lv_layer_top) popups and dialogs
 *       - Any overlay UI elements
 */
esp_err_t screenshot_take(lv_obj_t *screen);

/**
 * @brief Get the path of the last saved screenshot
 */
const char* screenshot_get_last_path(void);

/**
 * @brief Check if SD card is available for screenshots
 */
bool screenshot_is_sd_available(void);

#ifdef __cplusplus
}
#endif

#endif /* __SCREENSHOT_H_ */

