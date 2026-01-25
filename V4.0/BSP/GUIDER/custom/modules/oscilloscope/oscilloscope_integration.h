/**
 * @file oscilloscope_integration.h
 * @brief Integration layer between oscilloscope core and UI
 */

#ifndef OSCILLOSCOPE_INTEGRATION_H
#define OSCILLOSCOPE_INTEGRATION_H

#include "oscilloscope_core.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global oscilloscope core context - accessible by event handlers */
extern osc_core_ctx_t *g_osc_core;

/**
 * @brief Initialize oscilloscope integration
 */
esp_err_t osc_integration_init(void);

/**
 * @brief Deinitialize oscilloscope integration
 */
void osc_integration_deinit(void);

/**
 * @brief Start oscilloscope (RUN mode)
 */
esp_err_t osc_integration_start(void);

/**
 * @brief Stop oscilloscope (STOP mode)
 */
esp_err_t osc_integration_stop(void);

/**
 * @brief Check if oscilloscope is running
 */
bool osc_integration_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // OSCILLOSCOPE_INTEGRATION_H
