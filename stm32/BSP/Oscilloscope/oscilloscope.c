/**
  ******************************************************************************
  * @file    oscilloscope.c
  * @brief   示波器模块实现 - 支持MCP6S21 PGA
  ******************************************************************************
  * @description
  *   信号链路: 输入 → AC/DC耦合 → 1/50衰减 → OPA365 → MCP6S21 PGA → 反相运放 → ADC
  *   
  *   电压转换公式:
  *     V_adc = 1.65 - V_pga
  *     V_pga = G × (Vin / 50)  (VREF=GND时)
  *     Vin = (1.65 - V_adc) × 50 / G
  *   
  *   输入范围: ±50V
  ******************************************************************************
  */

#include "oscilloscope.h"
#include <math.h>
#include <string.h>

/*===========================================================================*/
/*                           私有变量                                         */
/*===========================================================================*/
static OscConfig_t g_osc_config = {
    .pgaGain = PGA_GAIN_1,
    .coupling = OSC_COUPLING_DC,
    .sampleRate = OSC_SAMPLERATE_1M,
    .state = OSC_STATE_IDLE,
    .tripleADC = 1,         /* 默认开启三ADC模式 */
    .autoRange = 1
};

/* 采样缓冲区 */
static uint32_t g_osc_buffer[OSC_BUFFER_SIZE] __attribute__((aligned(4)));

/* 测量结果缓存 */
static OscMeasurement_t g_osc_measurement = {0};

/* 采样率对应的定时器周期值 (168MHz主频, TIM2预分频=1) */
static const uint32_t g_tim_period_table[OSC_SAMPLERATE_MAX] = {
    168000 - 1,     /* 1 KSPS */
    16800 - 1,      /* 10 KSPS */
    1680 - 1,       /* 100 KSPS */
    336 - 1,        /* 500 KSPS */
    168 - 1,        /* 1 MSPS */
    84 - 1,         /* 2 MSPS */
    28 - 1          /* 5 MSPS (三ADC交替) */
};

/* PGA增益值查找表 */
static const float g_pga_gain_table[PGA_GAIN_MAX] = {
    1.0f, 2.0f, 4.0f, 5.0f, 8.0f, 10.0f, 16.0f, 32.0f
};

/* 各增益档位对应的输入量程 (±V) */
/* 量程 = 1.65 × 50 / G = 82.5 / G */
static const float g_voltage_range_table[PGA_GAIN_MAX] = {
    82.5f,      /* ×1:  ±82.5V (限制到±50V) */
    41.25f,     /* ×2:  ±41.25V */
    20.625f,    /* ×4:  ±20.625V */
    16.5f,      /* ×5:  ±16.5V */
    10.3125f,   /* ×8:  ±10.3125V */
    8.25f,      /* ×10: ±8.25V */
    5.15625f,   /* ×16: ±5.15625V */
    2.578125f   /* ×32: ±2.578125V */
};

/*===========================================================================*/
/*                           私有函数声明                                     */
/*===========================================================================*/
static void Oscilloscope_ConfigADC_SingleMode(void);
static void Oscilloscope_ConfigADC_TripleMode(void);
static void Oscilloscope_ConfigDMA_SingleMode(void);
static void Oscilloscope_ConfigDMA_TripleMode(void);
static void Oscilloscope_SetTimerPeriod(uint32_t period);
static void PGA_WriteRegister(uint8_t cmd, uint8_t data);

/*===========================================================================*/
/*                           MCP6S21 PGA控制函数                              */
/*===========================================================================*/

/**
  * @brief  PGA SPI写寄存器
  * @param  cmd: 命令字节
  * @param  data: 数据字节
  */
static void PGA_WriteRegister(uint8_t cmd, uint8_t data)
{
    uint8_t txData[2] = {cmd, data};
    
    /* CS拉低 */
    HAL_GPIO_WritePin(PGA_CS_PORT, PGA_CS_PIN, GPIO_PIN_RESET);
    
    /* 发送16位数据 */
    HAL_SPI_Transmit(&hspi1, txData, 2, 100);
    
    /* CS拉高，执行命令 */
    HAL_GPIO_WritePin(PGA_CS_PORT, PGA_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  PGA初始化
  */
void PGA_Init(void)
{
    /* CS默认高电平 */
    HAL_GPIO_WritePin(PGA_CS_PORT, PGA_CS_PIN, GPIO_PIN_SET);
    
    /* 延时等待PGA上电稳定 */
    HAL_Delay(1);
    
    /* 设置默认增益 ×1 */
    PGA_SetGain(PGA_GAIN_1);
}

/**
  * @brief  设置PGA增益
  * @param  gain: 增益档位 (PGA_GAIN_1 ~ PGA_GAIN_32)
  */
void PGA_SetGain(PGA_Gain_t gain)
{
    if (gain >= PGA_GAIN_MAX) {
        gain = PGA_GAIN_1;
    }
    
    /* 写增益寄存器: 命令0x40, 数据为增益代码 */
    PGA_WriteRegister(MCP6S21_CMD_WRITE_GAIN, (uint8_t)gain);
    
    g_osc_config.pgaGain = gain;
}

/**
  * @brief  获取当前PGA增益档位
  */
PGA_Gain_t PGA_GetGain(void)
{
    return g_osc_config.pgaGain;
}

/**
  * @brief  获取增益档位对应的实际增益值
  */
float PGA_GetGainValue(PGA_Gain_t gain)
{
    if (gain >= PGA_GAIN_MAX) {
        return 1.0f;
    }
    return g_pga_gain_table[gain];
}

/**
  * @brief  PGA进入关机模式
  */
void PGA_Shutdown(void)
{
    PGA_WriteRegister(MCP6S21_CMD_SHUTDOWN, 0x00);
}

/**
  * @brief  PGA唤醒 (通过写增益寄存器唤醒)
  */
void PGA_Wakeup(void)
{
    PGA_SetGain(g_osc_config.pgaGain);
}

/*===========================================================================*/
/*                           初始化函数                                       */
/*===========================================================================*/

/**
  * @brief  示波器模块初始化
  */
void Oscilloscope_Init(void)
{
    /* 停止万用表的ADC采样 */
    HAL_ADC_Stop_DMA(&hadc1);
    
    /* 反初始化ADC */
    HAL_ADC_DeInit(&hadc1);
    HAL_ADC_DeInit(&hadc2);
    HAL_ADC_DeInit(&hadc3);
    
    /* 清空缓冲区 */
    memset(g_osc_buffer, 0, sizeof(g_osc_buffer));
    
    /* 初始化PGA */
    PGA_Init();
    
    /* 设置默认配置 */
    g_osc_config.state = OSC_STATE_IDLE;
    g_osc_config.autoRange = 1;
    
    /* 设置默认耦合 */
    Oscilloscope_SetCoupling(OSC_COUPLING_DC);
    
    /* 始终使用三ADC交替模式，获得最佳采样性能 */
    g_osc_config.tripleADC = 1;
    Oscilloscope_ConfigADC_TripleMode();
    Oscilloscope_ConfigDMA_TripleMode();
}

/**
  * @brief  示波器模块反初始化
  */
void Oscilloscope_DeInit(void)
{
    Oscilloscope_Stop();
    PGA_Shutdown();
    
    HAL_ADC_DeInit(&hadc1);
    HAL_ADC_DeInit(&hadc2);
    HAL_ADC_DeInit(&hadc3);
    
    MX_ADC1_Init();
    
    g_osc_config.state = OSC_STATE_IDLE;
}

/*===========================================================================*/
/*                           ADC配置函数                                      */
/*===========================================================================*/

/**
  * @brief  配置ADC为单ADC模式
  */
static void Oscilloscope_ConfigADC_SingleMode(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }
    
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief  配置ADC为三ADC交替模式
  */
static void Oscilloscope_ConfigADC_TripleMode(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    ADC_MultiModeTypeDef multimode = {0};
    
    /* ADC1配置 (主ADC) */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    /* ADC2配置 (从ADC) */
    hadc2.Instance = ADC2;
    hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc2.Init.Resolution = ADC_RESOLUTION_12B;
    hadc2.Init.ScanConvMode = DISABLE;
    hadc2.Init.ContinuousConvMode = DISABLE;
    hadc2.Init.DiscontinuousConvMode = DISABLE;
    hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc2.Init.NbrOfConversion = 1;
    hadc2.Init.DMAContinuousRequests = DISABLE;
    hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hadc2) != HAL_OK) {
        Error_Handler();
    }
    
    /* ADC3配置 (从ADC) */
    hadc3.Instance = ADC3;
    hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3.Init.ScanConvMode = DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3.Init.NbrOfConversion = 1;
    hadc3.Init.DMAContinuousRequests = DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hadc3) != HAL_OK) {
        Error_Handler();
    }
    
    /* 配置通道0 (PA0) */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_ConfigChannel(&hadc2, &sConfig);
    HAL_ADC_ConfigChannel(&hadc3, &sConfig);
    
    /* 配置多ADC模式 */
    multimode.Mode = ADC_TRIPLEMODE_INTERL;
    multimode.DMAAccessMode = ADC_DMAACCESSMODE_2;
    multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_5CYCLES;
    
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
        Error_Handler();
    }
}

/*===========================================================================*/
/*                           DMA配置函数                                      */
/*===========================================================================*/

static void Oscilloscope_ConfigDMA_SingleMode(void)
{
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
        Error_Handler();
    }
}

static void Oscilloscope_ConfigDMA_TripleMode(void)
{
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
        Error_Handler();
    }
}

/*===========================================================================*/
/*                           采样控制函数                                     */
/*===========================================================================*/

static void Oscilloscope_SetTimerPeriod(uint32_t period)
{
    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

void Oscilloscope_Start(void)
{
    if (g_osc_config.state == OSC_STATE_RUNNING) {
        return;
    }
    
    Oscilloscope_SetTimerPeriod(g_tim_period_table[g_osc_config.sampleRate]);
    
    if (g_osc_config.tripleADC) {
        HAL_ADC_Start(&hadc3);
        HAL_ADC_Start(&hadc2);
        HAL_ADCEx_MultiModeStart_DMA(&hadc1, g_osc_buffer, OSC_BUFFER_SIZE);
    } else {
        HAL_ADC_Start_DMA(&hadc1, g_osc_buffer, OSC_BUFFER_SIZE);
    }
    
    HAL_TIM_Base_Start(&htim2);
    g_osc_config.state = OSC_STATE_RUNNING;
}

void Oscilloscope_Stop(void)
{
    HAL_TIM_Base_Stop(&htim2);
    
    if (g_osc_config.tripleADC) {
        HAL_ADCEx_MultiModeStop_DMA(&hadc1);
        HAL_ADC_Stop(&hadc2);
        HAL_ADC_Stop(&hadc3);
    } else {
        HAL_ADC_Stop_DMA(&hadc1);
    }
    
    g_osc_config.state = OSC_STATE_IDLE;
}

OscState_t Oscilloscope_GetState(void)
{
    return g_osc_config.state;
}

/*===========================================================================*/
/*                           耦合控制                                         */
/*===========================================================================*/

void Oscilloscope_SetCoupling(OscCoupling_t coupling)
{
    if (coupling == OSC_COUPLING_AC) {
        HAL_GPIO_WritePin(OSC_AC_PORT, OSC_AC_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(OSC_AC_PORT, OSC_AC_PIN, GPIO_PIN_RESET);
    }
    g_osc_config.coupling = coupling;
}

OscCoupling_t Oscilloscope_GetCoupling(void)
{
    return g_osc_config.coupling;
}

/*===========================================================================*/
/*                           采样率控制                                       */
/*===========================================================================*/

void Oscilloscope_SetSampleRate(OscSampleRate_t rate)
{
    if (rate >= OSC_SAMPLERATE_MAX) {
        rate = OSC_SAMPLERATE_1M;
    }
    
    uint8_t wasRunning = (g_osc_config.state == OSC_STATE_RUNNING);
    if (wasRunning) {
        Oscilloscope_Stop();
    }
    
    g_osc_config.sampleRate = rate;
    /* 始终使用三ADC模式，只改变定时器周期 */
    
    if (wasRunning) {
        Oscilloscope_Start();
    }
}

OscSampleRate_t Oscilloscope_GetSampleRate(void)
{
    return g_osc_config.sampleRate;
}

/*===========================================================================*/
/*                           自动量程控制                                     */
/*===========================================================================*/

void Oscilloscope_SetAutoRange(uint8_t enable)
{
    g_osc_config.autoRange = enable ? 1 : 0;
}

uint8_t Oscilloscope_GetAutoRange(void)
{
    return g_osc_config.autoRange;
}

/**
  * @brief  自动调整PGA增益
  * @note   根据当前采样数据的幅度自动选择最佳增益
  *         目标: 使信号占用ADC量程的50%-80%
  */
void Oscilloscope_AutoAdjustGain(void)
{
    if (!g_osc_config.autoRange) {
        return;
    }
    
    /* 找出当前采样数据的最大最小值 */
    uint16_t maxVal = 0;
    uint16_t minVal = 4095;
    
    for (uint32_t i = 0; i < OSC_BUFFER_SIZE; i++) {
        uint16_t val;
        if (g_osc_config.tripleADC) {
            val = (uint16_t)(g_osc_buffer[i] & 0x0FFF);
        } else {
            val = (uint16_t)(g_osc_buffer[i] & 0xFFFF);
            if (val > 4095) val = 4095;
        }
        
        if (val > maxVal) maxVal = val;
        if (val < minVal) minVal = val;
    }
    
    /* 计算峰峰值占ADC量程的比例 */
    uint16_t pp = maxVal - minVal;
    float ratio = (float)pp / 4095.0f;
    
    PGA_Gain_t currentGain = g_osc_config.pgaGain;
    PGA_Gain_t newGain = currentGain;
    
    /* 信号太小 (< 20% 量程), 增大增益 */
    if (ratio < 0.2f && currentGain < PGA_GAIN_32) {
        newGain = (PGA_Gain_t)(currentGain + 1);
    }
    /* 信号太大 (> 90% 量程), 减小增益 */
    else if (ratio > 0.9f && currentGain > PGA_GAIN_1) {
        newGain = (PGA_Gain_t)(currentGain - 1);
    }
    /* 信号接近饱和 (max > 4000 或 min < 95), 立即减小增益 */
    else if ((maxVal > 4000 || minVal < 95) && currentGain > PGA_GAIN_1) {
        newGain = (PGA_Gain_t)(currentGain - 1);
    }
    
    if (newGain != currentGain) {
        PGA_SetGain(newGain);
    }
}

/*===========================================================================*/
/*                           数据获取                                         */
/*===========================================================================*/

uint32_t* Oscilloscope_GetBuffer(void)
{
    return g_osc_buffer;
}

uint32_t Oscilloscope_GetBufferSize(void)
{
    return OSC_BUFFER_SIZE;
}

/*===========================================================================*/
/*                           电压转换                                         */
/*===========================================================================*/

/**
  * @brief  获取当前增益档位对应的电压量程（用于电压转换计算）
  * @retval 理论量程 (±V)，不做限制
  * @note   量程 = 1.65 × 50 / G = 82.5 / G
  */
float Oscilloscope_GetVoltageRange(void)
{
    /* 返回真实的理论量程值，用于ESP32端电压转换计算 */
    return g_voltage_range_table[g_osc_config.pgaGain];
}

/**
  * @brief  ADC值转换为实际输入电压
  * @param  adcValue: 12位ADC值 (0-4095)
  * @retval 实际输入电压 (V)
  * 
  * @note   电压转换公式:
  *         V_adc = adcValue × 3.3 / 4095
  *         V_pga = 1.65 - V_adc  (反相运放: Vout = 1.65 - Vin)
  *         V_in = V_pga × 50 / G  (1/50衰减, PGA增益G)
  *         
  *         合并: V_in = (1.65 - V_adc) × 50 / G
  */
float Oscilloscope_ADCToVoltage(uint16_t adcValue)
{
    /* ADC电压 (0-3.3V) */
    float v_adc = (float)adcValue * OSC_ADC_VREF / OSC_ADC_RESOLUTION;
    
    /* PGA输出电压 (反相运放后) */
    float v_pga = OSC_ADC_OFFSET - v_adc;
    
    /* 获取当前PGA增益 */
    float gain = g_pga_gain_table[g_osc_config.pgaGain];
    
    /* 计算实际输入电压 */
    float v_input = v_pga * OSC_INPUT_ATTEN / gain;
    
    return v_input;
}

/*===========================================================================*/
/*                           测量函数                                         */
/*===========================================================================*/

OscMeasurement_t Oscilloscope_Measure(void)
{
    OscMeasurement_t result = {0};
    
    if (g_osc_config.state != OSC_STATE_RUNNING && 
        g_osc_config.state != OSC_STATE_COMPLETE) {
        return result;
    }
    
    uint32_t sum = 0;
    uint64_t sumSquare = 0;
    uint16_t maxVal = 0;
    uint16_t minVal = 4095;
    uint32_t sampleCount = OSC_BUFFER_SIZE;
    
    for (uint32_t i = 0; i < sampleCount; i++) {
        uint16_t val;
        
        if (g_osc_config.tripleADC) {
            val = (uint16_t)(g_osc_buffer[i] & 0x0FFF);
        } else {
            val = (uint16_t)(g_osc_buffer[i] & 0xFFFF);
            if (val > 4095) val = 4095;
        }
        
        sum += val;
        sumSquare += (uint64_t)val * val;
        
        if (val > maxVal) maxVal = val;
        if (val < minVal) minVal = val;
    }
    
    float avg = (float)sum / sampleCount;
    float rms = sqrtf((float)sumSquare / sampleCount);
    
    result.vmax = Oscilloscope_ADCToVoltage(maxVal);
    result.vmin = Oscilloscope_ADCToVoltage(minVal);
    result.vpp = result.vmax - result.vmin;
    result.vavg = Oscilloscope_ADCToVoltage((uint16_t)avg);
    result.vrms = Oscilloscope_ADCToVoltage((uint16_t)rms);
    
    /* 频率测量 (简单过零检测) */
    uint32_t zeroCrossings = 0;
    uint16_t midVal = (maxVal + minVal) / 2;
    uint8_t lastAbove = (g_osc_buffer[0] & 0x0FFF) > midVal;
    
    for (uint32_t i = 1; i < sampleCount; i++) {
        uint16_t val;
        if (g_osc_config.tripleADC) {
            val = (uint16_t)(g_osc_buffer[i] & 0x0FFF);
        } else {
            val = (uint16_t)(g_osc_buffer[i] & 0xFFFF);
        }
        
        uint8_t above = val > midVal;
        if (above != lastAbove) {
            zeroCrossings++;
            lastAbove = above;
        }
    }
    
    if (zeroCrossings >= 2) {
        float sampleRateHz = (float)Oscilloscope_GetSampleRateHz();
        float periods = (float)zeroCrossings / 2.0f;
        result.freq = periods * sampleRateHz / (float)sampleCount;
        if (result.freq > 0) {
            result.period = 1000000.0f / result.freq;  /* 周期 (us) */
        }
    }
    
    return result;
}

/*===========================================================================*/
/*                           回调函数                                         */
/*===========================================================================*/

void Oscilloscope_DMACompleteCallback(void)
{
    g_osc_config.state = OSC_STATE_COMPLETE;
    
    /* 自动量程调整 */
    if (g_osc_config.autoRange) {
        Oscilloscope_AutoAdjustGain();
    }
}

void Oscilloscope_DMAHalfCompleteCallback(void)
{
    /* 可用于双缓冲处理 */
}

/*===========================================================================*/
/*                           通信模块接口函数                                 */
/*===========================================================================*/

OscConfig_t* Oscilloscope_GetConfig(void)
{
    return &g_osc_config;
}

OscMeasurement_t* Oscilloscope_GetMeasurement(void)
{
    g_osc_measurement = Oscilloscope_Measure();
    return &g_osc_measurement;
}

uint32_t Oscilloscope_GetSampleRateHz(void)
{
    static const uint32_t sample_rate_hz_table[OSC_SAMPLERATE_MAX] = {
        1000, 10000, 100000, 500000, 1000000, 2000000, 5000000
    };
    
    if (g_osc_config.sampleRate < OSC_SAMPLERATE_MAX) {
        return sample_rate_hz_table[g_osc_config.sampleRate];
    }
    return 1000000;
}

void Oscilloscope_SingleCapture(void)
{
    if (g_osc_config.state == OSC_STATE_RUNNING) {
        Oscilloscope_Stop();
    }
    
    memset(g_osc_buffer, 0, sizeof(g_osc_buffer));
    Oscilloscope_Start();
    
    uint32_t timeout = HAL_GetTick() + 1000;
    while (g_osc_config.state == OSC_STATE_RUNNING) {
        if (HAL_GetTick() > timeout) {
            break;
        }
    }
    
    Oscilloscope_Stop();
    g_osc_config.state = OSC_STATE_COMPLETE;
}
