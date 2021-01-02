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
	SCREEN_1,	// UK Ia Pa
	SCREEN_2,	// Ue Uf Up
	SCREEN_SWEEP_IA,
	SCREEN_SET_IA,
	SCREEN_SET_UC,
	SCREEN_SET_UF,
	SCREEN_SET_UP,
	SCREEN_SET_UE,
	SCREEN_SET_SWEEP,
	SCREEN_POWERON_1,
	SCREEN_POWERON_2,
	SCREEN_POWEROFF,
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
