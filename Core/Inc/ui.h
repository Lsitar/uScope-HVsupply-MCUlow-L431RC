/*
 * ui.h
 *
 *  Created on: Dec 21, 2020
 *      Author: lukasz
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/

enum eScreen
{
	SCREEN_RESET = 0,
	// main screens
	SCREEN_1,	// UK Ia Pa
	SCREEN_2,	// Ue Uf Up
	SCREEN_CONTROL_UE,

	// settings screens group 1
	SCREEN_SET_IA,
	SCREEN_SET_UC,
	SCREEN_SET_UF,
	SCREEN_SET_UP,

	// settings screens group 2
	SCREEN_SET_UE,
	SCREEN_SET_UEMAX,
	SCREEN_SET_UEMODE,
	SCREEN_SET_LOGGER,

	// text only screens
	SCREEN_POWERON_1,
	SCREEN_POWERON_2,
	SCREEN_POWEROFF,
	SCREEN_LOWBATT,
};

/* Exported functions --------------------------------------------------------*/

void keyboardRoutine(void);
void uiInit(void);
void uiScreenChange(enum eScreen newScreen);
uint32_t uiGetScreenTime(void);
void uiScreenUpdate(void);

void encoderKnob_buttonCallback(void);
void encoderKnob_turnCallback(void);


#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
