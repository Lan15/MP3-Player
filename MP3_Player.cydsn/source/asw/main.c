/* ========================================
*
*  Copyright YOUR COMPANY, THE YEAR
*  All Rights Reserved
*  UNPUBLISHED, LICENSED SOFTWARE.
*
*  CONFIDENTIAL AND PROPRIETARY INFORMATION
*  WHICH IS THE PROPERTY OF your company.
*
* ========================================
* 
* \file    main.c
* \author  V S Agilan (application), PSoC Creator template
* \date    INC 11D: 2026-10
*
* \brief   Application entry point of the WAV / MP3 player: OS start, the tasks and
*          the interrupt service routines
*
* \details
* main() only starts the operating system (Erika OS, OSEK). Everything else runs
* in the tasks and ISRs of this file, which call the modules of the player:
*
*   | Task / ISR      | Started by                          | Does                                  |
*   |-----------------|-------------------------------------|---------------------------------------|
*   | systick_handler | SysTick, every 1 ms                 | counts cnt_systick (alarm time base)  |
*   | tsk_init        | start-up, runs once                 | drivers, EE_system_init, MP3_Init     |
*   | tsk_control     | ev_1s, ev_Button1..ev_Button4       | MP3_ProcessEvent() (player state)     |
*   | tsk_reader      | ev_BufEmpty (set by SOUND_isr)      | MP3_Reader() (SD read + convert)      |
*   | tsk_display     | alarm alrm_display, every 200 ms    | MP3__showSong() (TFT song list)       |
*   | tsk_background  | tsk_init                            | idle loop, lowest priority            |
*   | isr_Button      | button interrupt                    | SetEvent(tsk_control, ev_ButtonN)     |
*   | isr_Timer       | sample timer, 22050 Hz              | SOUND_isr() (next sample to the VDAC) |
*
* Events and alarms are configured in the OSEK configuration (OIL file), not here.
*
* Operation by the four buttons (HMI also via UART log):
*
*   | Button  | Stopped       | Playing       | Pausing |
*   |---------|---------------|---------------|---------|
*   | Button1 | previous song | volume +      | -       |
*   | Button2 | next song     | volume -      | -       |
*   | Button3 | play          | pause         | resume  |
*   | Button4 | -             | stop          | stop    |
*
* The table follows the code of MP3_ProcessEvent() (see also mp3.h). The comment that
* was here before INC 11D said "Button 1: Increase Volume, Next Song" and "Button 2:
* Decrease Volume, Prev Song"; the song direction was the other way round (N11).
*
* Start-up sequence of tsk_init (order matters, N1):
*   UART_LOG_Start, SOUND_SAMPLE_Start, LED_Init, SEVEN_Init, TFT_init, SOUND_Init,
*   EE_system_init, MP3_Init, SetRelAlarm (alrm_1s, alrm_display), ActivateTask
*   (tsk_reader, tsk_control, tsk_background), TerminateTask.
*
* \todo <todos>
* \warning tsk_control must not interrupt a running MP3_Reader() (shared reader state),
*          check the task priorities in the OIL file (N6, and N7 in mp3.c).
* \warning This file was changed in INC 11D by COMMENTS AND NOTES ONLY. No statement,
*          declaration or value was changed. Things that could be improved in the code
*          are listed in N2, N3, N8, N9 and N11 and are left for a code increment.
* \warning This software is a PROTOTYPE version and is not designed or intended
*          for use in production, especially not for safety-critical applications!
*
* Changelog:\n
*  - 6.11.18 : template
*  - INC 3   : debug task with the ISR tick count (now in the #if 0 block, N10)
*  - INC 10  : TFT_init() before MP3_Init(), tsk_display, alarm alrm_display
*  - INC 11D : Doxygen file header and function comments in the project style,
*              notes N1..N11 at the end of the file, ASCII only. No code change.
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

#include "project.h"
#include "global.h"

#include "button.h"
#include "led.h"
#include "seven.h"
#include "tft.h"

#include "sound.h"

#include "mp3.h"

/*****************************************************************************/
/* Interrupt service routine of the system tick                              */
/*****************************************************************************/
 
//ISR which will increment the systick counter every ms
/**
 * \brief System tick interrupt, every 1 ms: advance the counter cnt_systick.
 *
 * cnt_systick is the time base of the alarms alrm_1s and alrm_display (N5), so
 * one counter tick is one millisecond.
 *
 * \note Keep it short, it is called 1000 times per second.
 */
ISR(systick_handler)
{
    CounterTick(cnt_systick);
}

/*****************************************************************************/
/* Old sound tests (commented out, N9)                                       */
/*****************************************************************************/
 
/*void soundTest1()
{
    SOUND_Init();
 
    while(1)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            SOUND_SetSong(SOUND_Records[i].m_data, SOUND_Records[i].m_size);
 
            for (uint32_t s = 0; s < SOUND_Records[i].m_size; s++)
            {
                SOUND_isr();
                CyDelayUs(45);   //22kHz == 45us
            }
        }
 
    }
    while(1){;}
}
 
void soundTest2()
{
    SOUND_Init();
    SOUND_StartPlay();
 
    while(1)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            SOUND_SetSong(SOUND_Records[i].m_data, SOUND_Records[i].m_size);
 
            for (uint32_t s = 0; s < SOUND_Records[i].m_size; s++)
            {
                SOUND_PWM_WriteCompare(SOUND_Records[i].m_data[s]);
                CyDelayUs(45);   //22kHz == 45us
            }
        }
 
    }
    while(1){;}
}
*/

/*****************************************************************************/
/* Entry point and helper functions                                          */
/*****************************************************************************/
 
/**
 * \brief Program entry point: enable the interrupts, start the system tick and the OS.
 *
 * Sets the system tick period to 1 ms (derived from the bus clock BCLK__BUS_CLK__HZ),
 * enables its interrupt and starts it, then hands over to the operating system.
 * StartOS() does not return; the endless loop below it is a safety net only (N7).
 * The first task that runs is tsk_init.
 *
 * \return does not return (int by the C standard)
 * \note   Signature "int main()" is kept as it is (N8).
 */
int main()                                                                 /* N8 */
{
    CyGlobalIntEnable; /* Enable global interrupts. */
 
    //Set systick period to 1 ms. Enable the INT and start it.
	EE_systick_set_period(MILLISECONDS_TO_TICKS(1, BCLK__BUS_CLK__HZ));
	EE_systick_enable_int();
	EE_systick_start();
 
    // Start Operating System
    StartOS(OSDEFAULTAPPMODE);                                             /* N7: does not return */
    for(;;)
    {
        asm("nop");
    }
 
}

/**
 * \brief Trap for an unexpected exception: stop in the debugger.
 *
 * Executes a breakpoint instruction. Look at the call stack in the debugger to
 * see how the program got here. Without a debugger attached, a breakpoint instruction
 * typically ends in the hard fault handler of the Cortex-M3.
 *
 * \note Signature "void unhandledException()" is kept as it is (N8).
 */
void unhandledException()                                                  /* N8 */
{
    //Ooops, something terrible happened....check the call stack to see how we got here...
    __asm("bkpt");
}

/**
 * \brief Take the first set event bit out of an event mask (N3).
 *
 * Looks at the bits from bit 0 upwards and returns the first one that is set. This
 * bit is cleared in the caller's copy of the mask, so calling the function again
 * returns the next event. Used by tsk_control to handle several events one by one.
 *
 * \param EventMaskRefType ev : [IN/OUT] event mask copy of the caller; the returned bit is cleared in it
 *                              (the events of the OS are not touched, only this copy)
 * \return the single event bit (1 << bit number) when at least one bit was set,
 *         0 when the mask is empty
 * \note   "Highest" in the name means the highest priority, i.e. the lowest bit number.
 */
EventMaskType GetAndClearHighestEvent(EventMaskRefType ev)
{
    EventMaskType result = 0;
    EventMaskType lev = *ev;
    
    for (uint8_t i = 0; i < sizeof(EventMaskType) * 8; i++)
    {
        if (0 != (lev & (1<<i)))                                        /* N3 */
        {
            result = (1<<i);
            *ev &= ~result;
            
            return result;
        }
    }   
    
    return 0;
}

/********************************************************************************
 * Task Definitions
 ********************************************************************************/

/**
 * \brief Initialisation task: start the drivers, the application and the other tasks, then end.
 *
 * Runs once after StartOS(). The order of the calls matters (N1):
 *   1. start the hardware drivers (UART log, sample timer component, LED, seven segment,
 *      TFT, sound driver),
 *   2. EE_system_init() - reconfigures the ISRs with the OS parameters,
 *   3. MP3_Init() - scan the SD card, draw the TFT header,
 *   4. start the two alarms, activate the other tasks and terminate itself.
 *
 * The return values of SOUND_Init() and MP3_Init() are not evaluated here (N2).
 *
 * \note Runs only once. TerminateTask() is the last call.
 */
TASK(tsk_init)
{ 
    //Init MCAL Drivers
    UART_LOG_Start();
    SOUND_SAMPLE_Start();
    
    LED_Init();
    SEVEN_Init();
    TFT_init(); /* INC 10 — init TFT before MP3_Init so display is ready for song list */
    SOUND_Init();                                                           /* N1, N2 */
 
    //Reconfigure ISRs with OS parameters.
    //This line MUST be called after the hardware driver initialisation!
    EE_system_init();                                                      /* N1 */
 
    //Application Init
    MP3_Init();                                                            /* N2 */
 
    //Start the alarm with 1000ms cycle time
    SetRelAlarm(alrm_1s,1000,1000);                                        /* N5 */
    SetRelAlarm(alrm_display,1100,200);   /* INC 10 */                     /* N5 */
 
    ActivateTask(tsk_reader);
    ActivateTask(tsk_control);
    ActivateTask(tsk_background);
 
    TerminateTask(); 
}

/**
 * \brief Control task: wait for button and 1 s events and give them to the player.
 *
 * Waits for ev_1s and ev_Button1..ev_Button4 (N4). All events that arrived are read
 * and cleared at once, then handled one by one with GetAndClearHighestEvent(): each
 * event is passed to MP3_ProcessEvent(), which runs the player state machine.
 * The result of MP3_ProcessEvent() is not evaluated (N2).
 *
 * \note Never ends. Must not interrupt a running MP3_Reader() (N6).
 */
TASK(tsk_control)
{    
    EventMaskType ev = 0;
    EventMaskType nextEvent = 0;
    
    while(1)
    {
        //Wait, read and clear the event
        WaitEvent(ev_1s | ev_Button1 | ev_Button2 | ev_Button3 | ev_Button4 );   /* N4 */
        GetEvent(tsk_control,&ev);
        ClearEvent(ev);                                                          /* N4: only the bits read are cleared */
 
        //Process all events from the mask
        while(0 != ev)
        {
            nextEvent = GetAndClearHighestEvent(&ev);
            RC_t result = MP3_ProcessEvent(nextEvent);                           /* N2: result is not used */
        }
    }
}

/**
 * \brief Reader task: refill the sound buffer whenever the sound driver has freed one.
 *
 * SOUND_isr() sets ev_BufEmpty each time it has switched to the other buffer. The task
 * wakes up, clears the event and calls MP3_Reader(), which reads the next block from
 * the SD card, converts it and fills the freed buffer. The reader must finish within
 * one buffer time (1024 samples = 46.4 ms at 22050 Hz), otherwise the sound driver
 * counts an underrun.
 *
 * \note Never ends. The TerminateTask() at the end is not reached (N7).
 */
TASK(tsk_reader)
{
    while (1)
    {
        WaitEvent(ev_BufEmpty);
        ClearEvent(ev_BufEmpty);
 
        //woke - calling MP3_Reader
        MP3_Reader();
    }
 
    TerminateTask();                                                       /* N7: not reached */
}

/* INC 10 - dedicated TFT display task */
/**
 * \brief Display task (INC 10): redraw the TFT song list, then end.
 *
 * Activated every 200 ms by the alarm alrm_display (first after 1100 ms, N5). MP3__showSong()
 * compares the player state with what is drawn and draws only what has changed (N9 in mp3.c),
 * so most runs are short. The SPI display is slow, therefore the drawing is done in its own task.
 *
 * \note Basic task: ends with TerminateTask() after every run.
 */
TASK(tsk_display)
{
    //UART_LOG_PutString("\r\n[DISPLAY] tsk_display started\n");
 
    MP3__showSong();          // renders song list to TFT + UART
 
    TerminateTask();
}

/**
 * \brief Background task: does nothing, runs when no other task is ready.
 *
 * Activated by tsk_init. Endless loop with a nop, lowest priority (N6).
 *
 * \note Never ends.
 */
TASK(tsk_background)
{
    while(1)
    {
        //do something with low priority
        asm("nop");
    }
}

/********************************************************************************
 * ISR Definitions
 ********************************************************************************/

/**
 * \brief Button interrupt: turn the pressed buttons into events for tsk_control.
 *
 * Checks the four buttons and sets one event per pressed button (ev_Button1..ev_Button4).
 * Several buttons pressed at the same time give several events. The events are handled
 * in tsk_control (N4); the ISR itself does no work with the player.
 *
 * \note ISR category 2 (OS services allowed).
 */
ISR2(isr_Button)
{
    //Set the button bit
    if (BUTTON_IsPressed(BUTTON_1)) SetEvent(tsk_control, ev_Button1);
    if (BUTTON_IsPressed(BUTTON_2)) SetEvent(tsk_control, ev_Button2);
    if (BUTTON_IsPressed(BUTTON_3)) SetEvent(tsk_control, ev_Button3);
    if (BUTTON_IsPressed(BUTTON_4)) SetEvent(tsk_control, ev_Button4);
}
 
/**
 * \brief Sample timer interrupt, 22050 Hz: give the next sample to the sound driver.
 *
 * All work is done in SOUND_isr(): output of one sample, buffer swap and ev_BufEmpty
 * for tsk_reader. The return value of SOUND_isr() (RC_ERROR_BUFFER_EMTPY on an underrun)
 * is not evaluated here; the driver counts the underruns itself (N2).
 *
 * \note ISR category 2. Called 22050 times per second, so nothing else may be added here.
 */
ISR2(isr_Timer)
{
    /* N9: old PWM test, commented out
    static uint32_t a = 0;
    a = (a+1) % 2;
    SOUND_PWM_WriteCompare(a*255);
    */
 
    SOUND_isr();                                                           /* N2: result not used */
}

//Debugging aid
#if 0                                                                      /* N10 */
TASK(tsk_sd)
{
    char        sdVolName[10];
    FS_DIR    * pDir;
    FS_DIRENT * pDirEnt;
    FS_FILE   * pFile;
    char        entryName[64];
    char        firstWAV[64];
    char        fullPath[80];
    char        logBuf[80];
    U8          attr;
    U32         fileSize;
    U8          fileCount = 0u;
    WAVFile     wavFile;
 
    firstWAV[0] = '\0';  
    
    /* ------------------------------------------------------------------ */
    /* INC1 - Scan SD Card for Directory and files with size and count    */
    /* ------------------------------------------------------------------ */
    
    /* Step 1 - Init emFile */
    //UART_LOG_PutString("\r\n[SD] Init emFile...\n");
    FS_Init();
    //UART_LOG_PutString("\r\n[SD] emFile OK\n");
 
    /* Step 2 - Get volume name */
    if (0 != FS_GetVolumeName(0u, &sdVolName[0], 9u))
    {
        //sprintf(logBuf, "\r\n[SD] Volume: %s\n", sdVolName);
        //UART_LOG_PutString(logBuf);
    }
    else
    {
        UART_LOG_PutString("\r\n[SD] Failed to get volume name\n");
    }
 
    /* Step 3 - Open root directory */
    UART_LOG_PutString("\r\n[SD] Scanning root directory:\n");
    pDir = FS_OpenDir("\\");
    if (pDir != 0)
    {
        /* Step 4 - Read each entry */
        while (1)
        {
            pDirEnt = FS_ReadDir(pDir);
            if (pDirEnt == 0) break;
 
            /* Get name, size and attributes using emFile helper functions */
            FS_DirEnt2Name(pDirEnt, entryName);
            FS_DirEnt2Attr(pDirEnt, &attr);
            fileSize = FS_DirEnt2Size(pDirEnt);
 
            if (attr & FS_ATTR_DIRECTORY)
            {
                sprintf(logBuf, "\r\n[SD] <DIR>  %s\n", entryName);
            }
            else
            {
                sprintf(logBuf, "\r\n[SD] FILE  %-12s %lu bytes\n",
                        entryName, fileSize);
                fileCount++;
                /* FIX 2 - populate firstWAV from scan results */
                if (firstWAV[0] == '\0' && (strstr(entryName, ".WAV") != 0 || strstr(entryName, ".wav") != 0))
                {
                    strcpy(firstWAV, entryName);
                }
            }
            UART_LOG_PutString(logBuf);
        }
 
        /* Step 5 - Close directory */
        FS_CloseDir(pDir);
 
        sprintf(logBuf, "\r\n[SD] Done. %u file(s) found.\n", fileCount);
        UART_LOG_PutString(logBuf);
    }
    else
    {
        UART_LOG_PutString("\r\n[SD] ERROR: cannot open root directory\n");
        TerminateTask();
    }  
    
    /* ------------------------------------------------------------------ */
    /* INC2 - open first WAV file and parse header                        */
    /* ------------------------------------------------------------------ */
    
    if (firstWAV[0] == '\0')
    {
        UART_LOG_PutString("\r\n[WAV] No WAV file found\n");
        TerminateTask();
    }
 
    sprintf(logBuf, "\r\n[WAV] Opening: %s\n", firstWAV);
    UART_LOG_PutString(logBuf);
 
    /* T1 confirmed: plain filename works directly with FS_FOpen */
    pFile = FS_FOpen(firstWAV, "r");
    if (pFile == 0)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: FS_FOpen failed\n");
        TerminateTask();
    }
    UART_LOG_PutString("\r\n[WAV] FS_FOpen OK\n");
 
    /* Parse header - reads 44 bytes, validates, prints all fields 
    if (WAV_ParseHeader(pFile, &wavFile) == RC_SUCCESS)
    {
        //WAV_PrintHeader(&wavFile);
        //UART_LOG_PutString("\r\n[WAV] ready for streaming\n"); 
    } */
    
    /* -------------------------------------------------------- */
    /* 4B2 - Read first block of PCM into dual buffer           */
    /* -------------------------------------------------------- */
    if (WAV_ParseHeader(pFile, &wavFile) == RC_SUCCESS)
    {
        WAV_PrintHeader(&wavFile);
        UART_LOG_PutString("\r\n[WAV] Header OK\n");
 
        /* File pointer now at byte 78 - first PCM sample      */
        /* Read first 512 bytes into read buffer               */
        static uint8_t readBuf[512];
        U32 bytesRead = FS_Read(pFile, readBuf, 512u);
 
        sprintf(logBuf, "\r\n[4B2] FS_Read: %lu bytes\n", bytesRead);
        UART_LOG_PutString(logBuf);
 
        /* Fill dual buffer */
        if (SOUND_FillBuffer(readBuf, (uint16_t)bytesRead) == RC_SUCCESS)
        {
            UART_LOG_PutString("\r\n[4B2] FillBuffer OK\n");
            SOUND_SwapBuffers();
            UART_LOG_PutString("\r\n[4B2] SwapBuffers OK\n");
        }
        else
        {
            UART_LOG_PutString("\r\n[4B2] ERROR: FillBuffer failed\n");
        }
 
        /* Print first 32 bytes to verify PCM data             */
        /* For 8-bit unsigned PCM silence = 0x80               */
        /* Music samples should vary around 0x80               */
        UART_LOG_PutString("\r\n[4B2] First 32 bytes: ");
        uint8_t k;
        for (k = 0u; k < 32u; k++)
        {
            sprintf(logBuf, "%02X ", readBuf[k]);
            UART_LOG_PutString(logBuf);
        }
        UART_LOG_PutString("\n");
    }
    
    /*  The bytes should vary around 0x80 (128 decimal) which is the mid-rail for 8-bit unsigned PCM. 
        If you see all 0x80 - the file pointer is wrong. If you see all 0x00 - the buffer was not filled. 
        If you see values like 7E 80 82 7F varying around 0x80 - the PCM data is correct. */
    else
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: invalid header\n");
    }
 
    FS_FClose(pFile);
    TerminateTask();
}
 
// Debug code for File path name
TASK(tsk_sd)
{
    char        sdVolName[10];
    FS_DIR    * pDir;
    FS_DIRENT * pDirEnt;
    FS_FILE   * pFile;
    char        entryName[64];
    char        firstWAV[64];
    char        testPath[80];
    char        logBuf[80];
    U8          attr;
    U32         fileSize;
    U8          fileCount = 0u;
 
    /* Init */
    UART_LOG_PutString("\r\n[SD] Init emFile...\n");
    FS_Init();
    UART_LOG_PutString("\r\n[SD] emFile OK\n");
 
    /* Get volume name */
    if (0 != FS_GetVolumeName(0u, &sdVolName[0], 9u))
    {
        sprintf(logBuf, "\r\n[SD] Volume: '%s'\n", sdVolName);
        UART_LOG_PutString(logBuf);
    }
 
    /* Scan and find first WAV */
    firstWAV[0] = '\0';
    pDir = FS_OpenDir("\\");
    if (pDir != 0)
    {
        while (1)
        {
            pDirEnt = FS_ReadDir(pDir);
            if (pDirEnt == 0) break;
 
            FS_DirEnt2Name(pDirEnt, entryName);
            FS_DirEnt2Attr(pDirEnt, &attr);
            fileSize = FS_DirEnt2Size(pDirEnt);
 
            if (attr & FS_ATTR_DIRECTORY)
            {
                sprintf(logBuf, "\r\n[SD] <DIR>  %s\n", entryName);
            }
            else
            {
                sprintf(logBuf, "\r\n[SD] FILE  %-12s %lu bytes\n",
                        entryName, fileSize);
                fileCount++;
 
                if (firstWAV[0] == '\0' &&
                    (strstr(entryName, ".WAV") != 0 ||
                     strstr(entryName, ".wav") != 0))
                {
                    strcpy(firstWAV, entryName);
                }
            }
            UART_LOG_PutString(logBuf);
        }
        FS_CloseDir(pDir);
        sprintf(logBuf, "\r\n[SD] Done. %u file(s) found.\n", fileCount);
        UART_LOG_PutString(logBuf);
    }
 
    if (firstWAV[0] == '\0')
    {
        UART_LOG_PutString("\r\n[WAV] No WAV file found\n");
        TerminateTask();
    }
 
    sprintf(logBuf, "\r\n[WAV] First WAV found: '%s'\n", firstWAV);
    UART_LOG_PutString(logBuf);
 
    /* ------------------------------------------------------------------ */
    /* Try all path formats using the actual filename from scan            */
    /* ------------------------------------------------------------------ */
 
    /* Test 1 - plain name */
    sprintf(testPath, "%s", firstWAV);
    pFile = FS_FOpen(testPath, "r");
    sprintf(logBuf, "\r\nT1 '%s' : %s\n", testPath, pFile ? "OK" : "FAIL");
    UART_LOG_PutString(logBuf);
    if (pFile) { FS_FClose(pFile); TerminateTask(); }
 
    /* Test 2 - volume + name */
    sprintf(testPath, "%s%s", sdVolName, firstWAV);
    pFile = FS_FOpen(testPath, "r");
    sprintf(logBuf, "\r\nT2 '%s' : %s\n", testPath, pFile ? "OK" : "FAIL");
    UART_LOG_PutString(logBuf);
    if (pFile) { FS_FClose(pFile); TerminateTask(); }
 
    /* Test 3 - backslash + name */
    sprintf(testPath, "\\%s", firstWAV);
    pFile = FS_FOpen(testPath, "r");
    sprintf(logBuf, "\r\nT3 '%s' : %s\n", testPath, pFile ? "OK" : "FAIL");
    UART_LOG_PutString(logBuf);
    if (pFile) { FS_FClose(pFile); TerminateTask(); }
 
    /* Test 4 - volume + backslash + name */
    sprintf(testPath, "%s\\%s", sdVolName, firstWAV);
    pFile = FS_FOpen(testPath, "r");
    sprintf(logBuf, "\r\nT4 '%s' : %s\n", testPath, pFile ? "OK" : "FAIL");
    UART_LOG_PutString(logBuf);
    if (pFile) { FS_FClose(pFile); TerminateTask(); }
 
    UART_LOG_PutString("\r\n[WAV] All formats failed\n");
    TerminateTask();
}
 
/* ------------------------------------------------------------------ */
/* INC3 - tsk_background                                              */
/* Prints ISR tick count every 1 second                               */
/* Stopped : 0 ticks/s                                                */
/* Playing : ~22050 ticks/s                                           */
/* ------------------------------------------------------------------ */
TASK(tsk_background)
{
    static uint32_t lastCount = 0u;
    char tickBuf[56];
 
    while(1)
    {
        CyDelay(1000u);
 
        uint32_t current = SOUND_isrTickCount;
        uint32_t delta   = current - lastCount;
        lastCount        = current;
 
        //sprintf(tickBuf, "\r\n[ISR] ticks/s: %lu (expected 22050)\n", delta); // 22050 ticks x (1000ms / 642ms) = 34,349 ticks/s measured
        //UART_LOG_PutString(tickBuf);
        
        /* The CyDelay(1000) is not 1 second - the UART print in tsk_background takes ~358 ms extra time, 
        making the measurement window only ~642 ms instead of 1000 ms. So 22050 ticks in 642 ms = 34,349 ticks/s */
    }
}
#endif
 
/*
* Notes:
*
* N1 - Start-up order in tsk_init. The drivers are started first and EE_system_init() comes after
*      them: it reconfigures the ISRs with the OS parameters and must be called after the hardware
*      driver initialisation (original comment). SOUND_Init() (both buffers silent, sample timer
*      stopped, see sound.h) is done before isr_Timer can run. TFT_init() comes before MP3_Init()
*      (INC 10) so that the display is ready when the song list is drawn. MP3_Init() is last
*      because it scans the SD card and draws the TFT header. The alarms are started and the other
*      tasks are activated only after that, so tsk_control and tsk_reader never see a player
*      that is not initialised.
*
* N2 - Return values that are not evaluated here. SOUND_Init(): always RC_SUCCESS. MP3_Init():
*      RC_SUCCESS or RC_ERROR_OPEN (a folder could not be opened); the failure is already reported
*      by the player itself (UART log, error flag, TFT message), so there is nothing more to do in
*      tsk_init. MP3_ProcessEvent(): RC_ERROR_BAD_PARAM or RC_ERROR_INVALID_STATE for an event that
*      was ignored, which is normal operation. The result is stored in the local variable "result"
*      of tsk_control and never read: gcc reports it as unused, MISRA C:2012 Rule 2.2 and 17.7.
*      "(void)MP3_ProcessEvent(nextEvent);" would show that this is intended. SOUND_isr(): returns
*      RC_ERROR_BUFFER_EMTPY on an underrun tick; the driver counts it itself and an ISR cannot
*      react to it. Left unchanged, because INC 11D changes comments only.
*
* N3 - GetAndClearHighestEvent(). (a) The function returns the lowest bit number that is set;
*      "highest" means the highest priority. If several events arrive together they are handled
*      in the order of the event bits (numbering in the OIL file), not in the order of arrival.
*      (b) It works on the copy "ev" of the caller only. The events of the OS were already
*      cleared by ClearEvent(ev) in tsk_control. (c) "1<<i" is a shift of a signed int. If
*      EventMaskType has 32 bit, bit 31 would be a signed overflow (MISRA C:2012 Rule 10.1 and 12.2,
*      template rule 3: do not mix signed and unsigned). It is not reached as long as no event uses
*      bit 31 (tsk_control waits for 5 events, tsk_reader for 1). A code change should use "1u<<i".
*
* N4 - Event handling in tsk_control. WaitEvent() blocks until one of the five events is set,
*      GetEvent() reads the set events and ClearEvent(ev) clears exactly these bits. An event that
*      arrives after GetEvent() stays set and ends the next WaitEvent(), so nothing is lost.
*      Events are flags and not counters: two equal events before tsk_control runs (two quick
*      presses of one button, or a missed 1 s tick) count as one. Different events are handled in
*      one pass in the bit order of N3. isr_Button sets one event per pressed button, there is no
*      debouncing in this file.
*
* N5 - Time base and alarms. EE_systick_set_period(MILLISECONDS_TO_TICKS(1, BCLK__BUS_CLK__HZ))
*      makes one tick of cnt_systick one millisecond. alrm_1s starts after 1000 ms and repeats every
*      1000 ms (ev_1s for the play time of the player). alrm_display starts after 1100 ms and repeats
*      every 200 ms (tsk_display). The display times 1100, 1300, ... are odd multiples of 100 ms and
*      the 1 s ticks are even multiples, so both alarms never expire in the same millisecond. The
*      numbers are literals and not #defines (template rule 2: no magic numbers). What an alarm
*      does when it expires (event or task activation) is set in the OIL file.
*
* N6 - Tasks, priorities and shared data. Priority and type of the tasks are set in the OIL file
*      and cannot be seen here. mp3.c N7: tsk_control and tsk_reader share the state of the reader,
*      which is only safe if tsk_control cannot interrupt a running MP3_Reader() (reader has the
*      higher priority, or both are non-preemptive). tsk_display reads the player state while
*      tsk_control may change it; a redraw during a change is corrected by the next run 200 ms
*      later, because the display compares the player state with what is drawn on every run
*      (mp3.c N9). tsk_background must have the lowest priority, so it only uses the idle time.
*
* N7 - Code that does not return or is not reached. StartOS() does not return, the for(;;) loop
*      with the nop below it in main() is a safety net. In tsk_reader the TerminateTask() after the
*      endless while(1) is never reached (MISRA C:2012 Rule 2.1); it is kept because an OSEK task
*      has to end with TerminateTask() or ChainTask(). tsk_control and tsk_background are endless
*      loops. tsk_init and tsk_display end with TerminateTask() after every run.
*
* N8 - Rules not met, left unchanged because only comments were changed (MISRA C:2012 and
*      template). (a) Rule 8.2: "int main()" and "void unhandledException()" should have (void).
*      (b) GetAndClearHighestEvent() has external linkage and no prototype in this file; if it is
*      only used here it should be static with a prototype (Rule 8.4, 8.7). (c) Rule 1.2 and
*      Dir 4.3: asm("nop") and __asm("bkpt") are compiler specific assembler statements. (d) Shift
*      of a signed value: N3. (e) Result not used: N2. (f) Commented-out code, Dir 4.4: N9.
*      (g) Magic numbers 1000, 1100, 200: N5. (h) tsk_display calls MP3__showSong(); the prefix
*      MP3__ marks a private function (template rule 1) but the function is public in mp3.h.
*      A rename to MP3_showSong() would fit the rule; it needs a change in mp3.c and mp3.h too.
*
* N9 - Commented-out code that was kept (Dir 4.4): soundTest1() and soundTest2() (they use
*      SOUND_Records, which the sound driver does not have any more), the two "Test" lines in
*      tsk_init (the first one, "RC_t SOUND_StartPlay();", is a declaration and not a call), the
*      square wave test in isr_Timer and the old UART line in tsk_display. They are candidates
*      for removal in a code increment.
*
* N10 - The "#if 0" block at the end of the file is not compiled. It holds the old debug tasks
*      tsk_sd (SD card scan, WAV header test, path format test) and the INC 3 version of
*      tsk_background (ISR tick count). It cannot be switched on as it is: it defines
*      tsk_background a second time and uses SOUND_isrTickCount, which sound.h does not declare
*      any more (INC 11C). The only change in the block is that the long dashes in some comments
*      are replaced by "-" (ASCII only). It could be moved to a separate debug file or removed.
*
* N11 - Button description corrected. The comment that was at the top of the old file said
*      "Button 1 : Increase Volume, Next Song" and "Button 2 : Decrease Volume, Prev Song". The
*      code in MP3_ProcessEvent() does it the other way round for the songs: while stopped,
*      Button1 is the previous song and Button2 is the next song; while playing, Button1 is
*      volume + and Button2 is volume -. Button3 is play / pause / resume, Button4 is stop. The
*      table in the file header follows the code. If the old text was the wanted behaviour, then
*      MP3_ProcessEvent() in mp3.c has to be changed, not this comment.
*/
 
/* [main.c] END OF FILE */
