/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "printf.h"
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
#define ADC1_CH1_BAT_Pin GPIO_PIN_0
#define ADC1_CH1_BAT_GPIO_Port GPIOC
#define DAC_SPI2_MOSI_Pin GPIO_PIN_3
#define DAC_SPI2_MOSI_GPIO_Port GPIOC
#define ADC_SPI1_SCK_Pin GPIO_PIN_1
#define ADC_SPI1_SCK_GPIO_Port GPIOA
#define ADC_RESET_Pin GPIO_PIN_4
#define ADC_RESET_GPIO_Port GPIOA
#define ADC_SPI_CS_Pin GPIO_PIN_5
#define ADC_SPI_CS_GPIO_Port GPIOA
#define ADC_SPI1_MISO_Pin GPIO_PIN_6
#define ADC_SPI1_MISO_GPIO_Port GPIOA
#define ADC_SPI1_MOSI_Pin GPIO_PIN_7
#define ADC_SPI1_MOSI_GPIO_Port GPIOA
#define ADC_DRDY_EXTI4_Pin GPIO_PIN_4
#define ADC_DRDY_EXTI4_GPIO_Port GPIOC
#define DAC_LOADDATA_Pin GPIO_PIN_5
#define DAC_LOADDATA_GPIO_Port GPIOC
#define ENC_CHA_EXTI0_Pin GPIO_PIN_0
#define ENC_CHA_EXTI0_GPIO_Port GPIOB
#define ENC_CHB_Pin GPIO_PIN_1
#define ENC_CHB_GPIO_Port GPIOB
#define ENC_SW_Pin GPIO_PIN_2
#define ENC_SW_GPIO_Port GPIOB
#define DAC_SPI2_SCK_Pin GPIO_PIN_10
#define DAC_SPI2_SCK_GPIO_Port GPIOB
#define DAC_SPI_CS_Pin GPIO_PIN_11
#define DAC_SPI_CS_GPIO_Port GPIOB
#define DAC_SPI2_NSS_Pin GPIO_PIN_12
#define DAC_SPI2_NSS_GPIO_Port GPIOB
#define PWR_LOCK_Pin GPIO_PIN_14
#define PWR_LOCK_GPIO_Port GPIOB
#define KEY_PWR_Pin GPIO_PIN_15
#define KEY_PWR_GPIO_Port GPIOB
#define KEY_UP_Pin GPIO_PIN_6
#define KEY_UP_GPIO_Port GPIOC
#define KEY_DOWN_Pin GPIO_PIN_7
#define KEY_DOWN_GPIO_Port GPIOC
#define KEY_ENTER_Pin GPIO_PIN_8
#define KEY_ENTER_GPIO_Port GPIOC
#define KEY_ESC_Pin GPIO_PIN_9
#define KEY_ESC_GPIO_Port GPIOC
#define LED_BLUE_Pin GPIO_PIN_15
#define LED_BLUE_GPIO_Port GPIOA
#define LED_GREEN_Pin GPIO_PIN_10
#define LED_GREEN_GPIO_Port GPIOC
#define LED_ORANGE_Pin GPIO_PIN_11
#define LED_ORANGE_GPIO_Port GPIOC
#define LED_RED_Pin GPIO_PIN_12
#define LED_RED_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */

struct sSystem
{
	float fAnodeCurrent;
	float fKathodeVolt;
	float fExtractVolt;
	float fFocusVolt;
	float fPumpVolt;

	struct
	{
		uint_least8_t uRowActive;
	} menu;
};

extern struct sSystem System;


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
