/**
  ******************************************************************************
  * @file    multimeter.c
  * @brief   万用表模块实现
  ******************************************************************************
  * @description
  *   本模块实现万用表的电压、电流、电阻测量功能
  *   
  * @hardware
  *   电压测量:
  *     - ADC通道: PA1 (ADC1_IN1)
  *     - 分压比: 2.2kΩ/(47kΩ+2.2kΩ) = 1/22.36
  *     - 公式: V = (V_ADC - 1.65) × 22.36
  *     - 范围: ±36V
  *   
  *   电流测量:
  *     - ADC通道: PA3 (ADC1_IN3)
  *     - 采样电阻: 10mΩ, 增益: 50
  *     - 公式: I = (V_ADC - 1.65) × 2
  *     - 范围: ±2A
  *   
  *   电阻测量:
  *     - ADC通道: PA2 (ADC1_IN2)
  *     - 原理: 3.3V → Rref → 分压点(RES_VCC/红表笔) → Rx → GND(黑表笔)
  *     - 公式: Rx = Rref × V_ADC / (3.3 - V_ADC)
  *     - 档位: 100Ω, 1KΩ, 10KΩ, 100KΩ, 1MΩ (P-MOS切换, 低电平导通)
  *   
  *   继电器控制 (高电平导通):
  *     - PG11: 电阻通道 (RES_VCC连接红表笔)
  *     - PG12: 电压通道 (VOL_VCC连接红表笔)
  *     - PG13: 电流通道 (CUR_VCC连接红表笔)
  *   
  *   黑表笔接地控制:
  *     - PD4: 高电平导通MOS，黑表笔接地 (电阻测量时需要)
  *   
  *   电阻档位选择 (P-MOS AO3401, 低电平导通):
  *     - PG2: 100Ω
  *     - PG3: 1KΩ
  *     - PG4: 10KΩ
  *     - PG5: 100KΩ
  *     - PG6: 1MΩ
  ******************************************************************************
  */

#include "multimeter.h"
#include <math.h>

/*===========================================================================*/
/*                           私有变量                                         */
/*===========================================================================*/
static MeterMode_t g_current_mode = METER_MODE_IDLE;
static ResRange_t g_current_res_range = RES_RANGE_10K;
static uint8_t g_auto_range_enabled = 0;    /* 自动量程使能标志 */
static uint16_t g_adc_buffer[3] = {0};      /* ADC DMA缓冲区 */
static uint8_t g_adc_ready = 0;

/* 电阻档位参考值表 */
static const float g_res_ref_table[5] = {
    RES_REF_100R,
    RES_REF_1K,
    RES_REF_10K,
    RES_REF_100K,
    RES_REF_1M
};

/*===========================================================================*/
/*                           私有函数声明                                     */
/*===========================================================================*/
static void Multimeter_DisableAllChannels(void);
static void Multimeter_DisableAllResRanges(void);
static float Multimeter_ADCToVoltage(uint16_t adc_value);
static uint16_t Multimeter_GetFilteredADCInternal(uint8_t channel);
static void Multimeter_WaitADCStable(void);

/*===========================================================================*/
/*                           初始化函数                                       */
/*===========================================================================*/

/**
  * @brief  万用表模块初始化
  */
void Multimeter_Init(void)
{
    /* 关闭所有通道 */
    Multimeter_DisableAllChannels();
    
    /* 关闭所有电阻档位 (P-MOS高电平关闭) */
    Multimeter_DisableAllResRanges();
    
    /* 黑表笔默认不接地 */
    HAL_GPIO_WritePin(BLACK_CON_PORT, BLACK_CON_PIN, GPIO_PIN_RESET);
    
    /* 设置默认模式为空闲 */
    g_current_mode = METER_MODE_IDLE;
    g_current_res_range = RES_RANGE_10K;  /* 默认10K档位 */
    g_auto_range_enabled = 0;              /* 默认关闭自动量程 */
    g_adc_ready = 0;
}

/*===========================================================================*/
/*                           通道控制函数                                     */
/*===========================================================================*/

/**
  * @brief  关闭所有测量通道
  */
static void Multimeter_DisableAllChannels(void)
{
    HAL_GPIO_WritePin(RELAY_RES_PORT, RELAY_RES_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RELAY_VOL_PORT, RELAY_VOL_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RELAY_CUR_PORT, RELAY_CUR_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BLACK_CON_PORT, BLACK_CON_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  关闭所有电阻档位 (P-MOS, 高电平关闭)
  */
static void Multimeter_DisableAllResRanges(void)
{
    HAL_GPIO_WritePin(RES_100R_PORT, RES_100R_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RES_1K_PORT, RES_1K_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RES_10K_PORT, RES_10K_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RES_100K_PORT, RES_100K_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RES_1M_PORT, RES_1M_PIN, GPIO_PIN_SET);
}

/**
  * @brief  设置测量模式
  * @param  mode: 测量模式
  */
void Multimeter_SetMode(MeterMode_t mode)
{
    /* 先关闭所有通道 (互斥) */
    Multimeter_DisableAllChannels();
    
    /* 延时等待继电器断开 */
    HAL_Delay(RELAY_SWITCH_DELAY_MS);
    
    switch(mode) {
        case METER_MODE_VOLTAGE:
            HAL_GPIO_WritePin(RELAY_VOL_PORT, RELAY_VOL_PIN, GPIO_PIN_SET);
            break;
            
        case METER_MODE_CURRENT:
            HAL_GPIO_WritePin(RELAY_CUR_PORT, RELAY_CUR_PIN, GPIO_PIN_SET);
            break;
            
        case METER_MODE_RESISTANCE:
            HAL_GPIO_WritePin(RELAY_RES_PORT, RELAY_RES_PIN, GPIO_PIN_SET);
            /* 电阻测量需要黑表笔接地 */
            HAL_GPIO_WritePin(BLACK_CON_PORT, BLACK_CON_PIN, GPIO_PIN_SET);
            /* 设置档位 */
            Multimeter_SetResRange(g_current_res_range);
            break;
            
        case METER_MODE_IDLE:
        default:
            /* 保持所有通道关闭 */
            break;
    }
    
    g_current_mode = mode;
    
    /* 延时等待继电器稳定 */
    HAL_Delay(RELAY_STABLE_DELAY_MS);
    
    /* 等待ADC稳定 */
    Multimeter_WaitADCStable();
}

/**
  * @brief  获取当前测量模式
  */
MeterMode_t Multimeter_GetMode(void)
{
    return g_current_mode;
}

/*===========================================================================*/
/*                           电阻档位控制                                     */
/*===========================================================================*/

/**
  * @brief  设置电阻测量档位 (P-MOS AO3401, 低电平导通)
  * @param  range: 档位
  */
void Multimeter_SetResRange(ResRange_t range)
{
    /* 先关闭所有档位 (高电平关闭P-MOS) */
    Multimeter_DisableAllResRanges();
    
    /* 延时确保所有档位完全关闭 */
    HAL_Delay(10);
    
    /* 低电平导通对应档位的P-MOS */
    switch(range) {
        case RES_RANGE_100R:
            HAL_GPIO_WritePin(RES_100R_PORT, RES_100R_PIN, GPIO_PIN_RESET);
            g_current_res_range = RES_RANGE_100R;
            break;
        case RES_RANGE_1K:
            HAL_GPIO_WritePin(RES_1K_PORT, RES_1K_PIN, GPIO_PIN_RESET);
            g_current_res_range = RES_RANGE_1K;
            break;
        case RES_RANGE_10K:
            HAL_GPIO_WritePin(RES_10K_PORT, RES_10K_PIN, GPIO_PIN_RESET);
            g_current_res_range = RES_RANGE_10K;
            break;
        case RES_RANGE_100K:
            HAL_GPIO_WritePin(RES_100K_PORT, RES_100K_PIN, GPIO_PIN_RESET);
            g_current_res_range = RES_RANGE_100K;
            break;
        case RES_RANGE_1M:
            HAL_GPIO_WritePin(RES_1M_PORT, RES_1M_PIN, GPIO_PIN_RESET);
            g_current_res_range = RES_RANGE_1M;
            break;
        default:
            /* 默认使用10K档位 */
            HAL_GPIO_WritePin(RES_10K_PORT, RES_10K_PIN, GPIO_PIN_RESET);
            g_current_res_range = RES_RANGE_10K;
            break;
    }
    
    /* 延时等待稳定 */
    HAL_Delay(ADC_STABLE_DELAY_MS);
}

/**
  * @brief  获取当前电阻档位
  */
ResRange_t Multimeter_GetResRange(void)
{
    return g_current_res_range;
}


/*===========================================================================*/
/*                           ADC相关函数                                      */
/*===========================================================================*/

/**
  * @brief  ADC值转换为电压
  */
static float Multimeter_ADCToVoltage(uint16_t adc_value)
{
    return (float)adc_value * ADC_VREF / ADC_RESOLUTION;
}

/**
  * @brief  启动ADC DMA采集
  */
void Multimeter_StartADC(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adc_buffer, 3);
    g_adc_ready = 1;
}

/**
  * @brief  停止ADC采集
  */
void Multimeter_StopADC(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    g_adc_ready = 0;
}

/**
  * @brief  更新ADC缓冲区数据 (可在DMA回调中调用)
  */
void Multimeter_UpdateADC(uint16_t* adc_buffer)
{
    g_adc_buffer[0] = adc_buffer[0];
    g_adc_buffer[1] = adc_buffer[1];
    g_adc_buffer[2] = adc_buffer[2];
}

/**
  * @brief  获取原始ADC值
  */
uint16_t Multimeter_GetRawADC(uint8_t channel)
{
    if(channel < 3) {
        return g_adc_buffer[channel];
    }
    return 0;
}

/*===========================================================================*/
/*                           滤波函数                                         */
/*===========================================================================*/

/**
  * @brief  等待ADC稳定
  */
static void Multimeter_WaitADCStable(void)
{
    HAL_Delay(ADC_STABLE_DELAY_MS);
}

/**
  * @brief  获取滤波后的ADC值 (内部使用)
  * @param  channel: ADC通道
  * @retval 滤波后的ADC值
  */
static uint16_t Multimeter_GetFilteredADCInternal(uint8_t channel)
{
    uint32_t sum = 0;
    uint16_t i;
    
    if(channel >= 3) {
        return 0;
    }
    
    /* 采集FILTER_SAMPLE_COUNT次并求平均 */
    for(i = 0; i < FILTER_SAMPLE_COUNT; i++) {
        sum += g_adc_buffer[channel];
        HAL_Delay(1);  /* 每次采样间隔1ms */
    }
    
    return (uint16_t)(sum / FILTER_SAMPLE_COUNT);
}

/**
  * @brief  获取滤波后的ADC值 (外部接口)
  * @param  channel: ADC通道
  * @retval 滤波后的ADC值
  */
uint16_t Multimeter_GetFilteredADC(uint8_t channel)
{
    return Multimeter_GetFilteredADCInternal(channel);
}

/*===========================================================================*/
/*                           自动量程控制                                     */
/*===========================================================================*/

/**
  * @brief  启用/禁用自动量程
  * @param  enable: 1=启用, 0=禁用
  */
void Multimeter_EnableAutoRange(uint8_t enable)
{
    g_auto_range_enabled = enable ? 1 : 0;
}

/**
  * @brief  获取自动量程状态
  * @retval 1=已启用, 0=已禁用
  */
uint8_t Multimeter_IsAutoRangeEnabled(void)
{
    return g_auto_range_enabled;
}

/*===========================================================================*/
/*                           测量函数                                         */
/*===========================================================================*/

/**
  * @brief  测量电压 (带滤波)
  * @retval 电压值 (V), 正值表示红表笔为正极
  * @note   公式: V = (V_ADC - 1.65) × 22.36
  */
float Multimeter_MeasureVoltage(void)
{
    float v_adc;
    float voltage;
    uint16_t adc_filtered;
    
    /* 获取滤波后的ADC值 */
    adc_filtered = Multimeter_GetFilteredADCInternal(ADC_CHANNEL_VOLTAGE);
    
    /* 转换为电压值 */
    v_adc = Multimeter_ADCToVoltage(adc_filtered);
    
    /* 计算实际电压 */
    voltage = (v_adc - VOLTAGE_REF_OFFSET) * VOLTAGE_DIVIDER_RATIO;
    
    return voltage;
}

/**
  * @brief  测量电流 (带滤波)
  * @retval 电流值 (A), 正值表示电流从红表笔流入
  * @note   公式: I = (V_ADC - 1.65) × 2
  */
float Multimeter_MeasureCurrent(void)
{
    float v_adc;
    float current;
    uint16_t adc_filtered;
    
    /* 获取滤波后的ADC值 */
    adc_filtered = Multimeter_GetFilteredADCInternal(ADC_CHANNEL_CURRENT);
    
    /* 转换为电压值 */
    v_adc = Multimeter_ADCToVoltage(adc_filtered);
    
    /* 计算实际电流: I = (V_ADC - 1.65) × 2 */
    current = (v_adc - CURRENT_REF_OFFSET) * CURRENT_GAIN;
    
    return current;
}

/**
  * @brief  测量电阻 (带滤波)
  * @retval 电阻值 (Ω)
  * @note   公式: Rx = Rref × V_ADC / (3.3 - V_ADC)
  */
float Multimeter_MeasureResistance(void)
{
    float v_adc;
    float resistance;
    float r_ref;
    uint16_t adc_filtered;
    
    /* 获取当前档位的参考电阻值 */
    if(g_current_res_range < RES_RANGE_AUTO) {
        r_ref = g_res_ref_table[g_current_res_range];
    } else {
        r_ref = RES_REF_10K;  /* 默认10K */
    }
    
    /* 获取滤波后的ADC值 */
    adc_filtered = Multimeter_GetFilteredADCInternal(ADC_CHANNEL_RESISTANCE);
    
    /* 转换为电压值 */
    v_adc = Multimeter_ADCToVoltage(adc_filtered);
    
    /* 防止除零 - 开路检测 */
    if(v_adc >= (ADC_VREF - 0.05f)) {
        return INFINITY;  /* 开路 */
    }
    
    /* 短路检测 */
    if(v_adc <= 0.05f) {
        return 0.0f;  /* 短路 */
    }
    
    /* 计算电阻值: Rx = Rref × V_ADC / (Vcc - V_ADC) */
    resistance = r_ref * v_adc / (ADC_VREF - v_adc);
    
    return resistance;
}

/*===========================================================================*/
/*                           自动量程                                         */
/*===========================================================================*/

/**
  * @brief  电阻自动量程选择
  * @retval 最佳档位
  * @note   根据ADC电压选择合适的档位，使测量精度最高
  */
ResRange_t Multimeter_AutoRange(void)
{
    float v_adc;
    ResRange_t best_range = g_current_res_range;
    uint16_t adc_filtered;
    
    /* 获取滤波后的ADC电压 */
    adc_filtered = Multimeter_GetFilteredADCInternal(ADC_CHANNEL_RESISTANCE);
    v_adc = Multimeter_ADCToVoltage(adc_filtered);
    
    /* 
     * 最佳测量范围: ADC电压在0.3V~3.0V之间
     * 电压太低 -> 被测电阻远小于参考电阻，需要降档(使用更小的参考电阻)
     * 电压太高 -> 被测电阻远大于参考电阻，需要升档(使用更大的参考电阻)
     */
    
    if(v_adc < 0.3f) {
        /* 电压太低，需要降档 */
        if(g_current_res_range > RES_RANGE_100R) {
            best_range = (ResRange_t)(g_current_res_range - 1);
        }
    }
    else if(v_adc > 3.0f) {
        /* 电压太高，需要升档 */
        if(g_current_res_range < RES_RANGE_1M) {
            best_range = (ResRange_t)(g_current_res_range + 1);
        }
    }
    
    /* 如果需要切换档位 */
    if(best_range != g_current_res_range) {
        Multimeter_SetResRange(best_range);
    }
    
    return g_current_res_range;
}

/*===========================================================================*/
/*                           综合测量函数                                     */
/*===========================================================================*/

/**
  * @brief  根据当前模式进行测量 (带保护)
  * @retval 测量结果结构体
  */
MeterResult_t Multimeter_Measure(void)
{
    MeterResult_t result = {0};
    
    result.mode = g_current_mode;
    result.res_range = g_current_res_range;
    result.overrange = 0;
    result.open_circuit = 0;
    result.short_circuit = 0;
    result.polarity = 1;
    
    switch(g_current_mode) {
        case METER_MODE_VOLTAGE:
            result.voltage = Multimeter_MeasureVoltage();
            /* 判断极性 */
            if(result.voltage < 0) {
                result.polarity = -1;
            }
            /* 判断超量程 */
            if(fabsf(result.voltage) > VOLTAGE_MAX) {
                result.overrange = 1;
            }
            break;
            
        case METER_MODE_CURRENT:
            result.current = Multimeter_MeasureCurrent();
            /* 判断极性 */
            if(result.current < 0) {
                result.polarity = -1;
            }
            /* 判断超量程 */
            if(fabsf(result.current) > CURRENT_MAX) {
                result.overrange = 1;
            }
            break;
            
        case METER_MODE_RESISTANCE:
            /* 自动量程模式下先调整档位 */
            if(g_auto_range_enabled) {
                Multimeter_AutoRange();
            }
            result.resistance = Multimeter_MeasureResistance();
            result.res_range = g_current_res_range;
            
            /* 开路检测 */
            if(isinf(result.resistance)) {
                result.open_circuit = 1;
                result.overrange = 1;
            }
            /* 短路检测 */
            else if(result.resistance < 1.0f) {
                result.short_circuit = 1;
            }
            /* 超量程检测 */
            else if(result.resistance > RES_MAX) {
                result.overrange = 1;
            }
            break;
            
        case METER_MODE_IDLE:
        default:
            break;
    }
    
    return result;
}
