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
    
    
    
    //Test
    //RC_t SOUND_StartPlay();
    //SOUND_SetSong(SOUND_Records[0].m_data, SOUND_Records[0].m_size);

    
    
    //Reconfigure ISRs with OS parameters.
    //This line MUST be called after the hardware driver initialisation!
    EE_system_init();
    
    UART_LOG_PutString("MP3 Player started...\n");

    //Application Init
    MP3_Init();
    
    //Start the alarm with 1000ms cycle time
    SetRelAlarm(alrm_1s,1000,1000);
 
    ActivateTask(tsk_control);
    ActivateTask(tsk_background);
    
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


/* [] END OF FILE */
