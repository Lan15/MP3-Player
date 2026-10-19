/**
* \file ceh_type.h
* \author Duyen
* \date 25.02.2025
*
* \brief Fix configuration for central error handler service
*
* Here we define the macros, and provide the type definitions for the central error handler service
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

#ifndef CEH_TYPE_H_
#define CEH_TYPE_H_

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "global.h"
#include "ceh_cfg.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

/** \brief makro to create the enumeration values */
#define CEH_CFG_ENUM(name, debugAction, logAction, actAction) 			CEH_##name,

/** \brief Stringification of enumeration values */
#define CEH_CFG_ENUM_STR(name, debugAction, logAction, actAction) 		#name,

/** \brief Extraction of event action from the config */
#define CEH_CFG_ACTION(name, debugAction, logAction, actAction)			actAction,

/** \brief Makro to define an config entry */
#define CEH_CFG_ENTRY(name, debugAction, logAction, actAction)			{CEH_##name, debugAction, logAction, actAction},

/** \brief makro to create the enumeration values */
#define LOGGER_CFG_ENUM(name, handler) 		name,

/** \brief Makro to define a config entry */
#define LOGGER_CFG_ENTRY(name, handler)		{name, handler},

/*****************************************************************************/
/* Type declarations                                                         */
/*****************************************************************************/

/**
 * \brief CEH event escalation options
 */
typedef enum{
	CEH_NONE,				/**< \brief No safe state action */
	CEH_STOP,				/**< \brief Stop safe state action. e.g stop giving value to actuators */
	CEH_EMERGENCY_STOP,		/**< \brief Emergency safe state action. e.g disable the power for the actuators */
}CEH_reaction_t;

/**
 * \brief Logger source enumeration
 *
 */
typedef enum{
	LOGGER_CFG(LOGGER_CFG_ENUM)
	LOGGER_SOURCE_MAX				/**< \brief Number of events configured in the LOGGER */
} CEH_log_t;

/**
 * \brief Event source enumeration
 *
 */
typedef enum{
	CEH_CFG(CEH_CFG_ENUM)
	CEH_SOURCE_MAX					/**< \brief Number of events configured in the CEH */
} CEH_event_t;

/**
 * \brief Function pointer used as logger function
 *
 * \param CEH_event_t CEH_event	: [IN] Event to be reported
 * \param uint32_t data			: [IN] Possible payload
 * \param uint32_t currTime		: [IN] Timestamp in ms
 * \return RC_t 				: [OUT] RC_SUCCES or error code
 */
typedef RC_t (*CEH_logFunc_t)(CEH_event_t event, uint32_t data, uint32_t currTime);

/**
 * \brief Logger handler type
 *
 */
typedef struct {
	CEH_log_t dest;
	CEH_logFunc_t logger;
}CEH_logger_t;

/**
 * \brief Event storage entry type
 *
 */
typedef struct{
	CEH_event_t	    event;		/**< \brief Source of the error event */
	uint32_t        data;       /**< \brief Additional Payload */
    uint32_t        timestamp;  /**< \brief Timestamp in ms */
}CEH_event_log_entry_t;

/**
 * \brief Event configuration entry type
 *
 */
typedef struct {
	CEH_event_t event;			/**< \brief Source of the error event */
	uint8_t debugger;			/**< \brief Status of the debug option. e.g ON/OFF */
	CEH_log_t logDestination;	/**< \brief Interface logging the error event */
	CEH_reaction_t action;		/**< \brief Reaction for this event */
} CEH_event_table_entry_t;

/**
 * \brief CEH object type
 *
 */
typedef struct
{
    uint16_t size;                      /**< \brief Size of the buffer */
    uint16_t nextFree;                	/**< \brief Next Free Entry of the buffer */
    CEH_event_log_entry_t* buffer;      /**< \brief Pointer to the buffer */
} CEH_t;

/**\brief Array of strings, containing human readable representations of event sources */
extern const char * const CEH_source_strings[];

/*****************************************************************************/
/* User functions                                                            */
/*****************************************************************************/



#endif /* CEH_TYPE_H_ */
