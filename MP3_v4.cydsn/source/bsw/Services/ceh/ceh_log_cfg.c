/**
* \file ceh_log_cfg.c
* \author Duyen
* \date 25.02.2025
*
* \brief User defined functions file which contains required functions used for logging
*
* Here SEND_API function prototypes are provided so that the user can configure the required interface
* to log out the error message by providing the own driver implementation. Dump API function prototype is provided as
* well.
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

#include "ceh_log_cfg.h"

#include "sprintf.h"
#include "log.h"		// Access to Uart (usbLog)
//#include "swc_remote.h"	// Access to Xbee (REMOTE_uart)

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

#define TIME_STR_LENGTH		20
#define CEH_STR_BUFF_SIZE	100


/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/

/**
 * By giving the ENUM event as index in the CEH_source_strings array, the corresponding string is returned.
 * The user can use this string to form up a readable message to send out to a terminal
 */

/**\brief Array of strings, containing human readable representations of event sources */
const char * const CEH_source_strings[] = {
		CEH_CFG(CEH_CFG_ENUM_STR)
};

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/


/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/


/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

/**
 * \brief Formats an error message
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \param char* buffer			: [IN/OUT] Buffer to contain the formated error message
 * \param uint16_t bufferSize	: [IN] The size of the provided buffer
 * \return RC_t 				: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not
 * 										large enough, RC_SUCCESS if success
 */
static RC_t CEH__formatErrMsg(CEH_event_t event, uint32_t data, uint32_t currTime, char* buffer, uint16_t bufferSize);

/**
 * \brief Formats a milliseconds value to a timestamp string
 *
 * \param uint32_t currTime			: [IN] Time in milliseconds
 * \param char* timeStrBuffer		: [IN/OUT] Buffer to contain the formated time string
 * \param uint16_t bufferSize		: [IN] The size of the provided buffer
 * \return RC_t 					: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not
 * 											large enough, RC_SUCCESS if success
 */
static RC_t CEH__ms_to_timestamp(uint32_t currTime, char* timeStrBuffer, uint16_t bufferSize);


/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * \brief Send data through UART interface
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_UART_send(CEH_event_t event, uint32_t data, uint32_t currTime)
{
	// user defined function
	RC_t ret = RC_SUCCESS;

	// Construct the error message
	char errMsg[CEH_STR_BUFF_SIZE];
	ret = CEH__formatErrMsg(event, data, currTime, errMsg, CEH_STR_BUFF_SIZE);
	if (RC_SUCCESS != ret) return ret;

	// Call driver function to send the packet
	UART_LOG_PutString(errMsg);

	return ret;
}

/**
 * \brief Save data in EEPROM
 *
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_EEPROM_send(CEH_event_t event, uint32_t data, uint32_t currTime)
{
	// user defined function
	// user defined function
	RC_t ret = RC_SUCCESS;

	// Construct the error message
	char errMsg[CEH_STR_BUFF_SIZE];
	ret = CEH__formatErrMsg(event, data, currTime, errMsg, CEH_STR_BUFF_SIZE);
	if (RC_SUCCESS != ret) return ret;

	// Call driver function to send the packet
	//ToDo: ret = eeprom.putString(errMsg);

	return ret;
}

/**
 * \brief Send data through UART interface and save it in EEPROM
 *
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_UART_EEPROM_send(CEH_event_t event, uint32_t data, uint32_t currTime)
{
	// user defined function
	RC_t ret = RC_SUCCESS;

	// Construct the error message
	char errMsg[CEH_STR_BUFF_SIZE];
	ret = CEH__formatErrMsg(event, data, currTime, errMsg, CEH_STR_BUFF_SIZE);
	if (RC_SUCCESS != ret) return ret;

	// Call driver function to send the packet
	UART_LOG_PutString(errMsg);
	//ToDo: ret = eeprom.putString(errMsg);

	return ret;
}

/**
 * \brief Send data through XBEE interface
 *
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_UART_XBEE_send(CEH_event_t event, uint32_t data, uint32_t currTime)
{
	// user defined function
	RC_t ret = RC_SUCCESS;

	// Construct the error message
	char errMsg[CEH_STR_BUFF_SIZE];
	ret = CEH__formatErrMsg(event, data, currTime, errMsg, CEH_STR_BUFF_SIZE);
	if (RC_SUCCESS != ret) return ret;

	// Call driver function to send the packet
	UART_LOG_PutString(errMsg);
	//ToDo: ret = XBEE.putString(errMsg);

	return ret;
}

/**
 * \brief Send data through WIFI interface
 *
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_WIFI_send(CEH_event_t event, uint32_t data, uint32_t currTime)
{
	// user defined function
	RC_t ret = RC_SUCCESS;

	// Construct the error message
	char errMsg[CEH_STR_BUFF_SIZE];
	ret = CEH__formatErrMsg(event, data, currTime, errMsg, CEH_STR_BUFF_SIZE);
	if (RC_SUCCESS != ret) return ret;

	// Call driver function to send the packet
	//ToDo: ret = WIFI.putString(errMsg);

	return ret;
}

/**
 * \brief Dump out the error records saved in the internal ring buffer
 *
 * \param CEH_event_log_entry_t *record		: [IN] Pointer to the event error record
 * \return RC_t								: [OUT] Status of the dumping
 */
RC_t CEH_dumpErrorRecord(CEH_event_log_entry_t * record)
{

	// user defined function
	/**
	 * construct the packet
	 * call the driver func to send the packet
	 */

	// Dump records via Uart. Maybe it is better to dump the data to non-volatile memory

	//usbLog.putString("\n\n-----------------DUMPING DATA--------------------\n\n");

	// user defined function
	RC_t ret = RC_SUCCESS;

	// Construct the error message
	char errMsg[CEH_STR_BUFF_SIZE];
	ret = CEH__formatErrMsg(record->event, record->data, record->timestamp, errMsg, CEH_STR_BUFF_SIZE);
	if (RC_SUCCESS != ret) return ret;

	// Call driver function to send the packet
	UART_LOG_PutString(errMsg);

/*
	char errBuffer[CEH_STR_BUFF_SIZE];
	usbLog.putString("\n");

	IO_snprintf((uint8_t*)errBuffer, CEH_STR_BUFF_SIZE, "%s - ", CEH_source_strings[record->event]);
	usbLog.putString(errBuffer);

	IO_snprintf((uint8_t*)errBuffer, CEH_STR_BUFF_SIZE, "%ld at ", record->data);
	usbLog.putString(errBuffer);

	IO_snprintf((uint8_t*)errBuffer, CEH_STR_BUFF_SIZE, "%ld ms\n", record->timestamp);
	usbLog.putString(errBuffer);
*/
	return RC_SUCCESS;
}


/**
 * \brief Formats an error message
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \param char* buffer			: [IN/OUT] Buffer to contain the formated error message
 * \param uint16_t bufferSize	: [IN] The size of the provided buffer
 * \return RC_t 				: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not
 * 										large enough, RC_SUCCESS if success
 */
RC_t CEH__formatErrMsg(CEH_event_t event, uint32_t data, uint32_t currTime, char* buffer, uint16_t bufferSize)
{
	// Get timestamp string
	char timeStr[TIME_STR_LENGTH];
	RC_t ret = CEH__ms_to_timestamp(currTime, timeStr, TIME_STR_LENGTH);
	if (RC_SUCCESS != ret) return ret;

	// Construct the error message
	ret = IO_snprintf((uint8_t*)buffer, bufferSize, "CEH [%s] %s : %d\n", timeStr, CEH_source_strings[event], (int)data);

	return ret;
}

/**
 * \brief Formats a milliseconds value to a timestamp string
 *
 * \param uint32_t currTime			: [IN] Time in milliseconds
 * \param char* timeStrBuffer		: [IN/OUT] Buffer to contain the formated time string
 * \param uint16_t bufferSize		: [IN] The size of the provided buffer
 * \return RC_t 					: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not
 * 											large enough, RC_SUCCESS if success
 */
RC_t CEH__ms_to_timestamp(uint32_t currTime, char* timeStrBuffer, uint16_t bufferSize)
{
	uint32_t hh, mm, ss, ms =  0;
	uint32_t t =  currTime;

	hh = t / 3600 / 1000;
	t -= hh * 3600 * 1000;
	mm = t / 60 / 1000;
	t -= mm * 60 * 1000;
	ss = t / 1000;
	t -= ss * 1000;
	ms = t;

	RC_t ret = IO_snprintf((uint8_t*)timeStrBuffer, bufferSize, "%02d:%02d:%02d.%03d", (int)hh, (int)mm, (int)ss, (int)ms);

	return ret;
}
