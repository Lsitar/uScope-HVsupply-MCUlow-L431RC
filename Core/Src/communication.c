/*
 * communication.c
 *
 *  Created on: Dec 24, 2020
 *      Author: lukasz
 */

#include <string.h>
#include "communication.h"
#include "main.h"		// for uart handle
#include "stm32l4xx_hal.h"
#include "typedefs.h"
#include "utilities.h"

/* Private variables ---------------------------------------------------------*/

static const uint32_t password = 0xC0CAC01A;

static bool bIdle = true;

/* Global variables ----------------------------------------------------------*/

union uCommFrame commFrame;

/* Exported functions --------------------------------------------------------*/

void sendResults(void)
{
	bIdle = false;
	// Password is not for error checking, but for frame begin marker
	memcpy(&commFrame.data.password, &password, sizeof (uint32_t));
	memcpy(&commFrame.data.fExtVolt, &System.meas.fExtractVolt, sizeof (float));
	memcpy(&commFrame.data.fFocusVolt, &System.meas.fFocusVolt, sizeof (float));
	HAL_UART_Transmit_IT(&huart1, commFrame.uartRxBuff, sizeof (struct sCommFrame));
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	UNUSED(huart);
	bIdle = true;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
#ifdef MCU_HIGH
	return;
#else
	static uint32_t uTimeTick;

	if (HAL_GetTick() - uTimeTick > 100)
	{
		uTimeTick = HAL_GetTick();
		ledGreen(TOGGLE);
	}

	if (commFrame.data.password != password)
		ledRed(ON);
	else
		ledRed(OFF);

	memcpy(&System.meas.fExtractVolt, &commFrame.data.fExtVolt, sizeof(float));
	memcpy(&System.meas.fFocusVolt, &commFrame.data.fFocusVolt, sizeof(float));

	// receive next byte
	HAL_UART_Receive_IT(&huart1, &(commFrame.uartRxBuff[0]), sizeof(struct sCommFrame));
#endif // MCU_HIGH
}

bool uartIsIdle(void)
{
	return bIdle;
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
