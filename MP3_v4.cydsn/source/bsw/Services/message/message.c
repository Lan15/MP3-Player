/**
* \file message.c
* \author P. Fromm
* \date 26.7.22
*
* \brief Simple messaging system supporting messages of variable length
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
#include "stdlib.h"

#include "message.h"
#include "log.h"
#include "ceh.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

//Heap symbols from linker to check correct allocation
extern void* __cy_heap_start;
extern void* __cy_heap_end;

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

/**
 * Will check if the allocated adress is in the heap area
 * Limitation: Size of the allocated memory is not checked
 */
RC_t MSG_checkAllocation(void* adr);

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * Initialises the ringbuffer and sets up the memory for the metadata
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param uint16_t const size           : [IN] Size of the ringbuffer
 * \param EventMaskType const ev        : [IN] Event which will be fired in case of a new message
 * \param TaskType const task           : [IN] Receiver task of the event
 * \param ResourceType const res        : [IN] Ressource to protect critical region
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */
RC_t MSG_init(MSG_messagebox_t *const me, uint16_t const size, EventMaskType const ev, TaskType const task, ResourceType const res)
{
    me->m_ev = ev;
    me->m_task = task;
    me->m_ressource = res;
    me->m_readCount = 0;
    me->m_writeCount = 0;
    me->m_readIdx = 0;
    me->m_writeIdx = 0;
    
    if (size > 1)
    {
        me->m_size = size;
    }
    else
    {
        me->m_size = 10;
    }
    
    me->m_content = (MSG_message_t*)malloc(me->m_size * sizeof(MSG_message_t));
    if (0 == me->m_content) return RC_ERROR_MEMORY;
    
    for (uint16 i = 0; i < me->m_size; i++) 
    {
        me->m_content[i].m_size = 0;
#ifdef MSG_STATIC_ALLOCATION        
        me->m_content[i].mp_memory = malloc(MSG_STATIC_ALLOCATION_SIZE);
        if (RC_SUCCESS != MSG_checkAllocation(me->m_content[i].mp_memory))
        {
            LOG_E("MSG Error allocating static memory");
        }
#else

        me->m_content[i].mp_memory = 0;
#endif    
    }
    
    return RC_SUCCESS;
}
  
#ifndef MSG_STATIC_ALLOCATION

/**
 * Send a message by creating a temporary transfer buffer, copying the data into this buffer and firing an event to the receiver task
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param void const* const pData       : [IN] Data to be transmitted
 * \param uint16_t const size           : [IN] Size of the transmitted data
 * \param MSG_id_t id                   : [IN] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */    
RC_t MSG_sendMessage(MSG_messagebox_t *const me, void const* const pData, uint16_t const size, MSG_id_t id)
{
    
    RC_t res = RC_SUCCESS;
    
#ifdef MSG_TIME_ANALYSIS
    MSG_startMeasurement();
#endif    
    
    if (MSG_getFillLevel(me) <  me->m_size)
    {
        
        void* pBuffer = 0;
        
        //Critical section starts
        
        if (me->m_ressource != 0) GetResource(me->m_ressource);
        MSG_DI;
        
        //Clear the old message buffer and reallocate memory unless we can reuse it (same size)
        if (size != me->m_content[me->m_writeIdx].m_size)
        {
            if (0 != me->m_content[me->m_writeIdx].m_size)
            {
                //Previous memory does not fit, let's clear it
                free(me->m_content[me->m_writeIdx].mp_memory);
                
                //And set it to size 0, as no payload may be required
                //Will happen later as well, but just to be on the safe side....
                me->m_content[me->m_writeIdx].mp_memory = 0;
                me->m_content[me->m_writeIdx].m_size = 0;
            }        

            //Allocate memory for transmission, if needed
            if (0 != pData && 0 != size)
            {
                pBuffer = malloc(size); 
                res = MSG_checkAllocation(pBuffer);
                if (RC_SUCCESS != res) 
                {
                    pBuffer = 0;
                }
            }
        }
        else
        {   
            //Existing buffer will do
            pBuffer = me->m_content[me->m_writeIdx].mp_memory;
        }

        //We now have a valid buffer, let's copy the data, at least if we have some
        if (0 != pData && 0 != size)
        {
            //Deep copy of the data into the allocated memory
            for (uint16_t i = 0; i < size; i++)
            {
                ((uint8*)pBuffer)[i] = ((uint8*)pData)[i]; 
            }
        } 
        
        //Copy the content into the message
        me->m_content[me->m_writeIdx].m_size = size;     
        me->m_content[me->m_writeIdx].m_msgId = id;     
        me->m_content[me->m_writeIdx].mp_memory = pBuffer;  

        me->m_writeCount++;  
        me->m_writeIdx++;                  
        me->m_writeIdx %= me->m_size;     
        
        MSG_EI;
        if (me->m_ressource != 0) ReleaseResource(me->m_ressource);
        
        SetEvent(me->m_task, me->m_ev);    
    }
    else
    {
        LOG_E("MSG MSG_sendMessage buffer full");
        CEH_report(&CEH, CEH_URTE_MESSAGE_BUFFERFULL, 0);
        
        res =  RC_ERROR_BUFFER_FULL;       
    }
    
    return res;
    
}




#else

/**
 * Send a message by creating a temporary transfer buffer, copying the data into this buffer and firing an event to the receiver task
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param void const* const pData       : [IN] Data to be transmitted
 * \param uint16_t const size           : [IN] Size of the transmitted data
 * \param MSG_id_t id                   : [IN] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */    
RC_t MSG_sendMessage(MSG_messagebox_t *const me, void const* const pData, uint16_t const size, MSG_id_t id)
{
    RC_t res = RC_SUCCESS;
    
#ifdef MSG_TIME_ANALYSIS
    MSG_startMeasurement();
#endif    
    
    //LOG_D("MSG","Sending id %d size %d", id.m_signal, size);

    if (MSG_getFillLevel(me) <  me->m_size)
    {
        
        if (me->m_ressource != 0) GetResource(me->m_ressource);
        MSG_DI;
        
        uint8_t usedSize = size;
        
        void* pBuffer = me->m_content[me->m_writeIdx].mp_memory;

        //We need to check if the static buffer is large enough
        if (size > MSG_STATIC_ALLOCATION_SIZE)
        {
            LOG_E("MSG MSG_sendMessage static buffer too small, truncating transmitted message");
            usedSize = MSG_STATIC_ALLOCATION_SIZE;
            res = RC_ERROR_BAD_DATA;
        }
        
        //Deep copy of the data into the allocated memory
        for (uint16_t i = 0; i < usedSize; i++)
        {
            ((uint8*)pBuffer)[i] = ((uint8*)pData)[i]; 
        }
        
        //COpy the content into the message
        me->m_content[me->m_writeIdx].m_size = usedSize;     
        me->m_content[me->m_writeIdx].m_msgId = id;     
        
        me->m_writeIdx++;                  
        me->m_writeIdx %= me->m_size;     
        me->m_writeCount++;                
        
        //MSG_dbgPrint(me);
        
        MSG_EI;
        if (me->m_ressource != 0) ReleaseResource(me->m_ressource);
        
        SetEvent(me->m_task, me->m_ev);    
        
              
    }
    else
    {
        LOG_E("MSG MSG_sendMessage buffer full");
        CEH_report(CEH_MESSAGE_BUFFERFULL, 0);
        res = RC_ERROR_BUFFER_FULL;       
    }
    
    return res;   
}
 
   
#endif    


/**
 * Reads the message from the transfer buffer and releases the allocated memory
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param void *const pData             : [OUT] Pointer to the memory for storing the received message
 * \param uint16_t const size           : [IN] Size of the transmitted data. Will be compared with the size of the received message to avoid buffer overrun
 * \param MSG_id_t *const id            : [OUT] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_receiveMessage(MSG_messagebox_t *const me, void* pData, uint16_t const size, MSG_id_t *const id)
{
    
#ifdef MSG_TIME_ANALYSIS
    MSG_stopMeasurement();
#endif    

    if (MSG_getFillLevel(me) > 0)
    {
        //Get the content record from the postbox
        uint16_t mSize = me->m_content[me->m_readIdx].m_size;
        uint8_t* pBuffer = me->m_content[me->m_readIdx].mp_memory;
        *id = me->m_content[me->m_readIdx].m_msgId;
        //LOG_D("MB_recMessage","size tmd = %d, size stored = %d, buffer = %x", size, mSize, pBuffer);

        if (mSize != size) 
        {
            LOG_E("MSG MSG_receiveMessage bad size");
            return RC_ERROR_BAD_PARAM;
        }

        if (me->m_ressource != 0) GetResource(me->m_ressource);
        MSG_DI;
        
        //Deep copy of the data into the allocated memory
        for (uint16_t i = 0; i < size; i++)
        {
            ((uint8*)pData)[i] = ((uint8*)pBuffer)[i];
        }

        //LOG_D("MB_recMessage","memory = %x, size = %d, data = %x, copied data %x",(uint8*)pBuffer, size, *(uint32_t*)pBuffer, *(uint32_t*)pData);
        
        //Release memory after transmission
        //free(me->m_content[me->m_readIdx].mp_memory);
        //me->m_content[me->m_readIdx].mp_memory = 0;
        
        me->m_readIdx++;
        me->m_readIdx %= me->m_size;
        me->m_readCount++;
        
        MSG_EI;
        if (me->m_ressource != 0) ReleaseResource(me->m_ressource);

        return RC_SUCCESS;
    }
    else
    {
        LOG_I("MSG MSG_receiveMessage buffer empty");
        return RC_ERROR_BUFFER_EMTPY;
    }
}
/**
 * Returns the size of the next message in the buffer
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param uint16_t *const size          : [OUT] Size of the next message, 0 in case of no message
 * \param MSG_id_t *const id            : [OUT] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_getSizeAndIdOfNextMessage(MSG_messagebox_t *const me, uint16_t *const size, MSG_id_t *const id)
{
    if (MSG_getFillLevel(me) > 0)
    {
        
        *size = me->m_content[me->m_readIdx].m_size;
        *id = me->m_content[me->m_readIdx].m_msgId;
        //LOG_D("MB_getSize","readIdx = %d, size = %d", me->m_readIdx, *size);
        
        return RC_SUCCESS;
    }
    else
    {
        LOG_I("MSG MSG_getSizeAndIdOfNextMessage empty buffer");
        *size = 0;
        return RC_ERROR_BUFFER_EMTPY;
    }
}

/**
 * Wait for the next message (using OS services)
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_waitNextMessage(MSG_messagebox_t *const me)
{
    WaitEvent(me->m_ev);
    ClearEvent(me->m_ev);
    
    return RC_SUCCESS;
}



/**
 * Debug print of the message buffer
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 */
RC_t MSG_dbgPrint(MSG_messagebox_t *const me)
{
    LOG_D("MSG Size = %d, Filllevel = %d, ReadIndex = %d, WriteIndex = %d", me->m_size, MSG_getFillLevel(me), me->m_readIdx, me->m_writeIdx);
    
    for (uint16_t i = 0; i < me->m_size; i++)
    {
        LOG_D("   CONT [%d] Adress = %x, Size = %d",i, me->m_content[i].mp_memory, me->m_content[i].m_size);
    }
    
    return RC_SUCCESS;
}

/**
 * Provides references to the metadata and payload. Better interface for RTE
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \param void** ppData                 : [OUT] Pointer to the memory for storing the received message
 * \param uint16_t *const psize         : [OUT] Size of the transmitted data. 
 * \param MSG_id_t *const id            : [OUT] Identifier of the message
 * \return RC_t: RC_SUCCESS in case of no error else error code
 */ 
RC_t MSG_receiveMessageRef(MSG_messagebox_t *const me, void** ppData, uint16_t *const psize, MSG_id_t *const id)
{
    
#ifdef MSG_TIME_ANALYSIS
    MSG_stopMeasurement();
#endif      
    
    if (MSG_getFillLevel(me) > 0)
    {
        if (me->m_ressource != 0) GetResource(me->m_ressource);
        MSG_DI;
                
        //Get the content record from the postbox
        *psize = me->m_content[me->m_readIdx].m_size;
        *ppData = me->m_content[me->m_readIdx].mp_memory;
        *id = me->m_content[me->m_readIdx].m_msgId;
        
        if (0 != *ppData && RC_SUCCESS != MSG_checkAllocation(*ppData) )
        {
            //This should not happen, as the allocation is protected on the sending side
            //0 is transmitted in case no payload, therefore filtered out
            LOG_E("MSG MSG_receiveMessageRef got wrong adress %x", (uint32_t)*ppData);
        }        
        
        me->m_readIdx++;
        me->m_readIdx %= me->m_size;
        me->m_readCount++;
        
        MSG_EI;
        if (me->m_ressource != 0) ReleaseResource(me->m_ressource);
        
        return RC_SUCCESS;
    }
    else
    {
        LOG_I("MSG MSG_receiveMessageRef buffer empty");
        return RC_ERROR_BUFFER_EMTPY;
    }
    
}

/**
 * Wait for the next message (using OS services)
 * \param MSG_messagebox_t *const me    : [IN/OUT] Message Box Object
 * \return uint16_t: Number of messaages contained in the buffer
 */ 
uint16_t MSG_getFillLevel(MSG_messagebox_t *const me)
{
    return (uint16_t)(me->m_writeCount - me->m_readCount);
}


/**
 * Will check if the allocated adress is in the heap area
 * Limitation: Size of the allocated memory is not checked
 */
RC_t MSG_checkAllocation(void* adr)
{
    if (adr < (void*)(&__cy_heap_start) || adr > (void*)(&__cy_heap_end))
    {
        LOG_E("MSG Memory allocation/adress out of of heap area: %x (%x - %x)", (unsigned int)adr, (unsigned int)&__cy_heap_start, (unsigned int)&__cy_heap_end);
        CEH_report(CEH_MESSAGE_MALLOC, (uint32_t)adr);
        return RC_ERROR_MEMORY;
    }
    
    return RC_SUCCESS;
}