/*******************************************************************************
* File Name: SOUND_PWM_CLOCK.h
* Version 2.20
*
*  Description:
*   Provides the function and constant definitions for the clock component.
*
*  Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_CLOCK_SOUND_PWM_CLOCK_H)
#define CY_CLOCK_SOUND_PWM_CLOCK_H

#include <cytypes.h>
#include <cyfitter.h>


/***************************************
* Conditional Compilation Parameters
***************************************/

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component cy_clock_v2_20 requires cy_boot v3.0 or later
#endif /* (CY_PSOC5LP) */


/***************************************
*        Function Prototypes
***************************************/

void SOUND_PWM_CLOCK_Start(void) ;
void SOUND_PWM_CLOCK_Stop(void) ;

#if(CY_PSOC3 || CY_PSOC5LP)
void SOUND_PWM_CLOCK_StopBlock(void) ;
#endif /* (CY_PSOC3 || CY_PSOC5LP) */

void SOUND_PWM_CLOCK_StandbyPower(uint8 state) ;
void SOUND_PWM_CLOCK_SetDividerRegister(uint16 clkDivider, uint8 restart) 
                                ;
uint16 SOUND_PWM_CLOCK_GetDividerRegister(void) ;
void SOUND_PWM_CLOCK_SetModeRegister(uint8 modeBitMask) ;
void SOUND_PWM_CLOCK_ClearModeRegister(uint8 modeBitMask) ;
uint8 SOUND_PWM_CLOCK_GetModeRegister(void) ;
void SOUND_PWM_CLOCK_SetSourceRegister(uint8 clkSource) ;
uint8 SOUND_PWM_CLOCK_GetSourceRegister(void) ;
#if defined(SOUND_PWM_CLOCK__CFG3)
void SOUND_PWM_CLOCK_SetPhaseRegister(uint8 clkPhase) ;
uint8 SOUND_PWM_CLOCK_GetPhaseRegister(void) ;
#endif /* defined(SOUND_PWM_CLOCK__CFG3) */

#define SOUND_PWM_CLOCK_Enable()                       SOUND_PWM_CLOCK_Start()
#define SOUND_PWM_CLOCK_Disable()                      SOUND_PWM_CLOCK_Stop()
#define SOUND_PWM_CLOCK_SetDivider(clkDivider)         SOUND_PWM_CLOCK_SetDividerRegister(clkDivider, 1u)
#define SOUND_PWM_CLOCK_SetDividerValue(clkDivider)    SOUND_PWM_CLOCK_SetDividerRegister((clkDivider) - 1u, 1u)
#define SOUND_PWM_CLOCK_SetMode(clkMode)               SOUND_PWM_CLOCK_SetModeRegister(clkMode)
#define SOUND_PWM_CLOCK_SetSource(clkSource)           SOUND_PWM_CLOCK_SetSourceRegister(clkSource)
#if defined(SOUND_PWM_CLOCK__CFG3)
#define SOUND_PWM_CLOCK_SetPhase(clkPhase)             SOUND_PWM_CLOCK_SetPhaseRegister(clkPhase)
#define SOUND_PWM_CLOCK_SetPhaseValue(clkPhase)        SOUND_PWM_CLOCK_SetPhaseRegister((clkPhase) + 1u)
#endif /* defined(SOUND_PWM_CLOCK__CFG3) */


/***************************************
*             Registers
***************************************/

/* Register to enable or disable the clock */
#define SOUND_PWM_CLOCK_CLKEN              (* (reg8 *) SOUND_PWM_CLOCK__PM_ACT_CFG)
#define SOUND_PWM_CLOCK_CLKEN_PTR          ((reg8 *) SOUND_PWM_CLOCK__PM_ACT_CFG)

/* Register to enable or disable the clock */
#define SOUND_PWM_CLOCK_CLKSTBY            (* (reg8 *) SOUND_PWM_CLOCK__PM_STBY_CFG)
#define SOUND_PWM_CLOCK_CLKSTBY_PTR        ((reg8 *) SOUND_PWM_CLOCK__PM_STBY_CFG)

/* Clock LSB divider configuration register. */
#define SOUND_PWM_CLOCK_DIV_LSB            (* (reg8 *) SOUND_PWM_CLOCK__CFG0)
#define SOUND_PWM_CLOCK_DIV_LSB_PTR        ((reg8 *) SOUND_PWM_CLOCK__CFG0)
#define SOUND_PWM_CLOCK_DIV_PTR            ((reg16 *) SOUND_PWM_CLOCK__CFG0)

/* Clock MSB divider configuration register. */
#define SOUND_PWM_CLOCK_DIV_MSB            (* (reg8 *) SOUND_PWM_CLOCK__CFG1)
#define SOUND_PWM_CLOCK_DIV_MSB_PTR        ((reg8 *) SOUND_PWM_CLOCK__CFG1)

/* Mode and source configuration register */
#define SOUND_PWM_CLOCK_MOD_SRC            (* (reg8 *) SOUND_PWM_CLOCK__CFG2)
#define SOUND_PWM_CLOCK_MOD_SRC_PTR        ((reg8 *) SOUND_PWM_CLOCK__CFG2)

#if defined(SOUND_PWM_CLOCK__CFG3)
/* Analog clock phase configuration register */
#define SOUND_PWM_CLOCK_PHASE              (* (reg8 *) SOUND_PWM_CLOCK__CFG3)
#define SOUND_PWM_CLOCK_PHASE_PTR          ((reg8 *) SOUND_PWM_CLOCK__CFG3)
#endif /* defined(SOUND_PWM_CLOCK__CFG3) */


/**************************************
*       Register Constants
**************************************/

/* Power manager register masks */
#define SOUND_PWM_CLOCK_CLKEN_MASK         SOUND_PWM_CLOCK__PM_ACT_MSK
#define SOUND_PWM_CLOCK_CLKSTBY_MASK       SOUND_PWM_CLOCK__PM_STBY_MSK

/* CFG2 field masks */
#define SOUND_PWM_CLOCK_SRC_SEL_MSK        SOUND_PWM_CLOCK__CFG2_SRC_SEL_MASK
#define SOUND_PWM_CLOCK_MODE_MASK          (~(SOUND_PWM_CLOCK_SRC_SEL_MSK))

#if defined(SOUND_PWM_CLOCK__CFG3)
/* CFG3 phase mask */
#define SOUND_PWM_CLOCK_PHASE_MASK         SOUND_PWM_CLOCK__CFG3_PHASE_DLY_MASK
#endif /* defined(SOUND_PWM_CLOCK__CFG3) */

#endif /* CY_CLOCK_SOUND_PWM_CLOCK_H */


/* [] END OF FILE */
