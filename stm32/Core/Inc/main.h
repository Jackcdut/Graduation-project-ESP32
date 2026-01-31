/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define POWER_VFB_ADC_Pin GPIO_PIN_6
#define POWER_VFB_ADC_GPIO_Port GPIOF
#define POWER_IFB_ADC_Pin GPIO_PIN_7
#define POWER_IFB_ADC_GPIO_Port GPIOF
#define POWER_Input_ADC_Pin GPIO_PIN_8
#define POWER_Input_ADC_GPIO_Port GPIOF
#define POWER_NTC_ADC_Pin GPIO_PIN_9
#define POWER_NTC_ADC_GPIO_Port GPIOF
#define POWER_CC_ADC_Pin GPIO_PIN_10
#define POWER_CC_ADC_GPIO_Port GPIOF
#define AD9833_MISO_Pin GPIO_PIN_2
#define AD9833_MISO_GPIO_Port GPIOC
#define AD9833_MOSI_Pin GPIO_PIN_3
#define AD9833_MOSI_GPIO_Port GPIOC
#define Oscilloscope_ADC_Pin GPIO_PIN_0
#define Oscilloscope_ADC_GPIO_Port GPIOA
#define Multimeter_Vol_Collect_Pin GPIO_PIN_1
#define Multimeter_Vol_Collect_GPIO_Port GPIOA
#define Multimeter_Res_Collect_Pin GPIO_PIN_2
#define Multimeter_Res_Collect_GPIO_Port GPIOA
#define Multimeter_Cur_Collect_Pin GPIO_PIN_3
#define Multimeter_Cur_Collect_GPIO_Port GPIOA
#define POWER_CV_DAC_Pin GPIO_PIN_4
#define POWER_CV_DAC_GPIO_Port GPIOA
#define POWER_CCS_DAC_Pin GPIO_PIN_5
#define POWER_CCS_DAC_GPIO_Port GPIOA
#define Oscilloscope_MISO_Pin GPIO_PIN_6
#define Oscilloscope_MISO_GPIO_Port GPIOA
#define Oscilloscope_MOSI_Pin GPIO_PIN_7
#define Oscilloscope_MOSI_GPIO_Port GPIOA
#define Oscilloscope_PGA_CS_Pin GPIO_PIN_4
#define Oscilloscope_PGA_CS_GPIO_Port GPIOC
#define MCP41010_CS_Pin GPIO_PIN_14
#define MCP41010_CS_GPIO_Port GPIOE
#define AD9833_CS_Pin GPIO_PIN_15
#define AD9833_CS_GPIO_Port GPIOE
#define AD9833_SCK_Pin GPIO_PIN_10
#define AD9833_SCK_GPIO_Port GPIOB
#define POWER_CFG1_Pin GPIO_PIN_8
#define POWER_CFG1_GPIO_Port GPIOD
#define POWER_CFG2_Pin GPIO_PIN_9
#define POWER_CFG2_GPIO_Port GPIOD
#define POWER_CFG3_Pin GPIO_PIN_10
#define POWER_CFG3_GPIO_Port GPIOD
#define Multimeter_1KR_Pin GPIO_PIN_3
#define Multimeter_1KR_GPIO_Port GPIOG
#define Multimeter_10KR_Pin GPIO_PIN_4
#define Multimeter_10KR_GPIO_Port GPIOG
#define Multimeter_100KR_Pin GPIO_PIN_5
#define Multimeter_100KR_GPIO_Port GPIOG
#define Multimeter_1MR_Pin GPIO_PIN_6
#define Multimeter_1MR_GPIO_Port GPIOG
#define Oscilloscope_AC_Pin GPIO_PIN_7
#define Oscilloscope_AC_GPIO_Port GPIOG
#define Data_TX_Pin GPIO_PIN_9
#define Data_TX_GPIO_Port GPIOA
#define Data_RX_Pin GPIO_PIN_10
#define Data_RX_GPIO_Port GPIOA
#define Multimeter_Black_Con_Pin GPIO_PIN_4
#define Multimeter_Black_Con_GPIO_Port GPIOD
#define Multimeter_Res_Con_Pin GPIO_PIN_11
#define Multimeter_Res_Con_GPIO_Port GPIOG
#define Multimeter_Vol_Con_Pin GPIO_PIN_12
#define Multimeter_Vol_Con_GPIO_Port GPIOG
#define Multimeter_Cur_Con_Pin GPIO_PIN_13
#define Multimeter_Cur_Con_GPIO_Port GPIOG
#define Oscilloscope_SCK_Pin GPIO_PIN_3
#define Oscilloscope_SCK_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
