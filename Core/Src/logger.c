/*
 * logger.c
 *
 *  Created on: Jan 29, 2021
 *      Author: lukasz
 */

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "logger.h"
#include "typedefs.h"

/*
 * NOTE:	currently, values from logger are intended to be copied from RAM to
 * 			spreadsheet on PC via debugger on halted core, from Expressions
 * 			view. The practical limit is ca. 2000 records, while copying longer
 * 			buffers the IDE crashes. Even copying one larger buffer [8000] in
 * 			portions of 1000 samples is unreliable. Better is to save several
 * 			buffer not longer than [2000] in sequence and copy them
 * 			independently, merge in spreadsheet.
 */

/* Private variables ---------------------------------------------------------*/

#define LOGGER_BUFF_SIZE		2000

static uint32_t sweepFallingCnt;
static float sweepPeakCurr;		// result (IA)
static float sweepPeakVolt;		// result (UE)
static uint32_t highFreqBuffNo;
static uint32_t loggerBuffIndex;
static float loggerBuffIa[LOGGER_BUFF_SIZE];	// 10 ms 0.5 V step -> 500 V 10 s in 1000 steps.
static float loggerBuffUc[LOGGER_BUFF_SIZE];	// increase buffer size for high freq logger
static float loggerBuffUe[LOGGER_BUFF_SIZE];
static float loggerBuffUf[LOGGER_BUFF_SIZE];
static float fUserValueBackup;

/* Exported functions --------------------------------------------------------*/

/*
 * Init variables for Ue sweep
 */
void sweepUeInit(void)
{
	SPAM(("%s\n", __func__));
	memset(loggerBuffIa, 0x00, sizeof(loggerBuffIa));
	memset(loggerBuffUe, 0x00, sizeof(loggerBuffUe));
	fUserValueBackup = System.ref.fExtractVoltUserRef;
	System.ref.fExtractVoltUserRef = 0.0f;
	sweepFallingCnt = 0;
	loggerBuffIndex = 0;

	sweepPeakCurr = 0.0f;
	sweepPeakVolt = 0.0f;
	System.bSweepOn = true;
}



void loggerInit(void)
{
	SPAM(("%s\n", __func__));
	memset(loggerBuffIa, 0x00, sizeof(loggerBuffIa));
	memset(loggerBuffUc, 0x00, sizeof(loggerBuffUc));
	memset(loggerBuffUe, 0x00, sizeof(loggerBuffUe));
	memset(loggerBuffUf, 0x00, sizeof(loggerBuffUf));
	//fUserValueBackup = System.ref.fExtractVoltUserRef;
	//System.ref.fExtractVoltUserRef = 0.0f;
//	sweepFallingCnt = 0;
	loggerBuffIndex = 0;
	highFreqBuffNo = 0;
	//sweepPeakCurr = 0.0f;
	//sweepPeakVolt = 0.0f;
}



/*
 * @brief	Call it in regular periods to sample Ue and Ia and increment Ue at
 * 			calling frequency (period not controlled internally). In this case,
 * 			use same sampling time as regulator period.
 */
void sweepUePeriod(void)
{
	//const float fStepVolt = 0.5f;	// 0.5 V * 2 kS = 1000 V
	const float fStepVolt = 0.25f;	// 0.25 V * 2 kS = 500 V

	if (System.bSweepOn && System.ref.extMode == EXT_SWEEP)
	{
		if (loggerBuffIndex < LOGGER_BUFF_SIZE)
		{
			loggerBuffIa[loggerBuffIndex] = System.meas.fAnodeCurrent;
			loggerBuffUc[loggerBuffIndex] = System.meas.fCathodeVolt;
			loggerBuffUe[loggerBuffIndex] = System.meas.fExtractVolt;
			loggerBuffUf[loggerBuffIndex] = System.meas.fFocusVolt;
			loggerBuffIndex++;
		}

		// change only ref, will be set by regulator loop (allow little overdrive)
		if (System.ref.fExtractVoltUserRef < (System.ref.fExtractVoltLimit * 1.1f))
			System.ref.fExtractVoltUserRef += fStepVolt;

		// update sample if current is rising
		if (System.meas.fAnodeCurrent > sweepPeakCurr)
		{
			sweepPeakCurr = System.meas.fAnodeCurrent;
			sweepPeakVolt = System.meas.fExtractVolt;
			sweepFallingCnt = 0;
		}
		else
			sweepFallingCnt++;

		// check exit conditions (current falling or volt limit)
//		if ((System.meas.fExtractVolt > (0.25f * System.ref.fExtractVoltLimit)) && (sweepFallingCnt > 100))
//		{
//			SPAM(("falling, "));
//			sweepUeExit(true);
//		}
		if (System.meas.fExtractVolt > System.ref.fExtractVoltLimit)
		{
			SPAM(("voltage, "));
			sweepUeExit(true); // TODO temp exit on falling current should be true, and on the voltage should be false
		}
	} // bSweepOn
}



void loggerPeriod(void)
{
	static uint32_t uTimeConsoleText;
	static uint32_t uLogIntervalCnt;
#ifdef LOGGER_250ms
	const uint32_t uLogInterval = 25;
#elif defined (LOGGER_10ms)
	const uint32_t uLogInterval = 1;
#endif

	if (System.bLoggerOn)
	{
		uLogIntervalCnt++;
		if (uLogIntervalCnt >=  uLogInterval)
		{
			uLogIntervalCnt = 0;

			// print something to indicate logging progress
			if (HAL_GetTick() - uTimeConsoleText > 1000)
			{
				uTimeConsoleText = HAL_GetTick();
				ITM_SendChar('.');
			}

			// save sample
			if (loggerBuffIndex < LOGGER_BUFF_SIZE)
			{
				//if (System.ref.loggerMode == LOGGER_UE)
				loggerBuffIa[loggerBuffIndex] = System.meas.fAnodeCurrent;
				loggerBuffUc[loggerBuffIndex] = System.meas.fCathodeVolt;
				loggerBuffUe[loggerBuffIndex] = System.meas.fExtractVolt;
				loggerBuffUf[loggerBuffIndex] = System.meas.fFocusVolt;
				loggerBuffIndex++;
			}
			else
			// exit
			{
				SPAM(("Logger full.\n"));
				System.bLoggerOn = false;
			}
		}
	}
}



/*
 * It's called at ADS samples Rx, logs every sample.
 * Guard the call with checking logger mode, to not interact with slower logger.
 */
void loggerHighFreqSample(void)
{
	static uint32_t uTimeConsoleText;

	if (System.bLoggerOn)
	{
		// print something to indicate logging progress
		if (HAL_GetTick() - uTimeConsoleText > 1000)
		{
			uTimeConsoleText = HAL_GetTick();
			ITM_SendChar('.');
		}
		// save sample
		if (loggerBuffIndex < LOGGER_BUFF_SIZE)
		{
			if (highFreqBuffNo == 0)
			{
				#ifdef LOGGER_LOG_HF_IA
					loggerBuffIa[loggerBuffIndex++] = System.meas.fAnodeCurrent;
				#elif defined (LOGGER_LOG_HF_UC)
					loggerBuffIa[loggerBuffIndex++] = System.meas.fCathodeVolt;
				#endif
			} else if (highFreqBuffNo == 1) {
				#ifdef LOGGER_LOG_HF_IA
					loggerBuffUc[loggerBuffIndex++] = System.meas.fAnodeCurrent;
				#elif defined (LOGGER_LOG_HF_UC)
					loggerBuffUc[loggerBuffIndex++] = System.meas.fCathodeVolt;
				#endif
			} else if (highFreqBuffNo == 2) {
				#ifdef LOGGER_LOG_HF_IA
					loggerBuffUe[loggerBuffIndex++] = System.meas.fAnodeCurrent;
				#elif defined (LOGGER_LOG_HF_UC)
					loggerBuffUe[loggerBuffIndex++] = System.meas.fCathodeVolt;
				#endif
			} else if (highFreqBuffNo == 3) {
				#ifdef LOGGER_LOG_HF_IA
					loggerBuffUf[loggerBuffIndex++] = System.meas.fAnodeCurrent;
				#elif defined (LOGGER_LOG_HF_UC)
					loggerBuffUf[loggerBuffIndex++] = System.meas.fCathodeVolt;
				#endif
			}
		}
		else
		// exit
		{
			highFreqBuffNo++;
			loggerBuffIndex = 0;
			if (highFreqBuffNo >= 4)
			{
				System.bLoggerOn = false;
				loggerBuffIndex = 0;
				SPAM(("Logger full.\n"));
			}
		}
	}
}



void sweepUeExit(bool success)
{
	SPAM(("%s\n", __func__));

	memcpy(&System.sweepResult, &System.meas, sizeof(struct sRegulatedVal));
	if (success == true)
	{
		System.sweepResult.fAnodeCurrent = sweepPeakCurr;
		System.sweepResult.fExtractVolt = sweepPeakVolt;
	}
	else
	{
		System.sweepResult.fAnodeCurrent = NAN;
		System.sweepResult.fExtractVolt = NAN;
	}

	System.ref.fExtractVoltUserRef = fUserValueBackup;
	System.bSweepOn = false;
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
