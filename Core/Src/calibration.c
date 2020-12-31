/*
 * calibration.c
 *
 *  Created on: Dec 30, 2020
 *      Author: lukasz
 */

#include "calibration.h"
#include "communication.h"
#include "main.h"
#include "typedefs.h"

/* Private defines -----------------------------------------------------------*/

typedef struct
{
	float gain;
	int32_t offset;
} tsCoeff;

// default coefficients
const float fCoeffUcDefault = 1.2f * (100e+3/50e+3) * (500e+6/180e+3) / ((float)(1u << 23));	// [V/bit] // 1.08 V on ADC at 6 kV in
const float fCoeffUeDefault = 1.2f * (100e+3/24e+3) * (100e+6/76744.2f) / ((float)(1u << 23));	// [V/bit] // 1.16 V on ADC at 6 kV in
const float fCoeffUfDefault = 1.2f * (100e+3/24e+3) * (100e+6/76744.2f) / ((float)(1u << 23));	// [V/bit] // 1.16 V on ADC at 6 kV in
const float fCoeffIaDefault = (-1.0f) * 1.2f * (100e+3/47e+3) * (1.0f/51e+3) / ((float)(1u << 23));	// [A/bit] // 1.1985 V on ADC at 50 uA in
const float fCoeffUpDefault = 1.0f/((6000.0f/12.0f) * (16300.0f/4300.0f) * 3.3f);	// [duty/V] // 0.959286 duty at 6 kV out and 3.3V ref

// coeff variables
tsCoeff fCoeffUc;
tsCoeff fCoeffUe;
tsCoeff fCoeffUf;
tsCoeff fCoeffIa;
tsCoeff fCoeffUp;

/* Exported functions --------------------------------------------------------*/

void initCoefficients(void)
{
	// load calibration coefficients
	// MCU_LOW Ch0
	fCoeffIa.gain = fCoeffIaDefault;
	fCoeffIa.offset = -37850;

	// MCU_LOW Ch1
	fCoeffUc.gain = fCoeffUcDefault * (935.0f/900.0f);
	fCoeffUc.offset = -29325;

	// MCU_HIGH Ch0
	fCoeffUe.gain = fCoeffUeDefault * (759.6f/735.4f);	// (meas external ref / meas ADS)
	fCoeffUe.offset = -48034;

	// MCU_HIGH Ch1
	fCoeffUf.gain = fCoeffUfDefault * (734.0f/708.2f);	// (meas external ref / meas ADS)
	fCoeffUf.offset = -39380;

	fCoeffUp.gain = fCoeffUpDefault;
	fCoeffUp.offset = 0;
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



void calcualteSamples(void)
{
#ifdef MCU_HIGH
//    System.meas.fExtractVolt = fCoeffUeDefault * System.ads.data.channel0;
//    System.meas.fFocusVolt = fCoeffUfDefault * System.ads.data.channel1;
    System.meas.fExtractVolt = fCoeffUe.gain * (System.ads.data.channel0 - fCoeffUe.offset);
    System.meas.fFocusVolt = fCoeffUf.gain * (System.ads.data.channel1 - fCoeffUf.offset);

    if (uartIsIdle())
    	sendResults();

#else // MCU_LOW
//    System.meas.fAnodeCurrent = fCoeffIaDefault * System.ads.data.channel0;
//    System.meas.fCathodeVolt = fCoeffUcDefault * System.ads.data.channel1;
    System.meas.fAnodeCurrent = fCoeffIa.gain * (System.ads.data.channel0 - fCoeffIa.offset);
    System.meas.fCathodeVolt = fCoeffUc.gain * (System.ads.data.channel1 - fCoeffUc.offset);
//    pidMeasOscPeriod();
#endif // MCU_HIGH
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
