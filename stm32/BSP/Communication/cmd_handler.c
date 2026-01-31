/**
  ******************************************************************************
  * @file    cmd_handler.c
  * @brief   命令处理模块实现
  ******************************************************************************
  * @description
  *   本文件实现ESP32控制命令的解析和执行
  ******************************************************************************
  */

#include "cmd_handler.h"
#include "uart_comm.h"
#include "oscilloscope.h"
#include "multimeter.h"
#include "signal_generator.h"
#include "power_supply.h"

/*===========================================================================*/
/*                           私有函数声明                                     */
/*===========================================================================*/

static void CmdHandler_RxCallback(CommFuncCode_t func_code, 
                                  const uint8_t *data, uint16_t data_len);

/*===========================================================================*/
/*                           初始化函数                                       */
/*===========================================================================*/

/**
 * @brief  命令处理模块初始化
 */
void CmdHandler_Init(void)
{
    /* 注册UART接收回调函数 */
    UartComm_RegisterRxCallback(CmdHandler_RxCallback);
}

/*===========================================================================*/
/*                           回调函数                                         */
/*===========================================================================*/

/**
 * @brief  UART接收回调函数
 * @note   当接收到完整数据帧时被调用
 */
static void CmdHandler_RxCallback(CommFuncCode_t func_code, 
                                  const uint8_t *data, uint16_t data_len)
{
    CmdHandler_ProcessCommand(func_code, data, data_len);
}

/*===========================================================================*/
/*                           命令处理函数                                     */
/*===========================================================================*/

/**
 * @brief  命令处理主函数
 * @param  func_code: 功能码
 * @param  data: 数据指针
 * @param  data_len: 数据长度
 */
void CmdHandler_ProcessCommand(CommFuncCode_t func_code, 
                               const uint8_t *data, uint16_t data_len)
{
    switch (func_code) {
        /* 示波器控制命令 */
        case FUNC_OSC_CONTROL:
            CmdHandler_OscControl(data, data_len);
            break;
        
        /* 万用表控制命令 */
        case FUNC_METER_CONTROL:
            CmdHandler_MeterControl(data, data_len);
            break;
        
        /* 信号发生器控制命令 */
        case FUNC_SIGGEN_CONTROL:
            CmdHandler_SignalGenControl(data, data_len);
            break;
        
        /* 数控电源控制命令 */
        case FUNC_POWER_CONTROL:
            CmdHandler_PowerControl(data, data_len);
            break;
        
        /* 工作模式切换命令 */
        case FUNC_MODE_CONTROL:
            CmdHandler_ModeControl(data, data_len);
            break;
        
        /* 心跳包 - 回复心跳 */
        case FUNC_HEARTBEAT:
            /* 收到ESP32心跳，可以回复确认 */
            UartComm_SendHeartbeat(DEVICE_STATUS_IDLE, 0);
            break;
        
        default:
            /* 未知命令，忽略 */
            break;
    }
}

/**
 * @brief  处理示波器控制命令
 */
void CmdHandler_OscControl(const uint8_t *data, uint16_t len)
{
    /* 检查数据长度 */
    if (len < sizeof(OscControlFrame_t)) {
        return;
    }
    
    /* 解析命令帧 */
    const OscControlFrame_t *cmd = (const OscControlFrame_t *)data;
    
    switch (cmd->cmd_type) {
        case OSC_CMD_START:
            /* 启动示波器采样 */
            Oscilloscope_Init();
            Oscilloscope_Start();
            break;
        
        case OSC_CMD_STOP:
            /* 停止示波器采样 */
            Oscilloscope_Stop();
            break;
        
        case OSC_CMD_SINGLE:
            /* 单次采样模式 */
            Oscilloscope_SingleCapture();
            break;
        
        case OSC_CMD_SET_GAIN:
            /* 设置PGA增益档位 */
            if (cmd->gain < PGA_GAIN_MAX) {
                PGA_SetGain((PGA_Gain_t)cmd->gain);
            }
            break;
        
        case OSC_CMD_SET_COUPLING:
            /* 设置耦合模式 */
            Oscilloscope_SetCoupling((OscCoupling_t)cmd->coupling);
            break;
        
        case OSC_CMD_SET_SAMPLERATE:
            /* 设置采样率 */
            Oscilloscope_SetSampleRate((OscSampleRate_t)cmd->sample_rate);
            break;
        
        case OSC_CMD_SET_AUTORANGE:
            /* 设置自动量程 */
            Oscilloscope_SetAutoRange(cmd->auto_range);
            break;
        
        default:
            break;
    }
}

/**
 * @brief  处理万用表控制命令
 */
void CmdHandler_MeterControl(const uint8_t *data, uint16_t len)
{
    /* 检查数据长度 */
    if (len < sizeof(MeterControlFrame_t)) {
        return;
    }
    
    /* 解析命令帧 */
    const MeterControlFrame_t *cmd = (const MeterControlFrame_t *)data;
    
    switch (cmd->cmd_type) {
        case METER_CMD_SET_MODE:
            /* 设置测量模式 */
            Multimeter_SetMode((MeterMode_t)cmd->mode);
            break;
        
        case METER_CMD_SET_RANGE:
            /* 设置量程档位 (仅电阻模式有效) */
            if (Multimeter_GetMode() == METER_MODE_RESISTANCE) {
                Multimeter_SetResRange((ResRange_t)cmd->range);
            }
            break;
        
        case METER_CMD_AUTO_RANGE:
            /* 自动量程开关 */
            Multimeter_EnableAutoRange(cmd->auto_range);
            break;
        
        default:
            break;
    }
}

/**
 * @brief  处理信号发生器控制命令
 */
void CmdHandler_SignalGenControl(const uint8_t *data, uint16_t len)
{
    /* 检查数据长度 */
    if (len < sizeof(SignalGenControlFrame_t)) {
        return;
    }
    
    /* 解析命令帧 */
    const SignalGenControlFrame_t *cmd = (const SignalGenControlFrame_t *)data;
    
    switch (cmd->cmd_type) {
        case SIGGEN_CMD_SET_WAVE:
            /* 设置波形类型 */
            SignalGenerator_SetWaveform((WaveType_t)cmd->wave_type);
            break;
        
        case SIGGEN_CMD_SET_FREQ:
            /* 设置频率 */
            SignalGenerator_SetFrequency((double)cmd->frequency);
            break;
        
        case SIGGEN_CMD_SET_AMP:
            /* 设置幅值 */
            SignalGenerator_SetAmplitude(cmd->amplitude);
            break;
        
        case SIGGEN_CMD_ENABLE:
            /* 使能输出 - 使用当前设置重新输出 */
            /* 注: AD9833没有单独的使能控制，通过设置输出来实现 */
            break;
        
        case SIGGEN_CMD_DISABLE:
            /* 禁用输出 - 复位AD9833 */
            AD9833_Reset();
            break;
        
        case SIGGEN_CMD_SET_ALL:
            /* 设置所有参数 */
            SignalGenerator_SetOutput((double)cmd->frequency, 
                                      (WaveType_t)cmd->wave_type, 
                                      cmd->amplitude);
            break;
        
        default:
            break;
    }
}


/**
 * @brief  处理数控电源控制命令
 */
void CmdHandler_PowerControl(const uint8_t *data, uint16_t len)
{
    /* 检查数据长度 */
    if (len < sizeof(PowerControlFrame_t)) {
        return;
    }
    
    /* 解析命令帧 */
    const PowerControlFrame_t *cmd = (const PowerControlFrame_t *)data;
    
    switch (cmd->cmd_type) {
        case POWER_CMD_ENABLE:
            /* 使能输出 */
            PowerSupply_EnableOutput(true);
            break;
        
        case POWER_CMD_DISABLE:
            /* 禁用输出 */
            PowerSupply_EnableOutput(false);
            break;
        
        case POWER_CMD_SET_VOLTAGE:
            /* 设置电压 */
            PowerSupply_SetVoltage(cmd->voltage_set);
            break;
        
        case POWER_CMD_SET_CURRENT:
            /* 设置电流 */
            PowerSupply_SetCurrent(cmd->current_set);
            break;
        
        case POWER_CMD_SET_MODE:
            /* 设置模式 */
            PowerSupply_SetMode((PowerMode_t)cmd->mode);
            break;
        
        case POWER_CMD_SET_PD:
            /* 设置PD电压 */
            PowerSupply_SetPDVoltage((PowerPDVoltage_t)cmd->pd_voltage);
            break;
        
        case POWER_CMD_SET_ALL:
            /* 设置所有参数 */
            {
                PowerConfig_t config;
                config.voltage_set = cmd->voltage_set;
                config.current_set = cmd->current_set;
                config.mode = (PowerMode_t)cmd->mode;
                config.pd_voltage = (PowerPDVoltage_t)cmd->pd_voltage;
                config.output_enable = cmd->output_enable;
                PowerSupply_ApplyConfig(&config);
            }
            break;
        
        case POWER_CMD_CLEAR_PROT:
            /* 清除保护状态 */
            PowerSupply_ClearProtection();
            break;
        
        default:
            break;
    }
}

/**
 * @brief  处理工作模式切换命令
 */
void CmdHandler_ModeControl(const uint8_t *data, uint16_t len)
{
    /* 检查数据长度 */
    if (len < sizeof(ModeControlFrame_t)) {
        return;
    }
    
    /* 解析命令帧 */
    const ModeControlFrame_t *cmd = (const ModeControlFrame_t *)data;
    
    /* 获取当前模式 */
    ReporterMode_t current_mode = DataReporter_GetMode();
    
    /* 如果模式没有变化，直接返回 */
    if ((cmd->mode == MODE_IDLE && current_mode == REPORTER_MODE_IDLE) ||
        (cmd->mode == MODE_OSCILLOSCOPE && current_mode == REPORTER_MODE_OSC) ||
        (cmd->mode == MODE_MULTIMETER && current_mode == REPORTER_MODE_METER) ||
        (cmd->mode == MODE_SIGGEN && current_mode == REPORTER_MODE_SIGGEN) ||
        (cmd->mode == MODE_POWER_SUPPLY && current_mode == REPORTER_MODE_POWER)) {
        return;
    }
    
    /* 退出当前模式 - 停止相关功能 */
    switch (current_mode) {
        case REPORTER_MODE_OSC:
            Oscilloscope_Stop();
            Oscilloscope_DeInit();
            break;
        case REPORTER_MODE_METER:
            /* 万用表不需要特殊停止处理 */
            break;
        case REPORTER_MODE_SIGGEN:
            /* 信号发生器保持输出，不停止 */
            break;
        case REPORTER_MODE_POWER:
            /* 电源保持当前状态，不自动关闭 */
            break;
        default:
            break;
    }
    
    /* 根据新模式初始化对应功能并切换数据上报 */
    switch (cmd->mode) {
        case MODE_IDLE:
            DataReporter_SetMode(REPORTER_MODE_IDLE);
            break;
        
        case MODE_OSCILLOSCOPE:
            /* 初始化示波器并启动采样 */
            Oscilloscope_Init();
            Oscilloscope_Start();
            DataReporter_SetMode(REPORTER_MODE_OSC);
            break;
        
        case MODE_MULTIMETER:
            /* 万用表已在main中初始化，只需切换上报模式 */
            DataReporter_SetMode(REPORTER_MODE_METER);
            break;
        
        case MODE_SIGGEN:
            /* 信号发生器已在main中初始化，只需切换上报模式 */
            DataReporter_SetMode(REPORTER_MODE_SIGGEN);
            break;
        
        case MODE_POWER_SUPPLY:
            /* 电源已在main中初始化，只需切换上报模式 */
            DataReporter_SetMode(REPORTER_MODE_POWER);
            break;
        
        default:
            break;
    }
}
