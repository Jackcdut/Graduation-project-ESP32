/**
  ******************************************************************************
  * @file    power_supply.c
  * @brief   数控电源模块实现
  ******************************************************************************
  */

#include "power_supply.h"
#include "dac.h"
#include "adc.h"
#include "gpio.h"
#include <math.h>
#include <string.h>

/*===========================================================================*/
/*                           私有宏定义                                       */
/*===========================================================================*/

/* DAC通道定义 */
#define DAC_CHANNEL_CV      DAC_CHANNEL_1   /* PA4 - 电压设定 */
#define DAC_CHANNEL_CC      DAC_CHANNEL_2   /* PA5 - 电流设定 */

/* CV DAC计算: DAC输出直接作为运放设定值 */
/* PA4输出范围0-3.3V，对应输出电压0-12V */
/* 需要根据实际电路调整比例系数 */
#define CV_DAC_SCALE        (POWER_DAC_RESOLUTION / (POWER_VOLTAGE_MAX / POWER_VFB_DIVIDER_RATIO / POWER_DAC_VREF))

/* CC DAC计算: PA5输出作为电流设定值 */
/* 电流反馈: IFB = I * 1V/A, 所以2A对应2V */
#define CC_DAC_SCALE        (POWER_DAC_RESOLUTION / (POWER_CURRENT_MAX * POWER_IFB_GAIN / POWER_DAC_VREF))

/*===========================================================================*/
/*                           私有变量                                         */
/*===========================================================================*/

/* 电源配置 */
static PowerConfig_t g_config = {
    .voltage_set = POWER_VOLTAGE_DEFAULT,
    .current_set = POWER_CURRENT_DEFAULT,
    .mode = POWER_MODE_AUTO,
    .output_enable = false,
    .pd_voltage = POWER_PD_12V
};

/* 测量数据 */
static PowerMeasurement_t g_measurement = {0};

/* 工作状态 */
static PowerState_t g_state = POWER_STATE_OFF;

/* 软启动相关 */
static bool g_softstart_active = false;
static uint32_t g_softstart_start_time = 0;
static float g_softstart_target_voltage = 0;

/* ADC滤波缓冲区 */
static uint16_t g_vfb_buffer[POWER_FILTER_SAMPLES];
static uint16_t g_ifb_buffer[POWER_FILTER_SAMPLES];
static uint16_t g_vin_buffer[POWER_FILTER_SAMPLES];
static uint16_t g_ntc_buffer[POWER_FILTER_SAMPLES];
static uint16_t g_cc_buffer[POWER_FILTER_SAMPLES];
static uint8_t g_filter_index = 0;

/* 状态回调 */
static PowerStateCallback_t g_state_callback = NULL;

/* 上次处理时间 */
static uint32_t g_last_process_time = 0;

/*===========================================================================*/
/*                           私有函数声明                                     */
/*===========================================================================*/

static void PowerSupply_UpdateDAC(void);
static void PowerSupply_ReadADC(void);
static uint16_t PowerSupply_FilterADC(uint16_t *buffer);
static float PowerSupply_CalcTemperature(uint16_t adc_value);
static void PowerSupply_CheckProtection(void);
static void PowerSupply_SetState(PowerState_t new_state);
static void PowerSupply_SoftStartProcess(void);
static void PowerSupply_ConfigPDPins(PowerPDVoltage_t voltage);

/*===========================================================================*/
/*                           初始化函数                                       */
/*===========================================================================*/

void PowerSupply_Init(void)
{
    /* 清零测量数据 */
    memset(&g_measurement, 0, sizeof(g_measurement));
    
    /* 清零滤波缓冲区 */
    memset(g_vfb_buffer, 0, sizeof(g_vfb_buffer));
    memset(g_ifb_buffer, 0, sizeof(g_ifb_buffer));
    memset(g_vin_buffer, 0, sizeof(g_vin_buffer));
    memset(g_ntc_buffer, 0, sizeof(g_ntc_buffer));
    memset(g_cc_buffer, 0, sizeof(g_cc_buffer));
    g_filter_index = 0;
    
    /* 初始化DAC输出为0 */
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CV, DAC_ALIGN_12B_R, 0);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, 0);
    
    /* 启动DAC */
    HAL_DAC_Start(&hdac, DAC_CHANNEL_CV);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_CC);
    
    /* 配置PD电压 */
    PowerSupply_ConfigPDPins(g_config.pd_voltage);
    
    /* 设置初始状态 */
    g_state = POWER_STATE_OFF;
    g_config.output_enable = false;
    
    g_last_process_time = HAL_GetTick();
}

void PowerSupply_DeInit(void)
{
    /* 关闭输出 */
    PowerSupply_EnableOutput(false);
    
    /* 停止DAC */
    HAL_DAC_Stop(&hdac, DAC_CHANNEL_CV);
    HAL_DAC_Stop(&hdac, DAC_CHANNEL_CC);
    
    g_state = POWER_STATE_OFF;
}

/*===========================================================================*/
/*                           输出控制函数                                     */
/*===========================================================================*/

void PowerSupply_EnableOutput(bool enable)
{
    if (enable && !g_config.output_enable) {
        /* 检查是否处于保护状态 */
        if (g_state == POWER_STATE_OTP || g_state == POWER_STATE_OVP || 
            g_state == POWER_STATE_OCP || g_state == POWER_STATE_ERROR) {
            return;  /* 保护状态下不允许开启 */
        }
        
        /* 启动软启动 */
        g_softstart_active = true;
        g_softstart_start_time = HAL_GetTick();
        g_softstart_target_voltage = g_config.voltage_set;
        
        /* 先设置电流限制 */
        uint32_t cc_dac = (uint32_t)(g_config.current_set * CC_DAC_SCALE);
        if (cc_dac > POWER_DAC_RESOLUTION - 1) cc_dac = POWER_DAC_RESOLUTION - 1;
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, cc_dac);
        
        g_config.output_enable = true;
        PowerSupply_SetState(POWER_STATE_SOFTSTART);
        
    } else if (!enable && g_config.output_enable) {
        /* 关闭输出 */
        g_config.output_enable = false;
        g_softstart_active = false;
        
        /* DAC输出设为0 */
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CV, DAC_ALIGN_12B_R, 0);
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, 0);
        
        PowerSupply_SetState(POWER_STATE_OFF);
    }
}

bool PowerSupply_IsOutputEnabled(void)
{
    return g_config.output_enable;
}

int PowerSupply_SetVoltage(float voltage)
{
    /* 参数检查 */
    if (voltage < POWER_VOLTAGE_MIN || voltage > POWER_VOLTAGE_MAX) {
        return -1;
    }
    
    g_config.voltage_set = voltage;
    
    /* 如果输出已使能且不在软启动中，立即更新DAC */
    if (g_config.output_enable && !g_softstart_active) {
        PowerSupply_UpdateDAC();
    }
    
    return 0;
}

int PowerSupply_SetCurrent(float current)
{
    /* 参数检查 */
    if (current < POWER_CURRENT_MIN || current > POWER_CURRENT_MAX) {
        return -1;
    }
    
    g_config.current_set = current;
    
    /* 如果输出已使能，立即更新DAC */
    if (g_config.output_enable) {
        uint32_t cc_dac = (uint32_t)(current * CC_DAC_SCALE);
        if (cc_dac > POWER_DAC_RESOLUTION - 1) cc_dac = POWER_DAC_RESOLUTION - 1;
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, cc_dac);
    }
    
    return 0;
}

float PowerSupply_GetSetVoltage(void)
{
    return g_config.voltage_set;
}

float PowerSupply_GetSetCurrent(void)
{
    return g_config.current_set;
}

/*===========================================================================*/
/*                           测量函数                                         */
/*===========================================================================*/

float PowerSupply_GetVoltage(void)
{
    return g_measurement.voltage_out;
}

float PowerSupply_GetCurrent(void)
{
    return g_measurement.current_out;
}

float PowerSupply_GetPower(void)
{
    return g_measurement.power_out;
}

float PowerSupply_GetInputVoltage(void)
{
    return g_measurement.voltage_in;
}

float PowerSupply_GetTemperature(void)
{
    return g_measurement.temperature;
}

void PowerSupply_GetMeasurement(PowerMeasurement_t *measurement)
{
    if (measurement != NULL) {
        memcpy(measurement, &g_measurement, sizeof(PowerMeasurement_t));
    }
}

/*===========================================================================*/
/*                           状态函数                                         */
/*===========================================================================*/

PowerState_t PowerSupply_GetState(void)
{
    return g_state;
}

bool PowerSupply_IsCCMode(void)
{
    return g_measurement.is_cc_mode;
}

void PowerSupply_SetMode(PowerMode_t mode)
{
    g_config.mode = mode;
}

PowerMode_t PowerSupply_GetMode(void)
{
    return g_config.mode;
}

/*===========================================================================*/
/*                           PD配置函数                                       */
/*===========================================================================*/

void PowerSupply_SetPDVoltage(PowerPDVoltage_t voltage)
{
    g_config.pd_voltage = voltage;
    PowerSupply_ConfigPDPins(voltage);
}

PowerPDVoltage_t PowerSupply_GetPDVoltage(void)
{
    return g_config.pd_voltage;
}

/**
 * @brief  配置CH224A的CFG引脚
 * @note   根据数据手册:
 *         CFG1 CFG2 CFG3 -> 请求电压
 *         0    0    0    -> 9V
 *         0    0    1    -> 12V
 *         0    1    1    -> 20V
 *         0    1    0    -> 28V
 *         1    X    X    -> 5V
 */
static void PowerSupply_ConfigPDPins(PowerPDVoltage_t voltage)
{
    GPIO_PinState cfg1 = GPIO_PIN_RESET;
    GPIO_PinState cfg2 = GPIO_PIN_RESET;
    GPIO_PinState cfg3 = GPIO_PIN_RESET;
    
    switch (voltage) {
        case POWER_PD_5V:
            cfg1 = GPIO_PIN_SET;
            cfg2 = GPIO_PIN_RESET;
            cfg3 = GPIO_PIN_RESET;
            break;
        case POWER_PD_9V:
            cfg1 = GPIO_PIN_RESET;
            cfg2 = GPIO_PIN_RESET;
            cfg3 = GPIO_PIN_RESET;
            break;
        case POWER_PD_12V:
            cfg1 = GPIO_PIN_RESET;
            cfg2 = GPIO_PIN_RESET;
            cfg3 = GPIO_PIN_SET;
            break;
        case POWER_PD_20V:
            cfg1 = GPIO_PIN_RESET;
            cfg2 = GPIO_PIN_SET;
            cfg3 = GPIO_PIN_SET;
            break;
        case POWER_PD_28V:
            cfg1 = GPIO_PIN_RESET;
            cfg2 = GPIO_PIN_SET;
            cfg3 = GPIO_PIN_RESET;
            break;
        default:
            cfg1 = GPIO_PIN_SET;  /* 默认5V */
            break;
    }
    
    HAL_GPIO_WritePin(POWER_CFG1_GPIO_Port, POWER_CFG1_Pin, cfg1);
    HAL_GPIO_WritePin(POWER_CFG2_GPIO_Port, POWER_CFG2_Pin, cfg2);
    HAL_GPIO_WritePin(POWER_CFG3_GPIO_Port, POWER_CFG3_Pin, cfg3);
}

/*===========================================================================*/
/*                           配置函数                                         */
/*===========================================================================*/

PowerConfig_t* PowerSupply_GetConfig(void)
{
    return &g_config;
}

void PowerSupply_ApplyConfig(const PowerConfig_t *config)
{
    if (config == NULL) return;
    
    /* 保存输出使能状态 */
    bool was_enabled = g_config.output_enable;
    
    /* 复制配置 */
    g_config.voltage_set = config->voltage_set;
    g_config.current_set = config->current_set;
    g_config.mode = config->mode;
    g_config.pd_voltage = config->pd_voltage;
    
    /* 限制参数范围 */
    if (g_config.voltage_set < POWER_VOLTAGE_MIN) g_config.voltage_set = POWER_VOLTAGE_MIN;
    if (g_config.voltage_set > POWER_VOLTAGE_MAX) g_config.voltage_set = POWER_VOLTAGE_MAX;
    if (g_config.current_set < POWER_CURRENT_MIN) g_config.current_set = POWER_CURRENT_MIN;
    if (g_config.current_set > POWER_CURRENT_MAX) g_config.current_set = POWER_CURRENT_MAX;
    
    /* 配置PD电压 */
    PowerSupply_ConfigPDPins(g_config.pd_voltage);
    
    /* 处理输出使能变化 */
    if (config->output_enable != was_enabled) {
        PowerSupply_EnableOutput(config->output_enable);
    } else if (g_config.output_enable && !g_softstart_active) {
        /* 输出已使能，更新DAC */
        PowerSupply_UpdateDAC();
    }
}

/*===========================================================================*/
/*                           回调函数                                         */
/*===========================================================================*/

void PowerSupply_RegisterStateCallback(PowerStateCallback_t callback)
{
    g_state_callback = callback;
}

static void PowerSupply_SetState(PowerState_t new_state)
{
    if (g_state != new_state) {
        g_state = new_state;
        g_measurement.state = new_state;
        
        if (g_state_callback != NULL) {
            g_state_callback(new_state);
        }
    }
}

/*===========================================================================*/
/*                           周期处理函数                                     */
/*===========================================================================*/

void PowerSupply_Process(void)
{
    uint32_t now = HAL_GetTick();
    
    /* 限制处理频率 (每10ms处理一次) */
    if ((now - g_last_process_time) < 10) {
        return;
    }
    g_last_process_time = now;
    
    /* 读取ADC */
    PowerSupply_ReadADC();
    
    /* 软启动处理 */
    if (g_softstart_active) {
        PowerSupply_SoftStartProcess();
    }
    
    /* 保护检测 */
    PowerSupply_CheckProtection();
    
    /* 更新CC/CV模式状态 */
    if (g_config.output_enable && g_state == POWER_STATE_RUNNING) {
        /* 通过比较CC运放输出判断当前模式 */
        uint16_t cc_adc = PowerSupply_FilterADC(g_cc_buffer);
        float cc_voltage = (float)cc_adc * POWER_ADC_VREF / POWER_ADC_RESOLUTION;
        
        /* CC运放输出低于阈值表示处于CC模式 */
        if (cc_voltage < 1.0f) {
            g_measurement.is_cc_mode = true;
            g_measurement.is_cv_mode = false;
            PowerSupply_SetState(POWER_STATE_CC_MODE);
        } else {
            g_measurement.is_cc_mode = false;
            g_measurement.is_cv_mode = true;
            PowerSupply_SetState(POWER_STATE_CV_MODE);
        }
    }
}

void PowerSupply_ClearProtection(void)
{
    if (g_state == POWER_STATE_OTP || g_state == POWER_STATE_OVP || 
        g_state == POWER_STATE_OCP) {
        /* 检查温度是否已恢复 */
        if (g_measurement.temperature < POWER_TEMP_RECOVERY) {
            PowerSupply_SetState(POWER_STATE_OFF);
        }
    }
}

/*===========================================================================*/
/*                           私有函数实现                                     */
/*===========================================================================*/

/**
 * @brief  更新DAC输出
 */
static void PowerSupply_UpdateDAC(void)
{
    if (!g_config.output_enable) return;
    
    /* 计算CV DAC值 */
    /* 输出电压 -> VFB电压 -> DAC电压 */
    float vfb_target = g_config.voltage_set * POWER_VFB_DIVIDER_RATIO;
    uint32_t cv_dac = (uint32_t)(vfb_target / POWER_DAC_VREF * POWER_DAC_RESOLUTION);
    if (cv_dac > POWER_DAC_RESOLUTION - 1) cv_dac = POWER_DAC_RESOLUTION - 1;
    
    /* 计算CC DAC值 */
    float ifb_target = g_config.current_set * POWER_IFB_GAIN;
    uint32_t cc_dac = (uint32_t)(ifb_target / POWER_DAC_VREF * POWER_DAC_RESOLUTION);
    if (cc_dac > POWER_DAC_RESOLUTION - 1) cc_dac = POWER_DAC_RESOLUTION - 1;
    
    /* 设置DAC */
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CV, DAC_ALIGN_12B_R, cv_dac);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, cc_dac);
}

/**
 * @brief  读取ADC并更新测量值
 */
static void PowerSupply_ReadADC(void)
{
    /* 注意: 这里假设ADC3用于电源模块的采样 */
    /* 实际实现需要根据CubeMX配置调整 */
    
    /* 配置ADC通道并读取 - VFB (PF6, ADC3_IN4) */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_4;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);
    HAL_ADC_PollForConversion(&hadc3, 10);
    g_vfb_buffer[g_filter_index] = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);
    
    /* IFB (PF7, ADC3_IN5) */
    sConfig.Channel = ADC_CHANNEL_5;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);
    HAL_ADC_PollForConversion(&hadc3, 10);
    g_ifb_buffer[g_filter_index] = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);
    
    /* VIN (PF8, ADC3_IN6) */
    sConfig.Channel = ADC_CHANNEL_6;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);
    HAL_ADC_PollForConversion(&hadc3, 10);
    g_vin_buffer[g_filter_index] = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);
    
    /* NTC (PF9, ADC3_IN7) */
    sConfig.Channel = ADC_CHANNEL_7;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);
    HAL_ADC_PollForConversion(&hadc3, 10);
    g_ntc_buffer[g_filter_index] = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);
    
    /* CC状态 (PF10, ADC3_IN8) */
    sConfig.Channel = ADC_CHANNEL_8;
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    HAL_ADC_Start(&hadc3);
    HAL_ADC_PollForConversion(&hadc3, 10);
    g_cc_buffer[g_filter_index] = HAL_ADC_GetValue(&hadc3);
    HAL_ADC_Stop(&hadc3);
    
    /* 更新滤波索引 */
    g_filter_index = (g_filter_index + 1) % POWER_FILTER_SAMPLES;
    
    /* 计算滤波后的值并转换为实际物理量 */
    uint16_t vfb_filtered = PowerSupply_FilterADC(g_vfb_buffer);
    uint16_t ifb_filtered = PowerSupply_FilterADC(g_ifb_buffer);
    uint16_t vin_filtered = PowerSupply_FilterADC(g_vin_buffer);
    uint16_t ntc_filtered = PowerSupply_FilterADC(g_ntc_buffer);
    
    /* 转换为电压 */
    float vfb_voltage = (float)vfb_filtered * POWER_ADC_VREF / POWER_ADC_RESOLUTION;
    float ifb_voltage = (float)ifb_filtered * POWER_ADC_VREF / POWER_ADC_RESOLUTION;
    float vin_voltage = (float)vin_filtered * POWER_ADC_VREF / POWER_ADC_RESOLUTION;
    
    /* 计算实际值 */
    g_measurement.voltage_out = vfb_voltage / POWER_VFB_DIVIDER_RATIO;
    g_measurement.current_out = ifb_voltage / POWER_IFB_GAIN;
    g_measurement.voltage_in = vin_voltage / POWER_VIN_DIVIDER_RATIO;
    g_measurement.power_out = g_measurement.voltage_out * g_measurement.current_out;
    g_measurement.temperature = PowerSupply_CalcTemperature(ntc_filtered);
}

/**
 * @brief  ADC滤波 (移动平均)
 */
static uint16_t PowerSupply_FilterADC(uint16_t *buffer)
{
    uint32_t sum = 0;
    for (int i = 0; i < POWER_FILTER_SAMPLES; i++) {
        sum += buffer[i];
    }
    return (uint16_t)(sum / POWER_FILTER_SAMPLES);
}

/**
 * @brief  计算NTC温度
 * @param  adc_value: ADC采样值
 * @retval 温度 (°C)
 */
static float PowerSupply_CalcTemperature(uint16_t adc_value)
{
    if (adc_value == 0 || adc_value >= POWER_ADC_RESOLUTION - 1) {
        return -999.0f;  /* 无效值 */
    }
    
    /* 计算NTC电阻值 */
    /* Vntc = Vadc = Vref * adc / 4096 */
    /* Rntc = Rpullup * Vntc / (Vref - Vntc) */
    float v_adc = (float)adc_value * POWER_ADC_VREF / POWER_ADC_RESOLUTION;
    float r_ntc = POWER_NTC_PULLUP * v_adc / (POWER_ADC_VREF - v_adc);
    
    /* 使用B值公式计算温度 */
    /* 1/T = 1/T25 + (1/B) * ln(R/R25) */
    float temp_k = 1.0f / (1.0f / POWER_NTC_T25 + (1.0f / POWER_NTC_B) * logf(r_ntc / POWER_NTC_R25));
    
    /* 转换为摄氏度 */
    return temp_k - 273.15f;
}

/**
 * @brief  保护检测
 */
static void PowerSupply_CheckProtection(void)
{
    /* 过温保护 */
    if (g_measurement.temperature > POWER_TEMP_SHUTDOWN) {
        if (g_config.output_enable) {
            /* 关闭输出 */
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CV, DAC_ALIGN_12B_R, 0);
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, 0);
            g_config.output_enable = false;
            g_softstart_active = false;
        }
        PowerSupply_SetState(POWER_STATE_OTP);
        return;
    }
    
    /* 过压保护 (输出电压超过设定值20%) */
    if (g_config.output_enable && g_measurement.voltage_out > g_config.voltage_set * 1.2f) {
        if (g_measurement.voltage_out > POWER_VOLTAGE_MAX * 1.1f) {
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CV, DAC_ALIGN_12B_R, 0);
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, 0);
            g_config.output_enable = false;
            g_softstart_active = false;
            PowerSupply_SetState(POWER_STATE_OVP);
            return;
        }
    }
    
    /* 过流保护 (输出电流超过设定值50%) */
    if (g_config.output_enable && g_measurement.current_out > g_config.current_set * 1.5f) {
        if (g_measurement.current_out > POWER_CURRENT_MAX * 1.2f) {
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CV, DAC_ALIGN_12B_R, 0);
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CC, DAC_ALIGN_12B_R, 0);
            g_config.output_enable = false;
            g_softstart_active = false;
            PowerSupply_SetState(POWER_STATE_OCP);
            return;
        }
    }
}

/**
 * @brief  软启动处理
 */
static void PowerSupply_SoftStartProcess(void)
{
    uint32_t elapsed = HAL_GetTick() - g_softstart_start_time;
    
    if (elapsed >= POWER_SOFTSTART_TIME_MS) {
        /* 软启动完成 */
        g_softstart_active = false;
        PowerSupply_UpdateDAC();
        PowerSupply_SetState(POWER_STATE_RUNNING);
    } else {
        /* 计算当前软启动电压 */
        float progress = (float)elapsed / POWER_SOFTSTART_TIME_MS;
        float current_voltage = g_softstart_target_voltage * progress;
        
        /* 计算并设置DAC */
        float vfb_target = current_voltage * POWER_VFB_DIVIDER_RATIO;
        uint32_t cv_dac = (uint32_t)(vfb_target / POWER_DAC_VREF * POWER_DAC_RESOLUTION);
        if (cv_dac > POWER_DAC_RESOLUTION - 1) cv_dac = POWER_DAC_RESOLUTION - 1;
        
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_CV, DAC_ALIGN_12B_R, cv_dac);
    }
}
