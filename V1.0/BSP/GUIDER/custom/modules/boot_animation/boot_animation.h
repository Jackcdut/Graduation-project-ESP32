/*
* Boot Animation Header
* Displays school logo during system initialization
*/

#ifndef __BOOT_ANIMATION_H_
#define __BOOT_ANIMATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "esp_lvgl_port_compatibility.h"
#include "gui_guider.h"

/**
 * @brief Create and display boot animation screen
 *
 * This function creates a boot animation screen with the school logo
 * centered on a black background. The logo will fade in smoothly.
 *
 * @param disp Pointer to the LVGL display object
 */
void boot_animation_create(lv_display_t *disp);

/**
 * @brief Start the boot animation with fade-in effect
 * 
 * This function starts the fade-in animation for the logo
 */
void boot_animation_start(void);

/**
 * @brief Transition from boot animation to main UI
 * 
 * This function performs a smooth transition from the boot animation
 * to the main user interface with fade-out effect.
 * 
 * @param ui Pointer to the main UI structure
 * @param transition_complete_cb Callback function called when transition is complete
 */
void boot_animation_transition_to_main(lv_ui *ui, void (*transition_complete_cb)(void));

/**
 * @brief Delete the boot animation screen
 * 
 * This function cleans up and deletes the boot animation screen
 */
void boot_animation_delete(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_ANIMATION_H_ */

