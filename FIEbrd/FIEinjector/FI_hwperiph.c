/******************************************************
 * Dioghenes
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

/* Application specific includes */
#include "FI_globals.h"
#include "FI_hwperiph.h"

/* Micro specific includes */
#include "stm32f3xx.h"
#include "stm32f3_discovery.h"

void FI_sysclock(void){
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;	// Select external oscillator
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;		// Select external prescaler
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;					// Enable external oscillator
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;					// Disable internal oscillator
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;				// Enable PLL
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;		// Select PLL clock source
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;				// Select PLL multiplier
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);	// Select the clock to configure
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;	// Select system clock source
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;			// AMBA HighPerformance clock divider
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;			// AMBA Peripheral 1 clock divider
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;			// AMBA Peripheral 2 clock divider
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

void FI_usartx(void){
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	huartx.Instance = USART1;									// Select USART1
	huartx.Init.BaudRate = 9600; 								// Baudrate setting
	huartx.Init.WordLength = UART_WORDLENGTH_8B;				// Words of 8 bits
	huartx.Init.StopBits = UART_STOPBITS_1;						// 1 Stopbit
	huartx.Init.Parity = UART_PARITY_NONE;						// No parity
	huartx.Init.Mode = UART_MODE_TX_RX;							// Only TX
	huartx.Init.HwFlowCtl = UART_HWCONTROL_NONE;				// No HW control
	huartx.Init.OverSampling = UART_OVERSAMPLING_8;				// Oversampling 8 bits
	huartx.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;	// Disable single bit sampling
	HAL_UART_Init(&huartx);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5; 				// PC5 = TX  ;  PC4 = RX on the board
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;						// Select alternate function PushPull
	GPIO_InitStruct.Pull = GPIO_NOPULL;							// No pulldown nor pullup activated
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;				// Use at high frequencies
	GPIO_InitStruct.Alternate = GPIO_AF7_USART1;				// USART1 functionality of those pins is used
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void FI_leds(void){
	__HAL_RCC_GPIOE_CLK_ENABLE();
	GPIO_InitTypeDef gpio_ledinit;
	gpio_ledinit.Pin = LED3_PIN|LED4_PIN|LED5_PIN|LED6_PIN|LED7_PIN|LED8_PIN|LED9_PIN|LED10_PIN;
	gpio_ledinit.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_ledinit.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOE,&gpio_ledinit);
	HAL_GPIO_WritePin(GPIOE, LED3_PIN|LED4_PIN|LED5_PIN|LED6_PIN|LED7_PIN|LED8_PIN|LED9_PIN|LED10_PIN, GPIO_PIN_RESET);
}
