/******************************************************
 * Dioghenes
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

/* GLOBAL file */
#include "FI_globals.h"

/* FIE files*/
#include "FI_config.h"
#include "FI_trc.h"
#include "FI_environment.h"

/* Micro specific includes */
#include "stm32f3xx.h"

/* OS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* GC includes */
#include "string.h"

/* SEGGER RTT include */
#include "SEGGER_RTT.h"

#if(cFIE_TRACE_ENABLED)

	/*********************************************************
	 * TRACE VARIABLES MONITORING OS
	 *********************************************************/
	volatile unsigned int FIE_TRC_SWIN;		// Number of timer a task was switched in
	volatile unsigned int FIE_TRC_SWOUT;	// Number of timer a task was switched out
	volatile unsigned int FIE_TRC_MOVRDY;	// Number of timer a task was moved into ready list
	volatile unsigned int FIE_TRC_DELAY;	// Number of timer a task is delayed
	volatile unsigned int FIE_TRC_SUSPEND;	// Number of timer a task is suspended
	volatile unsigned int FIE_TRC_RESUME;	// Number of timer a task is resumed
	volatile unsigned int FIE_TRC_QRECV;	// Number of timer a queue if taken
	volatile unsigned int FIE_TRC_QSEND;	// Number of timer a queue is released
	volatile unsigned int FIE_TRC_LASTEVENT;// Number updated with the last of the events above

	/*********************************************************
	 * ANL GLOBAL VARIABLES
	 *********************************************************/
	#if (cFIE_TRACEPLUS_ENABLED)
		struct FIEtrace_struct{
			uint32_t count;
			char event;
			char actor;
			char end;
		};
		volatile struct FIEtrace_struct FIEtrc_eventslist[cFIE_ANL_BUFLEN];
		volatile int FIEtrc_eventlist_i;
		volatile int FIEtrc_countercycle;
		void FIE_timx_anl(void){
			__HAL_RCC_TIM2_CLK_ENABLE();
			htimx_anl.Instance = TIM4;									// Select timer to use
			htimx_anl.Init.Prescaler = cFIE_RES_D;						// Choose prescaler
			htimx_anl.Init.Period = cFIE_RES_T;							// Select max period
			htimx_anl.Init.CounterMode = TIM_COUNTERMODE_UP;			// Counting mode
			htimx_anl.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;		// Divisor for the timer
			htimx_anl.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
			htimx_anl.Init.RepetitionCounter = 0;
			HAL_TIM_OC_Init(&htimx_anl);
			__HAL_TIM_CLEAR_FLAG(&htimx_anl, TIM_FLAG_UPDATE);
		}
		void TIM4_IRQHandler(void){
			HAL_TIM_IRQHandler(&htimx_anl);
		}
		void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim){
			for(int i = 0; i < cFIE_ANL_BUFLEN; i++){
				if(FIEtrc_eventslist[i].event==0) break;
				SEGGER_RTT_printf(0,"%u\t%d\t%d\r\n",FIEtrc_eventslist[i].count+cFIE_RES_T*FIEtrc_countercycle,FIEtrc_eventslist[i].event,FIEtrc_eventslist[i].actor);
			}
			FIEtrc_countercycle++;
		}
	#endif

	/*********************************************************
	 * @Name	FIE_trc_start
	 * @Brief	Initialize FIE TRC environment
	 * @Long	Initialize FIE TRC resetting to 0 all the
	 * 			global variables.
	 *********************************************************/
	void FIE_trc_start(void){
		FIE_TRC_SWIN = 0;
		FIE_TRC_SWOUT = 0;
		FIE_TRC_MOVRDY = 0;
		FIE_TRC_DELAY = 0;
		FIE_TRC_SUSPEND = 0;
		FIE_TRC_RESUME = 0;
		FIE_TRC_QRECV = 0;
		FIE_TRC_QSEND = 0;
		FIE_TRC_LASTEVENT = 0;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_countercycle = 0;
			FIEtrc_eventlist_i = 0;
		#endif
	}

	/*********************************************************
	 * @Name	FIE_trc_*
	 * @Brief	Functions logging events
	 * @Long	Each of this function is called by the related
	 * 			macro, during the execution of some kernel
	 * 			code.
	 *********************************************************/
	void FIE_trc_swin(void){
		FIE_TRC_SWIN++;
		FIE_TRC_LASTEVENT=1;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 1;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = -1;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}
	void FIE_trc_swout(void){
		FIE_TRC_SWOUT++;
		FIE_TRC_LASTEVENT=2;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 2;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = -1;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}
	void FIE_trc_movrdy(void * pxTCB){
		FIE_TRC_MOVRDY++;
		FIE_TRC_LASTEVENT=3;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 3;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = ((FIE_TCB_t*)pxTCB)->uxTCBNumber;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}
	void FIE_trc_delay(void){
		FIE_TRC_DELAY++;
		FIE_TRC_LASTEVENT=4;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 4;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = -1;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}
	void FIE_trc_suspend(void * pxTaskToSuspend){
		FIE_TRC_SUSPEND++;
		FIE_TRC_LASTEVENT=5;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 5;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = ((FIE_TCB_t*)pxTaskToSuspend)->uxTCBNumber;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}
	void FIE_trc_resume(void *  pxTaskToResume){
		FIE_TRC_RESUME++;
		FIE_TRC_LASTEVENT=6;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 6;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = ((FIE_TCB_t*)pxTaskToResume)->uxTCBNumber;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}
	void FIE_trc_qrecv(void * pxQueue){
		FIE_TRC_QRECV++;
		FIE_TRC_LASTEVENT=7;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 7;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = ((FIE_Queue_t*)pxQueue)->uxQueueNumber;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}
	void FIE_trc_qsend(void * pxQueue){
		FIE_TRC_QSEND++;
		FIE_TRC_LASTEVENT=8;
		#if(cFIE_TRACEPLUS_ENABLED)
			FIEtrc_eventslist[FIEtrc_eventlist_i].count = __HAL_TIM_GET_COUNTER(&htimx_anl);
			FIEtrc_eventslist[FIEtrc_eventlist_i].event = 8;
			FIEtrc_eventslist[FIEtrc_eventlist_i].actor = ((FIE_Queue_t*)pxQueue)->uxQueueNumber;
			FIEtrc_eventlist_i++;
			if (FIEtrc_eventlist_i>=cFIE_ANL_BUFLEN) FIEtrc_eventlist_i=0;
		#endif
	}

#endif

