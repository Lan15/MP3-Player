/**
* \file message.h
* \author Peter Fromm
* \date 26.7.22
*
* \brief Simple messaging mechanism for ERIKA
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
* \copyright Copyright ©2022
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


 
#ifndef MSG_H
#define MSG_H

#include "global.h"
#include "project.h"
#include "message_cfg.h"
/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/


//####################### Structures
/**
* \brief The purpose as well as the members of a structure have to be documented.
*
* Make clear what the structure is used for and what is the purpose of the members.
*/


typedef struct
{
    void*       mp_memory;  //! Pointer to the allocated memory
    uint16_t    m_size;     //! size of the allocated memory
    MSG_OPTIONAL_METADATA;  //! optional application specific payload for the metadata
} MSG_message_t;



typedef struct
{
    MSG_message_t*  m_content;                 //!< Ringbuffer for the transmitted elements
    uint16_t        m_size;                     //!< Size of the ringbuffer
    uint16_t        m_writeIdx;                 //!< Index to the next position to be written to
    uint16_t        m_readIdx;                  //!< Index to the next position to be read from
    uint32_t        m_writeCount;               //!< Number of elements written
    uint32_t        m_readCount;                //!< Number of elements read
    EventMaskType   m_ev;                       //!< Event to be fired and received upon update
    TaskType        m_task;                     //!< Receivertask
    ResourceType    m_ressource;                //!< Resource
} MSG_messagebox_t;



/*****************************************************************************/
/* External API               */
/*****************************************************************************/


/**
 * Initialises the ringbuffer and sets up the memory for the metadata
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param uint16_t const size           : [IN] Size of the ringbuffer
 * \param EventMaskType const ev        : [IN] Event which will be fired in case of a new message
 * \param TaskType const task           : [IN] Receiver task of the event
 * \param ResourceType const res        : [IN] Ressource to protect critical region. Use 0 in case no ressources is needed
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */
RC_t MSG_init(MSG_messagebox_t *const me, uint16_t const size, EventMaskType const ev, TaskType const task, ResourceType const res);


/**
 * Send a message by creating a temporary transfer buffer, copying the data into this buffer and firing an event to the receiver task
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param void const* const pData       : [IN] Data to be transmitted
 * \param uint16_t const size           : [IN] Size of the transmitted data
 * \param MSG_id_t id                   : [IN] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */    
RC_t MSG_sendMessage(MSG_messagebox_t *const me, void const* const pData, uint16_t const size, MSG_id_t id);


/**
 * Returns the size of the next message in the buffer
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param uint16_t *const size          : [OUT] Size of the next message, 0 in case of no message
 * \param MSG_id_t *const id            : [OUT] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_getSizeAndIdOfNextMessage(MSG_messagebox_t *const me, uint16_t *const size, MSG_id_t *const id);


/**
 * \Obsolete
 * Wait for the next message (using OS services)
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_waitNextMessage(MSG_messagebox_t *const me);


/**
 * Reads the message from the transfer buffer and releases the allocated memory
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param void *const pData             : [IN/OUT] Pointer to the memory for storing the received message
 * \param uint16_t const size           : [IN] Size of the transmitted data. Will be compared with the size of the received message to avoid buffer overrun
 * \param MSG_id_t *const id            : [OUT] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_receiveMessage(MSG_messagebox_t *const me, void* pData, uint16_t const size, MSG_id_t *const id);

/**
 * Provides references to the metadata and payload. Better interface for RTE
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param void** ppData                 : [OUT] Pointer to the memory for storing the received message
 * \param uint16_t *const psize         : [OUT] Size of the transmitted data. 
 * \param MSG_id_t *const id            : [OUT] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_receiveMessageRef(MSG_messagebox_t *const me, void** ppData, uint16_t *const psize, MSG_id_t *const id);

/**
 * Debug print of the message buffer
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 */
RC_t MSG_dbgPrint(MSG_messagebox_t *const me);

/**
 * Wait for the next message (using OS services)
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \return uint16_t: Number of messaages contained in the buffer
 */ 
uint16_t MSG_getFillLevel(MSG_messagebox_t *const me);

#endif /* MSG_H */
