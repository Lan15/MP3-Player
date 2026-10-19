/**
* \file log_cfg.c
* \author Duyen
* \date 14.02.2025
*
* \brief Configuration file for logging
*
* Here we define the logging level and buffer sizes.
* Furthermore, the glue logic functions to the logging MCAL function, e.g. a UART driver are implemented here
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

#include "log_cfg.h"
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
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_e(uint8_t* buffer)
{
	// user code
	UART_LOG_PutString("ERROR   : ");
	UART_LOG_PutString((char*)buffer);
	UART_LOG_PutString("\n");
}

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_d(uint8_t* buffer)
{
	// user code
	UART_LOG_PutString("DEBUG   : ");
	UART_LOG_PutString((char*)buffer);
	UART_LOG_PutString("\n");
}

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_v(uint8_t* buffer)
{
	// user code
	UART_LOG_PutString("VERBOSE : ");
	UART_LOG_PutString((char*)buffer);
	UART_LOG_PutString("\n");
}

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_i(uint8_t* buffer)
{
	// user code
	UART_LOG_PutString("INFO    : ");
	UART_LOG_PutString((char*)buffer);
	UART_LOG_PutString("\n");
}

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_w(uint8_t* buffer)
{
	// user code
	UART_LOG_PutString("WARNING : ");
	UART_LOG_PutString((char*)buffer);
	UART_LOG_PutString("\n");
}

//--------------------------------------------------------------------------

