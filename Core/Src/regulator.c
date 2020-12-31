/*
 * regulator.c
 *
 *  Created on: Dec 27, 2020
 *      Author: lukasz
 */

#include "stm32l4xx_hal.h"
#include "pid_controller.h"
#include "regulator.h"
#include "typedefs.h"
#include "utilities.h"

/* Private defines -----------------------------------------------------------*/

#define PID_PERIOD	(0.01f)		// 10 ms
#define PID_OUT_MIN	(0.0f)

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

/* **** U focus regulator tuning *********************************************
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

PIDControl pidUc, pidUe, pidUf;

/* Exported functions --------------------------------------------------------*/

void regulatorInit(void)
{
	PIDInit(&pidUc,
			PID_UC_KP,	PID_UC_KI,	PID_UC_KD,
			PID_PERIOD,
			PID_OUT_MIN,	PID_OUT_MAX_UC,
			AUTOMATIC,	// mode
			REVERSE);	// direction

	PIDInit(&pidUe,
			PID_UE_KP,	PID_UE_KI,	PID_UE_KD,
			PID_PERIOD,
			PID_OUT_MIN,	PID_OUT_MAX_UE,
			AUTOMATIC,	// mode
			DIRECT);	// direction

	PIDInit(&pidUf,
			PID_UF_KP,	PID_UF_KI,	PID_UF_KD,
			PID_PERIOD,
			PID_OUT_MIN,	PID_OUT_MAX_UF,
			AUTOMATIC,	// mode
			DIRECT);	// direction

	PIDSetpointSet(&pidUc, System.ref.fCathodeVolt);
	PIDSetpointSet(&pidUe, System.ref.fExtractVolt);
	PIDSetpointSet(&pidUf, System.ref.fFocusVolt);

	HAL_TIM_Base_Start_IT(&htim6);
}



void regulatorDeInit(void)
{
	PIDModeSet(&pidUc, MANUAL);
	PIDModeSet(&pidUe, MANUAL);
	PIDModeSet(&pidUf, MANUAL);
	HAL_TIM_Base_Stop_IT(&htim6);
}


/*
 * Call every PID_PERIOD s
 */
void regulatorPeriodCallback(void)
{
	PIDInputSet(&pidUc, System.meas.fCathodeVolt);	// TODO to moze lepiej powiazac jakos z przerwaniem od ADS
	PIDSetpointSet(&pidUc, System.ref.fCathodeVolt);
	PIDCompute(&pidUc);
	pwmSetDuty(PWM_CHANNEL_UC, PIDOutputGet(&pidUc));

	// Run regulator only, when there are valid samples from High side. Else, stay on previous value.
	if (System.bCommunicationOk == true)
	{
		PIDInputSet(&pidUe, System.meas.fExtractVolt);
//		PIDSetpointSet(&pidUe, System.ref.fExtractVolt);
		PIDSetpointSet(&pidUe, System.ref.fPumpVolt);	// for PID tuning
		PIDCompute(&pidUe);
		pwmSetDuty(PWM_CHANNEL_UE, PIDOutputGet(&pidUe));

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

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
