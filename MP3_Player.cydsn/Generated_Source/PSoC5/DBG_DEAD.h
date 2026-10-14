/*******************************************************************************
* File Name: DBG_DEAD.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_DBG_DEAD_H) /* Pins DBG_DEAD_H */
#define CY_PINS_DBG_DEAD_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "DBG_DEAD_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 DBG_DEAD__PORT == 15 && ((DBG_DEAD__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    DBG_DEAD_Write(uint8 value);
void    DBG_DEAD_SetDriveMode(uint8 mode);
uint8   DBG_DEAD_ReadDataReg(void);
uint8   DBG_DEAD_Read(void);
void    DBG_DEAD_SetInterruptMode(uint16 position, uint16 mode);
uint8   DBG_DEAD_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the DBG_DEAD_SetDriveMode() function.
     *  @{
     */
        #define DBG_DEAD_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define DBG_DEAD_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define DBG_DEAD_DM_RES_UP          PIN_DM_RES_UP
        #define DBG_DEAD_DM_RES_DWN         PIN_DM_RES_DWN
        #define DBG_DEAD_DM_OD_LO           PIN_DM_OD_LO
        #define DBG_DEAD_DM_OD_HI           PIN_DM_OD_HI
        #define DBG_DEAD_DM_STRONG          PIN_DM_STRONG
        #define DBG_DEAD_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define DBG_DEAD_MASK               DBG_DEAD__MASK
#define DBG_DEAD_SHIFT              DBG_DEAD__SHIFT
#define DBG_DEAD_WIDTH              1u

/* Interrupt constants */
#if defined(DBG_DEAD__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in DBG_DEAD_SetInterruptMode() function.
     *  @{
     */
        #define DBG_DEAD_INTR_NONE      (uint16)(0x0000u)
        #define DBG_DEAD_INTR_RISING    (uint16)(0x0001u)
        #define DBG_DEAD_INTR_FALLING   (uint16)(0x0002u)
        #define DBG_DEAD_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define DBG_DEAD_INTR_MASK      (0x01u) 
#endif /* (DBG_DEAD__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define DBG_DEAD_PS                     (* (reg8 *) DBG_DEAD__PS)
/* Data Register */
#define DBG_DEAD_DR                     (* (reg8 *) DBG_DEAD__DR)
/* Port Number */
#define DBG_DEAD_PRT_NUM                (* (reg8 *) DBG_DEAD__PRT) 
/* Connect to Analog Globals */                                                  
#define DBG_DEAD_AG                     (* (reg8 *) DBG_DEAD__AG)                       
/* Analog MUX bux enable */
#define DBG_DEAD_AMUX                   (* (reg8 *) DBG_DEAD__AMUX) 
/* Bidirectional Enable */                                                        
#define DBG_DEAD_BIE                    (* (reg8 *) DBG_DEAD__BIE)
/* Bit-mask for Aliased Register Access */
#define DBG_DEAD_BIT_MASK               (* (reg8 *) DBG_DEAD__BIT_MASK)
/* Bypass Enable */
#define DBG_DEAD_BYP                    (* (reg8 *) DBG_DEAD__BYP)
/* Port wide control signals */                                                   
#define DBG_DEAD_CTL                    (* (reg8 *) DBG_DEAD__CTL)
/* Drive Modes */
#define DBG_DEAD_DM0                    (* (reg8 *) DBG_DEAD__DM0) 
#define DBG_DEAD_DM1                    (* (reg8 *) DBG_DEAD__DM1)
#define DBG_DEAD_DM2                    (* (reg8 *) DBG_DEAD__DM2) 
/* Input Buffer Disable Override */
#define DBG_DEAD_INP_DIS                (* (reg8 *) DBG_DEAD__INP_DIS)
/* LCD Common or Segment Drive */
#define DBG_DEAD_LCD_COM_SEG            (* (reg8 *) DBG_DEAD__LCD_COM_SEG)
/* Enable Segment LCD */
#define DBG_DEAD_LCD_EN                 (* (reg8 *) DBG_DEAD__LCD_EN)
/* Slew Rate Control */
#define DBG_DEAD_SLW                    (* (reg8 *) DBG_DEAD__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define DBG_DEAD_PRTDSI__CAPS_SEL       (* (reg8 *) DBG_DEAD__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define DBG_DEAD_PRTDSI__DBL_SYNC_IN    (* (reg8 *) DBG_DEAD__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define DBG_DEAD_PRTDSI__OE_SEL0        (* (reg8 *) DBG_DEAD__PRTDSI__OE_SEL0) 
#define DBG_DEAD_PRTDSI__OE_SEL1        (* (reg8 *) DBG_DEAD__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define DBG_DEAD_PRTDSI__OUT_SEL0       (* (reg8 *) DBG_DEAD__PRTDSI__OUT_SEL0) 
#define DBG_DEAD_PRTDSI__OUT_SEL1       (* (reg8 *) DBG_DEAD__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define DBG_DEAD_PRTDSI__SYNC_OUT       (* (reg8 *) DBG_DEAD__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(DBG_DEAD__SIO_CFG)
    #define DBG_DEAD_SIO_HYST_EN        (* (reg8 *) DBG_DEAD__SIO_HYST_EN)
    #define DBG_DEAD_SIO_REG_HIFREQ     (* (reg8 *) DBG_DEAD__SIO_REG_HIFREQ)
    #define DBG_DEAD_SIO_CFG            (* (reg8 *) DBG_DEAD__SIO_CFG)
    #define DBG_DEAD_SIO_DIFF           (* (reg8 *) DBG_DEAD__SIO_DIFF)
#endif /* (DBG_DEAD__SIO_CFG) */

/* Interrupt Registers */
#if defined(DBG_DEAD__INTSTAT)
    #define DBG_DEAD_INTSTAT            (* (reg8 *) DBG_DEAD__INTSTAT)
    #define DBG_DEAD_SNAP               (* (reg8 *) DBG_DEAD__SNAP)
    
	#define DBG_DEAD_0_INTTYPE_REG 		(* (reg8 *) DBG_DEAD__0__INTTYPE)
#endif /* (DBG_DEAD__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_DBG_DEAD_H */


/* [] END OF FILE */
