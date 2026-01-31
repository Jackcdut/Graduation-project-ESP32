/**
  ******************************************************************************
  * @file    signal_generator.c
  * @brief   信号发生器模块 (AD9833 DDS + MCP41010 数字电位器)
  ******************************************************************************
  * @description
  *   本模块实现基于AD9833 DDS芯片和MCP41010数字电位器的信号发生器功能
  *   - AD9833: 产生正弦波、三角波、方波，频率范围0.1Hz~12.5MHz
  *   - MCP41010: 控制输出幅值，实现幅度可调
  *   
  * @hardware
  *   - AD9833 CS: PE15
  *   - MCP41010 CS: PE14
  *   - SPI2: PB10(SCK), PC2(MISO), PC3(MOSI)
  ******************************************************************************
  */

#include "signal_generator.h"

/*===========================================================================*/
/*                           AD9833 寄存器定义                                */
/*===========================================================================*/
/* 
 * AD9833 控制寄存器位定义 (参考数据手册)
 * D15,D14: 00=控制寄存器, 01=FREQ0, 10=FREQ1, 11=PHASE
 * D13(B28): 1=连续写入28位频率, 0=分别写高/低14位
 * D12(HLB): B28=0时, 0=写LSB, 1=写MSB
 * D11(FSELECT): 0=使用FREQ0, 1=使用FREQ1
 * D10(PSELECT): 0=使用PHASE0, 1=使用PHASE1
 * D8(RESET): 1=复位, 0=正常运行
 * D7(SLEEP1): 1=禁用内部时钟
 * D6(SLEEP12): 1=禁用DAC
 * D5(OPBITEN): 1=方波输出使能
 * D3(DIV2): OPBITEN=1时, 1=MSB/2输出
 * D1(MODE): OPBITEN=0时, 0=正弦波, 1=三角波
 */
#define AD9833_REG_FREQ0    0x4000  /* FREQ0寄存器前缀 */
#define AD9833_REG_FREQ1    0x8000  /* FREQ1寄存器前缀 */
#define AD9833_REG_PHASE0   0xC000  /* PHASE0寄存器前缀 */
#define AD9833_REG_PHASE1   0xE000  /* PHASE1寄存器前缀 */

#define AD9833_B28          0x2000  /* D13: 连续28位频率数据写入 */
#define AD9833_HLB          0x1000  /* D12: 写MSB(B28=0时使用) */
#define AD9833_FSELECT      0x0800  /* D11: 选择FREQ1 */
#define AD9833_PSELECT      0x0400  /* D10: 选择PHASE1 */
#define AD9833_RESET        0x0100  /* D8: 复位 */
#define AD9833_SLEEP1       0x0080  /* D7: 禁用内部时钟 */
#define AD9833_SLEEP12      0x0040  /* D6: 禁用DAC */
#define AD9833_OPBITEN      0x0020  /* D5: 方波输出使能 */
#define AD9833_DIV2         0x0008  /* D3: MSB/2 */
#define AD9833_MODE         0x0002  /* D1: 三角波模式 */

/*===========================================================================*/
/*                           模块私有变量                                     */
/*===========================================================================*/
static double g_current_freq = 1000.0;      /* 当前频率 */
static WaveType_t g_current_wave = WAVE_SINE; /* 当前波形 */
static float g_current_amplitude = 1.0f;    /* 当前幅值 */

/*===========================================================================*/
/*                           AD9833 底层函数                                  */
/*===========================================================================*/

/**
  * @brief  向AD9833发送16位数据
  * @param  data: 要发送的数据
  */
void AD9833_Write(uint16_t data)
{
    uint8_t txData[2];
    txData[0] = (uint8_t)((data >> 8) & 0xFF);  /* 高8位, MSB先发 */
    txData[1] = (uint8_t)(data & 0xFF);         /* 低8位 */

    AD9833_CS_LOW();
    HAL_SPI_Transmit(&hspi2, txData, 2, 100);
    AD9833_CS_HIGH();
}

/**
  * @brief  复位AD9833
  */
void AD9833_Reset(void)
{
    AD9833_Write(AD9833_B28 | AD9833_RESET);  /* 0x2100 */
}

/**
  * @brief  设置AD9833的频率和波形类型
  * @param  freq: 频率值(Hz), 范围: 0.1Hz ~ 12.5MHz
  * @param  wave_type: 波形类型 (0:正弦波, 1:三角波, 2:方波, 3:方波/2)
  */
void AD9833_SetFrequencyWaveform(double freq, uint8_t wave_type)
{
    uint16_t freq_lsb, freq_msb;
    uint32_t freq_reg_value;
    uint16_t control_word;
    double actual_freq = freq;

    /* 频率寄存器计算公式: FREQREG = (Freq * 2^28) / Fref */
    const double FREQ_REG_RESOLUTION = 268435456.0;  /* 2^28 */

    /* 方波模式补偿: 方波输出频率是设定频率的一半 */
    if(wave_type == 2) {
        actual_freq = freq * 2.0;
    } else if(wave_type == 3) {
        actual_freq = freq * 4.0;
    }

    double freq_reg_calc = (actual_freq * FREQ_REG_RESOLUTION) / AD9833_REF_FREQ;

    /* 四舍五入并限制在28位范围内 */
    freq_reg_value = (uint32_t)(freq_reg_calc + 0.5);
    if(freq_reg_value > 0x0FFFFFFF) {
        freq_reg_value = 0x0FFFFFFF;
    }

    /* 将28位频率寄存器值分为两个14位数据 */
    freq_lsb = (uint16_t)(freq_reg_value & 0x3FFF);
    freq_msb = (uint16_t)((freq_reg_value >> 14) & 0x3FFF);

    /* 步骤1: 发送控制字, B28=1, RESET=1 */
    AD9833_Write(AD9833_B28 | AD9833_RESET);

    /* 步骤2: 写入FREQ0寄存器低14位 */
    AD9833_Write(AD9833_REG_FREQ0 | freq_lsb);

    /* 步骤3: 写入FREQ0寄存器高14位 */
    AD9833_Write(AD9833_REG_FREQ0 | freq_msb);

    /* 步骤4: 发送控制字, 设置波形类型 */
    control_word = AD9833_B28;

    switch(wave_type) {
        case 0:  /* 正弦波 */
            break;
        case 1:  /* 三角波 */
            control_word |= AD9833_MODE;
            break;
        case 2:  /* 方波 */
            control_word |= AD9833_OPBITEN | AD9833_SLEEP12;
            break;
        case 3:  /* 方波/2 */
            control_word |= AD9833_OPBITEN | AD9833_DIV2 | AD9833_SLEEP12;
            break;
        default:
            break;
    }

    AD9833_Write(control_word);
}

/**
  * @brief  AD9833初始化
  */
void AD9833_Init(void)
{
    AD9833_CS_HIGH();
    AD9833_Write(AD9833_B28 | AD9833_RESET);
    AD9833_SetFrequencyWaveform(1000.0, 0);  /* 默认1kHz正弦波 */
}


/*===========================================================================*/
/*                          MCP41010 底层函数                                 */
/*===========================================================================*/

/**
  * @brief  向MCP41010写入数据
  * @param  value: 电位器值 (0~255)
  */
void MCP41010_Write(uint8_t value)
{
    uint8_t data[2];
    data[0] = MCP41010_CMD_WRITE;
    data[1] = value;

    /* 切换到SPI Mode 0 (CPOL=0) for MCP41010 */
    hspi2.Instance->CR1 &= ~SPI_CR1_SPE;
    hspi2.Instance->CR1 &= ~SPI_CR1_CPOL;
    hspi2.Instance->CR1 |= SPI_CR1_SPE;

    MCP41010_CS_LOW();
    HAL_SPI_Transmit(&hspi2, data, 2, 100);
    MCP41010_CS_HIGH();

    /* 切换回SPI Mode 2 (CPOL=1) for AD9833 */
    hspi2.Instance->CR1 &= ~SPI_CR1_SPE;
    hspi2.Instance->CR1 |= SPI_CR1_CPOL;
    hspi2.Instance->CR1 |= SPI_CR1_SPE;
}

/**
  * @brief  MCP41010初始化
  */
void MCP41010_Init(void)
{
    MCP41010_CS_HIGH();
}

/**
  * @brief  设置输出电压值
  * @param  voltage: 目标输出电压 (V)
  */
void MCP41010_SetVoltage(float voltage)
{
    uint8_t value;
    float temp;

    const float AD9833_VPP = 0.60f;
    const float AMP_GAIN = 6.1f;

    temp = (voltage * 256.0f) / (AD9833_VPP * AMP_GAIN);

    if(temp > 255.0f) temp = 255.0f;
    if(temp < 0.0f) temp = 0.0f;

    value = (uint8_t)(temp + 0.5f);
    MCP41010_Write(value);
}

/*===========================================================================*/
/*                          频率补偿函数                                      */
/*===========================================================================*/

/**
  * @brief  正弦波频率补偿系数
  */
static float GetSineCompensation(double freq)
{
    if(freq <= 100000.0) {
        return 1.00f;
    }
    else if(freq <= 500000.0) {
        return 0.99f;
    }
    else if(freq <= 1000000.0) {
        float t = (float)(freq - 500000.0) / 500000.0f;
        return 1.01f + t * 0.06f;
    }
    else if(freq <= 1500000.0) {
        float t = (float)(freq - 1000000.0) / 500000.0f;
        return 1.07f + t * 0.08f;
    }
    else if(freq <= 2000000.0) {
        float t = (float)(freq - 1500000.0) / 500000.0f;
        return 1.15f + t * 0.20f;
    }
    else {
        float t = (float)(freq - 2000000.0) / 500000.0f;
        return 1.35f + t * 0.40f;
    }
}

/**
  * @brief  三角波频率补偿系数
  */
static float GetTriangleCompensation(double freq)
{
    if(freq <= 100000.0) {
        return 1.02f;
    }
    else if(freq <= 200000.0) {
        float t = (float)(freq - 100000.0) / 100000.0f;
        return 1.02f + t * 0.02f;
    }
    else if(freq <= 300000.0) {
        float t = (float)(freq - 200000.0) / 100000.0f;
        return 1.04f + t * 0.02f;
    }
    else if(freq <= 500000.0) {
        float t = (float)(freq - 300000.0) / 200000.0f;
        return 1.06f + t * 0.04f;
    }
    else if(freq <= 1000000.0) {
        float t = (float)(freq - 500000.0) / 500000.0f;
        return 1.10f + t * 0.14f;
    }
    else if(freq <= 1500000.0) {
        float t = (float)(freq - 1000000.0) / 500000.0f;
        return 1.24f + t * 0.16f;
    }
    else if(freq <= 2000000.0) {
        float t = (float)(freq - 1500000.0) / 500000.0f;
        return 1.40f + t * 0.14f;
    }
    else {
        float t = (float)(freq - 2000000.0) / 500000.0f;
        return 1.54f + t * 0.28f;
    }
}

/*===========================================================================*/
/*                          信号发生器综合控制                                 */
/*===========================================================================*/

/**
  * @brief  信号发生器初始化
  */
void SignalGenerator_Init(void)
{
    AD9833_Init();
    MCP41010_Init();
}

/**
  * @brief  设置信号发生器输出
  * @param  freq: 频率 (Hz)
  * @param  wave_type: 波形类型
  * @param  amplitude: 峰峰值电压 (V)
  */
void SignalGenerator_SetOutput(double freq, WaveType_t wave_type, float amplitude)
{
    float compensated_voltage;

    /* 保存当前设置 */
    g_current_freq = freq;
    g_current_wave = wave_type;
    g_current_amplitude = amplitude;

    /* 设置频率和波形 */
    AD9833_SetFrequencyWaveform(freq, (uint8_t)wave_type);

    /* 根据波形类型选择补偿系数 */
    switch(wave_type) {
        case WAVE_SINE:
            compensated_voltage = amplitude * GetSineCompensation(freq);
            break;
        case WAVE_TRIANGLE:
            compensated_voltage = amplitude * GetTriangleCompensation(freq);
            break;
        case WAVE_SQUARE:
        case WAVE_SQUARE_DIV2:
        default:
            compensated_voltage = amplitude;
            break;
    }

    MCP41010_SetVoltage(compensated_voltage);
}

/**
  * @brief  单独设置频率
  */
void SignalGenerator_SetFrequency(double freq)
{
    SignalGenerator_SetOutput(freq, g_current_wave, g_current_amplitude);
}

/**
  * @brief  单独设置波形
  */
void SignalGenerator_SetWaveform(WaveType_t wave_type)
{
    SignalGenerator_SetOutput(g_current_freq, wave_type, g_current_amplitude);
}

/**
  * @brief  单独设置幅值
  */
void SignalGenerator_SetAmplitude(float amplitude)
{
    SignalGenerator_SetOutput(g_current_freq, g_current_wave, amplitude);
}
