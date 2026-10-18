/**
* \file    sound.c
* \author  P. Fromm (template), V S Agilan (streaming)
* \date    6.11.18 (INC 11C: 2026-10)
*
* \brief   PWM sound driver with a dual buffer fed from the SD card
*
* \details
* See sound.h for the overview. Two contexts use this file:
*
*   - tsk_reader : SOUND_FillBuffer() writes the inactive buffer, SOUND_SwapBuffers() once before the start
*   - ISR2(isr_Timer) : SOUND_isr() plays the active buffer, swaps when it is done
*
* Design notes N1..N9 are collected at the end of this file.
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
*/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include "sound.h"
#include "project.h"
#include <string.h>         /* memcpy(), memset() */

#include "DBG_DEAD.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/

/**
 * \brief State of the driver, shared between tsk_reader and the timer ISR (N1, N2).
 *
 * The pointers are volatile themselves ("uint8_t * volatile"), the buffer bytes
 * are plain data: a buffer is only touched by one side at a time (N2).
 */
typedef struct
{
    uint8_t const *    m_song;          /**< Legacy flash path: song data, stored only (N6)              */
    uint32_t           m_size;          /**< Legacy flash path: size, stored only (N6)                   */
    uint32_t           m_pos;           /**< Legacy flash path: position, stored only (N6)               */
    uint8_t * volatile m_activeBuf;     /**< buffer played by the ISR, NULL = dual buffer path off       */
    uint8_t * volatile m_inactiveBuf;   /**< buffer filled by the reader                                 */
    volatile uint16_t  m_activeFill;    /**< valid samples in the active buffer                          */
    volatile uint16_t  m_inactiveFill;  /**< valid samples in the inactive buffer, 0 = empty / not ready */
    volatile uint16_t  m_readPos;       /**< next sample of the active buffer that the ISR outputs       */
    volatile uint8_t   m_volume;        /**< volume in percent 0..SOUND_VOLUME_MAX                       */
} SOUND__Driver_t;

/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

static uint8_t SOUND__bufA[SOUND_BUFFER_SIZE];          /**< \brief sample buffer A, ISR plays it or the reader fills it (N1) */
static uint8_t SOUND__bufB[SOUND_BUFFER_SIZE];          /**< \brief sample buffer B, ISR plays it or the reader fills it (N1) */

static SOUND__Driver_t SOUND__currentSong = {NULL, 0u, 0u, NULL, NULL, 0u, 0u, 0u, SOUND_VOLUME_MAX};	/**< \brief Driver state, volume starts at 100 %. */

static volatile uint32_t SOUND__underruns = 0u;	        /**< \brief ticks where no sample was ready (N7). */

/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

static uint8_t * SOUND__GetInactiveBuf(void);                   /**< \brief buffer that the ISR is NOT playing (N1) */
static void      SOUND__CompilerBarrier(void);                  /**< \brief stop the compiler from reordering memory accesses (N2) */

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
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
RC_t SOUND_Init(void)
{
    /* Pre-fill both buffers with silence - no click on start */
    (void)memset(SOUND__bufA, (int)SOUND_SILENCE, SOUND_BUFFER_SIZE);
    (void)memset(SOUND__bufB, (int)SOUND_SILENCE, SOUND_BUFFER_SIZE);
 
    /* Dual buffer: A active, B inactive */
    SOUND__currentSong.m_activeBuf    = SOUND__bufA;
    SOUND__currentSong.m_inactiveBuf  = SOUND__bufB;
    SOUND__currentSong.m_activeFill   = SOUND_BUFFER_SIZE;
    SOUND__currentSong.m_inactiveFill = 0u;
    SOUND__currentSong.m_readPos      = 0u;
 
    /* Flash path: cleared */
    SOUND__currentSong.m_song = NULL;
    SOUND__currentSong.m_size = 0u;
    SOUND__currentSong.m_pos  = 0u;
 
    /* Hardware init */
    SOUND_Control_Write(SOUND_CH1);
    SOUND_ENABLE_Write(1u);
    (void)SOUND_StopPlay();
    SOUND_PWM_CLOCK_Start();
    SOUND_PWM_Start();
    SOUND_PWM_WriteCompare(SOUND_SILENCE);
 
    return RC_SUCCESS;
}
 
/**
 * \brief Start the sample clock; the timer ISR then calls SOUND_isr() every tick.
 * \return RC_SUCCESS (always, N9)
 */
RC_t SOUND_StartPlay(void)
{
    SOUND_SAMPLE_CLOCK_Start();
 
    return RC_SUCCESS;
}
 
/**
 * \brief Stop the sample clock; SOUND_isr() is no longer called, the PWM holds its last value.
 * \return RC_SUCCESS (always, N9)
 */
RC_t SOUND_StopPlay(void)
{
    SOUND_SAMPLE_CLOCK_Stop();      /* isr_Timer_Disable() does not work, the clock is stopped instead */
 
    return RC_SUCCESS;
}
 
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
RC_t SOUND_SetSong(uint8_t const * const song, uint32_t const sizeOfSong)
{
    /* Disable dual buffer path */
    SOUND__currentSong.m_activeBuf = NULL;
 
    /* Set flash path (stored only, N6) */
    SOUND__currentSong.m_song = song;
    SOUND__currentSong.m_size = sizeOfSong;
    SOUND__currentSong.m_pos  = 0u;
 
    return RC_SUCCESS;
}
 
/**
 * \brief Set the output volume.
 *
 * Values above SOUND_VOLUME_MAX are limited to SOUND_VOLUME_MAX and reported (N9).
 * 
 * \param uint8_t const volume      : [IN] volume in percent, 0..SOUND_VOLUME_MAX
 * \return RC_SUCCESS when the volume was set as requested,
 *         RC_ERROR_RANGE when volume is above SOUND_VOLUME_MAX; the limited value SOUND_VOLUME_MAX is used
 */
RC_t SOUND_SetVolume(uint8_t const volume)
{
    RC_t    rc  = RC_SUCCESS;
    uint8_t vol = volume;
 
    if (vol > SOUND_VOLUME_MAX)
    {
        vol = (uint8_t)SOUND_VOLUME_MAX;
        rc  = RC_ERROR_RANGE;                   /* N9: the value is limited, the limited value is used */
    }
 
    SOUND__currentSong.m_volume = vol;
 
    return rc;
}
 
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
RC_t SOUND_FillBuffer(uint8_t const * const data, uint16_t const size)
{
    RC_t rc = RC_SUCCESS;
 
    if (data == NULL)
    {
        rc = RC_ERROR_NULL;                             /* N9: nothing to copy (before: memcpy from NULL) */
    }
    else if (size == 0u)
    {
        rc = RC_ERROR_ZERO;                             /* N9 */
    }
    else if (size > SOUND_BUFFER_SIZE)
    {
        rc = RC_ERROR_OVERRUN;                          /* N9 */
    }
    else
    {
        uint8_t * dst;
 
        /* Switch to dual buffer mode if not already */
        if (SOUND__currentSong.m_activeBuf == NULL)
        {
            SOUND__currentSong.m_activeBuf   = SOUND__bufA;
            SOUND__currentSong.m_inactiveBuf = SOUND__bufB;
            SOUND__currentSong.m_readPos     = 0u;
            SOUND__currentSong.m_activeFill  = 0u;
        }
 
        /* Write into the inactive buffer - the ISR never touches this one (N1) */
        dst = SOUND__GetInactiveBuf();
        SOUND__currentSong.m_inactiveBuf = dst;
 
        (void)memcpy(dst, data, size);                                              /* N5: block copy instead of a byte loop */
        (void)memset(&dst[size], (int)SOUND_SILENCE, (size_t)SOUND_BUFFER_SIZE - size);   /* pad the remainder with silence */
 
        SOUND__CompilerBarrier();                       /* N2: all bytes are written before the buffer is announced */
        SOUND__currentSong.m_inactiveFill = size;       /* last step: now the ISR may swap */
    }
 
    return rc;
}
 
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
RC_t SOUND_SwapBuffers(void)
{
    uint8_t * tmpBuf;
 
    /* Swap the pointers and take over the fill level. In the ISR this cannot be
       interrupted by the reader (N1). */
    tmpBuf                            = SOUND__currentSong.m_activeBuf;
    SOUND__currentSong.m_activeBuf    = SOUND__currentSong.m_inactiveBuf;
    SOUND__currentSong.m_inactiveBuf  = tmpBuf;
    SOUND__currentSong.m_activeFill   = SOUND__currentSong.m_inactiveFill;
    SOUND__currentSong.m_inactiveFill = 0u;
    SOUND__currentSong.m_readPos      = 0u;
 
    return RC_SUCCESS;
}
 
/**
 * \brief Return the number of underrun ticks since the last call and reset the counter (N7).
 *
 * An underrun is a tick where the active buffer was finished and no new one was ready
 * because the reader was late. Diagnostic only: a tick that happens between reading and
 * clearing the counter is lost.
 *
 * \return number of ISR ticks without a new sample, 0..4294967295 (a count, not a status code, N9)
 */
uint32_t SOUND__GetAndClearUnderruns(void)
{
    uint32_t const n = SOUND__underruns;
 
    SOUND__underruns = 0u;                      /* N7: a tick between the two lines is lost */
    return n;
}
 
/*****************************************************************************/
/* Private helpers                                                           */
/*****************************************************************************/
 
/**
 * \brief Return the buffer that the ISR is NOT playing.
 * \return SOUND__bufB when A is active, otherwise SOUND__bufA
 */
static uint8_t * SOUND__GetInactiveBuf(void)
{
    return (SOUND__currentSong.m_activeBuf == SOUND__bufA) ? SOUND__bufB : SOUND__bufA;
}
 
/**
 * \brief Stop the compiler from moving memory accesses across this call (N2).
 *
 * GNU C inline assembler with a "memory" clobber, no instruction is generated.
 *
 */
static void SOUND__CompilerBarrier(void)
{
    __asm volatile ("" : : : "memory");
}
 
/********************************************************************************
 * ISR Definitions
 ********************************************************************************/
 
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
RC_t SOUND_isr(void)
{
    RC_t rc = RC_SUCCESS;
 
    /* Dual buffer streaming. The buffer pointer and the read position are read once (N3). */
    uint8_t const * const active = SOUND__currentSong.m_activeBuf;
 
    if (active != NULL)
    {
        uint16_t const pos = SOUND__currentSong.m_readPos;
 
        if (pos < SOUND__currentSong.m_activeFill)
        {
            /* N4: sample * volume / 100, the compiler turns "/ 100u" into a multiplication */
            uint16_t const sample = (uint16_t)(((uint16_t)active[pos] * (uint16_t)SOUND__currentSong.m_volume) / 100u);
 
            SOUND_PWM_WriteCompare((uint8_t)sample);
            SOUND__currentSong.m_readPos = (uint16_t)(pos + 1u);
        }
        else if (SOUND__currentSong.m_inactiveFill > 0u)
        {
            /* INC 6B.2: active buffer finished and the next one is ready.
               Swap, then ask the reader to refill the buffer just freed.
               The reader now has a full buffer time (1024 samples = 46.4 ms)
               instead of half. */
            (void)SOUND_SwapBuffers();
            (void)SetEvent(tsk_reader, ev_BufEmpty);
            DBG_DEAD_Write(1u);                 /* low again at the end of MP3__readBlock() */
        }
        else
        {
            /* Reader late (or end of song): hold last output, do not swap.
               Swapping only a filled buffer means FillBuffer can never write
               into the buffer the ISR is playing. */
            SOUND__underruns++;
            rc = RC_ERROR_BUFFER_EMTPY;         /* N9 */
        }
    }
 
    return rc;
}
 
/*----------------------------------------------------------------------------------------
* Notes:
*
* N1 - Hand shake between reader (task) and ISR. m_inactiveFill is the signal:
*        0      the inactive buffer is empty or being filled - the ISR must not swap
*        > 0    the inactive buffer is complete - the ISR may swap when it is done with the active one
*      SOUND_FillBuffer() writes all samples first and sets m_inactiveFill last. The ISR swaps only
*      when m_inactiveFill > 0 and clears it in the swap. So the reader never writes the buffer that
*      is being played, and the ISR never plays a half-filled buffer. This only works if the reader
*      fills while m_inactiveFill is 0, which is true after ev_BufEmpty (sent by the ISR after
*      each swap). The swap is not interruptible: it runs in the ISR, the reader is a task.
*
* N2 - volatile. The pointers m_activeBuf / m_inactiveBuf are volatile (the ISR changes them
*      while the reader runs). The sample bytes are NOT volatile any more: a buffer belongs to
*      one side at a time (N1), so every access is ordered by the hand shake and the bytes can be
*      copied with memcpy(). One thing is left for the compiler: it must not move the byte
*      writes behind the write of m_inactiveFill. SOUND__CompilerBarrier() does that. The
*      Cortex-M3 itself executes in order on one core, so no hardware barrier (DMB) is needed.
*      Before INC 11B the pointer pointed to volatile bytes ("volatile uint8_t *"), but the
*      pointer itself was not volatile - the wrong way round.
*
* N3 - Fewer memory accesses in the ISR. m_activeBuf and m_readPos were read 2 to 3 times per
*      tick (every access to a volatile struct member is a real load). Now each is read once.
*      The ISR is the only writer of m_readPos and the only place that swaps while the timer
*      runs, so the values cannot change during the tick. Behaviour is identical.
*
* N4 - Volume. sample = byte * volume / 100. For byte 0..255 and volume 0..100 the product fits
*      in 16 bit. Division by the constant 100 is compiled to a multiply and a shift, so there is
*      no slow divide instruction in the ISR. A look-up table would be faster but needs
*      256 bytes per volume step and would change nothing audible, so it was not done.
*
* N5 - SOUND_FillBuffer() used two byte loops with volatile stores (1024 iterations in total
*      per call, every iteration a load, a store and the loop test). memcpy() / memset() of the
*      C library copy words. DBG_FILL measured 0.3 ms (about 23 cycles per byte at 80 MHz) for
*      the old version, see the estimate at the end.
*
* N6 - Flash song (INC 1 to 4). SOUND_SetSong() stores the song, but the ISR plays only the dual
*      buffer, so the stored values are never used. The function stays because main.c still has
*      calls to it; it only switches the dual buffer path off (m_activeBuf = NULL), which
*      SOUND_FillBuffer() switches on again. The three legacy fields (12 bytes RAM) can be
*      removed together with the function.
*
* N7 - SOUND__underruns is incremented in the ISR and cleared by SOUND__GetAndClearUnderruns()
*      in a task. A tick between "read" and "clear" is lost. It is a diagnostic counter, so this
*      is accepted. For an exact count the two lines would have to run with interrupts
*      disabled (CyEnterCriticalSection()).
*
* N8 - MISRA C:2012 deviations that remain (checked by hand):
*      - Rule 1.2 / Dir 4.? (language extension): "__asm volatile" in SOUND__CompilerBarrier()
*        (GNU C). Replace by the CMSIS __DMB() if a hardware barrier is wanted.
*      - Rule 2.2 (dead code): the legacy fields m_song, m_size, m_pos are written, never read (N6).
*      - Naming rule of the course: SOUND__GetAndClearUnderruns() is public but has the private
*        "__" prefix. It is kept because the name was used in earlier increments.
*      - Rule 17.7: return values of SOUND_StopPlay(), SOUND_SwapBuffers(), SetEvent() are cast to
*        void, they cannot fail (RC_SUCCESS always, E_OK for an existing task and event).
*      - Rule 20.10 / Dir 4.9: the DBG_HI/DBG_LO macros with ## were removed.
*      - Rule 4.4 / 3.1 (commented-out code): the two "//isr_Timer_Disable" lines were removed,
*        the reason stays in one comment in SOUND_StopPlay().
*
* N9 - Return codes (INC 11C.1). Functions that can fail for a reason return RC_t:
*        SOUND_FillBuffer()  RC_ERROR_NULL (data is NULL, new check, before: crash in memcpy),
*                            RC_ERROR_ZERO (size 0), RC_ERROR_OVERRUN (size > SOUND_BUFFER_SIZE).
*                            Before: RC_ERROR for the last two. Nothing is changed in these cases.
*        SOUND_SetVolume()   RC_ERROR_RANGE when volume > SOUND_VOLUME_MAX. The limited value is
*                            still stored, so the sound is as before; the caller can see that
*                            it asked for too much.
*        SOUND_isr()         RC_ERROR_BUFFER_EMTPY for a tick that counted an underrun, RC_SUCCESS
*                            otherwise. The output is identical; the ISR2 wrapper ignores the value
*                            (MISRA 17.7: cast it to void there).
*      Always RC_SUCCESS because nothing can fail (the PSoC component calls return void):
*        SOUND_Init(), SOUND_StartPlay(), SOUND_StopPlay(), SOUND_SetSong() (legacy, N6),
*        SOUND_SwapBuffers(). SOUND__GetAndClearUnderruns() returns a count, not a status.
*      Callers: mp3.c casts every result to void and passes volume steps of 0..100, so nothing
*      changes there. The comment at its SOUND_FillBuffer() call may say RC_ERROR_ZERO /
*      RC_ERROR_OVERRUN instead of RC_ERROR.
*
*
* Time (estimated):
* The ISR runs every 45.35 us (22050 Hz). Measured before: DBG_FILL (SOUND_FillBuffer) 0.30 ms
* of the 47.06 ms buffer time. The estimate for the new version is in the INC 11B report;
* measure the new value with the DBG_FILL pin.
*/