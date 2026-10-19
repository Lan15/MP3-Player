/**
* \file ceh_log_cfg.h
* \author Duyen
* \date 25.02.2025
*
* \brief User defined functions file which contains required functions used for logging
*
* Here SEND_API function prototypes are provided so that the user can configure the required interface
* to log out the error message by providing the own driver implementation. Dump API function prototype is provided as
* well.
*
*
* The API provides function prototypes so that the user can configure the required interface
* to log out the error message, and provide as well own driver implementation
*
*
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

#ifndef CEH_LOG_CFG_H_
#define CEH_LOG_CFG_H_

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "global.h"
#include "ceh_type.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/


/*****************************************************************************/
/* Type declarations                                                         */
/*****************************************************************************/


/*****************************************************************************/
/* User functions                                                            */
/*****************************************************************************/

/*=======================[ Log Function Prototypes ]===============================================================*/

/**
 * The function prototypes in this section provide some communication API that could be used for logging.
 * The user has to implement the actual code for them, or leave them as no-op function if they are not used.
 * They should be configured in the logger configuration table in the ceh_cfg.h if used.
 * In case a new communication API needs to be created, it has to follow the below API.
 *
 * \param CEH_event_t event     : [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */

/**
 * \brief Send data through UART interface
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_UART_send(CEH_event_t event, uint32_t data, uint32_t currTime);

/**
 * \brief Save data in EEPROM
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_EEPROM_send(CEH_event_t event, uint32_t data, uint32_t currTime);

/**
 * \brief Send data through UART interface and save it in EEPROM
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_UART_EEPROM_send(CEH_event_t event, uint32_t data, uint32_t currTime);

/**
 * \brief Send data through XBEE interface
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_UART_XBEE_send(CEH_event_t event, uint32_t data, uint32_t currTime);

/**
 * \brief Send data through WIFI interface
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] Status of the sending
 */
RC_t CEH_WIFI_send(CEH_event_t event, uint32_t data, uint32_t currTime);

/*=======================[ Dumping Function Prototypes ]===============================================================*/

/**
 * The user needs to provide the implementation for the below function prototype. The record
 * pointer is provided with essential properties to pass into the communication API
 * to dump out the message
 *
 */

/**
 * \brief Dump out the error records saved in the internal ring buffer
 *
 * \param CEH_event_log_entry_t *record		: [IN] Pointer to the event error record
 * \return RC_t								: [OUT] Status of the dumping
 */
RC_t CEH_dumpErrorRecord(CEH_event_log_entry_t * record);

#endif /* CEH_LOG_CFG_H_ */
