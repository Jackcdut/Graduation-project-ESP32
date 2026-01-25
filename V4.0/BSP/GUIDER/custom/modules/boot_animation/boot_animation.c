/*
* Boot Animation Implementation
* Displays school logo during system initialization with smooth animations
*/

#include "boot_animation.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BOOT_ANIM";

/* Declare the school logo image */
LV_IMG_DECLARE(_3_alpha_508x384);

/* Boot animation objects */
static lv_obj_t *boot_screen = NULL;
static lv_obj_t *logo_img = NULL;
static lv_obj_t *bg_obj = NULL;

/* Animation state */
static bool animation_active = false;
static void (*g_transition_cb)(void) = NULL;

/* Forward declarations */
static void fade_in_anim_cb(void * var, int32_t value);
static void fade_out_anim_cb(void * var, int32_t value);
static void fade_out_complete_cb(lv_anim_t * anim);

/**
 * @brief Create and display boot animation screen
 */
void boot_animation_create(lv_display_t *disp)
{
    ESP_LOGI(TAG, "Creating boot animation screen");
    
    /* Create a new screen for boot animation */
    boot_screen = lv_obj_create(NULL);
    
    /* Create black background */
    bg_obj = lv_obj_create(boot_screen);
    lv_obj_set_size(bg_obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(bg_obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(bg_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bg_obj, 0, 0);
    lv_obj_set_style_radius(bg_obj, 0, 0);  /* No rounded corners */
    lv_obj_clear_flag(bg_obj, LV_OBJ_FLAG_SCROLLABLE);

    /* Create logo image in the center, moved up by 20 pixels */
    logo_img = lv_img_create(boot_screen);
    lv_img_set_src(logo_img, &_3_alpha_508x384);
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, -20);

    /* Logo is fully visible (no animation) */
    lv_obj_set_style_img_opa(logo_img, LV_OPA_COVER, 0);

    /* Directly set the boot screen as active, bypassing lv_scr_load which can crash */
    lv_disp_t *disp_ptr = lv_disp_get_default();
    if (disp_ptr != NULL && boot_screen != NULL) {
        disp_ptr->act_scr = boot_screen;
        disp_ptr->scr_to_load = NULL;
        disp_ptr->prev_scr = NULL;
        disp_ptr->del_prev = false;
        lv_obj_invalidate(boot_screen);
    }

    ESP_LOGI(TAG, "Boot animation screen created and displayed - logo size: 533x378");
    animation_active = true;
}

/**
 * @brief Fade-in animation callback
 */
static void fade_in_anim_cb(void * var, int32_t value)
{
    lv_obj_set_style_img_opa((lv_obj_t *)var, value, 0);
}

/**
 * @brief Start the boot animation with fade-in effect
 */
void boot_animation_start(void)
{
    if (!logo_img || !animation_active) {
        ESP_LOGW(TAG, "Boot animation not initialized");
        return;
    }
    
    ESP_LOGI(TAG, "Starting boot animation fade-in");
    
    /* Create fade-in animation for logo */
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, logo_img);
    lv_anim_set_exec_cb(&anim, fade_in_anim_cb);
    lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&anim, 1000);  /* 1 second fade-in */
    lv_anim_set_delay(&anim, 200);   /* Small delay before starting */
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

/**
 * @brief Fade-out animation callback
 */
static void fade_out_anim_cb(void * var, int32_t value)
{
    lv_obj_set_style_img_opa((lv_obj_t *)var, value, 0);
}

/**
 * @brief Fade-out complete callback
 */
static void fade_out_complete_cb(lv_anim_t * anim)
{
    ESP_LOGI(TAG, "Boot animation fade-out complete");
    
    /* Call the transition complete callback if set */
    if (g_transition_cb) {
        g_transition_cb();
        g_transition_cb = NULL;
    }
}

/**
 * @brief Transition from boot animation to main UI
 */
void boot_animation_transition_to_main(lv_ui *ui, void (*transition_complete_cb)(void))
{
    if (!logo_img || !animation_active) {
        ESP_LOGW(TAG, "Boot animation not active");
        if (transition_complete_cb) {
            transition_complete_cb();
        }
        return;
    }
    
    ESP_LOGI(TAG, "Starting transition to main UI");
    
    /* Store the callback */
    g_transition_cb = transition_complete_cb;
    
    /* Create fade-out animation for logo */
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, logo_img);
    lv_anim_set_exec_cb(&anim, fade_out_anim_cb);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&anim, 800);  /* 0.8 second fade-out */
    lv_anim_set_delay(&anim, 1500); /* Wait 1.5 seconds before fading out */
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_set_ready_cb(&anim, fade_out_complete_cb);
    lv_anim_start(&anim);
    
    animation_active = false;
}

/**
 * @brief Delete the boot animation screen
 */
void boot_animation_delete(void)
{
    ESP_LOGI(TAG, "Deleting boot animation screen");
    
    if (boot_screen) {
        lv_obj_del(boot_screen);
        boot_screen = NULL;
        logo_img = NULL;
        bg_obj = NULL;
    }
    
    animation_active = false;
}

