/*******************************************************************************
* File Name: SOUND_PWM_PM.c
* Version 3.30
*
* Description:
*  This file provides the power management source code to API for the
*  PWM.
*
* Note:
*
********************************************************************************
* Copyright 2008-2014, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "SOUND_PWM.h"

static SOUND_PWM_backupStruct SOUND_PWM_backup;


/*******************************************************************************
* Function Name: SOUND_PWM_SaveConfig
********************************************************************************
*
* Summary:
*  Saves the current user configuration of the component.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  SOUND_PWM_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void SOUND_PWM_SaveConfig(void) 
{

    #if(!SOUND_PWM_UsingFixedFunction)
        #if(!SOUND_PWM_PWMModeIsCenterAligned)
            SOUND_PWM_backup.PWMPeriod = SOUND_PWM_ReadPeriod();
        #endif /* (!SOUND_PWM_PWMModeIsCenterAligned) */
        SOUND_PWM_backup.PWMUdb = SOUND_PWM_ReadCounter();
        #if (SOUND_PWM_UseStatus)
            SOUND_PWM_backup.InterruptMaskValue = SOUND_PWM_STATUS_MASK;
        #endif /* (SOUND_PWM_UseStatus) */

        #if(SOUND_PWM_DeadBandMode == SOUND_PWM__B_PWM__DBM_256_CLOCKS || \
            SOUND_PWM_DeadBandMode == SOUND_PWM__B_PWM__DBM_2_4_CLOCKS)
            SOUND_PWM_backup.PWMdeadBandValue = SOUND_PWM_ReadDeadTime();
        #endif /*  deadband count is either 2-4 clocks or 256 clocks */

        #if(SOUND_PWM_KillModeMinTime)
             SOUND_PWM_backup.PWMKillCounterPeriod = SOUND_PWM_ReadKillTime();
        #endif /* (SOUND_PWM_KillModeMinTime) */

        #if(SOUND_PWM_UseControl)
            SOUND_PWM_backup.PWMControlRegister = SOUND_PWM_ReadControlRegister();
        #endif /* (SOUND_PWM_UseControl) */
    #endif  /* (!SOUND_PWM_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_PWM_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration of the component.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  SOUND_PWM_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void SOUND_PWM_RestoreConfig(void) 
{
        #if(!SOUND_PWM_UsingFixedFunction)
            #if(!SOUND_PWM_PWMModeIsCenterAligned)
                SOUND_PWM_WritePeriod(SOUND_PWM_backup.PWMPeriod);
            #endif /* (!SOUND_PWM_PWMModeIsCenterAligned) */

            SOUND_PWM_WriteCounter(SOUND_PWM_backup.PWMUdb);

            #if (SOUND_PWM_UseStatus)
                SOUND_PWM_STATUS_MASK = SOUND_PWM_backup.InterruptMaskValue;
            #endif /* (SOUND_PWM_UseStatus) */

            #if(SOUND_PWM_DeadBandMode == SOUND_PWM__B_PWM__DBM_256_CLOCKS || \
                SOUND_PWM_DeadBandMode == SOUND_PWM__B_PWM__DBM_2_4_CLOCKS)
                SOUND_PWM_WriteDeadTime(SOUND_PWM_backup.PWMdeadBandValue);
            #endif /* deadband count is either 2-4 clocks or 256 clocks */

            #if(SOUND_PWM_KillModeMinTime)
                SOUND_PWM_WriteKillTime(SOUND_PWM_backup.PWMKillCounterPeriod);
            #endif /* (SOUND_PWM_KillModeMinTime) */

            #if(SOUND_PWM_UseControl)
                SOUND_PWM_WriteControlRegister(SOUND_PWM_backup.PWMControlRegister);
            #endif /* (SOUND_PWM_UseControl) */
        #endif  /* (!SOUND_PWM_UsingFixedFunction) */
    }


/*******************************************************************************
* Function Name: SOUND_PWM_Sleep
********************************************************************************
*
* Summary:
*  Disables block's operation and saves the user configuration. Should be called
*  just prior to entering sleep.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  SOUND_PWM_backup.PWMEnableState:  Is modified depending on the enable
*  state of the block before entering sleep mode.
*
*******************************************************************************/
void SOUND_PWM_Sleep(void) 
{
    #if(SOUND_PWM_UseControl)
        if(SOUND_PWM_CTRL_ENABLE == (SOUND_PWM_CONTROL & SOUND_PWM_CTRL_ENABLE))
        {
            /*Component is enabled */
            SOUND_PWM_backup.PWMEnableState = 1u;
        }
        else
        {
            /* Component is disabled */
            SOUND_PWM_backup.PWMEnableState = 0u;
        }
    #endif /* (SOUND_PWM_UseControl) */

    /* Stop component */
    SOUND_PWM_Stop();

    /* Save registers configuration */
    SOUND_PWM_SaveConfig();
}


/*******************************************************************************
* Function Name: SOUND_PWM_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration. Should be called just after
*  awaking from sleep.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  SOUND_PWM_backup.pwmEnable:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void SOUND_PWM_Wakeup(void) 
{
     /* Restore registers values */
    SOUND_PWM_RestoreConfig();

    if(SOUND_PWM_backup.PWMEnableState != 0u)
    {
        /* Enable component's operation */
        SOUND_PWM_Enable();
    } /* Do nothing if component's block was disabled before */

}


/* [] END OF FILE */
