/**
* \file ceh_cfg.c
* \author Duyen
* \date 07.02.2025
*
* \brief Configuration file for central error handler service
*
* User defined functions which are required for the CEH service are implemented here
*
* \note <notes>
* \todo <todos>
* \warning <warnings, e.g. dependencies, order of execution etc.>
*
*  Changelog:\n
*  - <version; data of change; author>
*            - <description of the change>
*
* \copyright Copyright ©2025
* Department of electrical engineering and information technology, Hochschule Darmstadt - University of applied sciences (h_da). All Rights Reserved.
* Permission to use, copy, modify, and distribute this software and its documentation for educational, and research purposes in the context of non-commercial
* (unless permitted by h_da) and official h_da projects, is hereby granted for enrolled students of h_da, provided that the above copyright notice,
* this paragraph and the following paragraph appear in all copies, modifications, and distributions.
* Contact Prof.Dr.-Ing. Peter Fromm, peter.fromm@h-da.de, Birkenweg 8 64295 Darmstadt - GERMANY for commercial requests.
*
* \warning This software is a PROTOTYPE version and is not designed or intended for use in production, especially not for safety-critical applications!
* The user represents and warrants that it will NOT use or redistribute the Software for such purposes.
* This prototype is for research purposes only. This software is provided "AS IS," without a warranty of any kind.
*/

#ifndef CEH_CFG_C_
#define CEH_CFG_C_

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "global.h"
#include "project.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/




/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/


/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/


/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/


/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/


/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * \brief Callback function if the debug feature is enable for this
 * error event
 *
 */
void CEH_debug()
{
	// user defined function

	// Debug method depends on build setting
	#ifdef DEBUG
		//For debugger breakpoint (build configuration set to DEBUG)
		__asm("bkpt");
	#elif
		//For final system endless loop (build configuration set to RELEASE)
		while(1);
	#endif
}

/**
 * \brief Callback function when the actuator action for the event
 * is configured to CEH_STOP
 *
 */
void CEH_deactiveActor()
{
	// user defined function

}

/**
 * \brief Callback function when the actuator action for the event
 * is configured to CEH_EMERGENCY_STOP
 *
 */
void CEH_emergencyStop()
{
	// user defined function

}

/**
 * \brief Lock the critical region
 *
 */
void CEH_lock()
{
	// user defined function
	// Mutex required?
}

/**
 * \brief Unlock the critical region
 *
 */
void CEH_unlock()
{
	// user defined function
	// Mutex required?
}

/**
 * \brief Get the time in millisecond
 *
 * \return uint32_t		: [OUT] Current time in millisecond
 */
uint32_t CEH_getTimeInMs()
{
    TickType t = 0;
    GetCounterValue(cnt_systick, &t);
	return (uint32_t) t;
}

#endif /* CEH_CFG_C_ */
