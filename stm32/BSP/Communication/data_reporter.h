/**
  ******************************************************************************
  * @file    data_reporter.h
  * @brief   数据上报模块头文件
  ******************************************************************************
  * @description
  *   本模块负责定时向ESP32上报各功能模块的测量数据
  *   
  * @features
  *   - 示波器数据上报: 波形数据、测量结果
  *   - 万用表数据上报: 测量值、状态
  *   - 信号发生器状态上报: 当前设置
  *   - 心跳包: 定时发送保持连接
  *   
  * @timing
  *   - 示波器波形: 20-50Hz (根据采样率调整)
  *   - 万用表数据: 10Hz
  *   - 信号发生器状态: 变化时上报
  *   - 心跳包: 1Hz
  ******************************************************************************
  */

#ifndef __DATA_REPORTER_H
#define __DATA_REPORTER_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/*                           配置参数                                         */
/*===========================================================================*/

/* 上报周期配置 (毫秒) */
#define REPORT_PERIOD_OSC_WAVEFORM      50      /* 示波器波形: 50ms (20Hz) */
#define REPORT_PERIOD_OSC_MEASUREMENT   100     /* 示波器测量: 100ms (10Hz) */
#define REPORT_PERIOD_METER             100     /* 万用表: 100ms (10Hz) */
#define REPORT_PERIOD_SIGGEN            500     /* 信号发生器: 500ms (2Hz) */
#define REPORT_PERIOD_POWER             100     /* 数控电源: 100ms (10Hz) */
#define REPORT_PERIOD_HEARTBEAT         1000    /* 心跳包: 1000ms (1Hz) */

/*===========================================================================*/
/*                           工作模式定义                                     */
/*===========================================================================*/

typedef enum {
    REPORTER_MODE_IDLE = 0,     /* 空闲模式 - 仅发送心跳 */
    REPORTER_MODE_OSC,          /* 示波器模式 */
    REPORTER_MODE_METER,        /* 万用表模式 */
    REPORTER_MODE_SIGGEN,       /* 信号发生器模式 */
    REPORTER_MODE_POWER,        /* 数控电源模式 */
    REPORTER_MODE_ALL           /* 全部模式 - 同时上报所有数据 */
} ReporterMode_t;

/*===========================================================================*/
/*                           函数声明                                         */
/*===========================================================================*/

/**
 * @brief  数据上报模块初始化
 */
void DataReporter_Init(void);

/**
 * @brief  设置工作模式
 * @param  mode: 工作模式
 */
void DataReporter_SetMode(ReporterMode_t mode);

/**
 * @brief  获取当前工作模式
 * @retval 当前工作模式
 */
ReporterMode_t DataReporter_GetMode(void);

/**
 * @brief  数据上报周期处理
 * @note   在主循环中调用，根据时间间隔发送数据
 */
void DataReporter_Process(void);

/**
 * @brief  使能/禁用示波器波形上报
 * @param  enable: true=使能, false=禁用
 */
void DataReporter_EnableOscWaveform(bool enable);

/**
 * @brief  使能/禁用示波器测量结果上报
 * @param  enable: true=使能, false=禁用
 */
void DataReporter_EnableOscMeasurement(bool enable);

/**
 * @brief  使能/禁用万用表数据上报
 * @param  enable: true=使能, false=禁用
 */
void DataReporter_EnableMeterData(bool enable);

/**
 * @brief  使能/禁用信号发生器状态上报
 * @param  enable: true=使能, false=禁用
 */
void DataReporter_EnableSignalGenStatus(bool enable);

/**
 * @brief  使能/禁用心跳包
 * @param  enable: true=使能, false=禁用
 */
void DataReporter_EnableHeartbeat(bool enable);

/**
 * @brief  立即发送示波器波形数据
 * @note   用于单次触发等场景
 */
void DataReporter_SendOscWaveformNow(void);

/**
 * @brief  立即发送万用表数据
 */
void DataReporter_SendMeterDataNow(void);

/**
 * @brief  立即发送信号发生器状态
 */
void DataReporter_SendSignalGenStatusNow(void);

/**
 * @brief  使能/禁用数控电源数据上报
 * @param  enable: true=使能, false=禁用
 */
void DataReporter_EnablePowerData(bool enable);

/**
 * @brief  立即发送数控电源数据
 */
void DataReporter_SendPowerDataNow(void);

#endif /* __DATA_REPORTER_H */
