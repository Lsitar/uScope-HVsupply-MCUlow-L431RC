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

#include "printf.h"
#include "hd44780_i2c.h"

enum eScreen
{
	SCREEN_RESET = 0,
	SCREEN_1,	// UK Ia Pa
	SCREEN_2,	// Ue Uf Up
	SCREEN_SET_IA,
	SCREEN_SET_UC,
	SCREEN_SET_UF,
	SCREEN_SET_UP,
	SCREEN_POWERON_1,
	SCREEN_POWERON_2,
	SCREEN_POWEROFF,
};


struct sMenu
{
	uint_least8_t uRowActive;
};


void readKeyboard(void);
void uiScreenChange(enum eScreen newScreen);
uint32_t uiGetScreenTime(void);
void uiScreenUpdate(void);

void encoderKnob_buttonCallback(void);
void encoderKnob_turnCallback(void);


#ifdef __cplusplus
}
#endif
