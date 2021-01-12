/*
 * communication.c
 *
 *  Created on: Dec 24, 2020
 *      Author: lukasz
 */

#include <string.h>
#include "calibration.h"
#include "communication.h"
#include "main.h"		// for uart handle
#include "regulator.h"
#include "stm32l4xx_hal.h"
#include "typedefs.h"
#include "utilities.h"

/* Private variables ---------------------------------------------------------*/

static bool bIdle = true;
static bool bLedSetByCommunication;

static uint32_t cntTriggered = 0;
static uint32_t cntReceived = 0;
static uint32_t cntSent = 0;

/* Global variables ----------------------------------------------------------*/

union uCommFrame commFrame;

/* Private functions ---------------------------------------------------------*/

//static inline void _swapByteOrder(uint8_t *buff, uint8_t len)
//{
//	uint8_t temp[len];
//
//	for (uint32_t i=0; i<len; i++)
//	{
//		temp[len - i - 1] = buff[i];
//	}
//
//	memcpy(buff, temp, len);
//}

/* Exported functions --------------------------------------------------------*/

_OPT_O3 uint8_t crc8(const uint8_t *data, uint32_t length)
{
	uint8_t crc = 0x00;
	uint8_t extract;
	uint8_t sum;

	for(uint32_t i=0; i<length; i++)
	{
		extract = *data;
		for (uint32_t tempI = 8; tempI; tempI--)
		{
			sum = (crc ^ extract) & 0x01;
			crc >>= 1;
			if (sum)
				crc ^= 0x8C;
			extract >>= 1;
		}
		data++;
	}
	return crc;
}



void sendResults(void)
{
	UNUSED(cntTriggered);
#ifdef MCU_HIGH
	HAL_StatusTypeDef retVal;

	bIdle = false;
	commFrame.data.values.fExtVolt = System.meas.fExtractVolt;
	commFrame.data.values.fFocusVolt = System.meas.fFocusVolt;
	commFrame.data.values.crc8 = crc8(commFrame.uartRxBuff, 2*sizeof(float));

	retVal = HAL_UART_Transmit_IT(&huart1, commFrame.uartRxBuff, sizeof(commFrame.data.values));
	if (retVal != HAL_OK)
	{
		ledRed(ON);
		bLedSetByCommunication = true;
	}
	else
	{
		cntTriggered++;
		if (bLedSetByCommunication)
		{
			ledRed(OFF);
			bLedSetByCommunication = false;
		}
	}
//	if (cntTriggered % 100 == 0) SPAM(("Triggered: %u\n", cntTriggered));
#else
	SPAM(("MCU low %s!\n", __func__));
#endif
	return;
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

#ifdef CUSTOM_RX
/*
 * Set necessary interrupts etc.
 */
void uartCustomRxInit(void)
{
	CLEAR_BIT(USART1->CR1, USART_CR1_UE);		// uart disable

	/* CR1 */
	CLEAR_BIT(USART1->CR1, USART_CR1_TE);		// transmitter disable
	CLEAR_BIT(USART1->CR1, USART_CR1_CMIE);		// character match interrupt disable
	CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);	// transmit interrupt disable
	CLEAR_BIT(USART1->CR1, USART_CR1_TCIE);		// transmission complete interrupt disable

	SET_BIT(USART1->CR1, USART_CR1_RXNEIE);		// Rx interrupt enable
	SET_BIT(USART1->CR1, USART_CR1_PEIE);		// parity error interrupt enable
	SET_BIT(USART1->CR1, USART_CR1_IDLEIE);		// Idle interrupt enable

	/* CR2 */
	CLEAR_BIT(USART1->CR2, USART_CR2_RTOEN);	// Rx timeout disable
	/* CR3 */
	CLEAR_BIT(USART1->CR3, USART_CR3_EIE);		// error interrupt disable (noise, frame, overrun)

	// clear interrupt flags
	USART1->ICR = USART_ICR_PECF + USART_ICR_FECF + USART_ICR_NECF + 		\
				USART_ICR_ORECF + USART_ICR_IDLECF + USART_ICR_TCCF + 		\
				USART_ICR_TCBGTCF + USART_ICR_LBDCF + USART_ICR_CTSCF + 	\
				USART_ICR_RTOCF + USART_ICR_EOBCF + USART_ICR_CMCF + USART_ICR_WUCF;

	SET_BIT(USART1->CR1, USART_CR1_UE);			// uart enable
}



void uartCustomRxTrigger(void)
{
	SET_BIT(USART1->CR1, USART_CR1_RE);	// enable Rx - begins to search start bit
}



void uartCustomIrqHandler(void)
{
	static uint32_t rxIndex = 0;

	uint32_t flags = USART1->ISR;

	// check & clear errors
	if (flags & (USART_ISR_PE + USART_ISR_FE + USART_ISR_NE + USART_ISR_ORE))
	{
		if (flags & USART_ISR_PE)
		{	// parity error
			USART1->ICR = USART_ICR_PECF;
			commFrame.data.bErrParity = true;
		}
		if (flags & USART_ISR_FE)
		{	// frame error
			USART1->ICR = USART_ICR_FECF;
			commFrame.data.bErrFrame = true;
		}
		if (flags & USART_ISR_NE)
		{	// noise error (NF bit)
			USART1->ICR |= USART_ICR_NCF;
			commFrame.data.bErrNoise = true;
		}
		if (flags & USART_ISR_ORE)
		{	// overrun error
			USART1->ICR |= USART_ICR_ORECF;
			commFrame.data.bErrOverrun = true;
		}
	}

	// read data
	if (flags & USART_ISR_RXNE)
	{
		// copy to buffer or flush when reached limit
		if (rxIndex < sizeof(union uCommFrame))
			commFrame.uartRxBuff[rxIndex++] = USART1->RDR;
		else
			USART1->RQR |= USART_RQR_RXFRQ;
	}
	// decode data
	if (flags & USART_ISR_IDLE)
	{	// struct received
		HAL_UART_RxCpltCallback(&huart1);
		rxIndex = 0;			// reset buffer index after buffer is processed
		memset(&commFrame, 0x00, sizeof (union uCommFrame)); // reset data & flags
		USART1->ICR |= USART_ICR_IDLECF;
		uartCustomRxTrigger();
	}
}
#endif // CUSTOM_RX



void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	UNUSED(huart);
	UNUSED(cntSent);
#ifdef MCU_HIGH
	cntSent++;
//	if (cntSent % 100 == 0) SPAM(("Sent: %u\n", cntSent));

	ledGreen(BLINK);

	bIdle = true;
#else // MCU_LOW
	SPAM(("MCU low Tx!\n"));
#endif
	return;
}



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	UNUSED(cntReceived);
#ifdef MCU_HIGH

	SPAM(("err: MCU high Rx!\n"));
	return;

#else // MCU_LOW
	cntReceived++;

	ledGreen(BLINK);

	// check frame errors
	if (commFrame.data.bErrFrame || commFrame.data.bErrNoise || commFrame.data.bErrOverrun || commFrame.data.bErrParity)
	{
		SPAM(("uart err: "));
		if (commFrame.data.bErrParity)
			SPAM(("parity, "));
		if (commFrame.data.bErrFrame)
			SPAM(("frame, "));
		if (commFrame.data.bErrNoise)
			SPAM(("noise, "));
		if (commFrame.data.bErrOverrun)
			SPAM(("overrun, "));
		SPAM((".\n"));
	}

	if (commFrame.data.values.crc8 != crc8(commFrame.uartRxBuff, 2*sizeof(float)))
	{
		ledRed(ON);
		bLedSetByCommunication = true;
		System.bCommunicationOk = false;
		//ITM_SendChar('x'); ITM_SendChar('\n');
	}
	else
	{
		if (bLedSetByCommunication)
		{
			ledRed(OFF);
			bLedSetByCommunication = false;
		}

#ifdef USE_MOVAVG_UE_MCULOW
		System.meas.fExtractVolt = movAvgAddSample(&movAvgUe, commFrame.data.values.fExtVolt);
#else
		memcpy(&System.meas.fExtractVolt, &commFrame.data.values.fExtVolt, sizeof(float));
#endif

#ifdef USE_MOVAVG_UF_MCULOW
		System.meas.fFocusVolt = movAvgAddSample(&movAvgUf, commFrame.data.values.fFocusVolt);
#else
		memcpy(&System.meas.fFocusVolt, &commFrame.data.values.fFocusVolt, sizeof(float));
#endif
		System.bCommunicationOk = true;
//		pidMeasOscPeriod(PWM_CHANNEL_UE);
//		pidMeasOscPeriod(PWM_CHANNEL_UF);
	}

#ifndef CUSTOM_RX
	// receive next byte
	uartReceiveFrameIT();
#endif

#endif // MCU_HIGH
}



/*
 * (Not used function.)
 */
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

	if (System.bHighSidePowered == true)
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
