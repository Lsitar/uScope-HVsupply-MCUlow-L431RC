/*
 * hd44780_i2c_DMA_link.c
 *
 *  Created on: Dec 18, 2020
 *      Author: Lukasz Sitarek
 */

#include "hd44780_i2c_DMA_link.h"

void HD44780_delay_ms(unsigned int ms)
{
	;
}

void HD44780_delay_us(unsigned int us)
{
	;
}

/*
 * Control of enable / disable of DMA
 */
void HD44780_DMA_cmd(FunctionalState state)
{
	DMA_Cmd(DMA1_Channel6, state );
}





/*********************************END OF FILE**********************************/
