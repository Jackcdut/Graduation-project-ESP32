idf_component_register(
    SRCS main.c
    INCLUDE_DIRS .
    REQUIRES GUIDER nvs_flash espressif__esp32_p4_function_ev_board bsp_extra esp_wifi esp_netif esp_event json esp_http_client lwip mbedtls driver)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB}
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
)
