/*
 * gui.c
 *
 *  Created on: Dec 21, 2020
 *      Author: lukasz
 */
#include <string.h>
#include "communication.h"
#include "hd44780_i2c.h"
#include "main.h"
#include "math.h"
//#include "pid_controller.h"
#include "printf.h"
#include "regulator.h"
#include "typedefs.h"
#include "ui.h"
#include "utilities.h"

/* Private variables ---------------------------------------------------------*/

static char LCD_buff[20];				// working buffer for snprintf
static enum eScreen actualScreen;
static enum eScreen returnScreen;		// save screen to return to - f.ex. from settings
static enum eScreen settingsScreenGr1;	// save last settings screen (group 1) - to enter always last setted value
static enum eScreen settingsScreenGr2;	// save last settings screen (group 2) - to enter always last setted value
static uint32_t uScreenTimer;			// measure time from last screenChange
static bool bBlink;						// helper variable for blinking text on screen
static int printedCharsLine[4] = {0};	// save number of value digits plotted in each line

static struct sRegulatedVal localRef;	// local copy of values changed at settings
static int32_t setDigit = 1;

/* config / constants --------------------------------------------------------*/

#define KB_READ_INTERVAL		50	// ms
#define KB_PRESSED_THRESHOLD	2	// ticks ca. 100 ms
#define KB_POWEROFF_THRESHOLD	40	// ticks ca. 2 s
#define LCD_UPDATERATE_MS		250	// 4 Hz
#define LCD_BLINK_TIME			333	// ms per state. NOTE: possible blinking pe-
									// riod is limited down to LCD_UPDATERATE_MS



#define IS_SETTINGS_SCREEN_GROUP_1	(  (actualScreen == SCREEN_SET_UC)   \
									|| (actualScreen == SCREEN_SET_IA)   \
									|| (actualScreen == SCREEN_SET_UF)   \
									|| (actualScreen == SCREEN_SET_UP))  \

#define IS_SETTINGS_SCREEN_GROUP_2	(  (actualScreen == SCREEN_SET_UE)		\
									|| (actualScreen == SCREEN_SET_UEMAX)   \
									|| (actualScreen == SCREEN_SET_UEMODE))	\

#define IS_SETTINGS_SCREEN		( IS_SETTINGS_SCREEN_GROUP_1 || IS_SETTINGS_SCREEN_GROUP_2 )

/* Private types -------------------------------------------------------------*/

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
static bool encoderSwReleased;

/* Private functions ---------------------------------------------------------*/

static void _changeScreenLeftRight(enum eKey key)
{
	// Switch MAIN screens /////////////////////////////////////////////////////
	if (actualScreen == SCREEN_1)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SWEEP_UE);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_2);
	}
	else if (actualScreen == SCREEN_2)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_1);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SWEEP_UE);
	}
	else if (actualScreen == SCREEN_SWEEP_UE)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_2);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_1);
	}

	// Switch SETTINGS group 1 screens /////////////////////////////////////////
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

	// Switch SETTINGS group 2 screens /////////////////////////////////////////
	else if (actualScreen == SCREEN_SET_UE)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UEMODE);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UEMAX);
	}
	else if (actualScreen == SCREEN_SET_UEMAX)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UE);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UEMODE);
	}
	else if (actualScreen == SCREEN_SET_UEMODE)
	{
		if (key == KEY_LEFT)
			uiScreenChange(SCREEN_SET_UEMAX);
		else if (key == KEY_RIGHT)
			uiScreenChange(SCREEN_SET_UE);
	}

//	if (IS_SETTINGS_SCREEN)
//		setDigit = 1;
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



static void _clearField(int8_t x, uint8_t y, uint8_t len)
{
	char buff[len+1];

	for (uint32_t i=0; i<len; i++)
		buff[i] = ' ';

	buff[len] = '\0';

	HD44780_Puts(x, y, buff);
}



/*
 * Used to blink text in settings.
 * NOTE: the texts blinked now are the same length. Function was not tested for
 * 		switching between different length texts.
 */
static inline void _blinkText(int8_t x, uint8_t y, char* text)
{
	// the global printedCharsLine[] is for numbers. For this texts use local variable for now.
	static uint32_t printedLength = 0;

	if (_blinkTimer() == true)
	{
		if (bBlink == true)
			_clearField(x, y, printedLength);
		else
			printedLength = HD44780_Puts(x, y, text);
	}
}



static int32_t _printVoltage(float voltage, char* buff, uint8_t buffSize)
{
	float absVoltage = fabsf(voltage);

	if (isnanf(voltage))
		return snprintf_(buff, buffSize, "----- V");
	else if (absVoltage < 100.0f)
		return snprintf_(buff, buffSize, "%.2f V", voltage);
	else if (absVoltage < 1000.0f)
		return snprintf_(buff, buffSize, "%.1f V", voltage);
	else
		return snprintf_(buff, buffSize, "%.0f V", voltage);
}



static int32_t _printCurrent(float current, char* buff, uint8_t buffSize)
{
	current = fabsf(current * 1000000.0f);	// convert to uA

	if (isnanf(current))
		return snprintf_(buff, buffSize, "----- uA");
	else if (current < 49.0f)
		return snprintf_(buff, buffSize, "%.2f uA", current);
	else
		return snprintf_(buff, buffSize, ">49.0 uA", current);
}



static int32_t _printPower(float power, char* buff, uint8_t buffSize)
{
	power = fabsf(power * 1000.0f);

	if (power < 10.0f)
		return snprintf_(buff, buffSize, "%.2f mW", power);
	else if (power < 100.0f)
		return snprintf_(buff, buffSize, "%.1f mW", power);
	else
		return snprintf_(buff, buffSize, "%.0f mW", power);
}

/* Exported functions --------------------------------------------------------*/

void uiInit(void)
{
	// init variables
	settingsScreenGr1 = SCREEN_SET_UC;
	settingsScreenGr2 = SCREEN_SET_UE;
	returnScreen = SCREEN_1;

	// init LCD on I2C interface
	HAL_StatusTypeDef i2cStatus = HAL_I2C_IsDeviceReady(&hi2c1, PCF8574_ADDR_WRITE, 3, 100);
	if (i2cStatus == HAL_OK)
	{
		SPAM(("I2C_ready\n"));
		HD44780_Init(20, 4);
	}
	else
	{
		SPAM(("I2C_LCD_error\n"));
		ledError(5);
		powerLockOff();
		while (0xDEAD);
	}
	uiScreenChange(SCREEN_POWERON_1);
}



uint32_t uiGetScreenTime(void)
{
	return HAL_GetTick() - uScreenTimer;
}



void uiScreenChange(enum eScreen newScreen)
{
	HD44780_Clear();

	memset(printedCharsLine,0x00,sizeof(printedCharsLine));

	switch (newScreen)
	{
	case SCREEN_RESET:
		HAL_Delay(10);	// give some time to send rest from buffer
		HD44780_Init(20, 4);
		break;

	case SCREEN_1:
		HD44780_Puts(5, 0, "UC:");		// line 1 - Cathode voltage
		HD44780_Puts(5, 1, "IA:");		// line 2 - Cathode current
		HD44780_Puts(1, 2, "PowerA:");	// line 3 - Cathode power
		HD44780_Puts(0, 3, "Battery:");	// line 4 - Battery SoC
		// don't need to print values here, all 'll be refreshed later
		break;

	case SCREEN_2:
		HD44780_Puts(5, 0, "UE:");		// line 1 - Extract voltage
		HD44780_Puts(1, 1, "Ufocus:");	// line 2 - Focus voltage
		HD44780_Puts(2, 2, "Upump:");	// line 3 - Pump voltage
		HD44780_Puts(0, 3, "Battery:");	// line 4 - Battery SoC
		// don't need to print values here, all 'll be refreshed later
		break;

	case SCREEN_SWEEP_UE:
		HD44780_Puts(5, 0, "UE:");		// line 1 - Extract voltage
		HD44780_Puts(5, 1, "IA:");		// line 2 - Cathode current
		HD44780_Puts(0, 2, "Peak:");	// line 3 - Peak voltage
		HD44780_Puts(0, 3, "Peak:");	// line 4 - Peak current
		// don't need to print values here, all 'll be refreshed later
		break;

	case SCREEN_SET_UC:
	case SCREEN_SET_IA:
	case SCREEN_SET_UF:
	case SCREEN_SET_UP:
		HD44780_Puts(0, 0, "SET UC:");
		HD44780_Puts(0, 1, "SET IA:");
		HD44780_Puts(0, 2, "SET UF:");
		HD44780_Puts(0, 3, "SET UP:");
		// Need to print all values here, beacouse only one 'll be refreshed later.
		// print UC
		printedCharsLine[0] = snprintf_(LCD_buff, 9, "%.0f V", (localRef.fCathodeVolt) );
		HD44780_Puts(10, 0, LCD_buff);
		// print IA
		printedCharsLine[1] = snprintf_(LCD_buff, 9, "%.1f uA", (localRef.fAnodeCurrent * 1e6) );
		HD44780_Puts(10, 1, LCD_buff);
		// print UF
		printedCharsLine[2] = snprintf_(LCD_buff, 9, "%.0f V", (localRef.fFocusVolt) );
		HD44780_Puts(10, 2, LCD_buff);
		// print UP
		printedCharsLine[3] = snprintf_(LCD_buff, 9, "%.0f V", (localRef.fPumpVolt) );
		HD44780_Puts(10, 3, LCD_buff);
		// correct blinking period
		if (bBlink == true)
		{
			uint8_t row = 4;
			if (newScreen == SCREEN_SET_UC) row = 0;
			else if (newScreen == SCREEN_SET_IA) row = 1;
			else if (newScreen == SCREEN_SET_UF) row = 2;
			else if (newScreen == SCREEN_SET_UP) row = 3;
			_clearField(0, row, 7);
		}
		break;

	case SCREEN_SET_UE:
	case SCREEN_SET_UEMAX:
	case SCREEN_SET_UEMODE:
		HD44780_Puts(0, 0, "SET UE:");
		HD44780_Puts(0, 1, "UE Limit:");
		HD44780_Puts(0, 2, "UE Mode:");
		// Need to print all values here, beacouse only one 'll be refreshed later.
		// print UE
		printedCharsLine[0] = snprintf_(LCD_buff, 9, "%.0f V", (localRef.fExtractVolt) );
		HD44780_Puts(10, 0, LCD_buff);
		// print UE Limit
		printedCharsLine[1] = snprintf_(LCD_buff, 9, "%.0f V", (localRef.fExtractVoltLimit) );
		HD44780_Puts(10, 1, LCD_buff);
		// print MODE
		if (localRef.extMode == EXT_REGULATE)
			printedCharsLine[2] = snprintf_(LCD_buff, 9, "REGULATE");
		else if (localRef.extMode == EXT_STEADY)
			printedCharsLine[2] = snprintf_(LCD_buff, 9, "STEADY");
		else if (localRef.extMode == EXT_SWEEP)
			printedCharsLine[2] = snprintf_(LCD_buff, 9, "SWEEP");
		HD44780_Puts(10, 2, LCD_buff);
		// correct blinking period
		if (bBlink == true)
		{
			uint8_t row = 4;
			if (newScreen == SCREEN_SET_UE) row = 0;
			else if (newScreen == SCREEN_SET_UEMAX) row = 1;
			else if (newScreen == SCREEN_SET_UEMODE) row = 2;
			_clearField(0, row, 7);
		}
		break;

	case SCREEN_POWERON_1:
		HD44780_Puts(2, 0, "Microscope supply");	// 17 chars
		HD44780_Puts(4, 1, "Politechnika");			// 12 chars
		HD44780_Puts(5, 2, "Wroclawska");			// 10 chars
		HD44780_Puts(5, 3, "W11  WEMiF");			// 9 chars
		break;

	case SCREEN_POWERON_2:
		HD44780_Puts(2, 0, "Microscope supply");	// 17 chars
		HD44780_Puts(3, 2, "Lukasz Sitarek");		// 14 chars
		HD44780_Puts(8, 3, "2021");					// 4 chars
		break;

	case SCREEN_POWEROFF:
		HD44780_Puts(5, 2, "Power off");
		break;
	}
	actualScreen = newScreen;
	uScreenTimer = HAL_GetTick();
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

	if (HAL_GetTick() - uTimeTick > LCD_UPDATERATE_MS)
	{
		uTimeTick += LCD_UPDATERATE_MS;

		switch (actualScreen)
		{
		case SCREEN_RESET:
			break;

		case SCREEN_1:
		{
			// line 1 - Cathode voltage
			_clearField(9, 0, printedCharsLine[0]);
			printedCharsLine[0] = _printVoltage(System.meas.fCathodeVolt, LCD_buff, 11);
			HD44780_Puts(9, 0, LCD_buff);
			// line 2 - Anode current
			_clearField(9, 1, printedCharsLine[1]);
			printedCharsLine[1] = _printCurrent(System.meas.fAnodeCurrent, LCD_buff, 11);
			HD44780_Puts(9, 1, LCD_buff);
			// line 3 - Anode power
			_clearField(9, 2, printedCharsLine[2]);
			printedCharsLine[2] = _printPower((System.meas.fCathodeVolt * System.meas.fAnodeCurrent), LCD_buff, 11);
			HD44780_Puts(9, 2, LCD_buff);
			// line 4 - Battery SoC TODO
			_clearField(9, 0, printedCharsLine[3]);
			break;
		}
		case SCREEN_2: // Ue Uf Up
			// line 1 - Extract voltage
			_clearField(9, 0, printedCharsLine[0]);
			printedCharsLine[0] = _printVoltage(System.meas.fExtractVolt, LCD_buff, 11);
			HD44780_Puts(9, 0, LCD_buff);
			// line 2 - Focus voltage
			_clearField(9, 1, printedCharsLine[1]);
			printedCharsLine[1] = _printVoltage(System.meas.fFocusVolt, LCD_buff, 11);
			HD44780_Puts(9, 1, LCD_buff);
			// line 3 - Pump voltage - print ref value, there's no measurement
			_clearField(9, 2, printedCharsLine[2]);
			if (System.bHighSidePowered == true)
				printedCharsLine[2] = _printVoltage(System.ref.fPumpVolt, LCD_buff, 11);
			else
				printedCharsLine[2] = _printVoltage(NAN, LCD_buff, 11);
			HD44780_Puts(9, 2, LCD_buff);
			// line 4 - Battery SoC TODO
			_clearField(9, 3, printedCharsLine[3]);
			break;

		case SCREEN_SWEEP_UE:
			// line 1 - Extract voltage
			_clearField(9, 0, printedCharsLine[0]);
			printedCharsLine[0] = _printVoltage(System.meas.fExtractVolt, LCD_buff, 11);
			HD44780_Puts(9, 0, LCD_buff);
			// line 2 - Anode current
			_clearField(9, 1, printedCharsLine[1]);
			printedCharsLine[1] = _printCurrent(System.meas.fAnodeCurrent, LCD_buff, 11);
			HD44780_Puts(9, 1, LCD_buff);
			// line 3 - Peak voltage
			_clearField(9, 2, printedCharsLine[2]);
			printedCharsLine[2] = _printVoltage(System.sweepResult.fExtractVolt, LCD_buff, 11);
			HD44780_Puts(9, 2, LCD_buff);
			// line 4 - Peak current
			_clearField(9, 3, printedCharsLine[3]);
			printedCharsLine[3] = _printCurrent(System.sweepResult.fAnodeCurrent, LCD_buff, 11);
			HD44780_Puts(9, 3, LCD_buff);
			break;

		// settings group 1 ////////////////////////////////////////////////////
		case SCREEN_SET_UC:
			_blinkText(0, 0, "SET UC:");
			_clearField(10, 0, printedCharsLine[0]);
			printedCharsLine[0] = snprintf_(LCD_buff, 10, "%.0f V", (localRef.fCathodeVolt) );
			HD44780_Puts(10, 0, LCD_buff);
			break;

		case SCREEN_SET_IA:
			_blinkText(0, 1, "SET IA:");
			_clearField(10, 1, printedCharsLine[1]);
			printedCharsLine[1] = snprintf_(LCD_buff, 10, "%.1f uA", (localRef.fAnodeCurrent * 1e6) );
			HD44780_Puts(10, 1, LCD_buff);
			break;

		case SCREEN_SET_UF:
			_blinkText(0, 2, "SET UF:");
			_clearField(10, 2, printedCharsLine[2]);
			printedCharsLine[2] = snprintf_(LCD_buff, 10, "%.0f V", (localRef.fFocusVolt) );
			HD44780_Puts(10, 2, LCD_buff);
			break;

		case SCREEN_SET_UP:
			_blinkText(0, 3, "SET UP:");
			_clearField(10, 3, printedCharsLine[3]);
			printedCharsLine[3] = snprintf_(LCD_buff, 10, "%.0f V", (localRef.fPumpVolt) );
			HD44780_Puts(10, 3, LCD_buff);
			break;

		// settings group 2 ////////////////////////////////////////////////////
		case SCREEN_SET_UE:
			_blinkText(0, 0, "SET UE:");
			_clearField(10, 0, printedCharsLine[0]);
			printedCharsLine[0] = snprintf_(LCD_buff, 10, "%.0f V", (localRef.fExtractVolt) );
			HD44780_Puts(10, 0, LCD_buff);
			break;

		case SCREEN_SET_UEMAX:
			_blinkText(0, 1, "UE Limit:");
			_clearField(10, 1, printedCharsLine[1]);
			printedCharsLine[1] = snprintf_(LCD_buff, 10, "%.0f V", (localRef.fExtractVoltLimit) );
			HD44780_Puts(10, 1, LCD_buff);
			break;

		case SCREEN_SET_UEMODE:
			_blinkText(0, 2, "UE Mode:");
			_clearField(10, 2, printedCharsLine[2]);
			if (localRef.extMode == EXT_REGULATE)
				printedCharsLine[2] = snprintf_(LCD_buff, 9, "REGULATE");
			else if (localRef.extMode == EXT_STEADY)
				printedCharsLine[2] = snprintf_(LCD_buff, 9, "STEADY");
			else if (localRef.extMode == EXT_SWEEP)
				printedCharsLine[2] = snprintf_(LCD_buff, 9, "SWEEP");
			HD44780_Puts(10, 2, LCD_buff);
			break;

		case SCREEN_POWERON_1:
		case SCREEN_POWERON_2:
		case SCREEN_POWEROFF:
			break;
		}
	}
}



/*
 * Call in main loop.
 * Execution period (KB read) is controlled internally (50 ms period now).
 */
void keyboardRoutine(void)
{
	static uint32_t uTimeTick = 0;
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
				HD44780_Clear();
				HD44780_Puts(5, 2, "Power off");
				highSideShutdown();
				powerLockOff();
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
					returnScreen = actualScreen;
					//setDigit = 1;
					if (actualScreen == SCREEN_SWEEP_UE)
						uiScreenChange(settingsScreenGr2);
					else
						uiScreenChange(settingsScreenGr1);
				}
				else
				{	// settings confirmed
					memcpy(&System.ref, &localRef, sizeof(System.ref));
					if (IS_SETTINGS_SCREEN_GROUP_1)
						settingsScreenGr1 = actualScreen;
					else
						settingsScreenGr2 = actualScreen;

					uiScreenChange(returnScreen);
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
				{	// settings abandoned
					if (IS_SETTINGS_SCREEN_GROUP_1)
						settingsScreenGr1 = actualScreen;
					else
						settingsScreenGr2 = actualScreen;

					uiScreenChange(returnScreen);
				}
				else
				{	// reset HD44780 controller
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
		if (uKeysPressedTime[KEY_ENC] != 0)	// this variable will be triggered in corresponding EXTI interrupt
		{
			if (encoderSwReleased)
			{
				if (uKeysPressedTime[KEY_ENC] >= KB_PRESSED_THRESHOLD)
				{
					if (IS_SETTINGS_SCREEN)
					{
						// increase digit position
						setDigit *= 10;

						// wrap digit position (depends on screen)
						if (actualScreen == SCREEN_SET_IA)
						{
							if (setDigit > 100)	// 10 uA resolution
								setDigit = 1;	// 0.1 uA resolution
						}
						else
						{
							if (setDigit > 1000)	// 1000 V resolution
								setDigit = 1;	// 1 V resolution
						}
					}
					else if (actualScreen == SCREEN_SWEEP_UE)
					{
						if (System.bSweepOn == false)
						{
							if (System.bHighSidePowered)
								sweepUeInit();
						}
						else
						{
							sweepUeExit(false);
						}
					}
				}
				uKeysPressedTime[KEY_ENC] = 0;
				encoderSwReleased = false;
			}
			else
			{	// count pressed key till it reach hold time
				uKeysPressedTime[KEY_ENC]++;

				if (uKeysPressedTime[KEY_ENC] >= KB_POWEROFF_THRESHOLD)
				{
					if (HAL_GPIO_ReadPin(TP32_GPIO_Port, TP32_Pin) == GPIO_PIN_RESET)	// HV enable switch - toggle
					{
						if (!IS_SETTINGS_SCREEN)
							highSideStart();
					}
					else
						highSideShutdown();

					uKeysPressedTime[KEY_ENC] = 0;	// finish pressed counting
				}
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
	static int32_t setValue;
	static uint32_t lastTimeStamp;
	GPIO_PinState levelA, levelB;
	UNUSED(levelA);

	// read pin states ASAP with minimum delay between them
//	levelA = HAL_GPIO_ReadPin(ENC_CHA_EXTI0_GPIO_Port, ENC_CHA_EXTI0_Pin);	// assume interrupt is only on one edge (here: rising)
	levelB = HAL_GPIO_ReadPin(ENC_CHB_GPIO_Port, ENC_CHB_Pin);

	if (HAL_GetTick() - lastTimeStamp > 4)
	{
		if (actualScreen == SCREEN_SET_UEMODE)
		{	// change enum
			if (levelB == GPIO_PIN_SET)
			{	// left
				if (localRef.extMode == EXT_REGULATE)
					localRef.extMode = EXT_SWEEP;
				else if (localRef.extMode == EXT_SWEEP)
					localRef.extMode = EXT_STEADY;
				else
					localRef.extMode = EXT_REGULATE;
			}
			else
			{	// right
				if (localRef.extMode == EXT_REGULATE)
					localRef.extMode = EXT_STEADY;
				else if (localRef.extMode == EXT_STEADY)
					localRef.extMode = EXT_SWEEP;
				else
					localRef.extMode = EXT_REGULATE;
			}
		}
		else
		{	// change numeric values
			/* load float value to uint */
			if (actualScreen == SCREEN_SET_IA)
				setValue = localRef.uAnodeCurrent;
			else if (actualScreen == SCREEN_SET_UC)
				setValue = localRef.fCathodeVolt;
			else if (actualScreen == SCREEN_SET_UF)
				setValue = localRef.fFocusVolt;
			else if (actualScreen == SCREEN_SET_UP)
				setValue = localRef.fPumpVolt;
			else if (actualScreen == SCREEN_SET_UE)
				setValue = localRef.fExtractVolt;
			else if (actualScreen == SCREEN_SET_UEMAX)
				setValue = localRef.fExtractVoltLimit;

			/* rising edge on node A - check B level (test proven that here A is always high) */
			if (levelB == GPIO_PIN_SET)
			{
				//ITM_SendChar('L'); ITM_SendChar('\n');
				setValue -= setDigit;
			}
			else
			{
				//ITM_SendChar('R'); ITM_SendChar('\n');
				setValue += setDigit;
			}

			/* save uint to float & check bounds */

			// settings group 1 ////////////////////////////////////////////////////
			if (actualScreen == SCREEN_SET_IA)
			{
				if ((setValue < 500)&&(setValue >= 0))		// 0 - 50 uA limit
				{
					localRef.uAnodeCurrent = setValue;
					localRef.fAnodeCurrent = ((float)(localRef.uAnodeCurrent))/(1e+7);
				}
			}
			else if (actualScreen == SCREEN_SET_UC)
			{
				//if (setValue < 5001)	// 5 kV limit
				if ((setValue > -5001) && (setValue <= 0))
					localRef.fCathodeVolt = (float)setValue;
			}
			else if (actualScreen == SCREEN_SET_UF)
			{
				if ((setValue < 5001)&&(setValue >= 0))//(setValue < 5001)	// 5 kV limit
					localRef.fFocusVolt = (float)setValue;
			}
			else if (actualScreen == SCREEN_SET_UP)
			{
				if ((setValue < 5001)&&(setValue >= 0))//(setValue < 5001)	// 5 kV limit
					localRef.fPumpVolt = (float)setValue;
			}

			// settings group 2 ////////////////////////////////////////////////////
			else if (actualScreen == SCREEN_SET_UE)
			{
				if ((setValue < 2501)&&(setValue >= 0))//(setValue < 5001)	// 2.5 kV limit
					localRef.fExtractVolt = (float)setValue;
			}
			else if (actualScreen == SCREEN_SET_UEMAX)
			{
				if ((setValue < 2501)&&(setValue >= 0))//(setValue < 5001)	// 2.5 kV limit
					localRef.fExtractVoltLimit = (float)setValue;
			}
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
		encoderSwReleased = true;
	}
	else
	{
//		SPAM(("Enc pressed\n"));

		/* Just init the variable and the rest will be done in keyboard routine
		 * as long as key is pressed.
		 */
		encoderSwReleased = false;
		uKeysPressedTime[KEY_ENC] = 1;
	}
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
