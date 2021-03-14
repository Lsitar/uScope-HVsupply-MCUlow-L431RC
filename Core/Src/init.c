/*
 * init.c
 *
 *  Created on: Dec 19, 2020
 *      Author: Lukasz Sitarek
 */

#include <math.h>	// NAN
#include <string.h>	// memset()
#include <stdbool.h>
#include "ads131m0x.h"
#include "calibration.h"
#include "communication.h"
#include "hd44780_i2c.h"
#include "init.h"
#include "main.h"
#include "pid_controller.h"
#include "regulator.h"
#include "typedefs.h"
#include "ui.h"
#include "utilities.h"



/*
 * User init functions after HAL peripherals are inited.
 * Should be called before main() loop.
 */
void init_user( void )
{
	memset(&System, 0x00, sizeof(System));

	initCoefficients();

	HAL_ADC_Start_IT(&hadc1);

#ifdef MCU_HIGH

	#ifdef USE_MOVAVG_UE_MCUHIGH
		movAvgInit(&movAvgUe);
	#endif
	#ifdef USE_MOVAVG_UF_MCUHIGH
		movAvgInit(&movAvgUf);
	#endif
	InitADC();

#else // MCU_LOW

	powerLockOn();
	System.bHighSidePowered = false;
	System.bCommunicationOk = false;
	System.bLoggerOn = false;
	System.bLowBatt = false;
	System.bSweepOn = false;

		movAvgInit(&movAvgAdcBatt);
	#ifdef USE_MOVAVG_IA_FILTER
		movAvgInit(&movAvgIa);
	#endif
	#ifdef USE_MOVAVG_UC_FILTER
		movAvgInit(&movAvgUc);
	#endif
	#ifdef USE_MOVAVG_UE_MCULOW
		movAvgInit(&movAvgUe);
	#endif
	#ifdef USE_MOVAVG_UF_MCULOW
		movAvgInit(&movAvgUf);
	#endif

	System.meas.uAnodeCurrent = 0;
	System.meas.fAnodeCurrent = NAN;
	System.meas.fCathodeVolt = NAN;
	System.meas.fFocusVolt = NAN;
	System.meas.fPumpVolt = NAN;
	System.meas.fExtractVolt = NAN;
	System.meas.fExtractVoltIaRef = NAN;
	System.meas.fExtractVoltUserRef = NAN;
	System.meas.fExtractVoltLimit = NAN;
	System.meas.extMode = EXT_REGULATE_IA;
	System.meas.loggerMode = LOGGER_IA_UE_UF;

	memcpy(&System.sweepResult, &System.meas, sizeof(tsRegulatedVal));

	if (flashReadConfig())
	{	// error loading settings from flash - set default
		SPAM(("Settings read error! load default\n"));
		System.ref.extMode = EXT_REGULATE_IA;
		System.ref.uAnodeCurrent = 60;
		System.ref.fAnodeCurrent = 6e-7;
		System.ref.fCathodeVolt = -2500.0f;
		System.ref.fExtractVolt = 0.0f;
		System.ref.fExtractVoltIaRef = 0.0f;
		System.ref.fExtractVoltUserRef = 0.0f;
		System.ref.fExtractVoltLimit = 500.0f;
		System.ref.fFocusVolt = 0.0f;
		System.ref.fPumpVolt = 0.0f;
	}
	else
	{	// settings from flash loaded - apply
		memcpy(&System.ref, uFlashData.buff8, sizeof(tsRegulatedVal));
		SPAM(("Settings loaded from flash\n"));
	}

	// output PWM
	pwmSetDuty(PWM_CHANNEL_UC, 0.0f);
	pwmSetDuty(PWM_CHANNEL_UE, 0.0f);
	pwmSetDuty(PWM_CHANNEL_UF, 0.0f);
	pwmSetDuty(PWM_CHANNEL_PUMP, 0.0f);

	HAL_TIMEx_PWMN_Start(&htim1, PWM_CHANNEL_UC);
	HAL_TIM_PWM_Start(&htim1, PWM_CHANNEL_UE);
	HAL_TIM_PWM_Start(&htim1, PWM_CHANNEL_UF);
	HAL_TIM_PWM_Start(&htim1, PWM_CHANNEL_PUMP);

	// PID regulator - not now, init when High Side MCU 'll be running.

	// display module, screen variables
	uiInit();

	InitADC();

//	// UART - trigger first receiving
//	HAL_UART_Receive_IT(&huart1, &(commFrame.uartRxBuff[0]), sizeof(struct sCommFrame));

	// show second screen
	while (uiGetScreenTime() < 1500) __NOP();
	uiScreenChange(SCREEN_POWERON_2);
	while (uiGetScreenTime() < 1500) __NOP();
	uiScreenChange(SCREEN_1);

#endif // MCU_HIGH
	return;
}



/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
