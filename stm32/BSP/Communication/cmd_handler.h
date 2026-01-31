/**
  ******************************************************************************
  * @file    cmd_handler.h
  * @brief   命令处理模块头文件
  ******************************************************************************
  * @description
  *   本模块处理ESP32发送的控制命令，并调用相应的功能模块执行
  *   
  * @features
  *   - 示波器控制: 启动/停止采样、设置增益、设置耦合模式
  *   - 万用表控制: 设置测量模式、设置量程
  *   - 信号发生器控制: 设置波形、频率、幅值
  ******************************************************************************
  */

#ifndef __CMD_HANDLER_H
#define __CMD_HANDLER_H

#include "main.h"
#include "comm_protocol.h"
#include <stdint.h>

/*===========================================================================*/
/*                           函数声明                                         */
/*===========================================================================*/

/**
 * @brief  命令处理模块初始化
 * @note   注册UART接收回调函数
 */
void CmdHandler_Init(void);

/**
 * @brief  命令处理回调函数
 * @param  func_code: 功能码
 * @param  data: 数据指针
 * @param  data_len: 数据长度
 * @note   此函数由UART接收回调调用
 */
void CmdHandler_ProcessCommand(CommFuncCode_t func_code, 
                               const uint8_t *data, uint16_t data_len);

/**
 * @brief  处理示波器控制命令
 * @param  data: 命令数据指针
 * @param  len: 数据长度
 */
void CmdHandler_OscControl(const uint8_t *data, uint16_t len);

/**
 * @brief  处理万用表控制命令
 * @param  data: 命令数据指针
 * @param  len: 数据长度
 */
void CmdHandler_MeterControl(const uint8_t *data, uint16_t len);

/**
 * @brief  处理信号发生器控制命令
 * @param  data: 命令数据指针
 * @param  len: 数据长度
 */
void CmdHandler_SignalGenControl(const uint8_t *data, uint16_t len);

/**
 * @brief  处理数控电源控制命令
 * @param  data: 命令数据指针
 * @param  len: 数据长度
 */
void CmdHandler_PowerControl(const uint8_t *data, uint16_t len);

/**
 * @brief  处理工作模式切换命令
 * @param  data: 命令数据指针
 * @param  len: 数据长度
 */
void CmdHandler_ModeControl(const uint8_t *data, uint16_t len);

#endif /* __CMD_HANDLER_H */
