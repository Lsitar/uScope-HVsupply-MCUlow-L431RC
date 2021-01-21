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



/* Moving average filter -----------------------------------------------------*/

struct sMovAvg	movAvgIa, 		\
				movAvgUc, 		\
				movAvgUe, 		\
				movAvgUf,		\
				movAvgAdcBatt;	\

void movAvgInit(struct sMovAvg* movAvg)
{
	movAvg->fSum = 0.0f;
	movAvg->uIndex = 0;

	// set buff to 0.0 float (not 0x00 hex)
	for (uint32_t i=0; i<MOVAVG_SIZE; i++)
		movAvg->fBuff[i] = 0.0f;
}



float movAvgAddSample(struct sMovAvg* movAvg, float newSample)
{
	// remove oldest sample from sum
	movAvg->fSum -= movAvg->fBuff[movAvg->uIndex];
	// replace the old sample with new
	movAvg->fBuff[movAvg->uIndex] = newSample;
	// add newest sample to sum
	movAvg->fSum += newSample;
	// point to next (oldest) sample
	movAvg->uIndex++;
	// wrap buffer
	if (movAvg->uIndex >= MOVAVG_SIZE)
	{
		// numeric error may accumulate and it may be necessary to re-calculate the sum after some time
		movAvg->fSum = 0.0f;
		for (uint32_t i=0; i<MOVAVG_SIZE; i++)
			movAvg->fSum += movAvg->fBuff[i];

		movAvg->uIndex = 0;
	}

	return movAvg->fSum / ((float)MOVAVG_SIZE);
}



/*
 * NOTE:	make sure this is executed in interrupt and don't preemptioned by
 * 			routines using (System.meas). When using MovAvg filter, this routine
 * 			stores in System.meas temporary (wrong) result to allow log
 * 			highFreqLogger before filter. This can be modified to use local
 * 			variable, but then logger must be modified to log variable given in
 * 			parameter instead the global System.meas.
 */
void calcualteSamples(void)
{
#ifdef MCU_HIGH

	#ifdef USE_MOVAVG_UE_MCUHIGH
		System.meas.fExtractVolt = movAvgAddSample(&movAvgUe, fCoeffUe.gain * (System.ads.data.channel0 - fCoeffUe.offset));
	#else
		System.meas.fExtractVolt = fCoeffUe.gain * (System.ads.data.channel0 - fCoeffUe.offset);
	#endif

	#ifdef USE_MOVAVG_UF_MCUHIGH
		System.meas.fFocusVolt = movAvgAddSample(&movAvgUf, fCoeffUf.gain * (System.ads.data.channel1 - fCoeffUf.offset));
	#else
		System.meas.fFocusVolt = fCoeffUf.gain * (System.ads.data.channel1 - fCoeffUf.offset);
	#endif

#else // MCU_LOW

	System.meas.fAnodeCurrent = fCoeffIa.gain * (System.ads.data.channel0 - fCoeffIa.offset);
	System.meas.fCathodeVolt = fCoeffUc.gain * (System.ads.data.channel1 - fCoeffUc.offset);

	#ifdef LOGGER_BEFORE_FILTER
		if ((System.ref.loggerMode == LOGGER_HF_UC_STEADY)||(System.ref.loggerMode == LOGGER_HF_UC_STARTUP))
			loggerHighFreqSample(); /* Turn this on for sampling BEFORE filter */
	#endif

	#ifdef USE_MOVAVG_IA_FILTER
		System.meas.fAnodeCurrent = movAvgAddSample(&movAvgIa, System.meas.fAnodeCurrent);
	#endif

	#ifdef USE_MOVAVG_UC_FILTER
		System.meas.fCathodeVolt = movAvgAddSample(&movAvgUc, System.meas.fCathodeVolt);
	#endif

	#ifdef LOGGER_AFTER_FILTER
		if ((System.ref.loggerMode == LOGGER_HF_UC_STEADY)||(System.ref.loggerMode == LOGGER_HF_UC_STARTUP))
			loggerHighFreqSample(); /* Turn this on for sampling AFTER filter */
	#endif

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
