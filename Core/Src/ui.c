/*
 * gui.c
 *
 *  Created on: Dec 21, 2020
 *      Author: lukasz
 */
#include "main.h"
#include "ui.h"
#include "typedefs.h"
#include "utilities.h"



static char LCD_buff[20];
static enum eScreen actualScreen;
static uint32_t uScreenTimer;	// measure time from last screenChange
static uint32_t uBlinkTimer;
bool bBlink;

#define KB_READ_INTERVAL		50	// ms
#define KB_PRESSED_THRESHOLD	2	// ticks ca. 100 ms
#define KB_POWEROFF_THRESHOLD	40	// ticks ca. 2 s

enum eKey
{
	KEY_ENTER,
	KEY_ESC,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_POWER,
	KEY_ENC,
	KEY_NUMBER_OF,
};



static void _changeScreenLeftRight(enum eKey key)
{

	if (actualScreen == SCREEN_1)
		uiScreenChange(SCREEN_2);
	else if (actualScreen == SCREEN_2)
		uiScreenChange(SCREEN_1);
	else if (actualScreen == SCREEN_SET_UK)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UF);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_IA);
	}
	else if (actualScreen == SCREEN_SET_IA)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UK);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UF);
	}
	else if (actualScreen == SCREEN_SET_UF)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_IA);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UK);
	}
}



void uiScreenChange(enum eScreen newScreen)
{
	HD44780_Clear();

	switch (newScreen)
	{
	case SCREEN_RESET:
		break;

	case SCREEN_1:
		// line 1 - Cathode voltage
		HD44780_Puts(5, 0, "UC:");
		// line 2 - Cathode current
		HD44780_Puts(5, 1, "IA:");
		// line 3 - Cathode power
		HD44780_Puts(2, 2, "Power:");
		// line 4 - Battery SoC
		HD44780_Puts(0, 3, "Battery:");
		break;

	case SCREEN_2:
		// line 1 - Extract voltage
		HD44780_Puts(5, 0, "UE:");
		// line 2 - Focus voltage
		HD44780_Puts(1, 1, "Ufocus:");
		// line 3 - Pump voltage
		HD44780_Puts(2, 2, "Upump:");
		// line 4 - Battery SoC
		HD44780_Puts(0, 3, "Battery:");
		break;

	case SCREEN_SET_IA:
		break;

	case SCREEN_SET_UK:
		break;

	case SCREEN_SET_UF:
		break;

	case SCREEN_POWERON_1:
		HD44780_Puts(2, 0, "Microscope supply");	// 17 chars
		HD44780_Puts(4, 1, "Politechnika");	// 12 chars
		HD44780_Puts(5, 2, "Wroclawska");	// 10 chars
		HD44780_Puts(5, 3, "W11  WEMiF");	// 9 chars
		break;

	case SCREEN_POWERON_2:
		HD44780_Puts(2, 0, "Microscope supply");	// 17 chars
		HD44780_Puts(3, 2, "Lukasz Sitarek");	// 14 chars
		HD44780_Puts(8, 3, "2021");	// 4 chars
//		HD44780_Puts(3, 3, "Wroclaw University");	// 18 chars
//		HD44780_Puts(3, 3, "of Science and Technology");	// 25 chars
		break;

	case SCREEN_POWEROFF:
		HD44780_Puts(5, 2, "Power off");
		break;
	}
	actualScreen = newScreen;
	uScreenTimer = HAL_GetTick();
}



uint32_t uiGetScreenTime(void)
{
	return HAL_GetTick() - uScreenTimer;
}



/*
 * Call this function every.. 50 ms?
 */
void uiScreenUpdate(void)
{
	switch (actualScreen)
	{
	case SCREEN_RESET:
		break;

	case SCREEN_1:
		// line 1 - Cathode voltage
//		snprintf_(LCD_buff, 20, "%i", System.ads.data.channel1);
		snprintf_(LCD_buff, 20, "%.2f V", System.meas.fCathodeVolt);
		HD44780_Puts(9, 0, LCD_buff);
		// line 2 - Anode current
//		snprintf_(LCD_buff, 20, "%i", System.ads.data.channel0);
		snprintf_(LCD_buff, 20, "%.2f uA", (System.meas.fAnodeCurrent * 1e+6));
		HD44780_Puts(9, 1, LCD_buff);
		// line 3 - Anode power
//		snprintf_(LCD_buff, 20, "%i", (System.ads.data.channel0 * System.ads.data.channel1));
		snprintf_(LCD_buff, 20, "%.2f mW", (System.meas.fCathodeVolt * System.meas.fAnodeCurrent * 1000.0f));
		HD44780_Puts(9, 2, LCD_buff);
		// line 4 - Battery SoC TODO
		break;

	case SCREEN_2:
		break;

	case SCREEN_SET_IA:
		// blink text
		if (HAL_GetTick() - uBlinkTimer > 500)
		{
			if (bBlink == true)
			{
				HD44780_Clear();
				bBlink = false;
			}
			else
			{
				HD44780_Puts(0, 2, "SET IA:");
				bBlink = true;
			}
			uBlinkTimer = HAL_GetTick();
		}
		// print number anyway
		int value = System.ref.fAnodeCurrent * 1e7;
		snprintf_(LCD_buff, 20, "%i.%i", value/10, value%10 );
		HD44780_Puts(10, 2, LCD_buff);
		break;

	case SCREEN_SET_UK:
		break;

	case SCREEN_SET_UF:
		break;

	case SCREEN_POWERON_1:
	case SCREEN_POWERON_2:
	case SCREEN_POWEROFF:
		break;
	}
}



/*
 * Internal time interval: 50 ms
 */
void readKeyboard(void)
{
	static uint32_t uTimeTick = 0;
	static uint32_t uKeysPressedTime[KEY_NUMBER_OF] = {0};

	if (HAL_GetTick() - uTimeTick > KB_READ_INTERVAL)
	{
		uTimeTick += KB_READ_INTERVAL;

		// KEY_POWER ///////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_PWR_GPIO_Port, KEY_PWR_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_POWER]++;

			if (uKeysPressedTime[KEY_POWER] > KB_POWEROFF_THRESHOLD)
			{
				powerLockOff();
				HD44780_Clear();
				HD44780_Puts(5, 2, "Power off");
				ledRed(ON);
				while(0xDEADBABE);
			}
		}
		else
			uKeysPressedTime[KEY_POWER] = 0;

		// KEY_ENTER ///////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_ENTER_GPIO_Port, KEY_ENTER_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_ENTER]++;
			if (	(actualScreen != SCREEN_SET_IA)
				 && (actualScreen != SCREEN_SET_UF)
				 && (actualScreen != SCREEN_SET_UK))
			{
				uiScreenChange(SCREEN_SET_IA);
			}
			else
			{
				//TODO accept settings
			}
		}
		else
			uKeysPressedTime[KEY_ENTER] = 0;

		// KEY_ESC /////////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_ESC_GPIO_Port, KEY_ESC_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_ESC]++;
		}
		else
			uKeysPressedTime[KEY_ESC] = 0;

		// KEY_LEFT ////////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_LEFT_GPIO_Port, KEY_LEFT_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_LEFT]++;

			if (uKeysPressedTime[KEY_LEFT] == KB_PRESSED_THRESHOLD)	// do not repeat 'pressed' action
			{
				_changeScreenLeftRight(KEY_LEFT);
			}
		}
		else
			uKeysPressedTime[KEY_LEFT] = 0;

		// KEY_RIGHT ///////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_RIGHT_GPIO_Port, KEY_RIGHT_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_RIGHT]++;

			if (uKeysPressedTime[KEY_RIGHT] == KB_PRESSED_THRESHOLD)	// do not repeat 'pressed' action
			{
				_changeScreenLeftRight(KEY_RIGHT);
			}
		}
		else
			uKeysPressedTime[KEY_RIGHT] = 0;
	}
}






void encoderKnob_turnCallback(void)
{
	/* falling edge on node A - check B level */

//	if (locked == false)
//	{

		if (HAL_GPIO_ReadPin(ENC_CHB_GPIO_Port, ENC_CHB_Pin) == GPIO_PIN_SET)
		{
			// turn right impulse
			;
		}
		else
		{
			// turn left impulse
			;
		}
//	}
}


void encoderKnob_buttonCallback(void)
{
//	if (Bit_SET == GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_11))
//	{	/* Button released */
//		__NOP();
//	}
//	else
//	{	/* Button pressed */
//		if (locked == false)
//			locked = true;
//		else
//			locked = false;
//	}
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
