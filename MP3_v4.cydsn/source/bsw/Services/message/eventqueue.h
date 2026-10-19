/**
* \file message_queue.h
* \author P. Fromm
* \date 15.10.21
*
* \brief Message queue service using ERIKA
*
* Simple message queue handler
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
* \copyright Copyright ©2016
* Department of electrical engineering and information technology, Hochschule Darmstadt - University of applied sciences (h_da). All Rights Reserved.
* Permission to use, copy, modify, and distribute this software and its documentation for educational, and research purposes in the context of non-commercial
* (unless permitted by h_da) and official h_da projects, is hereby granted for enrolled students of h_da, provided that the above copyright notice,
* this paragraph and the following paragraph appear in all copies, modifications, and distributions.
* Contact Prof.Dr.-Ing. Peter Fromm, peter.fromm@h-da.de, Birkenweg 8 64295 Darmstadt - GERMANY for commercial requests.
*
* \warning This software is a PROTOTYPE version and is not designed or intended for use in production, especially not for safety-critical applications!
* The user represents and warrants that it will NOT use or redistribute the Software for such purposes.
* This prototype is for research purposes only. This software is provided "AS IS," without a warranty of any kind.
**/


 
#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include "global.h"
#include "project.h"
#include "eventqueue_cfg.h"
    
/**
* \brief Structure representing a event queue object
*
*/
typedef struct {
    EventMaskType     m_event;        /**< /brief event which will be fired in case of a write operation */
    TaskType          m_receiverTask; /**< /brief task receiving the event */
    uint16_t          m_bufferSize;
    uint16_t          m_bufferReadIndex;
    uint16_t          m_bufferWriteIndex;
    uint16_t          m_bufferFillLevel;
    EVQ_content_t*    m_Buffer;     /**< /brief Message payload, typically uint32_t, which will be casted to an enum */
}EVQ_t;



/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/

/**
 * Initialise a mesage object and allocate memory for the buffer
 * \param MESSAGE_t* const me       - [IN/OUT] message queue object
 * \param EventMaskType event       - [IN] event which will be fired in case of new message
 * \param TaskType task             - [IN] receiver task for event
 * \param uint16_t size             - [IN] size of buffer
 * @return RC_SUCCESS or error code
 */
RC_t EVQ_init(EVQ_t* const me,  EventMaskType event, TaskType task, uint16_t size);

/**
 * Initialise a mesage object and allocate memory for the buffer
 * \param EVQ_t* const me       - [IN/OUT] message queue object
 * \param EVQ_content_t data    - [IN] data to be written
 * @return RC_SUCCESS or error code
 */
RC_t EVQ_write(EVQ_t* const me,  EVQ_content_t data);

/**
 * Initialise a mesage object and allocate memory for the buffer
 * \param EVQ_t* const me           - [IN/OUT] message queue object
 * \param MESSAGE_content_t *const data - [OUT] data read
 * @return RC_SUCCESS or error code
 */
RC_t EVQ_read(EVQ_t* const me,  EVQ_content_t* const data);

/**
 * Initialise a mesage object and allocate memory for the buffer
 * \param EVQ_t* const me           - [IN/OUT] message queue object
 * @return TRUE if at least one more message is in the buffer
 */
boolean_t EVQ_available(EVQ_t* const me);


#endif /* MESSAGEQUEUE_H */
