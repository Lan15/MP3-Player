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
*  - 6.11.18   : template, flash song played in an endless loop
*  - INC 3     : ISR tick counter for the rate check
*  - INC 5     : dual buffer, SOUND_FillBuffer(), SOUND_SwapBuffers()
*  - INC 6B    : underrun counter, swap in the ISR, 1024 byte buffers
*  - INC 11C   : Doxygen, MISRA C:2012 clean-up, faster SOUND_FillBuffer() (memcpy/memset),
*                fewer volatile accesses in the ISR, volatile pointers and volume,
*                unused SOUND_isrTickCount declaration removed, notes N1..N8 in sound.c
*  - INC 11C.1 : function comments in the project style (\param type name : [IN|OUT] text,
*                \return per value); RC_t return codes: SOUND_FillBuffer() RC_ERROR_NULL/_ZERO/_OVERRUN,
*                SOUND_SetVolume() RC_ERROR_RANGE, SOUND_isr() RC_ERROR_BUFFER_EMTPY on an underrun;
*                header prototypes with the same const qualifiers as the definitions; note N9
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
 * \brief Initialise the sound driver and the PWM / VDAC hardware (called once from tsk_init).
 *
 * Fills both buffers with silence (no click at the start), makes buffer A active and
 * buffer B inactive, clears the legacy flash fields (N6), selects input channel 1,
 * enables the amplifier, keeps the sample clock stopped and sets the PWM to the
 * silence level. The volume is not touched (default SOUND_VOLUME_MAX).
 *
 * Call once from tsk_init, before the sample timer ISR can run.
 *
 * \return RC_SUCCESS (always, nothing can fail, N9)
 */
RC_t SOUND_Init(void);
 
/**
 * \brief Start the sample clock; the timer ISR then calls SOUND_isr() every tick.
 * \return RC_SUCCESS (always, N9)
 */
RC_t SOUND_StartPlay(void);
 
/**
 * \brief Stop the sample clock; SOUND_isr() is no longer called, the PWM holds its last value.
 * \return RC_SUCCESS (always, N9)
 */
RC_t SOUND_StopPlay(void);
 
/**
 * \brief Set a song in flash (old flash player, no longer played, N6).
 *
 * Switches the dual buffer path off: the ISR outputs nothing until the next
 * SOUND_FillBuffer() call. The song is only stored.
 *
 * \param uint8_t const *const song : [IN] pointer to the song data (stored only, not dereferenced)
 * \param uint32_t const sizeOfSong : [IN] size of the song data in bytes (stored only)
 * \return RC_SUCCESS (always, N9)
 */
RC_t SOUND_SetSong(uint8_t const * const song, uint32_t const sizeOfSong);
 
/**
 * \brief Set the output volume.
 *
 * Values above SOUND_VOLUME_MAX are limited to SOUND_VOLUME_MAX and reported (N9).
 *
 * \param uint8_t const volume      : [IN] volume in percent, 0..SOUND_VOLUME_MAX
 * \return RC_SUCCESS when the volume was set as requested,
 *         RC_ERROR_RANGE when volume is above SOUND_VOLUME_MAX; the limited value SOUND_VOLUME_MAX is used
 */
RC_t SOUND_SetVolume(uint8_t const volume);
 
/**
 * \brief Copy PCM data into the inactive buffer and announce it to the ISR (N1, N2, N5).
 *
 * The rest of the buffer is padded with SOUND_SILENCE. m_inactiveFill is set last;
 * only then may the ISR swap. Switches the dual buffer path on if SOUND_SetSong()
 * had switched it off.
 *
 * Only call while the inactive buffer is empty (after ev_BufEmpty).
 *
 * \param uint8_t const *const data : [IN] PCM bytes (unsigned 8 bit), at least size bytes, must not overlap the driver buffers
 * \param uint16_t const size       : [IN] number of valid bytes, 1..SOUND_BUFFER_SIZE
 * \return RC_SUCCESS when the buffer was filled,
 *         RC_ERROR_NULL when data is NULL,
 *         RC_ERROR_ZERO when size is 0,
 *         RC_ERROR_OVERRUN when size is larger than SOUND_BUFFER_SIZE (nothing is changed in
 *         the three error cases)
 */
RC_t SOUND_FillBuffer(uint8_t const * const data, uint16_t const size);
 
/**
 * \brief Exchange active and inactive buffer (N1).
 *
 * The new active buffer gets the fill level of the former inactive one, the new
 * inactive buffer is marked empty and the read position restarts at 0.
 * Called from SOUND_isr() when the active buffer is finished and the inactive one
 * is ready, and once from tsk_reader before the timer is started (pre-fill of both
 * buffers, see mp3.c). Never call it from a task while the sample timer runs.
 *
 * \return RC_SUCCESS (always, N9)
 */
RC_t SOUND_SwapBuffers(void);
 
/**
 * \brief Return the number of underrun ticks since the last call and reset the counter (N7).
 *
 * An underrun is a tick where the active buffer was finished and no new one was ready
 * because the reader was late. Diagnostic only: a tick that happens between reading and
 * clearing the counter is lost.
 *
 * \return number of ISR ticks without a new sample, 0..4294967295 (a count, not a status code, N9)
 */
uint32_t SOUND__GetAndClearUnderruns(void);
 
/**
 * \brief Timer interrupt body: output one sample, swap buffers when needed (N1, N3, N4).
 *
 * Per tick exactly one of: output a sample (volume scaled), swap + SetEvent(tsk_reader,
 * ev_BufEmpty), or count an underrun. Does nothing while the dual buffer path is off.
 *
 * Call only from ISR2(isr_Timer).
 *
 * \return RC_SUCCESS when a sample was output, a swap was done or the dual buffer path is off,
 *         RC_ERROR_BUFFER_EMTPY when this tick was an underrun (reader late, last value held, N9)
 */
RC_t SOUND_isr(void);

/*****************************************************************************/
/* Private stuff, only visible for Together, declared static in cpp - File   */
/*****************************************************************************/


#ifdef TOGETHER


};
#endif /* Together */

#endif /* SOUND_H */