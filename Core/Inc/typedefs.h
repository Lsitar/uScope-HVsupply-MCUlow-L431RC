/*
 * typedefs.h
 *
 *  Created on: Dec 20, 2020
 *      Author: Lukasz Sitarek
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "logger.h"	// for eLoggerMode definition
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

typedef struct
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
	uint32_t uAnodeCurrent;		// 0.01 uA unit - for settings
	enum eExtMode extMode;
	enum eLoggerMode loggerMode;	// choose value to log
} tsRegulatedVal;

struct sSystem
{
	tsRegulatedVal meas;
	tsRegulatedVal ref;
	tsRegulatedVal sweepResult;
	bool bCommunicationOk;
	bool bHighSidePowered;
	bool bLowBatt;
	bool bSweepOn;
	bool bLoggerOn;
	adsStatus_t ads;
	float battVolt;
	uint32_t battProc;
};

extern struct sSystem System;

#ifdef __cplusplus
}
#endif
