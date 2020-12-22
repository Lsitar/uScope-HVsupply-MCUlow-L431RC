/*
 * init_and_end.c
 *
 *  Created on: Nov 19, 2018
 *      Author: lukasz
 */


#include <string.h>	// memset()
#include <stdbool.h>
#include "main.h"
#include "init.h"
//#include "encoderKnob.h"
//#include "i2c.h"
#include "hd44780_i2c.h"
#include "typedefs.h"
#include "ui.h"
#include "utilities.h"

#include "ads131m0x.h"



/*
 * Entry function for initialising peripherals.
 * Should be called before main().
 */
void init_user( void )
{
	powerLockOn();

	memset(&System, 0x00, sizeof (System));

	// output PWM
	pwmSetDuty(PWM_CHANNEL_UK, (1.0/3.3));
	pwmSetDuty(PWM_CHANNEL_UE, 0.0f);
	pwmSetDuty(PWM_CHANNEL_UF, 0.0f);
	pwmSetDuty(PWM_CHANNEL_PUMP, 0.0f);

	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

	// display module
	HAL_StatusTypeDef i2cStatus = HAL_I2C_IsDeviceReady(&hi2c1, PCF8574_ADDR_WRITE, 3, 100);

	if (i2cStatus == HAL_OK)
	{
		SPAM(("I2C_LCD_ready\n"));
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

	InitADC();


//	init_adc12();

//	init_usart3();

//	encoderKnob_init();		/* use EXTI10 and EXTI11 */

	// show second screen
	while (uiGetScreenTime() < 1500) __NOP();
	uiScreenChange(SCREEN_POWERON_2);
	while (uiGetScreenTime() < 1500) __NOP();
	uiScreenChange(SCREEN_1);

	return;
}



/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
