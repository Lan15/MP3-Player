/**
* \file ceh_cfg.h
* \author Duyen
* \date 07.02.2025
*
* \brief Configuration file for central error handler service
*
* Here we define the buffer size of the central error handler service, and the configuration for this service.
* User defined functions which are required for the CEH service are declared here
*
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

#ifndef CEH_CFG_H_
#define CEH_CFG_H_

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "global.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#define CEH_BUFFERSIZE 100  /** \brief Size of the buffer stores the error records */

/*****************************************************************************/
/* Type declarations                                                         */
/*****************************************************************************/

/*=======================[ Logger Configurations ]===============================================================*/

/**
 * The user can configure the communication interfaces are required to
 * be used in their project.
 *
 * LOG_DEST: Name of the communication interface, new interface name can be added as needed
 * SEND_API: Callback function respective to such interface. 0 indicate no-op. They should be
 * define in the ceh_usr_logger files
 *
 */

//================= LOG_DEST			SEND_API ===============================================================//
#define LOGGER_CFG(DEF) \
	DEF(			UART,				CEH_UART_send) \
	DEF(			EEPROM,				CEH_EEPROM_send) \
	DEF(			UART_EEPROM, 		CEH_UART_EEPROM_send) \
	DEF(			UART_XBEE, 			CEH_UART_XBEE_send) \
	DEF(			WIFI, 				CEH_WIFI_send) \
	DEF(			NO_LOG, 			0) \

/*=======================[ Event Configurations ]===============================================================*/

/**
 * The user can configure the event configuration table as needed. Based on the configuration,
 * the corresponding behavior is activated
 *
 * Event: Error event that is reported to the CEH. New event can be added as needed
 * ACTION_DEBUG: ON/OFF, enable/disable the debug callback function
 * LOG_DEST: Enum value coming from the LOGGER_CFG table
 * ACTUATOR_ACTION: CEH_NONE/CEH_STOP/CEH_EMERGENCY_STOP, the corresponding safestate action
 * is called based on the ENUM configured
 */

//================= Event Name						ACTION_DEBUG	LOG_DEST		ACTUATOR_ACTION	 ===============================================//
#define CEH_CFG(DEF) \
	DEF(			EMPTY,							OFF,			NO_LOG,			CEH_NONE			)	/* NOP, do not remove */ \
	DEF(			HASBOOTED,						OFF,			UART,			CEH_NONE			)	/* Bootup message */ \
	/* ------------------- MCAL Errors */\
	DEF(			DRIVER_RETURN,                	ON,				UART, 			CEH_NONE    		)   /* Driver return code error */ \
    /* ------------------- Service Level Errors */\
	DEF(			RB_FULL,                		ON,				UART, 			CEH_NONE    		)   /* Ringbuffer full */ \
	DEF(			RB_EMPTY,                		OFF,			UART, 			CEH_NONE    		)   /* Ringbuffer empty */ \
    DEF(			MESSAGE_MALLOC,					ON,             UART,           CEH_NONE            )	/* message buffer could not be allocated */	\
    DEF(			MESSAGE_BUFFERFULL,				ON,             UART,	        CEH_NONE            )	/* message buffer overrun */	\
 	/* ------------------- RTE Level Errors */\
	/* ------------------- Application Level Errors */\
    DEF(			FATAL,				            ON,             UART,           CEH_EMERGENCY_STOP  )	/* Fatal application error */	\
    DEF(            SD_SCAN_FAILED,                 OFF,            UART,           CEH_NONE            )   /* folder could not be opened, data = RC_t          */ \
    DEF(            FILE_OPEN_FAILED,               OFF,            UART,           CEH_NONE            )   /* FS_FOpen failed, data = song index               */ \
    DEF(            WAV_INVALID,                    OFF,            UART,           CEH_NONE            )   /* header rejected, data = RC_t of WAV_ParseHeader  */ \
    DEF(            SD_READ_FAILED,                 OFF,            UART,           CEH_STOP            )   /* read error during playback, data = song index    */ \
    DEF(            SOUND_UNDERRUN,                 OFF,            UART,           CEH_NONE            )   /* reader late, data = underruns in the last second */ \
    
/*****************************************************************************/
/* User functions                                                            */
/*****************************************************************************/

/**
 * \brief Callback function if the debug feature is enable for this
 * error event
 *
 */
void CEH_debug();

/**
 * \brief Callback function when the actuator action for the event
 * is configured to CEH_STOP
 *
 */
void CEH_deactiveActor();

/**
 * \brief Callback function when the actuator action for the event
 * is configured to CEH_EMERGENCY_STOP
 *
 */
void CEH_emergencyStop();

/**
 * \brief Lock the critical region
 *
 */
void CEH_lock();

/**
 * \brief Unlock the critical region
 *
 */
void CEH_unlock();

/**
 * \brief Get the time in millisecond
 *
 * \return uint32_t		: [OUT] Current time in millisecond
 */
uint32_t CEH_getTimeInMs();

#endif /* CEH_CFG_H_ */
