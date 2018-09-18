/******************************************************
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

#ifndef FI_HWPERIPH_H
#define FI_HWPERIPH_H

void FI_sysclock(void);		// Setup system clock
void FI_usartx(void);		// Setup USART peripheral for logging
void FI_leds(void);			// Setup status to be used arbitrarily

#endif
