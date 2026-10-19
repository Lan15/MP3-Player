/**
* \file ringbuffer.h
* \author Prof. P. Fromm
* \date 11/2016
*
* \brief ringbuffer for uart ports and similar
*
* Provides a uint8 based ringbuffer of variable size
*
* \note none
* \todo none
* \warning none
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
*/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "global.h"
#include "project.h"

#include "ringbuffer_cfg.h"   
    
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define')                           */
/*****************************************************************************/


/*****************************************************************************/
/* Public stuff                                                              */
/*****************************************************************************/
//Types



typedef struct{
    RB_size_t m_size;           //!< Size of the buffer
    RB_size_t m_readIndex;      //!< Read Index, next element will be read from this position
    RB_size_t m_writeIndex;     //!< Write Index, next element will be written to this position
    uint32_t  m_countPut;       //!< Number of elements added. We use 2 seperate attributes for a bit more threadsafety between read/write operations
    uint32_t  m_countGet;       //!< Number of elements read out.
    RB_data_t* m_pBuffer;       //!< Pointer to the buffer
    boolean_t m_bufOnHeap;      //!< Flag to memorize of buffer was allovated on heap. In this case heap will be cleared upon deinit
} RB_ringbuffer_t;

//Operations

/**
 * Init function for ringbuffer. Use pBuffer paramater to allocate memory directly or via heap.
 * \param ringbuffer_t* const me    : [OUT] pointer to the ringbuffer object
 * \param RB_size_t size            : [IN]  size of the ringbuffer in byte. 
 * \param RB_data_t* pBuffer        : [IN]  pointer to the buffer. If set to 0, malloc will be used to allocate memory
 * \return RC_ERRORMEMORY in case memory allocation failed 
 */
RC_t RB_init(RB_ringbuffer_t* const me, RB_size_t size, RB_data_t* pBuffer);

/**
 * Write function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [IN/OUT] pointer to the ringbuffer object
 * \param RB_data_t const *const data   : [IN]     element which will be stored
 * \return RC_ERROR_BUFFER_FULL in case no empty space is available anymore
 */
RC_t RB_put(RB_ringbuffer_t* const me, RB_data_t const *const data);

/**
 * Read function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [IN/OUT] pointer to the ringbuffer object
 * \param RB_data_t* data               : [OUT]    pointer to the element read from the buffer
 * \return RC_ERROR_BUFFER_EMTPY if buffer was empty
 */
RC_t RB_get(RB_ringbuffer_t* const me, RB_data_t* const data);

/**
 * Clear function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [OUT] pointer to the ringbuffer object
 * \return RC_SUCCESS
 */
RC_t RB_clear(RB_ringbuffer_t* const me);

/**
 * DeInit function for ringbuffer
 * \param RB_ringbuffer_t* const me     : [IN] pointer to the ringbuffer object
 * \return RC_SUCCESS
 */
RC_t RB_deinit(RB_ringbuffer_t* const me);

/**
 * Get number of stored elements
 * \param RB_ringbuffer_t* const me     : [IN/OUT} pointer to the ringbuffer object
 * \param RB_size_t* pFillLevel         : [OUT]    pointer to fillLevel value
 * \return RC_ERROR_INVALID_STATE if countGet > countPut
 */
RC_t RB_getFillLevel(RB_ringbuffer_t* const me, RB_size_t* pFillLevel);

#endif /* RINGBUFFER_H */
