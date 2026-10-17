/**
* \file    mp3.c
* \author  P. Fromm (template), V S Agilan (player)
* \date    6.11.18 (INC 11B: 2026-10)
*
* \brief   A simple MP3 Player (WAV player) application layer (state machine, SD card scan,
*          PCM streaming and conversion, TFT song list)
*
* \details
* See mp3.h for the overview. This file contains four parts:
*
*   1. Player state machine   MP3_ProcessEvent()  (tsk_control)
*   2. Reader state machine   MP3_Reader()        (tsk_reader)
*   3. SD card scan           MP3__scanSD()       (tsk_init, subfolders included)
*   4. TFT song list          MP3__showSong()     (tsk_display)
*
* Design notes N1..N10 are collected at the end of this file. A marker like
* "N4" in a comment means: read note N4 at the end for the reason.
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

#include <string.h>         /* memcpy(), strlen(), strcmp() - no <stdio.h>, see N10 */

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

/** \name Player
 *  \{ */
#define MP3__MAXVOLUME          (5u)        /**< \brief highest volume step                                   */
#define MP3__VOLUME_STEP        (20u)       /**< \brief percent per volume step: 5 steps * 20 = 100 %         */
#define MP3__BUTTON_MASK        (ev_Button1 | ev_Button2 | ev_Button3 | ev_Button4)  /**< \brief all button events                               */
#define MP3__EVENT_MASK         (MP3__BUTTON_MASK | ev_1s)                           /**< \brief all events the player state machine knows (N11) */
/** \} */

/** \name SD card scan (N6)
 *  \{ */
#define MP3__PATH_SEP           ('\\')                      /**< \brief path separator of emFile                        */
#define MP3__SCAN_MAX_DEPTH     (8u)                        /**< \brief deepest folder level that is scanned (root = 0) */
#define MP3__SCAN_LEVELS        (MP3__SCAN_MAX_DEPTH + 1u)  /**< \brief number of levels: root + subfolder levels       */
#ifdef FS_MAX_PATH
#define MP3__ENTRY_NAME_LEN     (FS_MAX_PATH)               /**< \brief size of the directory entry name buffer         */
#else
#define MP3__ENTRY_NAME_LEN     (260u)                      /**< \brief size of the directory entry name buffer         */
#endif
/** \} */

/** \name Streaming (N1..N5)
 *  \{ */
#define MP3__MAX_BLOCK_ALIGN    (6u)        /**< \brief largest supported frame: 24-bit stereo = 2 ch * 3 bytes      */
#define MP3__OUTPUT_RATE        (22050u)    /**< \brief SOUND_SAMPLE ISR rate - one output sample per tick           */
#define MP3__SECTOR_SIZE        (512u)      /**< \brief SD sector size - reads are kept sector-aligned (INC 8, N5)   */
#define MP3__MAX_INBYTES        (12u)       /**< \brief largest input bytes per output sample: 24-bit stereo, step 2 */
#define MP3__MAX_AVG            (4u)        /**< \brief most values averaged into one output sample: 2 ch * step 2   */
#define MP3__MAX_READ           ((uint32_t)MP3__MAX_BLOCK_ALIGN * SOUND_BUFFER_SIZE)  /**< \brief largest raw read per block */
/** \} */

/** \name TFT layout in pixels (N9)
 *  \{ */
#define MP3__TEXT_SIZE          (1u)        /**< \brief TFT text size 1 = 6x8 pixel characters                */
#define MP3__HEADER_X           (2)         /**< \brief header text position                                  */
#define MP3__HEADER_Y           (2)
#define MP3__TEXT_X             (2)         /**< \brief x of the song text                                    */
#define MP3__ROW0_RECT_Y        (18)        /**< \brief y of the clear rectangle of row 0                     */
#define MP3__ROW0_TEXT_Y        (20)        /**< \brief y of the text of row 0                                */
#define MP3__ROW_PITCH          (12)        /**< \brief distance from one row to the next                     */
#define MP3__ROW_RECT_H         (11)        /**< \brief height of the clear rectangle of a row                */
#define MP3__ROW_PREFIX_LEN     (4u)        /**< \brief length of "(*) " or "( ) "                            */
#define MP3__ROW_TEXT_LEN       (MP3__ROW_PREFIX_LEN + MP3_FILENAME_MAXLEN)   /**< \brief prefix + path + NUL */
/** \} */

/** \name TFT content shown right now (shadow state, N9)
 *  \{ */
#define MP3__DISP_NONE          (0xFFFFu)   /**< \brief "nothing drawn yet" - forces the first full draw      */
#define MP3__DISP_MODE_NONE     (0xFFu)     /**< \brief nothing drawn yet                                     */
#define MP3__DISP_MODE_LIST     (0u)        /**< \brief the song list                                         */
#define MP3__DISP_MODE_NOSONGS  (1u)        /**< \brief message "No Song Detected :("                         */
#define MP3__DISP_MODE_ERROR    (2u)        /**< \brief message "Error: See UART Log"                         */
/** \} */

/** Set to 1 to compile the event/state trace MP3__showEventAndState() (N10). */
#define MP3__DEBUG_LOG          (0u)

/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/

/** \brief States of the player (tsk_control). */
enum eMP3__State
{
    MP3STATE_ISSTOPPED,                     /**< \brief no song is played, a song can be selected */
    MP3STATE_ISPLAYING,                     /**< \brief a song is played                          */
    MP3STATE_ISPAUSING,                     /**< \brief the song is paused                        */
};
typedef enum eMP3__State MP3__State_t;

/** \brief States of the reader (tsk_reader). */
enum eMP3__ReaderState
{
    MP3READER_IDLE,                         /**< \brief nothing open, waits for a song request    */
    MP3READER_OPEN,                         /**< \brief transient: open file, pre-fill buffers    */
    MP3READER_STREAMING,                    /**< \brief refill the inactive buffer on every run   */
    MP3READER_EOF,                          /**< \brief last block read, stop and close           */
};
typedef enum eMP3__ReaderState MP3__ReaderState_t;

/** \brief Player data. Fields used by more than one task are volatile (N7). */
typedef struct
{
    MP3__State_t      m_state;              /**< \brief state of the state machine (tsk_control only)           */
    uint16_t          m_noSongs;            /**< \brief number of songs in the list, written by MP3_Init only   */
    volatile uint16_t m_currentSong;        /**< \brief selected song 0..m_noSongs-1 (tsk_control, tsk_display) */
    uint8_t           m_volume;             /**< \brief volume step 0..MP3__MAXVOLUME                           */
    volatile uint16_t m_remainingPlayTime;  /**< \brief remaining play time in s (tsk_control, tsk_reader)      */
} MP3__Player_t;

/** \brief Reader data. Shared between tsk_control and tsk_reader, therefore volatile (N7). */
typedef struct
{
    volatile MP3__ReaderState_t m_state;            /**< \brief state of the reader                             */
    volatile uint16_t           m_songIndex;        /**< \brief song to open on the next request                */
    volatile boolean_t          m_newSongRequest;   /**< \brief set by MP3__playSong, cleared by the reader     */
    volatile uint16_t           m_failCount;        /**< \brief INC 7: consecutive files that failed to open    */
} MP3__Reader_t;

/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

static MP3__Player_t MP3__player = {MP3STATE_ISSTOPPED, 0u, 0u, 0u, 0u};
static MP3__Reader_t MP3__reader = {MP3READER_IDLE, 0u, FALSE, 0u};

static char MP3__songList[MP3_MAX_SONGS][MP3_FILENAME_MAXLEN];	/**< \brief Song path list built from the SD card scan, e.g. "\MUSIC\SONG.WAV".                              */

static char MP3__entryName[MP3__ENTRY_NAME_LEN];				/**< \brief Name of the directory entry just read. static: FS_MAX_PATH bytes are too big for the stack (N6). */

static char     MP3__scanPath[MP3_FILENAME_MAXLEN];				/**< \brief Path of the folder that is scanned right now. The path of every upper level is the beginning of it (N6). Also in MP3__scanLen. */
static uint8_t  MP3__scanLen[MP3__SCAN_LEVELS];					/**< \brief Length of the folder path of each scan level (N6). */
static U16      MP3__scanIdx[MP3__SCAN_LEVELS];					/**< \brief Entries already read in the folder of each scan level, to skip them after a reopen (N6). */

static FS_FILE * MP3__pFile = NULL;								/**< \brief emFile handle of the open song, static so it is not on the tsk_reader stack. */

static WAVFile  MP3__wavInfo;									/**< \brief Decoded header of the open song. */

static uint8_t  MP3__readBuf[MP3__MAX_READ + MP3__MAX_INBYTES];	/**< \brief One raw block of the largest frame format + MP3__MAX_INBYTES in front, so a carried partial frame fits before a full-size read (INC 8, N5). Kept off the tsk_reader stack. */

static uint32_t MP3__bytesRemaining = 0u;						/**< \brief Remaining PCM bytes of the open song that are not read yet. */

static uint16_t MP3__alignBytes = 0u;							/**< \brief INC 8: raw bytes up to the next sector boundary, 0 = aligned (N5). */
static uint8_t  MP3__carry[MP3__MAX_INBYTES];					/**< \brief INC 8: partial output sample left over after a read (N5). */
static uint8_t  MP3__carryLen = 0u;								/**< \brief INC 8: number of valid bytes in MP3__carry. */

static uint16_t MP3__dispCount = MP3__DISP_NONE;				/**< \brief INC 10B: number of song rows currently drawn on the TFT (N9). */
static uint16_t MP3__dispSong  = MP3__DISP_NONE;				/**< \brief INC 10B: row currently drawn as selected "(*)" (N9). */
static uint8_t  MP3__dispMode  = MP3__DISP_MODE_NONE;			/**< \brief INC 10C: kind of content on the TFT, MP3__DISP_MODE_* (N9). */
static volatile boolean_t MP3__error = FALSE;					/**< \brief INC 10C: system error flag. Set by any task, cleared by a button press (N8). */

/**< \brief Text of one list row, built before it is sent to the TFT (N9). */
static char MP3__rowText[MP3__ROW_TEXT_LEN];

/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

/* Player state machine helpers (tsk_control) */
static void MP3__nextSong(void);                                                                                      /**< \brief select the next song, go to the first one at the end of the list         */
static void MP3__prevSong(void);                                                                                      /**< \brief select the previous song, go to the last one at the start of the list    */
static void MP3__applyVolume(void);                                                                                   /**< \brief show the volume on the 7 segment display and give it to the sound driver */
static void MP3__increaseVolume(void);                                                                                /**< \brief increase the volume by one step                                          */
static void MP3__decreaseVolume(void);                                                                                /**< \brief decrease the volume by one step                                          */
static void MP3__playSong(void);                                                                                      /**< \brief request the reader to play the selected song                             */
static void MP3__stopSong(void);                                                                                      /**< \brief stop the sound timer, close the file, send the reader back to IDLE       */
static void MP3__pauseSong(void);                                                                                     /**< \brief pause: stop the sound timer, keep file and position                      */
static void MP3__resumeSong(void);                                                                                    /**< \brief resume after pause: start the sound timer again                          */
static void MP3__showRemainingPlaytime(void);                                                                         /**< \brief count the remaining play time down by one second and show it (7 segment) */
static void MP3__setRemainingPlaytime(void);                                                                          /**< \brief calculate and show the play time of the open song once                   */
static boolean_t MP3__songEnded(void);                                                                                /**< \brief check if the remaining play time has ended or the last open failed       */
 
/* Reader helpers (tsk_reader) */
static RC_t MP3__openWavFile(uint16_t const songIndex);                                                               /**< \brief open a song and decode its WAV header (N5, N11)                            */
static void MP3__closeWavFile(void);                                                                                  /**< \brief close the open song and reset the streaming state                          */
static void MP3__readBlock(void);                                                                                     /**< \brief read one PCM block, convert it and fill the inactive sound buffer (N1..N5) */
static void MP3__convertBlock(uint16_t const samples);                                                                /**< \brief convert raw PCM frames to unsigned 8 bit samples, in place (N1..N4)        */
 
/* SD card scan (tsk_init) */
static boolean_t MP3__isWavName(const char * const name);                                                             /**< \brief is the file name a WAV file (".WAV" or ".wav")?                          */
static boolean_t MP3__hasAttr(U8 const attr, uint32_t const flag);                                                    /**< \brief does an emFile attribute byte contain a flag?                            */
static RC_t MP3__buildPath(char * const dst, size_t const dstSize, const char * const dir, const char * const name);  /**< \brief build "dir\name" with a length check (N6, N11)                           */
static void MP3__logPath(const char * const prefix, const char * const path);                                         /**< \brief write "prefix + path + '" to the UART log (N10)                          */
static const char * MP3__shortName(const char * const fullPath);                                                      /**< \brief file name of a full path, for the display                                */
static FS_DIR * MP3__scanReopen(uint8_t const level);                                                                 /**< \brief reopen the folder of a scan level, skip the entries already read (N6)    */
static RC_t MP3__scanSD(void);                                                                                        /**< \brief scan the SD card from the root, fill the song list (N6, N11)             */
 
/* TFT (tsk_init, tsk_display) */
static void MP3__initDisplay(void);                                                                                   /**< \brief clear the screen and draw the static header once                         */
static void MP3__clearRow(uint16_t const i);                                                                          /**< \brief clear the area of one list row                                           */
static void MP3__drawRow(uint16_t const i, uint16_t const selected);                                                  /**< \brief INC 10B: draw ONE song row (N9)                                          */
static void MP3__drawMessage(uint8_t const mode);                                                                     /**< \brief INC 10C: "No Song Detected :(" / "Error: See UART Log"                   */
static void MP3__switchMode(uint8_t const mode);                                                                      /**< \brief kind of content changed: wipe the old rows, prepare the new content (N9) */
static void MP3__updateList(uint16_t const cur, uint16_t const no);                                                   /**< \brief redraw all rows or only the old and the new selected row (N9)            */
 
#if (MP3__DEBUG_LOG != 0u)
static void MP3__showEventAndState(EventMaskType const ev, MP3__State_t const state);	                              /**< \brief debug aid: print state and event as text via UART (N10)                  */
#endif

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/* ------------------------------------------------------------------------- */
/* Public API                                                                */
/* ------------------------------------------------------------------------- */


/**
 * \brief Initialise the player: scan the SD card, set the start volume, draw the TFT header.
 *
 * Resets player and reader to STOPPED / IDLE with the first song selected, calls
 * FS_Init() and scans the card from the root (MP3__scanSD, N6). A scan problem
 * sets the error flag (N8). Writes "No WAV files found" once if nothing was found.
 *
 * Call once from tsk_init after TFT_init() and SOUND_Init().
 *
 * \return RC_SUCCESS when the card was scanned (also with 0 songs found),
 *         RC_ERROR_OPEN when a folder could not be opened, the list may be incomplete
 *         (UART line written, error shown on the TFT)
 */
RC_t MP3_Init(void)
{
    RC_t rc;
 
    MP3__player.m_state       = MP3STATE_ISSTOPPED;
    MP3__player.m_volume      = MP3__MAXVOLUME;
    MP3__player.m_currentSong = 0u;
    MP3__player.m_noSongs     = 0u;
 
    MP3__reader.m_state          = MP3READER_IDLE;
    MP3__reader.m_newSongRequest = FALSE;
 
    /* Scan SD card for WAV files */
    UART_LOG_PutString("\r\n[MP3] Scanning SD card...\r\n");
 
    FS_Init();
    rc = MP3__scanSD();                             /* INC 9: scan from the root, subfolders included (N6, N11) */
 
    /* Reported once here, not at the end of every folder (N6). An error
       was already reported by the scan itself. */
    if ((MP3__player.m_noSongs == 0u) && (MP3__error == FALSE))
    {
        UART_LOG_PutString("[MP3] No WAV files found\r\n");
    }
 
    MP3__applyVolume();
    MP3__initDisplay();                             /* INC 10: header is drawn once, here */
 
    return rc;                                      /* N11: RC_SUCCESS also for an empty card */
}
 
/**
 * \brief Player state machine, processes ONE event (tsk_control).
 *
 * Events, actions (function calls) and state changes are all visible in the
 * switch below. While stopped without songs all events are ignored. Any button
 * press clears a displayed error (N8).
 *
 * \param EventMaskType nextEvent    : [IN] single event bit: ev_Button1..ev_Button4 or ev_1s
 * \return RC_SUCCESS when the event was processed (an event without a function in the
 *         current state counts as processed),
 *         RC_ERROR_BAD_PARAM when no known event bit is set (nothing happens),
 *         RC_ERROR_INVALID_STATE when stopped without songs (event ignored, N8)
 */
RC_t MP3_ProcessEvent(EventMaskType nextEvent)
{
    RC_t rc = RC_SUCCESS;
 
#if (MP3__DEBUG_LOG != 0u)
    MP3__showEventAndState(nextEvent, MP3__player.m_state);
#endif
 
    if ((nextEvent & MP3__EVENT_MASK) == 0u)
    {
        rc = RC_ERROR_BAD_PARAM;                    /* N11: not one of the events of this state machine, nothing to do */
    }
    else if ((MP3__player.m_noSongs != 0u) || (MP3__player.m_state != MP3STATE_ISSTOPPED))
    {
        /* INC 10C: any button press acknowledges a shown error (N8) */
        if (nextEvent & MP3__BUTTON_MASK)
        {
            MP3__error = FALSE;
        }
 
        switch (MP3__player.m_state)
        {
            case MP3STATE_ISSTOPPED:
                if (nextEvent & ev_Button1)
                {
                    MP3__prevSong();
                }
                else if (nextEvent & ev_Button2)
                {
                    MP3__nextSong();
                }
                else if (nextEvent & ev_Button3)
                {
                    MP3__reader.m_failCount = 0u;               /* INC 7: fresh start (reader is IDLE here) */
                    MP3__playSong();
                    MP3__player.m_state = MP3STATE_ISPLAYING;
                }
                else
                {
                    /* Button4 and ev_1s have no function while stopped */
                }
                break;
 
            case MP3STATE_ISPLAYING:
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
                        MP3__error          = TRUE;             /* INC 10C */
                        MP3__player.m_state = MP3STATE_ISSTOPPED;
                    }
                    else
                    {
                        MP3__nextSong();
                        MP3__playSong();
                    }
                }
                else
                {
                    /* other events are ignored */
                }
                break;
 
            case MP3STATE_ISPAUSING:
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
                else
                {
                    /* other events are ignored */
                }
                break;
 
            default:
                /* not reachable: m_state only gets the values above */
                break;
        }
    }
    else
    {
        /* INC 10C: nothing to play -> ignore all events. This also protects
           the "% m_noSongs" in MP3__nextSong() against a division by zero (N8). */
        rc = RC_ERROR_INVALID_STATE;                /* N11 */
    }
 
    return rc;
}

/**
 * \brief Run the reader state machine one step (tsk_reader, once per ev_BufEmpty).
 *
 * IDLE accepts a song request, OPEN opens the file and starts the sound,
 * STREAMING refills the inactive buffer, EOF stops and closes (N7). Like the player
 * state machine, all actions and state changes are written out in the switch.
 * The function returns after one step; the WaitEvent() loop is in main.c.
 *
 * Call only from TASK(tsk_reader). No return code, see N11.
 *
 */
void MP3_Reader(void)
{
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
            /* fall through - open the new song with the same event (N10, Rule 16.3) */
        }

        case MP3READER_OPEN:
        {
            if (MP3__openWavFile(MP3__reader.m_songIndex) != RC_SUCCESS)
            {
                MP3__reader.m_failCount = (uint16_t)(MP3__reader.m_failCount + 1u);     /* INC 7 */
                MP3__reader.m_state     = MP3READER_IDLE;
                /* INC 7: player stays PLAYING. MP3__songEnded() is now true, so the
                   next ev_1s in tsk_control moves on to the next song. */
            }
            else
            {
                MP3__reader.m_failCount = 0u;               /* INC 7: playable file found */

                MP3__setRemainingPlaytime();

                MP3__readBlock();                           /* INC 5C: pre-fill buffer A, becomes active */
                (void)SOUND_SwapBuffers();

                if (MP3__bytesRemaining > 0u)
                {
                    MP3__readBlock();                       /* pre-fill buffer B, inactive, ready for the first swap */
                }

                (void)SOUND_StartPlay();

                MP3__reader.m_state = MP3READER_STREAMING;
            }
            break;
        }

        case MP3READER_STREAMING:
        {
            if (MP3__reader.m_newSongRequest == TRUE)
            {
                /* INC 5E: new song request - leave the current song, IDLE handles it */
                (void)SOUND_StopPlay();
                MP3__closeWavFile();
                MP3__reader.m_state = MP3READER_IDLE;
                (void)SetEvent(tsk_reader, ev_BufEmpty);    /* run the reader again right now */
            }
            else if (MP3__bytesRemaining == 0u)
            {
                MP3__reader.m_state = MP3READER_EOF;        /* all data read, EOF is handled on the next run (N7) */
            }
            else
            {
                MP3__readBlock();       /* INC 5C: fill the inactive buffer while the ISR plays the active one */
            }
            break;
        }

        case MP3READER_EOF:
        {
            (void)SOUND_StopPlay();
            MP3__closeWavFile();

            MP3__player.m_remainingPlayTime = 0u;
            SEVEN_Set(SEVEN_0, 0u);

            MP3__reader.m_state = MP3READER_IDLE;

            /* INC 5E: the event that brought us here may be a new song request
               (from MP3__playSong) - let IDLE handle it right away */
            if (MP3__reader.m_newSongRequest == TRUE)
            {
                (void)SetEvent(tsk_reader, ev_BufEmpty);
            }
            break;
        }

        default:
            /* not reachable: m_state only gets the values above */
            MP3__reader.m_state = MP3READER_IDLE;
            break;
    }
}

/**
 * \brief Update the TFT: song list, "No Song Detected :(" or "Error: See UART Log" (tsk_display).
 *
 * Decides the content from the error flag and the number of songs, then redraws
 * only what changed against the shadow state (N9): everything on a mode or
 * song-count change, otherwise the old and the new selected row.
 *
 * Call only from TASK(tsk_display). Public although the name has "__" (main.c uses it).
 *
 */
void MP3__showSong(void)
{
    /* Snapshot the shared values once, the player task may change them any time (N7) */
    uint16_t const cur  = MP3__player.m_currentSong;
    uint16_t const no   = MP3__player.m_noSongs;
    uint8_t  mode;

    if (MP3__error == TRUE)
    {
        mode = MP3__DISP_MODE_ERROR;
    }
    else if (no == 0u)
    {
        mode = MP3__DISP_MODE_NOSONGS;
    }
    else
    {
        mode = MP3__DISP_MODE_LIST;
    }

    if (mode != MP3__dispMode)
    {
        MP3__switchMode(mode);                      /* kind of content changed (N9) */
    }

    if (mode == MP3__DISP_MODE_LIST)
    {
        MP3__updateList(cur, no);
    }
}

/* ------------------------------------------------------------------------- */
/* Player state machine (tsk_control)                                        */
/* ------------------------------------------------------------------------- */

/**
 * \brief Select the next song, wrap to the first one at the end of the list.
 *
 * Does nothing if the list is empty (zero-song guard, N8).
 *
 */
static void MP3__nextSong(void)
{
    uint16_t const no = MP3__player.m_noSongs;

    if (no != 0u)   /* zero-song guard */
    {
        MP3__player.m_currentSong = (uint16_t)(((uint32_t)MP3__player.m_currentSong + 1u) % no);
    }
}

/**
 * \brief Select the previous song, wrap to the last one at the start of the list.
 *
 * Does nothing if the list is empty (zero-song guard, N8).
 *
 */
static void MP3__prevSong(void)
{
    uint16_t const no  = MP3__player.m_noSongs;
    uint16_t const cur = MP3__player.m_currentSong;

    if (no != 0u)   /* zero-song guard */
    {
        MP3__player.m_currentSong = (cur == 0u) ? (uint16_t)(no - 1u) : (uint16_t)(cur - 1u);
    }
}

/**
 * \brief Show the volume on the 7 segment display and give it to the sound driver.
 *
 * The sound driver gets volume step * MP3__VOLUME_STEP percent (5 steps * 20 = 100 %).
 *
 */
static void MP3__applyVolume(void)
{
    uint8_t const vol = MP3__player.m_volume;

    SEVEN_Set(SEVEN_1, vol);
    (void)SOUND_SetVolume((uint8_t)(vol * MP3__VOLUME_STEP));
}

/**
 * \brief Volume one step up, limited to MP3__MAXVOLUME.
 */
static void MP3__increaseVolume(void)
{
    if (MP3__player.m_volume < MP3__MAXVOLUME)
    {
        MP3__player.m_volume++;
    }
    MP3__applyVolume();
}

/**
 * \brief Volume one step down, limited to 0.
 */
static void MP3__decreaseVolume(void)
{
    if (MP3__player.m_volume > 0u)
    {
        MP3__player.m_volume--;
    }
    MP3__applyVolume();
}

/**
 * \brief Request the reader to play the selected song.
 *
 * Sets the song index and the request flag and wakes the reader with ev_BufEmpty.
 * The reader opens the file (N7).
 *
 */
static void MP3__playSong(void)
{
    MP3__player.m_remainingPlayTime = 0u;
    MP3__reader.m_songIndex         = MP3__player.m_currentSong;
    MP3__reader.m_newSongRequest    = TRUE;

    (void)SetEvent(tsk_reader, ev_BufEmpty);        /* wake the reader */
}

/**
 * \brief Stop the sound timer, close the file and send the reader back to IDLE.
 *
 * Runs in tsk_control; see N7 for the condition under which this is safe.
 *
 */
static void MP3__stopSong(void)
{
    (void)SOUND_StopPlay();
    MP3__closeWavFile();
    MP3__reader.m_state = MP3READER_IDLE;
}

/**
 * \brief Pause: stop the sound timer, keep file and position.
 */
static void MP3__pauseSong(void)
{
    (void)SOUND_StopPlay();
}

/**
 * \brief Resume after pause: start the sound timer again.
 */
static void MP3__resumeSong(void)
{
    (void)SOUND_StartPlay();
}

/**
 * \brief Count the remaining play time down by one second and show it (called every second while playing).
 */
static void MP3__showRemainingPlaytime(void)
{
    uint16_t remaining = MP3__player.m_remainingPlayTime;

    if (remaining > 0u)
    {
        remaining--;
    }
    MP3__player.m_remainingPlayTime = remaining;
    SEVEN_Set(SEVEN_0, remaining);
}

/**
 * \brief Calculate the play time of the open song once and show it.
 *
 * seconds = data bytes / (samples per second * bytes per frame), limited to 65535 s.
 *
 */
static void MP3__setRemainingPlaytime(void)
{
    uint32_t const bytesPerSec = MP3__wavInfo.header.sample_rate * (uint32_t)MP3__wavInfo.header.block_align;
    uint32_t seconds = 0u;

    if (bytesPerSec != 0u)
    {
        seconds = MP3__wavInfo.data_length / bytesPerSec;
    }
    if (seconds > 0xFFFFu)
    {
        seconds = 0xFFFFu;
    }

    MP3__player.m_remainingPlayTime = (uint16_t)seconds;
    SEVEN_Set(SEVEN_0, (uint16_t)seconds);
}

/**
 * \brief Has the song ended (or did it fail to open)?
 * \return TRUE when no play time is left or the last open failed (INC 7),
 *         FALSE otherwise
 */
static boolean_t MP3__songEnded(void)
{
    boolean_t ended = FALSE;

    if ((MP3__player.m_remainingPlayTime == 0u) || (MP3__reader.m_failCount > 0u))
    {
        ended = TRUE;
    }
    return ended;
}

/* ------------------------------------------------------------------------- */
/* Reader helpers (tsk_reader)                                               */
/* ------------------------------------------------------------------------- */

/**
 * \brief Open a song and decode its WAV header.
 *
 * On success MP3__bytesRemaining and the sector alignment (N5) are set up. On
 * failure the file is closed, a line is written to the UART and the error flag
 * is set (N8). The return codes are described in N11.
 *
 * \param uint16_t const songIndex   : [IN] index into MP3__songList, 0..m_noSongs-1
 * \return RC_SUCCESS when the file is open, the header valid and everything is ready to stream,
 *         RC_ERROR_RANGE when songIndex is not in the list (nothing is opened),
 *         RC_ERROR_OPEN when FS_FOpen() failed,
 *         the code of WAV_ParseHeader() when the header is not usable (RC_ERROR_READ_FAILS,
 *         RC_ERROR_INVALID or RC_ERROR_NOT_IMPLEMENTED), the file is closed again
 */
static RC_t MP3__openWavFile(uint16_t const songIndex)
{
    RC_t rc = RC_ERROR_RANGE;                               /* N11: songIndex is not in the list */
 
    if (songIndex < MP3__player.m_noSongs)
    {
        MP3__pFile = FS_FOpen(MP3__songList[songIndex], "r");
 
        if (MP3__pFile == NULL)
        {
            UART_LOG_PutString("\r\n[MP3] ERROR: FS_FOpen failed\n");
            MP3__error = TRUE;                              /* INC 10C */
            rc         = RC_ERROR_OPEN;
        }
        else
        {
            rc = WAV_ParseHeader(MP3__pFile, &MP3__wavInfo);    /* N11: the reason of the parser is passed on */
 
            if (rc != RC_SUCCESS)
            {
                (void)FS_FClose(MP3__pFile);
                MP3__pFile = NULL;
                UART_LOG_PutString("\r\n[MP3] ERROR: WAV header invalid\n");
                MP3__error = TRUE;                          /* INC 10C */
            }
            else
            {
                I32      const dataPos = FS_FTell(MP3__pFile);
                uint32_t const rem     = (dataPos > 0) ? ((uint32_t)dataPos % MP3__SECTOR_SIZE) : 0u;
 
                MP3__bytesRemaining = MP3__wavInfo.data_length;
 
                /* INC 8 / N5: WAV_ParseHeader() leaves the file position on the
                   first PCM byte. Distance from there to the next sector boundary: */
                MP3__alignBytes = (rem == 0u) ? 0u : (uint16_t)(MP3__SECTOR_SIZE - rem);
                MP3__carryLen   = 0u;
            }
        }
    }
 
    return rc;
}

/**
 * \brief Close the open song (if any) and reset the streaming state.
 */
static void MP3__closeWavFile(void)
{
    if (MP3__pFile != NULL)
    {
        (void)FS_FClose(MP3__pFile);
        MP3__pFile = NULL;
    }
    MP3__bytesRemaining = 0u;

    MP3__alignBytes     = 0u;                       /* INC 8 */
    MP3__carryLen       = 0u;
}

/**
 * \brief Read one block of PCM, convert it and fill the inactive sound buffer.
 *
 * The DBG pins show the time of the three steps on the oscilloscope:
 * DBG_FS = FS_Read(), DBG_CONV = conversion, DBG_FILL = SOUND_FillBuffer(),
 * DBG_DEAD = from the buffer swap in the ISR until this function is done.
 *
 *   Format         block_align  raw bytes per 1024 output samples
 *   8-bit  mono         1          1024   (no conversion)
 *   16-bit mono         2          2048
 *   24-bit mono         3          3072
 *   stereo           2 x mono   2 x mono   (L+R averaged, N3)
 *   44100 Hz           2 x      (2 frames averaged, N4)
 *
 * Read errors force the EOF: MP3__bytesRemaining is set to 0.
 */
static void MP3__readBlock(void)
{
    uint16_t const blockAlign = MP3__wavInfo.header.block_align;                     /* bytes per input frame, 1..6  */
    uint8_t  const step       = (uint8_t)(MP3__wavInfo.header.sample_rate / MP3__OUTPUT_RATE); /* 1 or 2, validated  */
    uint16_t const inBytes    = (uint16_t)(blockAlign * step);                       /* input bytes per output sample */
    uint32_t maxBytes         = (uint32_t)SOUND_BUFFER_SIZE * inBytes;
    uint32_t toRead;
    U32      bytesRead;
    uint32_t total;
    uint16_t samples;

    /* INC 6A: never read more than the buffer can hold */
    if (maxBytes > MP3__MAX_READ)
    {
        maxBytes = MP3__MAX_READ;
    }
    /* INC 6D: read whole output samples only */
    maxBytes -= (maxBytes % inBytes);

    /* INC 8 / N5: the first read stops on a sector boundary, all later reads
       are maxBytes (a multiple of 512 for the usual formats) */
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

    /* INC 8 / N5: put back the partial output sample kept from the previous call */
    if (MP3__carryLen > 0u)
    {
        (void)memcpy(MP3__readBuf, MP3__carry, MP3__carryLen);
    }

    DBG_FS_Write(1u);                               /* shows the total SD read time */
    bytesRead = FS_Read(MP3__pFile, &MP3__readBuf[MP3__carryLen], toRead);
    DBG_FS_Write(0u);

    if (bytesRead == 0u)
    {
        if (toRead > 0u)                            /* INC 10C: asked for data, got none = read error */
        {
            UART_LOG_PutString("\r\n[MP3] ERROR: SD read failed\n");
            MP3__error = TRUE;
        }
        MP3__bytesRemaining = 0u;                   /* read error -> force EOF, avoid an endless loop */
        MP3__carryLen       = 0u;
    }
    else
    {
        MP3__bytesRemaining -= (uint32_t)bytesRead;

        total         = (uint32_t)MP3__carryLen + bytesRead;
        samples       = (uint16_t)(total / inBytes);
        MP3__carryLen = (uint8_t)(total - ((uint32_t)samples * inBytes));

        if (MP3__carryLen > 0u)
        {
            (void)memcpy(MP3__carry, &MP3__readBuf[(uint32_t)samples * inBytes], MP3__carryLen);
        }

        DBG_CONV_Write(1u);                         /* shows the conversion run time */
        if (inBytes > 1u)                           /* 8-bit mono 22050 Hz needs no conversion */
        {
            MP3__convertBlock(samples);
        }
        DBG_CONV_Write(0u);

        if (samples > 0u)
        {
            DBG_FILL_Write(1u);                     /* shows copy and padding time */
            (void)SOUND_FillBuffer(MP3__readBuf, samples);  /* samples is 1..SOUND_BUFFER_SIZE, so no RC_ERROR (N10) */
            DBG_FILL_Write(0u);
        }
    }

    DBG_DEAD_Write(0u);     /* shows the actual deadline margin (from the buffer swap in the ISR until here) */
}

/**
 * \brief Convert raw PCM frames in MP3__readBuf to one unsigned 8 bit sample each, in place.
 *
 * Every output sample is made from n = channels * step input values (the MSB
 * of each channel of each frame, N2):
 *
 *      out = ( sum(v) - 128 * n ) / n + 128      with v = MSB ^ 0x00 (8 bit) or MSB ^ 0x80 (16/24 bit)
 *
 * which is the signed average moved to 0..255 (N1, N3, N4). n is 1, 2 or 4,
 * so every case has its own short loop without inner loops (N1).
 *
 * Only called for blocks with more than one input byte per output sample.
 *
 * \param uint16_t const samples     : [IN] number of output samples, 1..SOUND_BUFFER_SIZE
 */
static void MP3__convertBlock(uint16_t const samples)
{
    uint16_t const blockAlign = MP3__wavInfo.header.block_align;
    uint8_t  const channels   = (uint8_t)MP3__wavInfo.header.number_of_channels;    /* 1 or 2 */
    uint8_t  const bytesPerCh = (uint8_t)(MP3__wavInfo.header.bits_per_sample / 8u);/* 1..3   */
    uint8_t  const msbOff     = (uint8_t)(bytesPerCh - 1u);                         /* MSB is the last byte (little endian), N2 */
    uint8_t  const step       = (uint8_t)(MP3__wavInfo.header.sample_rate / MP3__OUTPUT_RATE);  /* 1 or 2 */
    uint16_t const inBytes    = (uint16_t)(blockAlign * step);
    uint8_t  const n          = (uint8_t)(channels * step);                         /* values per output sample: 1, 2 or 4 */
    uint16_t const flip       = (bytesPerCh == 1u) ? 0x00u : 0x80u;                 /* N1: unsigned 8 bit stays, signed MSB is flipped */
    uint8_t  off[MP3__MAX_AVG];
    uint8_t  f;
    uint8_t  c;
    uint8_t  k = 0u;
    uint16_t i;
    uint8_t * p = MP3__readBuf;                     /* start of input sample i */

    /* Byte offset of each value inside one input sample (computed once per block, not per sample) */
    for (f = 0u; f < step; f++)
    {
        for (c = 0u; c < channels; c++)
        {
            off[k] = (uint8_t)((f * blockAlign) + (c * bytesPerCh) + msbOff);
            k++;
        }
    }

    /* N2: in place is safe - all input bytes of sample i are read before index i is written,
       and index i is always lower than the input of sample i+1 */
    switch (n)
    {
        case 1u:        /* mono, 22050 Hz, 16/24 bit: the MSB alone */
            for (i = 0u; i < samples; i++)
            {
                MP3__readBuf[i] = (uint8_t)((uint16_t)p[off[0]] ^ flip);
                p += inBytes;
            }
            break;

        case 2u:        /* stereo 22050 Hz or mono 44100 Hz */
            for (i = 0u; i < samples; i++)
            {
                int16_t const acc = (int16_t)(((int16_t)((uint16_t)p[off[0]] ^ flip)
                                             + (int16_t)((uint16_t)p[off[1]] ^ flip)) - 256);
                MP3__readBuf[i] = (uint8_t)((acc / 2) + 128);   /* N3: division truncates toward 0 */
                p += inBytes;
            }
            break;

        case 4u:        /* stereo 44100 Hz */
            for (i = 0u; i < samples; i++)
            {
                int16_t const acc = (int16_t)(((int16_t)((uint16_t)p[off[0]] ^ flip)
                                             + (int16_t)((uint16_t)p[off[1]] ^ flip)
                                             + (int16_t)((uint16_t)p[off[2]] ^ flip)
                                             + (int16_t)((uint16_t)p[off[3]] ^ flip)) - 512);
                MP3__readBuf[i] = (uint8_t)((acc / 4) + 128);
                p += inBytes;
            }
            break;

        default:
            /* not reachable: WAV_ParseHeader() only accepts 1/2 channels and 22050/44100 Hz */
            break;
    }
}

/* ------------------------------------------------------------------------- */
/* SD card scan (tsk_init)                                                   */
/* ------------------------------------------------------------------------- */

/**
 * \brief Is the file name a WAV file (".WAV" or ".wav")?
 * \param char const *const name     : [IN] NUL terminated file name, not NULL
 * \return TRUE when the name is longer than 4 characters and ends with ".WAV" or ".wav",
 *         FALSE otherwise
 */
static boolean_t MP3__isWavName(const char * const name)
{
    size_t    const len = strlen(name);
    boolean_t       isWav = FALSE;

    if (len > 4u)
    {
        if ((strcmp(&name[len - 4u], ".WAV") == 0) || (strcmp(&name[len - 4u], ".wav") == 0))
        {
            isWav = TRUE;
        }
    }
    return isWav;
}

/**
 * \brief Does an emFile attribute byte contain a flag?
 * \param U8 const attr              : [IN] attribute byte of a directory entry
 * \param uint32_t const flag        : [IN] FS_ATTR_* mask
 * \return TRUE when the flag is set,
 *         FALSE otherwise
 */
static boolean_t MP3__hasAttr(U8 const attr, uint32_t const flag)
{
    return (((uint32_t)attr & flag) != 0u) ? TRUE : FALSE;
}

/**
 * \brief Build "dir\name" (or "\name" for the root) with a length check.
 * \param char *const dst            : [OUT] buffer of dstSize bytes, written only on RC_SUCCESS
 * \param size_t const dstSize       : [IN]  size of dst including the terminating NUL
 * \param char const *const dir      : [IN]  parent folder, "\" for the root
 * \param char const *const name     : [IN]  entry name
 * \return RC_SUCCESS when the path fits and is NUL terminated in dst,
 *         RC_ERROR_OVERRUN when the path is too long (dst is not written)
 */
static RC_t MP3__buildPath(char * const dst, size_t const dstSize, const char * const dir, const char * const name)
{
    size_t    const dirLen    = strlen(dir);
    size_t    const nameLen   = strlen(name);
    boolean_t const isRoot    = ((dir[0] == MP3__PATH_SEP) && (dir[1] == '\0')) ? TRUE : FALSE;
    size_t    const prefixLen = (isRoot == TRUE) ? 1u : (dirLen + 1u);      /* "\" or "dir\" */
    RC_t            rc        = RC_ERROR_OVERRUN;                          /* N11: the path does not fit */
 
    if ((prefixLen + nameLen) < dstSize)
    {
        if (isRoot == FALSE)
        {
            (void)memcpy(dst, dir, dirLen);
        }
        dst[prefixLen - 1u] = MP3__PATH_SEP;
        (void)memcpy(&dst[prefixLen], name, nameLen + 1u);      /* including the NUL */
        rc = RC_SUCCESS;
    }
    return rc;
}

/**
 * \brief Write "prefix + path + '" to the UART log (no sprintf, N10).
 * \param char const *const prefix   : [IN] text before the path, ends with a quote
 * \param char const *const path     : [IN] path to show
 */
static void MP3__logPath(const char * const prefix, const char * const path)
{
    UART_LOG_PutString(prefix);
    UART_LOG_PutString(path);
    UART_LOG_PutString("'\r\n");
}

/**
 * \brief Extract the file name from a full path for display (TFT/UART), INC 9.
 *
 * "\MUSIC\SONG.WAV" gives "SONG.WAV". A string without a separator is returned unchanged.
 *
 * Not called yet (kept for the display). A compiler may warn "defined but not used".
 *
 * \param char const *const fullPath : [IN] NUL terminated path as stored in MP3__songList, not NULL
 * \return pointer into fullPath, never NULL
 */
static const char * MP3__shortName(const char * const fullPath)
{
    const char * const p = strrchr(fullPath, (int)MP3__PATH_SEP);

    return (p != NULL) ? &p[1] : fullPath;
}

/**
 * \brief Reopen the folder of a scan level and skip the entries that were already read (N6).
 *
 * The path of the level is the first MP3__scanLen[level] characters of MP3__scanPath.
 *
 * \param uint8_t const level        : [IN] scan level, 0..MP3__SCAN_MAX_DEPTH
 * \return handle of the reopened folder,
 *         NULL when FS_OpenDir() failed (UART line written, error flag set)
 */
static FS_DIR * MP3__scanReopen(uint8_t const level)
{
    FS_DIR * pDir;
    U16      skip = 0u;

    MP3__scanPath[MP3__scanLen[level]] = '\0';              /* cut the path back to this level */
    pDir = FS_OpenDir(MP3__scanPath);
    if (pDir == NULL)
    {
        MP3__logPath("[SCAN] Cannot reopen dir: '", MP3__scanPath);
        MP3__error = TRUE;                                  /* INC 10C */
    }
    else
    {
        while ((skip < MP3__scanIdx[level]) && (FS_ReadDir(pDir) != NULL))
        {
            skip++;
        }
    }
    return pDir;
}

/**
 * \brief Scan the whole card from the root and store all WAV files in MP3__songList.
 *
 * Stores the full path of each WAV file, e.g. "\MUSIC\SONG.WAV". Entries
 * starting with '.', hidden and system entries are skipped. The scan stops when
 * the list is full. Subfolders are scanned down to MP3__SCAN_MAX_DEPTH levels.
 *
 * It is a loop with an explicit level counter instead of a recursive function
 * (MISRA C:2012 Rule 17.2). The parent folder is closed while a subfolder is
 * scanned and opened again afterwards, skipping what was already read (N6).
 *
 * \return RC_SUCCESS when every folder could be opened (also with 0 songs found),
 *         RC_ERROR_OPEN when a folder could not be opened or reopened, the list may be
 *         incomplete (UART line written, error flag set, N11)
 */
static RC_t MP3__scanSD(void)
{
    FS_DIR    * pDir;
    FS_DIRENT * pDirEnt;
    char        fullPath[MP3_FILENAME_MAXLEN];
    U8          attr;
    uint8_t     level     = 0u;             /* folder level: 0 = root */
    boolean_t   active    = TRUE;           /* FALSE when the root is finished */
    boolean_t   levelDone;
    RC_t        rc        = RC_SUCCESS;     /* N11: RC_ERROR_OPEN as soon as one folder could not be opened */
 
    MP3__scanPath[0] = MP3__PATH_SEP;       /* root = "\" */
    MP3__scanPath[1] = '\0';
    MP3__scanLen[0]  = 1u;
    MP3__scanIdx[0]  = 0u;
 
    pDir = FS_OpenDir(MP3__scanPath);
    if (pDir == NULL)
    {
        MP3__logPath("[SCAN] Cannot open dir: '", MP3__scanPath);
        MP3__error = TRUE;                                  /* INC 10C */
        rc         = RC_ERROR_OPEN;
        active     = FALSE;
    }
 
    while (active == TRUE)
    {
        /* A level is done when its folder could not be (re)opened, the list is full, or the folder has no more entries */
        levelDone = ((pDir == NULL) || (MP3__player.m_noSongs >= MP3_MAX_SONGS)) ? TRUE : FALSE;
 
        if (levelDone == FALSE)
        {
            pDirEnt = FS_ReadDir(pDir);
            if (pDirEnt == NULL)
            {
                levelDone = TRUE;                           /* end of folder */
            }
            else
            {
                MP3__scanIdx[level]++;                      /* entries consumed so far in this folder */
 
                FS_DirEnt2Name(pDirEnt, MP3__entryName);
                FS_DirEnt2Attr(pDirEnt, &attr);
 
                if ((MP3__entryName[0] == '.')
                    || (MP3__hasAttr(attr, FS_ATTR_HIDDEN) == TRUE)
                    || (MP3__hasAttr(attr, FS_ATTR_SYSTEM) == TRUE))
                {
                    /* ".", "..", hidden and system entries are skipped */
                }
                else if (MP3__buildPath(fullPath, sizeof(fullPath), MP3__scanPath, MP3__entryName) != RC_SUCCESS)
                {
                    MP3__logPath("[SCAN] Path too long, skipped: '", MP3__entryName);
                }
                else if (MP3__hasAttr(attr, FS_ATTR_DIRECTORY) == TRUE)
                {
                    if (level >= MP3__SCAN_MAX_DEPTH)
                    {
                        MP3__logPath("[SCAN] Folder too deep: '", fullPath);
                    }
                    else
                    {
                        /* N6: release the handle BEFORE opening the subfolder */
                        (void)FS_CloseDir(pDir);
                        level++;
                        (void)memcpy(MP3__scanPath, fullPath, strlen(fullPath) + 1u);
                        MP3__scanLen[level] = (uint8_t)strlen(fullPath);
                        MP3__scanIdx[level] = 0u;
 
                        pDir = FS_OpenDir(MP3__scanPath);
                        if (pDir == NULL)
                        {
                            MP3__logPath("[SCAN] Cannot open dir: '", MP3__scanPath);
                            MP3__error = TRUE;              /* INC 10C */
                            rc         = RC_ERROR_OPEN;
                        }
                    }
                }
                else if (MP3__isWavName(MP3__entryName) == TRUE)
                {
                    /* fullPath is shorter than MP3_FILENAME_MAXLEN (checked in MP3__buildPath) */
                    (void)memcpy(MP3__songList[MP3__player.m_noSongs], fullPath, strlen(fullPath) + 1u);
                    MP3__player.m_noSongs++;
                }
                else
                {
                    /* not a WAV file */
                }
            }
        }
 
        if (levelDone == TRUE)
        {
            if (pDir != NULL)
            {
                (void)FS_CloseDir(pDir);
                pDir = NULL;
            }
            if (level == 0u)
            {
                active = FALSE;                             /* root finished */
            }
            else
            {
                level--;
                pDir = MP3__scanReopen(level);              /* back in the parent folder */
                if (pDir == NULL)
                {
                    rc = RC_ERROR_OPEN;                     /* error flag and UART line were set by MP3__scanReopen() */
                }
            }
        }
    }
 
    return rc;
}

/* ------------------------------------------------------------------------- */
/* TFT (tsk_init, tsk_display)                                               */
/* ------------------------------------------------------------------------- */

/**
 * \brief Clear the screen and draw the static header. Called once from MP3_Init().
 */
static void MP3__initDisplay(void)
{
    TFT_fillScreen(BLACK);
    MP3__dispCount = MP3__DISP_NONE;        /* INC 10B: the screen was wiped -> next MP3__showSong() redraws all rows */
    MP3__dispSong  = MP3__DISP_NONE;
    MP3__dispMode  = MP3__DISP_MODE_NONE;   /* INC 10C */
    TFT_setTextSize(MP3__TEXT_SIZE);
    TFT_setTextWrap(FALSE);
    TFT_setColors(CYAN, BLACK);
    TFT_setCursor(MP3__HEADER_X, MP3__HEADER_Y);
    TFT_println("==== MP3 Player ====");     /* N10: tft.h takes char *, a literal is passed */
}

/**
 * \brief Clear the area of one list row (black, no text).
 * \param uint16_t const i           : [IN] row, 0..MP3_MAX_SONGS-1
 */
static void MP3__clearRow(uint16_t const i)
{
    TFT_fillRect(0, (int16_t)(MP3__ROW0_RECT_Y + ((int16_t)i * MP3__ROW_PITCH)), CFG_TFT_WIDTH, MP3__ROW_RECT_H, BLACK);
}

/**
 * \brief Draw ONE song row (INC 10B). No full screen clear (N9).
 *
 * Selected row: BLACK on DARKBLUE with "(*)". Other rows: GREEN on BLACK with "( )".
 *
 * \param uint16_t const i           : [IN] row to draw, 0..m_noSongs-1
 * \param uint16_t const selected    : [IN] currently selected row
 */
static void MP3__drawRow(uint16_t const i, uint16_t const selected)
{
    boolean_t const sel = (i == selected) ? TRUE : FALSE;
    int16_t   const row = (int16_t)((int16_t)i * MP3__ROW_PITCH);
    size_t    const len = strlen(MP3__songList[i]);

    /* Clear only this row's area */
    TFT_fillRect(0, (int16_t)(MP3__ROW0_RECT_Y + row), CFG_TFT_WIDTH, MP3__ROW_RECT_H, (sel == TRUE) ? DARKBLUE : BLACK);

    TFT_setTextSize(MP3__TEXT_SIZE);
    TFT_setTextWrap(FALSE);
    TFT_setColors((sel == TRUE) ? BLACK : GREEN, (sel == TRUE) ? DARKBLUE : BLACK);
    TFT_setCursor(MP3__TEXT_X, (int16_t)(MP3__ROW0_TEXT_Y + row));

    /* N9: "(*) path" built in a static buffer, MP3__songList entries are always NUL terminated and shorter than MP3_FILENAME_MAXLEN */
    (void)memcpy(MP3__rowText, (sel == TRUE) ? "(*) " : "( ) ", MP3__ROW_PREFIX_LEN);
    (void)memcpy(&MP3__rowText[MP3__ROW_PREFIX_LEN], MP3__songList[i], len + 1u);
    TFT_print(MP3__rowText);
}

/**
 * \brief Draw the one-line message of a message mode in the first list row (INC 10C).
 * \param uint8_t const mode         : [IN] MP3__DISP_MODE_ERROR or MP3__DISP_MODE_NOSONGS
 */
static void MP3__drawMessage(uint8_t const mode)
{
    TFT_setTextSize(MP3__TEXT_SIZE);
    TFT_setTextWrap(FALSE);
    TFT_setCursor(MP3__TEXT_X, MP3__ROW0_TEXT_Y);
    if (mode == MP3__DISP_MODE_ERROR)
    {
        TFT_setColors(RED, BLACK);
        TFT_print("Error: See UART Log");      /* N10: tft.h takes char *, a literal is passed */
    }
    else
    {
        TFT_setColors(YELLOW, BLACK);
        TFT_print("No Song Detected :(");      /* N10 */
    }
}

/**
 * \brief The kind of content changed: wipe the rows the old content used and prepare the new one (N9).
 * \param uint8_t const mode         : [IN] new MP3__DISP_MODE_*
 */
static void MP3__switchMode(uint8_t const mode)
{
    uint16_t used;
    uint16_t i;

    if (MP3__dispMode != MP3__DISP_MODE_NONE)
    {
        /* a list used dispCount rows, a message used one */
        used = ((MP3__dispMode == MP3__DISP_MODE_LIST) && (MP3__dispCount != MP3__DISP_NONE)) ? MP3__dispCount : 1u;
        for (i = 0u; i < used; i++)
        {
            MP3__clearRow(i);
        }
    }

    if (mode == MP3__DISP_MODE_LIST)
    {
        MP3__dispCount = MP3__DISP_NONE;    /* force a full list draw */
        MP3__dispSong  = MP3__DISP_NONE;
    }
    else
    {
        MP3__drawMessage(mode);
        MP3__dispCount = 0u;
    }
    MP3__dispMode = mode;
}

/**
 * \brief List mode: redraw everything if the number of songs changed, else only the old and the new selected row (INC 10B, N9).
 * \param uint16_t const cur         : [IN] selected song, 0..no-1
 * \param uint16_t const no          : [IN] number of songs, 1..MP3_MAX_SONGS
 */
static void MP3__updateList(uint16_t const cur, uint16_t const no)
{
    uint16_t i;

    if (no != MP3__dispCount)
    {
        for (i = 0u; i < no; i++)
        {
            MP3__drawRow(i, cur);
        }
        if (MP3__dispCount != MP3__DISP_NONE)
        {
            for (i = no; i < MP3__dispCount; i++)       /* rows that are gone */
            {
                MP3__clearRow(i);
            }
        }
        MP3__dispCount = no;
        MP3__dispSong  = cur;
    }
    else if (cur != MP3__dispSong)
    {
        if (MP3__dispSong < no)
        {
            MP3__drawRow(MP3__dispSong, cur);           /* old row loses the (*) */
        }
        if (cur < no)
        {
            MP3__drawRow(cur, cur);                     /* new row gets the (*) */
        }
        MP3__dispSong = cur;
    }
    else
    {
        /* nothing changed: nothing to draw */
    }
}

#if (MP3__DEBUG_LOG != 0u)
/**
 * \brief Debug aid: print state and event as text via UART (N10).
 *
 * Compiled only with MP3__DEBUG_LOG 1.
 *
 * \param EventMaskType const ev     : [IN] event processed
 * \param MP3__State_t const state   : [IN] player state
 */
static void MP3__showEventAndState(EventMaskType const ev, MP3__State_t const state)
{
    const char * stateStr;
    const char * eventStr;

    switch (ev)
    {
        case ev_1s :      eventStr = "ev_1s";      break;
        case ev_Button1 : eventStr = "ev_Button1"; break;
        case ev_Button2 : eventStr = "ev_Button2"; break;
        case ev_Button3 : eventStr = "ev_Button3"; break;
        case ev_Button4 : eventStr = "ev_Button4"; break;
        default :         eventStr = "WRONG EVENT"; break;
    }

    switch (state)
    {
        case MP3STATE_ISPLAYING : stateStr = "IsPlaying"; break;
        case MP3STATE_ISPAUSING : stateStr = "IsPausing"; break;
        case MP3STATE_ISSTOPPED :
        default :                 stateStr = "IsStopped"; break;
    }

    UART_LOG_PutString("State: ");
    UART_LOG_PutString(stateStr);
    UART_LOG_PutString(" | Event: ");
    UART_LOG_PutString(eventStr);
    UART_LOG_PutString("\n");
}
#endif

/*----------------------------------------------------------------------------------------
* Notes:
*
* N1 - Sample conversion without inner loops. The VDAC8 wants one unsigned 8 bit value per
*      tick. Per output sample the MSB of every channel of every input frame is used. A
*      signed MSB (16/24 bit PCM) becomes unsigned by flipping bit 7 (MSB ^ 0x80), 8 bit WAV
*      is unsigned already (^ 0x00). With n values per output sample:
*          out = (sum(v) - 128 * n) / n + 128
*      For n = 1 that is just v, a single XOR. The generic version of INC 6 ran two nested
*      loops (frames, channels) with 3 multiplications per value for every sample. Now the
*      byte offsets of the n values are calculated once per block and each case (n = 1, 2, 4)
*      has a straight loop. The result is bit-identical to INC 6/8 (checked on the host
*      for all 12 formats with random data, see the INC 11C report).
*
* N2 - Frame layout (little endian): [ch0 LSB..MSB][ch1 LSB..MSB]. The MSB of channel 0 sits
*      at offset bytesPerCh-1, lower bytes are dropped (VDAC8 resolution). The conversion is
*      done in place: output sample i is written to MP3__readBuf[i], and sample j > i is read
*      from index j * inBytes + offset >= j > i, so no unread byte is overwritten. This only
*      holds for inBytes > 1, therefore 8 bit mono 22050 Hz (inBytes == 1) skips the conversion.
*
* N3 - Stereo is mixed to mono by averaging the signed L and R MSBs. Averaging (not summing)
*      keeps the result in -128..127, so it cannot clip. C integer division truncates toward
*      zero; this is kept (no shift, which would round toward minus infinity) so the sound is
*      exactly as before. The compiler turns "/ 2" and "/ 4" into shifts with a sign fix.
*
* N4 - 44100 Hz is decimated by 2: each output sample averages 2 consecutive input frames
*      (a simple low-pass that reduces aliasing compared with dropping every 2nd frame). The
*      read size is rounded down to whole output samples so a frame pair never splits across
*      two blocks. Stereo 44100 Hz therefore averages 4 values.
*
* N5 - Sector alignment (INC 8). emFile is configured without cache and file buffer
*      (FS_SUPPORT_CACHE 0, FS_USE_FILE_BUFFER 0), so every FS_Read() reads whole sectors from
*      the card. A read that starts in the middle of a sector costs one sector more. The PCM data
*      starts behind the header (44 bytes or more), so the first read only goes up to the next
*      sector boundary (MP3__alignBytes), then all reads are sector aligned. A read does not
*      always end on a frame boundary: the rest of a frame is kept in MP3__carry and put in front
*      of the next block. MP3__readBuf has MP3__MAX_INBYTES extra bytes for it.
*
* N6 - SD card scan. (a) The parent folder is closed before a subfolder is opened and reopened
*      afterwards (the already consumed entries are read again and skipped). emFile may allow only
*      a few open directory handles at the same time; closing the parent first is the safe way.
*      (Whether a nested FS_OpenDir() really fails on the target was not confirmed.)
*      (b) The scan is a loop with a level counter, not a recursive function (MISRA Rule 17.2). Its
*      state is small: MP3__scanPath holds the path of the folder in work; the path of an upper level
*      is just the beginning of it, so only the length (MP3__scanLen) and the number of entries read
*      (MP3__scanIdx) are kept per level - about 90 bytes of RAM for 9 levels, no stack growth.
*      (c) MP3__entryName is static: FS_DirEnt2Name() writes the whole name without a size
*      limit, so it must be big enough for the longest name (FS_MAX_PATH). With the old 32 byte
*      stack buffer a longer name wrote beyond the buffer. (d) Paths that do not fit into
*      MP3_FILENAME_MAXLEN are skipped with a log line instead of being cut (a cut path cannot be
*      opened). (e) Folders deeper than MP3__SCAN_MAX_DEPTH levels are not scanned. (f) "No WAV
*      files found" is written once from MP3_Init(), not at the end of every folder.
*
* N7 - Tasks and shared data. tsk_control (buttons, 1 s) and tsk_reader (SD streaming) share
*      m_state, m_newSongRequest, m_songIndex and m_failCount of the reader and
*      m_remainingPlayTime of the player; tsk_display reads m_currentSong. These are volatile.
*      They are 16 bit values on a 32 bit CPU, so single reads and writes are atomic. What is NOT
*      protected: MP3__stopSong() closes the file and resets the reader state from tsk_control. This
*      is only safe as long as tsk_control cannot interrupt a running MP3_Reader(), i.e. the
*      reader has the higher (or the same, non-preemptive) priority in the OS configuration.
*      The EOF is detected one run after the last block was read, so a song is cut about
*      two buffers (up to ~93 ms) before its end if the next song is requested at once.
*
* N8 - Error handling (INC 10C). Any task sets MP3__error (SD card scan, file open, bad header,
*      read error, no playable song). The TFT shows "Error: See UART Log" while it is set. A
*      button press clears it - but only if there is at least one song, otherwise the player
*      ignores all events and the display shows "No Song Detected :(" (the error flag has
*      priority over it). MP3_ProcessEvent() ignores events completely while stopped without
*      songs, so MP3__nextSong() / MP3__prevSong() never use a list of length 0.
*
* N9 - TFT. The display keeps a shadow of what is drawn (MP3__dispMode, MP3__dispCount,
*      MP3__dispSong) and compares it with the player state on every tsk_display run. Drawing
*      on the SPI display is slow, so nothing is drawn if nothing changed; if only the selection
*      changed, two rows are redrawn; if the number of songs changed, all rows. A row is cleared
*      by a black rectangle instead of a full screen clear, so there is no flicker. The text
*      "(*) path" is built in the static MP3__rowText and sent with one TFT_print(), instead of
*      TFT_printf() whose macro would place a 128 byte buffer on the stack of tsk_display and
*      uses sprintf (N10).
*
* N10 - MISRA C:2012 deviations that remain (checked by hand):
*      - Rule 16.3 (fall through): in MP3_Reader() the IDLE case runs into the OPEN case on
*        purpose, to open a requested song in the same run. Marked with a comment.
*      - Rule 17.2 (recursion): removed, the folder scan is a loop now (N6).
*      - Rule 14.4 (non-Boolean condition): the state machines test events as "nextEvent & ev_x",
*        as in the original code, for readability. Not changed on purpose.
*      - Rule 21.6 (stdio): none in this file any more. sprintf/snprintf were replaced by
*        string pieces and the length checked MP3__buildPath().
*      - Rule 7.4 (string literal to char *): TFT_print() and TFT_println() in tft.h take
*        "char *". Fix in tft.h: "const char *" (no behaviour change).
*      - Rule 17.7 (return value ignored): the return values of SOUND_* and SetEvent() are
*        cast to void. SOUND_FillBuffer() can only fail for size 0 or > SOUND_BUFFER_SIZE, and
*        samples is in 1..SOUND_BUFFER_SIZE by construction. SOUND_StartPlay/StopPlay/
*        SwapBuffers/SetVolume always return RC_SUCCESS.
*      - Rule 8.7/8.9 style: all helpers are static now (they were global before).
*      - Rule 2.1/2.2/3.1 (dead and commented-out code): all commented-out debug prints were removed. MP3__shortName() is kept (needed for
*        the display) but is not called yet, so the compiler may report it as unused. MP3__showEventAndState() stays, but is only
*        compiled with MP3__DEBUG_LOG 1.
*      - Dir 4.9 / Rule 20.10: the DBG_HI/DBG_LO macros with ## were replaced by direct
*        DBG_xxx_Write() calls.
*      - Naming rule of the course: MP3__showSong() is public but has the private "__" prefix.
*        It is kept because main.c calls it.
*
* N11 - Return codes (INC 11B.1). Functions that can fail for a reason return RC_t:
*        MP3__openWavFile()   RC_ERROR_RANGE (songIndex not in the list, new check),
*                             RC_ERROR_OPEN (FS_FOpen() failed) or the code of WAV_ParseHeader()
*                             (RC_ERROR_READ_FAILS, _INVALID, _NOT_IMPLEMENTED, see N10 in
*                             wav_parser.c). MP3_Reader() still only tests "!= RC_SUCCESS".
*        MP3__buildPath()     RC_ERROR_OVERRUN (path does not fit into the buffer), was boolean_t.
*        MP3__scanSD()        RC_ERROR_OPEN as soon as a folder could not be (re)opened (the list
*                             may be incomplete), was void. The error flag is set as before.
*        MP3_Init()           returns the result of MP3__scanSD(). RC_SUCCESS also for a card
*                             without WAV files (the TFT shows "No Song Detected :(").
*        MP3_ProcessEvent()   RC_ERROR_BAD_PARAM (no known event bit) and RC_ERROR_INVALID_STATE
*                             (stopped, no songs). In both cases the event is ignored as before.
*      No RC_t on purpose:
*        MP3_Reader()         body of a task, nobody to report to. Its errors go to the player
*                             through m_failCount and MP3__error (N8).
*        MP3__readBlock()     a read error sets MP3__bytesRemaining to 0 (EOF) and MP3__error; a
*                             return code would only repeat this.
*        MP3__convertBlock(), volume, play, stop, pause, resume and display functions cannot fail
*                             (SOUND_* always return RC_SUCCESS, the TFT driver reports nothing).
*        MP3__nextSong() / MP3__prevSong() are guarded against an empty list, no caller could react.
*        MP3__scanReopen()    returns the FS_DIR handle, NULL = failed (like FS_OpenDir()).
*        MP3__isWavName(), MP3__hasAttr(), MP3__songEnded() are yes/no questions (boolean_t).
*      main.c ignores the result of MP3_Init() and stores that of MP3_ProcessEvent() in an unused
*      variable, so nothing changes there. MISRA 17.7 asks for a "(void)" cast if the result is ignored.
*
*
* Time:
* SPI       SCLK                4MHz
* DBG_FS    FS_Read             9.2ms   19.5% of 47.06ms (shared)
*               SPI             8.9ms
*               other           0.3
* DBG_FILL  SOUND_FillBuffer    0.3ms   1.00% of 47.06ms (shared)
* DBG_DEAD  swap to refill done 9.7ms   20.6% of 47.06ms (shared)
*
* Refill period: 47.06
* Free time: 47.06 - 9.7 = 37.4 ms (about 79%) per refill.
*
* The ISR fires once per sample, every 1 / 22050 Hz = 45.35 us, and outputs one sample to the PWM.
* A buffer holds 1024 samples, so playing it takes 1024 * 45.35 us = 46.4 ms. We measured 47.06 ms.
* So 47 ms is just 1024 samples at 22.05 kHz, the time the ISR needs to run through one buffer.
* The reader takes about 9.7 ms to refill the inactive buffer.
*
* SEGGER forum says "DMA is heavily used and preferred" for SD card mode on PSoC 5LP. That is true
* for PSoC 6 with its dedicated SDHC peripheral. On PSoC 5LP there is no dedicated SD/MMC hardware
* block: the SPIM component is used over a general purpose SPI bus. The Cypress AppNote AN73854
* (standard reference for emFile + PSoC 5LP) implements FS_MMC_HW_X_Read as a CPU polling loop
* over the SPIM RX FIFO, not DMA. DMA is possible with the PSoC 5LP DMA controller, but it needs
* custom HAL work. So for this hardware polling in the 512 byte read is the most likely reality.
*
* Format             Bytes per refill  Sectors  Samples per refill  SD time now        Share of budget  Free
* C8M 22k                 1024            2          1024            9.2 ms (measured)       20%          80%
* C16M, C8S 22k;          2048            4          1024            16.6 ms                 35%          65%
* C8M 44k
* C24M 22k                3072            6          1024            24.0 ms                 51%          49%
* C16S 22k;               4096            8          1024            31.4 ms                 67%          33%
* C16M, C8S 44k
* C24S 22k;               6144           12          1024            46.2 ms                 98%           2%
* C24M 44k
* C16S 44k                6144           12           768            46.2 ms                 131% of 35 ms  underrun
* C24S 44k                6144           12           512            46.2 ms                 197% of 23.5 ms underrun
*
* The table is the SD read time only. The conversion (DBG_CONV) comes on top; INC 11C makes it
* shorter (N1), which matters most in the rows with little free time. The SPI clock (4 MHz) is
* the real limit: SD time scales with 1 / SCLK.
*/