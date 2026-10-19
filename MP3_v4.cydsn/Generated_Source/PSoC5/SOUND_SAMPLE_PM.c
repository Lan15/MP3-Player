/*******************************************************************************
* File Name: SOUND_SAMPLE_PM.c  
* Version 3.0
*
*  Description:
*    This file provides the power management source code to API for the
*    Counter.  
*
*   Note:
*     None
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "SOUND_SAMPLE.h"

static SOUND_SAMPLE_backupStruct SOUND_SAMPLE_backup;


/*******************************************************************************
* Function Name: SOUND_SAMPLE_SaveConfig
********************************************************************************
* Summary:
*     Save the current user configuration
*
* Parameters:  
*  void
*
* Return: 
*  void
*
* Global variables:
*  SOUND_SAMPLE_backup:  Variables of this global structure are modified to 
*  store the values of non retention configuration registers when Sleep() API is 
*  called.
*
*******************************************************************************/
void SOUND_SAMPLE_SaveConfig(void) 
{
    #if (!SOUND_SAMPLE_UsingFixedFunction)

        SOUND_SAMPLE_backup.CounterUdb = SOUND_SAMPLE_ReadCounter();

        #if(!SOUND_SAMPLE_ControlRegRemoved)
            SOUND_SAMPLE_backup.CounterControlRegister = SOUND_SAMPLE_ReadControlRegister();
        #endif /* (!SOUND_SAMPLE_ControlRegRemoved) */

    #endif /* (!SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:  
*  void
*
* Return: 
*  void
*
* Global variables:
*  SOUND_SAMPLE_backup:  Variables of this global structure are used to 
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void SOUND_SAMPLE_RestoreConfig(void) 
{      
    #if (!SOUND_SAMPLE_UsingFixedFunction)

       SOUND_SAMPLE_WriteCounter(SOUND_SAMPLE_backup.CounterUdb);

        #if(!SOUND_SAMPLE_ControlRegRemoved)
            SOUND_SAMPLE_WriteControlRegister(SOUND_SAMPLE_backup.CounterControlRegister);
        #endif /* (!SOUND_SAMPLE_ControlRegRemoved) */

    #endif /* (!SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_Sleep
********************************************************************************
* Summary:
*     Stop and Save the user configuration
*
* Parameters:  
*  void
*
* Return: 
*  void
*
* Global variables:
*  SOUND_SAMPLE_backup.enableState:  Is modified depending on the enable 
*  state of the block before entering sleep mode.
*
*******************************************************************************/
void SOUND_SAMPLE_Sleep(void) 
{
    #if(!SOUND_SAMPLE_ControlRegRemoved)
        /* Save Counter's enable state */
        if(SOUND_SAMPLE_CTRL_ENABLE == (SOUND_SAMPLE_CONTROL & SOUND_SAMPLE_CTRL_ENABLE))
        {
            /* Counter is enabled */
            SOUND_SAMPLE_backup.CounterEnableState = 1u;
        }
        else
        {
            /* Counter is disabled */
            SOUND_SAMPLE_backup.CounterEnableState = 0u;
        }
    #else
        SOUND_SAMPLE_backup.CounterEnableState = 1u;
        if(SOUND_SAMPLE_backup.CounterEnableState != 0u)
        {
            SOUND_SAMPLE_backup.CounterEnableState = 0u;
        }
    #endif /* (!SOUND_SAMPLE_ControlRegRemoved) */
    
    SOUND_SAMPLE_Stop();
    SOUND_SAMPLE_SaveConfig();
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration
*  
* Parameters:  
*  void
*
* Return: 
*  void
*
* Global variables:
*  SOUND_SAMPLE_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void SOUND_SAMPLE_Wakeup(void) 
{
    SOUND_SAMPLE_RestoreConfig();
    #if(!SOUND_SAMPLE_ControlRegRemoved)
        if(SOUND_SAMPLE_backup.CounterEnableState == 1u)
        {
            /* Enable Counter's operation */
            SOUND_SAMPLE_Enable();
        } /* Do nothing if Counter was disabled before */    
    #endif /* (!SOUND_SAMPLE_ControlRegRemoved) */
    
}


/* [] END OF FILE */
