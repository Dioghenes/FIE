/******************************************************
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

#ifndef FI_GLOBALS_H
#define FI_GLOBALS_H

/* Micro specific includes */
#include "stm32f3xx.h"

/* OS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* Global handlers */
UART_HandleTypeDef huartx;
SemaphoreHandle_t hsem_leds;

#endif
