/**
  ******************************************************************************
  * @file    comm_protocol.c
  * @brief   STM32与ESP32通信协议实现
  ******************************************************************************
  * @description
  *   本文件实现通信协议的打包和解包功能
  ******************************************************************************
  */

#include "comm_protocol.h"
#include <string.h>

/*===========================================================================*/
/*                           协议处理函数实现                                 */
/*===========================================================================*/

/**
 * @brief  计算校验和
 * @param  data: 数据指针 (从功能码开始)
 * @param  len: 数据长度
 * @retval 校验和值 (所有字节异或)
 * @note   校验和计算范围: 功能码 + 长度字段 + 数据段
 */
uint8_t Comm_CalcChecksum(const uint8_t *data, uint16_t len)
{
    uint8_t checksum = 0;
    
    /* 遍历所有字节进行异或运算 */
    for (uint16_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    
    return checksum;
}

/**
 * @brief  构建数据帧
 * @param  buffer: 输出缓冲区 (需要足够大以容纳完整帧)
 * @param  func_code: 功能码
 * @param  data: 数据指针 (可以为NULL)
 * @param  data_len: 数据长度
 * @retval 帧总长度
 * 
 * @note   帧格式: [帧头(1)][功能码(1)][长度(2)][数据(N)][校验和(1)]
 *         总长度 = 1 + 1 + 2 + N + 1 = N + 5
 */
uint16_t Comm_BuildFrame(uint8_t *buffer, CommFuncCode_t func_code, 
                         const void *data, uint16_t data_len)
{
    uint16_t index = 0;
    
    /* 1. 写入帧头 */
    buffer[index++] = COMM_FRAME_HEADER;
    
    /* 2. 写入功能码 */
    buffer[index++] = (uint8_t)func_code;
    
    /* 3. 写入数据长度 (小端序: 低字节在前) */
    buffer[index++] = (uint8_t)(data_len & 0xFF);         /* 低字节 */
    buffer[index++] = (uint8_t)((data_len >> 8) & 0xFF);  /* 高字节 */
    
    /* 4. 写入数据段 */
    if (data != NULL && data_len > 0) {
        memcpy(&buffer[index], data, data_len);
        index += data_len;
    }
    
    /* 5. 计算并写入校验和 */
    /* 校验和计算范围: 从功能码到数据段结束 (不包括帧头) */
    uint8_t checksum = Comm_CalcChecksum(&buffer[1], 3 + data_len);
    buffer[index++] = checksum;
    
    return index;  /* 返回帧总长度 */
}

/**
 * @brief  解析数据帧
 * @param  buffer: 输入缓冲区
 * @param  len: 缓冲区长度
 * @param  func_code: 输出功能码指针
 * @param  data: 输出数据指针的指针 (指向buffer中的数据段)
 * @param  data_len: 输出数据长度指针
 * @retval 0=成功, 负数=错误码
 * 
 * @note   错误码定义:
 *         -1: 数据长度不足
 *         -2: 帧头错误
 *         -3: 校验和错误
 *         -4: 数据长度超出范围
 */
int Comm_ParseFrame(const uint8_t *buffer, uint16_t len,
                    CommFuncCode_t *func_code, uint8_t **data, uint16_t *data_len)
{
    /* 1. 检查最小帧长度 (帧头 + 功能码 + 长度 + 校验和 = 5字节) */
    if (len < COMM_HEADER_SIZE + COMM_CHECKSUM_SIZE) {
        return -1;  /* 数据长度不足 */
    }
    
    /* 2. 检查帧头 */
    if (buffer[0] != COMM_FRAME_HEADER) {
        return -2;  /* 帧头错误 */
    }
    
    /* 3. 提取功能码 */
    *func_code = (CommFuncCode_t)buffer[1];
    
    /* 4. 提取数据长度 (小端序) */
    *data_len = buffer[2] | ((uint16_t)buffer[3] << 8);
    
    /* 5. 检查数据长度是否合理 */
    if (*data_len > COMM_MAX_DATA_LEN) {
        return -4;  /* 数据长度超出范围 */
    }
    
    /* 6. 检查缓冲区是否包含完整帧 */
    uint16_t frame_len = COMM_HEADER_SIZE + *data_len + COMM_CHECKSUM_SIZE;
    if (len < frame_len) {
        return -1;  /* 数据长度不足 */
    }
    
    /* 7. 验证校验和 */
    uint8_t calc_checksum = Comm_CalcChecksum(&buffer[1], 3 + *data_len);
    uint8_t recv_checksum = buffer[COMM_HEADER_SIZE + *data_len];
    
    if (calc_checksum != recv_checksum) {
        return -3;  /* 校验和错误 */
    }
    
    /* 8. 设置数据指针 (指向buffer中的数据段起始位置) */
    if (*data_len > 0) {
        *data = (uint8_t *)&buffer[COMM_HEADER_SIZE];
    } else {
        *data = NULL;
    }
    
    return 0;  /* 解析成功 */
}
