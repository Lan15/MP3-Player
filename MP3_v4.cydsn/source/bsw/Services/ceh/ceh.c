/**
* \file ceh.c
* \author Duyen
* \date 07.02.2025
*
* \brief Central error handler service which reports errors occurred during the system's operation. Reporting feature
* can be configured by the user in the configuration file
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

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "ceh.h"

#include "ceh_log_cfg.h"

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

//The following inituialisations do not work, as the common_data section in reality is a bss section
//Todo: Create proper base system section

/**\brief Buffer which stores the error events */
//__attribute__ ((section (".common_data"))) static CEH_event_log_entry_t CEH__buffer[CEH_BUFFERSIZE] = {};
__attribute__ ((section ("BaseSystem"))) static CEH_event_log_entry_t CEH__buffer[CEH_BUFFERSIZE] = {};

/**\brief Default CEH object */
//__attribute__ ((section (".common_data"))) static CEH_t CEH__errorList = {
//		.size = CEH_BUFFERSIZE,
//		.nextFree = 0,
//		.buffer = &CEH__buffer[0]
//};
__attribute__ ((section ("BaseSystem"))) static CEH_t CEH__errorList = {
		.size = CEH_BUFFERSIZE,
		.nextFree = 0,
		.buffer = &CEH__buffer[0]
};



/**\brief Configuration storage */
static const CEH_event_table_entry_t CEH__event__table[] = {
		CEH_CFG(CEH_CFG_ENTRY)
};

/**\brief Table which stores the logging interface for the corresponding enum destination  */
static const CEH_logger_t CEH__log__table[] = {
		LOGGER_CFG(LOGGER_CFG_ENTRY)
};

/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/


/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * \brief Report an store an diagnostic event
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \return RC_t 				: [OUT] RC_SUCCES or error code
 */
RC_t CEH_report(CEH_event_t CEH_event, uint32_t data)
{
	RC_t result = RC_SUCCESS;

	uint32_t currentTime = CEH_getTimeInMs();

	if (CEH__errorList.nextFree < CEH__errorList.size)
	{
		CEH_lock();

		CEH__errorList.buffer[CEH__errorList.nextFree].event = CEH_event;
		CEH__errorList.buffer[CEH__errorList.nextFree].data = data;
		CEH__errorList.buffer[CEH__errorList.nextFree].timestamp = currentTime;

		CEH__errorList.nextFree++;

		CEH_unlock();
	}
	else
	{
		result = RC_ERROR_BUFFER_FULL;
	}

	// Check if logging is enabled and call the appropriate log function
	CEH_log_t logDest = CEH__event__table[CEH_event].logDestination;
	if (logDest != NO_LOG)
	{
		CEH_logFunc_t logger = CEH__log__table[logDest].logger;

		if (logger != 0)
		{
			result = logger(CEH_event, data, currentTime);
		}
	}

	// Call the error handler
	switch(CEH__event__table[CEH_event].action){
	case CEH_NONE:
		break;

	case CEH_STOP:
		CEH_deactiveActor();
		break;

	case CEH_EMERGENCY_STOP:
		CEH_emergencyStop();
		break;
	}

	// Check if debugger is enabled
	if (CEH__event__table[CEH_event].debugger == ON)
	{
		CEH_debug();
	}

	return result;
}

/**
 * \brief Init content of CEH Buffer
 *
 * \return RC_t			: [OUT] RC_SUCCESS
 */
RC_t CEH_init()
{
	CEH__errorList.size = CEH_BUFFERSIZE;
	CEH__errorList.buffer = &CEH__buffer[0];
	CEH__errorList.nextFree = 0;
    return RC_SUCCESS;
}

/**
 * \brief Clear content of CEH Buffer
 *
 * \return RC_t			: [OUT] RC_SUCCESS
 */
RC_t CEH_clear()
{
	CEH__errorList.nextFree = 0;
    return RC_SUCCESS;
}

/**
 * \brief Dump out the error records saved in the internal ring buffer
 *
 * \return RC_t			: [OUT] RC_SUCCESS
 */
RC_t CEH_dump()
{
	RC_t result = RC_SUCCESS;

    CEH_lock();

	for (uint32_t i=0; i<CEH__errorList.nextFree; i++)
	{
		CEH_event_log_entry_t record = CEH__errorList.buffer[i];

		result = CEH_dumpErrorRecord(&record);
	}

    CEH_unlock();

	return result;
}

/**
 * Will return the number of elements
 */
uint16_t CEH_getNumberElements()
{
	return CEH__errorList.nextFree;
}

/**
 * \brief Will provide the error entry at the given index
 *
 * \param uint16_t index				: [IN] The index for the returned error entry. Value must be in the range 0...fillLevel
 * \param CEH_event_log_entry_t* pEntry : [OUT] The returned index. Will be 0 in case of invalid index
 *
 * \return RC_BADPARAM in case of invalid index, RC_WRONGSTATE in case of wrong CEH configuration
 */
RC_t CEH_getBufferyEntry(uint16_t index, CEH_event_log_entry_t* pEntry)
{
	if (0 == CEH__errorList.buffer || 0 ==  CEH__errorList.size)
	{
		return RC_ERROR_INVALID_STATE;
	}

	if (index > CEH__errorList.nextFree)
	{
		return RC_ERROR_BAD_PARAM;
	}

	//Get the corresponding entry
	*pEntry = CEH__errorList.buffer[index];

	return RC_SUCCESS;
}
