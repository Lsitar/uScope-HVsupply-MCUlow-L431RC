/*
 * utilities.h
 *
 *  Created on: Dec 13, 2020
 *      Author: Lukasz Sitarek
 */

#pragma once

#include <stdbool.h>
#include "main.h"	// for GPIO's name definition, HAL lib, System struct
#include "printf.h"	// for SPAM macro
#include "typedefs.h"	// for tsRegulatedVal definition

/* Exported types ------------------------------------------------------------*/

enum eState
{
	ON,
	OFF,
	TOGGLE,
	BLINK,
};

#define FLASH_DATA_PARTS	((uint32_t)((sizeof(tsRegulatedVal)/sizeof(uint64_t))+1))

typedef union
{
	struct
	{
		tsRegulatedVal data;
		uint8_t crc;
	} flashData;

	uint64_t buff64[FLASH_DATA_PARTS];
	uint8_t buff8[FLASH_DATA_PARTS * sizeof(uint64_t)];
} tuFlashData;

extern tuFlashData uFlashData;

/* Exported inline snippets --------------------------------------------------*/

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
	else if (state == TOGGLE)
		HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
	else if (state == BLINK)
	{
		static uint32_t uTimeTick;
		if (HAL_GetTick() - uTimeTick > 100)
		{
			uTimeTick = HAL_GetTick();
			HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
		}
	}
}

static inline void ledGreen(enum eState state)
{
	if (state == ON)
		HAL_GPIO_WritePinHigh(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	else if (state == OFF)
		HAL_GPIO_WritePinLow(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	else if (state == TOGGLE)
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	else if (state == BLINK)
	{
		static uint32_t uTimeTick;
		if (HAL_GetTick() - uTimeTick > 100)
		{
			uTimeTick = HAL_GetTick();
			HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
		}
	}
}

static inline void ledOrange(enum eState state)
{
	if (state == ON)
		HAL_GPIO_WritePinHigh(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin);
	else if (state == OFF)
		HAL_GPIO_WritePinLow(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin);
	else if (state == TOGGLE)
		HAL_GPIO_TogglePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin);
}

static inline void ledRed(enum eState state)
{
	if (state == ON)
		HAL_GPIO_WritePinHigh(LED_RED_GPIO_Port, LED_RED_Pin);
	else if (state == OFF)
		HAL_GPIO_WritePinLow(LED_RED_GPIO_Port, LED_RED_Pin);
	else if (state == TOGGLE)
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

/* Exported functions --------------------------------------------------------*/

void delay_us(uint32_t us);
static inline void delay_ms(uint32_t ms) { while ( ms-- ) delay_us(1000); }

uint8_t crc8(const uint8_t *, uint32_t);

void ledDemo(void);
void ledError(uint32_t);

bool flashReadConfig(void);
void flashSaveConfig(void);

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
