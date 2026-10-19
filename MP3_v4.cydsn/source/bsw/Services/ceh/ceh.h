/**
* \file ceh.h
* \author Duyen
* \date 07.02.2025
*
* \brief Central error handler service which reports errors occurred during the system's operation. Reporting feature
* can be configured by the user in the configuration file
*
*
* The API provides function prototypes so that the user can configure the required interface
* to log out the error message, and provide as well own driver implementation
*
*
* \note <notes>
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

#ifndef CEH_H_
#define CEH_H_

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

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
/**
 * \brief Report an store an diagnostic event
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \return RC_t 				: [OUT] RC_SUCCES or error code
 */
RC_t CEH_report(CEH_event_t CEH_event, uint32_t data);

/**
 * \brief Init content of CEH Buffer
 *
 * \return RC_t			: [OUT] RC_SUCCESS
 */
RC_t CEH_init();

/**
 * \brief Clear content of CEH Buffer
 *
 * \return RC_t			: [OUT] RC_SUCCESS
 */
RC_t CEH_clear();

/**
 * \brief Dump out the error records saved in the internal ring buffer
 *
 * \return RC_t			: [OUT] RC_SUCCESS
 */
RC_t CEH_dump();

//---------------------------------------------------------------- Diagnostic helper functions - use with care

/**
 * Will return the number of elements
 */
uint16_t CEH_getNumberElements();

/**
 * \brief Will provide the error entry at the given index
 *
 * \param uint16_t index				: [IN] The index for the returned error entry. Value must be in the range 0...fillLevel
 * \param CEH_event_log_entry_t* pEntry : [OUT] The returned index. Will be 0 in case of invalid index
 *
 * \return RC_BADPARAM in case of invalid index, RC_WRONGSTATE in case of wrong CEH configuration
 */
RC_t CEH_getBufferyEntry(uint16_t index, CEH_event_log_entry_t* pEntry);


#endif /* CEH_H_ */
