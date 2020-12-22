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
#include "ui.h"

struct sSystem
{
	struct
	{
		float fAnodeCurrent;
		float fCathodeVolt;
		float fExtractVolt;
		float fFocusVolt;
	} meas;

	struct
	{
		float fAnodeCurrent;
		float fCathodeVolt;
		float fFocusVolt;
	} ref;

	adsStatus_t ads;

	struct sMenu menu;
};

extern struct sSystem System;


#ifdef __cplusplus
}
#endif
