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

#define PID_PERIOD	(0.01f)		// 10 ms
#define PID_OUT_MIN	(0.0f)

/* Kp (0.0001f) gives stable, but 50% of value set
 * Kp (0.001f) gives oscillations, 2800-3900 V (4 kV set), SW meas 50 ms period.
 * Ku = (0.00045f), Tu = 0.05 s
 * Calculate for PI regulator:
 * Kp = Ku * 0.45 = (0.00002025f)
 * Ti = Tu * 0.8 = (0.040f) -> Ki = 0.54 * (0.00045f)/(0.050f) = (0.009f)
 * PI works fine, fast, no visible overshoot!
 *
 */
#define PID_UK_KP	(0.00002025f)
#define PID_UK_KI	(0.009f)	//(0.0108f)	//(0.0001f)
#define PID_UK_KD	(0.0f)
#define PID_OUT_MAX_UC	(0.9f)

#define PID_UE_KP	(0.01f)
#define PID_UE_KI	(0.01f)
#define PID_UE_KD	(0.0f)
#define PID_OUT_MAX_UE	(0.5f)

#define PID_UF_KP	(0.01f)
#define PID_UF_KI	(0.01f)
#define PID_UF_KD	(0.0f)
#define PID_OUT_MAX_UF	(0.9f)

PIDControl pidUc, pidUe, pidUf;

void regulatorInit(void)
{
	PIDInit(&pidUc,
			PID_UK_KP,	PID_UK_KI,	PID_UK_KD,
			PID_PERIOD,
			PID_OUT_MIN,	PID_OUT_MAX_UC,
			AUTOMATIC,	// mode
			REVERSE);	// direction

	PIDInit(&pidUe,
			PID_UE_KP,	PID_UE_KI,	PID_UE_KD,
			PID_PERIOD,
			PID_OUT_MIN,	PID_OUT_MAX_UE,
			MANUAL,	//AUTOMATIC,	// mode
			DIRECT);	// direction

	PIDInit(&pidUf,
			PID_UF_KP,	PID_UF_KI,	PID_UF_KD,
			PID_PERIOD,
			PID_OUT_MIN,	PID_OUT_MAX_UF,
			MANUAL,	//AUTOMATIC,	// mode
			DIRECT);	// direction

	PIDSetpointSet(&pidUc, System.ref.fCathodeVolt);
	PIDSetpointSet(&pidUe, System.ref.fExtractVolt);
	PIDSetpointSet(&pidUf, System.ref.fFocusVolt);

	HAL_TIM_Base_Start_IT(&htim6);
}



/*
 * every 10 ms
 */
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
void regulatorPeriodCallback(void)
{
	PIDInputSet(&pidUc, System.meas.fCathodeVolt);	// TODO to moze lepiej powiazac jakos z przerwaniem od ADS
	PIDSetpointSet(&pidUc, System.ref.fCathodeVolt);
	PIDCompute(&pidUc);
	pwmSetDuty(PWM_CHANNEL_UC, PIDOutputGet(&pidUc));

	if (System.bHighSideOk == true)
	{
		PIDInputSet(&pidUe, System.meas.fExtractVolt);
		PIDSetpointSet(&pidUe, System.ref.fExtractVolt);
		PIDCompute(&pidUe);
		pwmSetDuty(PWM_CHANNEL_UE, PIDOutputGet(&pidUe));

		PIDInputSet(&pidUf, System.meas.fFocusVolt);
		PIDSetpointSet(&pidUf, System.ref.fFocusVolt);
		PIDCompute(&pidUf);
		pwmSetDuty(PWM_CHANNEL_UF, PIDOutputGet(&pidUf));
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
void pidMeasOscPeriod(void)
{
	static uint32_t uTimetick;
	static float fLastSample;
	static bool bLastSlope;
	static uint32_t period;
	static uint32_t index = 0;
	const uint32_t samplesNo = 10;
	// measure period between rising slopes

	if (System.meas.fCathodeVolt - fLastSample > 0.0f)
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
	fLastSample = System.meas.fCathodeVolt;
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
