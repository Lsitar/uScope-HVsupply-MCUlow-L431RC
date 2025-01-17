/*
 * utilities.c
 *
 *  Created on: Dec 13, 2020
 *      Author: Lukasz Sitarek
 */

#include <stdint.h>
#include <string.h>
#include "typedefs.h"
#include "hd44780_i2c.h"
#include "utilities.h"

uint64_t *flashPtr;

//#define FLASH_DATA_PARTS	((uint32_t)((sizeof(tsRegulatedVal)/sizeof(uint64_t))+1))

//union
//{
//	struct
//	{
//		tsRegulatedVal data;
//		uint8_t crc;
//	} flashData;
//
//	uint64_t buff64[FLASH_DATA_PARTS];
//	uint8_t buff8[FLASH_DATA_PARTS * sizeof(uint64_t)];
//} tuFlashData;

tuFlashData uFlashData;

/*
 * The Flash memory contains 128 pages of 2 Kbyte. (256k total).
 * It is organized as 72-bit wide memory cells (64 bits plus 8 ECC bits).
 * It is erased by pages.
 * It is programmed by 72 bits at a time (64 bits + 8 bits ECC).
 *
 * STANDARD PROGRAMMING
 * It is only possible to program double word (2 x 32-bit = 8 B data). Any attempt to
 * write byte or half-word will set SIZERR flag. Any attempt to write a double
 * word which is not aligned with a double word address will set PGAERR flag.
 * Programming in a previously programmed (and not erased) address is not allowed
 * except if the data to write is full zero, and any attempt will set PROGERR flag.
 *
 * FAST PROGRAMMING
 * this mode programs a row (32 x double word = 256 B) : 32x standard programming size.
 *
 * Flash memory range 0x0800 0000 to 0x0804 0000
 * (2^18 addresses = 2^7 pages of 2^11 bytes)
 *
 * 256000 = 0x3E800
 * 256k = 256 * 1024 = 262144 = 0x40000
 * 254k = 262144 - 1024 = 261120 = 0x3FC00
 * 252k = 262144 - 2048 = 260096 = 0x3F800
 *
 * Linker sections:
 * FLASH    (rx)    : ORIGIN = 0x8000000,   LENGTH = 252K
 * FLASHSTORAGE1    (rx)    : ORIGIN = 0x803F800,   LENGTH = 2K
 * FLASHSTORAGE2    (rx)    : ORIGIN = 0x803FC00,   LENGTH = 2K
 */
#define FLASHSTORAGE1	0x803F800
#define FLASHSTORAGE2	0x803FC00

/* Private functions ---------------------------------------------------------*/

/*
 * @return 0 - ok, 1 - error
 */
static bool flashErase(uint32_t addr)
{
	bool error = false;
	HAL_StatusTypeDef status;
	FLASH_EraseInitTypeDef config = {0};
	uint32_t uPageError = 0xFFFFFFFF;

	config.Banks = FLASH_BANK_1;
	config.NbPages = 1;
	config.TypeErase = FLASH_TYPEERASE_PAGES;

	if (addr == FLASHSTORAGE1)
		config.Page = 126;
	else if (addr == FLASHSTORAGE2)
		config.Page = 127;
	else
		return true;

	status = HAL_FLASH_Unlock();
	if (status != HAL_OK)
	{
		SPAM(("Flash err unlock\n"));
		error = true;
	}
	else
	{
		status = HAL_FLASHEx_Erase(&config, &uPageError);
		if (status != HAL_OK)
			error = true;

		status = HAL_FLASH_Lock();
		if (status != HAL_OK)
		{
			SPAM(("Flash err lock\n"));
			error = true;
		}
	}
	if (uPageError != 0xFFFFFFFF)
	{
		error = true;
		SPAM(("Flash erase page %u error\n", config.Page));
	}
	else
	{
		SPAM(("Flash erase page %u success\n", config.Page));
	}
	return error;
}

/* Exported functions --------------------------------------------------------*/

#pragma GCC push_options
#pragma GCC optimize("O0")
void delay_us(uint32_t us)
{
//	uint32_t cnt = (uint32_t)((uint64_t)us * (uint64_t)SystemCoreClock / (uint64_t)21500000);
//	while ( cnt-- ) ;//__NOP();	/* 6 instructions */
	while(us--) {
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP();
		/* @72 MHz : 52x NOP @ Nucleo F303RE - got experimentally */

		__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP(); __NOP();
		/* @80 MHz : 66x NOP @ stm32 L431 - got experimentally */
	}
}
#pragma GCC pop_options



void ledDemo(void)
{
	static uint32_t uTimeTick = 0;
	static uint32_t uLedNo = 0;

	if (HAL_GetTick() - uTimeTick > 250)	// every 250 ms
	{
		if (uLedNo == 0)
			ledBlue(TOGGLE);
		else if (uLedNo == 1)
			ledGreen(TOGGLE);
		else if (uLedNo == 2)
			ledOrange(TOGGLE);
		else
		{
			ledRed(TOGGLE);
		}

		if (uLedNo >= 3)
			uLedNo = 0;
		else
			uLedNo++;

		uTimeTick = HAL_GetTick();
	}
}

/* Blink red LED 'errNo' times. Blink the sequence twice. */
void ledError(uint32_t errNo)
{
	const uint32_t delay = 250;

	uint32_t repeats = errNo;

	while (repeats--)
	{
		ledRed(ON);
		HAL_Delay(delay);
		ledRed(OFF);
		HAL_Delay(delay);
	}

	HAL_Delay(delay + delay + delay + delay);

	while (errNo--)
	{
		ledRed(ON);
		HAL_Delay(delay);
		ledRed(OFF);
		HAL_Delay(delay);
	}

	HAL_Delay(delay + delay + delay + delay);
}



_OPT_O3 uint8_t crc8(const uint8_t *data, uint32_t length)
{
	uint8_t crc = 0x00;
	uint8_t extract;
	uint8_t sum;

	for(uint32_t i=0; i<length; i++)
	{
		extract = *data;
		for (uint32_t tempI = 8; tempI; tempI--)
		{
			sum = (crc ^ extract) & 0x01;
			crc >>= 1;
			if (sum)
				crc ^= 0x8C;
			extract >>= 1;
		}
		data++;
	}
	return crc;
}



/*
 * @return 0 - success, 1 - error
 */
static bool _flashSave(const uint64_t *data)
{
	HAL_StatusTypeDef status;
	bool bError = false;

	status = HAL_FLASH_Unlock();
	if (status != HAL_OK)
	{
		SPAM(("Flash err unlock\n"));
		bError = true;
	}
	else
	{
		for (uint32_t i=0; i<FLASH_DATA_PARTS; i++)
		{
			status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FLASHSTORAGE2 + (i*sizeof(uint64_t)), data[i]);
			if (status != HAL_OK)
			{
				SPAM(("Flash err program\n"));
				bError = true;
				break;
			}
			else
			{
				SPAM(("Part %u saved\n", i));
			}
		}
		status = HAL_FLASH_Lock();
		if (status != HAL_OK)
			SPAM(("Flash err lock\n"));
	}
	__NOP();
	return bError;
}



void flashSaveConfig(void)
{
	if (flashErase(FLASHSTORAGE2) == false)
	{
		memcpy(&uFlashData.flashData.data, &System.ref, sizeof(tsRegulatedVal));
		uFlashData.flashData.crc = crc8(uFlashData.buff8, sizeof(tsRegulatedVal));

		_flashSave(uFlashData.buff64);
		SPAM(("Config saved!\n"));
	}
}



/*
 * Loads settings from flash to global uFlashData, if crc is correct.
 * @return 0 - success, 1 - error
 */
bool flashReadConfig(void)
{
	tuFlashData localFlashData;

	memcpy(&localFlashData.buff8, (void*)FLASHSTORAGE2, sizeof(uFlashData.flashData));

	uint8_t crc = crc8(localFlashData.buff8, sizeof(tsRegulatedVal));

	if (crc == localFlashData.flashData.crc)
	{
		memcpy(uFlashData.buff8, localFlashData.buff8, sizeof(tuFlashData));
		return false;
	}
	else
	{
		return true;
	}
}

/************************ (C) COPYRIGHT LSITA ******************END OF FILE****/
