/*
 * utilities.h
 *
 *  Created on: Dec 13, 2020
 *      Author: lukasz
 */

#pragma once

#include "main.h"	// for GPIO's name definition, HAL lib, System struct
#include "printf.h"	// for SPAM macro

/*** Exported types ***********************************************************/

enum eState
{
	ON,
	OFF,
	TOGGLE,
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

static inline void powerLockOn (void)
{
	HAL_GPIO_WritePinHigh(PWR_LOCK_GPIO_Port, PWR_LOCK_Pin);
}

static inline void powerLockOff (void)
{
	SPAM(("%s\n", __func__));
	HAL_GPIO_WritePinLow(PWR_LOCK_GPIO_Port, PWR_LOCK_Pin);
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

/*** Exported functions *******************************************************/

void ledDemo(void);
void readKeyboard(void);



/**************** END OF FILE *************************************************/
