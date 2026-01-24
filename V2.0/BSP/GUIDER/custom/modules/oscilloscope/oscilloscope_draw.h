/**
 * @file oscilloscope_draw.h
 * @brief High-performance oscilloscope waveform drawing with hardware acceleration
 * 
 * Features:
 * - Direct canvas drawing (no chart widget overhead)
 * - ESP32-P4 PPA hardware acceleration for line drawing
 * - DMA2D acceleration for memory operations
 * - Optimized for 100Hz refresh rate (10ms timer)
 * - Real ADC data display
 */

#ifndef OSCILLOSCOPE_DRAW_H
#define OSCILLOSCOPE_DRAW_H

#include "lvgl.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Oscilloscope display parameters */
#define OSC_CANVAS_WIDTH    688     // Waveform area width (WAVEFORM_WIDTH - 4)
#define OSC_CANVAS_HEIGHT   381     // Waveform area height (WAVEFORM_HEIGHT - 4)
#define OSC_GRID_COLS       16      // Horizontal divisions
#define OSC_GRID_ROWS       9       // Vertical divisions

/* Color definitions */
#define OSC_COLOR_BG        0x000000    // Black background
#define OSC_COLOR_GRID      0x303030    // Dark gray grid
#define OSC_COLOR_WAVEFORM  0xFFFF00    // Yellow waveform
#define OSC_COLOR_FFT       0xE040FB    // Purple FFT

/**
 * @brief Oscilloscope drawing context
 */
typedef struct {
    lv_obj_t *canvas;               // Canvas object for direct drawing
    lv_color_t *canvas_buf;         // Canvas buffer (in PSRAM)
    lv_draw_ctx_t *draw_ctx;        // Drawing context for hardware acceleration
    
    // Waveform data
    int16_t *waveform_data;         // Current waveform Y coordinates (688 points)
    float *voltage_data;            // Voltage values for measurements
    
    // Hardware acceleration
    bool hw_accel_enabled;          // PPA hardware acceleration available
    
    // Performance monitoring
    uint32_t frame_count;
    uint32_t last_fps_time;
    float current_fps;
    
} osc_draw_ctx_t;

/**
 * @brief Waveform parameters
 */
typedef struct {
    float time_per_div;             // Time scale (s/div)
    float volts_per_div;            // Voltage scale (V/div)
    float x_offset;                 // Horizontal offset (s)
    float y_offset;                 // Vertical offset (V)
    bool fft_mode;                  // FFT mode enabled
    
    // Real ADC data
    const float *voltage_buffer;    // Pointer to voltage data array
    uint32_t voltage_count;         // Number of voltage samples
} osc_waveform_params_t;

/**
 * @brief Initialize oscilloscope drawing context
 * 
 * @param parent Parent LVGL object to attach canvas to
 * @param x X position
 * @param y Y position
 * @return Oscilloscope drawing context, or NULL on error
 */
osc_draw_ctx_t *osc_draw_init(lv_obj_t *parent, int x, int y);

/**
 * @brief Deinitialize oscilloscope drawing context
 * 
 * @param ctx Drawing context
 */
void osc_draw_deinit(osc_draw_ctx_t *ctx);

/**
 * @brief Clear canvas (fill with background color)
 * 
 * @param ctx Drawing context
 */
void osc_draw_clear(osc_draw_ctx_t *ctx);

/**
 * @brief Draw grid lines
 * 
 * @param ctx Drawing context
 */
void osc_draw_grid(osc_draw_ctx_t *ctx);

/**
 * @brief Draw waveform using hardware acceleration
 * 
 * @param ctx Drawing context
 * @param params Waveform parameters
 */
void osc_draw_waveform(osc_draw_ctx_t *ctx, const osc_waveform_params_t *params);

/**
 * @brief Draw FFT spectrum
 * 
 * @param ctx Drawing context
 * @param params Waveform parameters
 */
void osc_draw_fft(osc_draw_ctx_t *ctx, const osc_waveform_params_t *params);

/**
 * @brief Update canvas display (call after drawing)
 * 
 * @param ctx Drawing context
 */
void osc_draw_update(osc_draw_ctx_t *ctx);

/**
 * @brief Get current FPS
 * 
 * @param ctx Drawing context
 * @return Current FPS
 */
float osc_draw_get_fps(osc_draw_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // OSCILLOSCOPE_DRAW_H
