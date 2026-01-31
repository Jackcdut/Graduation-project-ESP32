/**
  ******************************************************************************
  * @file    signal_generator.h
  * @brief   信号发生器模块头文件 (AD9833 + MCP41010)
  ******************************************************************************
  */

#ifndef __SIGNAL_GENERATOR_H
#define __SIGNAL_GENERATOR_H

#include "main.h"
#include "spi.h"

/*===========================================================================*/
/*                           波形类型定义                                     */
/*===========================================================================*/
typedef enum {
    WAVE_SINE = 0,      /* 正弦波 */
    WAVE_TRIANGLE,      /* 三角波 */
    WAVE_SQUARE,        /* 方波 */
    WAVE_SQUARE_DIV2    /* 方波/2 */
} WaveType_t;

/*===========================================================================*/
/*                           AD9833 相关定义                                  */
/*===========================================================================*/
/* AD9833 片选引脚控制 (PE15) */
#define AD9833_CS_LOW()     HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET)
#define AD9833_CS_HIGH()    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET)

/* AD9833 参考时钟频率 */
#define AD9833_REF_FREQ     25000000.0  /* 25MHz */

/*===========================================================================*/
/*                          MCP41010 相关定义                                 */
/*===========================================================================*/
/* MCP41010 片选引脚控制 (PE14) */
#define MCP41010_CS_LOW()   HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET)
#define MCP41010_CS_HIGH()  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET)

/* MCP41010 命令字节定义 */
#define MCP41010_CMD_WRITE      0x11    /* 写入电位器数据命令 */
#define MCP41010_CMD_SHUTDOWN   0x21    /* 关闭电位器命令 */

/*===========================================================================*/
/*                          函数声明                                          */
/*===========================================================================*/

/* 信号发生器综合控制 */
void SignalGenerator_Init(void);
void SignalGenerator_SetOutput(double freq, WaveType_t wave_type, float amplitude);
void SignalGenerator_SetFrequency(double freq);
void SignalGenerator_SetWaveform(WaveType_t wave_type);
void SignalGenerator_SetAmplitude(float amplitude);

/* AD9833 底层函数 */
void AD9833_Init(void);
void AD9833_Write(uint16_t data);
void AD9833_Reset(void);
void AD9833_SetFrequencyWaveform(double freq, uint8_t wave_type);

/* MCP41010 底层函数 */
void MCP41010_Init(void);
void MCP41010_Write(uint8_t value);
void MCP41010_SetVoltage(float voltage);

#endif /* __SIGNAL_GENERATOR_H */
