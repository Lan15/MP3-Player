/**
* \file    mp3.h
* \author  P. Fromm (template), V S Agilan (player)
* \date    6.11.18 (INC 11C: 2026-10)
*
* \brief   A simple MP3 Player (WAV player) application layer: song list, state machine, SD card
*          streaming and TFT song list
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
* The module has four entry points, each one is called from its own task:
*
*   | Task        | Entry point        | Purpose                                  |
*   |-------------|--------------------|------------------------------------------|
*   | tsk_init    | MP3_Init()         | scan the SD card, draw the TFT header    |
*   | tsk_control | MP3_ProcessEvent() | buttons + 1 s tick -> player state       |
*   | tsk_reader  | MP3_Reader()       | open file, read + convert PCM blocks     |
*   | tsk_display | MP3__showSong()    | redraw the TFT, only what has changed    |
*
* Player state machine (MP3_ProcessEvent):
*
*   | State    | Button1       | Button2       | Button3 | Button4 | 1 s tick            |
*   |----------|---------------|---------------|---------|---------|---------------------|
*   | STOPPED  | previous song | next song     | play    | -       | -                   |
*   | PLAYING  | volume +      | volume -      | pause   | stop    | count down / next   |
*   | PAUSING  | -             | -             | resume  | stop    | -                   |
*
* Reader state machine (MP3_Reader): IDLE -> OPEN -> STREAMING -> EOF -> IDLE.
* The reader is woken by ev_BufEmpty, which the sound driver sets every time
* the ISR has switched to the other buffer, and by the player on a song request.
*
* Supported files: see wav_parser.h (PCM, 8/16/24 bit, mono/stereo, 22050 or
* 44100 Hz). Every block is converted to one unsigned 8 bit sample per output
* tick for the VDAC8 (N1..N4 in mp3.c).
*
* \todo <todos>
* \warning The reader state, the song request and the remaining play time are
*          shared between tsk_control and tsk_reader. tsk_control must not
*          interrupt a running MP3_Reader() - check the task priorities in
*          the OIL file (see N7 in mp3.c).
* \warning MP3_Init() calls FS_Init() and scans the card, so the SD card driver
*          must be ready when it is called.
* \warning This software is a PROTOTYPE version and is not designed or intended
*          for use in production, especially not for safety-critical applications!
*
* Changelog:\n
*  - 6.11.18 : template
*  - INC 5   : streaming from the SD card, dual buffer, reader state machine
*  - INC 6   : 16/24 bit, stereo, 44100 Hz
*  - INC 7   : files that fail to open are skipped
*  - INC 8   : sector aligned SD reads
*  - INC 9   : recursive scan of the SD card, full paths
*  - INC 10  : TFT song list, partial redraw, "No songs" / error message
*  - INC 11C : Doxygen, MISRA C:2012 clean-up, faster sample conversion,
*              scan buffers bounded, notes N1..N10 at the end of mp3.c
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

#ifndef MP3_H
#define MP3_H

#include "global.h"
#include "project.h"

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#define MP3_FILENAME_MAXLEN   (64u)	/** \brief Size of one stored song path, including the terminating NUL. Holds e.g. "\MUSIC\SUBFOLDER\SONG.WAV" (INC 9: full path for subfolders). */
#define MP3_MAX_SONGS         (10u)	/** \brief Maximum number of songs in the list. Further files are ignored. The TFT shows one row per song, so this also limits the display. */

/* Wrapper to allow representing the file in Together as class */
#ifdef TOGETHER

class MP3
{
public:
#endif /* Together */


/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/

/**
 * \brief Initialise the player: scan the SD card for WAV files, set the start
 *        volume and draw the TFT header.
 *
 * Starts in the STOPPED state with the first song selected. The card is
 * scanned recursively from the root, hidden and system entries and names that
 * start with '.' are ignored, at most #MP3_MAX_SONGS files are stored.
 * If the card cannot be read the error flag is set, the TFT then shows
 * "Error: See UART Log"; if no WAV file is found it shows "No Song Detected :(".
 *
 * \return RC_SUCCESS always (problems are reported via UART and the TFT).
 * \note   Call once from tsk_init, after TFT_init() and SOUND_Init().
 */
RC_t MP3_Init(void);

/**
 * \brief Player state machine. Processes ONE event.
 *
 * \param[in] EventMaskType nextEvent   : The event to be processed [IN]
 *  A single event bit: ev_Button1..ev_Button4 or ev_1s. Other values are ignored.
 * \return RC_SUCCESS always.
 *
 * \note While no song is in the list and the player is stopped, all events
 *       are ignored (no division by zero in the song selection).
 * \note Any button press clears a displayed error.
 */
RC_t MP3_ProcessEvent(EventMaskType nextEvent);

/**
 * \brief SD card streaming, body of tsk_reader (one run per ev_BufEmpty).
 *
 * Runs the reader state machine one step:
 *  - IDLE      : wait for a song request
 *  - OPEN      : open the file, parse the header, pre-fill both buffers, start the sound
 *  - STREAMING : refill the inactive buffer; a new song request ends the song
 *  - EOF       : stop the sound, close the file
 *
 * \note Call only from TASK(tsk_reader), after WaitEvent(ev_BufEmpty).
 * \note The function returns after each step. It does not contain the
 *       WaitEvent loop (that is in main.c).
 */
void MP3_Reader(void);

/**
 * \brief Update the TFT: song list, "No songs" or error message.
 *
 * Only what changed since the last call is redrawn (rows of the old and the
 * new selection, or the whole list if the number of songs changed).
 * The first call after MP3_Init() draws everything.
 *
 * \note Call only from TASK(tsk_display). The name has a double underscore
 *       although it is public, because main.c already uses it.
 */
void MP3__showSong(void);

/*****************************************************************************/
/* Private stuff, only visible for Together, declared static in cpp - File   */
/*****************************************************************************/


#ifdef TOGETHER


};
#endif /* Together */

#endif /* MP3_H */