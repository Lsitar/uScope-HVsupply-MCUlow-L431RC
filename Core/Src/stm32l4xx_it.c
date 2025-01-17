/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include "calibration.h"
#include "communication.h"
#include "regulator.h"
#include "typedefs.h"
#include "utilities.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
static bool bLedSetBySPI;
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern DMA_HandleTypeDef hdma_spi1_rx;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
	ledRed(ON);
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
	  ledError(2);
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

	if (commWatchdog > 0)
		commWatchdog--;
	else
	{
		System.bCommunicationOk = false;
		ledGreen(OFF);
	}

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32L4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line0 interrupt.
  */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */

  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
  * @brief This function handles EXTI line2 interrupt.
  */
void EXTI2_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI2_IRQn 0 */

  /* USER CODE END EXTI2_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
  /* USER CODE BEGIN EXTI2_IRQn 1 */

  /* USER CODE END EXTI2_IRQn 1 */
}

/**
  * @brief This function handles EXTI line4 interrupt.
  */
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */

  /* USER CODE END EXTI4_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
  /* USER CODE BEGIN EXTI4_IRQn 1 */

  /* USER CODE END EXTI4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel2 global interrupt.
  */
void DMA1_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_IRQn 0 */

  /* USER CODE END DMA1_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
  /* USER CODE BEGIN DMA1_Channel2_IRQn 1 */

  /* USER CODE END DMA1_Channel2_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel3 global interrupt.
  */
void DMA1_Channel3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */

  /* USER CODE END DMA1_Channel3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
  /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */

  /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/**
  * @brief This function handles ADC1 global interrupt.
  */
void ADC1_IRQHandler(void)
{
  /* USER CODE BEGIN ADC1_IRQn 0 */

  /* USER CODE END ADC1_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  /* USER CODE BEGIN ADC1_IRQn 1 */

  /* USER CODE END ADC1_IRQn 1 */
}

/**
  * @brief This function handles I2C1 event interrupt.
  */
void I2C1_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_EV_IRQn 0 */
	__NOP();
  /* USER CODE END I2C1_EV_IRQn 0 */
  HAL_I2C_EV_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_EV_IRQn 1 */

  /* USER CODE END I2C1_EV_IRQn 1 */
}

/**
  * @brief This function handles I2C1 error interrupt.
  */
void I2C1_ER_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_ER_IRQn 0 */

  /* USER CODE END I2C1_ER_IRQn 0 */
  HAL_I2C_ER_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_ER_IRQn 1 */

  /* USER CODE END I2C1_ER_IRQn 1 */
}

/**
  * @brief This function handles SPI1 global interrupt.
  */
void SPI1_IRQHandler(void)
{
  /* USER CODE BEGIN SPI1_IRQn 0 */

  /* USER CODE END SPI1_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi1);
  /* USER CODE BEGIN SPI1_IRQn 1 */

  /* USER CODE END SPI1_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
	static uint32_t cntIrq = 0;

	cntIrq++;

#if defined (CUSTOM_RX) && !defined(MCU_HIGH)
	uartCustomIrqHandler();
#else

//	uint32_t regIsrBefore = USART1->ISR;

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

//	uint32_t regIsrAfter = USART1->ISR;
//
//	uint32_t regDiff = regIsrBefore ^ regIsrAfter;
//	uint32_t regNoChange = regIsrBefore & regIsrAfter;
//
//	if (regIsrAfter & USART_ISR_PE)
//		__NOP();	// parity error
//	if (regIsrAfter & USART_ISR_FE)
//		USART1->ICR |= USART_ICR_FECF;	//__NOP();	// frame error
//	if (regIsrAfter & USART_ISR_NE)
//		USART1->ICR |= USART_ICR_NCF;	//__NOP();	// noise error (NF bit)
//	if (regIsrAfter & USART_ISR_ORE)
//		__NOP();	// overrun error
//	if (regIsrAfter & USART_ISR_RXNE)
//	{
//		regDiff = USART1->RDR;	//__NOP();	// Rx not empty error
//		HAL_UART_AbortReceive_IT(&huart1);
//	}
//	if (regIsrAfter & USART_ISR_PE)
//		__NOP();	// parity error

#endif
	return;
  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt, DAC channel1 and channel2 underrun error interrupts.
  */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/*
 * Reads battery voltage.
 */
_OPT_O3 void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	// period 2.5 ms
	float fVolt = 6.6f * (3.74f/3.64f) * (float)(HAL_ADC_GetValue(&hadc1))/4095.0f;
	System.battVolt = movAvgAddSample(&movAvgAdcBatt, fVolt);
	System.battProc = (uint32_t)(100.0f * (System.battVolt - 3.0f)/1.2f);
	HAL_ADC_Start_IT(&hadc1);
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == ADC_DRDY_EXTI4_Pin)
	{
		if (System.ads.ready == true)
		{
#ifdef MCU_HIGH
	#ifdef ADS_SPI_USE_INT
			adsReadDataIT();
	#elif defined (ADS_SPI_USE_DMA)
			adsReadDataDMA();
	#else
			//adsReadDataOptimized(&System.ads.data);
			static uint32_t cntSent;
			static uint32_t cntSkipped;
			static uint32_t cntWrong;

			if (true == adsReadDataITcallback(&System.ads.data))
			{
				calcualteSamples();
				ledRed(OFF);
				ledBlue(BLINK);
				if (uartIsIdle())
				{
					sendResults();
					cntSent++;
				}
				else
					cntSkipped++;
			}
			else
			{
				cntWrong++;
				ledRed(ON);
			}
	#endif
#else


	#ifdef ADS_SPI_USE_INT
			adsReadDataIT();
	#elif defined (ADS_SPI_USE_DMA)
			adsReadDataDMA();
	#else
			if (true == adsReadDataOptimized(&System.ads.data))
			{
				ledBlue(BLINK);
//				calibOffset();
				calcualteSamples();
			}
			else
			{
				// wrong crc
			}

	#endif

#endif

		}
	}
	else if (GPIO_Pin == ENC_CHA_EXTI0_Pin)
	{
		encoderKnob_turnCallback();
	}
	else if (GPIO_Pin == ENC_SW_Pin)
	{
		encoderKnob_buttonCallback();
	}
}



void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	UNUSED(hspi);
#ifdef MCU_HIGH
	#if defined (ADS_SPI_USE_INT) || defined (ADS_SPI_USE_DMA)

		static uint32_t cntSent;
		static uint32_t cntSkipped;
		static uint32_t cntWrong;

		if (true == adsReadDataITcallback(&System.ads.data))
		{
			calcualteSamples();

			if (uartIsIdle())
			{
				sendResults();
				cntSent++;
			}
			else
				cntSkipped++;

			ledBlue(BLINK);
			if (bLedSetBySPI)
			{
				ledRed(OFF);
				bLedSetBySPI = false;
			}
		}
		else
		{
			cntWrong++;
			ledRed(ON);
			bLedSetBySPI = true;
		}
	#endif

#else

	#if defined (ADS_SPI_USE_INT) || defined (ADS_SPI_USE_DMA)

//		adsSyncPulse();		// prevent occasionally Overrun error
		if (true == adsReadDataITcallback(&System.ads.data))
		{
//			calibOffset();
			calcualteSamples();
			if (bLedSetBySPI)
			{
				ledRed(OFF);
				bLedSetBySPI = false;
			}

//			if ((System.ref.loggerMode == LOGGER_HF_UC_STEADY)||(System.ref.loggerMode == LOGGER_HF_UC_STARTUP))
//				loggerHighFreqSample(); /* Turn this on for sampling before filter */
		}
		else
		{	// wrong crc
			ITM_SendChar('*');
		}
		ledBlue(BLINK);

	#endif

#endif // MCU_HIGH
}



#if defined (ADS_SPI_USE_INT) || defined (ADS_SPI_USE_DMA)
/*
 * 115 - 216 us with printf (!)
 * 10 us without printf (y)
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
//	SPAM(("ADS clbk err: %u\n", hspi->ErrorCode)); // ADS clbk err: 4

//	System.ads.ready = false;
//	System.ads.error = true;
//	HAL_NVIC_DisableIRQ(EXTI4_IRQn);
	adsSetCS(HIGH);
	ledBlue(OFF);
	ledRed(ON);
	bLedSetBySPI = true;

//	uint32_t errno = HAL_SPI_GetError(hspi);
//	SPAM(("ADS clbk err: "));
//	if (errno & HAL_SPI_ERROR_MODF)
//		SPAM(("MODF, "));
//	if (errno & HAL_SPI_ERROR_CRC)
//		SPAM(("CRC, "));
//	if (errno & HAL_SPI_ERROR_OVR)
//		SPAM(("OVR, "));
//	if (errno & HAL_SPI_ERROR_FRE)
//		SPAM(("FRE, "));
//	if (errno & HAL_SPI_ERROR_DMA)
//		SPAM(("DMA, "));
//	if (errno & HAL_SPI_ERROR_FLAG)
//		SPAM(("FLAG, "));
//	if (errno & HAL_SPI_ERROR_ABORT)
//		SPAM(("ABORT, "));		/* error during Abort procedure */
//	SPAM((".\n"));

#ifdef ADS_SPI_USE_DMA
	if (HAL_OK != HAL_SPI_DMAStop(&hspi1))
#elif defined (ADS_SPI_USE_INT)
	if (HAL_OK != HAL_SPI_Abort_IT(&hspi1))
#endif
	{
		SPAM(("HAL_SPI_Abort error\n"));
	}
	adsSyncPulse(); // according to ADS datasheet it doesnt do much, but somehow helps here

}
#endif



/*
 * 10 ms tick for PID regulators and logger
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	UNUSED(htim);
	// loggers take 2 us (inactive) - 5 us (active)
	if (System.bSweepOn)
		sweepUePeriod();
	else if (System.ref.loggerMode == LOGGER_IA_UE_UF)
		loggerPeriod();

	// regulator takes 23 us
	regulatorPeriodCallback();
}

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
