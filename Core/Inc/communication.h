/*
 * communication.h
 *
 *  Created on: Dec 24, 2020
 *      Author: Lukasz Sitarek
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define CUSTOM_RX

/* Exported types ------------------------------------------------------------*/

struct sCommFrame
{
	struct
	{
		float fExtVolt;
		float fFocusVolt;
		uint8_t crc8;
	} values;

	bool bErrParity;
	bool bErrFrame;
	bool bErrNoise;
	bool bErrOverrun;
};

union uCommFrame
{
	struct sCommFrame data;
	uint8_t uartRxBuff[sizeof(struct sCommFrame)];	// uint8 size for 8 bit data with parity
};

/* Exported variables --------------------------------------------------------*/

extern union uCommFrame commFrame;
extern int32_t commWatchdog;

/* Exported functions --------------------------------------------------------*/

void uartReceiveFrameIT(void);
bool uartIsIdle(void);
void sendResults(void);

#ifdef CUSTOM_RX
void uartCustomRxInit(void);
void uartCustomIrqHandler(void);
#endif // CUSTOM_RX

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
