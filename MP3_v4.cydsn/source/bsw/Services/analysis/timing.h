/**
* \file     timing.h
* \author   P. Fromm
* \date     18.11.24
*
* \brief timing analysis module providing different timing functions
*
* detailed description what the file does
*
* \note <notes>
* Programming rules (may be deleted in the final release of the file)
* ===================================================================
*
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


 
#ifndef TIMINGANALYSER_H
#define TIMINGANALYSER_H

#include "global.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

//####################### Defines/Macros

/**
 * \brief typedef for function pointer write method
 */   
typedef void (*TA_fpPinWrite_t)(uint8_t val);


//####################### Enumerations

/**
* \brief Enumerations for the timing analyser state.
*
*/
 typedef enum {
    TA_ISSTOPPED,         //!< Timer is created but stopped
    TA_ISRUNNING,         //!< Timer is created and running
    TA_ISPAUSING,         //!< Timer is created and pausing
} TA_state_t;

/**
* \brief Enumerations for the timing analyser mode.
*
*/
 typedef enum {
    TA_NONE,            //!< Only pin mode
    TA_DWT,             //!< Timer is created but stopped
    TA_SYSTICK,         //!< Timer is created and pausing
} TA_mode_t;

//####################### Structures
/**
* \brief This is the class structire of a timing analyser
*
*/
typedef struct {
    char const *    m_name;             //!< The name of the timer object
    uint32_t        m_startTime;        //!< The value of the counter when starting
    uint32_t        m_elapsedTime;      //!< The elapsed time, relevant for the pausing state
    TA_state_t      m_state;            //!< The current state of the timer, set by the API functions
    TA_mode_t       m_mode;             //!< The measurement mode
    TA_fpPinWrite_t p_pinWrite;         //!< Function pointer to the pin write function (or Null)
} TA_t;


/**
 * The start function for the timing analyser hardware
 * Only called once to avoid problems with multiple timer objects
 *
 * Cycle Counter:
 * Overrun and resolution depends on bus frequency. 
 *                  24 MHz (default)        72 MHz (typical)
 * Resolution       0.0416 us               0.0138 us     
 * Overrun          178s                    59s
 *
 * Systick Counter:
 * Resolution       1000 us                
 * Overrun          4.294.967s (49d)
 *
 **/
RC_t TA_init();

/**
 * The config function for the timing analyser objects
 *
 * \param TA_t *const me                : [OUT] The timing object
 * \param char const *const name        : [IN] The name of the timer object
 * \param TA_mode_t mode                : [IN] Timer measurement mode
 * \param TA_fpPinWrite_t pinWrite      : [IN] Pin write function
 * \return RC_SUCCESS or error code
 */
RC_t TA_config(TA_t *const me, char const *const name, TA_mode_t mode, TA_fpPinWrite_t pinWrite);

/**
 * The start function for the timing analyser, will start the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_start(TA_t *const me);

/**
 * The stop function for the timing analyser, will stop the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_stop(TA_t *const me);

/**
 * The pause function for the timing analyser, will pause the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_pause(TA_t *const me);

/**
 * The resume function for the timing analyser, will resume the measurement
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_resume(TA_t *const me);

/**
 * The print function for the timing analyser, will print the measurement values.
 * Will only work if the timer is not running
 * \param TA_t *const me                : [IN/OUT] The timing object
 */
RC_t TA_print(TA_t const * const me);

#endif /* TIMINGANALYSER_H */
