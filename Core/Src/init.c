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
#include "utilities.h"

#include "ads131m0x.h"






//void init_variables( void )
//{
//	memset( (void*)&System, 0x00, sizeof(struct s_System) );
//
////	for(int i = 0; i < FILTER_LENGTH_ADC; i++)
////	{
////		MovAvgI[0].buff[i] = 2000;
////	}
////	MovAvgI[0].sum = 2000 * FILTER_LENGTH_ADC;
////	MovAvgI[0].pos = 0;
////	MovAvgI[0].AvgOutput = 2000;
//
//	return;
//}



/*
 * Entry function for initialising peripherals.
 * Should be called before main().
 */
void init_user( void )
{
	powerLockOn();

	// output PWM
	pwmSetDuty(PWM_CHANNEL_UK, (1.0/3.3));
	pwmSetDuty(PWM_CHANNEL_UE, (1.4142135/3.3));
	pwmSetDuty(PWM_CHANNEL_UF, (2.71/3.3));
	pwmSetDuty(PWM_CHANNEL_PUMP, (3.1415/3.3));

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

	InitADC();


//	init_adc12();

//	init_usart3();

//	encoderKnob_init();		/* use EXTI10 and EXTI11 */

	return;
}



/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
