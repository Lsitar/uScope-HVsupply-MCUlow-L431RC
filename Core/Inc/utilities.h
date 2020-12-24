/*
 * utilities.h
 *
 *  Created on: Dec 13, 2020
 *      Author: lukasz
 */

#pragma once

#include <stdbool.h>
#include "main.h"	// for GPIO's name definition, HAL lib, System struct
#include "printf.h"	// for SPAM macro

/*** Exported types ***********************************************************/

enum eState
{
	ON,
	OFF,
	TOGGLE,
};

enum ePwmChannel
{
	PWM_CHANNEL_UK,
	PWM_CHANNEL_UE,
	PWM_CHANNEL_UF,
	PWM_CHANNEL_PUMP,
};

/*** Exported inline snippets *************************************************/

static inline void HAL_GPIO_WritePinLow(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->BRR = (uint32_t)GPIO_Pin;
}

static inline void HAL_GPIO_WritePinHigh(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	GPIOx->BSRR = (uint32_t)GPIO_Pin;
}

static inline void ledBlue(enum eState state)
{
	if (state == ON)
		HAL_GPIO_WritePinHigh(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
	else if (state == OFF)
		HAL_GPIO_WritePinLow(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
	else
		HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
}

static inline void ledGreen(enum eState state)
{
	if (state == ON)
		HAL_GPIO_WritePinHigh(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	else if (state == OFF)
		HAL_GPIO_WritePinLow(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	else
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
}

static inline void ledOrange(enum eState state)
{
	if (state == ON)
		HAL_GPIO_WritePinHigh(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin);
	else if (state == OFF)
		HAL_GPIO_WritePinLow(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin);
	else
		HAL_GPIO_TogglePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin);
}

static inline void ledRed(enum eState state)
{
	if (state == ON)
		HAL_GPIO_WritePinHigh(LED_RED_GPIO_Port, LED_RED_Pin);
	else if (state == OFF)
		HAL_GPIO_WritePinLow(LED_RED_GPIO_Port, LED_RED_Pin);
	else
		HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
}

static inline void powerLockOn (void)
{
	HAL_GPIO_WritePinHigh(PWR_LOCK_GPIO_Port, PWR_LOCK_Pin);
}

static inline void powerLockOff (void)
{
	SPAM(("%s\n", __func__));
	HAL_GPIO_WritePinLow(PWR_LOCK_GPIO_Port, PWR_LOCK_Pin);
}

static inline void powerHVon (void)
{
	SPAM(("%s\n", __func__));
	ledOrange(ON);
	HAL_GPIO_WritePinHigh(TP32_GPIO_Port, TP32_Pin);
}

static inline void powerHVoff (void)
{
	SPAM(("%s\n", __func__));
	ledOrange(OFF);
	HAL_GPIO_WritePinLow(TP32_GPIO_Port, TP32_Pin);
}

static inline void pwmSetDuty(enum ePwmChannel channel, float duty)
{
	if (duty > 1.0f)
	{
		SPAM(("%s wrong duty!", __func__ ));
		return;
	}

	switch (channel)
	{
	case PWM_CHANNEL_UK:
		TIM1->CCR1 = (uint32_t)(65535.0 * duty);
		break;
	case PWM_CHANNEL_UE:
		TIM1->CCR2 = (uint32_t)(65535.0 * duty);
		break;
	case PWM_CHANNEL_UF:
		TIM1->CCR3 = (uint32_t)(65535.0 * duty);
		break;
	case PWM_CHANNEL_PUMP:
		TIM1->CCR4 = (uint32_t)(65535.0 * duty);
		break;
	}
}

/*
 * Testpoints
 * 3 pins on left:
 * 	TP28 - TP31 - TP32
 *
 * 4 pins by the edge:
 * 	TP29 - TP30 - TP25 - TP26
 */
static inline void testpin29(bool state)
{
	if (state)
		HAL_GPIO_WritePinHigh(TP29_GPIO_Port, TP29_Pin);
	else
		HAL_GPIO_WritePinLow(TP29_GPIO_Port, TP29_Pin);
}

/*** Exported functions *******************************************************/

void ledDemo(void);
void ledError(uint32_t);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);


/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
