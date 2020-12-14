/*
 * utilities.c
 *
 *  Created on: Dec 13, 2020
 *      Author: lukasz
 */


#include "utilities.h"


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


void readKeyboard(void)
{
	static uint32_t uTimeTick = 0;
	static uint32_t uPowerCounter = 0;

	if (HAL_GetTick() - uTimeTick > 10)
	{
		uTimeTick += 10;
		// read every 10 ms

		if (HAL_GPIO_ReadPin(KEY_PWR_GPIO_Port, KEY_PWR_Pin) == GPIO_PIN_RESET)
			uPowerCounter++;
		else
			uPowerCounter = 0;

		if (uPowerCounter > 200)
		{
			powerLockOff();
			while(0xDEADBABE);
		}
	}
}



