/**
  ******************************************************************************
  * @file    power_supply.h
  * @brief   数控电源模块头文件
  ******************************************************************************
  * @description
  *   本模块实现基于MK9218的CC/CV数控电源功能
  *   
  * @features
  *   - 恒压(CV)模式: 0-12V可调输出
  *   - 恒流(CC)模式: 0-2A可调限流
  *   - CC/CV自动切换
  *   - 输入电压监测 (USB PD: 5V/9V/12V/20V/28V)
  *   - 输出电压/电流实时监测
  *   - 温度监测与保护
  *   - 软启动功能
  *   
  * @hardware
  *   - MK9218: 同步降压控制器
  *   - CH224A: USB PD协议芯片
  *   - SGM8632: CC/CV控制运放
  *   - INA180A2: 电流检测放大器
  *   
  * @pins
  *   - PA4 (DAC1_OUT1): CV电压设定
  *   - PA5 (DAC1_OUT2): CC电流设定
  *   - PF6 (ADC3_IN4): 电压反馈VFB
  *   - PF7 (ADC3_IN5): 电流反馈IFB
  *   - PF8 (ADC3_IN6): 输入电压监测
  *   - PF9 (ADC3_IN7): NTC温度监测
  *   - PF10 (ADC3_IN8): CC状态监测
  *   - PD8/9/10: CH224A配置引脚
  ******************************************************************************
  */

#ifndef __POWER_SUPPLY_H
#define __POWER_SUPPLY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/*                           配置参数                                         */
/*===========================================================================*/

/* 电压范围 (V) */
#define POWER_VOLTAGE_MIN           0.0f
#define POWER_VOLTAGE_MAX           12.0f
#define POWER_VOLTAGE_DEFAULT       5.0f

/* 电流范围 (A) */
#define POWER_CURRENT_MIN           0.0f
#define POWER_CURRENT_MAX           2.0f
#define POWER_CURRENT_DEFAULT       1.0f

/* DAC配置 */
#define POWER_DAC_RESOLUTION        4096        /* 12位DAC */
#define POWER_DAC_VREF              3.3f        /* DAC参考电压 */

/* ADC配置 */
#define POWER_ADC_RESOLUTION        4096        /* 12位ADC */
#define POWER_ADC_VREF              3.3f        /* ADC参考电压 */

/* 电压反馈分压比 (根据原理图计算) */
/* VFB = VOUT * R1080 / (R1070 + R1080) */
#define POWER_VFB_DIVIDER_RATIO     0.5f        /* 1080/(1070+1080) ≈ 0.5 */

/* 电流检测增益 (INA180A2, 100V/V, 10mΩ采样电阻) */
/* IFB = IOUT * 0.01Ω * 100 = IOUT * 1V/A */
#define POWER_IFB_GAIN              1.0f        /* V/A */

/* 输入电压分压比 */
/* POWER_ADC = VBUS * R120 / (R119 + R120) = VBUS * 2k / 16k = VBUS / 8 */
#define POWER_VIN_DIVIDER_RATIO     0.125f      /* 2k/(14k+2k) = 0.125 */

/* NTC温度计算参数 (10K NTC, B=3950) */
#define POWER_NTC_R25               10000.0f    /* 25°C时的电阻值 */
#define POWER_NTC_B                 3950.0f     /* B值 */
#define POWER_NTC_T25               298.15f     /* 25°C的开尔文温度 */
#define POWER_NTC_PULLUP            10000.0f    /* 上拉电阻 */

/* 温度保护阈值 */
#define POWER_TEMP_WARNING          60.0f       /* 警告温度 (°C) */
#define POWER_TEMP_SHUTDOWN         80.0f       /* 关断温度 (°C) */
#define POWER_TEMP_RECOVERY         50.0f       /* 恢复温度 (°C) */

/* 软启动配置 */
#define POWER_SOFTSTART_TIME_MS     100         /* 软启动时间 (ms) */
#define POWER_SOFTSTART_STEPS       20          /* 软启动步数 */

/* 采样滤波配置 */
#define POWER_FILTER_SAMPLES        8           /* 滤波采样次数 */

/*===========================================================================*/
/*                           类型定义                                         */
/*===========================================================================*/

/**
 * @brief 电源工作状态
 */
typedef enum {
    POWER_STATE_OFF = 0,        /* 关闭状态 */
    POWER_STATE_SOFTSTART,      /* 软启动中 */
    POWER_STATE_RUNNING,        /* 正常运行 */
    POWER_STATE_CC_MODE,        /* 恒流模式 */
    POWER_STATE_CV_MODE,        /* 恒压模式 */
    POWER_STATE_OTP,            /* 过温保护 */
    POWER_STATE_OVP,            /* 过压保护 */
    POWER_STATE_OCP,            /* 过流保护 */
    POWER_STATE_ERROR           /* 错误状态 */
} PowerState_t;

/**
 * @brief 控制模式
 */
typedef enum {
    POWER_MODE_CV = 0,          /* 恒压优先模式 */
    POWER_MODE_CC,              /* 恒流优先模式 */
    POWER_MODE_AUTO             /* 自动切换模式 */
} PowerMode_t;

/**
 * @brief USB PD输入电压档位
 */
typedef enum {
    POWER_PD_5V = 0,            /* 5V */
    POWER_PD_9V,                /* 9V */
    POWER_PD_12V,               /* 12V */
    POWER_PD_20V,               /* 20V */
    POWER_PD_28V                /* 28V (PPS) */
} PowerPDVoltage_t;

/**
 * @brief 电源配置结构体
 */
typedef struct {
    float           voltage_set;        /* 设定电压 (V) */
    float           current_set;        /* 设定电流 (A) */
    PowerMode_t     mode;               /* 控制模式 */
    bool            output_enable;      /* 输出使能 */
    PowerPDVoltage_t pd_voltage;        /* PD输入电压档位 */
} PowerConfig_t;

/**
 * @brief 电源测量数据结构体
 */
typedef struct {
    float           voltage_out;        /* 输出电压 (V) */
    float           current_out;        /* 输出电流 (A) */
    float           power_out;          /* 输出功率 (W) */
    float           voltage_in;         /* 输入电压 (V) */
    float           temperature;        /* 温度 (°C) */
    PowerState_t    state;              /* 当前状态 */
    bool            is_cc_mode;         /* 是否处于CC模式 */
    bool            is_cv_mode;         /* 是否处于CV模式 */
} PowerMeasurement_t;

/**
 * @brief 电源状态回调函数类型
 */
typedef void (*PowerStateCallback_t)(PowerState_t state);

/*===========================================================================*/
/*                           函数声明                                         */
/*===========================================================================*/

/* ==================== 初始化函数 ==================== */

/**
 * @brief  数控电源模块初始化
 * @note   初始化DAC、ADC和GPIO，设置默认参数
 */
void PowerSupply_Init(void);

/**
 * @brief  数控电源模块反初始化
 */
void PowerSupply_DeInit(void);

/* ==================== 输出控制函数 ==================== */

/**
 * @brief  使能/禁用输出
 * @param  enable: true=使能输出, false=禁用输出
 */
void PowerSupply_EnableOutput(bool enable);

/**
 * @brief  获取输出使能状态
 * @retval true=输出已使能, false=输出已禁用
 */
bool PowerSupply_IsOutputEnabled(void);

/**
 * @brief  设置输出电压
 * @param  voltage: 目标电压 (V), 范围: 0-12V
 * @retval 0=成功, -1=参数错误
 */
int PowerSupply_SetVoltage(float voltage);

/**
 * @brief  设置输出电流限制
 * @param  current: 目标电流 (A), 范围: 0-2A
 * @retval 0=成功, -1=参数错误
 */
int PowerSupply_SetCurrent(float current);

/**
 * @brief  获取设定电压
 * @retval 设定电压值 (V)
 */
float PowerSupply_GetSetVoltage(void);

/**
 * @brief  获取设定电流
 * @retval 设定电流值 (A)
 */
float PowerSupply_GetSetCurrent(void);

/* ==================== 测量函数 ==================== */

/**
 * @brief  获取输出电压
 * @retval 实际输出电压 (V)
 */
float PowerSupply_GetVoltage(void);

/**
 * @brief  获取输出电流
 * @retval 实际输出电流 (A)
 */
float PowerSupply_GetCurrent(void);

/**
 * @brief  获取输出功率
 * @retval 输出功率 (W)
 */
float PowerSupply_GetPower(void);

/**
 * @brief  获取输入电压
 * @retval 输入电压 (V)
 */
float PowerSupply_GetInputVoltage(void);

/**
 * @brief  获取温度
 * @retval 温度 (°C)
 */
float PowerSupply_GetTemperature(void);

/**
 * @brief  获取完整测量数据
 * @param  measurement: 测量数据结构体指针
 */
void PowerSupply_GetMeasurement(PowerMeasurement_t *measurement);

/* ==================== 状态函数 ==================== */

/**
 * @brief  获取当前工作状态
 * @retval 工作状态
 */
PowerState_t PowerSupply_GetState(void);

/**
 * @brief  检查是否处于CC模式
 * @retval true=CC模式, false=CV模式
 */
bool PowerSupply_IsCCMode(void);

/**
 * @brief  设置控制模式
 * @param  mode: 控制模式
 */
void PowerSupply_SetMode(PowerMode_t mode);

/**
 * @brief  获取控制模式
 * @retval 控制模式
 */
PowerMode_t PowerSupply_GetMode(void);

/* ==================== PD配置函数 ==================== */

/**
 * @brief  设置PD输入电压
 * @param  voltage: PD电压档位
 * @note   通过CH224A的CFG引脚配置
 */
void PowerSupply_SetPDVoltage(PowerPDVoltage_t voltage);

/**
 * @brief  获取当前PD电压设置
 * @retval PD电压档位
 */
PowerPDVoltage_t PowerSupply_GetPDVoltage(void);

/* ==================== 配置函数 ==================== */

/**
 * @brief  获取配置结构体指针
 * @retval 配置结构体指针
 */
PowerConfig_t* PowerSupply_GetConfig(void);

/**
 * @brief  应用配置
 * @param  config: 配置结构体指针
 */
void PowerSupply_ApplyConfig(const PowerConfig_t *config);

/* ==================== 回调函数 ==================== */

/**
 * @brief  注册状态变化回调函数
 * @param  callback: 回调函数指针
 */
void PowerSupply_RegisterStateCallback(PowerStateCallback_t callback);

/* ==================== 周期处理函数 ==================== */

/**
 * @brief  电源模块周期处理
 * @note   在主循环中调用，用于ADC采样、保护检测等
 */
void PowerSupply_Process(void);

/* ==================== 保护函数 ==================== */

/**
 * @brief  清除保护状态
 * @note   在保护触发后，需要手动清除才能重新启动
 */
void PowerSupply_ClearProtection(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_SUPPLY_H */
