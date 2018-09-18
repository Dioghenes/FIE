/**
  ******************************************************************************
  * @file    stm32f3xx_it.c
  * @author  Ac6
  * @version V1.0
  * @date    02-Feb-2015
  * @brief   Default Interrupt Service Routines.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "stm32f3xx.h"
#include "stm32f3_discovery.h"

#include "GLOBALS.h"
#include "FI_environment.h"

#ifdef USE_RTOS_SYSTICK
#include <cmsis_os.h>
#endif
#include "stm32f3xx_it.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            	  	    Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles SysTick Handler, but only if no RTOS defines it.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
#ifdef USE_RTOS_SYSTICK
	osSystickHandler();
#endif
}

#if (cFIE_ENABLED==cFIE_urtmode)
	void FIE_SysErrorHandler(void)
	{
		FIE_REP_SYS_ETAF = __HAL_TIM_GET_COUNTER(&htimx_res);
		GPIO_InitTypeDef gpio;
		gpio.Pin = LED4_PIN;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOE,&gpio);
		HAL_GPIO_WritePin(GPIOE, LED4_PIN, GPIO_PIN_SET);
		FIE_stop();
		GB_fakedelay(200);
		HAL_NVIC_SystemReset();

		/* Should never get here */
		while(1);
	}
#else
	void FIE_SysErrorHandler(void)
	{
		GPIO_InitTypeDef gpio;
		gpio.Pin = LED4_PIN;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOE,&gpio);
		HAL_GPIO_WritePin(GPIOE, LED4_PIN, GPIO_PIN_SET);
		while(1);
	}
#endif
