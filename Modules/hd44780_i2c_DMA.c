/*
 * hd44780_i2c_DMA.c
 *
 *  Created on: Aug 5, 2020
 *      Author: lukasz
 *
 * HOW to use this driver
 * 1. call HD44780_Init()
 * 3. in HD44780_Init() is called init function of I2C with DMA TX request - must provide one.
 * 3. should work.
 *
 * In case of overlapping messages (new msg when previous is still in progress): waits in blocking mode.
 * In case of transmission hang/crash: resets peripheral after timeout.
 * In case of too long message: should print only begin part that fits in buffer, cut the rest.
 *
 * https://www.playembedded.org/blog/hd44780-backpack-stm32/
 * https://github.com/lcdproc/lcdproc/blob/master/server/drivers/hd44780-i2c.c
 */

#include "hd44780_i2c_DMA_link.h"
#include "hd44780_i2c_DMA.h"
#include <string.h>

/**************************** Exported variables ******************************/

struct HD44780_sDMAdata HD44780_DMAdata;

/******************** Private variables, types & defines **********************/

const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};

enum eCommand
{
	HD44780_ClearDisplay       = (1u << 0),	/* execution time ? */
	HD44780_ReturnHome         = (1u << 1),	/* 1.52 ms */
	HD44780_EntryModeSet       = (1u << 2),	/* 37 us */
	HD44780_DisplayControl     = (1u << 3),	/* 37 us */
	HD44780_CursorShift        = (1u << 4),	/* 37 us */
	HD44780_FunctionSet        = (1u << 5),	/* 37 us */
	HD44780_SetCGRAMaddr       = (1u << 6),	/* 37 us */
	HD44780_SetDDRAMaddr       = (1u << 7)
};

/* Flags for display entry mode.
 * Sets cursor move direction and specifies display shift. */
enum eEntryMode
{
	EntryMode_RIGHT            = 0x00U,
	EntryMode_LEFT             = 0x02U,
	EntryMode_SHIFTINCREMENT   = 0x01U,
	EntryMode_SHIFTDECREMENT   = 0x00U,
};

enum eDisplayControl
{
	DisplayControl_DISPLAYON   = 0x04U,
	DisplayControl_CURSORON    = 0x02U,
	DisplayControl_BLINKON     = 0x01U,
};

enum eCursorShift
{
	CursorShift_DISPLAYMOVE    =  0x08U,
	CursorShift_CURSORMOVE     =  0x00U,
	CursorShift_MOVERIGHT      =  0x04U,
	CursorShift_MOVELEFT       =  0x00U,
};

enum eFunctionSet
{
	FunctionSet_8BITMODE       =  0x10U,
	FunctionSet_4BITMODE       =  0x00U,
	FunctionSet_2LINE          =  0x08U,
	FunctionSet_5x10DOTS       =  0x04U,
	FunctionSet_5x8DOTS        =  0x00U,
};

enum eDataType
{
	DataType_data              = 0x00,
	DataType_command
};

/* Private HD44780 structure */
static struct
{
	uint8_t DisplayControl;
	uint8_t FunctionSet;	/* interface data length, display lines and font size */
	uint8_t EntryMode;		/* cursor move direction, display shift */
	uint8_t Rows;			/* display size */
	uint8_t Cols;			/* display size */
	uint8_t currentX;		/* cursor pos */
	uint8_t currentY;		/* cursor pos */
} HD44780_State;

/* Assign pins of PCF8574 */
enum ePin
{
	PIN_RS                     = (1U << 0),	/* Register select bit: 0-command, 1-data */
	PIN_RW                     = (1U << 1),	/* Read/Write bit: 0-write, 1-read */
	PIN_EN                     = (1U << 2),	/* Enable bit */
	PIN_BACKLIGHT              = (1U << 3),
	PIN_D4                     = (1U << 4),
	PIN_D5                     = (1U << 5),
	PIN_D6                     = (1U << 6),
	PIN_D7                     = (1U << 7)
};



/***************************** Private functions ******************************/



/* Clears buffers etc.
 * Call before every transmission involving DMA.
 */
static void dma_init(void)
{
	/* wait for previous transmission to end */
	uint32_t previousTick = System.timer.tick;
	while(HD44780_DMAdata.busy)
	{
		if ((System.timer.tick - previousTick) > 50)
		{	/* reset peripherals */
			I2C_Cmd(I2C1, DISABLE);							/* PE bit in CR1 */
			I2C_DMACmd(I2C1, I2C_DMAReq_Tx, DISABLE);
			DMA_Cmd(DMA1_Channel6, DISABLE);
			I2C_DMACmd(I2C1, I2C_DMAReq_Tx, ENABLE);
			I2C_Cmd(I2C1, ENABLE);							/* PE bit in CR1 */
			SPAM(("HD44780 driver crash\n"));
			break;
		}
	}

	/* lock begin of new transmission */
	HD44780_DMAdata.busy = true;

	/* init variables */
	HD44780_DMAdata.index = 0;
}





/*
 *  add data to DMA buffer
 */
static void loadCommandToDMAbuff(enum eDataType DataType_, uint8_t byte)
{
	if ( HD44780_DMAdata.index + 4 >= HD44780_DMA_BUFF_LEN)
		return;

	/* first byte - data MSB & signals */
	{
		HD44780_DMAdata.buff[HD44780_DMAdata.index] = PIN_EN + PIN_BACKLIGHT;	/* write: PIN_RW = 0 */

		if (DataType_ == DataType_data)
			HD44780_DMAdata.buff[HD44780_DMAdata.index] |= PIN_RS;		/* RS = 1 for data, else if command RS = 0 */

		HD44780_DMAdata.buff[HD44780_DMAdata.index] |= (uint8_t)(byte & (uint8_t)0xF0);	/* set D4-D7 */
	}
	/* second byte - E falling edge */
	{
		HD44780_DMAdata.buff[HD44780_DMAdata.index+1] = (uint8_t)(HD44780_DMAdata.buff[HD44780_DMAdata.index] & ~(PIN_EN));
	}
	/* third byte - data LSB & E high */
	{
		HD44780_DMAdata.buff[HD44780_DMAdata.index+2] = HD44780_DMAdata.buff[HD44780_DMAdata.index];
		HD44780_DMAdata.buff[HD44780_DMAdata.index+2] &= 0x0F;		/* clear D4-D7 */
		HD44780_DMAdata.buff[HD44780_DMAdata.index+2] |= (uint8_t)(byte << 4);	/* set D4-D7 */
	}
	/* four byte - E falling edge */
	HD44780_DMAdata.buff[HD44780_DMAdata.index+3] = (uint8_t)(HD44780_DMAdata.buff[HD44780_DMAdata.index+2] & ~(PIN_EN));

	HD44780_DMAdata.index += 4u;
}


/*
 * Send via I2C in blocking mode.
 */
static void send(enum eDataType DataType_, uint8_t byte)
{	/* First construct 4-bytes frame, then call I2C function. */

	uint8_t data[4];

	/* first byte - data MSB & signals */
	{
		data[0] = PIN_EN + PIN_BACKLIGHT;	/* write: PIN_RW = 0 */

		if (DataType_ == DataType_data)
			data[0] |= PIN_RS;		/* RS = 1 for data, else if command RS = 0 */

		data[0] |= (uint8_t)(byte & (uint8_t)0xF0);	/* set D4-D7 */
	}
	/* second byte - E falling edge */
	{
		data[1] = (uint8_t)(data[0] & ~(PIN_EN));
		//data[1] &= ~(PIN_EN);
	}
	/* third byte - data LSB & E high */
	{
		data[2] = data[0];
		data[2] &= 0x0F;		/* clear D4-D7 */
		data[2] |= (uint8_t)(byte << 4);	/* set D4-D7 */
	}
	/* four byte - E falling edge */
	data[3] = (uint8_t)(data[2] & ~(PIN_EN));

	i2c_sendData(data, 4);
}




/*
 * Loads cursor command to DMA buffer.
 */
static void CursorSet(uint8_t col, uint8_t row)
{
	if (row >= HD44780_State.Rows) row = 0;
	if (col >= HD44780_State.Cols) col = 0;

	HD44780_State.currentX = col;
	HD44780_State.currentY = row;

	loadCommandToDMAbuff(DataType_command,(HD44780_SetDDRAMaddr | (col + row_offsets[row])));
}



/****** Exported functions ****************************************************/


/*
 * Call it in transmission end interrupt.
 * May be interrupt from I2C or DMA.
 */
void HD44780_TransferEndCallback(void)
{
	HD44780_DMAdata.busy = false;
}



/* Init display and necessary peripherals.
 * Sends commands in blocking mode (without DMA).
 */
void HD44780_Init(uint8_t cols, uint8_t rows)
{
	memset( &HD44780_State, 0x00, sizeof(HD44780_State));
	memset( &HD44780_DMAdata, 0x00, sizeof(HD44780_DMAdata));

	HD44780_State.Rows = rows;
	HD44780_State.Cols = cols;

	/* "The busy state lasts for 10 ms after VCC rises to 4.5 V" */
	HD44780_delay_ms(20);

	/* see HD44780 chapter "Initializiing by Instruction" */
	if (HD44780_State.FunctionSet & FunctionSet_8BITMODE)
	{	/* Figure 23, page 45 : 8 bit mode */
		;
	}
	else
	{	/* Figure 24, page 46 : recover to 4 bit mode from any state */
		send(DataType_command, 0x03);
		HD44780_delay_ms(5);					/* at least 4.1 ms */
		send(DataType_command, 0x03);
		HD44780_delay_us(100);					/* at least 100 us */
		send(DataType_command, 0x03);
		send(DataType_command, 0x02);
		/* now the interface is 4-bit wide */

		/* repeat the sequence - one time is not enough */
		send(DataType_command, 0x03);
		HD44780_delay_ms(5);					/* at least 4.1 ms */
		send(DataType_command, 0x03);
		HD44780_delay_us(100);					/* at least 100 us */
		send(DataType_command, 0x03);
		send(DataType_command, 0x02);
	}

	HD44780_State.FunctionSet |= FunctionSet_5x8DOTS;
	if (rows > 1)
		HD44780_State.FunctionSet |= FunctionSet_2LINE;	/* default (0): only 1 line */
	send(DataType_command,(HD44780_FunctionSet | HD44780_State.FunctionSet));

	HD44780_State.EntryMode = EntryMode_LEFT | EntryMode_SHIFTDECREMENT;
	send(DataType_command,((uint8_t)HD44780_EntryModeSet | HD44780_State.EntryMode));

	HD44780_delay_ms(5);

	/* init I2C1 interface & DMA1 Chanel 6 - I2C1_TX request
	 * see RM 28.4.16 - "Transmission using DMA"
	 *
	 * Data is loaded to the I2C_TXDR register whenever the TXIS bit is set. In this case, TXIE bit does not need to be enabled.
	 * In master mode: START bit is programmed by software (the transmitted slave address cannot be transferred with DMA).
	 * When all data are transferred using DMA, the DMA must be initialized before setting the START bit.
	 * The end of transfer is managed with the NBYTES counter. */
	init_DMA1_Ch6();

	init_i2c1();
	/* now can transfer by DMA */
	HD44780_DisplayOn();	/* first dummy command */
	HD44780_DisplayOn();
	HD44780_Clear();

	/* LCD TEST begin */
//#ifdef DEBUG
//	uint32_t row, col;
//	char text[] = "Hello World";	/* is terminated by \0 */
//	while(1)
//	{
//		if (col++ > 18) { col = 0; row++; }
//		if (row > 4) row = 0;
//		HD44780_Puts(col,row,text);
//		delay_ms(500);
//	}
//#endif
	/* LCD TEST end */

}




/*
 * 100 us on "Hello World", core 72 MHz
 * - ca. 10 us/char
 */
void HD44780_Puts(uint8_t x, uint8_t y, char* str) {

	dma_init();

	CursorSet(x, y);

	while (*str != '\0')
	{
		if (HD44780_State.currentX >= HD44780_State.Cols)
		{
			CursorSet(0U, (uint8_t)(HD44780_State.currentY + 1U) );
		}
		if (*str == '\n')
		{
			CursorSet(HD44780_State.currentX, (uint8_t)(HD44780_State.currentY + 1U) );
		}
		else if (*str == '\r')
		{
			CursorSet(0, HD44780_State.currentY);
		}
		else
		{
			loadCommandToDMAbuff(DataType_data, *str);
			HD44780_State.currentX++;
		}
		str++;
	}
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}



/*
 *
 */
void HD44780_Clear(void)
{
	dma_init();
	loadCommandToDMAbuff(DataType_command, (uint8_t)HD44780_ClearDisplay);
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
	HD44780_delay_ms(20);
}

void HD44780_DisplayOn(void)
{
	dma_init();
	HD44780_State.DisplayControl |= DisplayControl_DISPLAYON;
	loadCommandToDMAbuff(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}

void HD44780_DisplayOff(void)
{
	dma_init();
	HD44780_State.DisplayControl &= (unsigned char)(~DisplayControl_DISPLAYON);
	loadCommandToDMAbuff(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}

void HD44780_BlinkOn(void)
{
	dma_init();
	HD44780_State.DisplayControl |= DisplayControl_BLINKON;
	loadCommandToDMAbuff(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}

void HD44780_BlinkOff(void)
{
	dma_init();
	HD44780_State.DisplayControl &= (unsigned char)(~DisplayControl_BLINKON);
	loadCommandToDMAbuff(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}

void HD44780_CursorOn(void)
{
	dma_init();
	HD44780_State.DisplayControl |= DisplayControl_CURSORON;
	loadCommandToDMAbuff(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}

void HD44780_CursorOff(void)
{
	dma_init();
	HD44780_State.DisplayControl &= (unsigned char)(~DisplayControl_CURSORON);
	loadCommandToDMAbuff(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}

void HD44780_ScrollLeft(void)
{
	dma_init();
	loadCommandToDMAbuff(DataType_command,(HD44780_CursorShift + CursorShift_DISPLAYMOVE + CursorShift_MOVELEFT));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}

void HD44780_ScrollRight(void)
{
	dma_init();
	loadCommandToDMAbuff(DataType_command,(HD44780_CursorShift + CursorShift_DISPLAYMOVE + CursorShift_MOVERIGHT));
	i2c_sendDataDMA(HD44780_DMAdata.buff, HD44780_DMAdata.index);
}


/*********************************END OF FILE**********************************/
