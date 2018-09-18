/******************************************************
 * Dioghenes
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

#ifndef FI_ENVIRONMENT_H
#define FI_ENVIRONMENT_H

/* OS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* FIE files */
#include "FI_config.h"

/***** EXPORTED FREERTOS VARIABLES *****/
extern volatile UBaseType_t uxCurrentNumberOfTasks;
extern volatile TickType_t xTickCount;
extern volatile UBaseType_t uxTopReadyPriority;
extern volatile BaseType_t xSchedulerRunning;
extern volatile UBaseType_t uxPendedTicks ;
extern volatile BaseType_t xYieldPending;
extern volatile BaseType_t xNumOfOverflows;
extern UBaseType_t uxTaskNumber;
extern volatile TickType_t xNextTaskUnblockTime;
extern volatile UBaseType_t	uxSchedulerSuspended;
extern TaskHandle_t xIdleTaskHandle;
extern tskTCB * volatile pxCurrentTCB;

#if (cFIE_ENABLED==cFIE_urtmode)

	/***** PRECISE INJECTION *****/
	#if (cFIE_PRECISEINJ_ENABLED==1)
		extern volatile unsigned int FIE_PRECISE_INJECTNOW;
	#endif

	/***** REPORT VARIABLES : they are exportable in your code *****/
	extern volatile unsigned int FIE_REP_BM_CRC_OK[cFIE_BM_NUMBER];		// Number of times when the CRC was correct
	extern volatile unsigned int FIE_REP_BM_CRC_WRONG[cFIE_BM_NUMBER];	// Number of times when the CRC was wrong
	extern volatile unsigned int FIE_REP_BM_MICROIT[cFIE_BM_NUMBER];	// Number of micro-iterations done *to implement
	extern volatile unsigned int FIE_REP_BM_MACROIT[cFIE_BM_NUMBER];	// Number of macro-iterations done *to implement
	extern volatile uint16_t FIE_REP_SYS_ETAF;							// Elapsed time after fault injection

	/***** TIMERS HANDLERS *****/
	TIM_HandleTypeDef htimx_inj;
	TIM_HandleTypeDef htimx_res;

	/***** INJECTOR FUNCTION *****/
	void FIE_start(void);		// Read variables,setup FIE and start FIE logging
	void FIE_stop(void);		// Stop FIE logging and update variables: returns 1 if it is the last round, 0 otherwise
	void FIE_timx_inj(void);	// Setup TIM2 used for the injection
	void FIE_timx_res(void);	// Setup TIM3 used for the resume operation
	void FIE_injector(void);	// This function performs the injection when the Base count interrupt  of TIM 2 is generated

	/***** VARIOUS FAULT LISTS *****/
	#if (cFIE_LIST == 1)										// *** FIE_LIST 1 - Global FreeRTOS data
		#define cFIE_LIST_LEN 		(uint16_t)10
		#define FAULT0 uxCurrentNumberOfTasks					// u32	Total number of tasks in the system
		#define FAULT1 xTickCount								// u32	Tick count since scheduler started
		#define FAULT2 uxTopReadyPriority						// u32	Highest ready priority
		#define FAULT3 xSchedulerRunning						// s32	Is scheduler running?
		#define FAULT4 uxPendedTicks							// u32 	Ticks occurred when scheduler was suspended
		#define FAULT5 xYieldPending							// s32  Say if there is a pending switch or not
		#define FAULT6 xNumOfOverflows							// s32  Number of overflows of the systick counter
		#define FAULT7 uxTaskNumber								// u32  Used to enumerate tasks
		#define FAULT8 xNextTaskUnblockTime						// u32	Time when the next task has to be unblocked
		#define FAULT9 uxSchedulerSuspended						// u32	Is the scheduler suspended?
	#endif

	#if (cFIE_LIST == 2)										// *** FIE_LIST 2 - Injection on TCB of a task
		#define cFIE_LIST_LEN		(uint16_t)19
		#define FAULT0 FI_TCBp->pxTopOfStack					// u32	Pointer to the top of stack
		#define FAULT1 FI_TCBp->uxPriority						// u32	Priority of the task
		#define FAULT2 FI_TCBp->pxStack							// u32	Pointer to base of stack
		#define FAULT3 FI_TCBp->uxTCBNumber						// u32	DEBUG TCB number
		#define FAULT4 FI_TCBp->uxTaskNumber					// u32	DEBUG Number of the task; used for trace purposes
		#define FAULT5 FI_TCBp->uxBasePriority					// u32	Used for priority inheritance
		#define FAULT6 FI_TCBp->uxMutexesHeld					// u32 	If the task holds a mutex
		#define FAULT7 FI_TCBp->ulNotifiedValue					// u32	Task notification system
		#define FAULT8 FI_TCBp->ucNotifyState					// u8	Task notification system
		#define FAULT9 (FI_TCBp->xStateListItem).xItemValue		// u32	Cardinal number of the listed element (State of the task list)
		#define FAULT10 (FI_TCBp->xStateListItem).pxNext		// u32*	(xLIST_ITEM*)Pointer to the next item in the list
		#define FAULT11 (FI_TCBp->xStateListItem).pxPrevious	// u32*	(xLIST_ITEM*)Pointer to the previous item in the list
		#define FAULT12 (FI_TCBp->xStateListItem).pvOwner		// u32*	(void*)Pointer to the TCB structure that owns that structure
		#define FAULT13 (FI_TCBp->xStateListItem).pvContainer	// u32* (void*)Pointer to the containing list
		#define FAULT14 (FI_TCBp->xEventListItem).xItemValue	// u32	Cardinal number of the listed element (If the task is part of an event list)
		#define FAULT15 (FI_TCBp->xEventListItem).pxNext		// u32*	(xLIST_ITEM*)Pointer to the next item in the list
		#define FAULT16 (FI_TCBp->xEventListItem).pxPrevious 	// u32*	(xLIST_ITEM*)Pointer to the previous item in the list
		#define FAULT17 (FI_TCBp->xEventListItem).pvOwner		// u32*	(void*)Pointer to the TCB structure that owns that structure
		#define FAULT18 (FI_TCBp->xEventListItem).pvContainer	// u32*	(void*)Pointer to the containing list
	#endif

	#if (cFIE_LIST == 3)										// *** FIE_LIST 3 - Injection on delayed or suspended tasks lists
		#define cFIE_LIST_LEN		(uint16_t)5
		#define FAULT0 FI_Listp->uxNumberOfItems				// u32	Number of items in the concatenated list
		#define FAULT1 FI_Listp->pxIndex						// u32*	(ListItem_t*)Used to walk through the list. Points to the last item returned by a call to listGET_OWNER_OF_NEXT_ENTRY()
		#define FAULT2 (FI_Listp->xListEnd).xItemValue			// u32 	The value being listed.  In most cases this is used to sort the list in descending order
		#define FAULT3 (FI_Listp->xListEnd).pxNext				// u32*	(xLIST_ITEM*)Pointer to the next ListItem_t in the list
		#define FAULT4 (FI_Listp->xListEnd).pxPrevious			// u32*	(xLIST_ITEM*)Pointer to the previous ListItem_t in the list
	#endif

	#if (cFIE_LIST == 4)														// *** FIE_LIST 4 - Mutex/Semaphore data
		#define cFIE_LIST_LEN		(uint16_t)22
		FIE_Queue_t *FI_Queuep;													// Mutex/Queue variable to be used for assignment
		#define FAULT0 FI_Queuep->pcHead										// u32*	Pointer to the beginning of the queue storage area
		#define FAULT1 FI_Queuep->pcTail										// u32*	Pointer to the end of the queue storage area
		#define FAULT2 FI_Queuep->pcWriteTo										// u32* Pointer to the last position written in the queue; initialized with pcHead
		#define FAULT3 (FI_Queuep->u).pcReadFrom								// u32*	Pointer to the last item accessed in read mode when the struct is used as queue
		#define FAULT4 (FI_Queuep->u).uxRecursiveCallCount						// u32	Number of times the mutex is taken when the structure is used as mutex
		#define FAULT5 (FI_Queuep->xTasksWaitingToSend).uxNumberOfItems			// u32  List of tasks that are blocked waiting to post onto this queue
		#define FAULT6 (FI_Queuep->xTasksWaitingToSend).pxIndex					// u32*
		#define FAULT7 (FI_Queuep->xTasksWaitingToSend).xListEnd.xItemValue		// u32
		#define FAULT8 (FI_Queuep->xTasksWaitingToSend).xListEnd.pxNext			// u32*
		#define FAULT9 (FI_Queuep->xTasksWaitingToSend).xListEnd.pxPrevious		// u32*
		#define FAULT10 (FI_Queuep->xTasksWaitingToReceive).uxNumberOfItems		// u32  List of tasks that are blocked waiting to read from this queue
		#define FAULT11 (FI_Queuep->xTasksWaitingToReceive).pxIndex				// u32*
		#define FAULT12 (FI_Queuep->xTasksWaitingToReceive).xListEnd.xItemValue	// u32
		#define FAULT13 (FI_Queuep->xTasksWaitingToReceive).xListEnd.pxNext		// u32*
		#define FAULT14 (FI_Queuep->xTasksWaitingToReceive).xListEnd.pxPrevious	// u32*
		#define FAULT15 FI_Queuep->uxMessagesWaiting							// u32	The number of items currently in the queue
		#define FAULT16 FI_Queuep->uxLength										// u32  The number of items the queue will hold, not the number of bytes
		#define FAULT17 FI_Queuep->uxItemSize									// u32	The size of each items that the queue will hold
		#define FAULT18 FI_Queuep->cRxLock										// s8	Stores the number of items received from the queue (removed from the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked
		#define FAULT19 FI_Queuep->cTxLock										// s8	Stores the number of items transmitted to the queue (added to the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked
		#define FAULT20 FI_Queuep->uxQueueNumber								// u32 	DEBUG Just for debug
		#define FAULT21 FI_Queuep->ucQueueType									// u8	DEBUG Just for debug
	#endif

	#if (cFIE_LIST == 5)								// *** FIE_LIST 5 - Injection on a user defined location
		#define cFIE_LIST_LEN		(uint16_t)1			// User defined list length
		#define FAULT0 ((size_t*)0x20000f0f)			// User defined memory location
	#endif

	#define CONCAT(A,B)	A##B
	#define FAULT(N) CONCAT(FAULT, N)

#endif

#endif
