/**
  ******************************************************************************
  * @file    multimeter.h
  * @brief   万用表模块头文件
  ******************************************************************************
  * @description
  *   支持电压、电流、电阻三种测量模式
  *   - 电压测量: 0~±36V, ADC通道PA1
  *   - 电流测量: ±2A, ADC通道PA3
  *   - 电阻测量: 10Ω~1MΩ (5档自动量程), ADC通道PA2
  ******************************************************************************
  */

#ifndef __MULTIMETER_H
#define __MULTIMETER_H

#include "main.h"
#include "adc.h"

/*===========================================================================*/
/*                           测量模式定义                                     */
/*===========================================================================*/
typedef enum {
    METER_MODE_IDLE = 0,    /* 空闲模式 */
    METER_MODE_VOLTAGE,     /* 电压测量模式 */
    METER_MODE_CURRENT,     /* 电流测量模式 */
    METER_MODE_RESISTANCE   /* 电阻测量模式 */
} MeterMode_t;

/*===========================================================================*/
/*                           电阻档位定义                                     */
/*===========================================================================*/
typedef enum {
    RES_RANGE_100R = 0,     /* 100Ω档位 */
    RES_RANGE_1K,           /* 1KΩ档位 */
    RES_RANGE_10K,          /* 10KΩ档位 */
    RES_RANGE_100K,         /* 100KΩ档位 */
    RES_RANGE_1M,           /* 1MΩ档位 */
    RES_RANGE_AUTO          /* 自动量程 */
} ResRange_t;

/*===========================================================================*/
/*                           测量结果结构体                                   */
/*===========================================================================*/
typedef struct {
    float voltage;          /* 电压值 (V) */
    float current;          /* 电流值 (A) */
    float resistance;       /* 电阻值 (Ω) */
    int8_t polarity;        /* 极性: 1=正, -1=负 */
    uint8_t overrange;      /* 超量程标志 */
    uint8_t open_circuit;   /* 开路标志 (电阻测量) */
    uint8_t short_circuit;  /* 短路标志 (电阻测量) */
    ResRange_t res_range;   /* 当前电阻档位 */
    MeterMode_t mode;       /* 当前测量模式 */
} MeterResult_t;

/*===========================================================================*/
/*                           ADC通道定义                                      */
/*===========================================================================*/
/* ADC DMA缓冲区索引 (按ADC扫描顺序: IN1, IN2, IN3) */
#define ADC_CHANNEL_VOLTAGE     0   /* PA1 - ADC1_IN1 - 电压采集 */
#define ADC_CHANNEL_RESISTANCE  1   /* PA2 - ADC1_IN2 - 电阻采集 */
#define ADC_CHANNEL_CURRENT     2   /* PA3 - ADC1_IN3 - 电流采集 */

/* ADC参考电压 */
#define ADC_VREF                3.3f
#define ADC_RESOLUTION          4095.0f

/*===========================================================================*/
/*                           测量参数定义                                     */
/*===========================================================================*/
/* 电压测量参数 */
#define VOLTAGE_DIVIDER_RATIO   22.36f      /* 分压比 49.2/2.2 */
#define VOLTAGE_REF_OFFSET      1.65f       /* 参考电压偏移 */
#define VOLTAGE_MAX             36.0f       /* 最大测量电压 */

/* 电流测量参数 */
#define CURRENT_GAIN            2.0f        /* 电流换算系数: I = (V_ADC - 1.65) × 2 */
#define CURRENT_REF_OFFSET      1.65f       /* 参考电压偏移 */
#define CURRENT_MAX             2.0f        /* 最大测量电流 */

/* 电阻档位参考电阻值 */
#define RES_REF_100R            100.0f
#define RES_REF_1K              1000.0f
#define RES_REF_10K             10000.0f
#define RES_REF_100K            100000.0f
#define RES_REF_1M              1000000.0f
#define RES_MAX                 1100000.0f  /* 最大测量电阻 */

/* 滤波参数 */
#define FILTER_SAMPLE_COUNT     100         /* 均值滤波采样次数 */

/* 继电器延时参数 */
#define RELAY_SWITCH_DELAY_MS   30          /* 继电器切换延时(ms) */
#define RELAY_STABLE_DELAY_MS   50          /* 继电器稳定延时(ms) */
#define ADC_STABLE_DELAY_MS     20          /* ADC稳定延时(ms) */

/*===========================================================================*/
/*                           GPIO控制引脚 (使用main.h中的定义)                  */
/*===========================================================================*/
/* 通道选择继电器控制 (高电平导通继电器) */
#define RELAY_RES_PORT          Multimeter_Res_Con_GPIO_Port
#define RELAY_RES_PIN           Multimeter_Res_Con_Pin      /* PG11 - 电阻通道 */
#define RELAY_VOL_PORT          Multimeter_Vol_Con_GPIO_Port
#define RELAY_VOL_PIN           Multimeter_Vol_Con_Pin      /* PG12 - 电压通道 */
#define RELAY_CUR_PORT          Multimeter_Cur_Con_GPIO_Port
#define RELAY_CUR_PIN           Multimeter_Cur_Con_Pin      /* PG13 - 电流通道 */

/* 黑表笔接地控制 (高电平导通MOS，黑表笔接地) */
#define BLACK_CON_PORT          Multimeter_Black_Con_GPIO_Port
#define BLACK_CON_PIN           Multimeter_Black_Con_Pin    /* PD4 - 黑表笔接地 */

/* 电阻档位选择 (P-MOS AO3401, 低电平导通!) */
#define RES_100R_PORT           Multimeter_100R_GPIO_Port
#define RES_100R_PIN            Multimeter_100R_Pin         /* PG2 - 100Ω */
#define RES_1K_PORT             Multimeter_1KR_GPIO_Port
#define RES_1K_PIN              Multimeter_1KR_Pin          /* PG3 - 1KΩ */
#define RES_10K_PORT            Multimeter_10KR_GPIO_Port
#define RES_10K_PIN             Multimeter_10KR_Pin         /* PG4 - 10KΩ */
#define RES_100K_PORT           Multimeter_100KR_GPIO_Port
#define RES_100K_PIN            Multimeter_100KR_Pin        /* PG5 - 100KΩ */
#define RES_1M_PORT             Multimeter_1MR_GPIO_Port
#define RES_1M_PIN              Multimeter_1MR_Pin          /* PG6 - 1MΩ */

/*===========================================================================*/
/*                           函数声明                                         */
/*===========================================================================*/

/* 初始化 */
void Multimeter_Init(void);

/* 模式切换 */
void Multimeter_SetMode(MeterMode_t mode);
MeterMode_t Multimeter_GetMode(void);

/* 电阻档位控制 */
void Multimeter_SetResRange(ResRange_t range);
ResRange_t Multimeter_GetResRange(void);

/* 测量函数 */
float Multimeter_MeasureVoltage(void);
float Multimeter_MeasureCurrent(void);
float Multimeter_MeasureResistance(void);

/* 综合测量 (根据当前模式) */
MeterResult_t Multimeter_Measure(void);

/* ADC数据处理 */
void Multimeter_StartADC(void);
void Multimeter_StopADC(void);
void Multimeter_UpdateADC(uint16_t* adc_buffer);

/* 获取原始ADC值 */
uint16_t Multimeter_GetRawADC(uint8_t channel);

/* 自动量程 */
ResRange_t Multimeter_AutoRange(void);

/* 启用/禁用自动量程 */
void Multimeter_EnableAutoRange(uint8_t enable);
uint8_t Multimeter_IsAutoRangeEnabled(void);

/* 获取滤波后的ADC值 */
uint16_t Multimeter_GetFilteredADC(uint8_t channel);

#endif /* __MULTIMETER_H */
