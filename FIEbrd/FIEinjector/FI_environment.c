/******************************************************
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

/* Globals */
#include "FI_globals.h"

/* FIE files */
#include "FI_config.h"
#include "FI_environment.h"

/* Micro specific includes */
#include "stm32f3xx.h"
#include "stm32f3_discovery.h"

/* OS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* GCC includes */
#include "string.h"
#include "stdlib.h"

#if (cFIE_ENABLED==cFIE_urtmode)

	/*********************************************************
	 * GLOBALS
	 *********************************************************/
	uint32_t FIE_crc32(unsigned char *data, int size);
	char FIEmon_buffer[128];

	#if (cFIE_PRECISEINJ_ENABLED==1)
		volatile unsigned int FIE_PRECISE_INJECTNOW = 0;
	#endif

	/*********************************************************
	 * SYSTEM STATUS
	 *********************************************************/
	static int FIE_SYSCRASH = 1;

	/*********************************************************
	 * SETUP VARIABLES RECEIVED THROUGH UART
	 *********************************************************/
	struct FIEsetup_struct{
		uint16_t FIE_INJ_BITN;		// Define in which bit to perform the flip (0-31)
		uint16_t FIE_INJ_LOCUS;		// Code to identify the injection locus
		uint16_t FIE_INJ_TIME_T;	// Timer Period
		uint16_t FIE_INJ_TIME_D;	// Timer Prescaler
		uint32_t FIE_SETUP_CRC;		// CRC of values got
	};
	static struct FIEsetup_struct FIE_SETUP_STRUCT __attribute__((section(".no_load_fie_sect"))) = {0};

	/*********************************************************
	 * REPORT VARIABLES SENT THROUGH UART
	 *********************************************************/
	volatile unsigned int FIE_REP_BM_CRC_OK[cFIE_BM_NUMBER];	// Number of times when the CRC was correct
	volatile unsigned int FIE_REP_BM_CRC_WRONG[cFIE_BM_NUMBER];	// Number of times when the CRC was wrong
	volatile unsigned int FIE_REP_BM_MICROIT[cFIE_BM_NUMBER];	// Number of micro-iterations done *to implement
	volatile unsigned int FIE_REP_BM_MACROIT[cFIE_BM_NUMBER];	// Number of macro-iterations done *to implement
	volatile uint16_t FIE_REP_SYS_ETAF;							// Elapsed time after fault injection
	volatile unsigned char FIE_REP_SYS_OK;	 					// The system survives to the injection?
	volatile unsigned char FIE_REP_SYS_INJ;						// The injection was done?
	volatile unsigned char FIE_REP_SYS_INJOK;					// Is the injector ok? (CRC based check)

	/*********************************************************
	 * 	@Name 	FIE_start
	 *  @Brief	Setup the FIE
	 *  @Long	Reset the REP variables, read setup variables
	 *  		from UART and initialize the FIEmon logger.
	 *********************************************************/
	void FIE_start(void){
		// *** Reset 8 volatile variables
		FIE_SYSCRASH = 1;
		for(int i=0;i<cFIE_BM_NUMBER;i++){
			FIE_REP_BM_MICROIT[i] = 0;
			FIE_REP_BM_MACROIT[i] = 0;
			FIE_REP_BM_CRC_OK[i] = 0;
			FIE_REP_BM_CRC_WRONG[i] = 0;
		}
		FIE_REP_SYS_ETAF = 0;
		FIE_REP_SYS_OK = 0;
		FIE_REP_SYS_INJ = 0;
		FIE_REP_SYS_INJOK = 0;

		// *** Send start character
		HAL_UART_Transmit(&huartx, (uint8_t*)cFIEmon_STARTITCHAR, 1, 0xFFFF);

		// *** INIT Synchro barrier
		while(HAL_UART_Receive(&huartx, (uint8_t*)FIEmon_buffer, 4, 0x00F0)!=HAL_OK){
			FIE_SETUP_STRUCT.FIE_INJ_BITN = 0;
			FIE_SETUP_STRUCT.FIE_INJ_LOCUS = 0;
			FIE_SETUP_STRUCT.FIE_INJ_TIME_T = 0;
			FIE_SETUP_STRUCT.FIE_INJ_TIME_D = 0;
		}

		// *** Receive injection parameters from PC
		HAL_UART_Receive(&huartx, (uint8_t*)FIEmon_buffer, 48, 0x00F0);
		sscanf(FIEmon_buffer,"%hu\t%hu\t%hu\t%hu",&(FIE_SETUP_STRUCT.FIE_INJ_BITN),&(FIE_SETUP_STRUCT.FIE_INJ_LOCUS),&(FIE_SETUP_STRUCT.FIE_INJ_TIME_T),&(FIE_SETUP_STRUCT.FIE_INJ_TIME_D));

		// *** Calc CRC of received values
		FIE_SETUP_STRUCT.FIE_SETUP_CRC = FIE_crc32((unsigned char*)&FIE_SETUP_STRUCT,8);

		// *** Send back injection parameters to host for debug
		sprintf(FIEmon_buffer,"%hu\t%hu\t%hu\t%hu\t*\t",FIE_SETUP_STRUCT.FIE_INJ_BITN,FIE_SETUP_STRUCT.FIE_INJ_LOCUS,FIE_SETUP_STRUCT.FIE_INJ_TIME_T,FIE_SETUP_STRUCT.FIE_INJ_TIME_D);
		HAL_UART_Transmit(&huartx, (uint8_t*)FIEmon_buffer, strlen(FIEmon_buffer), 0xFFFF);
	}

	/******************************************************
	 * 	@Name 	FIE_stop
	 *  @Brief	Stop the FIE and update variables
	 *  @Long	Try to understand which is the status of
	 *  		the system, send the results of the run
	 *  		through UART. This function is called by
	 *  		the error handler.
	 ******************************************************/
	void FIE_stop(void){
		// *** Status of the system
		if(FIE_SYSCRASH) FIE_REP_SYS_OK = 0;
		else FIE_REP_SYS_OK = 1;

		// *** Send the 4 BM variables about the various benchmarks and then the 3 SYS variables
		for(int i=0;i<cFIE_BM_NUMBER;i++){
			sprintf(FIEmon_buffer,"%hu\t%hu\t%hu\t%hu\t-\t",FIE_REP_BM_CRC_OK[i],FIE_REP_BM_CRC_WRONG[i],FIE_REP_BM_MICROIT[i],FIE_REP_BM_MACROIT[i]);
			HAL_UART_Transmit(&huartx, (uint8_t*)FIEmon_buffer, strlen(FIEmon_buffer), 0xFFFF);
		}

		#if(cFIE_TRACE_ENABLED)
			// *** Send tracing data
			sprintf(FIEmon_buffer,"%hu\t%hu\t%hu\t%hu\t%hu\t%hu\t%hu\t%hu\t%hu\t-\t",FIE_TRC_SWIN,FIE_TRC_SWOUT,FIE_TRC_MOVRDY,FIE_TRC_DELAY,FIE_TRC_SUSPEND,FIE_TRC_RESUME,FIE_TRC_QRECV,FIE_TRC_QSEND,FIE_TRC_LASTEVENT);
			HAL_UART_Transmit(&huartx, (uint8_t*)FIEmon_buffer, strlen(FIEmon_buffer), 0xFFFF);
		#endif

		// *** Check the injection environment
		FIE_REP_SYS_INJOK = (FIE_SETUP_STRUCT.FIE_SETUP_CRC==FIE_crc32((unsigned char*)&FIE_SETUP_STRUCT,8)) ? 1 : 0;

		// *** Send system data
		sprintf(FIEmon_buffer,"%hu\t%hu\t%hu\t%hu\r\n",FIE_REP_SYS_INJOK,FIE_REP_SYS_INJ,FIE_REP_SYS_OK,FIE_REP_SYS_ETAF);
		HAL_UART_Transmit(&huartx, (uint8_t*)FIEmon_buffer, strlen(FIEmon_buffer), 0xFFFF);

		// *** Send the stop iteration character
		HAL_UART_Transmit(&huartx, (uint8_t*)cFIEmon_STOPITCHAR, 1, 0xFFFF);
	}

	/******************************************************
	 * 	@Name 	FIE_crc32
	 *  @Brief	Calculate the CRC
	 *  @Long	Calculate the CRC of the variables to see
	 *  		if they are changed during the execution
	 ******************************************************/
	uint32_t FIE_crc32(unsigned char *data, int size){
		uint32_t r = 0xFFFFFFFF;

		for(int j=0; j<size; j++){
			r = r ^ *data++;
			for(int i = 0; i < 8; i++){
			  uint32_t t = ~((r&1) - 1);
			  r = (r>>1) ^ (0xEDB88320 & t);
			}
		}

		return ~r;
	}

	/******************************************************
	 * 	@Name 	FIE_timx_inj + ISR Handler
	 *  @Brief	Setup the injector timer
	 *  @Long	Setup the prescaler and the period using
	 *  		values given by the UART.
	 *  		Enable then the interrupt generated
	 *  		by the base counting mode.
	 ******************************************************/
	void FIE_timx_inj(void){
		__HAL_RCC_TIM2_CLK_ENABLE();
		htimx_inj.Instance = TIM2;									// Select timer to use
		htimx_inj.Init.Prescaler = FIE_SETUP_STRUCT.FIE_INJ_TIME_D;	// Set prescaler
		htimx_inj.Init.Period = FIE_SETUP_STRUCT.FIE_INJ_TIME_T;	// Set period
		htimx_inj.Init.CounterMode = TIM_COUNTERMODE_UP;			// Counting mode
		htimx_inj.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;		// Divisor for the timer
		htimx_inj.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
		htimx_inj.Init.RepetitionCounter = 0;
		HAL_TIM_Base_Init(&htimx_inj);
		__HAL_TIM_CLEAR_FLAG(&htimx_inj, TIM_FLAG_UPDATE);
	}
	void TIM2_IRQHandler(void){
		HAL_TIM_IRQHandler(&htimx_inj);
	}

	/******************************************************
	 * 	@Name 	FIE_timx_res + ISR Handler
	 *  @Brief	Setup the resume timer
	 *  @Long	Setup the prescaler and the period according
	 *			to the compile time values. Enable then the
	 *			interrupt generated by the base counting
	 *			mode.
	 ******************************************************/
	void FIE_timx_res(void){
		__HAL_RCC_TIM3_CLK_ENABLE();
		htimx_res.Instance = TIM3;								// Select timer to use
		htimx_res.Init.Prescaler = cFIE_INJ_TIMRPRES;			// Choose the prescaler value
		htimx_res.Init.CounterMode = TIM_COUNTERMODE_UP;		// Counting mode
		htimx_res.Init.Period = cFIE_INJ_TIMRPER;				// Timer period
		htimx_res.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;	// Divisor for the timer
		htimx_res.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
		htimx_res.Init.RepetitionCounter = 0;
		HAL_TIM_Base_Init(&htimx_res);
		__HAL_TIM_CLEAR_FLAG(&htimx_res, TIM_FLAG_UPDATE);
		HAL_TIM_Base_Start_IT(&htimx_res);
	}
	void TIM3_IRQHandler(void){
		HAL_TIM_IRQHandler(&htimx_res);
	}

	/******************************************************
	 * 	@Name 	HAL_TIM_PeriodElapsedCallback
	 *  @Brief	Decide what to do when a TIM raises an int
	 *  @Long	Spot which timer raised the interrupt:
	 *  		if it was the inj timer then stop its int
	 *  		generation, do the injection if required
	 *  		by the mode and setup the resume timer.
	 *  		If this ISR was called by the resume timer,
	 *  		just set the system crash to 0 and jump
	 *  		to the error handler.
	 ******************************************************/
	void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
		if(htim->Instance==TIM2){
			HAL_TIM_Base_Stop_IT(&htimx_inj);
			#if (cFIE_PRECISEINJ_ENABLED==1)
				FIE_PRECISE_INJECTNOW = 1;
			#else
				FIE_injector();
			#endif
			GPIO_InitTypeDef gpio;
			gpio.Pin = LED3_PIN;
			gpio.Mode = GPIO_MODE_OUTPUT_PP;
			gpio.Pull = GPIO_PULLDOWN;
			HAL_GPIO_Init(GPIOE,&gpio);
			HAL_GPIO_TogglePin(GPIOE, LED3_PIN);
			FIE_timx_res();
		}
		else if(htim->Instance==TIM3){
			FIE_SYSCRASH = 0;
			__asm volatile( "b FIE_SysErrorHandler" );
		}
	}

	/******************************************************
	 * 	@Name 	FIE_injector
	 *  @Brief	Do the injection
	 *  @Long	Inject on a locus according to the value of
	 *  		FIE_INJ_LOCUS variable and in the bit pointed
	 *  		by FIE_INJ_BITN
	 *  		A dummy injector is used when no injection
	 *  		must be done to simulate the behavior of the
	 *  		injection and so to get a similar result
	 ******************************************************/
	void FIE_injector(void){
		#if (cFIE_LIST == 1)
			FIE_REP_SYS_INJ = 1;
			if	   (FIE_SETUP_STRUCT.FIE_INJ_LOCUS==0){ FAULT(0) = (UBaseType_t) 	( (size_t) FAULT(0) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==1){ FAULT(1) = (TickType_t)		( (size_t) FAULT(1) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==2){ FAULT(2) = (UBaseType_t)	( (size_t) FAULT(2) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==3){ FAULT(3) = (BaseType_t)		( (size_t) FAULT(3) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==4){ FAULT(4) = (UBaseType_t)	( (size_t) FAULT(4) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==5){ FAULT(5) = (BaseType_t)		( (size_t) FAULT(5) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==6){ FAULT(6) = (BaseType_t)		( (size_t) FAULT(6) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==7){ FAULT(7) = (UBaseType_t)	( (size_t) FAULT(7) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==8){ FAULT(8) = (TickType_t)		( (size_t) FAULT(8) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==9){ FAULT(9) = (UBaseType_t)	( (size_t) FAULT(9) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
		#endif

		#if (cFIE_LIST == 2)
			FIE_TCB_t *FI_TCBp;
			#if (cFIE_LIST2_WHAT == cFIE_LIST2_IDLTCB)
				FI_TCBp = pxFIEGetIdleTCBHandle();					// Memory address of the TCB of the idle task
			#elif (cFIE_LIST2_WHAT == cFIE_LIST2_CURTCB)
				FI_TCBp = pxFIEGetCurrentTCBHandle();				// Memory address of the TCB of the current task
			#elif (cFIE_LIST2_WHAT == cFIE_LIST2_RDYTCB)
				FIE_List_t *FI_Listp = pxFIEGetReadyTasksHandle();	// Memory address of the ready List struct with index 0
				if(FI_Listp[cFIE_LIST2_PRIO].uxNumberOfItems == 0) FI_TCBp = NULL;
				else FI_TCBp = FI_Listp[cFIE_LIST2_PRIO].pxIndex->pvOwner;
			#elif (cFIE_LIST2_WHAT == cFIE_LIST2_DLDTCB)
				FIE_List_t *FI_Listp = pxFIEGetDelayedTasksHandle();// Memory address of the delayed List struct
				if(FI_Listp->uxNumberOfItems == 0){
					FI_Listp = pxFIEGetOvfDelayedTasksHandle();		// Memory address of the ovf delayed List struct
					if(FI_Listp->uxNumberOfItems == 0) FI_TCBp = NULL;
				}
				else FI_TCBp = FI_Listp->pxIndex->pxPrevious->pvOwner;
			#elif (cFIE_LIST2_WHAT == cFIE_LIST2_SUSTCB)
				FIE_List_t *FI_Listp = pxFIEGetSuspendedTasksHandle();	// Memory address of the suspended List struct
				if(FI_Listp->uxNumberOfItems == 0) FI_TCBp = NULL;
				else FI_TCBp = FI_Listp->pxIndex->pxPrevious->pvOwner;
			#endif

			if(FI_TCBp == NULL) return;
			else FIE_REP_SYS_INJ = 1;

			if	   (FIE_SETUP_STRUCT.FIE_INJ_LOCUS==0){ FAULT(0) = (StackType_t*) 	( (size_t) FAULT(0) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==1){ FAULT(1) = (UBaseType_t)  	( (size_t) FAULT(1) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==2){ FAULT(2) = (StackType_t*) 	( (size_t) FAULT(2) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==3){ FAULT(3) = (UBaseType_t) 	( (size_t) FAULT(3) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==4){ FAULT(4) = (UBaseType_t) 	( (size_t) FAULT(4) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==5){ FAULT(5) = (UBaseType_t) 	( (size_t) FAULT(5) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==6){ FAULT(6) = (UBaseType_t) 	( (size_t) FAULT(6) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==7){ FAULT(7) = (uint32_t) 		( (size_t) FAULT(7) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==8){ FAULT(8) = (uint8_t) 		( (size_t) FAULT(8) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}

			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==9){ FAULT(9)   = (TickType_t) 	( (size_t) FAULT(9)  cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==10){ FAULT(10) = (ListItem_t*) 	( (size_t) FAULT(10) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==11){ FAULT(11) = (ListItem_t*)  ( (size_t) FAULT(11) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==12){ FAULT(12) = (void*)  		( (size_t) FAULT(12) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==13){ FAULT(13) = (void*) 		( (size_t) FAULT(13) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}

			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==14){ FAULT(14) = (TickType_t) 	( (size_t) FAULT(14) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==15){ FAULT(15) = (ListItem_t*) 	( (size_t) FAULT(15) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==16){ FAULT(16) = (ListItem_t*)  ( (size_t) FAULT(16) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==17){ FAULT(17) = (void*)  		( (size_t) FAULT(17) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==18){ FAULT(18) = (void*) 		( (size_t) FAULT(18) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN) );}
		#endif

		#if (cFIE_LIST == 3)
			FIE_List_t *FI_Listp;
			#if (cFIE_LIST3_WHAT == cFIE_LIST3_RDYLST)
				FI_Listp = pxFIEGetReadyTasksHandle();		// Memory address of the first List struct
				FI_Listp = &(FI_Listp[cFIE_LIST3_PRIO]);	// Memory address of the desired List struct
			#elif (cFIE_LIST3_WHAT == cFIE_LIST3_DLDLST)
				FI_Listp = pxFIEGetDelayedTasksHandle();	// Memory address of the delayed List struct
			#elif (cFIE_LIST3_WHAT == cFIE_LIST3_DLDLST)
				FI_Listp = pxFIEGetOvfDelayedTasksHandle();	// Memory address of the ovf delayed List struct
			#elif (cFIE_LIST3_WHAT == cFIE_LIST3_SUSLST)
				FI_Listp = pxFIEGetSuspendedTasksHandle();	// Memory address of the suspended List struct
			#endif

			if(FI_Listp == NULL) return;
			else FIE_REP_SYS_INJ = 1;

			if	   (FIE_SETUP_STRUCT.FIE_INJ_LOCUS==0){ FAULT(0) = (UBaseType_t)  	( (size_t) FAULT(0) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==1){ FAULT(1) = (ListItem_t *)	( (size_t) FAULT(1) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==2){ FAULT(2) = (TickType_t)  	( (size_t) FAULT(2) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==3){ FAULT(3) = (ListItem_t *)	( (size_t) FAULT(3) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==4){ FAULT(4) = (ListItem_t *)	( (size_t) FAULT(4) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
		#endif

		#if (cFIE_LIST == 4)

			if(FI_Queuep == NULL) return;
			else FIE_REP_SYS_INJ = 1;

			if	   (FIE_SETUP_STRUCT.FIE_INJ_LOCUS==0){ FAULT(0) = (int8_t*) 	  ( (size_t) FAULT(0) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==1){ FAULT(1) = (int8_t*)  	  ( (size_t) FAULT(1) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==2){ FAULT(2) = (int8_t*) 	  ( (size_t) FAULT(2) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==3){ FAULT(3) = (int8_t*) 	  ( (size_t) FAULT(3) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==4){ FAULT(4) = (UBaseType_t)  ( (size_t) FAULT(4) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}

			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==5){ FAULT(5) = (UBaseType_t)  ( (size_t) FAULT(5) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==6){ FAULT(6) = (ListItem_t *) ( (size_t) FAULT(6) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==7){ FAULT(7) = (TickType_t)   ( (size_t) FAULT(7) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==8){ FAULT(8) = (ListItem_t *) ( (size_t) FAULT(8) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==9){ FAULT(9) = (ListItem_t *) ( (size_t) FAULT(9) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}

			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==10){ FAULT(10) = (UBaseType_t)  ( (size_t) FAULT(10) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==11){ FAULT(11) = (ListItem_t *) ( (size_t) FAULT(11) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==12){ FAULT(12) = (TickType_t)   ( (size_t) FAULT(12) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==13){ FAULT(13) = (ListItem_t *) ( (size_t) FAULT(13) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==14){ FAULT(14) = (ListItem_t *) ( (size_t) FAULT(14) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}

			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==15){ FAULT(15) = (UBaseType_t) ( (size_t) FAULT(15) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==16){ FAULT(16) = (UBaseType_t) ( (size_t) FAULT(16) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==17){ FAULT(17) = (UBaseType_t) ( (size_t) FAULT(17) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==18){ FAULT(18) = (int8_t)      ( (size_t) FAULT(18) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==19){ FAULT(19) = (int8_t) 	   ( (size_t) FAULT(19) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==20){ FAULT(20) = (UBaseType_t) ( (size_t) FAULT(20) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}
			else if(FIE_SETUP_STRUCT.FIE_INJ_LOCUS==21){ FAULT(21) = (int8_t) 	   ( (size_t) FAULT(21) cFIE_OP ( cFIE_INJ_MASK << FIE_SETUP_STRUCT.FIE_INJ_BITN ) );}

		#endif
		return;
	}

#endif
