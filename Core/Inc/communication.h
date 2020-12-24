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

#include <stdint.h>

struct sCommFrame
{
	uint32_t password;
	float fExtVolt;
	float fFocusVolt;
};

union uCommFrame
{
	struct sCommFrame data;
	uint8_t uartRxBuff[sizeof(struct sCommFrame) + 2];
};


//extern uint8_t uartRxBuff[];
//extern uint8_t uartTxBuff[];
extern union uCommFrame commFrame;



#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
