/*
 * utilities.c
 *
 *  Created on: Dec 13, 2020
 *      Author: lukasz
 */


#include "utilities.h"
#include "hd44780_i2c.h"


#pragma GCC push_options
#pragma GCC optimize("O0")
void delay_us(uint32_t us)
{
//	uint32_t cnt = (uint32_t)((uint64_t)us * (uint64_t)SystemCoreClock / (uint64_t)21500000);
//	while ( cnt-- ) ;//__NOP();	/* 6 instructions */
	while(us--) {
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP();
		/* @72 MHz : 52x NOP @ Nucleo F303RE - got experimentally */

		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP();
		/* @80 MHz : 66x NOP @ stm32 L431 - got experimentally */
	}
}
#pragma GCC pop_options


void delay_ms(uint32_t ms)
{
	while ( ms-- ) delay_us(1000);
}


void ledDemo(void)
{
	static uint32_t uTimeTick = 0;
	static uint32_t uLedNo = 0;

	if (HAL_GetTick() - uTimeTick > 250)	// every 250 ms
	{
		if (uLedNo == 0)
			ledBlue(TOGGLE);
		else if (uLedNo == 1)
			ledGreen(TOGGLE);
		else if (uLedNo == 2)
			ledOrange(TOGGLE);
		else
		{
			ledRed(TOGGLE);
		}

		if (uLedNo >= 3)
			uLedNo = 0;
		else
			uLedNo++;

		uTimeTick = HAL_GetTick();
	}
}

/* Blink red LED 'errNo' times. Blink the sequence twice. */
void ledError(uint32_t errNo)
{
	const uint32_t delay = 250;

	uint32_t repeats = errNo;

	while (repeats--)
	{
		ledRed(ON);
		HAL_Delay(delay);
		ledRed(OFF);
		HAL_Delay(delay);
	}

	HAL_Delay(delay + delay + delay + delay);

	while (errNo--)
	{
		ledRed(ON);
		HAL_Delay(delay);
		ledRed(OFF);
		HAL_Delay(delay);
	}
}



/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
