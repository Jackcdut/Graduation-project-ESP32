/**
 * @file stm32_comm.h
 * @brief ESP32-P4与STM32通信模块
 * 
 * 功能：
 * - 串口通信（UART2，GPIO49-TX，GPIO50-RX，921600bps）
 * - 协议解析（与STM32端comm_protocol.h一致）
 * - 信号发生器控制命令发送
 * - 示波器数据接收与处理
 * - 万用表数据接收与控制
 */

#ifndef STM32_COMM_H
#define STM32_COMM_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 协议常量定义（与STM32端一致）
 *============================================================================*/
#define COMM_FRAME_HEADER       0xAA
#define COMM_MAX_DATA_LEN       8200
#define COMM_HEADER_SIZE        4
#define COMM_CHECKSUM_SIZE      1

/*============================================================================
 * 功能码定义
 *============================================================================*/
typedef enum {
    /* STM32 -> ESP32 数据上报 */
    FUNC_OSC_WAVEFORM       = 0x01,
    FUNC_OSC_MEASUREMENT    = 0x02,
    FUNC_METER_DATA         = 0x03,
    FUNC_SIGGEN_STATUS      = 0x04,
    FUNC_POWER_DATA         = 0x05,
    
    /* ESP32 -> STM32 控制命令 */
    FUNC_OSC_CONTROL        = 0x10,
    FUNC_METER_CONTROL      = 0x11,
    FUNC_SIGGEN_CONTROL     = 0x12,
    FUNC_POWER_CONTROL      = 0x13,
    FUNC_MODE_CONTROL       = 0x14,
    
    /* 系统命令 */
    FUNC_HEARTBEAT          = 0xFE,
    FUNC_ACK                = 0xFF,
} stm32_func_code_t;

/*============================================================================
 * 数据结构定义（与STM32端一致，1字节对齐）
 *============================================================================*/

/* 示波器波形数据帧 */
typedef struct __attribute__((packed)) {
    uint8_t  gain;              /* PGA增益档位 (0-7) */
    uint8_t  coupling;          /* 耦合模式: 0=DC, 1=AC */
    uint8_t  state;             /* 运行状态: 0=停止, 1=运行, 2=单次 */
    uint8_t  auto_range;        /* 自动量程: 0=关闭, 1=开启 */
    uint32_t sample_rate;       /* 采样率 (Hz) */
    float    voltage_range;     /* 当前电压量程 (±V) */
    uint16_t data_count;        /* 数据点数 */
    int16_t  data[];            /* ADC数据数组 (有符号值，相对于中点) */
} stm32_osc_waveform_t;

/* 示波器测量结果帧 */
typedef struct __attribute__((packed)) {
    float vpp;
    float vmax;
    float vmin;
    float vavg;
    float vrms;
    float freq;
    float period;
    float duty;
} stm32_osc_measurement_t;

/* 示波器控制命令帧 */
typedef struct __attribute__((packed)) {
    uint8_t  cmd_type;
    uint8_t  gain;
    uint8_t  coupling;
    uint8_t  auto_range;        /* 自动量程使能 */
    uint32_t sample_rate;
} stm32_osc_control_t;

/* 万用表测量数据帧 */
typedef struct __attribute__((packed)) {
    uint8_t  mode;
    uint8_t  range;
    uint8_t  flags;
    uint8_t  reserved;
    float    value;
    float    secondary;
} stm32_meter_data_t;

/* 万用表控制命令帧 */
typedef struct __attribute__((packed)) {
    uint8_t  cmd_type;
    uint8_t  mode;
    uint8_t  range;
    uint8_t  auto_range;
} stm32_meter_control_t;

/* 信号发生器状态帧 */
typedef struct __attribute__((packed)) {
    uint8_t  wave_type;
    uint8_t  enabled;
    uint8_t  reserved[2];
    float    frequency;
    float    amplitude;
} stm32_siggen_status_t;

/* 信号发生器控制命令帧 */
typedef struct __attribute__((packed)) {
    uint8_t  cmd_type;
    uint8_t  wave_type;
    uint8_t  enabled;
    uint8_t  reserved;
    float    frequency;
    float    amplitude;
} stm32_siggen_control_t;

/* 心跳包帧 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint8_t  device_status;
    uint8_t  current_mode;
    uint8_t  reserved[2];
} stm32_heartbeat_t;

/* 数控电源数据帧 */
typedef struct __attribute__((packed)) {
    uint8_t  state;             /* 工作状态 */
    uint8_t  output_enable;     /* 输出使能 */
    uint8_t  mode;              /* 控制模式 */
    uint8_t  pd_voltage;        /* PD输入电压档位 */
    float    voltage_set;       /* 设定电压 (V) */
    float    current_set;       /* 设定电流 (A) */
    float    voltage_out;       /* 输出电压 (V) */
    float    current_out;       /* 输出电流 (A) */
    float    power_out;         /* 输出功率 (W) */
    float    voltage_in;        /* 输入电压 (V) */
    float    temperature;       /* 温度 (°C) */
} stm32_power_data_t;

/* 数控电源控制命令帧 */
typedef struct __attribute__((packed)) {
    uint8_t  cmd_type;          /* 命令类型 */
    uint8_t  output_enable;     /* 输出使能 */
    uint8_t  mode;              /* 控制模式 */
    uint8_t  pd_voltage;        /* PD输入电压档位 */
    float    voltage_set;       /* 设定电压 (V) */
    float    current_set;       /* 设定电流 (A) */
} stm32_power_control_t;

/* 工作模式控制命令帧 */
typedef struct __attribute__((packed)) {
    uint8_t  mode;              /* 工作模式 */
    uint8_t  reserved[3];       /* 保留字节 */
} stm32_mode_control_t;

/* 工作模式定义 */
#define STM32_MODE_IDLE             0x00
#define STM32_MODE_OSCILLOSCOPE     0x01
#define STM32_MODE_MULTIMETER       0x02
#define STM32_MODE_SIGGEN           0x03
#define STM32_MODE_POWER_SUPPLY     0x04

/*============================================================================
 * 命令类型定义
 *============================================================================*/

/* 示波器命令 */
#define OSC_CMD_START           0x01
#define OSC_CMD_STOP            0x02
#define OSC_CMD_SINGLE          0x03
#define OSC_CMD_SET_GAIN        0x04
#define OSC_CMD_SET_COUPLING    0x05
#define OSC_CMD_SET_SAMPLERATE  0x06
#define OSC_CMD_SET_AUTORANGE   0x07

/* 万用表命令 */
#define METER_CMD_SET_MODE      0x01
#define METER_CMD_SET_RANGE     0x02
#define METER_CMD_AUTO_RANGE    0x03

/* 信号发生器命令 */
#define SIGGEN_CMD_SET_WAVE     0x01
#define SIGGEN_CMD_SET_FREQ     0x02
#define SIGGEN_CMD_SET_AMP      0x03
#define SIGGEN_CMD_ENABLE       0x04
#define SIGGEN_CMD_DISABLE      0x05
#define SIGGEN_CMD_SET_ALL      0x06

/* 万用表模式 */
#define METER_MODE_IDLE         0
#define METER_MODE_VOLTAGE      1
#define METER_MODE_CURRENT      2
#define METER_MODE_RESISTANCE   3

/* 万用表标志位 */
#define METER_FLAG_OVERRANGE    (1 << 0)
#define METER_FLAG_NEGATIVE     (1 << 1)
#define METER_FLAG_OPEN         (1 << 2)
#define METER_FLAG_SHORT        (1 << 3)
#define METER_FLAG_AUTORANGE    (1 << 4)

/* 波形类型 */
#define WAVE_SINE               0
#define WAVE_TRIANGLE           1
#define WAVE_SQUARE             2
#define WAVE_SQUARE_DIV2        3

/* 数控电源命令 */
#define POWER_CMD_ENABLE        0x01
#define POWER_CMD_DISABLE       0x02
#define POWER_CMD_SET_VOLTAGE   0x03
#define POWER_CMD_SET_CURRENT   0x04
#define POWER_CMD_SET_MODE      0x05
#define POWER_CMD_SET_PD        0x06
#define POWER_CMD_SET_ALL       0x07
#define POWER_CMD_CLEAR_PROT    0x08

/* 电源工作状态 */
#define POWER_STATE_OFF         0
#define POWER_STATE_SOFTSTART   1
#define POWER_STATE_RUNNING     2
#define POWER_STATE_CC_MODE     3
#define POWER_STATE_CV_MODE     4
#define POWER_STATE_OTP         5
#define POWER_STATE_OVP         6
#define POWER_STATE_OCP         7
#define POWER_STATE_ERROR       8

/* 电源控制模式 */
#define POWER_MODE_CV           0
#define POWER_MODE_CC           1
#define POWER_MODE_AUTO         2

/* PD电压档位 */
#define POWER_PD_5V             0
#define POWER_PD_9V             1
#define POWER_PD_12V            2
#define POWER_PD_20V            3
#define POWER_PD_28V            4

/*============================================================================
 * 回调函数类型定义
 *============================================================================*/

/* 示波器波形数据回调 */
typedef void (*stm32_osc_waveform_cb_t)(const stm32_osc_waveform_t *data, uint16_t total_len);

/* 示波器测量结果回调 */
typedef void (*stm32_osc_measurement_cb_t)(const stm32_osc_measurement_t *data);

/* 万用表数据回调 */
typedef void (*stm32_meter_data_cb_t)(const stm32_meter_data_t *data);

/* 信号发生器状态回调 */
typedef void (*stm32_siggen_status_cb_t)(const stm32_siggen_status_t *data);

/* 数控电源数据回调 */
typedef void (*stm32_power_data_cb_t)(const stm32_power_data_t *data);

/* 心跳回调 */
typedef void (*stm32_heartbeat_cb_t)(const stm32_heartbeat_t *data);

/*============================================================================
 * 回调注册结构
 *============================================================================*/
typedef struct {
    stm32_osc_waveform_cb_t     osc_waveform;
    stm32_osc_measurement_cb_t  osc_measurement;
    stm32_meter_data_cb_t       meter_data;
    stm32_siggen_status_cb_t    siggen_status;
    stm32_power_data_cb_t       power_data;
    stm32_heartbeat_cb_t        heartbeat;
} stm32_comm_callbacks_t;

/*============================================================================
 * 核心API
 *============================================================================*/

/**
 * @brief 初始化STM32通信模块
 * @param callbacks 回调函数结构体指针（可为NULL）
 * @return ESP_OK成功
 */
esp_err_t stm32_comm_init(const stm32_comm_callbacks_t *callbacks);

/**
 * @brief 反初始化通信模块
 */
void stm32_comm_deinit(void);

/**
 * @brief 注册回调函数
 */
void stm32_comm_register_callbacks(const stm32_comm_callbacks_t *callbacks);

/**
 * @brief 检查通信是否已连接（收到心跳）
 */
bool stm32_comm_is_connected(void);

/*============================================================================
 * 信号发生器控制API
 *============================================================================*/

/**
 * @brief 设置信号发生器所有参数
 */
esp_err_t stm32_siggen_set_all(uint8_t wave_type, float frequency, float amplitude);

/**
 * @brief 设置波形类型
 */
esp_err_t stm32_siggen_set_waveform(uint8_t wave_type);

/**
 * @brief 设置频率
 */
esp_err_t stm32_siggen_set_frequency(float frequency);

/**
 * @brief 设置幅值
 */
esp_err_t stm32_siggen_set_amplitude(float amplitude);

/**
 * @brief 使能/禁用输出
 */
esp_err_t stm32_siggen_enable(bool enable);

/*============================================================================
 * 示波器控制API
 *============================================================================*/

/**
 * @brief 启动示波器采样
 */
esp_err_t stm32_osc_start(void);

/**
 * @brief 停止示波器采样
 */
esp_err_t stm32_osc_stop(void);

/**
 * @brief 单次采样
 */
esp_err_t stm32_osc_single(void);

/**
 * @brief 设置增益档位
 */
esp_err_t stm32_osc_set_gain(uint8_t gain);

/**
 * @brief 设置耦合模式
 */
esp_err_t stm32_osc_set_coupling(uint8_t coupling);

/**
 * @brief 设置采样率
 */
esp_err_t stm32_osc_set_samplerate(uint32_t sample_rate);

/**
 * @brief 设置自动量程
 */
esp_err_t stm32_osc_set_autorange(bool enable);

/*============================================================================
 * 万用表控制API
 *============================================================================*/

/**
 * @brief 设置万用表测量模式
 */
esp_err_t stm32_meter_set_mode(uint8_t mode);

/**
 * @brief 设置量程档位
 */
esp_err_t stm32_meter_set_range(uint8_t range);

/**
 * @brief 设置自动量程
 */
esp_err_t stm32_meter_set_autorange(bool enable);

/*============================================================================
 * 发送心跳
 *============================================================================*/
esp_err_t stm32_comm_send_heartbeat(void);

/*============================================================================
 * 数控电源控制API
 *============================================================================*/

/**
 * @brief 使能/禁用电源输出
 */
esp_err_t stm32_power_enable(bool enable);

/**
 * @brief 设置输出电压
 * @param voltage 电压值 (0-12V)
 */
esp_err_t stm32_power_set_voltage(float voltage);

/**
 * @brief 设置输出电流限制
 * @param current 电流值 (0-2A)
 */
esp_err_t stm32_power_set_current(float current);

/**
 * @brief 设置控制模式
 * @param mode 0=CV, 1=CC, 2=AUTO
 */
esp_err_t stm32_power_set_mode(uint8_t mode);

/**
 * @brief 设置PD输入电压
 * @param pd_voltage 0=5V, 1=9V, 2=12V, 3=20V, 4=28V
 */
esp_err_t stm32_power_set_pd_voltage(uint8_t pd_voltage);

/**
 * @brief 设置所有电源参数
 */
esp_err_t stm32_power_set_all(bool enable, float voltage, float current, 
                              uint8_t mode, uint8_t pd_voltage);

/**
 * @brief 清除保护状态
 */
esp_err_t stm32_power_clear_protection(void);

/*============================================================================
 * 工作模式切换API
 *============================================================================*/

/**
 * @brief 设置STM32工作模式
 * @param mode 工作模式 (STM32_MODE_xxx)
 */
esp_err_t stm32_set_mode(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif /* STM32_COMM_H */
