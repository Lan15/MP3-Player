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

#include "timing.h"

#include "log.h"
#include "project.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

#define TA_CYCLE2NANO                       1000 / BCLK__BUS_CLK__MHZ       //Translate cycles into nanoseconds 

#define TA_SYSTICK_INTERRUPT_VECTOR_NUMBER  15u                             //Hard vector
#define TA_SYSTICK_MAXVAL                   0x00FFFFFF                      //Max allowed SysTick counter value is 24bit
#define TA_SYSTICK_1MS_RELOAD               (BCLK__BUS_CLK__KHZ - 1)        //Reload value for 1ms

/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/

/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

static volatile uint32_t TA_systick_count =0;

/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

/**
 * Counts a software counter
 */
static void TA_systick_isr();

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * The start function for the timing analyser hardware
 * Only called once to avoid problems with multiple timer objects
 **/
RC_t TA_init()
{
    
    //Init the DWT COunter
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; //Activate the trace unit
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            //Activate the cycle counter
    DWT->CYCCNT = 0;                                //Set the start value to 0
 
    //Init the Systick Counter
    CySysTickInit();
    CySysTickSetCallback(0, TA_systick_isr);           //Set ISR for Systick
    CySysTickSetReload(TA_SYSTICK_1MS_RELOAD);   //1ms cycle Time
    CySysTickEnable();
    
    return RC_SUCCESS;
}

/**
 * The config function for the timing analyser objects
 * \param TA_t *const me                : [OUT] The timing object
 * \param char const *const name        : [IN] The name of the timer object
 * \param TA_mode_t mode                : [IN] Timer measurement mode
 * \param TA_fpPinWrite_t pinWrite      : [IN] Pin write function
 * \return RC_SUCCESS or error code
 */
RC_t TA_config(TA_t *const me, char const *const name, TA_mode_t mode, TA_fpPinWrite_t pinWrite)
{
    
    me->m_elapsedTime = 0;
    me->m_mode = mode;
    me->m_name = name;
    me->m_state = TA_ISSTOPPED;
    me->p_pinWrite = pinWrite;
    

    //Start the pin toggle if active
    if (0 != me->p_pinWrite)
    {
        me->p_pinWrite(0);
    }    

    return RC_SUCCESS;
}

/**
 * The start function for the timing analyser, will start the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_start(TA_t *const me)
{
    if (me->m_state == TA_ISRUNNING)
    {
        return RC_ERROR_INVALID_STATE;
    }
    
    switch (me->m_mode)
    {
        case TA_DWT :
            me->m_startTime = DWT->CYCCNT ;   //Not required for the DWT
            me->m_elapsedTime = 0;
            break;
        case TA_SYSTICK : 
            me->m_startTime = TA_systick_count;
            me->m_elapsedTime = 0;
            break;
        
        case TA_NONE : 
            //Nothing to be done
            break;
        
        default: 
            return RC_ERROR_INVALID;
    }
    
    //Start the pin toggle if active
    if (0 != me->p_pinWrite)
    {
        me->p_pinWrite(1);
    }
    
    me->m_state = TA_ISRUNNING;
    
    return RC_SUCCESS;
}

/**
 * The stop function for the timing analyser, will stop the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_stop(TA_t *const me)
{
    
    if (me->m_state == TA_ISSTOPPED)
    {
        return RC_ERROR_INVALID_STATE;
    }
    
    switch (me->m_mode)
    {
        case TA_DWT :
            me->m_elapsedTime += (DWT->CYCCNT - me->m_startTime);
            break;
        
        case TA_SYSTICK : 
            me->m_elapsedTime += (TA_systick_count - me->m_startTime);
            break;
        
        case TA_NONE : 
            //Nothing to be done
            break;
        
        default: 
            return RC_ERROR_INVALID;
    }
    
    //Stop the pin toggle if active
    if (0 != me->p_pinWrite)
    {
        me->p_pinWrite(0);
    }
        
    
    me->m_state = TA_ISSTOPPED;
    
    return RC_SUCCESS;
    
}

/**
 * The pause function for the timing analyser, will pause the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_pause(TA_t *const me)
{
    if (me->m_state != TA_ISRUNNING)
    {
        return RC_ERROR_INVALID_STATE;
    }
    
    switch (me->m_mode)
    {
        case TA_DWT :
            me->m_elapsedTime += (DWT->CYCCNT - me->m_startTime);
            break;
        case TA_SYSTICK : 
            me->m_elapsedTime += (TA_systick_count - me->m_startTime);
            break;
        
        case TA_NONE : 
            //Nothing to be done
            break;
        
        default: 
            return RC_ERROR_INVALID;
    }
    
    //No action for the pin required
    me->m_state = TA_ISPAUSING;
    
    return RC_SUCCESS;
}

/**
 * The resume function for the timing analyser, will resume the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_resume(TA_t *const me)
{
    if (me->m_state != TA_ISPAUSING)
    {
        return RC_ERROR_INVALID_STATE;
    }
    
    switch (me->m_mode)
    {
        case TA_DWT :
            me->m_startTime = DWT->CYCCNT;
            break;
        case TA_SYSTICK : 
            me->m_startTime = TA_systick_count;
            break;
        
        case TA_NONE : 
            //Nothing to be done
            break;
        
        default: 
            return RC_ERROR_INVALID;
    }
    
    //No action for the pin required
    me->m_state = TA_ISRUNNING;
    
    return RC_SUCCESS;
}

/**
 * The print function for the timing analyser, will print the measurement values.
 * Will only work if the timer is not running
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_print(TA_t const * const me)
{
    if (me->m_state == TA_ISRUNNING)
    {
        return RC_ERROR_INVALID_STATE;
    }

    uint64_t nanoseconds = 0;
    uint64_t nanoseconds_whole = 0;
    uint64_t nanoseconds_fraction = 0;
    
    switch (me->m_mode)
    {
        case TA_DWT :
            nanoseconds = (uint64_t)me->m_elapsedTime * TA_CYCLE2NANO;
            nanoseconds_whole = nanoseconds / 1000;
            nanoseconds_fraction = nanoseconds % 1000;
            LOG_D("TA %s (DWT) -> %u.%u us (%u ns | %u cycles)", me->m_name, (uint32_t)(nanoseconds_whole), (uint32_t)(nanoseconds_fraction), (uint32_t)nanoseconds, me->m_elapsedTime);
            break;
        
        case TA_SYSTICK : 
            LOG_D("TA %s (SysTick) -> %u ms", me->m_name, me->m_elapsedTime);
            
            break;
        
        case TA_NONE : 
            //Nothing to be done
            break;
        
        default: 
            return RC_ERROR_INVALID;
    }

    
    return RC_SUCCESS;    
}


/**
 * Counts a software counter
 */
static void TA_systick_isr()
{
    TA_systick_count++;
}
