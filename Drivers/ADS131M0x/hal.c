/**
 * \copyright Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "hal.h"
#include "utilities.h"
#include "typedefs.h"



//****************************************************************************
//
// Internal function prototypes
//
//****************************************************************************



//****************************************************************************
//
// External Functions (prototypes declared in hal.h)
//
//****************************************************************************


/*
 *
 */
void InitADC(void)
{
    // IMPORTANT: Make sure device is powered before setting GPIOs pins to HIGH state.

    // Initialize GPIOs pins used by ADS131M0x
    HAL_GPIO_WritePinHigh(nSYNC_nRESET_PORT, nSYNC_nRESET_PIN);
    HAL_GPIO_WritePinHigh(nCS_PORT, nCS_PIN);

    // Initialize SPI peripheral used by ADS131M0x
    HAL_SPIEx_FlushRxFifo(&hspi1);

    // Run ADC startup function
    adsStartup();

    System.ads.ready = true;
}



/*
 * Controls the state of the /CS GPIO pin.
 *
 * @param state boolean indicating which state to set the /CS pin (0=low, 1=high)
 */
void adsSetCS(const bool state)
{
	// tdSCCS = 10 ns - delay time CS rising edge after last SCLK falling edge
	__NOP();

    if (state == true)
    	HAL_GPIO_WritePinHigh(nCS_PORT, nCS_PIN);
	else
		HAL_GPIO_WritePinLow(nCS_PORT, nCS_PIN);

    // tdCSSC = 16 ns - delay time first SCLK rising edge after CS falling edge
    __NOP();
}



/*
 * Reset pulse on RESET pin: min 2048 tCLKIN.
 * ADS resets all registers.
 */
int adsResetHard(void)
{
	HAL_GPIO_WritePinLow(nSYNC_nRESET_PORT, nSYNC_nRESET_PIN);
	delay_ms(1);		// Reset time threshold: 2048 CLKIN / 8 MHz = 256 us
	HAL_GPIO_WritePinHigh(nSYNC_nRESET_PORT, nSYNC_nRESET_PIN);
	delay_us(5);		// wait tREGACQ after reset to access registers

    // Initialize internal 'registerMap' array with device default settings
	registerMapRestoreDefaults();

    // NOTE: The ADS131M0x's next response word should be (0xFF20 | CHANCNT).
    // A different response may be an indication that the device did not reset.

	/* (OPTIONAL) Validate first response word when beginning SPI communication.
	 * Reset is confirmed by response: (0xFF20 | CHANCNT)  */
	uint16_t response = adsSendCommand(OPCODE_NULL);
	if ((response & 0xFF20) != 0xFF20)
	{
		SPAM(("ADC ERROR after reset!\n"));
//		ledError(4);
//		powerLockOff();
//		while (0xDEAD);
		return -1;
	}
	else
	{
		SPAM(("ADC reset success\n"));
		return 0;
	}
}



/*
 * Sync pulse on RESET pin: between 1 and 2047 tCLKIN.
 * ADS restarts conversion and clears results in buffers.
 */
void adsSyncPulse(void)
{
	HAL_GPIO_WritePinLow(nSYNC_nRESET_PORT, nSYNC_nRESET_PIN);
	delay_us(1);		// Reset time threshold: 1 CLKIN / 8 MHz = 125 ns
	HAL_GPIO_WritePinHigh(nSYNC_nRESET_PORT, nSYNC_nRESET_PIN);
}



/*
 * Sends SPI byte array on MOSI pin and captures MISO data to a byte array.
 *
 * @param const uint8_t dataTx[]	byte array of SPI data to send on MOSI.
 * @param uint8_t dataRx[]			byte array of SPI data captured on MISO.
 * @param uint8_t byteLength		number of bytes to send & receive.
 *
 * NOTE: Make sure 'dataTx[]' and 'dataRx[]' contain at least as many bytes of
 * 		data, as indicated by 'byteLength'.
 */
void spiSendReceiveArrays(const uint8_t dataTx[], uint8_t dataRx[], const uint8_t byteLength)
{
    // Require that dataTx and dataRx are not NULL pointers
    assert(dataTx && dataRx);

    adsSetCS(LOW);

    for (int i = 0; i < byteLength; i++)
    {
        dataRx[i] = spiSendReceiveByte(dataTx[i]);
    }

    adsSetCS(HIGH);
}



/*
 * Sends SPI byte on MOSI pin and captures MISO return byte value.
 * This function should send and receive single bytes over the SPI.
 *
 * NOTE: This function does not control the /CS pin to allow for more
 * 		programming flexibility.
 *
 * NOTE: This function is called by spiSendReceiveArrays(). If it is called
 * 		directly, then the /CS pin must also be directly controlled.
 *
 * @param const uint8_t dataTx		data byte to send on MOSI pin.
 *
 * @return Captured MISO response byte.
 */
uint8_t spiSendReceiveByte(const uint8_t dataTx)
{
    // Remove any residual or old data from the receive FIFO
	HAL_SPIEx_FlushRxFifo(&hspi1);

    // SSI TX & RX
	uint8_t dataTxVar = dataTx;
    uint8_t dataRx;

    HAL_SPI_TransmitReceive(&hspi1, &dataTxVar, &dataRx, 1, 10);

    return dataRx;
}

//******************************************************************************
