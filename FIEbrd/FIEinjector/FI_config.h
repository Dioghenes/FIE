/******************************************************
 * Dioghenes
 * Polytechnic of Turin / LIRMM
 * 2018
 * FIE - Fault Injection Environment v04.6
 ******************************************************/

#ifndef FI_CONFIG_H
#define FI_CONFIG_H


/***** SETUP FIE - Setup the FIE from here *****/

	// ***** FIE MODE SELECTION
#define cFIE_urtmode			1		// SIJ,INJ,DEP,RAD mode
#define cFIE_runmode			0		// RUN mode - nothing happens
#define cFIE_ENABLED			cFIE_urtmode
#define cFIE_INJECT_ENABLED		0		// Set to 0 to create golden, to 1 to do injection
#define cFIE_PRECISEINJ_ENABLED	0		// Set to 1 to enable precise location injection
#define cFIE_TRACE_ENABLED		1		// Set to 1 to enable tracer
#define cFIE_TRACEPLUS_ENABLED	0		// Set to 1 to enable SEGGER RTT advanced tracer

	// ***** FIE INJECTION TYPE SELECTION
#define cFIE_OP_FLIP			^			//  Flip the masked bits
#define cFIE_OP_SET				|			//	Set to 1 the masked bits
#define cFIE_OP_RST				&			//	Reset to 0 the masked bits
#if(cFIE_INJECT_ENABLED)
	#define cFIE_INJ_MASK		0x00000001		// 	Bit mask used to do injection (single/multiple)
	#define cFIE_OP				cFIE_OP_FLIP	// '^' or '|'or '&' are the possible
#else
	#define cFIE_INJ_MASK		0x00000000		// Do not change
	#define cFIE_OP				cFIE_OP_SET		// Do not change
#endif

	// ***** Number of running benchmarks
#define cFIE_BM_NUMBER			3			// Number of running benchmarks

	// ***** Select the list to use during injection
#define cFIE_LIST 				1
	// ***** FIE Lists 2 and 3 require more setups
#if(cFIE_LIST == 2)
	#define cFIE_LIST2_IDLTCB	0
	#define cFIE_LIST2_CURTCB	1
	#define cFIE_LIST2_RDYTCB	2
	#define cFIE_LIST2_DLDTCB	3
	#define cFIE_LIST2_SUSTCB	4
	#define cFIE_LIST2_WHAT		cFIE_LIST2_DLDTCB
	#define cFIE_LIST2_PRIO		6
#elif(cFIE_LIST == 3)
	#define cFIE_LIST3_RDYLST	0
	#define cFIE_LIST3_DLDLST	1
	#define cFIE_LIST3_OVFLST	2
	#define cFIE_LIST3_SUSLST	3
	#define cFIE_LIST3_WHAT		cFIE_LIST3_DLDLST
	#define cFIE_LIST3_PRIO		6
#endif

	// ***** Special setup for TRACEPLUS mode
#if (cFIE_TRACEPLUS_ENABLED)
	#define cFIE_RES_T	 		2000	// Period
	#define cFIE_RES_D	 		500 	// Resolution prescaler
	#define cFIE_ANL_BUFLEN		64		// Set event buffer length
#endif

	// ***** Setup the defines for the resume timer (SIJ,INJ,DEP,RAD)
#define cFIE_INJ_TIMRPER		10000	// Resume TIM3 period setting
#define cFIE_INJ_TIMRPRES		5000	// Resume TIM3 prescaler setting

	// ***** FIEmon settings
#define cFIEmon_STARTITCHAR		"+"		// FIEmon : Say the PC to send the injection parameters
#define cFIEmon_STOPITCHAR		"."		// FIEmon : Say the PC to send the injection parameters



/***** FIE COMPILE ERRORS/WARNINGS *****/
#if (cFIE_ENABLED!=cFIE_runmode) && (cFIE_ENABLED!=cFIE_urtmode)
	#error "cFIE_ENABLED has a wrong configuration."
#endif

#endif
