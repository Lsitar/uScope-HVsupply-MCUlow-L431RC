/*
 * gui.c
 *
 *  Created on: Dec 21, 2020
 *      Author: lukasz
 */
#include <string.h>
#include "main.h"
#include "ui.h"
#include "typedefs.h"
#include "utilities.h"

/* private variables */

static char LCD_buff[20];			// working buffer for snprintf
static enum eScreen actualScreen;
static uint32_t uScreenTimer;		// measure time from last screenChange
static struct sRegulatedVal localRef;	// local copy of values changed at settings
static bool bBlink;


static uint32_t setDigit = 1;

/* config / constants */

#define KB_READ_INTERVAL		50	// ms
#define KB_PRESSED_THRESHOLD	2	// ticks ca. 100 ms
#define KB_POWEROFF_THRESHOLD	40	// ticks ca. 2 s
#define LCD_BLINK_TIME			500	// ms per state

/* macros */

#define IS_SETTINGS_SCREEN	(  (actualScreen == SCREEN_SET_UC)   \
							|| (actualScreen == SCREEN_SET_IA)   \
							|| (actualScreen == SCREEN_SET_UF)   \
							|| (actualScreen == SCREEN_SET_UP))  \


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

static uint32_t uKeysPressedTime[KEY_NUMBER_OF] = {0};


static void _changeScreenLeftRight(enum eKey key)
{
	// Switch MAIN screens /////////////////////////////////////////////////////
	if (actualScreen == SCREEN_1)
		uiScreenChange(SCREEN_2);
	else if (actualScreen == SCREEN_2)
		uiScreenChange(SCREEN_1);

	// Switch SETTINGS screens /////////////////////////////////////////////////
	else if (actualScreen == SCREEN_SET_UC)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UP);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_IA);
	}
	else if (actualScreen == SCREEN_SET_IA)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UC);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UF);
	}
	else if (actualScreen == SCREEN_SET_UF)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_IA);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UP);
	}
	else if (actualScreen == SCREEN_SET_UP)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UF);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UC);
	}

	if (IS_SETTINGS_SCREEN)
		setDigit = 1;
}



/*
 * @return	true if global 'bBlink' changed, false if not.
 */
static bool _blinkTimer(void)
{
	static uint32_t uBlinkTimer;

	if (HAL_GetTick() - uBlinkTimer > LCD_BLINK_TIME)
	{
		if (bBlink == true)
		{
			bBlink = false;
		}
		else
		{
			bBlink = true;
		}
		uBlinkTimer = HAL_GetTick();
		return true;
	}
	return false;
}



void uiScreenChange(enum eScreen newScreen)
{
	HD44780_Clear();

	switch (newScreen)
	{
	case SCREEN_RESET:
		HAL_Delay(5);	// give some time to send rest from buffer
		HD44780_Init(20, 4);
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

	case SCREEN_SET_UC:
	case SCREEN_SET_IA:
	case SCREEN_SET_UF:
	case SCREEN_SET_UP:
		HD44780_Puts(0, 0, "SET UC:");
		HD44780_Puts(0, 1, "SET IA:");
		HD44780_Puts(0, 2, "SET UF:");
		HD44780_Puts(0, 3, "SET UP:");
		// print UC
		snprintf_(LCD_buff, 20, "%.0f V", (localRef.fCathodeVolt) );
		HD44780_Puts(10, 0, LCD_buff);
		// print IA
		snprintf_(LCD_buff, 20, "%.1f uA", (localRef.fAnodeCurrent * 1e6) );
		HD44780_Puts(10, 1, LCD_buff);
		// print UF
		snprintf_(LCD_buff, 20, "%.0f V", (localRef.fFocusVolt) );
		HD44780_Puts(10, 2, LCD_buff);
		// print UP
		snprintf_(LCD_buff, 20, "%.0f V", (localRef.fPumpVolt) );
		HD44780_Puts(10, 3, LCD_buff);
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
 * Duration ca. 100 - 150 us
 */
void uiScreenUpdate(void)
{
	static uint32_t uTimeTick = 0;
	static bool bInit = false;

	if (bInit == false)
	{
		uTimeTick = HAL_GetTick();
		bInit = true;
	}

	if (HAL_GetTick() - uTimeTick > 50)
	{
		uTimeTick += 50;
//		uTimeTick = HAL_GetTick();

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

		case SCREEN_2: // Ue Uf Up
			// line 1 - Extract voltage
			snprintf_(LCD_buff, 20, "%.2f V", System.meas.fExtractVolt);
			HD44780_Puts(9, 0, LCD_buff);
			// line 2 - Focus voltage
			snprintf_(LCD_buff, 20, "%.2f V", (System.meas.fFocusVolt));
			HD44780_Puts(9, 1, LCD_buff);
			// line 3 - Pump voltage
			snprintf_(LCD_buff, 20, "%.2f V", (System.ref.fPumpVolt));
			HD44780_Puts(9, 2, LCD_buff);
			// line 4 - Battery SoC TODO
			break;

		case SCREEN_SET_UC:
			// blink text
			if (_blinkTimer() == true)
			{
				if (bBlink == true)
					HD44780_Puts(0, 0, "          ");	// clear one text line
				else
					HD44780_Puts(0, 0, "SET UC:");
			}

			// print number anyway
			HD44780_Puts(10, 0, "         ");	// clear line
			snprintf_(LCD_buff, 20, "%.0f V", (localRef.fCathodeVolt) );
			HD44780_Puts(10, 0, LCD_buff);
			break;

		case SCREEN_SET_IA:
			// blink text
			if (_blinkTimer() == true)
			{
				if (bBlink == true)
					HD44780_Puts(0, 1, "          ");	// clear one text line
				else
					HD44780_Puts(0, 1, "SET IA:");
			}

			// print number anyway
			HD44780_Puts(10, 1, "         ");	// clear line
			snprintf_(LCD_buff, 20, "%.1f uA", (localRef.fAnodeCurrent * 1e6) );
			HD44780_Puts(10, 1, LCD_buff);
			break;

		case SCREEN_SET_UF:
			// blink text
			if (_blinkTimer() == true)
			{
				if (bBlink == true)
					HD44780_Puts(0, 2, "          ");	// clear one text line
				else
					HD44780_Puts(0, 2, "SET UF:");
			}

			// print number anyway
			HD44780_Puts(10, 2, "         ");	// clear line
			snprintf_(LCD_buff, 20, "%.0f V", (localRef.fFocusVolt) );
			HD44780_Puts(10, 2, LCD_buff);
			break;

		case SCREEN_SET_UP:
			// blink text
			if (_blinkTimer() == true)
			{
				if (bBlink == true)
					HD44780_Puts(0, 3, "          ");	// clear one text line
				else
					HD44780_Puts(0, 3, "SET UP:");
			}

			// print number anyway
			HD44780_Puts(10, 3, "         ");	// clear line
			snprintf_(LCD_buff, 20, "%.0f V", (localRef.fPumpVolt) );
			HD44780_Puts(10, 3, LCD_buff);
			break;

		case SCREEN_POWERON_1:
		case SCREEN_POWERON_2:
		case SCREEN_POWEROFF:
			break;
		}
	}
}



/*
 * Internal time interval: 50 ms
 */
void readKeyboard(void)
{
	static uint32_t uTimeTick = 0;
//	static uint32_t uKeysPressedTime[KEY_NUMBER_OF] = {0};
	static bool bInit;

	if (bInit == false)
	{
		uTimeTick = HAL_GetTick();
		bInit = true;
	}

	if (HAL_GetTick() - uTimeTick > KB_READ_INTERVAL)
	{
		uTimeTick += KB_READ_INTERVAL;

		// KEY_POWER ///////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_PWR_GPIO_Port, KEY_PWR_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_POWER]++;

			if (uKeysPressedTime[KEY_POWER] > KB_POWEROFF_THRESHOLD)
			{
				// TODO shutdown safely: disable HV, wait for Voltmeter readings near 0 V
				powerLockOff();
				HD44780_Clear();
				HD44780_Puts(5, 2, "Power off");
				//ledRed(ON);
				while(0xDEADBABE);
			}
		}
		else
			uKeysPressedTime[KEY_POWER] = 0;

		// KEY_ENTER ///////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_ENTER_GPIO_Port, KEY_ENTER_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_ENTER]++;

			if (uKeysPressedTime[KEY_ENTER] == KB_PRESSED_THRESHOLD)	// do not repeat 'pressed' action
			{
				if (!IS_SETTINGS_SCREEN)
				{	// goto settings
					memcpy(&localRef, &System.ref, sizeof(localRef));
					uiScreenChange(SCREEN_SET_UC);
				}
				else
				{
					memcpy(&System.ref, &localRef, sizeof(System.ref));
					uiScreenChange(SCREEN_1);
				}
			}
		}
		else
			uKeysPressedTime[KEY_ENTER] = 0;

		// KEY_ESC /////////////////////////////////////////////////////////////
		if (HAL_GPIO_ReadPin(KEY_ESC_GPIO_Port, KEY_ESC_Pin) == GPIO_PIN_RESET)
		{
			uKeysPressedTime[KEY_ESC]++;

			if (uKeysPressedTime[KEY_ESC] == KB_PRESSED_THRESHOLD)	// do not repeat 'pressed' action
			{
				if (IS_SETTINGS_SCREEN)
				{
					uiScreenChange(SCREEN_1);
				}
				else
				{
					enum eScreen tmp = actualScreen;
					uiScreenChange(SCREEN_RESET);
					uiScreenChange(tmp);
				}
			}
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

		// KEY_ENC /////////////////////////////////////////////////////////////
		if (uKeysPressedTime[KEY_ENC] != 0)
		{
			uKeysPressedTime[KEY_ENC]++;

			if (uKeysPressedTime[KEY_ENC] == KB_POWEROFF_THRESHOLD)	// do not repeat 'pressed' action
			{
				if (HAL_GPIO_ReadPin(TP32_GPIO_Port, TP32_Pin) == GPIO_PIN_RESET)
					powerHVon();
				else
					powerHVoff();
			}
		}
	}
}



/*
 * Change value when in Settings screen.
 *
 * NOTE: in/decreasing floats runs into problems with rounding. Instead, always
 * 		operate here on integers. Back and forth casting of voltage with
 * 		resolution 1 V works fine. Casting current escalated problem with
 * 		rounding due to 1e+7 decimal multiplying. Therefore, always store
 * 		reference current as integer (unit 0.1 uA, 1e-7 step).
 *
 * NOTE: without SPAM'ing interrupt lasts ca. 5 us, with usual SPAM text was
 * 		100-150 us. With two chars send directly via ITM, IT lasts ca. 10 us.
 */
void encoderKnob_turnCallback(void)
{
	static uint32_t setValue;
	static uint32_t lastTimeStamp;
	GPIO_PinState levelA, levelB;
	UNUSED(levelA);

	// read pin states ASAP with minimum delay between them
//	levelA = HAL_GPIO_ReadPin(ENC_CHA_EXTI0_GPIO_Port, ENC_CHA_EXTI0_Pin);	// assume interrupt is only on one edge (here: rising)
	levelB = HAL_GPIO_ReadPin(ENC_CHB_GPIO_Port, ENC_CHB_Pin);

	if (HAL_GetTick() - lastTimeStamp > 4)
	{
		/* load float value to uint */
		if (actualScreen == SCREEN_SET_IA)
			setValue = localRef.uAnodeCurrent;
		else if (actualScreen == SCREEN_SET_UC)
			setValue = localRef.fCathodeVolt;
		else if (actualScreen == SCREEN_SET_UF)
			setValue = localRef.fFocusVolt;
		else if (actualScreen == SCREEN_SET_UP)
			setValue = localRef.fPumpVolt;

		/* rising edge on node A - check B level (test proven that here A is always high) */
		if (levelB == GPIO_PIN_SET)
		{
			//SPAM(("left\n"));
			//ITM_SendChar('L'); ITM_SendChar('\n');
			setValue -= setDigit;
		}
		else
		{
			//SPAM(("right\n"));
			//ITM_SendChar('R'); ITM_SendChar('\n');
			setValue += setDigit;
		}

		/* save uint to float */
		if (actualScreen == SCREEN_SET_IA)
		{
			if (setValue < 500)		// 50 uA limit
			{
				localRef.uAnodeCurrent = setValue;
				localRef.fAnodeCurrent = ((float)(localRef.uAnodeCurrent))/(1e+7);
			}
		}
		else if (actualScreen == SCREEN_SET_UC)
		{
			if (setValue < 5001)	// 5 kV limit
				localRef.fCathodeVolt = (float)setValue;
		}
		else if (actualScreen == SCREEN_SET_UF)
		{
			if (setValue < 5001)	// 5 kV limit
				localRef.fFocusVolt = (float)setValue;
		}
		else if (actualScreen == SCREEN_SET_UP)
		{
			if (setValue < 5001)	// 5 kV limit
				localRef.fPumpVolt = (float)setValue;
		}
	}
//	else
//	{
//		//SPAM(("Turn too fast\n"));
//		ITM_SendChar('F'); ITM_SendChar('\n');
//	}

	lastTimeStamp = HAL_GetTick();
}



void encoderKnob_buttonCallback(void)
{
	if (HAL_GPIO_ReadPin(ENC_SW_GPIO_Port, ENC_SW_Pin) == GPIO_PIN_SET)
	{
//		SPAM(("Enc released\n"));
		// increase digit position
		setDigit *= 10;

		// wrap digit position (depends on screen)
		if (actualScreen == SCREEN_SET_IA)
		{
			if (setDigit > 100)	// 10 uA resolution
				setDigit = 1;	// 0.1 uA resolution
		}
		else if (  (actualScreen == SCREEN_SET_UC)
				|| (actualScreen == SCREEN_SET_UF)
				|| (actualScreen == SCREEN_SET_UP) )
		{
			if (setDigit > 1000)	// 1000 V resolution
				setDigit = 1;	// 1 V resolution
		}
		uKeysPressedTime[KEY_ENC] = 0;
	}
	else
	{
//		SPAM(("Enc pressed\n"));
		uKeysPressedTime[KEY_ENC] = 1;
	}
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
