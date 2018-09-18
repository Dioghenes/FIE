/******************************************************
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

#ifndef FI_TRC_H
#define FI_TRC_H

/* FIE files */
#include "FI_config.h"

/* Micro specific includes */
#include "stm32f3xx.h"

#if(cFIE_TRACE_ENABLED)

	#if (cFIE_TRACEPLUS_ENABLED)
		TIM_HandleTypeDef htimx_anl;
		void FIE_timx_anl(void);
	#endif

	/***** FIE TRACING VARIABLES : they are here to be exported *****/
	extern volatile unsigned int FIE_TRC_SWIN;		// Number of timer a task was switched in
	extern volatile unsigned int FIE_TRC_SWOUT;		// Number of timer a task was switched out
	extern volatile unsigned int FIE_TRC_MOVRDY;	// Number of timer a task was moved into ready list
	extern volatile unsigned int FIE_TRC_DELAY;		// Number of timer a task is delayed
	extern volatile unsigned int FIE_TRC_SUSPEND;	// Number of timer a task is suspended
	extern volatile unsigned int FIE_TRC_RESUME;	// Number of timer a task is resumed
	extern volatile unsigned int FIE_TRC_QRECV;		// Number of timer a queue if taken
	extern volatile unsigned int FIE_TRC_QSEND;		// Number of timer a queue is released
	extern volatile unsigned int FIE_TRC_LASTEVENT;	// Number updated with the last of the events above

	/***** START TRC FUNCTION *****/
	void FIE_trc_start(void);

	/***** FIE TRC FreeRTOS TRACING MACRO REDEFINITION *****/
	void FIE_trc_swin(void);						// Trace switched-in events
	void FIE_trc_swout(void);						// Trace switched-out events
	void FIE_trc_movrdy(void * pxTCB);				// Trace moved-to-readylist events (void* is used to avoid recursive inclusion)
	void FIE_trc_delay(void);						// Trace task-delayed events
	void FIE_trc_suspend(void * pxTaskToSuspend);	// Trace task-suspended events (void* is used to avoid recursive inclusion)
	void FIE_trc_resume(void * pxTaskToResume);		// Trace task-resumed events (void* is used to avoid recursive inclusion)
	void FIE_trc_qrecv(void * pxQueue);				// Trace queue-received events (void* is used to avoid recursive inclusion)
	void FIE_trc_qsend(void * pxQueue);				// Trace queue-released events (void* is used to avoid recursive inclusion)

	#define traceTASK_SWITCHED_IN()					FIE_trc_swin()
	#define traceTASK_SWITCHED_OUT()				FIE_trc_swout()
	#define traceMOVED_TASK_TO_READY_STATE( pxTCB )	FIE_trc_movrdy( pxTCB )
	#define traceTASK_DELAY()						FIE_trc_delay()
	#define traceTASK_SUSPEND( pxTaskToSuspend )	FIE_trc_suspend( pxTaskToSuspend )
	#define traceTASK_RESUME( pxTaskToResume )		FIE_trc_resume( pxTaskToResume )
	#define traceQUEUE_RECEIVE( pxQueue )			FIE_trc_qrecv( pxQueue )
	#define traceQUEUE_SEND( pxQueue )				FIE_trc_qsend( pxQueue )

#endif

#endif
