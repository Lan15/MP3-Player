/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "global.h"

#include "button.h"
#include "led.h"
#include "seven.h"
#include "tft.h"

#include "sound.h"

#include "mp3.h"

//HMI via UART
// Button 1 : Increase Volume, Next Song
// Button 2 : Decrease Volume, Prev Song
// Button 3 : Play, Pause
// Button 4 : Stop

//ISR which will increment the systick counter every ms
ISR(systick_handler)
{
    CounterTick(cnt_systick);
}

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

int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    //Set systick period to 1 ms. Enable the INT and start it.
	EE_systick_set_period(MILLISECONDS_TO_TICKS(1, BCLK__BUS_CLK__HZ));
	EE_systick_enable_int();
	EE_systick_start();
   
    // Start Operating System
    StartOS(OSDEFAULTAPPMODE);
    for(;;)
    {
        asm("nop");
    }
        
}

void unhandledException()
{
    //Ooops, something terrible happened....check the call stack to see how we got here...
    __asm("bkpt");
}

EventMaskType GetAndClearHighestEvent(EventMaskRefType ev)
{
    EventMaskType result = 0;
    EventMaskType lev = *ev;
    
    for (uint8_t i = 0; i < sizeof(EventMaskType) * 8; i++)
    {
        if (0 != (lev & (1<<i)))
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

TASK(tsk_init)
{ 
    //Init MCAL Drivers
    UART_LOG_Start();
    SOUND_SAMPLE_Start();
    
    LED_Init();
    SEVEN_Init();
    TFT_init(); /* INC 10 — init TFT before MP3_Init so display is ready for song list */
    SOUND_Init();
    
    //Test
    //RC_t SOUND_StartPlay();
    //SOUND_SetSong(SOUND_Records[0].m_data, SOUND_Records[0].m_size);
    
    //Reconfigure ISRs with OS parameters.
    //This line MUST be called after the hardware driver initialisation!
    EE_system_init();

    //Application Init
    MP3_Init();
    
    //Start the alarm with 1000ms cycle time
    SetRelAlarm(alrm_1s,1000,1000);
    SetRelAlarm(alrm_display,1100,200);   /* INC 10 */

    ActivateTask(tsk_reader);
    ActivateTask(tsk_control);
    ActivateTask(tsk_background); 
    
    TerminateTask();
    
}

TASK(tsk_control)
{    
    EventMaskType ev = 0;
    EventMaskType nextEvent = 0;
    
    while(1)
    {
        //Wait, read and clear the event
        WaitEvent(ev_1s | ev_Button1 | ev_Button2 | ev_Button3 | ev_Button4 );
        GetEvent(tsk_control,&ev);
        ClearEvent(ev);
    
        //Process all events from the mask
        while(0 != ev)
        {
            nextEvent = GetAndClearHighestEvent(&ev);
            RC_t result = MP3_ProcessEvent(nextEvent);
        }
    }
}

TASK(tsk_reader)
{
    while (1)
    {
        WaitEvent(ev_BufEmpty);
        ClearEvent(ev_BufEmpty);
        
        //woke - calling MP3_Reader
        MP3_Reader();
    }

    TerminateTask();
}

/* INC 10 - dedicated TFT display task */
TASK(tsk_display)
{
    //UART_LOG_PutString("\r\n[DISPLAY] tsk_display started\n");

    MP3__showSong();          // renders song list to TFT + UART

    TerminateTask();
}

TASK(tsk_background)
{
    while(1)
    {
        //do something with low prioroty
        asm("nop");
    }
}

/********************************************************************************
 * ISR Definitions
 ********************************************************************************/

ISR2(isr_Button)
{
    //Set the button bit
    if (BUTTON_IsPressed(BUTTON_1)) SetEvent(tsk_control, ev_Button1);
    if (BUTTON_IsPressed(BUTTON_2)) SetEvent(tsk_control, ev_Button2);
    if (BUTTON_IsPressed(BUTTON_3)) SetEvent(tsk_control, ev_Button3);
    if (BUTTON_IsPressed(BUTTON_4)) SetEvent(tsk_control, ev_Button4);
}

ISR2(isr_Timer)
{
    /*
    static uint32_t a = 0;
    a = (a+1) % 2;
    SOUND_PWM_WriteCompare(a*255);
    */
    
    SOUND_isr();
}

//Debugging aid
#if 0
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
    /* INC1 — Scan SD Card for Directory and files with size and count    */
    /* ------------------------------------------------------------------ */
    
    /* Step 1 — Init emFile */
    //UART_LOG_PutString("\r\n[SD] Init emFile...\n");
    FS_Init();
    //UART_LOG_PutString("\r\n[SD] emFile OK\n");
 
    /* Step 2 — Get volume name */
    if (0 != FS_GetVolumeName(0u, &sdVolName[0], 9u))
    {
        //sprintf(logBuf, "\r\n[SD] Volume: %s\n", sdVolName);
        //UART_LOG_PutString(logBuf);
    }
    else
    {
        UART_LOG_PutString("\r\n[SD] Failed to get volume name\n");
    }
 
    /* Step 3 — Open root directory */
    UART_LOG_PutString("\r\n[SD] Scanning root directory:\n");
    pDir = FS_OpenDir("\\");
    if (pDir != 0)
    {
        /* Step 4 — Read each entry */
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
                /* FIX 2 — populate firstWAV from scan results */
                if (firstWAV[0] == '\0' && (strstr(entryName, ".WAV") != 0 || strstr(entryName, ".wav") != 0))
                {
                    strcpy(firstWAV, entryName);
                }
            }
            UART_LOG_PutString(logBuf);
        }
 
        /* Step 5 — Close directory */
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
    /* INC2 — open first WAV file and parse header                        */
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
 
    /* Parse header — reads 44 bytes, validates, prints all fields 
    if (WAV_ParseHeader(pFile, &wavFile) == RC_SUCCESS)
    {
        //WAV_PrintHeader(&wavFile);
        //UART_LOG_PutString("\r\n[WAV] ready for streaming\n"); 
    } */
    
    /* -------------------------------------------------------- */
    /* 4B2 — Read first block of PCM into dual buffer           */
    /* -------------------------------------------------------- */
    if (WAV_ParseHeader(pFile, &wavFile) == RC_SUCCESS)
    {
        WAV_PrintHeader(&wavFile);
        UART_LOG_PutString("\r\n[WAV] Header OK\n");

        /* File pointer now at byte 78 — first PCM sample      */
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

    /* Test 1 — plain name */
    sprintf(testPath, "%s", firstWAV);
    pFile = FS_FOpen(testPath, "r");
    sprintf(logBuf, "\r\nT1 '%s' : %s\n", testPath, pFile ? "OK" : "FAIL");
    UART_LOG_PutString(logBuf);
    if (pFile) { FS_FClose(pFile); TerminateTask(); }

    /* Test 2 — volume + name */
    sprintf(testPath, "%s%s", sdVolName, firstWAV);
    pFile = FS_FOpen(testPath, "r");
    sprintf(logBuf, "\r\nT2 '%s' : %s\n", testPath, pFile ? "OK" : "FAIL");
    UART_LOG_PutString(logBuf);
    if (pFile) { FS_FClose(pFile); TerminateTask(); }

    /* Test 3 — backslash + name */
    sprintf(testPath, "\\%s", firstWAV);
    pFile = FS_FOpen(testPath, "r");
    sprintf(logBuf, "\r\nT3 '%s' : %s\n", testPath, pFile ? "OK" : "FAIL");
    UART_LOG_PutString(logBuf);
    if (pFile) { FS_FClose(pFile); TerminateTask(); }

    /* Test 4 — volume + backslash + name */
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
 
        //sprintf(tickBuf, "\r\n[ISR] ticks/s: %lu (expected 22050)\n", delta); // 22050 ticks × (1000ms / 642ms) = 34,349 ticks/s measured
        //UART_LOG_PutString(tickBuf);
        
        /* The CyDelay(1000) is not 1 second - the UART print in tsk_background takes ~358 ms extra time, 
        making the measurement window only ~642 ms instead of 1000 ms. So 22050 ticks in 642 ms = 34,349 ticks/s */
    }
}
#endif

/* [main.c] END OF FILE */
