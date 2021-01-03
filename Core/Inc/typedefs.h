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



/*
 * Extract voltage control mode.
 *
 * EXT_REGULATE_IA - default mode, UE out of user control (but within
 * 				fExtractVoltLimit), regulated to achieve required IA value.
 *
 * EXT_STEADY - regulate UE to System.ref.fExtractVoltUserRef value. Ignore
 * 				required Ia, current regulator disabled.
 *
 * EXT_SWEEP - sweep UE voltage (once, on demand) from 0 up to fExtractVoltLimit
 * 				or up to passed peak Ia current.
 * 				(expect falling current on Ia when Uce is high enough to
 * 				overtake current from anode).
 */
enum eExtMode
{
	EXT_REGULATE_IA = 0,	// default mode - UE out of user control (but within
							// fExtractVoltLimit), regulated to achieve required
							// IA value
	EXT_STEADY,
	EXT_SWEEP,
};

struct sRegulatedVal
{	// direct regulated values
	float fAnodeCurrent;
	float fCathodeVolt;
	float fFocusVolt;
	float fPumpVolt;
	// helper variables
	float fExtractVolt;			// measured value
	float fExtractVoltUserRef;	// reference from user
	float fExtractVoltIaRef;	// reference from Ia regulator
	float fExtractVoltLimit;	// limit set by user
	uint32_t uAnodeCurrent;	// 0.1 uA unit - for settings
	enum eExtMode extMode;
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
