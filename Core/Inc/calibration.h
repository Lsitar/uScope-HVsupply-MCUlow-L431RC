/*
 * calibration.h
 *
 *  Created on: Dec 30, 2020
 *      Author: Lukasz Sitarek
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Config --------------------------------------------------------------------*/

#define MOVAVG_SIZE		33		// one size for all (may be modified to individual...)

#define USE_MOVAVG_IA_FILTER
#define USE_MOVAVG_UC_FILTER
/*
 * NOTE: at 250 SPS samples from MCU_HIGH could be filtered on LOW side after
 * 		uart transmission. At 2 kSPS ca. 75 % samples is lost on uart throughput
 * 		and the filter must be applied on HIGH side before uart communication.
 */
//#define USE_MOVAVG_UE_MCULOW
#define USE_MOVAVG_UE_MCUHIGH
//#define USE_MOVAVG_UF_MCULOW
#define USE_MOVAVG_UF_MCUHIGH

#if defined (USE_MOVAVG_UE_MCULOW) && defined (USE_MOVAVG_UE_MCUHIGH)
	#error "don't use the UE filter twice"
#endif
#if defined (USE_MOVAVG_UF_MCULOW) && defined (USE_MOVAVG_UF_MCUHIGH)
	#error "don't use the UF filter twice"
#endif

/* Exported types ------------------------------------------------------------*/

struct sMovAvg
{
	float fSum;
	float fBuff[MOVAVG_SIZE];
	uint32_t uIndex;
};

extern struct sMovAvg	movAvgIa, 		\
						movAvgUc, 		\
						movAvgUe, 		\
						movAvgUf,		\
						movAvgAdcBatt;	\

/* Exported functions --------------------------------------------------------*/

/*
 * Call it once at system init before receiving samples starts.
 *
 * @brief	Loads coefficients from constants to variables.
 * 			Well, since there's no calibration function now it's unneccessary
 * 			overhead, but it adds flexibility in programming and is For Future
 * 			Use.
 */
void initCoefficients(void);



/*
 * Call it once at system init before receiving samples starts.
 *
 * @brief	Initializes Moving Average filter of Anode current samples.
 * 			Re-sets its variables to zero.
 */
void movAvgInit(struct sMovAvg* movAvg);


float movAvgAddSample(struct sMovAvg* movAvg, float newSample);



/*
 * Call it after receiving samples from ADS.
 *
 * @note	Data source and target is global System struct (must save samples to
 * 			System first).
 */
void calcualteSamples(void);



/*
 * Call it in tandem with voltage regulator routine.
 *
 * @brief	Returns required PWM duty of Pump for given output voltage.
 * 			It is corrected by calibration coefficients.
 * 			It should be used when pump voltage is set in open loop (not
 * 			regulated in closed loop). In current hardware there's no feedback
 * 			from pump voltage.
 */
float getPumpDuty(float voltage);



/* [DEBUG only]
 * Call it after every received samples.
 *
 * @brief	Sums hard-coded number of samples and averages it.
 * 			Used to measure offsets of all channels of ADS (only Ch0 and Ch1 now).
 * 			For best results, before this measurement user should short circuit
 * 			the corresponding outputs (e.g. Ia to GND, Ue to HVGND etc.).
 *
 * @note	For now, there is no possibility to call it from UI and is only used
 * 			with debugger, activated by un-commenting call and outputs to SWO.
 * 			It is possible to integrate it into UI, to allow re-calibrate
 * 			offsets on demand.
 */
void calibOffset(void);



#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
