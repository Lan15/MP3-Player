/*******************************************************************************
* File Name: DBG_FILL.h  
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

#if !defined(CY_PINS_DBG_FILL_H) /* Pins DBG_FILL_H */
#define CY_PINS_DBG_FILL_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "DBG_FILL_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 DBG_FILL__PORT == 15 && ((DBG_FILL__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    DBG_FILL_Write(uint8 value);
void    DBG_FILL_SetDriveMode(uint8 mode);
uint8   DBG_FILL_ReadDataReg(void);
uint8   DBG_FILL_Read(void);
void    DBG_FILL_SetInterruptMode(uint16 position, uint16 mode);
uint8   DBG_FILL_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the DBG_FILL_SetDriveMode() function.
     *  @{
     */
        #define DBG_FILL_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define DBG_FILL_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define DBG_FILL_DM_RES_UP          PIN_DM_RES_UP
        #define DBG_FILL_DM_RES_DWN         PIN_DM_RES_DWN
        #define DBG_FILL_DM_OD_LO           PIN_DM_OD_LO
        #define DBG_FILL_DM_OD_HI           PIN_DM_OD_HI
        #define DBG_FILL_DM_STRONG          PIN_DM_STRONG
        #define DBG_FILL_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define DBG_FILL_MASK               DBG_FILL__MASK
#define DBG_FILL_SHIFT              DBG_FILL__SHIFT
#define DBG_FILL_WIDTH              1u

/* Interrupt constants */
#if defined(DBG_FILL__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in DBG_FILL_SetInterruptMode() function.
     *  @{
     */
        #define DBG_FILL_INTR_NONE      (uint16)(0x0000u)
        #define DBG_FILL_INTR_RISING    (uint16)(0x0001u)
        #define DBG_FILL_INTR_FALLING   (uint16)(0x0002u)
        #define DBG_FILL_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define DBG_FILL_INTR_MASK      (0x01u) 
#endif /* (DBG_FILL__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define DBG_FILL_PS                     (* (reg8 *) DBG_FILL__PS)
/* Data Register */
#define DBG_FILL_DR                     (* (reg8 *) DBG_FILL__DR)
/* Port Number */
#define DBG_FILL_PRT_NUM                (* (reg8 *) DBG_FILL__PRT) 
/* Connect to Analog Globals */                                                  
#define DBG_FILL_AG                     (* (reg8 *) DBG_FILL__AG)                       
/* Analog MUX bux enable */
#define DBG_FILL_AMUX                   (* (reg8 *) DBG_FILL__AMUX) 
/* Bidirectional Enable */                                                        
#define DBG_FILL_BIE                    (* (reg8 *) DBG_FILL__BIE)
/* Bit-mask for Aliased Register Access */
#define DBG_FILL_BIT_MASK               (* (reg8 *) DBG_FILL__BIT_MASK)
/* Bypass Enable */
#define DBG_FILL_BYP                    (* (reg8 *) DBG_FILL__BYP)
/* Port wide control signals */                                                   
#define DBG_FILL_CTL                    (* (reg8 *) DBG_FILL__CTL)
/* Drive Modes */
#define DBG_FILL_DM0                    (* (reg8 *) DBG_FILL__DM0) 
#define DBG_FILL_DM1                    (* (reg8 *) DBG_FILL__DM1)
#define DBG_FILL_DM2                    (* (reg8 *) DBG_FILL__DM2) 
/* Input Buffer Disable Override */
#define DBG_FILL_INP_DIS                (* (reg8 *) DBG_FILL__INP_DIS)
/* LCD Common or Segment Drive */
#define DBG_FILL_LCD_COM_SEG            (* (reg8 *) DBG_FILL__LCD_COM_SEG)
/* Enable Segment LCD */
#define DBG_FILL_LCD_EN                 (* (reg8 *) DBG_FILL__LCD_EN)
/* Slew Rate Control */
#define DBG_FILL_SLW                    (* (reg8 *) DBG_FILL__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define DBG_FILL_PRTDSI__CAPS_SEL       (* (reg8 *) DBG_FILL__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define DBG_FILL_PRTDSI__DBL_SYNC_IN    (* (reg8 *) DBG_FILL__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define DBG_FILL_PRTDSI__OE_SEL0        (* (reg8 *) DBG_FILL__PRTDSI__OE_SEL0) 
#define DBG_FILL_PRTDSI__OE_SEL1        (* (reg8 *) DBG_FILL__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define DBG_FILL_PRTDSI__OUT_SEL0       (* (reg8 *) DBG_FILL__PRTDSI__OUT_SEL0) 
#define DBG_FILL_PRTDSI__OUT_SEL1       (* (reg8 *) DBG_FILL__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define DBG_FILL_PRTDSI__SYNC_OUT       (* (reg8 *) DBG_FILL__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(DBG_FILL__SIO_CFG)
    #define DBG_FILL_SIO_HYST_EN        (* (reg8 *) DBG_FILL__SIO_HYST_EN)
    #define DBG_FILL_SIO_REG_HIFREQ     (* (reg8 *) DBG_FILL__SIO_REG_HIFREQ)
    #define DBG_FILL_SIO_CFG            (* (reg8 *) DBG_FILL__SIO_CFG)
    #define DBG_FILL_SIO_DIFF           (* (reg8 *) DBG_FILL__SIO_DIFF)
#endif /* (DBG_FILL__SIO_CFG) */

/* Interrupt Registers */
#if defined(DBG_FILL__INTSTAT)
    #define DBG_FILL_INTSTAT            (* (reg8 *) DBG_FILL__INTSTAT)
    #define DBG_FILL_SNAP               (* (reg8 *) DBG_FILL__SNAP)
    
	#define DBG_FILL_0_INTTYPE_REG 		(* (reg8 *) DBG_FILL__0__INTTYPE)
#endif /* (DBG_FILL__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_DBG_FILL_H */


/* [] END OF FILE */
