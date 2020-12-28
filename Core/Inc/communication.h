/*
 * communication.h
 *
 *  Created on: Dec 24, 2020
 *      Author: lukasz
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

struct sCommFrame
{
	uint32_t password;
	float fExtVolt;
	float fFocusVolt;
};

union uCommFrame
{
	struct sCommFrame data;
	uint8_t uartRxBuff[sizeof(struct sCommFrame) + 2];	// uint8 size for 8 bit data with parity
//	uint16_t uartRxBuff[sizeof(struct sCommFrame) + 2];
};


//extern uint8_t uartRxBuff[];
//extern uint8_t uartTxBuff[];
extern union uCommFrame commFrame;

/* Exported functions --------------------------------------------------------*/

void uartReceiveFrameIT(void);
bool uartIsIdle(void);
void sendResults(void);


#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
