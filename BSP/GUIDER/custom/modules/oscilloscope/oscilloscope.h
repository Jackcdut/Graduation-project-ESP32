/**
 * @file oscilloscope.h
 * @brief 示波器模块统一头文件
 * 
 * 功能：
 * - ADC采样与数据管理
 * - 波形测量（频率、电压等）
 * - FFT频谱分析
 * - 波形绘制
 * - 数据导出
 */

#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include "esp_err.h"
#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 常量定义
 *============================================================================*/

/* 显示参数 */
#define OSC_DISPLAY_WIDTH       688     // 波形显示宽度（像素）
#define OSC_DISPLAY_HEIGHT      381     // 波形显示高度（像素）
#define OSC_GRID_COLS           16      // 水平格数
#define OSC_GRID_ROWS           9       // 垂直格数

/* FFT参数 */
#define OSC_FFT_SIZE            1024    // FFT点数
#define OSC_FFT_BINS            512     // FFT频率bin数

/* 存储深度 */
#define OSC_MAX_STORAGE_DEPTH   (128 * 1024)  // 最大128K点

/* 电压范围 */
#define OSC_VOLTAGE_MIN         -50.0f  // 最小电压
#define OSC_VOLTAGE_MAX         50.0f   // 最大电压

/* 颜色定义 */
#define OSC_COLOR_BG            0x000000    // 背景色
#define OSC_COLOR_GRID          0x303030    // 网格色
#define OSC_COLOR_WAVEFORM      0xFFFF00    // 波形色（黄色）
#define OSC_COLOR_FFT           0xE040FB    // FFT色（紫色）

/*============================================================================
 * 枚举类型
 *============================================================================*/

/* 时基档位 */
typedef enum {
    OSC_TIME_8NS = 0,
    OSC_TIME_20NS, OSC_TIME_40NS, OSC_TIME_80NS,
    OSC_TIME_200NS, OSC_TIME_400NS, OSC_TIME_800NS,
    OSC_TIME_2US, OSC_TIME_4US, OSC_TIME_8US,
    OSC_TIME_20US, OSC_TIME_40US, OSC_TIME_80US,
    OSC_TIME_200US, OSC_TIME_400US, OSC_TIME_800US,
    OSC_TIME_2MS, OSC_TIME_4MS, OSC_TIME_8MS,
    OSC_TIME_20MS, OSC_TIME_40MS, OSC_TIME_80MS,
    OSC_TIME_200MS, OSC_TIME_400MS, OSC_TIME_800MS,
    OSC_TIME_2S, OSC_TIME_4S, OSC_TIME_8S,
    OSC_TIME_20S, OSC_TIME_40S,
    OSC_TIME_MAX
} osc_time_scale_t;

/* 电压档位 */
typedef enum {
    OSC_VOLT_10MV = 0,
    OSC_VOLT_20MV, OSC_VOLT_50MV,
    OSC_VOLT_100MV, OSC_VOLT_200MV, OSC_VOLT_500MV,
    OSC_VOLT_1V, OSC_VOLT_2V, OSC_VOLT_5V,
    OSC_VOLT_MAX
} osc_volt_scale_t;

/* 采样率 */
typedef enum {
    OSC_SAMPLE_1MSPS = 0,
    OSC_SAMPLE_500KSPS,
    OSC_SAMPLE_200KSPS,
    OSC_SAMPLE_100KSPS,
    OSC_SAMPLE_50KSPS,
    OSC_SAMPLE_10KSPS,
    OSC_SAMPLE_1KSPS,
} osc_sample_rate_t;

/* 运行状态 */
typedef enum {
    OSC_STATE_STOPPED = 0,
    OSC_STATE_RUNNING,
    OSC_STATE_WAITING,
} osc_state_t;

/* 工作模式 */
typedef enum {
    OSC_MODE_NORMAL = 0,
    OSC_MODE_ROLL,
    OSC_MODE_SINGLE,
} osc_mode_t;

/*============================================================================
 * 数据结构
 *============================================================================*/

/* 触发配置 */
typedef struct {
    bool enabled;               // 触发使能
    float level;                // 触发电平（V）
    bool rising_edge;           // true=上升沿
    float pre_trigger;          // 预触发比例（0-1）
} osc_trigger_t;

/* 测量结果 */
typedef struct {
    float freq;                 // 频率（Hz）
    float vmax;                 // 最大电压
    float vmin;                 // 最小电压
    float vpp;                  // 峰峰值
    float vrms;                 // 有效值
    float dc_offset;            // 直流偏置
    bool valid;                 // 数据有效
} osc_measurement_t;

/* FFT结果 */
typedef struct {
    float fundamental_freq;     // 基频（Hz）
    float fundamental_amp;      // 基波幅度
    float h2_amp, h3_amp;       // 谐波幅度
    float h4_amp, h5_amp;
    float thd;                  // 总谐波失真（%）
    float dc_offset;            // 直流分量
    float sample_rate;          // 采样率
    float freq_resolution;      // 频率分辨率
    bool valid;                 // 数据有效
} osc_fft_result_t;

/* 波形数据 */
typedef struct {
    float *data;                // 电压数据
    uint32_t count;             // 数据点数
    uint32_t capacity;          // 缓冲区容量
    float time_per_sample;      // 采样间隔（s）
    uint32_t trigger_pos;       // 触发位置
    osc_time_scale_t time_scale;
    osc_volt_scale_t volt_scale;
} osc_waveform_t;

/* 示波器上下文（不透明类型） */
typedef struct osc_ctx_t osc_ctx_t;

/* 绘图上下文（不透明类型） */
typedef struct osc_draw_ctx_t osc_draw_ctx_t;

/*============================================================================
 * 核心API
 *============================================================================*/

/**
 * @brief 初始化示波器
 * @return 示波器上下文，失败返回NULL
 */
osc_ctx_t *osc_init(void);

/**
 * @brief 释放示波器
 */
void osc_deinit(osc_ctx_t *ctx);

/**
 * @brief 启动采集
 */
esp_err_t osc_start(osc_ctx_t *ctx);

/**
 * @brief 停止采集
 */
esp_err_t osc_stop(osc_ctx_t *ctx);

/**
 * @brief 周期更新（定时器调用）
 */
esp_err_t osc_update(osc_ctx_t *ctx);

/**
 * @brief 获取运行状态
 */
osc_state_t osc_get_state(osc_ctx_t *ctx);

/**
 * @brief 获取工作模式
 */
osc_mode_t osc_get_mode(osc_ctx_t *ctx);

/*============================================================================
 * 设置API
 *============================================================================*/

esp_err_t osc_set_time_scale(osc_ctx_t *ctx, osc_time_scale_t scale);
esp_err_t osc_set_volt_scale(osc_ctx_t *ctx, osc_volt_scale_t scale);
esp_err_t osc_set_x_offset(osc_ctx_t *ctx, float offset_s);
esp_err_t osc_set_y_offset(osc_ctx_t *ctx, float offset_v);
esp_err_t osc_set_trigger(osc_ctx_t *ctx, const osc_trigger_t *trigger);

/**
 * @brief 直接设置时基值（秒/格）
 * @param ctx 示波器上下文
 * @param time_per_div 时基值（秒/格）
 * @note UI层应使用此函数直接设置时基值
 */
esp_err_t osc_set_time_per_div(osc_ctx_t *ctx, float time_per_div);

/**
 * @brief 直接设置电压档位值（伏/格）
 * @param ctx 示波器上下文
 * @param volts_per_div 电压档位值（伏/格）
 * @note UI层应使用此函数直接设置电压档位值
 */
esp_err_t osc_set_volts_per_div(osc_ctx_t *ctx, float volts_per_div);

/*============================================================================
 * 数据获取API
 *============================================================================*/

/**
 * @brief 获取显示波形数据
 */
esp_err_t osc_get_display_data(osc_ctx_t *ctx, float *buffer, uint32_t *count);

/**
 * @brief 获取测量结果
 */
esp_err_t osc_get_measurements(osc_ctx_t *ctx, osc_measurement_t *meas);

/**
 * @brief 获取原始波形数据（用于FFT和导出）
 */
esp_err_t osc_get_raw_waveform(osc_ctx_t *ctx, const osc_waveform_t **waveform);

/**
 * @brief 自动调整
 */
esp_err_t osc_auto_adjust(osc_ctx_t *ctx);

/*============================================================================
 * 工具函数
 *============================================================================*/

float osc_get_time_per_div(osc_time_scale_t scale);
float osc_get_volts_per_div(osc_volt_scale_t scale);
const char *osc_get_time_scale_str(osc_time_scale_t scale);
const char *osc_get_volt_scale_str(osc_volt_scale_t scale);
uint32_t osc_get_sample_rate_hz(osc_ctx_t *ctx);

/*============================================================================
 * 绘图API
 *============================================================================*/

/**
 * @brief 初始化绘图上下文
 */
osc_draw_ctx_t *osc_draw_init(lv_obj_t *parent, int x, int y);

/**
 * @brief 释放绘图上下文
 */
void osc_draw_deinit(osc_draw_ctx_t *ctx);

/**
 * @brief 清除画布
 */
void osc_draw_clear(osc_draw_ctx_t *ctx);

/**
 * @brief 绘制网格
 */
void osc_draw_grid(osc_draw_ctx_t *ctx);

/**
 * @brief 绘制波形
 */
void osc_draw_waveform(osc_draw_ctx_t *ctx, const float *data, uint32_t count, 
                       float volts_per_div, float y_offset);

/**
 * @brief 绘制FFT频谱
 */
void osc_draw_fft(osc_draw_ctx_t *ctx, const float *data, uint32_t count,
                  float sample_rate, osc_fft_result_t *result);

/**
 * @brief 刷新显示
 */
void osc_draw_update(osc_draw_ctx_t *ctx);

/**
 * @brief 获取帧率
 */
float osc_draw_get_fps(osc_draw_ctx_t *ctx);

/**
 * @brief 获取FFT结果
 */
const osc_fft_result_t *osc_draw_get_fft_result(osc_draw_ctx_t *ctx);

/*============================================================================
 * 全局实例（供事件处理使用）
 *============================================================================*/

extern osc_ctx_t *g_osc_ctx;

/**
 * @brief 初始化全局示波器实例
 */
esp_err_t osc_global_init(void);

/**
 * @brief 释放全局示波器实例
 */
void osc_global_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // OSCILLOSCOPE_H
