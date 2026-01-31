/**
 * @file oscilloscope_ui.c
 * @brief 示波器绘图与UI模块
 * 
 * 功能：
 * - 波形绘制
 * - FFT频谱分析与绘制（使用ESP-DSP硬件加速）
 * - 谐波分析（H1-H5, THD）
 */

#include "oscilloscope.h"
#include "esp_dsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <string.h>

static const char *TAG = "OSC_UI";

/*============================================================================
 * 绘图上下文结构
 *============================================================================*/
struct osc_draw_ctx_t {
    lv_obj_t *canvas;
    lv_color_t *canvas_buf;
    int16_t *waveform_y;        // Y坐标缓存
    float *voltage_cache;       // 电压缓存
    
    /* FFT缓冲区（16字节对齐，ESP-DSP要求） */
    float *fft_input;           // FFT输入（交织格式：real,imag,real,imag...）
    float *fft_magnitude;       // FFT幅度谱
    osc_fft_result_t fft_result;
    bool fft_initialized;
    
    /* 性能统计 */
    uint32_t frame_count;
    uint32_t last_fps_time;
    float current_fps;
};

/*============================================================================
 * FFT常量
 *============================================================================*/

/* 噪声门限 - 低于此值的信号视为噪声 */
#define FFT_NOISE_FLOOR             0.001f

/*============================================================================
 * FFT辅助函数
 *============================================================================*/

/**
 * @brief 抛物线插值精确查找峰值位置和幅度
 * 
 * @param left   左侧bin幅度
 * @param center 中心bin幅度
 * @param right  右侧bin幅度
 * @param delta  输出：峰值相对于中心bin的偏移量 (-0.5 ~ +0.5)
 * @return 插值后的峰值幅度
 */
static float parabolic_interpolation(float left, float center, float right, float *delta)
{
    float denom = left - 2.0f * center + right;
    if (fabsf(denom) < 1e-10f) {
        *delta = 0.0f;
        return center;
    }
    
    *delta = 0.5f * (left - right) / denom;
    if (*delta < -0.5f) *delta = -0.5f;
    if (*delta > 0.5f) *delta = 0.5f;
    
    float peak = center - 0.25f * (left - right) * (*delta);
    return peak;
}

/**
 * @brief 查找基频bin（带抛物线插值和DC信号检测）
 * 
 * @param magnitude 幅度谱数组
 * @param num_bins  bin数量
 * @param peak_mag  输出：插值后的峰值幅度
 * @param precise_bin 输出：精确的bin位置（含小数部分），DC信号返回0
 * @return 峰值所在的整数bin索引，DC信号返回0
 */
static int find_fundamental_bin(const float *magnitude, int num_bins, 
                                float *peak_mag, float *precise_bin)
{
    float max_mag = 0.0f;
    int peak_bin = 0;
    
    /* 跳过DC(bin 0)和极低频(bin 1)，从bin 2开始搜索 */
    for (int i = 2; i < num_bins - 1; i++) {
        if (magnitude[i] > max_mag) {
            max_mag = magnitude[i];
            peak_bin = i;
        }
    }
    
    /* DC信号检测：如果DC分量远大于AC分量，判定为直流信号 */
    float dc_mag = magnitude[0];
    if (dc_mag > max_mag * 10.0f || max_mag < FFT_NOISE_FLOOR) {
        /* 直流信号：频率为0 */
        *peak_mag = dc_mag;
        *precise_bin = 0.0f;
        return 0;
    }
    
    /* 使用抛物线插值精确定位 */
    if (peak_bin > 1 && peak_bin < num_bins - 1 && max_mag > FFT_NOISE_FLOOR) {
        float delta;
        *peak_mag = parabolic_interpolation(
            magnitude[peak_bin - 1],
            magnitude[peak_bin],
            magnitude[peak_bin + 1],
            &delta
        );
        *precise_bin = (float)peak_bin + delta;
    } else {
        *peak_mag = max_mag;
        *precise_bin = (float)peak_bin;
    }
    
    return peak_bin;
}

/**
 * @brief 获取谐波幅度（带抛物线插值）
 */
static float get_harmonic_amplitude(const float *magnitude, int num_bins, float target_bin)
{
    int bin = (int)(target_bin + 0.5f);
    
    if (bin <= 1 || bin >= num_bins - 1) return 0.0f;
    
    float left = magnitude[bin - 1];
    float center = magnitude[bin];
    float right = magnitude[bin + 1];
    
    if (center < FFT_NOISE_FLOOR && left < FFT_NOISE_FLOOR && right < FFT_NOISE_FLOOR) {
        return 0.0f;
    }
    
    float delta;
    return parabolic_interpolation(left, center, right, &delta);
}

/**
 * @brief 计算总谐波失真 (THD)
 * THD = sqrt(H2² + H3² + H4² + H5²) / H1 × 100%
 */
static float calculate_thd(float h1, float h2, float h3, float h4, float h5)
{
    if (h1 < FFT_NOISE_FLOOR) return 0.0f;
    
    float sum_sq = h2*h2 + h3*h3 + h4*h4 + h5*h5;
    float thd = sqrtf(sum_sq) / h1 * 100.0f;
    
    if (thd > 999.9f) thd = 999.9f;
    return thd;
}

/*============================================================================
 * 绘图API实现
 *============================================================================*/

osc_draw_ctx_t *osc_draw_init(lv_obj_t *parent, int x, int y)
{
    ESP_LOGI(TAG, "初始化绘图上下文");
    
    osc_draw_ctx_t *ctx = heap_caps_calloc(1, sizeof(osc_draw_ctx_t), MALLOC_CAP_8BIT);
    if (!ctx) return NULL;
    
    size_t buf_size = OSC_DISPLAY_WIDTH * OSC_DISPLAY_HEIGHT * sizeof(lv_color_t);
    ctx->canvas_buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    ctx->waveform_y = heap_caps_malloc(OSC_DISPLAY_WIDTH * sizeof(int16_t), MALLOC_CAP_8BIT);
    ctx->voltage_cache = heap_caps_malloc(OSC_DISPLAY_WIDTH * sizeof(float), MALLOC_CAP_8BIT);
    
    /* FFT缓冲区（16字节对齐） */
    ctx->fft_input = heap_caps_aligned_alloc(16, OSC_FFT_SIZE * 2 * sizeof(float), MALLOC_CAP_8BIT);
    ctx->fft_magnitude = heap_caps_malloc(OSC_FFT_BINS * sizeof(float), MALLOC_CAP_8BIT);
    
    if (!ctx->canvas_buf || !ctx->waveform_y || !ctx->voltage_cache ||
        !ctx->fft_input || !ctx->fft_magnitude) {
        ESP_LOGE(TAG, "内存分配失败");
        if (ctx->canvas_buf) heap_caps_free(ctx->canvas_buf);
        if (ctx->waveform_y) heap_caps_free(ctx->waveform_y);
        if (ctx->voltage_cache) heap_caps_free(ctx->voltage_cache);
        if (ctx->fft_input) heap_caps_free(ctx->fft_input);
        if (ctx->fft_magnitude) heap_caps_free(ctx->fft_magnitude);
        free(ctx);
        return NULL;
    }
    
    ctx->canvas = lv_canvas_create(parent);
    if (!ctx->canvas) {
        heap_caps_free(ctx->canvas_buf);
        heap_caps_free(ctx->waveform_y);
        heap_caps_free(ctx->voltage_cache);
        heap_caps_free(ctx->fft_input);
        heap_caps_free(ctx->fft_magnitude);
        free(ctx);
        return NULL;
    }
    
    lv_canvas_set_buffer(ctx->canvas, ctx->canvas_buf, 
                         OSC_DISPLAY_WIDTH, OSC_DISPLAY_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(ctx->canvas, x, y);
    lv_obj_set_size(ctx->canvas, OSC_DISPLAY_WIDTH, OSC_DISPLAY_HEIGHT);
    
    /* 初始化ESP-DSP FFT */
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, OSC_FFT_SIZE);
    if (ret == ESP_OK) {
        ctx->fft_initialized = true;
        ESP_LOGI(TAG, "ESP-DSP FFT初始化成功");
    } else {
        ctx->fft_initialized = false;
        ESP_LOGW(TAG, "ESP-DSP FFT初始化失败，将使用软件实现");
    }
    
    ctx->last_fps_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "绘图上下文初始化完成");
    return ctx;
}

void osc_draw_deinit(osc_draw_ctx_t *ctx)
{
    if (!ctx) return;
    if (ctx->canvas) lv_obj_del(ctx->canvas);
    if (ctx->voltage_cache) heap_caps_free(ctx->voltage_cache);
    if (ctx->waveform_y) heap_caps_free(ctx->waveform_y);
    if (ctx->canvas_buf) heap_caps_free(ctx->canvas_buf);
    if (ctx->fft_input) heap_caps_free(ctx->fft_input);
    if (ctx->fft_magnitude) heap_caps_free(ctx->fft_magnitude);
    free(ctx);
}

void osc_draw_clear(osc_draw_ctx_t *ctx)
{
    if (!ctx || !ctx->canvas) return;
    lv_canvas_fill_bg(ctx->canvas, lv_color_hex(OSC_COLOR_BG), LV_OPA_COVER);
}

void osc_draw_grid(osc_draw_ctx_t *ctx)
{
    if (!ctx || !ctx->canvas) return;
    
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(OSC_COLOR_GRID);
    line_dsc.width = 1;
    line_dsc.opa = LV_OPA_50;
    
    /* 垂直线 */
    int grid_x = OSC_DISPLAY_WIDTH / OSC_GRID_COLS;
    for (int i = 0; i <= OSC_GRID_COLS; i++) {
        int x = i * grid_x;
        lv_point_t pts[2] = {{x, 0}, {x, OSC_DISPLAY_HEIGHT}};
        lv_canvas_draw_line(ctx->canvas, pts, 2, &line_dsc);
    }
    
    /* 水平线 */
    int grid_y = OSC_DISPLAY_HEIGHT / OSC_GRID_ROWS;
    for (int i = 0; i <= OSC_GRID_ROWS; i++) {
        int y = i * grid_y;
        lv_point_t pts[2] = {{0, y}, {OSC_DISPLAY_WIDTH, y}};
        lv_canvas_draw_line(ctx->canvas, pts, 2, &line_dsc);
    }
    
    /* 中心线（更亮） */
    line_dsc.color = lv_color_hex(0xFFFFFF);
    line_dsc.opa = LV_OPA_70;
    
    int cy = OSC_DISPLAY_HEIGHT / 2;
    lv_point_t h_pts[2] = {{0, cy}, {OSC_DISPLAY_WIDTH, cy}};
    lv_canvas_draw_line(ctx->canvas, h_pts, 2, &line_dsc);
    
    int cx = OSC_DISPLAY_WIDTH / 2;
    lv_point_t v_pts[2] = {{cx, 0}, {cx, OSC_DISPLAY_HEIGHT}};
    lv_canvas_draw_line(ctx->canvas, v_pts, 2, &line_dsc);
}

void osc_draw_waveform(osc_draw_ctx_t *ctx, const float *data, uint32_t count,
                       float volts_per_div, float y_offset)
{
    if (!ctx || !ctx->canvas || !data || count == 0) return;
    
    const float center_y = OSC_DISPLAY_HEIGHT / 2.0f;
    float units_per_div = (float)OSC_DISPLAY_HEIGHT / (float)OSC_GRID_ROWS;
    float units_per_volt = units_per_div / volts_per_div;
    
    /* 重采样到显示宽度 */
    for (int i = 0; i < OSC_DISPLAY_WIDTH; i++) {
        float voltage;
        if (count >= (uint32_t)OSC_DISPLAY_WIDTH) {
            uint32_t idx = (i * count) / OSC_DISPLAY_WIDTH;
            if (idx >= count) idx = count - 1;
            voltage = data[idx];
        } else {
            float pos = (float)i * count / OSC_DISPLAY_WIDTH;
            uint32_t idx = (uint32_t)pos;
            if (idx >= count - 1) {
                voltage = data[count - 1];
            } else {
                float frac = pos - idx;
                voltage = data[idx] * (1.0f - frac) + data[idx + 1] * frac;
            }
        }
        
        ctx->voltage_cache[i] = voltage;
        float y = center_y - ((voltage + y_offset) * units_per_volt);
        int yi = (int)(y + 0.5f);
        if (yi < 0) yi = 0;
        if (yi >= OSC_DISPLAY_HEIGHT) yi = OSC_DISPLAY_HEIGHT - 1;
        ctx->waveform_y[i] = yi;
    }
    
    /* 绘制波形 */
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(OSC_COLOR_WAVEFORM);
    line_dsc.width = 2;
    line_dsc.opa = LV_OPA_COVER;
    
    for (int i = 0; i < OSC_DISPLAY_WIDTH - 1; i++) {
        lv_point_t pts[2] = {
            {i, ctx->waveform_y[i]},
            {i + 1, ctx->waveform_y[i + 1]}
        };
        lv_canvas_draw_line(ctx->canvas, pts, 2, &line_dsc);
    }
}

void osc_draw_fft(osc_draw_ctx_t *ctx, const float *data, uint32_t count,
                  float sample_rate, osc_fft_result_t *result)
{
    if (!ctx || !ctx->canvas) return;
    
    memset(&ctx->fft_result, 0, sizeof(osc_fft_result_t));
    
    if (!data || count == 0 || sample_rate <= 0.0f) {
        if (result) memcpy(result, &ctx->fft_result, sizeof(osc_fft_result_t));
        return;
    }
    
    ctx->fft_result.sample_rate = sample_rate;
    ctx->fft_result.freq_resolution = sample_rate / (float)OSC_FFT_SIZE;
    
    /* 准备FFT输入数据 */
    for (int i = 0; i < OSC_FFT_SIZE; i++) {
        float v;
        if (count >= (uint32_t)OSC_FFT_SIZE) {
            uint32_t idx = (i * count) / OSC_FFT_SIZE;
            if (idx >= count) idx = count - 1;
            v = data[idx];
        } else {
            float pos = (float)i * count / OSC_FFT_SIZE;
            uint32_t idx = (uint32_t)pos;
            if (idx >= count - 1) {
                v = data[count - 1];
            } else {
                float frac = pos - idx;
                v = data[idx] * (1.0f - frac) + data[idx + 1] * frac;
            }
        }
        /* 交织格式：[real0, imag0, real1, imag1, ...] */
        ctx->fft_input[i * 2] = v;
        ctx->fft_input[i * 2 + 1] = 0.0f;
    }
    
    /* 应用汉宁窗 */
    for (int i = 0; i < OSC_FFT_SIZE; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (OSC_FFT_SIZE - 1)));
        ctx->fft_input[i * 2] *= window;
    }
    
    /* 执行FFT */
    if (ctx->fft_initialized) {
        dsps_fft2r_fc32(ctx->fft_input, OSC_FFT_SIZE);
        dsps_bit_rev_fc32(ctx->fft_input, OSC_FFT_SIZE);
        
        /* 计算幅度谱 */
        for (int i = 0; i < OSC_FFT_BINS; i++) {
            float real = ctx->fft_input[i * 2];
            float imag = ctx->fft_input[i * 2 + 1];
            ctx->fft_magnitude[i] = sqrtf(real * real + imag * imag);
        }
    } else {
        /* 软件DFT回退 */
        for (int k = 0; k < OSC_FFT_BINS; k++) {
            float real_sum = 0.0f, imag_sum = 0.0f;
            for (int t = 0; t < OSC_FFT_SIZE; t++) {
                float angle = -2.0f * M_PI * k * t / OSC_FFT_SIZE;
                real_sum += ctx->fft_input[t * 2] * cosf(angle);
                imag_sum += ctx->fft_input[t * 2] * sinf(angle);
            }
            ctx->fft_magnitude[k] = sqrtf(real_sum * real_sum + imag_sum * imag_sum);
        }
    }
    
    /*
     * FFT幅度归一化说明：
     * 
     * 对于输入信号 x(t) = A*sin(2πf*t) + DC:
     * - FFT输出的幅度 |X[k]| 需要除以N得到正确的幅度
     * - 单边频谱：除DC和Nyquist外，其他频率分量需要乘2
     * - 汉宁窗补偿：汉宁窗的相干增益为0.5，需要乘2补偿
     * 
     * 最终：
     * - DC分量: |X[0]| / N * 2 (汉宁窗补偿)
     * - AC分量: |X[k]| / N * 2 (单边) * 2 (汉宁窗) = |X[k]| / N * 4
     */
    
    /* 归一化幅度谱 */
    for (int i = 0; i < OSC_FFT_BINS; i++) {
        ctx->fft_magnitude[i] /= (float)OSC_FFT_SIZE;
        
        if (i == 0) {
            /* DC分量：只需汉宁窗补偿 */
            ctx->fft_magnitude[i] *= 2.0f;
        } else {
            /* AC分量：单边频谱补偿 + 汉宁窗补偿 */
            ctx->fft_magnitude[i] *= 4.0f;
        }
    }
    
    /* 查找基频（带抛物线插值和DC检测） */
    float peak_mag;
    float precise_bin;
    int fund_bin = find_fundamental_bin(ctx->fft_magnitude, OSC_FFT_BINS, &peak_mag, &precise_bin);
    
    /* DC分量 */
    ctx->fft_result.dc_offset = ctx->fft_magnitude[0];
    
    /* 基频和幅度 */
    ctx->fft_result.fundamental_freq = precise_bin * ctx->fft_result.freq_resolution;
    ctx->fft_result.fundamental_amp = peak_mag;
    
    /* 计算谐波（仅对AC信号有效） */
    if (fund_bin > 1) {
        ctx->fft_result.h2_amp = get_harmonic_amplitude(ctx->fft_magnitude, OSC_FFT_BINS, precise_bin * 2.0f);
        ctx->fft_result.h3_amp = get_harmonic_amplitude(ctx->fft_magnitude, OSC_FFT_BINS, precise_bin * 3.0f);
        ctx->fft_result.h4_amp = get_harmonic_amplitude(ctx->fft_magnitude, OSC_FFT_BINS, precise_bin * 4.0f);
        ctx->fft_result.h5_amp = get_harmonic_amplitude(ctx->fft_magnitude, OSC_FFT_BINS, precise_bin * 5.0f);
        
        /* 计算THD */
        ctx->fft_result.thd = calculate_thd(
            ctx->fft_result.fundamental_amp,
            ctx->fft_result.h2_amp,
            ctx->fft_result.h3_amp,
            ctx->fft_result.h4_amp,
            ctx->fft_result.h5_amp
        );
    } else {
        /* DC信号：谐波为0 */
        ctx->fft_result.h2_amp = 0.0f;
        ctx->fft_result.h3_amp = 0.0f;
        ctx->fft_result.h4_amp = 0.0f;
        ctx->fft_result.h5_amp = 0.0f;
        ctx->fft_result.thd = 0.0f;
    }
    
    ctx->fft_result.valid = true;
    
    /* 转换为显示坐标（dB刻度） */
    float max_mag = 0.0f;
    for (int i = 0; i < OSC_FFT_BINS; i++) {
        if (ctx->fft_magnitude[i] > max_mag) max_mag = ctx->fft_magnitude[i];
    }
    if (max_mag < FFT_NOISE_FLOOR) max_mag = FFT_NOISE_FLOOR;
    
    for (int i = 0; i < OSC_DISPLAY_WIDTH; i++) {
        int bin = (i * OSC_FFT_BINS) / OSC_DISPLAY_WIDTH;
        if (bin >= OSC_FFT_BINS) bin = OSC_FFT_BINS - 1;
        
        float norm = ctx->fft_magnitude[bin] / max_mag;
        float db = 20.0f * log10f(norm + 0.001f);
        float db_norm = (db + 60.0f) / 60.0f;
        if (db_norm < 0.0f) db_norm = 0.0f;
        if (db_norm > 1.0f) db_norm = 1.0f;
        
        int y = OSC_DISPLAY_HEIGHT - 1 - (int)(db_norm * (OSC_DISPLAY_HEIGHT - 20));
        if (y < 0) y = 0;
        if (y >= OSC_DISPLAY_HEIGHT) y = OSC_DISPLAY_HEIGHT - 1;
        ctx->waveform_y[i] = y;
    }
    
    /* 绘制频谱 */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(OSC_COLOR_FFT);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.border_width = 0;
    
    for (int i = 0; i < OSC_DISPLAY_WIDTH; i += 2) {
        int y_top = ctx->waveform_y[i];
        int height = OSC_DISPLAY_HEIGHT - 1 - y_top;
        if (height > 0) {
            lv_canvas_draw_rect(ctx->canvas, i, y_top, 2, height, &rect_dsc);
        }
    }
    
    if (result) memcpy(result, &ctx->fft_result, sizeof(osc_fft_result_t));
}

void osc_draw_update(osc_draw_ctx_t *ctx)
{
    if (!ctx || !ctx->canvas) return;
    lv_obj_invalidate(ctx->canvas);
    
    ctx->frame_count++;
    uint32_t now = esp_timer_get_time();
    uint32_t elapsed = now - ctx->last_fps_time;
    
    if (elapsed >= 1000000) {
        ctx->current_fps = (float)ctx->frame_count * 1000000.0f / (float)elapsed;
        ctx->frame_count = 0;
        ctx->last_fps_time = now;
    }
}

float osc_draw_get_fps(osc_draw_ctx_t *ctx)
{
    return ctx ? ctx->current_fps : 0.0f;
}

const osc_fft_result_t *osc_draw_get_fft_result(osc_draw_ctx_t *ctx)
{
    return ctx ? &ctx->fft_result : NULL;
}
