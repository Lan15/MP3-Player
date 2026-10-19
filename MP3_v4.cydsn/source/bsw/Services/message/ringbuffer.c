/**
* \file ringbuffer
* \author P. Fromm
* \date 11/2016
*
* \brief ringbuffer
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

#include <stdlib.h>

#include "ringbuffer.h"

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
 * Init function for ringbuffer. Use pBuffer paramater to allocate memory directly or via heap.
 * \param ringbuffer_t* const me    : [OUT] pointer to the ringbuffer object
 * \param RB_size_t size            : [IN]  size of the ringbuffer in byte. 
 * \param RB_data_t* pBuffer        : [IN]  pointer to the buffer. If set to 0, malloc will be used to allocate memory
 * \return RC_ERRORMEMORY in case memory allocation failed 
 */
RC_t RB_init(RB_ringbuffer_t* const me, RB_size_t size, RB_data_t* pBuffer)
{
    me->m_size = size;

    if (pBuffer != 0)
    {
        //Some buffer is provided from the outside
        //It is the user's responsibility to ensure that size and pBuffer parameter are in sync
        me->m_bufOnHeap = FALSE;
        me->m_pBuffer = pBuffer;
    }
    else
    {
        //No buffer is provided, we use the heap
        me->m_bufOnHeap = TRUE;
        me->m_pBuffer = (RB_data_t*)malloc(me->m_size * sizeof(RB_data_t));
        
        if (0 == me->m_pBuffer) return RC_ERROR_MEMORY;
    }
    
    return RB_clear(me);
}

/**
 * Write function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [IN/OUT] pointer to the ringbuffer object
 * \param RB_data_t const *const data   : [IN]     element which will be stored
 * \return RC_ERROR_BUFFER_FULL in case no empty space is available anymore
 */
RC_t RB_put(RB_ringbuffer_t* const me, RB_data_t const *const data)
{
    
    RC_t ret = RC_SUCCESS;
    
    RB_enterCriticalRegion();
    
    RB_size_t fillLevel = 0;
    if ( (RC_SUCCESS == RB_getFillLevel(me, &fillLevel)) && (fillLevel < me->m_size) )
    {
        me->m_pBuffer[me->m_writeIndex++] = *data;
        me->m_writeIndex %= me->m_size;
        me->m_countPut++;
    }
    else
    {
        ret =  RC_ERROR_BUFFER_FULL;
    }
    
    RB_leaveCriticalRegion();
    
    return ret;
}

/**
 * Read function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [IN/OUT] pointer to the ringbuffer object
 * \param RB_data_t* data               : [OUT]    pointer to the element read from the buffer
 * \return RC_ERROR_BUFFER_EMTPY if buffer was empty
 */
RC_t RB_get(RB_ringbuffer_t* const me, RB_data_t* const data)
{
    RC_t ret = RC_SUCCESS;
    
    RB_enterCriticalRegion();
    
    RB_size_t fillLevel = 0;
    if ( (RC_SUCCESS == RB_getFillLevel(me, &fillLevel)) && (fillLevel > 0) )
    {
        *data = me->m_pBuffer[me->m_readIndex++];
        me->m_readIndex %= me->m_size;
        me->m_countGet++;
        
    }
    else
    {
        ret =  RC_ERROR_BUFFER_EMTPY;
    }
    
    RB_leaveCriticalRegion();
    
    return ret;    
    
}

/**
 * Read function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [OUT] pointer to the ringbuffer object
 * \return RC_SUCCESS
 */
RC_t RB_clear(RB_ringbuffer_t* const me)
{
    RB_enterCriticalRegion();
    
    me->m_readIndex = 0;
    me->m_writeIndex = 0;
    me->m_countGet = 0;
    me->m_countPut = 0;
    
    RB_leaveCriticalRegion();
    
    return RC_SUCCESS;
}

/**
 * DeInit function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [IN] pointer to the ringbuffer object
 * \return RC_SUCCESS
 */
RC_t RB_deinit(RB_ringbuffer_t* const me)
{
    if (me->m_bufOnHeap ==TRUE)
    {
        free(me->m_pBuffer);
    }
    
    return RC_SUCCESS;
}



/**
 * Get number of stored elements
 * \param RB_ringbuffer_t* const me     : [IN/OUT} pointer to the ringbuffer object
 * \param RB_size_t* pFillLevel         : [OUT]    pointer to fillLevel value
 * \return RC_ERROR_INVALID_STATE if countGet > countPut
 */
RC_t RB_getFillLevel(RB_ringbuffer_t* const me, RB_size_t* pFillLevel)
{
    RC_t ret = RC_SUCCESS;
    
    
    if (me->m_countPut >= me->m_countGet)
    {
        *pFillLevel = (RB_size_t)(me->m_countPut - me->m_countGet);
    }
    else
    {
        ret = RC_ERROR_INVALID_STATE;
    }
    
    
    return ret;
}
