/*
 * typedefs.h
 *
 *  Created on: Dec 20, 2020
 *      Author: lukasz
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ads131m0x.h"

struct sSystem
{
	float fAnodeCurrent;
	float fKathodeVolt;
	float fExtractVolt;
	float fFocusVolt;
	float fPumpVolt;

	adsStatus_t ads;

	struct
	{
		uint_least8_t uRowActive;
	} menu;
};

extern struct sSystem System;


#ifdef __cplusplus
}
#endif
