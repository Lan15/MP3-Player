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

/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

static struct {
    uint8_t const* m_song;
    uint32_t m_size;
    uint32_t m_pos;
    uint8_t m_volume;
} SOUND__currentSong = {0,0,0,0};


/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/


/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * Init Function for the sound driver
 * /return RC_SUCCESS
 */
RC_t SOUND_Init()
{
    //Set the MUX to channel 1
    SOUND_Control_Write(SOUND_CH1);
    
    //Enable power supply
    SOUND_ENABLE_Write(1);

    //Disable the song sample timer
    SOUND_StopPlay();

    //Initialize PWM and Clock
    SOUND_PWM_CLOCK_Start();
    SOUND_PWM_Start();
    SOUND_PWM_WriteCompare(0);
    
    
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
    SOUND__currentSong.m_song = song;
    SOUND__currentSong.m_size = sizeOfSong;
    SOUND__currentSong.m_pos = 0;
    
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

/**
 * isr code for timer interrupt
 * /return RC_SUCCESS
 */
RC_t SOUND_isr()
{
    
    
    //Send the next PWM value to the PWM register
    if (0 != SOUND__currentSong.m_song)
    {
        if (SOUND__currentSong.m_pos < SOUND__currentSong.m_size)
        {
            uint16_t data = ((uint16_t)SOUND__currentSong.m_song[SOUND__currentSong.m_pos++] * SOUND__currentSong.m_volume) / 100;
            SOUND_PWM_WriteCompare((uint8_t)data);
        }
        else
        {
            //Restart song
            SOUND__currentSong.m_pos = 0;
        }
    }
    
    return RC_SUCCESS;
}
