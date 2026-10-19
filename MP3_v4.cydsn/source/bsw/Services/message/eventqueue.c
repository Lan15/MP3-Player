/**
* \file <filename>
* \author <author-name>
* \date <date>
*
* \brief <Symbolic File name>
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
*/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include "eventqueue.h"
#include "stdlib.h"
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
 * Initialise a mesage object and allocate memory for the buffer
 * \param EVQ_t* const me       - [IN/OUT] EVQ queue object
 * \param EventMaskType event       - [IN] event which will be fired in case of new EVQ
 * \param TaskType task             - [IN] receiver task for event
 * \param uint16_t size             - [IN] size of buffer
 * @return RC_SUCCESS or error code
 */
RC_t EVQ_init(EVQ_t* const me,  EventMaskType event, TaskType task, uint16_t size)
{
    //Communication
    me->m_event = event;
    me->m_receiverTask = task;
    
    //Buffer
    me->m_bufferSize = size;
    me->m_bufferFillLevel = 0;
    me->m_bufferReadIndex = 0;
    me->m_bufferWriteIndex = 0;
    
    me->m_Buffer = (EVQ_content_t*)malloc(me->m_bufferSize * sizeof(EVQ_content_t));
    if (0 == me->m_Buffer) return RC_ERROR_MEMORY;
    
    for (uint16_t i = 0; i < me->m_bufferSize; i++)
    {
        me->m_Buffer[i] = 0;
    }
    
    return RC_SUCCESS;
    
}

/**
 * Initialise a mesage object and allocate memory for the buffer
 * \param EVQ_t* const me       - [IN/OUT] EVQ queue object
 * \param EVQ_content_t data    - [IN] data to be written
 * @return RC_SUCCESS or error code
 */
RC_t EVQ_write(EVQ_t* const me,  EVQ_content_t data)
{
    if (me->m_bufferFillLevel < me->m_bufferSize)
    {
        me->m_Buffer[me->m_bufferWriteIndex++] = data;
        me->m_bufferWriteIndex %= me->m_bufferSize;
        me->m_bufferFillLevel++;
        
        SetEvent(me->m_receiverTask, me->m_event);
        
        return RC_SUCCESS;
    }
    else
    {
        return RC_ERROR_BUFFER_FULL;
    }
    
}

/**
 * Initialise a mesage object and allocate memory for the buffer
 * \param EVQ_t* const me           - [IN/OUT] EVQ queue object
 * \param EVQ_content_t *const data - [OUT] data read
 * @return RC_SUCCESS or error code
 */
RC_t EVQ_read(EVQ_t* const me,  EVQ_content_t* const data)
{
    if (me->m_bufferFillLevel > 0)
    {
        *data = me->m_Buffer[me->m_bufferReadIndex++];
        me->m_bufferReadIndex %= me->m_bufferSize;
        me->m_bufferFillLevel--;
        
        return RC_SUCCESS;
    }
    else
    {
        return RC_ERROR_BUFFER_EMTPY;
    }
}

/**
 * Initialise a mesage object and allocate memory for the buffer
 * \param EVQ_t* const me           - [IN/OUT] EVQ queue object
 * @return TRUE if at least one more EVQ is in the buffer
 */
boolean_t EVQ_available(EVQ_t* const me)
{
    if (me->m_bufferFillLevel > 0) return TRUE; else return FALSE;   
}