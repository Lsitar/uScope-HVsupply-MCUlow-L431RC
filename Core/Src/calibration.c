/*
 * calibration.c
 *
 *  Created on: Dec 30, 2020
 *      Author: lukasz
 */

#include "calibration.h"
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
	fCoeffUc.gain = fCoeffUcDefault * (935.0f/900.0f);
	fCoeffUc.offset = -29325;

	fCoeffUe.gain = fCoeffUeDefault;
	fCoeffUe.offset = 0;

	fCoeffUf.gain = fCoeffUfDefault;
	fCoeffUf.offset = 0;

	fCoeffIa.gain = fCoeffIaDefault;
	fCoeffIa.offset = -37850;

	fCoeffUp.gain = fCoeffUpDefault;
	fCoeffUp.offset = 0;
}



/*
 * Call this function after every received samples.
 */
void calibOffset(void)
{
	static int32_t sumCh0 = 0, sumCh1 = 0;
	static index = 0;
	const int32_t samplesNo = 10000;

	sumCh0 += System.ads.data.channel0;
	sumCh1 += System.ads.data.channel1;
	index++;

	if (index > samplesNo)
	{
		__NOP();
		sumCh0 /= samplesNo;
		sumCh1 /= samplesNo;
		SPAM(("Ch0 offset: %i\n", sumCh0));	// MCU_LOW: Ia
		SPAM(("Ch1 offset: %i\n", sumCh1)); // MCU_LOW: Uc
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
