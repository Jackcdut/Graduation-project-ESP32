/**
 * @file gallery.h
 * @brief Screenshot Gallery - SD Card Only
 * 
 * Displays and manages screenshots stored on SD card.
 */

#ifndef GALLERY_H
#define GALLERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "gui_guider.h"
#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize gallery module
 */
esp_err_t gallery_init(lv_ui *ui);

/**
 * @brief Deinitialize gallery module
 */
esp_err_t gallery_deinit(void);

/**
 * @brief Refresh gallery list
 */
esp_err_t gallery_refresh(void);

/**
 * @brief View an image
 */
esp_err_t gallery_view_image(const char *filename);

/**
 * @brief Close image viewer
 */
esp_err_t gallery_close_viewer(void);

/**
 * @brief Delete current image
 */
esp_err_t gallery_delete_current_image(void);

/**
 * @brief USB switch changed (legacy)
 */
void gallery_usb_switch_changed(lv_ui *ui, bool enabled);

/**
 * @brief Check if gallery is available
 */
bool gallery_is_available(void);

/**
 * @brief Show USB mode dialog (called from events_init.c)
 */
void gallery_show_usb_mode_dialog(void);

#ifdef __cplusplus
}
#endif

#endif /* GALLERY_H */

