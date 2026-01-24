/**
 * @file oscilloscope_draw.c
 * @brief High-performance oscilloscope waveform drawing implementation
 */

#include "oscilloscope_draw.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <string.h>

static const char *TAG = "OscDraw";

/**
 * @brief Initialize oscilloscope drawing context
 */
osc_draw_ctx_t *osc_draw_init(lv_obj_t *parent, int x, int y)
{
    ESP_LOGI(TAG, "Initializing oscilloscope drawing context");
    
    // Allocate context
    osc_draw_ctx_t *ctx = heap_caps_malloc(sizeof(osc_draw_ctx_t), MALLOC_CAP_8BIT);
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return NULL;
    }
    memset(ctx, 0, sizeof(osc_draw_ctx_t));
    
    // Allocate canvas buffer in PSRAM (RGB565 format, 2 bytes per pixel)
    size_t canvas_buf_size = OSC_CANVAS_WIDTH * OSC_CANVAS_HEIGHT * sizeof(lv_color_t);
    ctx->canvas_buf = heap_caps_malloc(canvas_buf_size, MALLOC_CAP_SPIRAM);
    if (ctx->canvas_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate canvas buffer (%zu bytes)", canvas_buf_size);
        free(ctx);
        return NULL;
    }
    ESP_LOGI(TAG, "Canvas buffer allocated: %zu bytes in PSRAM", canvas_buf_size);
    
    // Allocate waveform data buffer
    ctx->waveform_data = heap_caps_malloc(OSC_CANVAS_WIDTH * sizeof(int16_t), MALLOC_CAP_8BIT);
    if (ctx->waveform_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate waveform data");
        heap_caps_free(ctx->canvas_buf);
        free(ctx);
        return NULL;
    }
    
    // Allocate voltage data buffer
    ctx->voltage_data = heap_caps_malloc(OSC_CANVAS_WIDTH * sizeof(float), MALLOC_CAP_8BIT);
    if (ctx->voltage_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate voltage data");
        heap_caps_free(ctx->waveform_data);
        heap_caps_free(ctx->canvas_buf);
        free(ctx);
        return NULL;
    }
    
    // Create canvas object
    ctx->canvas = lv_canvas_create(parent);
    if (ctx->canvas == NULL) {
        ESP_LOGE(TAG, "Failed to create canvas");
        heap_caps_free(ctx->voltage_data);
        heap_caps_free(ctx->waveform_data);
        heap_caps_free(ctx->canvas_buf);
        free(ctx);
        return NULL;
    }
    
    // Set canvas buffer
    lv_canvas_set_buffer(ctx->canvas, ctx->canvas_buf, OSC_CANVAS_WIDTH, OSC_CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(ctx->canvas, x, y);
    lv_obj_set_size(ctx->canvas, OSC_CANVAS_WIDTH, OSC_CANVAS_HEIGHT);
    
    // Check for hardware acceleration support
    ctx->hw_accel_enabled = false;
#if CONFIG_SOC_PPA_SUPPORTED
    ctx->hw_accel_enabled = true;
    ESP_LOGI(TAG, "PPA hardware acceleration enabled");
#else
    ESP_LOGI(TAG, "PPA hardware acceleration not available");
#endif
    
    // Initialize performance monitoring
    ctx->frame_count = 0;
    ctx->last_fps_time = esp_timer_get_time();
    ctx->current_fps = 0.0f;
    
    ESP_LOGI(TAG, "Oscilloscope drawing context initialized successfully");
    return ctx;
}

/**
 * @brief Deinitialize oscilloscope drawing context
 */
void osc_draw_deinit(osc_draw_ctx_t *ctx)
{
    if (ctx == NULL) return;
    
    if (ctx->canvas) {
        lv_obj_del(ctx->canvas);
    }
    
    if (ctx->voltage_data) {
        heap_caps_free(ctx->voltage_data);
    }
    
    if (ctx->waveform_data) {
        heap_caps_free(ctx->waveform_data);
    }
    
    if (ctx->canvas_buf) {
        heap_caps_free(ctx->canvas_buf);
    }
    
    free(ctx);
    ESP_LOGI(TAG, "Oscilloscope drawing context deinitialized");
}

/**
 * @brief Clear canvas (fill with background color)
 */
void osc_draw_clear(osc_draw_ctx_t *ctx)
{
    if (ctx == NULL || ctx->canvas == NULL) return;
    
    lv_canvas_fill_bg(ctx->canvas, lv_color_hex(OSC_COLOR_BG), LV_OPA_COVER);
}

/**
 * @brief Draw grid lines (optimized with hardware acceleration if available)
 */
void osc_draw_grid(osc_draw_ctx_t *ctx)
{
    if (ctx == NULL || ctx->canvas == NULL) return;
    
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(OSC_COLOR_GRID);
    line_dsc.width = 1;
    line_dsc.opa = LV_OPA_50;  // Semi-transparent
    
    // Draw vertical grid lines
    int grid_spacing_x = OSC_CANVAS_WIDTH / OSC_GRID_COLS;
    for (int i = 0; i <= OSC_GRID_COLS; i++) {
        int x = i * grid_spacing_x;
        lv_point_t points[2] = {
            {.x = x, .y = 0},
            {.x = x, .y = OSC_CANVAS_HEIGHT}
        };
        lv_canvas_draw_line(ctx->canvas, points, 2, &line_dsc);
    }
    
    // Draw horizontal grid lines
    int grid_spacing_y = OSC_CANVAS_HEIGHT / OSC_GRID_ROWS;
    for (int i = 0; i <= OSC_GRID_ROWS; i++) {
        int y = i * grid_spacing_y;
        lv_point_t points[2] = {
            {.x = 0, .y = y},
            {.x = OSC_CANVAS_WIDTH, .y = y}
        };
        lv_canvas_draw_line(ctx->canvas, points, 2, &line_dsc);
    }
    
    // Draw center lines (brighter)
    line_dsc.color = lv_color_hex(0xFFFFFF);
    line_dsc.opa = LV_OPA_70;
    
    // Horizontal center line
    int center_y = OSC_CANVAS_HEIGHT / 2;
    lv_point_t h_points[2] = {
        {.x = 0, .y = center_y},
        {.x = OSC_CANVAS_WIDTH, .y = center_y}
    };
    lv_canvas_draw_line(ctx->canvas, h_points, 2, &line_dsc);
    
    // Vertical center line
    int center_x = OSC_CANVAS_WIDTH / 2;
    lv_point_t v_points[2] = {
        {.x = center_x, .y = 0},
        {.x = center_x, .y = OSC_CANVAS_HEIGHT}
    };
    lv_canvas_draw_line(ctx->canvas, v_points, 2, &line_dsc);
}

/**
 * @brief Draw waveform using real ADC data
 * 
 * Coordinate system:
 * - Y=0 (top) corresponds to maximum positive voltage
 * - Y=center (OSC_CANVAS_HEIGHT/2) corresponds to 0V (ground)
 * - Y=OSC_CANVAS_HEIGHT (bottom) corresponds to maximum negative voltage
 */
void osc_draw_waveform(osc_draw_ctx_t *ctx, const osc_waveform_params_t *params)
{
    if (ctx == NULL || ctx->canvas == NULL || params == NULL) return;
    
    const int num_points = OSC_CANVAS_WIDTH;
    const float chart_center = OSC_CANVAS_HEIGHT / 2.0f;  // 0V reference line (horizontal axis)
    
    // Calculate units per volt
    // Total vertical range is OSC_GRID_ROWS divisions
    float units_per_division = (float)OSC_CANVAS_HEIGHT / (float)OSC_GRID_ROWS;
    float units_per_volt = units_per_division / params->volts_per_div;
    
    // Debug: Log data status
    static uint32_t draw_counter = 0;
    draw_counter++;
    if (draw_counter % 500 == 0) {  // 减少日志频率：每 500 帧打印一次
        ESP_LOGI(TAG, "Draw #%lu: voltage_buffer=%p, voltage_count=%lu, V/div=%.3f", 
                 draw_counter, params->voltage_buffer, params->voltage_count, params->volts_per_div);
    }
    
    // Check if we have real ADC data
    if (params->voltage_buffer != NULL && params->voltage_count > 0) {
        // Use real ADC data
        if (draw_counter % 500 == 0) {  // 减少日志频率
            // Calculate voltage statistics for debugging
            float vmin = 0.0f, vmax = 0.0f;
            bool first = true;
            for (uint32_t i = 0; i < params->voltage_count; i++) {
                float v = params->voltage_buffer[i];
                if (first) {
                    vmin = v;
                    vmax = v;
                    first = false;
                }
                if (v < vmin) vmin = v;
                if (v > vmax) vmax = v;
            }
            ESP_LOGI(TAG, "Drawing REAL ADC data: %lu points, Vmin=%.3fV, Vmax=%.3fV, Vpp=%.3fV", 
                     params->voltage_count, vmin, vmax, vmax - vmin);
        }
        
        for (int i = 0; i < num_points; i++) {
            float voltage;
            
            // Sample from voltage buffer (with interpolation if needed)
            if (params->voltage_count >= (uint32_t)num_points) {
                // Downsample: pick every Nth sample
                uint32_t src_idx = (i * params->voltage_count) / num_points;
                if (src_idx >= params->voltage_count) src_idx = params->voltage_count - 1;
                voltage = params->voltage_buffer[src_idx];
            } else {
                // Upsample: interpolate between samples
                float src_pos = (float)i * params->voltage_count / num_points;
                uint32_t src_idx = (uint32_t)src_pos;
                if (src_idx >= params->voltage_count - 1) {
                    voltage = params->voltage_buffer[params->voltage_count - 1];
                } else {
                    float frac = src_pos - src_idx;
                    voltage = params->voltage_buffer[src_idx] * (1.0f - frac) + 
                             params->voltage_buffer[src_idx + 1] * frac;
                }
            }
            
            ctx->voltage_data[i] = voltage;
            
            // Convert voltage to Y coordinate
            // Positive voltage -> above center (smaller Y)
            // Negative voltage -> below center (larger Y)
            // Y = center - (voltage * units_per_volt)
            float y_float = chart_center - (voltage * units_per_volt);
            
            // Debug: Log conversion for first few points
            static uint32_t conversion_log_count = 0;
            if (draw_counter <= 3 && i < 5) {
                ESP_LOGI(TAG, "  Point[%d]: voltage=%.3fV, center=%.1f, units_per_volt=%.2f, y_float=%.1f", 
                         i, voltage, chart_center, units_per_volt, y_float);
                conversion_log_count++;
            }
            
            // Clamp to canvas bounds
            int y = (int)(y_float + 0.5f);
            if (y < 0) y = 0;
            if (y >= OSC_CANVAS_HEIGHT) y = OSC_CANVAS_HEIGHT - 1;
            
            ctx->waveform_data[i] = y;
        }
    } else {
        // No ADC data available - draw flat line at center (0V)
        if (draw_counter % 500 == 0) {
            ESP_LOGW(TAG, "NO ADC data - drawing flat line at 0V");
        }
        
        for (int i = 0; i < num_points; i++) {
            ctx->voltage_data[i] = 0.0f;
            ctx->waveform_data[i] = (int)chart_center;
        }
    }
    
    // Draw waveform using connected lines (hardware accelerated if available)
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(OSC_COLOR_WAVEFORM);
    line_dsc.width = 2;
    line_dsc.opa = LV_OPA_COVER;
    
    // Draw line segments
    for (int i = 0; i < num_points - 1; i++) {
        lv_point_t points[2] = {
            {.x = i, .y = ctx->waveform_data[i]},
            {.x = i + 1, .y = ctx->waveform_data[i + 1]}
        };
        lv_canvas_draw_line(ctx->canvas, points, 2, &line_dsc);
    }
}

/**
 * @brief Simple FFT implementation (Cooley-Tukey algorithm)
 * Note: This is a simplified version for real-time display
 */
static void simple_fft(const float *input, float *magnitude, int n)
{
    // For simplicity, we'll use a DFT approximation for the first N/2 frequencies
    // This is computationally expensive but works for small N
    
    for (int k = 0; k < n / 2; k++) {
        float real_sum = 0.0f;
        float imag_sum = 0.0f;
        
        for (int t = 0; t < n; t++) {
            float angle = -2.0f * M_PI * k * t / n;
            real_sum += input[t] * cosf(angle);
            imag_sum += input[t] * sinf(angle);
        }
        
        // Calculate magnitude
        magnitude[k] = sqrtf(real_sum * real_sum + imag_sum * imag_sum) / n;
    }
}

/**
 * @brief Draw FFT spectrum using real ADC data
 */
void osc_draw_fft(osc_draw_ctx_t *ctx, const osc_waveform_params_t *params)
{
    if (ctx == NULL || ctx->canvas == NULL || params == NULL) return;
    
    const int num_points = OSC_CANVAS_WIDTH;
    const int fft_size = 512;  // Use 512 points for FFT (power of 2)
    
    // Check if we have real ADC data
    if (params->voltage_buffer == NULL || params->voltage_count == 0) {
        // No data - draw empty spectrum at bottom
        for (int i = 0; i < num_points; i++) {
            ctx->waveform_data[i] = OSC_CANVAS_HEIGHT - 1;
        }
        return;
    }
    
    // Prepare input data for FFT
    static float fft_input[512];
    static float fft_magnitude[256];  // Only need N/2 for real signal
    
    // Sample or interpolate voltage data to FFT size
    for (int i = 0; i < fft_size; i++) {
        if (params->voltage_count >= (uint32_t)fft_size) {
            // Downsample
            uint32_t src_idx = (i * params->voltage_count) / fft_size;
            if (src_idx >= params->voltage_count) src_idx = params->voltage_count - 1;
            fft_input[i] = params->voltage_buffer[src_idx];
        } else {
            // Upsample with interpolation
            float src_pos = (float)i * params->voltage_count / fft_size;
            uint32_t src_idx = (uint32_t)src_pos;
            if (src_idx >= params->voltage_count - 1) {
                fft_input[i] = params->voltage_buffer[params->voltage_count - 1];
            } else {
                float frac = src_pos - src_idx;
                fft_input[i] = params->voltage_buffer[src_idx] * (1.0f - frac) + 
                              params->voltage_buffer[src_idx + 1] * frac;
            }
        }
    }
    
    // Apply Hanning window to reduce spectral leakage
    for (int i = 0; i < fft_size; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fft_size - 1)));
        fft_input[i] *= window;
    }
    
    // Perform FFT
    simple_fft(fft_input, fft_magnitude, fft_size);
    
    // Find maximum magnitude for normalization
    float max_magnitude = 0.0f;
    for (int i = 1; i < fft_size / 2; i++) {  // Skip DC component (i=0)
        if (fft_magnitude[i] > max_magnitude) {
            max_magnitude = fft_magnitude[i];
        }
    }
    
    if (max_magnitude < 0.001f) max_magnitude = 0.001f;  // Avoid division by zero
    
    // Convert FFT magnitude to display coordinates
    // Y-axis: 0 (top) = maximum amplitude, OSC_CANVAS_HEIGHT (bottom) = zero amplitude
    for (int i = 0; i < num_points; i++) {
        // Map display pixel to FFT bin
        int fft_bin = (i * (fft_size / 2)) / num_points;
        if (fft_bin >= fft_size / 2) fft_bin = fft_size / 2 - 1;
        
        // Normalize and convert to dB scale for better visualization
        float normalized = fft_magnitude[fft_bin] / max_magnitude;
        float db = 20.0f * log10f(normalized + 0.001f);  // Add small value to avoid log(0)
        
        // Map dB range (-60dB to 0dB) to display height
        // 0dB (max) -> y=20 (near top)
        // -60dB (min) -> y=OSC_CANVAS_HEIGHT-1 (bottom)
        float db_normalized = (db + 60.0f) / 60.0f;  // 0.0 to 1.0
        if (db_normalized < 0.0f) db_normalized = 0.0f;
        if (db_normalized > 1.0f) db_normalized = 1.0f;
        
        int y = OSC_CANVAS_HEIGHT - 1 - (int)(db_normalized * (OSC_CANVAS_HEIGHT - 20));
        
        if (y < 0) y = 0;
        if (y >= OSC_CANVAS_HEIGHT) y = OSC_CANVAS_HEIGHT - 1;
        
        ctx->waveform_data[i] = y;
    }
    
    // Draw FFT spectrum as filled bars (向上的峰值)
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(OSC_COLOR_FFT);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.border_width = 0;
    
    // Draw bars from bottom to the magnitude height
    for (int i = 0; i < num_points; i += 2) {  // Draw every 2nd bar for performance
        int y_top = ctx->waveform_data[i];
        int y_bottom = OSC_CANVAS_HEIGHT - 1;
        int height = y_bottom - y_top + 1;
        
        if (height > 0) {
            lv_canvas_draw_rect(ctx->canvas, i, y_top, 2, height, &rect_dsc);
        }
    }
}

/**
 * @brief Update canvas display
 */
void osc_draw_update(osc_draw_ctx_t *ctx)
{
    if (ctx == NULL || ctx->canvas == NULL) return;
    
    // Invalidate canvas to trigger redraw
    lv_obj_invalidate(ctx->canvas);
    
    // Update FPS counter
    ctx->frame_count++;
    uint32_t current_time = esp_timer_get_time();
    uint32_t elapsed = current_time - ctx->last_fps_time;
    
    if (elapsed >= 1000000) {  // Update FPS every second
        ctx->current_fps = (float)ctx->frame_count * 1000000.0f / (float)elapsed;
        ctx->frame_count = 0;
        ctx->last_fps_time = current_time;
        
        ESP_LOGI(TAG, "FPS: %.1f", ctx->current_fps);
    }
}

/**
 * @brief Get current FPS
 */
float osc_draw_get_fps(osc_draw_ctx_t *ctx)
{
    if (ctx == NULL) return 0.0f;
    return ctx->current_fps;
}
