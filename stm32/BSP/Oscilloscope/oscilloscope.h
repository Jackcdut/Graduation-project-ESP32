/**
  ******************************************************************************
  * @file    oscilloscope.h
  * @brief   示波器模块头文件
  ******************************************************************************
  * @description
  *   实现示波器功能，支持三ADC交替采样实现高速采样
  *   
  * @hardware
  *   信号输入: PA0 (ADC123_IN0)
  *   信号链路:
  *     输入 → AC/DC耦合(PG7) → 1/50衰减 → OPA365缓冲 → MCP6S21 PGA → 反相运放 → ADC
  *   
  *   MCP6S21 PGA增益: ×1, ×2, ×4, ×5, ×8, ×10, ×16, ×32
  *   SPI控制: SPI1 (PA5=SCK, PA7=MOSI, PC4=CS)
  *   
  *   耦合选择:
  *     - PG7: AC/DC耦合 (低电平=DC, 高电平=AC)
  *   
  *   ADC输出公式: V_adc = 1.65 - V_pga
  *     ADC=1.65V → PGA输出0V
  *     ADC=0V → PGA输出+1.65V
  *     ADC=3.3V → PGA输出-1.65V
  *   
  *   输入电压范围: ±50V
  ******************************************************************************
  */

#ifndef __OSCILLOSCOPE_H
#define __OSCILLOSCOPE_H

#include "main.h"
#include "adc.h"
#include "tim.h"
#include "spi.h"

/*===========================================================================*/
/*                           采样缓冲区配置                                   */
/*===========================================================================*/
#define OSC_BUFFER_SIZE         4096    /* 采样缓冲区大小 */

/*===========================================================================*/
/*                           MCP6S21 PGA增益定义                              */
/*===========================================================================*/
typedef enum {
    PGA_GAIN_1  = 0x00,     /* ×1  : ±50V   量程 */
    PGA_GAIN_2  = 0x01,     /* ×2  : ±25V   量程 */
    PGA_GAIN_4  = 0x02,     /* ×4  : ±12.5V 量程 */
    PGA_GAIN_5  = 0x03,     /* ×5  : ±10V   量程 */
    PGA_GAIN_8  = 0x04,     /* ×8  : ±6.25V 量程 */
    PGA_GAIN_10 = 0x05,     /* ×10 : ±5V    量程 */
    PGA_GAIN_16 = 0x06,     /* ×16 : ±3.125V量程 */
    PGA_GAIN_32 = 0x07,     /* ×32 : ±1.5625V量程 */
    PGA_GAIN_MAX
} PGA_Gain_t;

/*===========================================================================*/
/*                           耦合模式定义                                     */
/*===========================================================================*/
typedef enum {
    OSC_COUPLING_DC = 0,    /* DC耦合 */
    OSC_COUPLING_AC         /* AC耦合 */
} OscCoupling_t;

/*===========================================================================*/
/*                           采样率定义                                       */
/*===========================================================================*/
typedef enum {
    OSC_SAMPLERATE_1K = 0,      /* 1 KSPS */
    OSC_SAMPLERATE_10K,         /* 10 KSPS */
    OSC_SAMPLERATE_100K,        /* 100 KSPS */
    OSC_SAMPLERATE_500K,        /* 500 KSPS */
    OSC_SAMPLERATE_1M,          /* 1 MSPS */
    OSC_SAMPLERATE_2M,          /* 2 MSPS */
    OSC_SAMPLERATE_5M,          /* 5 MSPS (三ADC交替) */
    OSC_SAMPLERATE_MAX
} OscSampleRate_t;

/*===========================================================================*/
/*                           示波器状态                                       */
/*===========================================================================*/
typedef enum {
    OSC_STATE_IDLE = 0,     /* 空闲 */
    OSC_STATE_RUNNING,      /* 采样中 */
    OSC_STATE_COMPLETE      /* 采样完成 */
} OscState_t;

/*===========================================================================*/
/*                           示波器配置结构体                                 */
/*===========================================================================*/
typedef struct {
    PGA_Gain_t pgaGain;         /* PGA增益档位 */
    OscCoupling_t coupling;     /* 耦合模式 */
    OscSampleRate_t sampleRate; /* 采样率 */
    OscState_t state;           /* 运行状态 */
    uint8_t tripleADC;          /* 是否使用三ADC交替模式 */
    uint8_t autoRange;          /* 自动量程开关 */
} OscConfig_t;

/*===========================================================================*/
/*                           测量结果结构体                                   */
/*===========================================================================*/
typedef struct {
    float vpp;          /* 峰峰值 (V) */
    float vmax;         /* 最大值 (V) */
    float vmin;         /* 最小值 (V) */
    float vavg;         /* 平均值 (V) */
    float vrms;         /* 有效值 (V) */
    float freq;         /* 频率 (Hz) */
    float period;       /* 周期 (us) */
    float duty;         /* 占空比 (%) */
} OscMeasurement_t;

/*===========================================================================*/
/*                           GPIO定义                                         */
/*===========================================================================*/
/* MCP6S21 PGA CS引脚 */
#define PGA_CS_PORT         Oscilloscope_PGA_CS_GPIO_Port
#define PGA_CS_PIN          Oscilloscope_PGA_CS_Pin

/* AC/DC耦合控制 */
#define OSC_AC_PORT         Oscilloscope_AC_GPIO_Port
#define OSC_AC_PIN          Oscilloscope_AC_Pin

/*===========================================================================*/
/*                           MCP6S21 SPI命令                                  */
/*===========================================================================*/
#define MCP6S21_CMD_NOP         0x00    /* 空操作 */
#define MCP6S21_CMD_SHUTDOWN    0x20    /* 关机模式 */
#define MCP6S21_CMD_WRITE_GAIN  0x40    /* 写增益寄存器 */
#define MCP6S21_CMD_WRITE_CH    0x41    /* 写通道寄存器 */

/*===========================================================================*/
/*                           ADC参数                                          */
/*===========================================================================*/
#define OSC_ADC_VREF        3.3f        /* ADC参考电压 */
#define OSC_ADC_RESOLUTION  4095.0f     /* 12位ADC分辨率 */
#define OSC_ADC_OFFSET      1.65f       /* ADC中点电压 */
#define OSC_INPUT_ATTEN     50.0f       /* 前端衰减比 1/50 */
#define OSC_MAX_INPUT_V     50.0f       /* 最大输入电压 ±50V */

/*===========================================================================*/
/*                           PGA增益值表                                      */
/*===========================================================================*/
/* 实际增益倍数 */
#define PGA_GAIN_VALUE_1    1.0f
#define PGA_GAIN_VALUE_2    2.0f
#define PGA_GAIN_VALUE_4    4.0f
#define PGA_GAIN_VALUE_5    5.0f
#define PGA_GAIN_VALUE_8    8.0f
#define PGA_GAIN_VALUE_10   10.0f
#define PGA_GAIN_VALUE_16   16.0f
#define PGA_GAIN_VALUE_32   32.0f

/*===========================================================================*/
/*                           函数声明                                         */
/*===========================================================================*/

/* 初始化与模式切换 */
void Oscilloscope_Init(void);
void Oscilloscope_DeInit(void);

/* 采样控制 */
void Oscilloscope_Start(void);
void Oscilloscope_Stop(void);
OscState_t Oscilloscope_GetState(void);

/* PGA控制函数 */
void PGA_Init(void);
void PGA_SetGain(PGA_Gain_t gain);
PGA_Gain_t PGA_GetGain(void);
float PGA_GetGainValue(PGA_Gain_t gain);
void PGA_Shutdown(void);
void PGA_Wakeup(void);

/* 配置函数 */
void Oscilloscope_SetCoupling(OscCoupling_t coupling);
OscCoupling_t Oscilloscope_GetCoupling(void);

void Oscilloscope_SetSampleRate(OscSampleRate_t rate);
OscSampleRate_t Oscilloscope_GetSampleRate(void);

/* 自动量程 */
void Oscilloscope_SetAutoRange(uint8_t enable);
uint8_t Oscilloscope_GetAutoRange(void);
void Oscilloscope_AutoAdjustGain(void);

/* 数据获取 */
uint32_t* Oscilloscope_GetBuffer(void);
uint32_t Oscilloscope_GetBufferSize(void);

/* 测量函数 */
OscMeasurement_t Oscilloscope_Measure(void);
float Oscilloscope_ADCToVoltage(uint16_t adcValue);
float Oscilloscope_GetVoltageRange(void);

/* 回调函数 (DMA完成时调用) */
void Oscilloscope_DMACompleteCallback(void);
void Oscilloscope_DMAHalfCompleteCallback(void);

/* 通信模块需要的额外函数 */
OscConfig_t* Oscilloscope_GetConfig(void);
OscMeasurement_t* Oscilloscope_GetMeasurement(void);
uint32_t Oscilloscope_GetSampleRateHz(void);
void Oscilloscope_SingleCapture(void);

/* 兼容旧接口 */
#define OscGain_t PGA_Gain_t
#define OSC_GAIN_0 PGA_GAIN_1
#define OSC_GAIN_1 PGA_GAIN_2
#define OSC_GAIN_2 PGA_GAIN_4
#define OSC_GAIN_3 PGA_GAIN_5
#define OSC_GAIN_4 PGA_GAIN_8
#define OSC_GAIN_5 PGA_GAIN_10
#define OSC_GAIN_MAX PGA_GAIN_MAX
#define Oscilloscope_SetGain(g) PGA_SetGain(g)
#define Oscilloscope_GetGain() PGA_GetGain()

#endif /* __OSCILLOSCOPE_H */
