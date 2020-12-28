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

static uint32_t cntTriggered = 0;
static uint32_t cntReceived = 0;
static uint32_t cntSent = 0;

/* Global variables ----------------------------------------------------------*/

union uCommFrame commFrame;

/* Private functions ---------------------------------------------------------*/

static inline void _swapByteOrder(uint8_t *buff, uint8_t len)
{
	uint8_t temp[len];

	for (uint32_t i=0; i<len; i++)
	{
		temp[len - i - 1] = buff[i];
	}

	memcpy(buff, temp, len);
}

/* Exported functions --------------------------------------------------------*/

void sendResults(void)
{
	HAL_StatusTypeDef retVal;

	bIdle = false;
	// Password is not for error checking, but for frame begin marker
//	memcpy(&commFrame.data.password, &password, sizeof (uint32_t));
//	memcpy(&commFrame.data.fExtVolt, &System.meas.fExtractVolt, sizeof (float));
//	memcpy(&commFrame.data.fFocusVolt, &System.meas.fFocusVolt, sizeof (float));
	commFrame.data.password = password;
	commFrame.data.fExtVolt = System.meas.fExtractVolt;
	commFrame.data.fFocusVolt = System.meas.fFocusVolt;
	_swapByteOrder(commFrame.uartRxBuff, sizeof(struct sCommFrame));

	retVal = HAL_UART_Transmit_IT(&huart1, commFrame.uartRxBuff, sizeof (struct sCommFrame));
	if (retVal != HAL_OK)
	{
		ledOrange(ON);
	}
	else
	{
		cntTriggered++;
		ledOrange(OFF);
	}
//	if (cntTriggered % 100 == 0) SPAM(("Triggered: %u\n", cntTriggered));
}



/*
 * Trigger receiving fixed number of bytes.
 */
void uartReceiveFrameIT(void)
{
	HAL_StatusTypeDef retVal;
	retVal = HAL_UART_Receive_IT(&huart1, &(commFrame.uartRxBuff[0]), sizeof(struct sCommFrame));
	if (retVal != HAL_OK)
	{
		SPAM(("Rx init error: %u\n", retVal));
	}
}



void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
#ifdef MCU_HIGH
	static uint32_t uTimeTick;

	if (HAL_GetTick() - uTimeTick > 100)
	{
		uTimeTick = HAL_GetTick();
		ledGreen(TOGGLE);
	}

	cntSent++;

//	if (cntSent % 100 == 0) SPAM(("Sent: %u\n", cntSent));

	UNUSED(huart);
	bIdle = true;
#else // MCU_LOW
	SPAM(("MCU low Tx!\n"));
	return;
#endif
}



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
#ifdef MCU_HIGH
	SPAM(("MCU high Rx!\n"));
	return;
#else // MCU_LOW
	static uint32_t uTimeTick;

	if (HAL_GetTick() - uTimeTick > 100)
	{
		uTimeTick = HAL_GetTick();
		ledGreen(TOGGLE);
	}

	cntReceived++;

	if (cntReceived > 0 && (cntReceived % 2) == 0 )
		__NOP();

	if (commFrame.data.password != password)
	{
		ledRed(ON);
//		ITM_SendChar('x');
	}
	else
	{
		ledRed(OFF);
//		ITM_SendChar('y');
	}
//	ITM_SendChar('\n');

	memcpy(&System.meas.fExtractVolt, &commFrame.data.fExtVolt, sizeof(float));
	memcpy(&System.meas.fFocusVolt, &commFrame.data.fFocusVolt, sizeof(float));

	// receive next byte
	uartReceiveFrameIT();

#endif // MCU_HIGH
}


//#define  HAL_UART_ERROR_NONE             ((uint32_t)0x00000000U)    /*!< No error                */
//#define  HAL_UART_ERROR_PE               ((uint32_t)0x00000001U)    /*!< Parity error            */
//#define  HAL_UART_ERROR_NE               ((uint32_t)0x00000002U)    /*!< Noise error             */
//#define  HAL_UART_ERROR_FE               ((uint32_t)0x00000004U)    /*!< Frame error             */
//#define  HAL_UART_ERROR_ORE              ((uint32_t)0x00000008U)    /*!< Overrun error           */
//#define  HAL_UART_ERROR_DMA              ((uint32_t)0x00000010U)    /*!< DMA transfer error      */
//#define  HAL_UART_ERROR_RTO              ((uint32_t)0x00000020U)    /*!< Receiver Timeout error  */
//
//#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
//#define  HAL_UART_ERROR_INVALID_CALLBACK ((uint32_t)0x00000040U)    /*!< Invalid Callback error  */
//#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	UNUSED(huart);

	uint32_t errno = HAL_UART_GetError(huart);
	//SPAM(("UART error: 0x%X\n", errno));

	SPAM(("UART error: "));
	if (errno & HAL_UART_ERROR_PE)
		SPAM(("parity, "));
	if (errno & HAL_UART_ERROR_NE)
		SPAM(("noise, "));
	if (errno & HAL_UART_ERROR_FE)
		SPAM(("frame, "));
	if (errno & HAL_UART_ERROR_ORE)
		SPAM(("overrun, "));
	if (errno & HAL_UART_ERROR_DMA)
		SPAM(("dma, "));
	if (errno & HAL_UART_ERROR_RTO)
		SPAM(("timeout, "));

	SPAM((".\n"));

	if (errno >= HAL_UART_ERROR_FE)
	{
		HAL_UART_AbortReceive_IT(&huart1);	// returns always HAL_OK
	}
}


void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart)
{
	UNUSED(huart);
	HAL_StatusTypeDef retVal;

	if (System.bHighSideShutdown == false)
	{	// recovery from transmission error
		retVal = HAL_UART_Receive_IT(&huart1, &(commFrame.uartRxBuff[0]), sizeof(struct sCommFrame));
		if (retVal != HAL_OK)
		{
			SPAM(("Rx init after abort error: %u\n", retVal));
		}
		else
		{
			SPAM(("Rx restarted\n"));
		}
	}
	else
	{	// disabling uart
		SPAM(("UART stop.\n"));
	}


}



bool uartIsIdle(void)
{
	return bIdle;
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
