/**
* \file sound.c
* \author P. Fromm
* \date 6.11.18
*
* \brief Sound driver
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
#include "sound.h"
#include "project.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/

typedef struct {
    uint8_t const * m_song;             /* Legacy flash params */
    uint32_t        m_size;
    uint32_t        m_pos;
    volatile uint8_t * m_activeBuf;     /* Dual buffer params */
    volatile uint8_t * m_inactiveBuf;
    volatile uint16_t  m_activeFill;
    volatile uint16_t  m_inactiveFill;
    volatile uint16_t  m_readPos;
    uint8_t m_volume;                   /* Shared */
} SOUND__Driver_t;


/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

static uint8_t SOUND__bufA[SOUND_BUFFER_SIZE];
static uint8_t SOUND__bufB[SOUND_BUFFER_SIZE];

static SOUND__Driver_t SOUND__currentSong = {0,0,0,0,0,0,0,0,100u};

/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

static volatile uint8_t * SOUND__GetInactiveBuf(void);

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

RC_t SOUND_Init()
{
    /* Pre-fill both buffers with silence - no click on start */
    memset(SOUND__bufA, SOUND_SILENCE, SOUND_BUFFER_SIZE);
    memset(SOUND__bufB, SOUND_SILENCE, SOUND_BUFFER_SIZE);

    /* Dual buffer: A active, B inactive */
    SOUND__currentSong.m_activeBuf    = SOUND__bufA;
    SOUND__currentSong.m_inactiveBuf  = SOUND__bufB;
    SOUND__currentSong.m_activeFill   = SOUND_BUFFER_SIZE;
    SOUND__currentSong.m_inactiveFill = 0u;
    SOUND__currentSong.m_readPos      = 0u;

    /* Flash path: cleared */
    SOUND__currentSong.m_song = 0;
    SOUND__currentSong.m_size = 0;
    SOUND__currentSong.m_pos  = 0;

    /* Hardware init */
    SOUND_Control_Write(SOUND_CH1);
    SOUND_ENABLE_Write(1);
    SOUND_StopPlay();
    SOUND_PWM_CLOCK_Start();
    SOUND_PWM_Start();
    SOUND_PWM_WriteCompare(SOUND_SILENCE);

    return RC_SUCCESS;
}

/**
 * Activate the Timer ISR for playing a song
 * /return RC_SUCCESS
 */
RC_t SOUND_StartPlay()
{
    SOUND_SAMPLE_CLOCK_Start();
    
    return RC_SUCCESS;
}

/**
 * DeActivate the Timer ISR for playing a song
 * /return RC_SUCCESS
 */
RC_t SOUND_StopPlay()
{
    SOUND_SAMPLE_CLOCK_Stop();
    
    //isr_Timer_Disable seems not to work
    //isr_Timer_Disable();
    
    return RC_SUCCESS;
}

/**
 * Set the song. This song will be played in an endless loop
 * /param   uint8_t const *const song       : pointer to the song data [IN]
 * /param   uint32_t sizeOfSong             : size of the song data [IN]
 * /return RC_SUCCESS
 */
RC_t SOUND_SetSong(uint8_t const *const song, uint32_t sizeOfSong)
{
    /* Disable dual buffer path */
    SOUND__currentSong.m_activeBuf = 0;

    /* Set flash path */
    SOUND__currentSong.m_song = song;
    SOUND__currentSong.m_size = sizeOfSong;
    SOUND__currentSong.m_pos  = 0u;

    return RC_SUCCESS;
}

/**
 * Set the volume
 * /param   uint8_t volume                 : volume in % 0...100
 * /return RC_SUCCESS
 */
RC_t SOUND_SetVolume(uint8_t volume)
{
    if (volume > 100) volume = 100;
    
    SOUND__currentSong.m_volume = volume;
    
    return RC_SUCCESS;
}

RC_t SOUND_FillBuffer(uint8_t const * const data, uint16_t size)
{
    uint16_t i;

    if ((size == 0u) || (size > SOUND_BUFFER_SIZE))
    {
        return RC_ERROR;
    }

    /* Switch to dual buffer mode if not already */
    if (SOUND__currentSong.m_activeBuf == 0)
    {
        SOUND__currentSong.m_activeBuf   = SOUND__bufA;
        SOUND__currentSong.m_inactiveBuf = SOUND__bufB;
        SOUND__currentSong.m_readPos     = 0u;
        SOUND__currentSong.m_activeFill  = 0u;
    }

    /* Write into inactive buffer — ISR never touches this one */
    SOUND__currentSong.m_inactiveBuf = SOUND__GetInactiveBuf();

    for (i = 0u; i < size; i++)
    {
        SOUND__currentSong.m_inactiveBuf[i] = data[i];
    }

    /* Pad remainder with silence for partial last buffer */
    for (i = size; i < SOUND_BUFFER_SIZE; i++)
    {
        SOUND__currentSong.m_inactiveBuf[i] = SOUND_SILENCE;
    }

    SOUND__currentSong.m_inactiveFill = size;

    return RC_SUCCESS;
}

RC_t SOUND_SwapBuffers()
{
    volatile uint8_t * tmpBuf;

    /* Single pointer swap — atomic on Cortex-M3 */
    tmpBuf                            = SOUND__currentSong.m_activeBuf;
    SOUND__currentSong.m_activeBuf    = SOUND__currentSong.m_inactiveBuf;
    SOUND__currentSong.m_inactiveBuf  = tmpBuf;
    SOUND__currentSong.m_activeFill   = SOUND__currentSong.m_inactiveFill;
    SOUND__currentSong.m_inactiveFill = 0u;
    SOUND__currentSong.m_readPos      = 0u;

    return RC_SUCCESS;
}

/*****************************************************************************/
/* Private helpers                                                           */
/*****************************************************************************/

static volatile uint8_t * SOUND__GetInactiveBuf(void)
{
    if (SOUND__currentSong.m_activeBuf == SOUND__bufA)
    {
        return SOUND__bufB;
    }
    else
    {
        return SOUND__bufA;
    }
}

/********************************************************************************
 * ISR Definitions
 ********************************************************************************/

RC_t SOUND_isr()
{

    // Dual buffer streaming
    if (SOUND__currentSong.m_activeBuf != 0)
    {
        if (SOUND__currentSong.m_readPos < SOUND__currentSong.m_activeFill)
        {
            uint16_t sample =
                ((uint16_t)SOUND__currentSong.m_activeBuf[SOUND__currentSong.m_readPos]
                 * (uint16_t)SOUND__currentSong.m_volume) / 100u;

            SOUND_PWM_WriteCompare((uint8_t)sample);
            SOUND__currentSong.m_readPos++;

            if (SOUND__currentSong.m_readPos == (SOUND__currentSong.m_activeFill / 2u))
            {
                SetEvent(tsk_reader, ev_BufEmpty);
            }
        }
        else
        {
            SOUND_StopPlay();
            SOUND_SwapBuffers();
            SOUND_StartPlay();
        }
        
        return RC_SUCCESS;
    }
    
    return RC_SUCCESS;
}