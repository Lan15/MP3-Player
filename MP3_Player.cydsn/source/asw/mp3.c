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
#include "seven.h"
#include "tft.h"

#include "DBG_FS.h"
#include "DBG_CONV.h"
#include "DBG_FILL.h"
#include "DBG_DEAD.h"

#include "wav_parser.h"
#include "FS.h"

#include <string.h>
#include <stdio.h>

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

#define MP3__MAXVOLUME          (5u)
#define MP3__SD_ROOT            "\\"        /* MP3__PATH_SEP */
#define MP3__MAX_BLOCK_ALIGN    (6u)        /* largest supported frame: 24-bit stereo = 2 ch * 3 bytes */
#define MP3__OUTPUT_RATE        (22050u)    /* SOUND_SAMPLE ISR rate - one output sample per tick */
#define MP3__SECTOR_SIZE        (512u)      /* SD sector size - reads are kept sector-aligned (INC 8) */
#define MP3__MAX_INBYTES        (12u)       /* largest input bytes per output sample: 24-bit stereo, step 2 */
#define MP3__MAX_READ           ((uint32_t)MP3__MAX_BLOCK_ALIGN * SOUND_BUFFER_SIZE)  /* largest raw read per block */

// Testing  
#define DBG_HI(pin)  pin##_Write(1u)
#define DBG_LO(pin)  pin##_Write(0u)

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
    uint16_t            m_failCount;        /* INC 7: consecutive files that failed to open */
} MP3__reader = {MP3READER_IDLE, 0u, FALSE, 0u};

static char MP3__songList[MP3_MAX_SONGS][MP3_FILENAME_MAXLEN];  /* Song filename list built from SD card scan */

static FS_FILE * MP3__pFile = 0;                                /* emFile file handle - static, not on stack */

static WAVFile MP3__wavInfo;                                    /* Decoded WAV info */

//static uint8_t MP3__readBuf[MP3__MAX_BLOCK_ALIGN * SOUND_BUFFER_SIZE];  /* 3072 bytes - one block of the largest frame format, kept off tsk_reader stack */ // INC 6B

/* INC 8: + MP3__MAX_INBYTES so a carried partial frame fits in front of a full-size read */
static uint8_t MP3__readBuf[MP3__MAX_READ + MP3__MAX_INBYTES];

static uint32_t MP3__bytesRemaining = 0u;                       /* Remaining PCM bytes not yet read from current file */

/* INC 8: sector alignment */
static uint16_t MP3__alignBytes = 0u;                           /* raw bytes up to the next sector boundary, 0 = aligned */
static uint8_t  MP3__carry[MP3__MAX_INBYTES];                   /* partial output sample left over after the first read */
static uint8_t  MP3__carryLen = 0u;


/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

void MP3__initDisplay();            /**< \brief draw static header once */

static void MP3__scanDir(const char * path);                /**< \brief recursive helper */
static const char * MP3__shortName(const char * fullPath);  /**< \brief short name for display */

void MP3__nextSong();                                       /**< \brief select the next song. Go to first in case of EOL */
void MP3__prevSong();                                       /**< \brief select the previous song. Go to last in case of EOL */

void MP3__increaseVolume();                                 /**< \brief increase the volume by one */
void MP3__decreaseVolume();                                 /**< \brief decrease the volume by one */

void MP3__playSong();                                       /**< \brief set the sound record pointer to the current song and activate the clock */
void MP3__stopSong();                                       /**< \brief Stop the clock for the timer ISR */
void MP3__pauseSong();                                      /**< \brief Stop the clock for the timer ISR */
void MP3__resumeSong();                                     /**< \brief Start the clock for the timer ISR */

void MP3__showRemainingPlaytime();                          /**< \brief Show the remaining play time via 7 segment */
void MP3__setRemainingPlaytime();                           /**< \brief Calculates and sets the initial play time once when the file opens */

boolean_t MP3__songEnded();                                 /**< \brief Check if the remaining playtime has ended */

void MP3__showEventAndState(EventMaskType ev, MP3__State_t state);  /**< \brief Helper function to show state and event */

static RC_t MP3__openWavFile(uint16_t songIndex);
static void MP3__closeWavFile(void);
static void MP3__readBlock(void);

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

RC_t MP3_Init(void)
{
    MP3__player.m_state       = MP3STATE_ISSTOPPED;
    MP3__player.m_volume      = MP3__MAXVOLUME;
    MP3__player.m_currentSong = 0u;
 
    MP3__reader.m_state          = MP3READER_IDLE;
    MP3__reader.m_newSongRequest = FALSE;
    
    MP3__player.m_noSongs     = 0u;
    MP3__player.m_currentSong = 0u;
 
    // Scan SD card for WAV files
    UART_LOG_PutString("\r\n[MP3] Scanning SD card...\r\n");
    
    FS_Init();
    /* INC 9 — recursive scan from root */
    MP3__scanDir(MP3__SD_ROOT);
    
    SEVEN_Set(SEVEN_1, MP3__player.m_volume);
    SOUND_SetVolume(MP3__player.m_volume * 20u);
    //MP3__showSong();
    /* INC 10 — draw header once here, not inside MP3__showSong */
    MP3__initDisplay();
    
    return RC_SUCCESS;
}

/* INC 9 — draw static header once; call from MP3_Init() only */
void MP3__initDisplay()
{
    TFT_fillScreen(BLACK);
    TFT_setTextSize(1);
    TFT_setTextWrap(FALSE);
    TFT_setColors(RED, BLACK);
    TFT_setCursor(2, 2);
    TFT_println("== MP3 Player ==");
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
                //MP3__showSong();
            }
            else if (nextEvent & ev_Button2)
            {
                MP3__nextSong();
                //MP3__showSong();
            }
            else if (nextEvent & ev_Button3)
            {
                MP3__reader.m_failCount = 0u;               /* INC 7: fresh start (reader is IDLE here) */
                MP3__playSong();
                //MP3__showSong();
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
                if (MP3__reader.m_failCount >= MP3__player.m_noSongs)
                {
                    /* INC 7: every file failed - stop instead of looping forever */
                    UART_LOG_PutString("[MP3] No playable song\r\n");
                    MP3__player.m_state = MP3STATE_ISSTOPPED;
                }
                else
                {
                    MP3__nextSong();
                    MP3__playSong();
                    //MP3__showSong();
                }
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
                MP3__reader.m_failCount++;                  /* INC 7 */
                MP3__reader.m_state = MP3READER_IDLE;
                /* INC 7: player stays PLAYING. MP3__songEnded() is now true, so the
                   next ev_1s in tsk_control moves on to the next song. */
                break;
            }
            MP3__reader.m_failCount = 0u;                   /* INC 7: playable file found */

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

            //sprintf(logBuf, "\r\n[MP3] Underruns before start: %lu\n", SOUND__GetAndClearUnderruns());  // INC 6B.3 DIAG
            //UART_LOG_PutString(logBuf);
            
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
                //sprintf(logBuf, "\r\n[MP3] Underruns in song: %lu\n", SOUND__GetAndClearUnderruns());  // INC 6B.3 DIAG
                //UART_LOG_PutString(logBuf);
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

/* INC 9 - recursive SD card scan, stores full paths for subfolder support */
static void MP3__scanDir(const char * path)
{
    FS_DIR    * pDir;
    FS_DIRENT * pDirEnt;
    char        entryName[32];
    char        fullPath[MP3_FILENAME_MAXLEN];
    U8          attr;
    char        logBuf[80];
    U16         idx = 0u;      /* entries consumed so far in this dir */
    U16         skip;

    pDir = FS_OpenDir(path);
    if (pDir == 0)
    {
        sprintf(logBuf, "[SCAN] Cannot open dir: '%s'\r\n", path);
        UART_LOG_PutString(logBuf);
        return;
    }

    while (MP3__player.m_noSongs < MP3_MAX_SONGS)
    {
        pDirEnt = FS_ReadDir(pDir);
        if (pDirEnt == 0) break;
        idx++;

        FS_DirEnt2Name(pDirEnt, entryName);
        FS_DirEnt2Attr(pDirEnt, &attr);

        if (entryName[0] == '.')   continue;
        if (attr & FS_ATTR_HIDDEN) continue;
        if (attr & FS_ATTR_SYSTEM) continue;

        /* Full path with leading backslash: "\SONG.WAV" or "\MUSIC\SONG.WAV" */
        if (path[0] == '\\' && path[1] == '\0')
            snprintf(fullPath, MP3_FILENAME_MAXLEN, "\\%s", entryName);
        else
            snprintf(fullPath, MP3_FILENAME_MAXLEN, "%s\\%s", path, entryName);
        fullPath[MP3_FILENAME_MAXLEN - 1u] = '\0';

        if (attr & FS_ATTR_DIRECTORY)
        {
            //sprintf(logBuf, "[SCAN] <DIR> '%s'\r\n", fullPath);
            //UART_LOG_PutString(logBuf);

            /* Release the handle BEFORE opening the subdir */
            FS_CloseDir(pDir);
            MP3__scanDir(fullPath);

            /* Reopen the parent and skip what was already consumed */
            pDir = FS_OpenDir(path);
            if (pDir == 0) return;
            for (skip = 0u; skip < idx; skip++)
            {
                if (FS_ReadDir(pDir) == 0) break;
            }
        }
        else
        {
            uint8_t len = (uint8_t)strlen(entryName);
            if (len > 4u &&
                (strcmp(&entryName[len - 4u], ".WAV") == 0 ||
                 strcmp(&entryName[len - 4u], ".wav") == 0))
            {
                strncpy(MP3__songList[MP3__player.m_noSongs],
                        fullPath, MP3_FILENAME_MAXLEN - 1u);
                MP3__songList[MP3__player.m_noSongs][MP3_FILENAME_MAXLEN - 1u] = '\0';
                /*sprintf(logBuf, "[SCAN] [%u] '%s'\r\n",
                        MP3__player.m_noSongs + 1u, fullPath);
                UART_LOG_PutString(logBuf);*/
                MP3__player.m_noSongs++;
            }
        }
    }
    FS_CloseDir(pDir);
    
    if (MP3__player.m_noSongs == 0u)
    {
        UART_LOG_PutString("[MP3] No WAV files found\r\n");
        /* fallback to flash songs if defined */
    }
    else
    {
        char logBuf[40];
        //sprintf(logBuf, "[MP3] %u song(s) found\n", MP3__player.m_noSongs);
        //UART_LOG_PutString(logBuf);
    }
}

/* INC 9 — extract filename from full path for display (TFT/UART) */
static const char * MP3__shortName(const char * fullPath)
{
    const char * p = strrchr(fullPath, '\\');
    return (p != 0) ? (p + 1u) : fullPath;
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
    MP3__player.m_remainingPlayTime  = 0u;
    MP3__reader.m_songIndex          = MP3__player.m_currentSong;
    MP3__reader.m_newSongRequest     = TRUE;

    SetEvent(tsk_reader, ev_BufEmpty);
}

void MP3__stopSong()
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

/*void MP3__showSong()
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
        //UART_LOG_PutString(MP3__songList[i]);
        // INC 9 - show short filename, full path in brackets
        UART_LOG_PutString(MP3__shortName(MP3__songList[i]));
        UART_LOG_PutString("\r\n");
    }
}*/

/* INC 9 — partial refresh: only song rows, header untouched */
void MP3__showSong()
{
    uint16_t i;

    //UART_LOG_PutString("\r\n===================\r\n");

    for (i = 0u; i < MP3__player.m_noSongs; i++)
    {
        // Clear only this row's area — no full screen clear 
        TFT_fillRect(0, 18 + i * 12, CFG_TFT_WIDTH, 11,
                     (i == MP3__player.m_currentSong) ? DARKBLUE : BLACK);

        TFT_setTextSize(1);
        TFT_setTextWrap(FALSE);
        TFT_setColors(
            (i == MP3__player.m_currentSong) ? BLACK : GREEN,
            (i == MP3__player.m_currentSong) ? DARKBLUE : BLACK);

        TFT_setCursor(2, 20 + i * 12);
        TFT_printf("%s %s",
                   (i == MP3__player.m_currentSong) ? "(*)" : "( )",
                   MP3__songList[i]);

        //UART_LOG_PutString((i == MP3__player.m_currentSong) ? "(*) " : "( ) ");
        //UART_LOG_PutString(MP3__songList[i]);
        //UART_LOG_PutString("\r\n");
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
    /* INC 7: a file that failed to open counts as ended, regardless of remaining time */
    return (MP3__player.m_remainingPlayTime == 0u) || (MP3__reader.m_failCount > 0u);
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
    char logBuf[48];
 
    /* INC 9 verify: log the exact path string passed to FS_FOpen */
    //sprintf(logBuf, "\r\n[MP3] FOpen: '%s'\n", MP3__songList[songIndex]);
    //UART_LOG_PutString(logBuf);
    
    MP3__pFile = FS_FOpen(MP3__songList[songIndex], "r");
    if (MP3__pFile == 0)
    {
        UART_LOG_PutString("\r\n[MP3] ERROR: FS_FOpen failed\n");
        return RC_ERROR;
    }
    //UART_LOG_PutString("\r\n[MP3] FS_FOpen OK\n");
 
    if (WAV_ParseHeader(MP3__pFile, &MP3__wavInfo) != RC_SUCCESS)
    {
        FS_FClose(MP3__pFile);
        MP3__pFile = 0;
        UART_LOG_PutString("\r\n[MP3] ERROR: WAV header invalid\n");
        return RC_ERROR;
    }
 
    MP3__bytesRemaining = MP3__wavInfo.data_length;
    
    /* INC 8: WAV_ParseHeader leaves the file position on the first PCM byte. */
    {
        I32      dataPos = FS_FTell(MP3__pFile);
        uint32_t rem     = (dataPos > 0) ? ((uint32_t)dataPos % MP3__SECTOR_SIZE) : 0u;

        MP3__alignBytes = (rem == 0u) ? 0u : (uint16_t)(MP3__SECTOR_SIZE - rem);
        MP3__carryLen   = 0u;
    }
 
    //WAV_PrintHeader(&MP3__wavInfo);
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
    
    MP3__alignBytes     = 0u;                  // INC 8
    MP3__carryLen       = 0u;
}

/**
 * Read one block of PCM, convert every frame to one 8-bit unsigned sample
 * for VDAC8 (MSB of each channel, stereo mixed to mono), fill inactive buffer.
 *
 *   Format         block_align  bytes/block (SOUND_BUFFER_SIZE = 1024)
 *   8-bit  mono         1          1024   (no conversion)
 *   16-bit mono         2          2048
 *   24-bit mono         3          3072
 *   stereo           2 x mono   2 x mono   (L+R averaged)
 */
/**
 * Read one block of PCM, convert to one 8-bit unsigned sample per ISR tick
 * for VDAC8, and fill the inactive sound buffer.
 *   - MSB of each channel (8/16/24-bit)
 *   - stereo mixed to mono (6C)
 *   - 44100 Hz: 2 input frames averaged per output sample (6D)
 */
static void MP3__readBlock(void)
{
    uint16_t blockAlign = MP3__wavInfo.header.block_align;                     /* bytes per input frame, 1..6  */
    uint8_t  channels   = (uint8_t)MP3__wavInfo.header.number_of_channels;     /* 1 or 2                       */
    uint8_t  bytesPerCh = (uint8_t)(MP3__wavInfo.header.bits_per_sample / 8u); /* 1..3                         */
    uint8_t  msbOff     = bytesPerCh - 1u;                                     /* MSB is the last byte (LE)    */
    uint8_t  step       = (uint8_t)(MP3__wavInfo.header.sample_rate / MP3__OUTPUT_RATE); /* 1 or 2, validated  */
    uint16_t inBytes    = blockAlign * step;                                   /* input bytes per output sample */
    int16_t  divisor    = (int16_t)(channels * step);                          /* values averaged per sample   */
    uint32_t maxBytes   = (uint32_t)SOUND_BUFFER_SIZE * inBytes;
    uint32_t toRead;
    U32      bytesRead;
    uint16_t samples;
    uint16_t i;
    uint32_t total;

        /* INC 6A: never read more than the buffer can hold */
    if (maxBytes > MP3__MAX_READ)
    {
        maxBytes = MP3__MAX_READ;
    }
    /* INC 6D: read whole output samples only */
    maxBytes -= (maxBytes % inBytes);

    /* INC 8: first read stops on a sector boundary, later reads are maxBytes (multiple of 512) */
    if (MP3__alignBytes != 0u)
    {
        toRead          = MP3__alignBytes;
        MP3__alignBytes = 0u;
    }
    else
    {
        toRead = maxBytes;
    }
    if (toRead > MP3__bytesRemaining)
    {
        toRead = MP3__bytesRemaining;
    }

    /* INC 8: put back the partial output sample kept from the previous call */
    if (MP3__carryLen > 0u)
    {
        memcpy(MP3__readBuf, MP3__carry, MP3__carryLen);
    }

    DBG_HI(DBG_FS); // Shows total SD read time
    bytesRead = FS_Read(MP3__pFile, &MP3__readBuf[MP3__carryLen], toRead);
    DBG_LO(DBG_FS);
    
    if (bytesRead == 0u)
    {
        MP3__bytesRemaining = 0u;          /* read error -> force EOF, avoid endless loop */
        MP3__carryLen       = 0u;
        return;
    }
    MP3__bytesRemaining -= (uint32_t)bytesRead;

    total         = (uint32_t)MP3__carryLen + bytesRead;
    samples       = (uint16_t)(total / inBytes);
    MP3__carryLen = (uint8_t)(total - ((uint32_t)samples * inBytes));
    
    if (MP3__carryLen > 0u)
    {
        memcpy(MP3__carry, &MP3__readBuf[(uint32_t)samples * inBytes], MP3__carryLen);
    }

    DBG_HI(DBG_CONV); // Shows the conversion loop run/compute time
    if (inBytes > 1u)                      /* 8-bit mono 22050 Hz needs no conversion */
    {
        for (i = 0u; i < samples; i++)
        {
            /* N2: in place is safe - all input bytes of sample i are read before index i is written */
            uint8_t const * const src = &MP3__readBuf[(uint32_t)i * inBytes];
            int16_t acc = 0;
            uint8_t f;
            uint8_t c;

            for (f = 0u; f < step; f++)            /* N4: 1 frame at 22050 Hz, 2 frames at 44100 Hz */
            {
                for (c = 0u; c < channels; c++)    /* N3: 1 or 2 channels */
                {
                    uint8_t msb = src[(f * blockAlign) + (c * bytesPerCh) + msbOff];

                    /* N1: to signed -128..127 */
                    acc += (bytesPerCh == 1u) ? ((int16_t)msb - 128) : (int16_t)(int8_t)msb;
                }
            }

            MP3__readBuf[i] = (uint8_t)((acc / divisor) + 128);
        }
    }
    DBG_LO(DBG_CONV);

    if (samples > 0u)
    {
        DBG_HI(DBG_FILL); // Shows copy and padding time
        SOUND_FillBuffer(MP3__readBuf, samples);
        DBG_LO(DBG_FILL);
    }
    DBG_LO(DBG_DEAD); // Shows the actual deadline margin (from buffer swap in the ISR until readBlock returns)
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
* N3 - Stereo is mixed to mono by averaging the signed L and R MSBs. Averaging (not summing)
*      keeps the result in -128..127, so it cannot clip. C integer division truncates toward 0.
*  
* N4 - 44100 Hz is decimated by 2: each output sample averages 2 consecutive input frames
*      (a simple low-pass that reduces aliasing vs. dropping every 2nd frame). The read size
*      is rounded down to whole output samples so a frame pair never splits across blocks.
* 
* Time:
* SPI       SCLK                4MHz
* DBG_FS    FS_Read             9.2ms   19.5% of 47.06ms (shared)
*               SPI             8.9ms
*               other           0.3
* DBG_FILL  SOUND_FillBuffer    0.3ms   1.00% of 47.06ms (shared)
* DBG_DEAD  swap to refill done 9.7ms   20.6% of 47.06ms (shared)
* 
* Refil period: 47.06
* Free time: 47.06 − 9.7 ≈ 37.4 ms (about 79%) per refill.
* 
* 
* SEGGER - Forum says “DMA is heavily used and preferred” for SD card mode on PSoC 5LP. That 
* is true for PSoC 6 with its dedicated SDHC peripheral, where the hardware block has DMA 
* built in. On PSoC 5LP specifically, there is no dedicated SD/MMC hardware block. We are 
* using the SPIM component over a general-purpose SPI bus. The Cypress AppNote AN73854 
* (the standard reference implementation for emFile + PSoC 5LP) implements FS_MMC_HW_X_Read 
* as a CPU polling loop over the SPIM RX FIFO - not DMA. DMA is possible with the PSoC 5LP 
* DMA controller, but it requires custom HAL work and is not the default out-of-the-box 
* configuration. So for we specific hardware, polling in the 512-byte read is the most 
* likely reality unless we explicitly added DMA to your HAL.
*
> The ISR fires once per sample, every 1 ÷ 22050 Hz = 45.35 µs, and outputs one sample to the PWM.
> A buffer holds 1024 samples, so playing it takes 1024 × 45.35 µs ≈ 46.4 ms. We measured 47.06 ms.
> So 47 ms is just 1024 samples at 22.05 kHz, the time the ISR needs to run through one buffer.
> The reader takes about 9.7 ms to refill the buffer inactive buffer.
*
*Format             Bytes per refill    Sectors     Samples per refill      SD time now         Share of budget     Free
 C8M 22k            1024	            2	        1024	                9.2 ms (measured)	20%	                80%
 
 C16M, C8S 22k;     2048	            4	        1024	                16.6 ms	            35%	                65% 	
 C8M 44k 

 C24M 22k	        3072	            6	        1024	                24.0 ms	            51%	                49%
 
 C16S 22k;          4096	            8	        1024	                31.4 ms	            67%	                33%
 C16M, C8S 44k	
 
 C24S 22k;          6144	            12	        1024	                46.2 ms	            98%	                2%
 C24M 44k	
 
 C16S 44k	        6144	            12	        768	                    46.2 ms	            131% of 35 ms	    underrun
 
 C24S 44k	        6144	            12	        512	                    46.2 ms	            197% of 23.5 ms	    underrun
*
*/