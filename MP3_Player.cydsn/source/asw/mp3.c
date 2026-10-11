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

#include "wav_parser.h"
#include "FS.h"

#include <string.h>
#include <stdio.h>

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

#define MP3__MAXVOLUME   (5u)
#define MP3__SD_ROOT     "\\"
#define MP3__MAX_BLOCK_ALIGN (6u)     /* largest supported frame: 24-bit stereo = 2 ch * 3 bytes */

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

enum eMP3__ReaderState
{
    MP3READER_IDLE,
    MP3READER_OPEN,
    MP3READER_STREAMING,
    MP3READER_EOF,
};
typedef enum eMP3__ReaderState MP3__ReaderState_t;

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

static struct
{
    MP3__ReaderState_t  m_state;
    uint16_t            m_songIndex;
    boolean_t           m_newSongRequest;
} MP3__reader = {MP3READER_IDLE, 0u, FALSE};

static char MP3__songList[MP3_MAX_SONGS][MP3_FILENAME_MAXLEN];  /* Song filename list built from SD card scan */

static FS_FILE * MP3__pFile = 0;                        /* emFile file handle - static, not on stack */

static WAVFile MP3__wavInfo;                            /* Decoded WAV info */
 
//static uint8_t MP3__readBuf[2u * SOUND_BUFFER_SIZE];    /* Static read buffer - 1024 bytes (16-bit needs 2 bytes/sample), kept off tsk_reader stack */ // INC 5C

static uint8_t MP3__readBuf[MP3__MAX_BLOCK_ALIGN * SOUND_BUFFER_SIZE];  /* 3072 bytes - one block of the largest frame format, kept off tsk_reader stack */ // INC 6B

static uint32_t MP3__bytesRemaining = 0u;               /* Remaining PCM bytes not yet read from current file */


/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

void MP3__readCard();               /**< \brief Read the card conetents */

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
void MP3__setRemainingPlaytime();   /**< \brief Calculates and sets the initial play time once when the file opens */

boolean_t MP3__songEnded();         /**< \brief Check if the remaining playtime has ended */

void MP3__showEventAndState(EventMaskType ev, MP3__State_t state);  /**< \brief Helper function to show state and event */

static RC_t MP3__openWavFile(uint16_t songIndex);
static void MP3__closeWavFile(void);

static void MP3__readBlock(void);

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

RC_t MP3_Init()
{
 
    MP3__player.m_state       = MP3STATE_ISSTOPPED;
    MP3__player.m_volume      = MP3__MAXVOLUME;
    MP3__player.m_currentSong = 0u;
 
    MP3__reader.m_state          = MP3READER_IDLE;
    MP3__reader.m_newSongRequest = FALSE;
 
    /* Scan SD card for WAV files */
    UART_LOG_PutString("\r\n[MP3] Scanning SD card...\n");
    
    FS_Init();
    MP3__readCard();
 
    SEVEN_Set(SEVEN_1, MP3__player.m_volume);
    SOUND_SetVolume(MP3__player.m_volume * 20u);
    MP3__showSong();
 
    //sprintf(logBuf, "\r\n[MP3] Ready. %u song(s).\n", MP3__player.m_noSongs);
    //UART_LOG_PutString(logBuf);
 
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

void MP3_Reader()
{
    char     logBuf[48];

    switch (MP3__reader.m_state)
    {
        case MP3READER_IDLE:
        {
            /* INC 5E: the only place a new song request is accepted */
            if (MP3__reader.m_newSongRequest == FALSE)
            {
                break;
            }
            MP3__reader.m_newSongRequest = FALSE;
            MP3__reader.m_state          = MP3READER_OPEN;
            /* fall through - open the new song with the same event */
        }

        case MP3READER_OPEN:
        {
            //UART_LOG_PutString("\r\n[MP3] Open: ");                     // INC 5E
            //UART_LOG_PutString(MP3__songList[MP3__reader.m_songIndex]);

            if (MP3__openWavFile(MP3__reader.m_songIndex) != RC_SUCCESS)
            {
                MP3__reader.m_state = MP3READER_IDLE;
                MP3__player.m_state = MP3STATE_ISSTOPPED;
                break;
            }

            MP3__setRemainingPlaytime();
            
            /* Pre-fill Buffer A - becomes active */                       // INC 5C
            MP3__readBlock();
            /* DEBUG 5D: first 16 samples after conversion - expect values around 0x80 */
            /*{
                uint8_t k;
                UART_LOG_PutString("\r\n[5D] First 16 samples: ");
                for (k = 0u; k < 16u; k++)
                {
                    sprintf(logBuf, "%02X ", MP3__readBuf[k]);
                    UART_LOG_PutString(logBuf);
                }
                UART_LOG_PutString("\n");
            }*/
            SOUND_SwapBuffers();

            /* Pre-fill Buffer B - inactive, ready for first swap */
            if (MP3__bytesRemaining > 0u)
            {
                MP3__readBlock();
            }

            sprintf(logBuf, "\r\n[MP3] Underruns before start: %lu\n", SOUND_GetAndClearUnderruns());  // INC 6B.3 DIAG
            UART_LOG_PutString(logBuf);
            
            SOUND_StartPlay();

            MP3__reader.m_state = MP3READER_STREAMING;
            break;
        }

        case MP3READER_STREAMING:
        {
            /* INC 5E: new song request - leave current song, let IDLE handle it */
            if (MP3__reader.m_newSongRequest == TRUE)
            {
                SOUND_StopPlay();
                MP3__closeWavFile();
                MP3__reader.m_state = MP3READER_IDLE;
                SetEvent(tsk_reader, ev_BufEmpty);  /* re-run reader now, IDLE opens new song */
                break;
            }

            /* EOF check */
            if (MP3__bytesRemaining == 0u)
            {
                sprintf(logBuf, "\r\n[MP3] Underruns in song: %lu\n", SOUND_GetAndClearUnderruns());  // INC 6B.3 DIAG
                UART_LOG_PutString(logBuf);
                MP3__reader.m_state = MP3READER_EOF;
                break;
            }

            /* Normal case - read next block into inactive buffer         // INC 5C
               while ISR still runs on active buffer */
            MP3__readBlock();

            break;
        }

        case MP3READER_EOF:
        {
            SOUND_StopPlay();
            MP3__closeWavFile();

            MP3__player.m_remainingPlayTime = 0u;
            SEVEN_Set(SEVEN_0, 0u);

            //UART_LOG_PutString("\r\n[MP3] EOF - song complete\n");
            MP3__reader.m_state = MP3READER_IDLE;

            /* INC 5E: the event that brought us here may be a new song request
               (from MP3__playSong) - let IDLE handle it right away */
            if (MP3__reader.m_newSongRequest == TRUE)
            {
                SetEvent(tsk_reader, ev_BufEmpty);
            }
            break;
        }
    }
}

/*****************************************************************************/
/* Private helpers                                                           */
/*****************************************************************************/

void MP3__readCard()
{
    FS_DIR    * pDir;
    FS_DIRENT * pDirEnt;
    char        entryName[64];
    //char        logBuf[48];
    U8          attr;
    uint16_t    songCount = 0;
    
    pDir = FS_OpenDir(MP3__SD_ROOT);
    if (pDir == 0)
    {
        UART_LOG_PutString("\r\n[MP3] ERROR: cannot open root dir\n");
        /* Fall back to flash songs */
        MP3__player.m_noSongs = SOUND_NoOfRecords;
        uint16_t i;
        for (i = 0u; i < SOUND_NoOfRecords && i < MP3_MAX_SONGS; i++)
            strncpy(MP3__songList[i], SOUND_Records[i].m_title,
                    MP3_FILENAME_MAXLEN - 1u);
    }
    else
    {
        while (songCount < MP3_MAX_SONGS)
        {
            pDirEnt = FS_ReadDir(pDir);
            if (pDirEnt == 0) break;
 
            FS_DirEnt2Name(pDirEnt, entryName);
            FS_DirEnt2Attr(pDirEnt, &attr);
 
            if (attr & FS_ATTR_DIRECTORY) continue;
 
            if (strstr(entryName, ".WAV") != 0 ||
                strstr(entryName, ".wav") != 0)
            {
                strncpy(MP3__songList[songCount], entryName,
                        MP3_FILENAME_MAXLEN - 1u);
                MP3__songList[songCount][MP3_FILENAME_MAXLEN - 1u] = '\0';
                songCount++;
 
                //sprintf(logBuf, "\r\n[MP3] Found: %s\n", entryName);
                //UART_LOG_PutString(logBuf);
            }
        }
        FS_CloseDir(pDir);
        MP3__player.m_noSongs = songCount;
    }
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

void MP3__playSong(void)
{
    MP3__player.m_remainingPlayTime  = 0u;
    MP3__reader.m_songIndex          = MP3__player.m_currentSong;
    MP3__reader.m_newSongRequest     = TRUE;

    SetEvent(tsk_reader, ev_BufEmpty);
}

void MP3__stopSong(void)
{
    SOUND_StopPlay();
    MP3__closeWavFile();
    MP3__reader.m_state = MP3READER_IDLE;
    UART_LOG_PutString("\r\n[MP3] Stopped\n");
}

void MP3__pauseSong()
{
    SOUND_StopPlay();
    UART_LOG_PutString("\r\n[MP3] Paused\n");
}
 
void MP3__resumeSong()
{
    SOUND_StartPlay();
    UART_LOG_PutString("\r\n[MP3] Resumed\n");
}

void MP3__showSong()
{
    UART_LOG_PutString("\r\n===================\r\n");
    
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
        UART_LOG_PutString(MP3__songList[i]);
        UART_LOG_PutString("\r\n");
    }

}
// decrements and displays it every second while playing
void MP3__showRemainingPlaytime()
{
    //char logBuf[24];    // INC 5B
    
    if (MP3__player.m_remainingPlayTime > 0u)
        MP3__player.m_remainingPlayTime--;
    SEVEN_Set(SEVEN_0,MP3__player.m_remainingPlayTime);
    
    //sprintf(logBuf, "\r\n[MP3] Remaining: %u s\n", MP3__player.m_remainingPlayTime);
    //UART_LOG_PutString(logBuf);
}

/* Set remaining play time from WAV header                    */   // INC 5B
/* seconds = data bytes / (samples per s * bytes per sample)  */
void MP3__setRemainingPlaytime()
{
    //char     logBuf[48];
    
    MP3__player.m_remainingPlayTime =
            (uint16_t)(MP3__wavInfo.data_length
                / ((uint32_t)MP3__wavInfo.header.sample_rate
                * MP3__wavInfo.header.block_align));
    SEVEN_Set(SEVEN_0, MP3__player.m_remainingPlayTime);

    /*sprintf(logBuf, "\r\n[MP3] Playtime: %u s (%u-bit)\n",
            MP3__player.m_remainingPlayTime,
            MP3__wavInfo.header.bits_per_sample);
    UART_LOG_PutString(logBuf);*/
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

static RC_t MP3__openWavFile(uint16_t songIndex)
{
    //char logBuf[48];
 
    MP3__pFile = FS_FOpen(MP3__songList[songIndex], "r");
    if (MP3__pFile == 0)
    {
        UART_LOG_PutString("\r\n[MP3] ERROR: FS_FOpen failed\n");
        return RC_ERROR;
    }
 
    if (WAV_ParseHeader(MP3__pFile, &MP3__wavInfo) != RC_SUCCESS)
    {
        FS_FClose(MP3__pFile);
        MP3__pFile = 0;
        UART_LOG_PutString("\r\n[MP3] ERROR: WAV header invalid\n");
        return RC_ERROR;
    }
 
    MP3__bytesRemaining = MP3__wavInfo.data_length;
 
    WAV_PrintHeader(&MP3__wavInfo);
    //sprintf(logBuf, "\r\n[MP3] Streaming: %lu bytes\n", MP3__bytesRemaining);
    //UART_LOG_PutString(logBuf);
 
    return RC_SUCCESS;
}
 
static void MP3__closeWavFile(void)
{
    if (MP3__pFile != 0)
    {
        FS_FClose(MP3__pFile);
        MP3__pFile = 0;
    }
    MP3__bytesRemaining = 0u;
}

/**
 * Read one block of PCM from the open WAV file, convert every frame to one
 * 8-bit unsigned sample for VDAC8, and fill the inactive sound buffer.
 * Always produces up to SOUND_BUFFER_SIZE samples, so ISR timing is
 * identical for every format.
 *
 *   Format         block_align  bytes read   MSB offset in frame
 *   8-bit  mono         1          512              0
 *   16-bit mono         2         1024              1
 *   24-bit mono         3         1536              2
 *   stereo           2 x mono     2 x mono      left channel only (6B)
 */
static void MP3__readBlock(void)
{
    uint16_t blockAlign = MP3__wavInfo.header.block_align;                     /* bytes per frame, 1..6     */
    uint8_t  bytesPerCh = (uint8_t)(MP3__wavInfo.header.bits_per_sample / 8u); /* bytes per channel, 1..3   */
    uint8_t  msbOff     = bytesPerCh - 1u;                                     /* MSB is the last byte (LE) */
    uint32_t maxBytes   = (uint32_t)SOUND_BUFFER_SIZE * blockAlign;
    uint32_t toRead;
    U32      bytesRead;
    uint16_t frames;
    uint16_t i;

    /* INC 6A: never read more than the buffer can hold */
    if (maxBytes > sizeof(MP3__readBuf))
    {
        maxBytes = sizeof(MP3__readBuf);
    }

    toRead = (MP3__bytesRemaining >= maxBytes)
             ? maxBytes
             : MP3__bytesRemaining;

    bytesRead = FS_Read(MP3__pFile, MP3__readBuf, toRead);
    if (bytesRead == 0u)
    {
        MP3__bytesRemaining = 0u;          /* read error -> force EOF, avoid endless loop */
        return;
    }
    MP3__bytesRemaining -= (uint32_t)bytesRead;

    frames = (uint16_t)(bytesRead / blockAlign);

    if (blockAlign > 1u)                   /* 8-bit mono needs no conversion */
    {
        for (i = 0u; i < frames; i++)
        {
            /* N2: in place is safe - write index i, read index i*blockAlign+msbOff >= i */
            uint8_t msb = MP3__readBuf[(i * blockAlign) + msbOff];   /* 6B: channel 0 (left) only */

            /* N1: 8-bit is already unsigned; 16/24-bit MSB is signed -> flip sign bit */
            MP3__readBuf[i] = (bytesPerCh == 1u) ? msb : (uint8_t)(msb ^ 0x80u);
        }
    }

    if (frames > 0u)
    {
        SOUND_FillBuffer(MP3__readBuf, frames);
    }
}

/*----------------------------------------------------------------------------------------
* Notes:
* N1 - hi ^ 0x80 because shifting a 16-bit sample right by 8 keeps only its high byte, which is 
*      signed (−128…127). Adding 128 moves that range to 0…255, the unsigned range VDAC8 expects. 
*      For a single byte, adding 128 is the same as flipping bit 7, so the whole conversion is 
*      one XOR. That keeps it cheap: 512 XORs per block.
*
* N2 - Frame layout (little-endian): [ch0 LSB..MSB][ch1 LSB..MSB]. The MSB of ch0 sits at
*      offset bytesPerCh-1. Lower bytes are dropped (VDAC8 resolution). In-place conversion
*      is safe because the write index i never passes the read index i*blockAlign+msbOff.
* 
*Rejected files also expose an existing behavior, not part of 6A. When MP3__OpenWavFile() fails, 
the reader sets MP3__player.m_state = MP3STATE_ISSTOPPED (mp3.c line 261). But MP3_ProcessEvent() 
sets m_state = MP3STATE_ISPLAYING after calling MP3__playSong() (line 185). So depending on task 
priorities, the player can show PLAYING on a file that failed to open, and then auto-advance a 
second later. A clean fix would be to skip unplayable files automatically. I suggest doing that 
as an optional increment after 6C, so 6B stays focused.
*
*
*
*/