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

struct sRegulatedVal
{
	float fAnodeCurrent;
	uint32_t uAnodeCurrent;	// 0.1 uA unit - for settings
	float fCathodeVolt;
	float fExtractVolt;
	float fExtractVoltLimit;
	float fFocusVolt;
	float fPumpVolt;
};

struct sSystem
{
	struct sRegulatedVal meas;
	struct sRegulatedVal ref;
	struct sRegulatedVal sweepResult;
	bool bCommunicationOk;
	bool bHighSidePowered;
	bool bSweepOn;
	adsStatus_t ads;
};

extern struct sSystem System;

#ifdef __cplusplus
}
#endif
