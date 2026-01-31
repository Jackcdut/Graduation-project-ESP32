/**
  ******************************************************************************
  * @file    data_reporter.c
  * @brief   数据上报模块实现
  ******************************************************************************
  * @description
  *   本文件实现定时数据上报功能
  *   根据当前工作模式，定时向ESP32发送相应的测量数据
  ******************************************************************************
  */

#include "data_reporter.h"
#include "uart_comm.h"
#include "comm_protocol.h"
#include "oscilloscope.h"
#include "multimeter.h"
#include "signal_generator.h"
#include "power_supply.h"
#include <string.h>

/*===========================================================================*/
/*                           私有变量                                         */
/*===========================================================================*/

/* 当前工作模式 */
static ReporterMode_t g_current_mode = REPORTER_MODE_IDLE;

/* 上报使能标志 */
static bool g_enable_osc_waveform = false;
static bool g_enable_osc_measurement = false;
static bool g_enable_meter_data = false;
static bool g_enable_siggen_status = false;
static bool g_enable_power_data = false;
static bool g_enable_heartbeat = true;  /* 心跳默认开启 */

/* 上次上报时间戳 */
static uint32_t g_last_osc_waveform_time = 0;
static uint32_t g_last_osc_measurement_time = 0;
static uint32_t g_last_meter_time = 0;
static uint32_t g_last_siggen_time = 0;
static uint32_t g_last_power_time = 0;
static uint32_t g_last_heartbeat_time = 0;

/*===========================================================================*/
/*                           私有函数声明                                     */
/*===========================================================================*/

static void DataReporter_SendOscWaveform(void);
static void DataReporter_SendOscMeasurement(void);
static void DataReporter_SendMeterData(void);
static void DataReporter_SendSignalGenStatus(void);
static void DataReporter_SendPowerData(void);
static void DataReporter_SendHeartbeat(void);

/*===========================================================================*/
/*                           初始化函数                                       */
/*===========================================================================*/

/**
 * @brief  数据上报模块初始化
 */
void DataReporter_Init(void)
{
    /* 初始化时间戳 */
    uint32_t now = HAL_GetTick();
    g_last_osc_waveform_time = now;
    g_last_osc_measurement_time = now;
    g_last_meter_time = now;
    g_last_siggen_time = now;
    g_last_power_time = now;
    g_last_heartbeat_time = now;
    
    /* 默认模式为空闲 */
    g_current_mode = REPORTER_MODE_IDLE;
    
    /* 默认只开启心跳 */
    g_enable_osc_waveform = false;
    g_enable_osc_measurement = false;
    g_enable_meter_data = false;
    g_enable_siggen_status = false;
    g_enable_power_data = false;
    g_enable_heartbeat = true;
}

/*===========================================================================*/
/*                           模式控制函数                                     */
/*===========================================================================*/

/**
 * @brief  设置工作模式
 */
void DataReporter_SetMode(ReporterMode_t mode)
{
    g_current_mode = mode;
    
    /* 根据模式自动配置上报使能 */
    switch (mode) {
        case REPORTER_MODE_IDLE:
            g_enable_osc_waveform = false;
            g_enable_osc_measurement = false;
            g_enable_meter_data = false;
            g_enable_siggen_status = false;
            g_enable_power_data = false;
            break;
        
        case REPORTER_MODE_OSC:
            g_enable_osc_waveform = true;
            g_enable_osc_measurement = true;
            g_enable_meter_data = false;
            g_enable_siggen_status = false;
            g_enable_power_data = false;
            break;
        
        case REPORTER_MODE_METER:
            g_enable_osc_waveform = false;
            g_enable_osc_measurement = false;
            g_enable_meter_data = true;
            g_enable_siggen_status = false;
            g_enable_power_data = false;
            break;
        
        case REPORTER_MODE_SIGGEN:
            g_enable_osc_waveform = false;
            g_enable_osc_measurement = false;
            g_enable_meter_data = false;
            g_enable_siggen_status = true;
            g_enable_power_data = false;
            break;
        
        case REPORTER_MODE_POWER:
            g_enable_osc_waveform = false;
            g_enable_osc_measurement = false;
            g_enable_meter_data = false;
            g_enable_siggen_status = false;
            g_enable_power_data = true;
            break;
        
        case REPORTER_MODE_ALL:
            g_enable_osc_waveform = true;
            g_enable_osc_measurement = true;
            g_enable_meter_data = true;
            g_enable_siggen_status = true;
            g_enable_power_data = true;
            break;
    }
}

/**
 * @brief  获取当前工作模式
 */
ReporterMode_t DataReporter_GetMode(void)
{
    return g_current_mode;
}

/*===========================================================================*/
/*                           使能控制函数                                     */
/*===========================================================================*/

void DataReporter_EnableOscWaveform(bool enable)
{
    g_enable_osc_waveform = enable;
}

void DataReporter_EnableOscMeasurement(bool enable)
{
    g_enable_osc_measurement = enable;
}

void DataReporter_EnableMeterData(bool enable)
{
    g_enable_meter_data = enable;
}

void DataReporter_EnableSignalGenStatus(bool enable)
{
    g_enable_siggen_status = enable;
}

void DataReporter_EnablePowerData(bool enable)
{
    g_enable_power_data = enable;
}

void DataReporter_EnableHeartbeat(bool enable)
{
    g_enable_heartbeat = enable;
}

/*===========================================================================*/
/*                           周期处理函数                                     */
/*===========================================================================*/

/**
 * @brief  数据上报周期处理
 * @note   在主循环中调用
 */
void DataReporter_Process(void)
{
    uint32_t now = HAL_GetTick();
    
    /* 检查发送状态，如果正在发送则跳过 */
    if (UartComm_GetTxState() == UART_TX_BUSY) {
        return;
    }
    
    /* 示波器波形数据上报 */
    if (g_enable_osc_waveform) {
        if ((now - g_last_osc_waveform_time) >= REPORT_PERIOD_OSC_WAVEFORM) {
            DataReporter_SendOscWaveform();
            g_last_osc_waveform_time = now;
            return;  /* 每次只发送一种数据，避免阻塞 */
        }
    }
    
    /* 示波器测量结果上报 */
    if (g_enable_osc_measurement) {
        if ((now - g_last_osc_measurement_time) >= REPORT_PERIOD_OSC_MEASUREMENT) {
            DataReporter_SendOscMeasurement();
            g_last_osc_measurement_time = now;
            return;
        }
    }
    
    /* 万用表数据上报 */
    if (g_enable_meter_data) {
        if ((now - g_last_meter_time) >= REPORT_PERIOD_METER) {
            DataReporter_SendMeterData();
            g_last_meter_time = now;
            return;
        }
    }
    
    /* 信号发生器状态上报 */
    if (g_enable_siggen_status) {
        if ((now - g_last_siggen_time) >= REPORT_PERIOD_SIGGEN) {
            DataReporter_SendSignalGenStatus();
            g_last_siggen_time = now;
            return;
        }
    }
    
    /* 数控电源数据上报 */
    if (g_enable_power_data) {
        if ((now - g_last_power_time) >= REPORT_PERIOD_POWER) {
            DataReporter_SendPowerData();
            g_last_power_time = now;
            return;
        }
    }
    
    /* 心跳包上报 */
    if (g_enable_heartbeat) {
        if ((now - g_last_heartbeat_time) >= REPORT_PERIOD_HEARTBEAT) {
            DataReporter_SendHeartbeat();
            g_last_heartbeat_time = now;
            return;
        }
    }
}

/*===========================================================================*/
/*                           数据发送函数                                     */
/*===========================================================================*/

/**
 * @brief  发送示波器波形数据
 */
static void DataReporter_SendOscWaveform(void)
{
    /* 获取示波器配置 */
    OscConfig_t *config = Oscilloscope_GetConfig();
    
    /* 检查示波器状态 */
    if (config->state != OSC_STATE_COMPLETE && config->state != OSC_STATE_RUNNING) {
        return;  /* 没有数据可发送 */
    }
    
    /* 获取采样数据 */
    uint32_t *adc_buffer = Oscilloscope_GetBuffer();
    uint16_t data_count = OSC_BUFFER_SIZE;
    
    /* 限制发送的数据点数 (避免数据量过大) */
    if (data_count > 1024) {
        data_count = 1024;  /* 最多发送1024点 */
    }
    
    /* 转换ADC数据为有符号值 (相对于中点2048) */
    static int16_t signed_data[1024];
    for (uint16_t i = 0; i < data_count; i++) {
        /* 三ADC模式下，数据是32位，取低12位 */
        uint16_t adc_val = (uint16_t)(adc_buffer[i] & 0x0FFF);
        signed_data[i] = (int16_t)(adc_val - 2048);
    }
    
    /* 计算采样率 */
    uint32_t sample_rate = Oscilloscope_GetSampleRateHz();
    
    /* 获取当前电压量程 */
    float voltage_range = Oscilloscope_GetVoltageRange();
    
    /* 发送波形数据 */
    UartComm_SendOscWaveform(
        config->pgaGain,
        config->coupling,
        config->state,
        config->autoRange,
        sample_rate,
        voltage_range,
        signed_data,
        data_count
    );
}

/**
 * @brief  发送示波器测量结果
 */
static void DataReporter_SendOscMeasurement(void)
{
    /* 获取测量结果 */
    OscMeasurement_t *meas = Oscilloscope_GetMeasurement();
    
    /* 构建测量结果帧 */
    OscMeasurementFrame_t frame;
    frame.vpp = meas->vpp;
    frame.vmax = meas->vmax;
    frame.vmin = meas->vmin;
    frame.vavg = meas->vavg;
    frame.vrms = meas->vrms;
    frame.freq = meas->freq;
    frame.period = (meas->freq > 0) ? (1.0f / meas->freq) : 0.0f;
    frame.duty = meas->duty;
    
    /* 发送测量结果 */
    UartComm_SendOscMeasurement(&frame);
}

/**
 * @brief  发送万用表数据
 */
static void DataReporter_SendMeterData(void)
{
    /* 执行测量 */
    MeterResult_t result = Multimeter_Measure();
    
    /* 构建状态标志 */
    uint8_t flags = 0;
    if (result.overrange) {
        flags |= METER_FLAG_OVERRANGE;
    }
    if (result.polarity < 0) {
        flags |= METER_FLAG_NEGATIVE;
    }
    if (result.open_circuit) {
        flags |= METER_FLAG_OPEN;
    }
    if (result.short_circuit) {
        flags |= METER_FLAG_SHORT;
    }
    if (Multimeter_IsAutoRangeEnabled()) {
        flags |= METER_FLAG_AUTORANGE;
    }
    
    /* 根据模式选择测量值 */
    float value = 0.0f;
    switch (result.mode) {
        case METER_MODE_VOLTAGE:
            value = result.voltage;
            break;
        case METER_MODE_CURRENT:
            value = result.current;
            break;
        case METER_MODE_RESISTANCE:
            value = result.resistance;
            break;
        default:
            break;
    }
    
    /* 发送万用表数据 */
    UartComm_SendMeterData(result.mode, result.res_range, flags, value);
}

/**
 * @brief  发送信号发生器状态
 */
static void DataReporter_SendSignalGenStatus(void)
{
    /* 获取当前信号发生器设置 */
    /* 注: 需要在signal_generator模块中添加获取函数 */
    /* 这里使用默认值作为示例 */
    
    UartComm_SendSignalGenStatus(
        0,      /* wave_type: 正弦波 */
        1,      /* enabled: 已使能 */
        1000.0f,    /* frequency: 1kHz */
        1.0f        /* amplitude: 1Vpp */
    );
}

/**
 * @brief  发送数控电源数据
 */
static void DataReporter_SendPowerData(void)
{
    /* 获取电源测量数据 */
    PowerMeasurement_t meas;
    PowerSupply_GetMeasurement(&meas);
    
    /* 获取配置 */
    PowerConfig_t *config = PowerSupply_GetConfig();
    
    /* 构建数据帧 */
    PowerDataFrame_t frame;
    frame.state = (uint8_t)meas.state;
    frame.output_enable = config->output_enable ? 1 : 0;
    frame.mode = (uint8_t)config->mode;
    frame.pd_voltage = (uint8_t)config->pd_voltage;
    frame.voltage_set = config->voltage_set;
    frame.current_set = config->current_set;
    frame.voltage_out = meas.voltage_out;
    frame.current_out = meas.current_out;
    frame.power_out = meas.power_out;
    frame.voltage_in = meas.voltage_in;
    frame.temperature = meas.temperature;
    
    /* 发送数据帧 */
    UartComm_SendFrame(FUNC_POWER_DATA, &frame, sizeof(frame));
}

/**
 * @brief  发送心跳包
 */
static void DataReporter_SendHeartbeat(void)
{
    uint8_t device_status;
    
    /* 根据当前模式设置设备状态 */
    switch (g_current_mode) {
        case REPORTER_MODE_OSC:
            device_status = DEVICE_STATUS_OSC;
            break;
        case REPORTER_MODE_METER:
            device_status = DEVICE_STATUS_METER;
            break;
        case REPORTER_MODE_SIGGEN:
            device_status = DEVICE_STATUS_SIGGEN;
            break;
        case REPORTER_MODE_POWER:
            device_status = DEVICE_STATUS_POWER;
            break;
        default:
            device_status = DEVICE_STATUS_IDLE;
            break;
    }
    
    UartComm_SendHeartbeat(device_status, g_current_mode);
}

/*===========================================================================*/
/*                           立即发送函数                                     */
/*===========================================================================*/

void DataReporter_SendOscWaveformNow(void)
{
    /* 等待上一次发送完成 */
    UartComm_WaitTxComplete(100);
    DataReporter_SendOscWaveform();
}

void DataReporter_SendMeterDataNow(void)
{
    UartComm_WaitTxComplete(100);
    DataReporter_SendMeterData();
}

void DataReporter_SendSignalGenStatusNow(void)
{
    UartComm_WaitTxComplete(100);
    DataReporter_SendSignalGenStatus();
}

void DataReporter_SendPowerDataNow(void)
{
    UartComm_WaitTxComplete(100);
    DataReporter_SendPowerData();
}
