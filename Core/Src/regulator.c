/*
 * regulator.c
 *
 *  Created on: Dec 27, 2020
 *      Author: lukasz
 */

#include <math.h>
#include <string.h>
#include "calibration.h"
#include "stm32l4xx_hal.h"
#include "pid_controller.h"
#include "regulator.h"
#include "typedefs.h"
#include "utilities.h"

/* Private defines -----------------------------------------------------------*/

#define LOGGER_BUFF_SIZE		2000

#define PID_PERIOD	(0.01f)		// 10 ms
#define PID_OUT_PWM_MIN	(0.0f)

/* **** U cathode regulator tuning *********************************************
 * Kp (0.0001f) gives stable, but 50% of value set
 * Kp (0.001f) gives oscillations, 2800-3900 V (4 kV set), SW meas 50 ms period.
 * Oscilations depends on load and voltage set, also the period.
 *
 * Ku = (0.00045f), Tu = 0.05 s - without load, 3.0 kV
 * Calculate for PI regulator:
 * Kp = Ku * 0.45 = (0.00002025f)
 * Ti = Tu * 0.8 = (0.040f) -> Ki = 0.54 * (0.00045f)/(0.050f) = (0.009f)
 * PI works fine, fast, no visible overshoot, but only without load!
 *
 * With load 12 MOhm, ca 800 V out:
 * Ku = (0.00030f), Tu = 138 ms --> Kp = (0.000135f), Ki = (0.001174f)
 *
 * (gain changed after calibration and caused unstability)
 * With load 12||10 MOhm, ca 800 V out:
 * Ku = (0.00038f), Tu = 98 ms --> Kp = 0.000171, Ki = 0.0020938
 */
#define PID_UC_KP	(0.000171)
#define PID_UC_KI	(0.0020938f)
#define PID_UC_KD	(0.0f)
#define PID_OUT_MAX_UC	(0.9f)

/* **** U extract regulator tuning *********************************************
 * Voltmeter calibrated first. Load 12 MOhm.
 * Ku = (0.0003f), osc 160 ms at 1600 V set (barely triggered)
 *
 * Ku = (0.0003f), Tu = 160 ms --> Kp = (0.000135f), Ki = (0.0010125f)
 */
#define PID_UE_KP	(0.000135f)
#define PID_UE_KI	(0.0010125f)
#define PID_UE_KD	(0.0f)
#define PID_OUT_MAX_UE	(0.5f)	// max 3 kV

/* **** U focus regulator tuning ***********************************************
 * Voltmeter calibrated first. Load 12 MOhm.
 * Ku = (0.00035f), osc -- ms cannot trigger
 * Ku = (0.000375f), osc 70 ms at 3100 V set (fading oscillations)
 * Ku = (0.0004f), osc 76 ms at 3100 V set (still oscillations)
 *
 * Ku = (0.00038f), Tu = 76 ms --> Kp = (0.00017f), Ki = (0.0027f)
 */
#define PID_UF_KP	(0.00017f)
#define PID_UF_KI	(0.0027f)
#define PID_UF_KD	(0.0f)
#define PID_OUT_MAX_UF	(0.9f)	// same as for Uc

/* **** Anode current regulator tuning *****************************************
 * Ammeter calibrated first. Load with real uScope.
 * Ku = (100000000.0f) (1e+8), osc -- ms cannot trigger. There was problem with startup beacouse of Ue near lower bound.
 * Ku = (150000000.0f), stable, 0.35 uA out of 1.0 set... but after changing ref, oscillates still 150 ms.
 * Ku = (200000000.0f), osc 85,99,111-115,124,137 ms ...
 *
 * Kp = 0.45 * Ku = 67 500 000
 * Ki = 0.54 * Ku/Tu = 0.54 * (150 000 000 / 0.150) = 540 000 000
 */
#define PID_IA_KP	(67500000.0f)
#define PID_IA_KI	(540000000.0f)
#define PID_IA_KD	(0.0f)
#define PID_OUT_MAX_IA	(System.ref.fExtractVoltLimit)			//(2500.0f)	// max UE ref - TODO set it dynamically from variable

//

/* Private variables ---------------------------------------------------------*/

PIDControl pidUc, pidUe, pidUf, pidIa;

/* Exported functions --------------------------------------------------------*/

void regulatorInit(void)
{
	PIDInit(&pidUc,
			PID_UC_KP,	PID_UC_KI,	PID_UC_KD,
			PID_PERIOD,
			PID_OUT_PWM_MIN,	PID_OUT_MAX_UC,
			AUTOMATIC,	// mode
			REVERSE);	// direction

	PIDInit(&pidUe,
			PID_UE_KP,	PID_UE_KI,	PID_UE_KD,
			PID_PERIOD,
			PID_OUT_PWM_MIN,	PID_OUT_MAX_UE,
			AUTOMATIC,	// mode
			DIRECT);	// direction

	PIDInit(&pidUf,
			PID_UF_KP,	PID_UF_KI,	PID_UF_KD,
			PID_PERIOD,
			PID_OUT_PWM_MIN,	PID_OUT_MAX_UF,
			AUTOMATIC,	// mode
			DIRECT);	// direction

	PIDSetpointSet(&pidUc, 0.0f);
	PIDSetpointSet(&pidUe, 0.0f);
	PIDSetpointSet(&pidUf, 0.0f);

	HAL_TIM_Base_Start_IT(&htim6);
}



/*
 * @NOTE	assumes, that regulator for voltages was already init.
 * 			If the voltage regulator wasn't init & run first, unexpected behaviour.
 */
void regulatorInitCurrent(void)
{
	PIDInit(&pidIa,
			PID_IA_KP,	PID_IA_KI,	PID_IA_KD,
			PID_PERIOD,
			100.0f,	PID_OUT_MAX_IA,	// minimum Ext ref: 100 V
			AUTOMATIC,	// mode
			DIRECT);	// direction

	PIDSetpointSet(&pidIa, 0.0f);
}



void regulatorDeInit(void)
{
	PIDModeSet(&pidUc, MANUAL);
	PIDModeSet(&pidUe, MANUAL);
	PIDModeSet(&pidUf, MANUAL);
	PIDModeSet(&pidIa, MANUAL);
	HAL_TIM_Base_Stop_IT(&htim6);
	pwmSetDuty(PWM_CHANNEL_UC, 0.0f);
	pwmSetDuty(PWM_CHANNEL_UE, 0.0f);
	pwmSetDuty(PWM_CHANNEL_UF, 0.0f);
	pwmSetDuty(PWM_CHANNEL_PUMP, 0.0f);
}



/*
 * Call every PID_PERIOD s
 */
void regulatorPeriodCallback(void)
{
	/* Cathode voltage */
	PIDInputSet(&pidUc, System.meas.fCathodeVolt);	// TODO to moze lepiej powiazac jakos z przerwaniem od ADS. Tylko te przerwania nie moga sie wcinac jedno w drugie. Teraz to ok, ale jak zrobie ADS na DMA to chyba bedzie sie wcinac.
	PIDSetpointSet(&pidUc, System.ref.fCathodeVolt);
	PIDCompute(&pidUc);
	pwmSetDuty(PWM_CHANNEL_UC, PIDOutputGet(&pidUc));

	/* Pump voltage - set open loop, it is not regulated */
	pwmSetVoltManual(PWM_CHANNEL_PUMP, System.ref.fPumpVolt);

	// Run following regulator only, when there are valid samples from High side. Else, stay on previous value.
	if (System.bCommunicationOk == true)
	{
		/* Anode current */
		if (System.ref.extMode == EXT_REGULATE_IA)
		{
			PIDInputSet(&pidIa, System.meas.fAnodeCurrent);
			PIDSetpointSet(&pidIa, System.ref.fAnodeCurrent);
			PIDCompute(&pidIa);
			System.ref.fExtractVoltIaRef = PIDOutputGet(&pidIa);
		}

		/* Extract voltage */
		PIDInputSet(&pidUe, System.meas.fExtractVolt);
		if (System.ref.extMode == EXT_REGULATE_IA)
			PIDSetpointSet(&pidUe, System.ref.fExtractVoltIaRef);
		else
			PIDSetpointSet(&pidUe, System.ref.fExtractVoltUserRef);
		PIDCompute(&pidUe);
		pwmSetDuty(PWM_CHANNEL_UE, PIDOutputGet(&pidUe));

		/* Focus voltage */
		PIDInputSet(&pidUf, System.meas.fFocusVolt);
		PIDSetpointSet(&pidUf, System.ref.fFocusVolt);
		PIDCompute(&pidUf);
		pwmSetDuty(PWM_CHANNEL_UF, PIDOutputGet(&pidUf));
		// for offset calibration
//		pwmSetDuty(PWM_CHANNEL_UE, 0.0f);
//		pwmSetDuty(PWM_CHANNEL_UF, 0.0f);
		// for gain calibration
//		pwmSetVoltManual(PWM_CHANNEL_UE, System.ref.fPumpVolt);
//		pwmSetVoltManual(PWM_CHANNEL_UF, System.ref.fFocusVolt);
	}
}



/*
 * Manually set duty based on required output voltage.
 */
void pwmSetVoltManual(enum ePwmChannel PWM_CHANNEL_, float voltage)
{
	const float fGain = (6000.0f/12.0f) * (16300.0f/4300.0f) * 3.3f;

	float duty = voltage / fGain;

	if (PWM_CHANNEL_ == PWM_CHANNEL_PUMP)
	{
		pwmSetDuty(PWM_CHANNEL_PUMP, getPumpDuty(voltage));
	}
	else
		pwmSetDuty(PWM_CHANNEL_, duty);
}



/*
 * Used to tune PID regulator (Ziegler-Nichols method).
 * Call every time after collecting adc sample.
 */
void pidMeasOscPeriod(enum ePwmChannel PWM_CHANNEL_)
{
	static uint32_t uTimetick;
	static float fLastSample;
	static bool bLastSlope;
	static uint32_t period;
	static uint32_t index = 0;
	const uint32_t samplesNo = 10;
	// measure period between rising slopes
	float value;

	if (PWM_CHANNEL_ == PWM_CHANNEL_UC)
		value = System.meas.fCathodeVolt;
	else if (PWM_CHANNEL_ == PWM_CHANNEL_UE)
		value = System.meas.fExtractVolt;
	else if (PWM_CHANNEL_ == PWM_CHANNEL_UF)
		value = System.meas.fFocusVolt;
	else if (PWM_CHANNEL_ == PWM_CHANNEL_PUMP)
		value = System.meas.fPumpVolt;
	else if (PWM_CHANNEL_ == REG_IA)
		value = System.meas.fAnodeCurrent;


	if (value - fLastSample > 0.0f)
	{	// rising slope
		if (bLastSlope == false)
		{
			period += HAL_GetTick() - uTimetick;
			if (index++ >= samplesNo)
			{
				period = period/samplesNo;
				SPAM(("osc: %u\n", period));
				period = 0;
				index = 0;
			}
			uTimetick = HAL_GetTick();
		}
		bLastSlope = true;
	}
	else
	{	// falling slope
		//if (bLastSlope == true)
		bLastSlope = false;
	}
	fLastSample = value;
}



/* Extract voltage sweep -----------------------------------------------------*/

static uint32_t sweepFallingCnt;
static float sweepPeakCurr;		// result (IA)
static float sweepPeakVolt;		// result (UE)
static uint32_t loggerBuffIndex;
static float loggerBuffIa[LOGGER_BUFF_SIZE];	// 10 ms 0.5 V -> 500 V 10 s 1000 steps.
static float loggerBuffUe[LOGGER_BUFF_SIZE];	// 10 ms 0.5 V -> 500 V 10 s 1000 steps.
static float fUserValueBackup;


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
	SPAM(("Logger on,"));
	memset(loggerBuffIa, 0x00, sizeof(loggerBuffIa));
	//memset(loggerBuffUe, 0x00, sizeof(loggerBuffUe));
	//fUserValueBackup = System.ref.fExtractVoltUserRef;
	//System.ref.fExtractVoltUserRef = 0.0f;
	//sweepFallingCnt = 0;
	loggerBuffIndex = 0;

	//sweepPeakCurr = 0.0f;
	//sweepPeakVolt = 0.0f;
	System.bLoggerOn = true;
}



/*
 * @brief	Sample Ue and Ia and increment Ue at calling frequency (period not
 * 			controlled internally). In this case, use same sampling time as
 * 			regulator period.
 */
void sweepUePeriod(void)
{
	const float fStepVolt = 0.5f;

	if (System.bSweepOn && System.ref.extMode == EXT_SWEEP)
	{
		if (loggerBuffIndex < LOGGER_BUFF_SIZE)
		{
			loggerBuffIa[loggerBuffIndex] = System.meas.fAnodeCurrent;
			loggerBuffUe[loggerBuffIndex++] = System.meas.fExtractVolt;
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
//			if (   ( (System.meas.fExtractVolt > 0.25f * System.ref.fExtractVoltLimit) && (sweepFallingCnt > 100))
//				|| (System.meas.fExtractVolt > System.ref.fExtractVoltLimit)   )
//			{
//				sweepUeExit(true); // TODO temp exit on falling current should be true, and on the voltage should be false
//			}
		if ((System.meas.fExtractVolt > (0.25f * System.ref.fExtractVoltLimit)) && (sweepFallingCnt > 100))
		{
			SPAM(("falling, "));
			sweepUeExit(true);
		}
		if (System.meas.fExtractVolt > System.ref.fExtractVoltLimit)
		{
			SPAM(("voltage, "));
			sweepUeExit(true);
		}
	} // bSweepOn
}



void loggerPeriod(void)
{
	static uint32_t uTimestampText;
	static uint32_t uIntervalCnt;
#ifdef LOGGER_250ms
	const uint32_t uLogInterval = 25;
#else
	const uint32_t uLogInterval = 1;
#endif


	if (System.bLoggerOn)
	{
		uIntervalCnt++;
		if (uIntervalCnt >=  uLogInterval)
		{
			uIntervalCnt = 0;

			// print something to indicate logging progress
			if (HAL_GetTick() - uTimestampText > 1000)
			{
				uTimestampText = HAL_GetTick();
				ITM_SendChar('.');
			}

			// save sample
			if (loggerBuffIndex < LOGGER_BUFF_SIZE)
			{
				if (System.ref.loggerMode == LOGGER_UE)
					loggerBuffIa[loggerBuffIndex++] = System.meas.fExtractVolt;
				else
					loggerBuffIa[loggerBuffIndex++] = System.meas.fAnodeCurrent;
			}
			else
			// exit
			{
				SPAM((" Logger full.\n"));
				System.bLoggerOn = false;
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
