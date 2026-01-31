/**
  ******************************************************************************
  * @file    uart_comm.c
  * @brief   UART通信模块实现
  ******************************************************************************
  * @description
  *   本文件实现基于DMA的UART通信功能
  *   
  * @implementation
  *   发送机制:
  *     - 使用DMA非阻塞发送
  *     - 发送前将数据打包成协议帧
  *     - 发送完成后通过回调通知
  *   
  *   接收机制:
  *     - 使用DMA + 空闲中断接收
  *     - 空闲中断触发时处理接收到的数据
  *     - 自动解析协议帧并调用回调函数
  ******************************************************************************
  */

#include "uart_comm.h"
#include <string.h>

/*===========================================================================*/
/*                           私有变量                                         */
/*===========================================================================*/

/* 发送相关 */
static uint8_t g_tx_buffer[UART_TX_BUFFER_SIZE];    /* 发送缓冲区 */
static volatile UartTxState_t g_tx_state = UART_TX_IDLE;  /* 发送状态 */

/* 接收相关 */
static uint8_t g_rx_buffer[UART_RX_BUFFER_SIZE];    /* DMA接收缓冲区 */
static uint8_t g_rx_frame_buffer[UART_RX_FRAME_SIZE]; /* 帧解析缓冲区 */
static volatile uint16_t g_rx_frame_len = 0;        /* 帧缓冲区中的数据长度 */
static volatile bool g_rx_data_ready = false;       /* 接收数据就绪标志 */

/* 回调函数 */
static UartCommRxCallback_t g_rx_callback = NULL;   /* 接收回调 */
static UartCommTxCallback_t g_tx_callback = NULL;   /* 发送完成回调 */

/* 外部UART句柄引用 (由CubeMX生成) */
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

/*===========================================================================*/
/*                           私有函数声明                                     */
/*===========================================================================*/

static void UartComm_StartReceive(void);
static void UartComm_ProcessRxData(const uint8_t *data, uint16_t len);

/*===========================================================================*/
/*                           初始化函数                                       */
/*===========================================================================*/

/**
 * @brief  UART通信模块初始化
 * @note   启动DMA接收，使能空闲中断
 */
void UartComm_Init(void)
{
    /* 清空缓冲区 */
    memset(g_tx_buffer, 0, sizeof(g_tx_buffer));
    memset(g_rx_buffer, 0, sizeof(g_rx_buffer));
    memset(g_rx_frame_buffer, 0, sizeof(g_rx_frame_buffer));
    
    /* 初始化状态 */
    g_tx_state = UART_TX_IDLE;
    g_rx_frame_len = 0;
    g_rx_data_ready = false;
    
    /* 使能UART空闲中断 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    
    /* 启动DMA接收 */
    UartComm_StartReceive();
}

/**
 * @brief  UART通信模块反初始化
 */
void UartComm_DeInit(void)
{
    /* 停止DMA传输 */
    HAL_UART_DMAStop(&huart1);
    
    /* 禁用空闲中断 */
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);
    
    /* 清除回调 */
    g_rx_callback = NULL;
    g_tx_callback = NULL;
    
    /* 重置状态 */
    g_tx_state = UART_TX_IDLE;
}

/**
 * @brief  启动DMA接收
 */
static void UartComm_StartReceive(void)
{
    /* 使用DMA接收数据到缓冲区 */
    HAL_UART_Receive_DMA(&huart1, g_rx_buffer, UART_RX_BUFFER_SIZE);
}

/*===========================================================================*/
/*                           回调注册函数                                     */
/*===========================================================================*/

/**
 * @brief  注册数据帧接收回调函数
 */
void UartComm_RegisterRxCallback(UartCommRxCallback_t callback)
{
    g_rx_callback = callback;
}

/**
 * @brief  注册发送完成回调函数
 */
void UartComm_RegisterTxCallback(UartCommTxCallback_t callback)
{
    g_tx_callback = callback;
}

/*===========================================================================*/
/*                           数据发送函数                                     */
/*===========================================================================*/

/**
 * @brief  发送数据帧 (非阻塞)
 * @param  func_code: 功能码
 * @param  data: 数据指针
 * @param  data_len: 数据长度
 * @retval 0=成功启动发送, -1=发送忙, -2=数据过长
 */
int UartComm_SendFrame(CommFuncCode_t func_code, const void *data, uint16_t data_len)
{
    /* 检查发送状态 */
    if (g_tx_state == UART_TX_BUSY) {
        return -1;  /* 发送忙 */
    }
    
    /* 检查数据长度 */
    if (data_len > COMM_MAX_DATA_LEN) {
        return -2;  /* 数据过长 */
    }
    
    /* 构建数据帧 */
    uint16_t frame_len = Comm_BuildFrame(g_tx_buffer, func_code, data, data_len);
    
    /* 设置发送状态 */
    g_tx_state = UART_TX_BUSY;
    
    /* 启动DMA发送 */
    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart1, g_tx_buffer, frame_len);
    
    if (status != HAL_OK) {
        g_tx_state = UART_TX_ERROR;
        return -3;  /* DMA启动失败 */
    }
    
    return 0;
}

/**
 * @brief  发送原始数据 (非阻塞)
 */
int UartComm_SendRaw(const uint8_t *data, uint16_t len)
{
    /* 检查发送状态 */
    if (g_tx_state == UART_TX_BUSY) {
        return -1;
    }
    
    /* 检查数据长度 */
    if (len > UART_TX_BUFFER_SIZE) {
        return -2;
    }
    
    /* 复制数据到发送缓冲区 */
    memcpy(g_tx_buffer, data, len);
    
    /* 设置发送状态 */
    g_tx_state = UART_TX_BUSY;
    
    /* 启动DMA发送 */
    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart1, g_tx_buffer, len);
    
    if (status != HAL_OK) {
        g_tx_state = UART_TX_ERROR;
        return -3;
    }
    
    return 0;
}

/**
 * @brief  获取发送状态
 */
UartTxState_t UartComm_GetTxState(void)
{
    return g_tx_state;
}

/**
 * @brief  等待发送完成 (阻塞)
 */
int UartComm_WaitTxComplete(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    
    while (g_tx_state == UART_TX_BUSY) {
        if ((HAL_GetTick() - start_tick) >= timeout_ms) {
            return -1;  /* 超时 */
        }
    }
    
    return 0;
}

/*===========================================================================*/
/*                           便捷发送函数                                     */
/*===========================================================================*/

/**
 * @brief  发送示波器波形数据
 */
int UartComm_SendOscWaveform(uint8_t gain, uint8_t coupling, uint8_t state,
                             uint8_t auto_range, uint32_t sample_rate, 
                             float voltage_range, const int16_t *adc_data, 
                             uint16_t data_count)
{
    /* 检查发送状态 */
    if (g_tx_state == UART_TX_BUSY) {
        return -1;
    }
    
    /* 计算数据帧大小 */
    uint16_t header_size = sizeof(OscWaveformFrame_t);
    uint16_t data_size = data_count * sizeof(int16_t);
    uint16_t total_data_len = header_size + data_size;
    
    /* 检查数据长度 */
    if (total_data_len > COMM_MAX_DATA_LEN) {
        return -2;
    }
    
    /* 在发送缓冲区中构建数据 */
    /* 跳过帧头部分，直接在数据区构建 */
    uint8_t *data_ptr = &g_tx_buffer[COMM_HEADER_SIZE];
    
    /* 填充波形帧头部 */
    OscWaveformFrame_t *frame = (OscWaveformFrame_t *)data_ptr;
    frame->gain = gain;
    frame->coupling = coupling;
    frame->state = state;
    frame->auto_range = auto_range;
    frame->sample_rate = sample_rate;
    frame->voltage_range = voltage_range;
    frame->data_count = data_count;
    
    /* 复制ADC数据 */
    memcpy(frame->data, adc_data, data_size);
    
    /* 构建完整帧 (帧头 + 功能码 + 长度 + 校验和) */
    g_tx_buffer[0] = COMM_FRAME_HEADER;
    g_tx_buffer[1] = FUNC_OSC_WAVEFORM;
    g_tx_buffer[2] = (uint8_t)(total_data_len & 0xFF);
    g_tx_buffer[3] = (uint8_t)((total_data_len >> 8) & 0xFF);
    
    /* 计算校验和 */
    uint8_t checksum = Comm_CalcChecksum(&g_tx_buffer[1], 3 + total_data_len);
    g_tx_buffer[COMM_HEADER_SIZE + total_data_len] = checksum;
    
    /* 计算总帧长度 */
    uint16_t frame_len = COMM_HEADER_SIZE + total_data_len + COMM_CHECKSUM_SIZE;
    
    /* 设置发送状态并启动DMA */
    g_tx_state = UART_TX_BUSY;
    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart1, g_tx_buffer, frame_len);
    
    if (status != HAL_OK) {
        g_tx_state = UART_TX_ERROR;
        return -3;
    }
    
    return 0;
}

/**
 * @brief  发送示波器测量结果
 */
int UartComm_SendOscMeasurement(const OscMeasurementFrame_t *measurement)
{
    return UartComm_SendFrame(FUNC_OSC_MEASUREMENT, measurement, 
                              sizeof(OscMeasurementFrame_t));
}

/**
 * @brief  发送万用表测量数据
 */
int UartComm_SendMeterData(uint8_t mode, uint8_t range, uint8_t flags, float value)
{
    MeterDataFrame_t frame;
    
    frame.mode = mode;
    frame.range = range;
    frame.flags = flags;
    frame.reserved = 0;
    frame.value = value;
    frame.secondary = 0.0f;
    
    return UartComm_SendFrame(FUNC_METER_DATA, &frame, sizeof(MeterDataFrame_t));
}

/**
 * @brief  发送信号发生器状态
 */
int UartComm_SendSignalGenStatus(uint8_t wave_type, uint8_t enabled,
                                  float frequency, float amplitude)
{
    SignalGenFrame_t frame;
    
    frame.wave_type = wave_type;
    frame.enabled = enabled;
    frame.reserved[0] = 0;
    frame.reserved[1] = 0;
    frame.frequency = frequency;
    frame.amplitude = amplitude;
    
    return UartComm_SendFrame(FUNC_SIGGEN_STATUS, &frame, sizeof(SignalGenFrame_t));
}

/**
 * @brief  发送心跳包
 */
int UartComm_SendHeartbeat(uint8_t device_status, uint8_t current_mode)
{
    HeartbeatFrame_t frame;
    
    frame.timestamp = HAL_GetTick();
    frame.device_status = device_status;
    frame.current_mode = current_mode;
    frame.reserved[0] = 0;
    frame.reserved[1] = 0;
    
    return UartComm_SendFrame(FUNC_HEARTBEAT, &frame, sizeof(HeartbeatFrame_t));
}

/*===========================================================================*/
/*                           中断处理函数                                     */
/*===========================================================================*/

/**
 * @brief  UART空闲中断处理
 * @note   当UART接收到一帧数据后产生空闲中断
 *         此时读取DMA计数器获取接收到的数据长度
 */
void UartComm_IDLE_IRQHandler(void)
{
    /* 检查是否是空闲中断 */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE)) {
        /* 清除空闲中断标志 */
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        
        /* 停止DMA接收 */
        HAL_UART_DMAStop(&huart1);
        
        /* 计算接收到的数据长度 */
        /* DMA计数器是递减的，剩余计数 = 总大小 - 已接收 */
        uint16_t rx_len = UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
        
        if (rx_len > 0) {
            /* 处理接收到的数据 */
            UartComm_ProcessRxData(g_rx_buffer, rx_len);
        }
        
        /* 重新启动DMA接收 */
        UartComm_StartReceive();
    }
}

/**
 * @brief  DMA发送完成回调
 */
void UartComm_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        /* 更新发送状态 */
        g_tx_state = UART_TX_IDLE;
        
        /* 调用用户回调 */
        if (g_tx_callback != NULL) {
            g_tx_callback();
        }
    }
}

/**
 * @brief  DMA接收完成回调 (缓冲区满)
 */
void UartComm_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        /* 缓冲区满，处理所有数据 */
        UartComm_ProcessRxData(g_rx_buffer, UART_RX_BUFFER_SIZE);
        
        /* 重新启动接收 */
        UartComm_StartReceive();
    }
}

/**
 * @brief  UART错误回调
 */
void UartComm_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        /* 清除错误标志 */
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        
        /* 重新启动接收 */
        UartComm_StartReceive();
    }
}

/*===========================================================================*/
/*                           数据处理函数                                     */
/*===========================================================================*/

/**
 * @brief  处理接收到的数据
 * @param  data: 数据指针
 * @param  len: 数据长度
 * @note   将数据追加到帧缓冲区，尝试解析完整帧
 */
static void UartComm_ProcessRxData(const uint8_t *data, uint16_t len)
{
    /* 将数据追加到帧缓冲区 */
    uint16_t copy_len = len;
    
    /* 检查缓冲区剩余空间 */
    if (g_rx_frame_len + len > UART_RX_FRAME_SIZE) {
        /* 缓冲区溢出，丢弃旧数据 */
        g_rx_frame_len = 0;
        copy_len = (len > UART_RX_FRAME_SIZE) ? UART_RX_FRAME_SIZE : len;
    }
    
    /* 复制数据到帧缓冲区 */
    memcpy(&g_rx_frame_buffer[g_rx_frame_len], data, copy_len);
    g_rx_frame_len += copy_len;
    
    /* 设置数据就绪标志，在主循环中处理 */
    g_rx_data_ready = true;
}

/**
 * @brief  通信模块周期处理
 * @note   在主循环中调用，用于解析接收到的数据帧
 */
void UartComm_Process(void)
{
    /* 检查是否有数据需要处理 */
    if (!g_rx_data_ready || g_rx_frame_len == 0) {
        return;
    }
    
    /* 清除就绪标志 */
    g_rx_data_ready = false;
    
    /* 查找帧头 */
    uint16_t start_idx = 0;
    while (start_idx < g_rx_frame_len) {
        /* 查找帧头 0xAA */
        if (g_rx_frame_buffer[start_idx] != COMM_FRAME_HEADER) {
            start_idx++;
            continue;
        }
        
        /* 检查剩余数据是否足够解析帧头 */
        uint16_t remaining = g_rx_frame_len - start_idx;
        if (remaining < COMM_HEADER_SIZE + COMM_CHECKSUM_SIZE) {
            break;  /* 数据不足，等待更多数据 */
        }
        
        /* 提取数据长度 */
        uint16_t data_len = g_rx_frame_buffer[start_idx + 2] | 
                           ((uint16_t)g_rx_frame_buffer[start_idx + 3] << 8);
        
        /* 检查数据长度是否合理 */
        if (data_len > COMM_MAX_DATA_LEN) {
            start_idx++;  /* 无效帧，跳过 */
            continue;
        }
        
        /* 计算完整帧长度 */
        uint16_t frame_len = COMM_HEADER_SIZE + data_len + COMM_CHECKSUM_SIZE;
        
        /* 检查是否有完整帧 */
        if (remaining < frame_len) {
            break;  /* 数据不足，等待更多数据 */
        }
        
        /* 尝试解析帧 */
        CommFuncCode_t func_code;
        uint8_t *frame_data;
        uint16_t frame_data_len;
        
        int result = Comm_ParseFrame(&g_rx_frame_buffer[start_idx], frame_len,
                                     &func_code, &frame_data, &frame_data_len);
        
        if (result == 0) {
            /* 解析成功，调用回调函数 */
            if (g_rx_callback != NULL) {
                g_rx_callback(func_code, frame_data, frame_data_len);
            }
            
            /* 移动到下一帧 */
            start_idx += frame_len;
        } else {
            /* 解析失败，跳过当前字节 */
            start_idx++;
        }
    }
    
    /* 移除已处理的数据 */
    if (start_idx > 0) {
        if (start_idx < g_rx_frame_len) {
            /* 将未处理的数据移到缓冲区开头 */
            memmove(g_rx_frame_buffer, &g_rx_frame_buffer[start_idx], 
                    g_rx_frame_len - start_idx);
            g_rx_frame_len -= start_idx;
        } else {
            /* 所有数据已处理 */
            g_rx_frame_len = 0;
        }
    }
}
