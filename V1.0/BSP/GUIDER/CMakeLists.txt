file(GLOB_RECURSE SOURCES
    "generated/*.c"
    "custom/*.c"
    "custom/modules/screenshot/*.c"
    "custom/modules/gallery/*.c"
    "custom/modules/boot_animation/*.c"
    "custom/modules/oscilloscope/*.c"
    "custom/modules/widgets/*.c"
    "custom/modules/fonts/*.c"
    "custom/modules/sdcard_manager/*.c"
    "custom/modules/cloud_manager/*.c"
    "custom/modules/wireless_serial/*.c"
    "custom/modules/media_player/*.c"
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        "."
        "generated"
        "custom"
        "custom/modules/screenshot"
        "custom/modules/gallery"
        "custom/modules/boot_animation"
        "custom/modules/oscilloscope"
        "custom/modules/widgets"
        "custom/modules/fonts"
        "custom/modules/sdcard_manager"
        "custom/modules/cloud_manager"
        "custom/modules/wireless_serial"
        "custom/modules/media_player"
        "generated/guider_fonts"
        "generated/guider_customer_fonts"
        "generated/images"
    REQUIRES lvgl__lvgl esp_wifi esp_netif main fatfs espressif__esp_lcd_touch driver esp_driver_jpeg esp_driver_ppa spiffs sdmmc esp_http_client esp_http_server json mbedtls
    PRIV_REQUIRES esp_mm nvs_flash espressif__avi_player
)

# Add LV_LVGL_H_INCLUDE_SIMPLE definition for GUIDER component itself
target_compile_definitions(__idf_GUIDER PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)

# Enable LVGL SJPG (Tiny JPEG Decoder) globally for both GUIDER and LVGL components
# Required for tjpgd.h/tjpgd.c symbols (JDEC/JRECT/jd_prepare/jd_decomp)
target_compile_definitions(__idf_GUIDER PRIVATE CONFIG_LV_USE_SJPG=1 LV_USE_SJPG=1)
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
if(lvgl_lib)
    target_compile_definitions(${lvgl_lib} PUBLIC CONFIG_LV_USE_SJPG=1 LV_USE_SJPG=1)
endif()

