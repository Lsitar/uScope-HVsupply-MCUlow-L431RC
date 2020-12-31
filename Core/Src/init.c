/*
 * init.c
 *
 *  Created on: Dec 19, 2020
 *      Author: lukasz
 */

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

	System.bHighSideShutdown = true;
	System.bCommunicationOk = false;
//	System.ref.fCathodeVolt = -900.0f;

	// output PWM
	pwmSetDuty(PWM_CHANNEL_UC, 0.0f);
	pwmSetDuty(PWM_CHANNEL_UE, 0.0f);
	pwmSetDuty(PWM_CHANNEL_UF, 0.0f);
	pwmSetDuty(PWM_CHANNEL_PUMP, 0.0f);

	HAL_TIMEx_PWMN_Start(&htim1, PWM_CHANNEL_UC);
	HAL_TIM_PWM_Start(&htim1, PWM_CHANNEL_UE);
	HAL_TIM_PWM_Start(&htim1, PWM_CHANNEL_UF);
	HAL_TIM_PWM_Start(&htim1, PWM_CHANNEL_PUMP);

	// PID regulator - init when High Side MCU 'll be running
//	regulatorInit();
//	PIDSetpointSet(&pidUc, 0.0f);
//	PIDSetpointSet(&pidUe, 0.0f);
//	PIDSetpointSet(&pidUf, 0.0f);
//	HAL_TIM_Base_Start_IT(&htim6);

#ifdef CUSTOM_RX
//	uartCustomRxInit();	// this not now, but at High side start
#endif

	// display module, screen variables
	uiInit();

//	HAL_StatusTypeDef i2cStatus = HAL_I2C_IsDeviceReady(&hi2c1, PCF8574_ADDR_WRITE, 3, 100);
//	if (i2cStatus == HAL_OK)
//	{
//		SPAM(("I2C_ready\n"));
//		HD44780_Init(20, 4);
//	}
//	else
//	{
//		SPAM(("I2C_LCD_error\n"));
//		ledError(5);
//		powerLockOff();
//		while (0xDEAD);
//	}
//	uiScreenChange(SCREEN_POWERON_1);

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
