/*******************************************************************************
* File Name: SOUND_SAMPLE.c  
* Version 3.0
*
*  Description:
*     The Counter component consists of a 8, 16, 24 or 32-bit counter with
*     a selectable period between 2 and 2^Width - 1.  
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

uint8 SOUND_SAMPLE_initVar = 0u;


/*******************************************************************************
* Function Name: SOUND_SAMPLE_Init
********************************************************************************
* Summary:
*     Initialize to the schematic state
* 
* Parameters:  
*  void  
*
* Return: 
*  void
*
*******************************************************************************/
void SOUND_SAMPLE_Init(void) 
{
        #if (!SOUND_SAMPLE_UsingFixedFunction && !SOUND_SAMPLE_ControlRegRemoved)
            uint8 ctrl;
        #endif /* (!SOUND_SAMPLE_UsingFixedFunction && !SOUND_SAMPLE_ControlRegRemoved) */
        
        #if(!SOUND_SAMPLE_UsingFixedFunction) 
            /* Interrupt State Backup for Critical Region*/
            uint8 SOUND_SAMPLE_interruptState;
        #endif /* (!SOUND_SAMPLE_UsingFixedFunction) */
        
        #if (SOUND_SAMPLE_UsingFixedFunction)
            /* Clear all bits but the enable bit (if it's already set for Timer operation */
            SOUND_SAMPLE_CONTROL &= SOUND_SAMPLE_CTRL_ENABLE;
            
            /* Clear the mode bits for continuous run mode */
            #if (CY_PSOC5A)
                SOUND_SAMPLE_CONTROL2 &= ((uint8)(~SOUND_SAMPLE_CTRL_MODE_MASK));
            #endif /* (CY_PSOC5A) */
            #if (CY_PSOC3 || CY_PSOC5LP)
                SOUND_SAMPLE_CONTROL3 &= ((uint8)(~SOUND_SAMPLE_CTRL_MODE_MASK));                
            #endif /* (CY_PSOC3 || CY_PSOC5LP) */
            /* Check if One Shot mode is enabled i.e. RunMode !=0*/
            #if (SOUND_SAMPLE_RunModeUsed != 0x0u)
                /* Set 3rd bit of Control register to enable one shot mode */
                SOUND_SAMPLE_CONTROL |= SOUND_SAMPLE_ONESHOT;
            #endif /* (SOUND_SAMPLE_RunModeUsed != 0x0u) */
            
            /* Set the IRQ to use the status register interrupts */
            SOUND_SAMPLE_CONTROL2 |= SOUND_SAMPLE_CTRL2_IRQ_SEL;
            
            /* Clear and Set SYNCTC and SYNCCMP bits of RT1 register */
            SOUND_SAMPLE_RT1 &= ((uint8)(~SOUND_SAMPLE_RT1_MASK));
            SOUND_SAMPLE_RT1 |= SOUND_SAMPLE_SYNC;     
                    
            /*Enable DSI Sync all all inputs of the Timer*/
            SOUND_SAMPLE_RT1 &= ((uint8)(~SOUND_SAMPLE_SYNCDSI_MASK));
            SOUND_SAMPLE_RT1 |= SOUND_SAMPLE_SYNCDSI_EN;

        #else
            #if(!SOUND_SAMPLE_ControlRegRemoved)
            /* Set the default compare mode defined in the parameter */
            ctrl = SOUND_SAMPLE_CONTROL & ((uint8)(~SOUND_SAMPLE_CTRL_CMPMODE_MASK));
            SOUND_SAMPLE_CONTROL = ctrl | SOUND_SAMPLE_DEFAULT_COMPARE_MODE;
            
            /* Set the default capture mode defined in the parameter */
            ctrl = SOUND_SAMPLE_CONTROL & ((uint8)(~SOUND_SAMPLE_CTRL_CAPMODE_MASK));
            
            #if( 0 != SOUND_SAMPLE_CAPTURE_MODE_CONF)
                SOUND_SAMPLE_CONTROL = ctrl | SOUND_SAMPLE_DEFAULT_CAPTURE_MODE;
            #else
                SOUND_SAMPLE_CONTROL = ctrl;
            #endif /* 0 != SOUND_SAMPLE_CAPTURE_MODE */ 
            
            #endif /* (!SOUND_SAMPLE_ControlRegRemoved) */
        #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
        
        /* Clear all data in the FIFO's */
        #if (!SOUND_SAMPLE_UsingFixedFunction)
            SOUND_SAMPLE_ClearFIFO();
        #endif /* (!SOUND_SAMPLE_UsingFixedFunction) */
        
        /* Set Initial values from Configuration */
        SOUND_SAMPLE_WritePeriod(SOUND_SAMPLE_INIT_PERIOD_VALUE);
        #if (!(SOUND_SAMPLE_UsingFixedFunction && (CY_PSOC5A)))
            SOUND_SAMPLE_WriteCounter(SOUND_SAMPLE_INIT_COUNTER_VALUE);
        #endif /* (!(SOUND_SAMPLE_UsingFixedFunction && (CY_PSOC5A))) */
        SOUND_SAMPLE_SetInterruptMode(SOUND_SAMPLE_INIT_INTERRUPTS_MASK);
        
        #if (!SOUND_SAMPLE_UsingFixedFunction)
            /* Read the status register to clear the unwanted interrupts */
            (void)SOUND_SAMPLE_ReadStatusRegister();
            /* Set the compare value (only available to non-fixed function implementation */
            SOUND_SAMPLE_WriteCompare(SOUND_SAMPLE_INIT_COMPARE_VALUE);
            /* Use the interrupt output of the status register for IRQ output */
            
            /* CyEnterCriticalRegion and CyExitCriticalRegion are used to mark following region critical*/
            /* Enter Critical Region*/
            SOUND_SAMPLE_interruptState = CyEnterCriticalSection();
            
            SOUND_SAMPLE_STATUS_AUX_CTRL |= SOUND_SAMPLE_STATUS_ACTL_INT_EN_MASK;
            
            /* Exit Critical Region*/
            CyExitCriticalSection(SOUND_SAMPLE_interruptState);
            
        #endif /* (!SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_Enable
********************************************************************************
* Summary:
*     Enable the Counter
* 
* Parameters:  
*  void  
*
* Return: 
*  void
*
* Side Effects: 
*   If the Enable mode is set to Hardware only then this function has no effect 
*   on the operation of the counter.
*
*******************************************************************************/
void SOUND_SAMPLE_Enable(void) 
{
    /* Globally Enable the Fixed Function Block chosen */
    #if (SOUND_SAMPLE_UsingFixedFunction)
        SOUND_SAMPLE_GLOBAL_ENABLE |= SOUND_SAMPLE_BLOCK_EN_MASK;
        SOUND_SAMPLE_GLOBAL_STBY_ENABLE |= SOUND_SAMPLE_BLOCK_STBY_EN_MASK;
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */  
        
    /* Enable the counter from the control register  */
    /* If Fixed Function then make sure Mode is set correctly */
    /* else make sure reset is clear */
    #if(!SOUND_SAMPLE_ControlRegRemoved || SOUND_SAMPLE_UsingFixedFunction)
        SOUND_SAMPLE_CONTROL |= SOUND_SAMPLE_CTRL_ENABLE;                
    #endif /* (!SOUND_SAMPLE_ControlRegRemoved || SOUND_SAMPLE_UsingFixedFunction) */
    
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_Start
********************************************************************************
* Summary:
*  Enables the counter for operation 
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
* Global variables:
*  SOUND_SAMPLE_initVar: Is modified when this function is called for the  
*   first time. Is used to ensure that initialization happens only once.
*
*******************************************************************************/
void SOUND_SAMPLE_Start(void) 
{
    if(SOUND_SAMPLE_initVar == 0u)
    {
        SOUND_SAMPLE_Init();
        
        SOUND_SAMPLE_initVar = 1u; /* Clear this bit for Initialization */        
    }
    
    /* Enable the Counter */
    SOUND_SAMPLE_Enable();        
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_Stop
********************************************************************************
* Summary:
* Halts the counter, but does not change any modes or disable interrupts.
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
* Side Effects: If the Enable mode is set to Hardware only then this function
*               has no effect on the operation of the counter.
*
*******************************************************************************/
void SOUND_SAMPLE_Stop(void) 
{
    /* Disable Counter */
    #if(!SOUND_SAMPLE_ControlRegRemoved || SOUND_SAMPLE_UsingFixedFunction)
        SOUND_SAMPLE_CONTROL &= ((uint8)(~SOUND_SAMPLE_CTRL_ENABLE));        
    #endif /* (!SOUND_SAMPLE_ControlRegRemoved || SOUND_SAMPLE_UsingFixedFunction) */
    
    /* Globally disable the Fixed Function Block chosen */
    #if (SOUND_SAMPLE_UsingFixedFunction)
        SOUND_SAMPLE_GLOBAL_ENABLE &= ((uint8)(~SOUND_SAMPLE_BLOCK_EN_MASK));
        SOUND_SAMPLE_GLOBAL_STBY_ENABLE &= ((uint8)(~SOUND_SAMPLE_BLOCK_STBY_EN_MASK));
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_SetInterruptMode
********************************************************************************
* Summary:
* Configures which interrupt sources are enabled to generate the final interrupt
*
* Parameters:  
*  InterruptsMask: This parameter is an or'd collection of the status bits
*                   which will be allowed to generate the counters interrupt.   
*
* Return: 
*  void
*
*******************************************************************************/
void SOUND_SAMPLE_SetInterruptMode(uint8 interruptsMask) 
{
    SOUND_SAMPLE_STATUS_MASK = interruptsMask;
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_ReadStatusRegister
********************************************************************************
* Summary:
*   Reads the status register and returns it's state. This function should use
*       defined types for the bit-field information as the bits in this
*       register may be permuteable.
*
* Parameters:  
*  void
*
* Return: 
*  (uint8) The contents of the status register
*
* Side Effects:
*   Status register bits may be clear on read. 
*
*******************************************************************************/
uint8   SOUND_SAMPLE_ReadStatusRegister(void) 
{
    return SOUND_SAMPLE_STATUS;
}


#if(!SOUND_SAMPLE_ControlRegRemoved)
/*******************************************************************************
* Function Name: SOUND_SAMPLE_ReadControlRegister
********************************************************************************
* Summary:
*   Reads the control register and returns it's state. This function should use
*       defined types for the bit-field information as the bits in this
*       register may be permuteable.
*
* Parameters:  
*  void
*
* Return: 
*  (uint8) The contents of the control register
*
*******************************************************************************/
uint8   SOUND_SAMPLE_ReadControlRegister(void) 
{
    return SOUND_SAMPLE_CONTROL;
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_WriteControlRegister
********************************************************************************
* Summary:
*   Sets the bit-field of the control register.  This function should use
*       defined types for the bit-field information as the bits in this
*       register may be permuteable.
*
* Parameters:  
*  void
*
* Return: 
*  (uint8) The contents of the control register
*
*******************************************************************************/
void    SOUND_SAMPLE_WriteControlRegister(uint8 control) 
{
    SOUND_SAMPLE_CONTROL = control;
}

#endif  /* (!SOUND_SAMPLE_ControlRegRemoved) */


#if (!(SOUND_SAMPLE_UsingFixedFunction && (CY_PSOC5A)))
/*******************************************************************************
* Function Name: SOUND_SAMPLE_WriteCounter
********************************************************************************
* Summary:
*   This funtion is used to set the counter to a specific value
*
* Parameters:  
*  counter:  New counter value. 
*
* Return: 
*  void 
*
*******************************************************************************/
void SOUND_SAMPLE_WriteCounter(uint16 counter) \
                                   
{
    #if(SOUND_SAMPLE_UsingFixedFunction)
        /* assert if block is already enabled */
        CYASSERT (0u == (SOUND_SAMPLE_GLOBAL_ENABLE & SOUND_SAMPLE_BLOCK_EN_MASK));
        /* If block is disabled, enable it and then write the counter */
        SOUND_SAMPLE_GLOBAL_ENABLE |= SOUND_SAMPLE_BLOCK_EN_MASK;
        CY_SET_REG16(SOUND_SAMPLE_COUNTER_LSB_PTR, (uint16)counter);
        SOUND_SAMPLE_GLOBAL_ENABLE &= ((uint8)(~SOUND_SAMPLE_BLOCK_EN_MASK));
    #else
        CY_SET_REG16(SOUND_SAMPLE_COUNTER_LSB_PTR, counter);
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
}
#endif /* (!(SOUND_SAMPLE_UsingFixedFunction && (CY_PSOC5A))) */


/*******************************************************************************
* Function Name: SOUND_SAMPLE_ReadCounter
********************************************************************************
* Summary:
* Returns the current value of the counter.  It doesn't matter
* if the counter is enabled or running.
*
* Parameters:  
*  void:  
*
* Return: 
*  (uint16) The present value of the counter.
*
*******************************************************************************/
uint16 SOUND_SAMPLE_ReadCounter(void) 
{
    /* Force capture by reading Accumulator */
    /* Must first do a software capture to be able to read the counter */
    /* It is up to the user code to make sure there isn't already captured data in the FIFO */
    #if(SOUND_SAMPLE_UsingFixedFunction)
		(void)CY_GET_REG16(SOUND_SAMPLE_COUNTER_LSB_PTR);
	#else
		(void)CY_GET_REG8(SOUND_SAMPLE_COUNTER_LSB_PTR_8BIT);
	#endif/* (SOUND_SAMPLE_UsingFixedFunction) */
    
    /* Read the data from the FIFO (or capture register for Fixed Function)*/
    #if(SOUND_SAMPLE_UsingFixedFunction)
        return ((uint16)CY_GET_REG16(SOUND_SAMPLE_STATICCOUNT_LSB_PTR));
    #else
        return (CY_GET_REG16(SOUND_SAMPLE_STATICCOUNT_LSB_PTR));
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_ReadCapture
********************************************************************************
* Summary:
*   This function returns the last value captured.
*
* Parameters:  
*  void
*
* Return: 
*  (uint16) Present Capture value.
*
*******************************************************************************/
uint16 SOUND_SAMPLE_ReadCapture(void) 
{
    #if(SOUND_SAMPLE_UsingFixedFunction)
        return ((uint16)CY_GET_REG16(SOUND_SAMPLE_STATICCOUNT_LSB_PTR));
    #else
        return (CY_GET_REG16(SOUND_SAMPLE_STATICCOUNT_LSB_PTR));
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_WritePeriod
********************************************************************************
* Summary:
* Changes the period of the counter.  The new period 
* will be loaded the next time terminal count is detected.
*
* Parameters:  
*  period: (uint16) A value of 0 will result in
*         the counter remaining at zero.  
*
* Return: 
*  void
*
*******************************************************************************/
void SOUND_SAMPLE_WritePeriod(uint16 period) 
{
    #if(SOUND_SAMPLE_UsingFixedFunction)
        CY_SET_REG16(SOUND_SAMPLE_PERIOD_LSB_PTR,(uint16)period);
    #else
        CY_SET_REG16(SOUND_SAMPLE_PERIOD_LSB_PTR, period);
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_ReadPeriod
********************************************************************************
* Summary:
* Reads the current period value without affecting counter operation.
*
* Parameters:  
*  void:  
*
* Return: 
*  (uint16) Present period value.
*
*******************************************************************************/
uint16 SOUND_SAMPLE_ReadPeriod(void) 
{
    #if(SOUND_SAMPLE_UsingFixedFunction)
        return ((uint16)CY_GET_REG16(SOUND_SAMPLE_PERIOD_LSB_PTR));
    #else
        return (CY_GET_REG16(SOUND_SAMPLE_PERIOD_LSB_PTR));
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
}


#if (!SOUND_SAMPLE_UsingFixedFunction)
/*******************************************************************************
* Function Name: SOUND_SAMPLE_WriteCompare
********************************************************************************
* Summary:
* Changes the compare value.  The compare output will 
* reflect the new value on the next UDB clock.  The compare output will be 
* driven high when the present counter value compares true based on the 
* configured compare mode setting. 
*
* Parameters:  
*  Compare:  New compare value. 
*
* Return: 
*  void
*
*******************************************************************************/
void SOUND_SAMPLE_WriteCompare(uint16 compare) \
                                   
{
    #if(SOUND_SAMPLE_UsingFixedFunction)
        CY_SET_REG16(SOUND_SAMPLE_COMPARE_LSB_PTR, (uint16)compare);
    #else
        CY_SET_REG16(SOUND_SAMPLE_COMPARE_LSB_PTR, compare);
    #endif /* (SOUND_SAMPLE_UsingFixedFunction) */
}


/*******************************************************************************
* Function Name: SOUND_SAMPLE_ReadCompare
********************************************************************************
* Summary:
* Returns the compare value.
*
* Parameters:  
*  void:
*
* Return: 
*  (uint16) Present compare value.
*
*******************************************************************************/
uint16 SOUND_SAMPLE_ReadCompare(void) 
{
    return (CY_GET_REG16(SOUND_SAMPLE_COMPARE_LSB_PTR));
}


#if (SOUND_SAMPLE_COMPARE_MODE_SOFTWARE)
/*******************************************************************************
* Function Name: SOUND_SAMPLE_SetCompareMode
********************************************************************************
* Summary:
*  Sets the software controlled Compare Mode.
*
* Parameters:
*  compareMode:  Compare Mode Enumerated Type.
*
* Return:
*  void
*
*******************************************************************************/
void SOUND_SAMPLE_SetCompareMode(uint8 compareMode) 
{
    /* Clear the compare mode bits in the control register */
    SOUND_SAMPLE_CONTROL &= ((uint8)(~SOUND_SAMPLE_CTRL_CMPMODE_MASK));
    
    /* Write the new setting */
    SOUND_SAMPLE_CONTROL |= compareMode;
}
#endif  /* (SOUND_SAMPLE_COMPARE_MODE_SOFTWARE) */


#if (SOUND_SAMPLE_CAPTURE_MODE_SOFTWARE)
/*******************************************************************************
* Function Name: SOUND_SAMPLE_SetCaptureMode
********************************************************************************
* Summary:
*  Sets the software controlled Capture Mode.
*
* Parameters:
*  captureMode:  Capture Mode Enumerated Type.
*
* Return:
*  void
*
*******************************************************************************/
void SOUND_SAMPLE_SetCaptureMode(uint8 captureMode) 
{
    /* Clear the capture mode bits in the control register */
    SOUND_SAMPLE_CONTROL &= ((uint8)(~SOUND_SAMPLE_CTRL_CAPMODE_MASK));
    
    /* Write the new setting */
    SOUND_SAMPLE_CONTROL |= ((uint8)((uint8)captureMode << SOUND_SAMPLE_CTRL_CAPMODE0_SHIFT));
}
#endif  /* (SOUND_SAMPLE_CAPTURE_MODE_SOFTWARE) */


/*******************************************************************************
* Function Name: SOUND_SAMPLE_ClearFIFO
********************************************************************************
* Summary:
*   This function clears all capture data from the capture FIFO
*
* Parameters:  
*  void:
*
* Return: 
*  None
*
*******************************************************************************/
void SOUND_SAMPLE_ClearFIFO(void) 
{

    while(0u != (SOUND_SAMPLE_ReadStatusRegister() & SOUND_SAMPLE_STATUS_FIFONEMP))
    {
        (void)SOUND_SAMPLE_ReadCapture();
    }

}
#endif  /* (!SOUND_SAMPLE_UsingFixedFunction) */


/* [] END OF FILE */

