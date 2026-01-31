/**
  ******************************************************************************
  * @file    uart_comm.h
  * @brief   UART通信模块头文件
  ******************************************************************************
  * @description
  *   本模块实现基于DMA的UART通信功能，用于STM32与ESP32之间的数据传输
  *   
  * @features
  *   - DMA发送: 非阻塞发送，支持发送队列
  *   - DMA接收: 空闲中断 + DMA双缓冲接收
  *   - 帧解析: 自动解析接收到的数据帧
  *   - 回调机制: 接收到完整帧后通过回调通知应用层
  *   
  * @hardware
  *   - USART1: PA9(TX), PA10(RX)
  *   - DMA2_Stream7: USART1_TX
  *   - DMA2_Stream2: USART1_RX
  *   - 波特率: 921600 bps
  ******************************************************************************
  */

#ifndef __UART_COMM_H
#define __UART_COMM_H

#include "main.h"
#include "usart.h"
#include "comm_protocol.h"
#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/*                           配置参数                                         */
/*===========================================================================*/

#define UART_TX_BUFFER_SIZE     8200    /* 发送缓冲区大小 (需容纳最大帧) */
#define UART_RX_BUFFER_SIZE     256     /* 单个接收缓冲区大小 */
#define UART_RX_FRAME_SIZE      512     /* 帧解析缓冲区大小 */

/*===========================================================================*/
/*                           回调函数类型定义                                 */
/*===========================================================================*/

/**
 * @brief  数据帧接收回调函数类型
 * @param  func_code: 功能码
 * @param  data: 数据指针
 * @param  data_len: 数据长度
 * @note   回调函数在中断上下文中执行，应尽快返回
 */
typedef void (*UartCommRxCallback_t)(CommFuncCode_t func_code, 
                                     const uint8_t *data, uint16_t data_len);

/**
 * @brief  发送完成回调函数类型
 * @note   可用于发送队列管理或状态更新
 */
typedef void (*UartCommTxCallback_t)(void);

/*===========================================================================*/
/*                           发送状态枚举                                     */
/*===========================================================================*/

typedef enum {
    UART_TX_IDLE = 0,       /* 空闲，可以发送 */
    UART_TX_BUSY,           /* 正在发送 */
    UART_TX_ERROR           /* 发送错误 */
} UartTxState_t;

/*===========================================================================*/
/*                           函数声明                                         */
/*===========================================================================*/

/* ==================== 初始化函数 ==================== */

/**
 * @brief  UART通信模块初始化
 * @note   调用此函数前需确保USART1和DMA已由CubeMX初始化
 */
void UartComm_Init(void);

/**
 * @brief  UART通信模块反初始化
 */
void UartComm_DeInit(void);

/* ==================== 回调注册函数 ==================== */

/**
 * @brief  注册数据帧接收回调函数
 * @param  callback: 回调函数指针
 */
void UartComm_RegisterRxCallback(UartCommRxCallback_t callback);

/**
 * @brief  注册发送完成回调函数
 * @param  callback: 回调函数指针
 */
void UartComm_RegisterTxCallback(UartCommTxCallback_t callback);

/* ==================== 数据发送函数 ==================== */

/**
 * @brief  发送数据帧 (非阻塞)
 * @param  func_code: 功能码
 * @param  data: 数据指针
 * @param  data_len: 数据长度
 * @retval 0=成功启动发送, -1=发送忙, -2=数据过长
 */
int UartComm_SendFrame(CommFuncCode_t func_code, const void *data, uint16_t data_len);

/**
 * @brief  发送原始数据 (非阻塞)
 * @param  data: 数据指针
 * @param  len: 数据长度
 * @retval 0=成功启动发送, -1=发送忙, -2=数据过长
 */
int UartComm_SendRaw(const uint8_t *data, uint16_t len);

/**
 * @brief  获取发送状态
 * @retval 发送状态
 */
UartTxState_t UartComm_GetTxState(void);

/**
 * @brief  等待发送完成 (阻塞)
 * @param  timeout_ms: 超时时间 (毫秒)
 * @retval 0=发送完成, -1=超时
 */
int UartComm_WaitTxComplete(uint32_t timeout_ms);

/* ==================== 便捷发送函数 ==================== */

/**
 * @brief  发送示波器波形数据
 * @param  gain: PGA增益档位 (0-7)
 * @param  coupling: 耦合模式 (0=DC, 1=AC)
 * @param  state: 运行状态 (0=停止, 1=运行, 2=单次)
 * @param  auto_range: 自动量程 (0=关闭, 1=开启)
 * @param  sample_rate: 采样率 (Hz)
 * @param  voltage_range: 当前电压量程 (±V)
 * @param  adc_data: ADC数据数组 (有符号值，相对于中点)
 * @param  data_count: 数据点数
 * @retval 0=成功, 负数=错误
 */
int UartComm_SendOscWaveform(uint8_t gain, uint8_t coupling, uint8_t state,
                             uint8_t auto_range, uint32_t sample_rate, 
                             float voltage_range, const int16_t *adc_data, 
                             uint16_t data_count);

/**
 * @brief  发送示波器测量结果
 * @param  measurement: 测量结果结构体指针
 * @retval 0=成功, 负数=错误
 */
int UartComm_SendOscMeasurement(const OscMeasurementFrame_t *measurement);

/**
 * @brief  发送万用表测量数据
 * @param  mode: 测量模式
 * @param  range: 量程档位
 * @param  flags: 状态标志
 * @param  value: 测量值
 * @retval 0=成功, 负数=错误
 */
int UartComm_SendMeterData(uint8_t mode, uint8_t range, uint8_t flags, float value);

/**
 * @brief  发送信号发生器状态
 * @param  wave_type: 波形类型
 * @param  enabled: 输出使能
 * @param  frequency: 频率
 * @param  amplitude: 幅值
 * @retval 0=成功, 负数=错误
 */
int UartComm_SendSignalGenStatus(uint8_t wave_type, uint8_t enabled,
                                  float frequency, float amplitude);

/**
 * @brief  发送心跳包
 * @param  device_status: 设备状态
 * @param  current_mode: 当前工作模式
 * @retval 0=成功, 负数=错误
 */
int UartComm_SendHeartbeat(uint8_t device_status, uint8_t current_mode);

/* ==================== 中断处理函数 ==================== */

/**
 * @brief  UART空闲中断处理 (需在stm32f4xx_it.c中调用)
 * @note   在USART1_IRQHandler中检测到空闲中断时调用
 */
void UartComm_IDLE_IRQHandler(void);

/**
 * @brief  DMA发送完成回调 (需在HAL回调中调用)
 * @param  huart: UART句柄
 */
void UartComm_TxCpltCallback(UART_HandleTypeDef *huart);

/**
 * @brief  DMA接收完成回调 (需在HAL回调中调用)
 * @param  huart: UART句柄
 */
void UartComm_RxCpltCallback(UART_HandleTypeDef *huart);

/**
 * @brief  UART错误回调 (需在HAL回调中调用)
 * @param  huart: UART句柄
 */
void UartComm_ErrorCallback(UART_HandleTypeDef *huart);

/* ==================== 周期处理函数 ==================== */

/**
 * @brief  通信模块周期处理 (在主循环中调用)
 * @note   用于处理接收缓冲区中的数据帧
 */
void UartComm_Process(void);

#endif /* __UART_COMM_H */
