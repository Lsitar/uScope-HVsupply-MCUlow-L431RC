/*
 * init.c
 *
 *  Created on: Dec 19, 2020
 *      Author: lukasz
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
	memset(&System, 0x00, sizeof (System));

	initCoefficients();

#ifdef MCU_HIGH

	InitADC();

#else
	powerLockOn();
	System.bHighSidePowered = false;
	System.bCommunicationOk = false;

#ifdef USE_MOVAVG_IA_FILTER
	movAvgInit();
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

	memcpy(&System.sweepResult, &System.meas, sizeof(struct sRegulatedVal));

	// TODO here may load previous settings from flash
	System.ref.extMode = EXT_REGULATE_IA;
	System.ref.uAnodeCurrent = 5;
	System.ref.fAnodeCurrent = 0.0f;
	System.ref.fCathodeVolt = -2500.0f;
	System.ref.fExtractVolt = 0.0f;
	System.ref.fExtractVoltIaRef = 0.0f;
	System.ref.fExtractVoltUserRef = 0.0f;
	System.ref.fExtractVoltLimit = 400.0f;
	System.ref.fFocusVolt = 0.0f;
	System.ref.fPumpVolt = 0.0f;

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

//	init_adc12();

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
