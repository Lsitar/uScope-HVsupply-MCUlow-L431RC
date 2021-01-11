/*
 */

#include <stdbool.h>
#include "hd44780_i2c.h"
#include "stm32l4xx_hal.h"
#include "utilities.h"		// delay
#include "main.h"			// pin definition

/****** Private types & defines ***********************************************/

//#define HD44780_BUFF_SIZE	1022	// 1 kB is too low
#define HD44780_BUFF_SIZE	(1024 + 512 + 256)	// 1.75 kB @4 Hz refresh rate is ok



static struct
{
	uint8_t	transmitBuff[HD44780_BUFF_SIZE+1];
	uint32_t tail;		// consumer, here reads
	uint32_t head;		// producer, here writes
//	uint32_t freeSpace;		// number of free bytes
//	uint32_t NumOfDataToSend;
//	uint32_t CounterOfSentData;
	bool flag_idle;		/* this flag is set in Tx ready interrupt, when there's no new data to send.
	 	 	 	 	 	 	 	 	 	 	 This flag is checked at loading data to circular buffer, and Tx is triggered if idle */
} HD44780_dataBuff;



static void buffAdd(uint8_t data)
{
//	HD44780_dataBuff.transmitBuff[HD44780_dataBuff.head++] = data;
//	if (HD44780_dataBuff.head == HD44780_BUFF_SIZE)
//		HD44780_dataBuff.head = 0;

	// check free space
	uint32_t freeSpace;
	if (HD44780_dataBuff.head >= HD44780_dataBuff.tail)
		freeSpace = HD44780_BUFF_SIZE - HD44780_dataBuff.head + HD44780_dataBuff.tail;
	else
		freeSpace = HD44780_dataBuff.tail - HD44780_dataBuff.head;

	if (freeSpace < 10)
		SPAM(("LCD buffer overflow!\n"));
		// TODO maybe LCD reset. It enters here sometimes, no matter how big the buffer. It may be caused somehow by EMI.
	else
	{
		HD44780_dataBuff.transmitBuff[HD44780_dataBuff.head++] = data;
		if (HD44780_dataBuff.head == HD44780_BUFF_SIZE)
			HD44780_dataBuff.head = 0;
	}

	/* NOTE: below snippet is buggy. When freeSpace is accesed from buffAdd
	 * (normal routine) and buffGet (interrupt) incrementations in interrupt may
	 * be lost due to non-atomic operation in normal routine.
	 */
//	if (HD44780_dataBuff.freeSpace)
//		HD44780_dataBuff.freeSpace--;
//	else
//		SPAM(("LCD buffer overflow!\n"));
}



_OPT_O3 int32_t buffGet(void)
{
	int32_t data;

	if (HD44780_dataBuff.tail == HD44780_dataBuff.head)
	{	// buffer empty
		return -1;
	}
	else
	{	// get data
		data = HD44780_dataBuff.transmitBuff[HD44780_dataBuff.tail++];
		// wrap buffer
		if (HD44780_dataBuff.tail == HD44780_BUFF_SIZE)
		{
			HD44780_dataBuff.tail = 0;
		}
//		HD44780_dataBuff.freeSpace++;
	}
	return data;
}



static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};



enum eCommand {
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
enum eEntryMode {
	EntryMode_RIGHT          = 0x00U,
	EntryMode_LEFT           = 0x02U,
	EntryMode_SHIFTINCREMENT = 0x01U,
	EntryMode_SHIFTDECREMENT = 0x00U,
};



enum eDisplayControl
{
	DisplayControl_DISPLAYON	= 0x04U,
	DisplayControl_CURSORON		= 0x02U,
	DisplayControl_BLINKON		= 0x01U,
};



enum eCursorShift
{
	CursorShift_DISPLAYMOVE       =  0x08U,
	CursorShift_CURSORMOVE        =  0x00U,
	CursorShift_MOVERIGHT         =  0x04U,
	CursorShift_MOVELEFT          =  0x00U,
};



enum eFunctionSet
{
	FunctionSet_8BITMODE          =  0x10U,
	FunctionSet_4BITMODE          =  0x00U,
	FunctionSet_2LINE             =  0x08U,
	FunctionSet_5x10DOTS          =  0x04U,
	FunctionSet_5x8DOTS           =  0x00U,
};



enum eDataType
{
	DataType_data	= 0x00,
	DataType_command
};



/* Private HD44780 structure */
static struct {
	uint8_t DisplayControl;
	uint8_t FunctionSet;	/* interface data length, display lines and font size */
	uint8_t EntryMode;		/* cursor move direction, display shift */
	uint8_t Rows;			/* display size */
	uint8_t Cols;			/* display size */
	uint8_t currentX;		/* cursor pos */
	uint8_t currentY;		/* cursor pos */
} HD44780_State;

/* Assign pins of PCF8574 */
enum ePin {
	PIN_RS		= (1U << 0),	/* Register select bit: 0-command, 1-data */
	PIN_RW		= (1U << 1),	/* Read/Write bit: 0-write, 1-read */
	PIN_EN		= (1U << 2),	/* Enable bit */
	PIN_BACKLIGHT 	= (1U << 3),
	PIN_D4		= (1U << 4),
	PIN_D5		= (1U << 5),
	PIN_D6		= (1U << 6),
	PIN_D7		= (1U << 7)
};

/****** Private functions *****************************************************/

/*
 * Send one byte to LCD
 */
static void send(enum eDataType DataType_, uint8_t byte)
{
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

	buffAdd(data[0]);
	buffAdd(data[1]);
	buffAdd(data[2]);
	buffAdd(data[3]);

	if (HD44780_dataBuff.flag_idle == true)
	{	// trigger new transmit
		HAL_I2C_MasterTxCpltCallback(&hi2c1);
		HD44780_dataBuff.flag_idle = false;
	}
	else
	{	// just add to buffer
		;
	}

	//i2c_sendData(data, 4);
//	HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8574_ADDR_WRITE, data, 4);
//	HAL_Delay(10);

}



_OPT_O3 void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	static uint8_t data;

	int32_t status = buffGet();

	if (status < 0)
		HD44780_dataBuff.flag_idle = true;
	else
	{
		data = (uint8_t)(status & 0x000000FF);
		HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8574_ADDR_WRITE, &data, 1);
	}
}



static void HD44780_CursorSet(uint8_t col, uint8_t row)
{
	if (row >= HD44780_State.Rows) row = 0;
	if (col >= HD44780_State.Cols) col = 0;

	HD44780_State.currentX = col;
	HD44780_State.currentY = row;

	send(DataType_command,(HD44780_SetDDRAMaddr | (col + row_offsets[row])));

//	send(DataType_command,((uint8_t) HD44780_SetDDRAMaddr | (unsigned char)(col + row_offsets[row])));
}



/****** Exported functions ****************************************************/

void HD44780_Init(uint8_t cols, uint8_t rows)
{
	uint8_t* ptr = (uint8_t*)&HD44780_State;
	for (uint32_t i=0; i<sizeof(HD44780_State); i++)
		*ptr++ = 0;

	ptr = (uint8_t*)&HD44780_dataBuff;
	for (uint32_t i=0; i<sizeof(HD44780_dataBuff); i++)
		*ptr++ = 0;

	HD44780_dataBuff.flag_idle = true;
//	HD44780_dataBuff.freeSpace = HD44780_BUFF_SIZE;

	HD44780_State.Rows = rows;
	HD44780_State.Cols = cols;

	/* "The busy state lasts for 10 ms after VCC rises to 4.5 V" */
	HAL_Delay(20);	//delay_ms(20);

	/* see chapter "Initializiing by Instruction" */
	if (HD44780_State.FunctionSet & FunctionSet_8BITMODE)
	{	/* Figure 23, page 45 : 8 bit mode */
		;
	}
	else
	{	/* Figure 24, page 46 : recover to 4 bit mode from any state */
		send(DataType_command, 0x03);
		HAL_Delay(5);	//delay_ms(5);					/* at least 4.1 ms */
		send(DataType_command, 0x03);
		delay_us(100);					/* at least 100 us */
		send(DataType_command, 0x03);
		send(DataType_command, 0x02);
		/* now the interface is 4-bit wide */
	}

	HD44780_State.FunctionSet |= FunctionSet_5x8DOTS;

	if (rows > 1)
		HD44780_State.FunctionSet |= FunctionSet_2LINE;	/* default (0): only 1 line */

	send(DataType_command,(HD44780_FunctionSet | HD44780_State.FunctionSet));

	HD44780_DisplayOn();
	HD44780_Clear();

	HD44780_State.EntryMode = EntryMode_LEFT | EntryMode_SHIFTDECREMENT;
	send(DataType_command,((uint8_t)HD44780_EntryModeSet | HD44780_State.EntryMode));

	delay_us(4500);

}



void HD44780_Clear(void)
{
	send(DataType_command, (uint8_t)HD44780_ClearDisplay);
	// wait till all instructions will be sent to LCD
	while (HD44780_dataBuff.flag_idle != true)
		__NOP();
	delay_us(4500);
}



uint32_t HD44780_Puts(uint8_t x, uint8_t y, char* str)
{
	HD44780_CursorSet(x, y);
	uint32_t stringLen = 0;
	while (*str != '\0')
	{
		if (HD44780_State.currentX >= HD44780_State.Cols)
		{
			HD44780_CursorSet(0U, (uint8_t)(HD44780_State.currentY + 1U) );
		}
		if (*str == '\n')
		{
			HD44780_CursorSet(HD44780_State.currentX, (uint8_t)(HD44780_State.currentY + 1U) );
		}
		else if (*str == '\r')
		{
			HD44780_CursorSet(0, HD44780_State.currentY);
		}
		else
		{
			send(DataType_data, *str);
			stringLen++;
			HD44780_State.currentX++;
		}
		str++;
	}
	return stringLen;
}



void HD44780_DisplayOn(void)
{
	HD44780_State.DisplayControl |= DisplayControl_DISPLAYON;
	send(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
}



void HD44780_DisplayOff(void)
{
	HD44780_State.DisplayControl &= (unsigned char)(~DisplayControl_DISPLAYON);
	send(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
}



void HD44780_BlinkOn(void)
{
	HD44780_State.DisplayControl |= DisplayControl_BLINKON;
	send(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
}



void HD44780_BlinkOff(void)
{
	HD44780_State.DisplayControl &= (unsigned char)(~DisplayControl_BLINKON);
	send(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
}



void HD44780_CursorOn(void)
{
	HD44780_State.DisplayControl |= DisplayControl_CURSORON;
	send(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
}



void HD44780_CursorOff(void)
{
	HD44780_State.DisplayControl &= (unsigned char)(~DisplayControl_CURSORON);
	send(DataType_command,(HD44780_DisplayControl | HD44780_State.DisplayControl));
}



void HD44780_ScrollLeft(void)
{
	send(DataType_command,(HD44780_CursorShift + CursorShift_DISPLAYMOVE + CursorShift_MOVELEFT));
}



void HD44780_ScrollRight(void)
{
	send(DataType_command,(HD44780_CursorShift + CursorShift_DISPLAYMOVE + CursorShift_MOVERIGHT));
}



void HD44780_CreateChar(uint8_t location, uint8_t *data)
{
	unsigned char i;
	/* There is 8 locations available for custom characters */
	location &= 0x07;
	send(DataType_command,( (uint8_t)HD44780_SetCGRAMaddr | (uint8_t)(location << 3)) );
	
	for (i = 0; i < 8; i++) {
		send(DataType_data,(data[i]));
	}
}



void HD44780_PutCustom(uint8_t x, uint8_t y, uint8_t location)
{
	HD44780_CursorSet(x, y);
	send(DataType_data,location);
}



void HD44780_demo(void)
{
	static uint8_t row, col;
	static char pattern[] = { '*', '\0' };

	HD44780_Puts(col++, row, pattern);
	if (col >= 20)
	{
		col = 0;
		row++;
		if (row >= 4)
		{
			row = 0;
			if (pattern[0] == ' ')
				pattern[0] = '*';
			else
				pattern[0] = ' ';
		}
	}
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
