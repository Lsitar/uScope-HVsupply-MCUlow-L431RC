/*
 * hd44780_i2c_DMA_link.h
 *
 *  Created on: Dec 18, 2020
 *      Author: lukasz
 *
 *  Description
 *  Link file between portable hd44780_i2c_DMA implementation and target MCU for
 *  all custom stuff.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// MCU definitions
#if defined  (USE_HAL_DRIVER)
	#include "stm32l4xx_hal.h"
#elif defined  (USE_STDPERIPH_DRIVER)
	#include "stm32f30x.h"
#else
	#error "HD44780 library needs HAL driver or StdPeriph driver"
#endif
// pin definitions
#include "main.h"


//#include "init.h"	/* for DMA init function */
//#include "i2c.h"	/* for I2C functions */

/* Exported functions prototypes ---------------------------------------------*/

void HD44780_delay_ms(unsigned int ms);
void HD44780_delay_us(unsigned int us);


#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
