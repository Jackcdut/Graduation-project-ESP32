/**
 * @file oscilloscope_integration.c
 * @brief Integration layer between oscilloscope core and UI
 */

#include "oscilloscope_integration.h"
#include "oscilloscope_core.h"
#include "oscilloscope_adc.h"
#include "esp_log.h"

static const char *TAG = "OscIntegration";

/* Global oscilloscope context - exported for use by event handlers */
osc_core_ctx_t *g_osc_core = NULL;

/**
 * @brief Initialize oscilloscope integration
 */
esp_err_t osc_integration_init(void)
{
    if (g_osc_core != NULL) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    // Initialize oscilloscope core (includes ADC)
    g_osc_core = osc_core_init();
    if (g_osc_core == NULL) {
        ESP_LOGE(TAG, "Failed to initialize oscilloscope core");
        return ESP_FAIL;
    }
    
    // Set default configuration
    osc_core_set_time_scale(g_osc_core, OSC_TIME_2MS);
    osc_core_set_volt_scale(g_osc_core, OSC_VOLT_1V);
    
    // Configure trigger
    osc_trigger_config_t trigger = {
        .enabled = true,
        .level_voltage = 1.65f,  // Mid-range for 3.3V ADC
        .rising_edge = true,
        .pre_trigger_ratio = 0.5f,
    };
    osc_core_set_trigger(g_osc_core, &trigger);
    
    ESP_LOGI(TAG, "Oscilloscope integration initialized");
    return ESP_OK;
}

/**
 * @brief Deinitialize oscilloscope integration
 */
void osc_integration_deinit(void)
{
    if (g_osc_core != NULL) {
        osc_core_deinit(g_osc_core);
        g_osc_core = NULL;
    }
    ESP_LOGI(TAG, "Oscilloscope integration deinitialized");
}

/**
 * @brief Start oscilloscope (RUN mode)
 */
esp_err_t osc_integration_start(void)
{
    if (g_osc_core == NULL) return ESP_ERR_INVALID_STATE;
    return osc_core_start(g_osc_core);
}

/**
 * @brief Stop oscilloscope (STOP mode)
 */
esp_err_t osc_integration_stop(void)
{
    if (g_osc_core == NULL) return ESP_ERR_INVALID_STATE;
    return osc_core_stop(g_osc_core);
}

/**
 * @brief Check if oscilloscope is running
 */
bool osc_integration_is_running(void)
{
    if (g_osc_core == NULL) return false;
    return (osc_core_get_state(g_osc_core) == OSC_STATE_RUNNING);
}
