/*
 * calibration.h
 *
 *  Created on: Dec 30, 2020
 *      Author: lukasz
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Exported functions --------------------------------------------------------*/

void calcualteSamples(void);
void initCoefficients(void);
void calibOffset(void);
float getPumpDuty(float voltage);

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
