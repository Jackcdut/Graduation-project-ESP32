/**
  ******************************************************************************
  * @file    comm_protocol.h
  * @brief   STM32与ESP32通信协议定义
  ******************************************************************************
  * @description
  *   本文件定义了STM32与ESP32之间的通信协议格式
  *   
  * @protocol
  *   帧格式: [帧头][功能码][数据长度][数据][校验和]
  *           0xAA   1字节    2字节    N字节   1字节
  *   
  *   帧头: 0xAA (固定值，用于帧同步)
  *   功能码: 标识数据类型
  *   数据长度: 数据段的字节数 (小端序，低字节在前)
  *   数据: 实际传输的数据内容
  *   校验和: 从功能码到数据段所有字节的异或值
  *   
  * @note
  *   - 所有多字节数据采用小端序 (Little Endian)
  *   - 浮点数采用IEEE 754标准格式
  ******************************************************************************
  */

#ifndef __COMM_PROTOCOL_H
#define __COMM_PROTOCOL_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/*                           协议常量定义                                     */
/*===========================================================================*/

#define COMM_FRAME_HEADER       0xAA    /* 帧头标识 */
#define COMM_MAX_DATA_LEN       8200    /* 最大数据长度 (4096*2 + 头部) */
#define COMM_HEADER_SIZE        4       /* 帧头大小: 帧头(1) + 功能码(1) + 长度(2) */
#define COMM_CHECKSUM_SIZE      1       /* 校验和大小 */

/*===========================================================================*/
/*                           功能码定义                                       */
/*===========================================================================*/

typedef enum {
    /* STM32 -> ESP32 数据上报 (0x01 - 0x0F) */
    FUNC_OSC_WAVEFORM       = 0x01,     /* 示波器波形数据 */
    FUNC_OSC_MEASUREMENT    = 0x02,     /* 示波器测量结果 */
    FUNC_METER_DATA         = 0x03,     /* 万用表测量数据 */
    FUNC_SIGGEN_STATUS      = 0x04,     /* 信号发生器状态 */
    FUNC_POWER_DATA         = 0x05,     /* 数控电源数据 */
    
    /* ESP32 -> STM32 控制命令 (0x10 - 0x1F) */
    FUNC_OSC_CONTROL        = 0x10,     /* 示波器控制命令 */
    FUNC_METER_CONTROL      = 0x11,     /* 万用表控制命令 */
    FUNC_SIGGEN_CONTROL     = 0x12,     /* 信号发生器控制命令 */
    FUNC_POWER_CONTROL      = 0x13,     /* 数控电源控制命令 */
    FUNC_MODE_CONTROL       = 0x14,     /* 工作模式切换命令 */
    
    /* 系统命令 (0xF0 - 0xFF) */
    FUNC_HEARTBEAT          = 0xFE,     /* 心跳包 */
    FUNC_ACK                = 0xFF,     /* 应答包 */
} CommFuncCode_t;

/*===========================================================================*/
/*                           示波器相关数据结构                               */
/*===========================================================================*/

/**
 * @brief 示波器波形数据帧
 * @note  用于传输ADC采样数据到ESP32显示
 * 
 * PGA增益档位 (MCP6S21):
 *   0=×1, 1=×2, 2=×4, 3=×5, 4=×8, 5=×10, 6=×16, 7=×32
 * 
 * 电压转换公式:
 *   V_input = (1.65 - V_adc) × 50 / G
 *   其中 V_adc = ADC值 × 3.3 / 4095
 */
#pragma pack(push, 1)  /* 按1字节对齐，确保结构体紧凑 */
typedef struct {
    uint8_t  gain;              /* PGA增益档位 (0-7) */
    uint8_t  coupling;          /* 耦合模式: 0=DC, 1=AC */
    uint8_t  state;             /* 运行状态: 0=停止, 1=运行, 2=单次 */
    uint8_t  auto_range;        /* 自动量程: 0=关闭, 1=开启 */
    uint32_t sample_rate;       /* 采样率 (Hz) */
    float    voltage_range;     /* 当前量程 (±V) */
    uint16_t data_count;        /* 数据点数 */
    int16_t  data[];            /* ADC数据数组 (相对于中点的有符号值) */
} OscWaveformFrame_t;
#pragma pack(pop)

/**
 * @brief 示波器测量结果帧
 * @note  用于传输测量计算结果
 */
#pragma pack(push, 1)
typedef struct {
    float    vpp;               /* 峰峰值 (V) */
    float    vmax;              /* 最大值 (V) */
    float    vmin;              /* 最小值 (V) */
    float    vavg;              /* 平均值 (V) */
    float    vrms;              /* 有效值 (V) */
    float    freq;              /* 频率 (Hz) */
    float    period;            /* 周期 (s) */
    float    duty;              /* 占空比 (%) */
} OscMeasurementFrame_t;
#pragma pack(pop)

/**
 * @brief 示波器控制命令帧 (ESP32 -> STM32)
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  cmd_type;          /* 命令类型 */
    uint8_t  gain;              /* PGA增益档位 (0-7) */
    uint8_t  coupling;          /* 耦合模式 */
    uint8_t  auto_range;        /* 自动量程使能 */
    uint32_t sample_rate;       /* 采样率设置 */
} OscControlFrame_t;
#pragma pack(pop)

/* 示波器控制命令类型 */
#define OSC_CMD_START           0x01    /* 开始采样 */
#define OSC_CMD_STOP            0x02    /* 停止采样 */
#define OSC_CMD_SINGLE          0x03    /* 单次采样 */
#define OSC_CMD_SET_GAIN        0x04    /* 设置增益 */
#define OSC_CMD_SET_COUPLING    0x05    /* 设置耦合 */
#define OSC_CMD_SET_SAMPLERATE  0x06    /* 设置采样率 */
#define OSC_CMD_SET_AUTORANGE   0x07    /* 设置自动量程 */

/*===========================================================================*/
/*                           万用表相关数据结构                               */
/*===========================================================================*/

/**
 * @brief 万用表测量数据帧
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  mode;              /* 测量模式: 0=空闲, 1=电压, 2=电流, 3=电阻 */
    uint8_t  range;             /* 量程档位 */
    uint8_t  flags;             /* 状态标志位 */
    uint8_t  reserved;          /* 保留字节 */
    float    value;             /* 测量值 */
    float    secondary;         /* 辅助值 (如电阻测量时的参考电阻) */
} MeterDataFrame_t;
#pragma pack(pop)

/* 万用表状态标志位定义 */
#define METER_FLAG_OVERRANGE    (1 << 0)    /* 超量程 */
#define METER_FLAG_NEGATIVE     (1 << 1)    /* 负极性 */
#define METER_FLAG_OPEN         (1 << 2)    /* 开路 (电阻测量) */
#define METER_FLAG_SHORT        (1 << 3)    /* 短路 (电阻测量) */
#define METER_FLAG_AUTORANGE    (1 << 4)    /* 自动量程模式 */

/**
 * @brief 万用表控制命令帧 (ESP32 -> STM32)
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  cmd_type;          /* 命令类型 */
    uint8_t  mode;              /* 测量模式 */
    uint8_t  range;             /* 量程档位 */
    uint8_t  auto_range;        /* 自动量程使能 */
} MeterControlFrame_t;
#pragma pack(pop)

/* 万用表控制命令类型 */
#define METER_CMD_SET_MODE      0x01    /* 设置测量模式 */
#define METER_CMD_SET_RANGE     0x02    /* 设置量程 */
#define METER_CMD_AUTO_RANGE    0x03    /* 自动量程开关 */

/*===========================================================================*/
/*                           信号发生器相关数据结构                           */
/*===========================================================================*/

/**
 * @brief 信号发生器状态帧
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  wave_type;         /* 波形类型: 0=正弦, 1=三角, 2=方波, 3=方波/2 */
    uint8_t  enabled;           /* 输出使能: 0=关闭, 1=开启 */
    uint8_t  reserved[2];       /* 保留字节 */
    float    frequency;         /* 频率 (Hz) */
    float    amplitude;         /* 幅值 (Vpp) */
} SignalGenFrame_t;
#pragma pack(pop)

/**
 * @brief 信号发生器控制命令帧 (ESP32 -> STM32)
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  cmd_type;          /* 命令类型 */
    uint8_t  wave_type;         /* 波形类型 */
    uint8_t  enabled;           /* 输出使能 */
    uint8_t  reserved;          /* 保留字节 */
    float    frequency;         /* 频率 (Hz) */
    float    amplitude;         /* 幅值 (Vpp) */
} SignalGenControlFrame_t;
#pragma pack(pop)

/* 信号发生器控制命令类型 */
#define SIGGEN_CMD_SET_WAVE     0x01    /* 设置波形类型 */
#define SIGGEN_CMD_SET_FREQ     0x02    /* 设置频率 */
#define SIGGEN_CMD_SET_AMP      0x03    /* 设置幅值 */
#define SIGGEN_CMD_ENABLE       0x04    /* 使能输出 */
#define SIGGEN_CMD_DISABLE      0x05    /* 禁用输出 */
#define SIGGEN_CMD_SET_ALL      0x06    /* 设置所有参数 */

/*===========================================================================*/
/*                           心跳包数据结构                                   */
/*===========================================================================*/

/**
 * @brief 心跳包帧
 */
#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp;         /* 时间戳 (ms) */
    uint8_t  device_status;     /* 设备状态 */
    uint8_t  current_mode;      /* 当前工作模式 */
    uint8_t  reserved[2];       /* 保留字节 */
} HeartbeatFrame_t;
#pragma pack(pop)

/* 设备状态定义 */
#define DEVICE_STATUS_IDLE      0x00    /* 空闲 */
#define DEVICE_STATUS_OSC       0x01    /* 示波器模式 */
#define DEVICE_STATUS_METER     0x02    /* 万用表模式 */
#define DEVICE_STATUS_SIGGEN    0x03    /* 信号发生器模式 */
#define DEVICE_STATUS_POWER     0x04    /* 数控电源模式 */
#define DEVICE_STATUS_ERROR     0xFF    /* 错误状态 */

/*===========================================================================*/
/*                           数控电源相关数据结构                             */
/*===========================================================================*/

/**
 * @brief 数控电源数据帧 (STM32 -> ESP32)
 * @note  用于上报电源状态和测量数据
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  state;             /* 工作状态: 0=关闭, 1=软启动, 2=运行, 3=CC, 4=CV, 5=OTP, 6=OVP, 7=OCP */
    uint8_t  output_enable;     /* 输出使能: 0=关闭, 1=开启 */
    uint8_t  mode;              /* 控制模式: 0=CV, 1=CC, 2=AUTO */
    uint8_t  pd_voltage;        /* PD输入电压档位: 0=5V, 1=9V, 2=12V, 3=20V, 4=28V */
    float    voltage_set;       /* 设定电压 (V) */
    float    current_set;       /* 设定电流 (A) */
    float    voltage_out;       /* 输出电压 (V) */
    float    current_out;       /* 输出电流 (A) */
    float    power_out;         /* 输出功率 (W) */
    float    voltage_in;        /* 输入电压 (V) */
    float    temperature;       /* 温度 (°C) */
} PowerDataFrame_t;
#pragma pack(pop)

/**
 * @brief 数控电源控制命令帧 (ESP32 -> STM32)
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  cmd_type;          /* 命令类型 */
    uint8_t  output_enable;     /* 输出使能 */
    uint8_t  mode;              /* 控制模式 */
    uint8_t  pd_voltage;        /* PD输入电压档位 */
    float    voltage_set;       /* 设定电压 (V) */
    float    current_set;       /* 设定电流 (A) */
} PowerControlFrame_t;
#pragma pack(pop)

/* 数控电源控制命令类型 */
#define POWER_CMD_ENABLE        0x01    /* 使能输出 */
#define POWER_CMD_DISABLE       0x02    /* 禁用输出 */
#define POWER_CMD_SET_VOLTAGE   0x03    /* 设置电压 */
#define POWER_CMD_SET_CURRENT   0x04    /* 设置电流 */
#define POWER_CMD_SET_MODE      0x05    /* 设置模式 */
#define POWER_CMD_SET_PD        0x06    /* 设置PD电压 */
#define POWER_CMD_SET_ALL       0x07    /* 设置所有参数 */
#define POWER_CMD_CLEAR_PROT    0x08    /* 清除保护状态 */

/*===========================================================================*/
/*                           工作模式控制数据结构                             */
/*===========================================================================*/

/**
 * @brief 工作模式控制命令帧 (ESP32 -> STM32)
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  mode;              /* 工作模式: 0=空闲, 1=示波器, 2=万用表, 3=信号发生器, 4=电源 */
    uint8_t  reserved[3];       /* 保留字节 */
} ModeControlFrame_t;
#pragma pack(pop)

/* 工作模式定义 */
#define MODE_IDLE               0x00    /* 空闲模式 */
#define MODE_OSCILLOSCOPE       0x01    /* 示波器模式 */
#define MODE_MULTIMETER         0x02    /* 万用表模式 */
#define MODE_SIGGEN             0x03    /* 信号发生器模式 */
#define MODE_POWER_SUPPLY       0x04    /* 数控电源模式 */

/*===========================================================================*/
/*                           协议处理函数声明                                 */
/*===========================================================================*/

/**
 * @brief  计算校验和
 * @param  data: 数据指针 (从功能码开始)
 * @param  len: 数据长度 (功能码 + 长度字段 + 数据)
 * @retval 校验和值
 */
uint8_t Comm_CalcChecksum(const uint8_t *data, uint16_t len);

/**
 * @brief  构建数据帧
 * @param  buffer: 输出缓冲区
 * @param  func_code: 功能码
 * @param  data: 数据指针
 * @param  data_len: 数据长度
 * @retval 帧总长度
 */
uint16_t Comm_BuildFrame(uint8_t *buffer, CommFuncCode_t func_code, 
                         const void *data, uint16_t data_len);

/**
 * @brief  解析数据帧
 * @param  buffer: 输入缓冲区
 * @param  len: 缓冲区长度
 * @param  func_code: 输出功能码
 * @param  data: 输出数据指针
 * @param  data_len: 输出数据长度
 * @retval 0=成功, 其他=错误码
 */
int Comm_ParseFrame(const uint8_t *buffer, uint16_t len,
                    CommFuncCode_t *func_code, uint8_t **data, uint16_t *data_len);

#endif /* __COMM_PROTOCOL_H */
