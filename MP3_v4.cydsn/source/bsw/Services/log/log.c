/**
* \file log.c
* \author Duyen
* \date 14.02.2025
*
* \brief Logging service which use the own sprintf implementation without heap or global data usage
* to construct the message for the logging MCAL function to send to specific interface, e.g UART
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

#include "log.h"
#include "sprintf.h"

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
 * \brief printf like function which constructs the message with the given format.
 * This function used as LOG_VERBOSE and contains verbose level check. If it is passed,
 * the message will be constructed, and the corresponding function will be called to
 * pump out the message to the respective channel
 *
 * \param const char *format	: [IN] String with the format like sprintf
 * \param ...					: [IN] List of the arguments
 * \return RC_t					: [OUT] RC_ERROR_WRITE_FAILS if it fails writing due to low logging level,
 * RC_ERROR_UNDERRUN if the provided buffer size is not large enough, RC_ERROR_BAD_PARAM if
 * provided buffer length is 0, RC_SUCCESS if success
 */
RC_t LOG_V(const char* format, ...)
{
	if (LOG_VERBOSE < CFG_LOGGING_LEVEL) {
		return RC_ERROR_WRITE_FAILS;
	}

	uint8_t buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);

    // construct the message
    RC_t ret = IO_snprintf_va(buffer, LOG_BUFFER_SIZE, format, &args);
	va_end(args);

	if (ret == RC_SUCCESS) {
		// send out the message
		LOG_print_v(buffer);
	}

	return ret;
}

/**
 * \brief printf like function which constructs the message with the given format.
 * This function used as LOG_INFO and contains verbose level check. If it is passed,
 * the message will be constructed, and the corresponding function will be called to
 * pump out the message to the respective channel
 *
 * \param const char *format	: [IN] String with the format like sprintf
 * \param ...					: [IN] List of the arguments
 * \return RC_t					: [OUT] RC_ERROR_WRITE_FAILS if it fails writing due to low logging level,
 * RC_ERROR_UNDERRUN if the provided buffer size is not large enough, RC_ERROR_BAD_PARAM if
 * provided buffer length is 0, RC_SUCCESS if success
 */
RC_t LOG_I(const char* format, ...)
{
	if (LOG_INFO < CFG_LOGGING_LEVEL) {
		return RC_ERROR_WRITE_FAILS;
	}

	uint8_t buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);

    // construct the message
    RC_t ret = IO_snprintf_va(buffer, LOG_BUFFER_SIZE, format, &args);
	va_end(args);

	if (ret == RC_SUCCESS) {
		// send out the message
		LOG_print_i(buffer);
	}

	return ret;
}

/**
 * \brief printf like function which constructs the message with the given format.
 * This function used as LOG_WARNING and contains verbose level check. If it is passed,
 * the message will be constructed, and the corresponding function will be called to
 * pump out the message to the respective channel
 *
 * \param const char *format	: [IN] String with the format like sprintf
 * \param ...					: [IN] List of the arguments
 * \return RC_t					: [OUT] RC_ERROR_WRITE_FAILS if it fails writing due to low logging level,
 * RC_ERROR_UNDERRUN if the provided buffer size is not large enough, RC_ERROR_BAD_PARAM if
 * provided buffer length is 0, RC_SUCCESS if success
 */
RC_t LOG_W(const char* format, ...)
{
	if (LOG_WARNING < CFG_LOGGING_LEVEL) {
		return RC_ERROR_WRITE_FAILS;
	}

	uint8_t buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);

    // construct the message
    RC_t ret = IO_snprintf_va(buffer, LOG_BUFFER_SIZE, format, &args);
	va_end(args);

	if (ret == RC_SUCCESS) {
		// send out the message
		LOG_print_w(buffer);
	}

	return ret;
}

/**
 * \brief printf like function which constructs the message with the given format.
 * This function used as LOG_ERROR and contains verbose level check. If it is passed,
 * the message will be constructed, and the corresponding function will be called to
 * pump out the message to the respective channel
 *
 * \param const char *format	: [IN] String with the format like sprintf
 * \param ...					: [IN] List of the arguments
 * \return RC_t					: [OUT] RC_ERROR_WRITE_FAILS if it fails writing due to low logging level,
 * RC_ERROR_UNDERRUN if the provided buffer size is not large enough, RC_ERROR_BAD_PARAM if
 * provided buffer length is 0, RC_SUCCESS if success
 */
RC_t LOG_E(const char* format, ...)
{
	if (LOG_ERROR < CFG_LOGGING_LEVEL) {
		return RC_ERROR_WRITE_FAILS;
	}

	uint8_t buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);

    // construct the message
    RC_t ret = IO_snprintf_va(buffer, LOG_BUFFER_SIZE, format, &args);
	va_end(args);

	if (ret == RC_SUCCESS) {
		// send out the message
		LOG_print_e(buffer);
	}

	return ret;
}

/**
 * \brief printf like function which constructs the message with the given format.
 * This function used as LOG_DEBUG and contains verbose level check. If it is passed,
 * the message will be constructed, and the corresponding function will be called to
 * pump out the message to the respective channel
 *
 * \param const char *format	: [IN] String with the format like sprintf
 * \param ...					: [IN] List of the arguments
 * \return RC_t					: [OUT] RC_ERROR_WRITE_FAILS if it fails writing due to low logging level,
 * RC_ERROR_UNDERRUN if the provided buffer size is not large enough, RC_ERROR_BAD_PARAM if
 * provided buffer length is 0, RC_SUCCESS if success
 */
RC_t LOG_D(const char* format, ...)
{
	if (LOG_DEBUG < CFG_LOGGING_LEVEL) {
		return RC_ERROR_WRITE_FAILS;
	}

	uint8_t buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);

    // construct the message
    RC_t ret = IO_snprintf_va(buffer, LOG_BUFFER_SIZE, format, &args);
	va_end(args);

	if (ret == RC_SUCCESS) {
		// send out the message
		LOG_print_d(buffer);
	}

	return ret;
}


//--------------------------------------------------------------------------

