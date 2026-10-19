/**
* \file log_cfg.h
* \author Duyen
* \date 14.02.2925
*
* \brief Configuration file for logging
*
* Here we define the logging level and buffer sizes. 
* Furthermore, the glue logic functions to the logging MCAL function, e.g. a UART driver are declared here
*
* \note <notes>
* Programming rules (may be deleted in the final release of the file)
* ===================================================================
*
* 1. Naming conventions:
*    - Prefix of your module in front of every function and static data. 
*    - Scope _ for public and __ for private functions / data / types, e.g. 
*       Public:  void CONTROL_straightPark_Init();
*       Private: static void CONTROL__calcDistance();
*       Public:  typedef enum {RED, GREEN, YELLOW} CONTROL_color_t
*    - Own type definitions e.g. for structs or enums get a postfix _t
*    - #define's and enums are written in CAPITAL letters
* 2. Code structure
*    - Be aware of the scope of your modules and functions. Provide only functions which belong to your module to your files
*    - Prepare your design before starting to code
*    - Implement the simple most solution (Too many if then else nestings are an indicator that you have not properly analysed your task)
*    - Avoid magic numbers, use enums and #define's instead
*    - Make sure, that all error conditions are properly handled
*    - If your module provides data structures, which are required in many other files, it is recommended to place them in a file_type.h file
*	  - If your module contains configurable parts, is is recommended to place these in a file_config.h|.c file
* 3. Data conventions
*    - Minimize the scope of data (and functions)
*    - Global data is not allowed outside of the signal layer (in case a signal layer is part of your design)
*    - All static objects have to be placed in a valid linker sections
*    - Data which is accessed in more than one task has to be volatile and needs to be protected (e.g. by using messages or semaphores)
*    - Do not mix signed and unsigned data in the same operation
* 4. Documentation
*    - Use self explaining function and variable names
*    - Use proper indentation
*    - Provide Javadoc / Doxygen compatible comments in your header file and C-File
*    		- Every  File has to be documented in the header
*			- Every function parameter and return value must be documented, the valid range needs to be specified
*     		- Logical code blocks in the C-File must be commented
*    - For a detailed list of doxygen commands check http://www.stack.nl/~dimitri/doxygen/index.html 
* 5. Qualification
*    - Perform and document design and code reviews for every module
*    - Provide test specifications for every module (focus on error conditions)
*
* Further information:
*    - Check the programming rules defined in the MIMIR project guide
*         - Code structure: https://fromm.eit.h-da.de/intern/mimir/methods/eng_codestructure/method.htm
*         - MISRA for C: https://fromm.eit.h-da.de/intern/mimir/methods/eng_c_rules/method.htm
*         - MISRA for C++: https://fromm.eit.h-da.de/intern/mimir/methods/eng_cpp_rules/method.htm 
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


#ifndef LOG_CFG_H_
#define LOG_CFG_H_

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include "global.h"
#include "project.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#define CFG_LOGGING_LEVEL       LOG_VERBOSE /** \brief Default logging level */
#define LOG_BUFFER_SIZE 		100 		/** \brief Default buffer size */

#define LOG_TXSENDERSIZE		16
#define LOG_TXMSGSIZE    		48

#define LOG_TXBUFFER_SIZE		500		//For PSOC, must be set in the topdesign / UART component


/*****************************************************************************/
/* Type declarations                                                         */
/*****************************************************************************/

/**
 * \brief Debugging Level.
 *
 * The debugging level defines the character of an event and is used to define which events are send to the log using #LOGGING_LEVEL
 */
typedef enum{
    LOG_VERBOSE=0,											/**< \brief Low Level Information */
    LOG_INFO=1,												/**< \brief High level information during normal operation */
    LOG_WARNING=2,											/**< \brief abnormal operation, which might be fixed */
    LOG_ERROR=3,											/**< \brief abnormal operation, which can not be fixed */
    LOG_DEBUG=4,											/**< \brief high priority for debugging messages (use only temporarily for debugging) */
    LOG_OFF=5												/**< \brief Event logging is switched off */
}LOG_level_t;

/*****************************************************************************/
/* User functions                                                            */
/*****************************************************************************/

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_e(uint8_t* buffer);

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_d(uint8_t* buffer);

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_v(uint8_t* buffer);

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_i(uint8_t* buffer);

/**
 * \brief user code which send out/store the buffer
 *
 * \param uint8_t* buffer      : [IN] string to be printed
 * \return void
 */
void LOG_print_w(uint8_t* buffer);

#endif /* LOG_CFG_H_ */
