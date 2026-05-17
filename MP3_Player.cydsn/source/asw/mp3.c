/**
* \file mp3.c
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
#include "mp3.h"
#include "sound.h"
#include "sound_data.h"
#include "seven.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/
#define MP3__MAXVOLUME 5
/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/
 enum eMP3__State{
    MP3STATE_ISSTOPPED,
    MP3STATE_ISPLAYING,
    MP3STATE_ISPAUSING,
} ;
typedef enum eMP3__State MP3__State_t;
/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

static struct
{
    MP3__State_t m_state;                       /**< \brief state of the state machine */
    uint16_t     m_noSongs;                     /**< \brief number of songs in flash */
    uint16_t     m_currentSong;                 /**< \brief song currently selected */
    uint8_t      m_volume;                      /**< \brief volume */
    uint16_t     m_remainingPlayTime;           /**< \brief remaining play time of the song */
} MP3__player = {MP3STATE_ISSTOPPED, 0, 0, 0, 0};

/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

void MP3__nextSong();               /**< \brief select the next song. Go to first in case of EOL */
void MP3__prevSong();               /**< \brief select the previous song. Go to last in case of EOL */

void MP3__increaseVolume();         /**< \brief increase the volume by one */
void MP3__decreaseVolume();         /**< \brief decrease the volume by one */

void MP3__playSong();               /**< \brief set the sound record pointer to the current song and activate the clock */
void MP3__stopSong();               /**< \brief Stop the clock for the timer ISR */
void MP3__pauseSong();              /**< \brief Stop the clock for the timer ISR */
void MP3__resumeSong();             /**< \brief Start the clock for the timer ISR */

void MP3__showSong();               /**< \brief Show the songs via uart */
void MP3__showRemainingPlaytime();  /**< \brief Show the remaining play time via 7 segment */

boolean_t MP3__songEnded();         /**< \brief Check if the remaining playtime has ended */

void MP3__showEventAndState(EventMaskType ev, MP3__State_t state);  /**< \brief Helper function to show state and event */


/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * Init Function for the sound driver
 * /return RC_SUCCESS
 */
RC_t MP3_Init()
{
    MP3__player.m_state = MP3STATE_ISSTOPPED;
    MP3__player.m_noSongs = SOUND_NoOfRecords;
    MP3__player.m_volume = MP3__MAXVOLUME;
    MP3__player.m_currentSong = 0;

    MP3__showSong();
    SEVEN_Set(SEVEN_1,MP3__player.m_volume);
    SOUND_SetVolume(MP3__player.m_volume * 20);
    
    return RC_SUCCESS;
}

/**
 * The implementation of the MP3 Player State Machine
 * /param EventMaskType nextEvent   : The event to be processed [IN]
 * /return RC_SUCCESS
 */
RC_t MP3_ProcessEvent(EventMaskType nextEvent)
{
    //Debugging aid
    //MP3__showEventAndState(nextEvent, MP3__player.m_state);
    
    switch (MP3__player.m_state)
    {
        case MP3STATE_ISSTOPPED :
            if (nextEvent & ev_Button1)
            {
                MP3__prevSong();
                MP3__showSong();
            }
            else if (nextEvent & ev_Button2)
            {
                MP3__nextSong();
                MP3__showSong();
            }
            else if (nextEvent & ev_Button3)
            {
                MP3__playSong();
                MP3__showSong();
                MP3__player.m_state = MP3STATE_ISPLAYING;
            }
            break;

        case MP3STATE_ISPLAYING :
            if (nextEvent & ev_Button1)         
            {
                MP3__increaseVolume();
            }
            else if (nextEvent & ev_Button2)
            {
                MP3__decreaseVolume();
            }
            else if (nextEvent & ev_Button3)
            {
                MP3__pauseSong();
                MP3__player.m_state = MP3STATE_ISPAUSING;
            }
            else if (nextEvent & ev_Button4)
            {
                MP3__stopSong();
                MP3__player.m_state = MP3STATE_ISSTOPPED;
            }
            else if (nextEvent & ev_1s && !MP3__songEnded())
            {
                MP3__showRemainingPlaytime();
            }
            else if (nextEvent & ev_1s && MP3__songEnded())
            {
                MP3__nextSong(); 
                MP3__playSong();
                MP3__showSong();
            }
            break;
            
        case MP3STATE_ISPAUSING :
            if (nextEvent & ev_Button3)
            {
                MP3__resumeSong();
                MP3__player.m_state = MP3STATE_ISPLAYING;
            }
            else if (nextEvent & ev_Button4)
            {
                MP3__stopSong();
                MP3__player.m_state = MP3STATE_ISSTOPPED;
            }

            break;
    }
    return RC_SUCCESS;
}




void MP3__nextSong()
{
    MP3__player.m_currentSong++;
    MP3__player.m_currentSong %= MP3__player.m_noSongs;
    
}

void MP3__prevSong()
{
    if (MP3__player.m_currentSong == 0)
    {
        MP3__player.m_currentSong += MP3__player.m_noSongs;
    }
    MP3__player.m_currentSong--;
}

void MP3__increaseVolume()
{
    if (MP3__player.m_volume < MP3__MAXVOLUME) MP3__player.m_volume++;
    SEVEN_Set(SEVEN_1,MP3__player.m_volume);
    SOUND_SetVolume(MP3__player.m_volume * 20);
}

void MP3__decreaseVolume()
{
    if (MP3__player.m_volume > 0) MP3__player.m_volume--;
    SEVEN_Set(SEVEN_1,MP3__player.m_volume);
    SOUND_SetVolume(MP3__player.m_volume * 20);

}

void MP3__playSong()
{
    MP3__player.m_remainingPlayTime = SOUND_Records[MP3__player.m_currentSong].m_length;
    SOUND_SetSong(SOUND_Records[MP3__player.m_currentSong].m_data, SOUND_Records[MP3__player.m_currentSong].m_size);
    SOUND_StartPlay();
}

void MP3__stopSong()
{
    SOUND_StopPlay();
}

void MP3__pauseSong()
{
   SOUND_StopPlay(); 
}

void MP3__resumeSong()
{
    SOUND_StartPlay();
}

void MP3__showSong()
{
    UART_LOG_PutString("\n\n===================\n");
    
    for (uint16_t i = 0; i < MP3__player.m_noSongs; i++)
    {
        if (i == MP3__player.m_currentSong)
        {
            UART_LOG_PutString("(*)");
        }
        else
        {
            UART_LOG_PutString("( )");
        }
        UART_LOG_PutString(SOUND_Records[i].m_title);
        UART_LOG_PutString("\n");
    }
    

}

void MP3__showRemainingPlaytime()
{
    MP3__player.m_remainingPlayTime--;
    SEVEN_Set(SEVEN_0,MP3__player.m_remainingPlayTime);
    
}

boolean_t MP3__songEnded()
{
    return MP3__player.m_remainingPlayTime == 0;
}

void MP3__showEventAndState(EventMaskType ev, MP3__State_t state)
{
    char* stateStr;
    char* eventStr;
    
    //Create the event string
    switch (ev)
    {
        case ev_1s : eventStr = "ev_1s"; break;
        case ev_Button1 : eventStr = "ev_Button1"; break;
        case ev_Button2 : eventStr = "ev_Button2"; break;
        case ev_Button3 : eventStr = "ev_Button3"; break;
        case ev_Button4 : eventStr = "ev_Button4"; break;
        default: eventStr = "WRONG EVENT"; break;
    }
    
    //Create the state string
    switch (state)
    {
        case MP3STATE_ISSTOPPED : stateStr = "IsStopped"; break; 
        case MP3STATE_ISPLAYING : stateStr = "IsPlaying"; break;
        case MP3STATE_ISPAUSING : stateStr = "IsPausing"; break;
        default : stateStr = "IsStopped"; break;
    }
    
    //Print it via USB
    UART_LOG_PutString("State: ");
    UART_LOG_PutString(stateStr);
    UART_LOG_PutString(" | Event: ");
    UART_LOG_PutString(eventStr);
    UART_LOG_PutString("\n");
}
    