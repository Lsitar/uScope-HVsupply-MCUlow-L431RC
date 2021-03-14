/*
 * logger.h
 *
 *  Created on: Jan 29, 2021
 *      Author: Lukasz Sitarek
 */

#pragma once

// config

#define LOGGER_250ms	// 2 kS --> 500 s = 8 min 33 s
//#define LOGGER_10ms		// 2 kS --> 20 s
#define LOGGER_HF

//#define LOGGER_BEFORE_FILTER
#define LOGGER_AFTER_FILTER

#ifdef MCU_LOW
//	#define LOGGER_LOG_HF_IA
	#define LOGGER_LOG_HF_UC
#endif

#if (defined (LOGGER_10ms) && defined (LOGGER_250ms)) || ( !defined (LOGGER_10ms) && !defined (LOGGER_250ms))
	#error "wrong Logger config - use 10ms or 250ms sampling"
#endif

#if (defined (LOGGER_BEFORE_FILTER) && defined (LOGGER_AFTER_FILTER)) || ( !defined (LOGGER_BEFORE_FILTER) && !defined (LOGGER_AFTER_FILTER))
	#error "wrong Logger config - use LOGGER_BEFORE_FILTER or LOGGER_AFTER_FILTER"
#endif

#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

enum eLoggerMode
{
	LOGGER_IA_UE_UF,
	LOGGER_HF_UC_STEADY,
	LOGGER_HF_UC_STARTUP,
};

/* Exported functions --------------------------------------------------------*/

void loggerInit(void);
void loggerPeriod(void);
void loggerHighFreqSample(void);

void sweepUeInit(void);
void sweepUePeriod(void);
void sweepUeExit(bool success);

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
