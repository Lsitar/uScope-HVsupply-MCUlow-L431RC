/*
 * calibration.c
 *
 *  Created on: Dec 30, 2020
 *      Author: lukasz
 */

#include "calibration.h"
#include "communication.h"
#include "main.h"
#include "regulator.h"	// for debug defines PWM_CHANNEL_
#include "typedefs.h"

/* Private defines -----------------------------------------------------------*/

typedef struct
{
	float gain;
	int32_t offset;
} tsCoeff;

// default coefficients
static const float fCoeffUcDefault = 1.2f * (100e+3/50e+3) * (500e+6/180e+3) / ((float)(1u << 23));	// [V/bit] // 1.08 V on ADC at 6 kV in
static const float fCoeffUeDefault = 1.2f * (100e+3/24e+3) * (100e+6/76744.2f) / ((float)(1u << 23));	// [V/bit] // 1.16 V on ADC at 6 kV in
static const float fCoeffUfDefault = 1.2f * (100e+3/24e+3) * (100e+6/76744.2f) / ((float)(1u << 23));	// [V/bit] // 1.16 V on ADC at 6 kV in
static const float fCoeffIaDefault = (-1.0f) * 1.2f * (100e+3/47e+3) * (1.0f/51e+3) / ((float)(1u << 23));	// [A/bit] // 1.1985 V on ADC at 50 uA in
static const float fCoeffUpDefault = 1.0f/((6000.0f/12.0f) * (16300.0f/4300.0f) * 3.3f);	// [duty/V] // 0.959286 duty at 6 kV out and 3.3V ref

// coeff variables (init with above values)
static tsCoeff fCoeffUc = {0.000794728636f, 0};
static tsCoeff fCoeffUe = {0.000776666449f, 0};
static tsCoeff fCoeffUf = {0.000776666449f, 0};
static tsCoeff fCoeffIa = {-5.96792451e-12, 0};
//static tsCoeff fCoeffUp = {0.000159881019f, 0};
static struct {
	float gain;
	float offset;
} fCoeffUp = {0.000159881019f, 0};

/* Exported functions --------------------------------------------------------*/

/*
 * load calibration coefficients
 */
void initCoefficients(void)
{
	// MCU_LOW Ch0
	fCoeffIa.gain = fCoeffIaDefault * (10.0f/9.62f);	// (meas external ref / meas ADS)	// [A/bit]
	fCoeffIa.offset = -37850;			// [bit]

	// MCU_LOW Ch1
	fCoeffUc.gain = fCoeffUcDefault * (935.0f/900.0f);	// [V/bit]
	fCoeffUc.offset = -29325;							// [bit]

	// MCU_HIGH Ch0
	fCoeffUe.gain = fCoeffUeDefault * (759.6f/735.4f);	// (meas external ref / meas ADS)
	fCoeffUe.offset = -48034;

	// MCU_HIGH Ch1
	fCoeffUf.gain = fCoeffUfDefault * (734.0f/708.2f);	// (meas external ref / meas ADS)
	fCoeffUf.offset = -39380;

	fCoeffUp.gain = fCoeffUpDefault;	// [duty/V]
	fCoeffUp.offset = 0;				// [V]
}



/*
 * Call this function after every received samples.
 */
void calibOffset(void)
{
	static int32_t sumCh0 = 0, sumCh1 = 0;
	static int32_t index = 0;
	const int32_t samplesNo = 10000;

	sumCh0 += System.ads.data.channel0;
	sumCh1 += System.ads.data.channel1;
	index++;

	if (index > samplesNo)
	{
		__NOP();
		sumCh0 /= samplesNo;
		sumCh1 /= samplesNo;
#ifdef MCU_HIGH
		SPAM(("Ue (MCU_HIGH Ch0) offset: %i\n", sumCh0));	// MCU_LOW: Ia, MCU_HIGH: Ue
		SPAM(("Uf (MCU_HIGH Ch1) offset: %i\n", sumCh1)); // MCU_LOW: Uc, MCU_HIGH: Uf
#else
		SPAM(("Ia (MCU_LOW Ch0) offset: %i\n", sumCh0));	// MCU_LOW: Ia, MCU_HIGH: Ue
		SPAM(("Uc (MCU_LOW Ch1) offset: %i\n", sumCh1)); // MCU_LOW: Uc, MCU_HIGH: Uf
#endif
		// reset variables
		sumCh0 = 0;
		sumCh1 = 0;
		index = 0;
	}
}



#define MOVAVG_SIZE		10
static float fMovAvgSum;
static float fMovAvgBuff[MOVAVG_SIZE];
uint32_t uMovAvgIndex;

struct sMovAvg
{
	float fSum;
	float fBuff[MOVAVG_SIZE];
	uint32_t uIndex;
};

void movAvgInit(void)
{
	fMovAvgSum = 0.0f;
	uMovAvgIndex = 0;

	// set buff to 0.0 float (not 0x00 hex)
	for (uint32_t i=0; i<MOVAVG_SIZE; i++)
		fMovAvgBuff[i] = 0.0f;
	;
}

float movAvgAddSample(float newSample)
{
	// remove oldest sample from sum
	fMovAvgSum -= fMovAvgBuff[uMovAvgIndex];
	// replace the old sample with new
	fMovAvgBuff[uMovAvgIndex] = newSample;
	// add newest sample to sum
	fMovAvgSum += fMovAvgBuff[uMovAvgIndex];
	// point to next (oldest) sample
	uMovAvgIndex++;
	// wrap buffer
	if (uMovAvgIndex >= MOVAVG_SIZE)
	{
		// TEST - numeric error may accumulate and it may be necessary to re-calculate the sum after some time
		fMovAvgSum = 0.0f;
		for (uint32_t i=0; i<MOVAVG_SIZE; i++)
			fMovAvgSum += fMovAvgBuff[i];
		// END TEST

		uMovAvgIndex = 0;
	}

	return fMovAvgSum / ((float)MOVAVG_SIZE);
}


void calcualteSamples(void)
{
#ifdef MCU_HIGH

    System.meas.fExtractVolt = fCoeffUe.gain * (System.ads.data.channel0 - fCoeffUe.offset);
    System.meas.fFocusVolt = fCoeffUf.gain * (System.ads.data.channel1 - fCoeffUf.offset);

    if (uartIsIdle())
    	sendResults();

#else // MCU_LOW

	#ifdef USE_MOVAVG_IA_FILTER
		System.meas.fAnodeCurrent = movAvgAddSample(fCoeffIa.gain * (System.ads.data.channel0 - fCoeffIa.offset));
	#else
		System.meas.fAnodeCurrent = fCoeffIa.gain * (System.ads.data.channel0 - fCoeffIa.offset);
	#endif

	System.meas.fCathodeVolt = fCoeffUc.gain * (System.ads.data.channel1 - fCoeffUc.offset);
    //pidMeasOscPeriod(PWM_CHANNEL_UC);
	//pidMeasOscPeriod(REG_IA);

#endif // MCU_HIGH
}



/*
 * Just scale, do not use PID regulator
 */
float getPumpDuty(float voltage)
{
	return (voltage - fCoeffUp.offset) * fCoeffUp.gain;
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
