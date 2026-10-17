/**
* \file    sound.h
* \author  P. Fromm (template), V S Agilan (streaming)
* \date    6.11.18 (INC 11C: 2026-10)
*
* \brief   PWM sound driver with a dual buffer fed from the SD card
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
* \details
* The driver outputs one 8 bit unsigned sample per tick of the SOUND_SAMPLE
* timer (22050 Hz) to the PWM/VDAC8. The samples come from two buffers of
* #SOUND_BUFFER_SIZE bytes:
*
*   - the ACTIVE buffer is played by the timer interrupt (SOUND_isr),
*   - the INACTIVE buffer is filled by tsk_reader (SOUND_FillBuffer).
*
* Hand shake between reader and interrupt (details in N1 in sound.c):
*
*   1. The reader fills the inactive buffer, the last step sets its fill level > 0.
*   2. When the active buffer is played out and the inactive one is ready, the
*      interrupt swaps both buffers, sets ev_BufEmpty for tsk_reader and
*      raises the DBG_DEAD pin.
*   3. The reader refills the buffer that was just freed, and so on.
*   4. If the reader is late, the interrupt holds the last output value and
*      counts an underrun; it never swaps an empty buffer.
*
* Volume is applied per sample in the interrupt (sample * volume / 100).
*
* \todo <todos>
* \warning SOUND_isr() runs in interrupt context (ISR2 of Erika OS). SOUND_FillBuffer()
*          must only be called while the inactive buffer is empty, i.e. before the
*          first swap or after ev_BufEmpty (see mp3.c).
* \warning SOUND_SetSong() belongs to the old flash player. The stored song is not
*          played any more (N6 in sound.c).
* \warning This software is a PROTOTYPE version and is not designed or intended
*          for use in production, especially not for safety-critical applications!
*
* Changelog:\n
*  - 6.11.18 : template, flash song played in an endless loop
*  - INC 3   : ISR tick counter for the rate check
*  - INC 5   : dual buffer, SOUND_FillBuffer(), SOUND_SwapBuffers()
*  - INC 6B  : underrun counter, swap in the ISR, 1024 byte buffers
*  - INC 11C : Doxygen, MISRA C:2012 clean-up, faster SOUND_FillBuffer() (memcpy/memset),
*              fewer volatile accesses in the ISR, volatile pointers and volume,
*              unused SOUND_isrTickCount declaration removed, notes N1..N8 in sound.c
*
* \copyright Copyright 2016
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

#ifndef SOUND_H
#define SOUND_H

#include "global.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

//extern volatile uint32_t SOUND_isrTickCount;
    
#define SOUND_BUFFER_SIZE   (1024u)	/** \brief Size of each of the two buffers in bytes = samples per buffer. 1024 samples = 46.4 ms of audio at 22050 Hz (INC 6B.3: fewer, larger SD reads). */

#define SOUND_SILENCE       (0x80u)	/** \brief Mid-rail silence value for 8 bit unsigned PCM. 0x80 = 128 maps to 0 V on the VDAC8. */

#define SOUND_VOLUME_MAX    (100u)	/** \brief Highest volume in percent. */

/** \brief Input channels of the analog multiplexer in front of the amplifier. */
enum eSOUND_Channel
{
    SOUND_OFF = 0,      /**< All Channel off */
    SOUND_CH1 = 1,      /**< Input Channel 1 */
    SOUND_CH2 = 2,      /**< Input Channel 2 */
    SOUND_CH3 = 3,      /**< Input Channel 3 */
};
typedef enum eSOUND_Channel SOUND_Channel_t;

/* Wrapper to allow representing the file in Together as class */
#ifdef TOGETHER

class SOUND
{
public:
#endif /* Together */


/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/

/**
 * \brief Initialise the sound driver.
 *
 * Fills both buffers with silence (no click at the start), makes buffer A
 * active, selects input channel 1, enables the amplifier, stops the sample
 * timer and starts the PWM at silence level.
 *
 * \return RC_SUCCESS always
 * \note   Call once from tsk_init, before the sample timer ISR can run.
 */
RC_t SOUND_Init(void);

/**
 * \brief Start the sample timer: the ISR now outputs one sample per tick.
 * \return RC_SUCCESS always
 */
RC_t SOUND_StartPlay(void);

/**
 * \brief Stop the sample timer: the output holds its last value.
 * \return RC_SUCCESS always
 */
RC_t SOUND_StopPlay(void);

/**
 * \brief Set a song in flash (old flash player, no longer played, N6 in sound.c).
 *
 * Switches the dual buffer path off: the ISR outputs nothing until the next
 * SOUND_FillBuffer() call.
 *
 * \param   uint8_t const *const song       : pointer to the song data [IN]
 * \param   uint32_t sizeOfSong             : size of the song data [IN]
 * \return RC_SUCCESS always
 */
RC_t SOUND_SetSong(uint8_t const * const song, uint32_t sizeOfSong);

/**
 * \brief Set the volume.
 * \param   uint8_t volume                  : volume in percent, 0..#SOUND_VOLUME_MAX; larger values are limited to 100
 * \return RC_SUCCESS always
 */
RC_t SOUND_SetVolume(uint8_t volume);

/**
 * \brief Fill the inactive buffer with PCM data (tsk_reader).
 *
 * Copies \p size samples into the inactive buffer, pads the rest with
 * #SOUND_SILENCE and makes the buffer available for the next swap.
 * If the flash path was active (SOUND_SetSong), the dual buffer path is
 * switched on again with buffer A active.
 *
 * \param[in] data  PCM samples (unsigned 8 bit), must not overlap the driver buffers
 * \param[in] size  number of valid samples, 1..#SOUND_BUFFER_SIZE
 * \return RC_SUCCESS buffer filled
 * \return RC_ERROR   size is 0 or larger than #SOUND_BUFFER_SIZE, nothing was changed
 * \note   Only call while the inactive buffer is empty (after ev_BufEmpty).
 */
RC_t SOUND_FillBuffer(uint8_t const * const data, uint16_t size);

/**
 * \brief Swap active and inactive buffer.
 *
 * Called by the ISR when the active buffer is played out and by tsk_reader once
 * before the timer is started (pre-fill of both buffers, see mp3.c).
 *
 * \return RC_SUCCESS always
 * \note   Never call from a task while the sample timer runs.
 */
RC_t SOUND_SwapBuffers(void);

/**
 * \brief Return the number of underrun ticks since the last call and reset it (INC 6B.1).
 *
 * An underrun is a timer tick where no sample was ready because the reader was late.
 * Diagnostic only: a tick that happens between reading and clearing the counter is lost (N7).
 *
 * \return number of ISR ticks without a sample
 */
uint32_t SOUND__GetAndClearUnderruns(void);

/**
 * \brief Timer interrupt body: output the next sample, swap buffers when the active one is done.
 *
 * \return RC_SUCCESS always
 * \note   Call only from ISR2(isr_Timer).
 */
RC_t SOUND_isr(void);

/*****************************************************************************/
/* Private stuff, only visible for Together, declared static in cpp - File   */
/*****************************************************************************/


#ifdef TOGETHER


};
#endif /* Together */

#endif /* SOUND_H */