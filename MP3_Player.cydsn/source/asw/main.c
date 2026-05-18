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

#include "sound.h"
#include "sound_data.h"

#include "mp3.h"
#include "emFile.h"

#include "FS.h"
#include <stdio.h>

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

void soundTest1()
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
    SOUND_Init();
    
    //UART_LOG_PutString("[INIT] SOUND_Init OK\n");
    
    //Test
    //RC_t SOUND_StartPlay();
    //SOUND_SetSong(SOUND_Records[0].m_data, SOUND_Records[0].m_size);

    
    //Reconfigure ISRs with OS parameters.
    //This line MUST be called after the hardware driver initialisation!
    EE_system_init();
    
    //UART_LOG_PutString("MP3 Player started...\n");

    //Application Init
    /* Original MP3_Init - still uses flash songs */
    //emFile();
    MP3_Init();
    
    //Start the alarm with 1000ms cycle time
    SetRelAlarm(alrm_1s,1000,1000);

    ActivateTask(tsk_sd);
    //ActivateTask(tsk_control);
    ActivateTask(tsk_background);
    
    TerminateTask();
    
}

TASK(tsk_sd)
{
    char        sdVolName[10];
    FS_DIR    * pDir;
    FS_DIRENT * pDirEnt;
    char        entryName[64];
    char        logBuf[80];
    U8          attr;
    U32         fileSize;
    U8          fileCount = 0u;
 
    /* Step 1 — Init emFile */
    UART_LOG_PutString("[SD] Init emFile...\n");
    FS_Init();
    UART_LOG_PutString("[SD] emFile OK\n");
 
    /* Step 2 — Get volume name */
    if (0 != FS_GetVolumeName(0u, &sdVolName[0], 9u))
    {
        sprintf(logBuf, "[SD] Volume: %s\n", sdVolName);
        UART_LOG_PutString(logBuf);
    }
    else
    {
        UART_LOG_PutString("[SD] Failed to get volume name\n");
    }
 
    /* Step 3 — Open root directory */
    UART_LOG_PutString("[SD] Scanning root directory:\n");
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
                sprintf(logBuf, "[SD] <DIR>  %s\n", entryName);
            }
            else
            {
                sprintf(logBuf, "[SD] FILE  %-12s %lu bytes\n",
                        entryName, fileSize);
                fileCount++;
            }
            UART_LOG_PutString(logBuf);
        }
 
        /* Step 5 — Close directory */
        FS_CloseDir(pDir);
 
        sprintf(logBuf, "[SD] Done. %u file(s) found.\n", fileCount);
        UART_LOG_PutString(logBuf);
    }
    else
    {
        UART_LOG_PutString("[SD] ERROR: cannot open root directory\n");
    }
 
    TerminateTask();
}

TASK(tsk_control)
{
 
    static uint8_t count = 0;
    
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

/* [main.c] END OF FILE */
