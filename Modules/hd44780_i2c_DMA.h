/**
 * hd44780_i2c_DMA.h
 *
 * @author  LSITA
 * @brief   HD44780 LCD driver via PCF8574 I/O expander
 *
 */

#pragma once

#include <stdbool.h>

/* DMA buffer
 * 4 x 20 = 80 chars,
 * 4 bytes per char = 320 B to fill whole screen at once */
#define HD44780_DMA_BUFF_LEN	255

struct HD44780_sDMAdata
{
	uint8_t	buff[HD44780_DMA_BUFF_LEN];		/* Note: I2C sending is more complicated above >255 bytes */
	uint8_t	index;
	bool	busy;
};

extern struct HD44780_sDMAdata HD44780_DMAdata;


void HD44780_TransferEndCallback(void);


void HD44780_Init(uint8_t cols, uint8_t rows);
void HD44780_Puts(uint8_t x, uint8_t y, char* str);
void HD44780_Clear(void);

void HD44780_DisplayOn(void);
void HD44780_DisplayOff(void);

void HD44780_BlinkOn(void);
void HD44780_BlinkOff(void);

void HD44780_CursorOn(void);
void HD44780_CursorOff(void);

void HD44780_ScrollLeft(void);
void HD44780_ScrollRight(void);



/*********************************END OF FILE**********************************/
