/*
 * regulator.h
 *
 *  Created on: Dec 27, 2020
 *      Author: lukasz
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pid_controller.h"
#include "stm32l4xx_hal.h"

/* Exported types ------------------------------------------------------------*/

#define LOGGER_250ms	// else 10 ms

enum eLoggerMode
{
	LOGGER_IA,
	LOGGER_UE,
};

enum ePwmChannel
{
	PWM_CHANNEL_UC = TIM_CHANNEL_1,	// TIM1 CH1N
	PWM_CHANNEL_UE = TIM_CHANNEL_2,	// TIM1 CH2
	PWM_CHANNEL_UF = TIM_CHANNEL_3,	// TIM1 CH3
	PWM_CHANNEL_PUMP = TIM_CHANNEL_4,	// TIM1 CH4
	REG_IA,		// used in regulator functions, not PWM
};

extern PIDControl pidUc, pidUe, pidUf, pidIa;

/* Exported snippets ---------------------------------------------------------*/

static inline void pwmSetDuty(enum ePwmChannel PWM_CHANNEL_, float duty)
{
	if (duty > 1.0f)
	{
		return;
	}

	switch (PWM_CHANNEL_)
	{
	case PWM_CHANNEL_UC:
		TIM1->CCR1 = (uint32_t)(65535.0 * duty);
		break;
	case PWM_CHANNEL_UE:
		TIM1->CCR2 = (uint32_t)(65535.0 * duty);
		break;
	case PWM_CHANNEL_UF:
		TIM1->CCR3 = (uint32_t)(65535.0 * duty);
		break;
	case PWM_CHANNEL_PUMP:
		TIM1->CCR4 = (uint32_t)(65535.0 * duty);
		break;
	case REG_IA:
		break;
	}
}

/* Exported functions --------------------------------------------------------*/

void regulatorInit(void);
void regulatorInitCurrent(void);
void regulatorDeInit(void);

void regulatorPeriodCallback(void);
void pwmSetVoltManual(enum ePwmChannel PWM_CHANNEL_, float voltage);
void pidMeasOscPeriod(enum ePwmChannel PWM_CHANNEL_);	// for PID tuning

void loggerInit(void);
void loggerPeriod(void);

void sweepUeInit(void);
void sweepUePeriod(void);
void sweepUeExit(bool success);

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
