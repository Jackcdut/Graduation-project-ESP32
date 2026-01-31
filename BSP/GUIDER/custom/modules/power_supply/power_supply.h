/**
 * @file power_supply.h
 * @brief 数控电源模块头文件 (ESP32端)
 * 
 * 功能：
 * - 接收STM32电源数据并显示
 * - 发送控制命令到STM32
 * - 电压/电流设置
 * - CC/CV模式显示
 * - 保护状态显示
 */

#ifndef POWER_SUPPLY_MODULE_H
#define POWER_SUPPLY_MODULE_H

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

/* 电压范围 */
#define POWER_VOLTAGE_MIN       0.0f
#define POWER_VOLTAGE_MAX       12.0f
#define POWER_VOLTAGE_STEP      0.1f

/* 电流范围 */
#define POWER_CURRENT_MIN       0.0f
#define POWER_CURRENT_MAX       2.0f
#define POWER_CURRENT_STEP      0.01f

/*============================================================================
 * 枚举类型
 *============================================================================*/

/* 电源工作状态 */
typedef enum {
    PS_STATE_OFF = 0,
    PS_STATE_SOFTSTART,
    PS_STATE_RUNNING,
    PS_STATE_CC_MODE,
    PS_STATE_CV_MODE,
    PS_STATE_OTP,
    PS_STATE_OVP,
    PS_STATE_OCP,
    PS_STATE_ERROR
} ps_state_t;

/* 控制模式 */
typedef enum {
    PS_MODE_CV = 0,
    PS_MODE_CC,
    PS_MODE_AUTO
} ps_mode_t;

/* PD电压档位 */
typedef enum {
    PS_PD_5V = 0,
    PS_PD_9V,
    PS_PD_12V,
    PS_PD_20V,
    PS_PD_28V
} ps_pd_voltage_t;

/*============================================================================
 * 数据结构
 *============================================================================*/

/* 电源数据 */
typedef struct {
    ps_state_t state;           /* 工作状态 */
    bool output_enable;         /* 输出使能 */
    ps_mode_t mode;             /* 控制模式 */
    ps_pd_voltage_t pd_voltage; /* PD输入电压档位 */
    float voltage_set;          /* 设定电压 (V) */
    float current_set;          /* 设定电流 (A) */
    float voltage_out;          /* 输出电压 (V) */
    float current_out;          /* 输出电流 (A) */
    float power_out;            /* 输出功率 (W) */
    float voltage_in;           /* 输入电压 (V) */
    float temperature;          /* 温度 (°C) */
    bool data_valid;            /* 数据有效标志 */
} ps_data_t;

/* 电源上下文 */
typedef struct ps_ctx_t ps_ctx_t;

/*============================================================================
 * 核心API
 *============================================================================*/

/**
 * @brief 初始化电源模块
 * @return 电源上下文，失败返回NULL
 */
ps_ctx_t *ps_init(void);

/**
 * @brief 释放电源模块
 */
void ps_deinit(ps_ctx_t *ctx);

/**
 * @brief 周期更新（定时器调用）
 */
esp_err_t ps_update(ps_ctx_t *ctx);

/*============================================================================
 * 控制API
 *============================================================================*/

/**
 * @brief 使能/禁用输出
 */
esp_err_t ps_enable_output(ps_ctx_t *ctx, bool enable);

/**
 * @brief 设置输出电压
 */
esp_err_t ps_set_voltage(ps_ctx_t *ctx, float voltage);

/**
 * @brief 设置输出电流限制
 */
esp_err_t ps_set_current(ps_ctx_t *ctx, float current);

/**
 * @brief 设置控制模式
 */
esp_err_t ps_set_mode(ps_ctx_t *ctx, ps_mode_t mode);

/**
 * @brief 设置PD输入电压
 */
esp_err_t ps_set_pd_voltage(ps_ctx_t *ctx, ps_pd_voltage_t pd_voltage);

/**
 * @brief 清除保护状态
 */
esp_err_t ps_clear_protection(ps_ctx_t *ctx);

/*============================================================================
 * 数据获取API
 *============================================================================*/

/**
 * @brief 获取电源数据
 */
esp_err_t ps_get_data(ps_ctx_t *ctx, ps_data_t *data);

/**
 * @brief 获取工作状态
 */
ps_state_t ps_get_state(ps_ctx_t *ctx);

/**
 * @brief 检查输出是否使能
 */
bool ps_is_output_enabled(ps_ctx_t *ctx);

/**
 * @brief 检查是否处于CC模式
 */
bool ps_is_cc_mode(ps_ctx_t *ctx);

/*============================================================================
 * 工具函数
 *============================================================================*/

/**
 * @brief 获取状态字符串
 */
const char *ps_get_state_str(ps_state_t state);

/**
 * @brief 获取模式字符串
 */
const char *ps_get_mode_str(ps_mode_t mode);

/**
 * @brief 获取PD电压字符串
 */
const char *ps_get_pd_voltage_str(ps_pd_voltage_t pd_voltage);

/*============================================================================
 * 全局实例
 *============================================================================*/

extern ps_ctx_t *g_ps_ctx;

/**
 * @brief 初始化全局电源实例
 */
esp_err_t ps_global_init(void);

/**
 * @brief 释放全局电源实例
 */
void ps_global_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // POWER_SUPPLY_MODULE_H
