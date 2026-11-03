/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32h7xx_hal.h"

// @@@ 
/* USER CODE BEGIN Private defines */

/* GPIO OUTPUT PINS */

#define GPIO_OUT0_Pin       GPIO_PIN_0
#define GPIO_OUT0_GPIO_Port GPIOA

#define GPIO_OUT1_Pin       GPIO_PIN_1
#define GPIO_OUT1_GPIO_Port GPIOA

#define GPIO_OUT2_Pin       GPIO_PIN_2
#define GPIO_OUT2_GPIO_Port GPIOA

#define GPIO_OUT3_Pin       GPIO_PIN_3
#define GPIO_OUT3_GPIO_Port GPIOA

#define GPIO_OUT4_Pin       GPIO_PIN_4
#define GPIO_OUT4_GPIO_Port GPIOA

#define GPIO_OUT5_Pin       GPIO_PIN_5
#define GPIO_OUT5_GPIO_Port GPIOA

#define GPIO_OUT6_Pin       GPIO_PIN_6
#define GPIO_OUT6_GPIO_Port GPIOA

#define GPIO_OUT7_Pin       GPIO_PIN_7
#define GPIO_OUT7_GPIO_Port GPIOA

#define PWM1_CHANNEL TIM_CHANNEL_1  // PB6 - TIM4 CH1

#define PWM2_CHANNEL TIM_CHANNEL_2  // PB7 - TIM4 CH2

#define PWM3_CHANNEL TIM_CHANNEL_3  // PB0 - TIM3 CH3

#define PWM4_CHANNEL TIM_CHANNEL_1  // PE9 - TIM1 CH1


/* USER CODE END Private defines */
// @@@


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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
