/*******************************************************************************
* File Name: DBG_FS.h  
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

#if !defined(CY_PINS_DBG_FS_H) /* Pins DBG_FS_H */
#define CY_PINS_DBG_FS_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "DBG_FS_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 DBG_FS__PORT == 15 && ((DBG_FS__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    DBG_FS_Write(uint8 value);
void    DBG_FS_SetDriveMode(uint8 mode);
uint8   DBG_FS_ReadDataReg(void);
uint8   DBG_FS_Read(void);
void    DBG_FS_SetInterruptMode(uint16 position, uint16 mode);
uint8   DBG_FS_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the DBG_FS_SetDriveMode() function.
     *  @{
     */
        #define DBG_FS_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define DBG_FS_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define DBG_FS_DM_RES_UP          PIN_DM_RES_UP
        #define DBG_FS_DM_RES_DWN         PIN_DM_RES_DWN
        #define DBG_FS_DM_OD_LO           PIN_DM_OD_LO
        #define DBG_FS_DM_OD_HI           PIN_DM_OD_HI
        #define DBG_FS_DM_STRONG          PIN_DM_STRONG
        #define DBG_FS_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define DBG_FS_MASK               DBG_FS__MASK
#define DBG_FS_SHIFT              DBG_FS__SHIFT
#define DBG_FS_WIDTH              1u

/* Interrupt constants */
#if defined(DBG_FS__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in DBG_FS_SetInterruptMode() function.
     *  @{
     */
        #define DBG_FS_INTR_NONE      (uint16)(0x0000u)
        #define DBG_FS_INTR_RISING    (uint16)(0x0001u)
        #define DBG_FS_INTR_FALLING   (uint16)(0x0002u)
        #define DBG_FS_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define DBG_FS_INTR_MASK      (0x01u) 
#endif /* (DBG_FS__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define DBG_FS_PS                     (* (reg8 *) DBG_FS__PS)
/* Data Register */
#define DBG_FS_DR                     (* (reg8 *) DBG_FS__DR)
/* Port Number */
#define DBG_FS_PRT_NUM                (* (reg8 *) DBG_FS__PRT) 
/* Connect to Analog Globals */                                                  
#define DBG_FS_AG                     (* (reg8 *) DBG_FS__AG)                       
/* Analog MUX bux enable */
#define DBG_FS_AMUX                   (* (reg8 *) DBG_FS__AMUX) 
/* Bidirectional Enable */                                                        
#define DBG_FS_BIE                    (* (reg8 *) DBG_FS__BIE)
/* Bit-mask for Aliased Register Access */
#define DBG_FS_BIT_MASK               (* (reg8 *) DBG_FS__BIT_MASK)
/* Bypass Enable */
#define DBG_FS_BYP                    (* (reg8 *) DBG_FS__BYP)
/* Port wide control signals */                                                   
#define DBG_FS_CTL                    (* (reg8 *) DBG_FS__CTL)
/* Drive Modes */
#define DBG_FS_DM0                    (* (reg8 *) DBG_FS__DM0) 
#define DBG_FS_DM1                    (* (reg8 *) DBG_FS__DM1)
#define DBG_FS_DM2                    (* (reg8 *) DBG_FS__DM2) 
/* Input Buffer Disable Override */
#define DBG_FS_INP_DIS                (* (reg8 *) DBG_FS__INP_DIS)
/* LCD Common or Segment Drive */
#define DBG_FS_LCD_COM_SEG            (* (reg8 *) DBG_FS__LCD_COM_SEG)
/* Enable Segment LCD */
#define DBG_FS_LCD_EN                 (* (reg8 *) DBG_FS__LCD_EN)
/* Slew Rate Control */
#define DBG_FS_SLW                    (* (reg8 *) DBG_FS__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define DBG_FS_PRTDSI__CAPS_SEL       (* (reg8 *) DBG_FS__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define DBG_FS_PRTDSI__DBL_SYNC_IN    (* (reg8 *) DBG_FS__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define DBG_FS_PRTDSI__OE_SEL0        (* (reg8 *) DBG_FS__PRTDSI__OE_SEL0) 
#define DBG_FS_PRTDSI__OE_SEL1        (* (reg8 *) DBG_FS__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define DBG_FS_PRTDSI__OUT_SEL0       (* (reg8 *) DBG_FS__PRTDSI__OUT_SEL0) 
#define DBG_FS_PRTDSI__OUT_SEL1       (* (reg8 *) DBG_FS__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define DBG_FS_PRTDSI__SYNC_OUT       (* (reg8 *) DBG_FS__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(DBG_FS__SIO_CFG)
    #define DBG_FS_SIO_HYST_EN        (* (reg8 *) DBG_FS__SIO_HYST_EN)
    #define DBG_FS_SIO_REG_HIFREQ     (* (reg8 *) DBG_FS__SIO_REG_HIFREQ)
    #define DBG_FS_SIO_CFG            (* (reg8 *) DBG_FS__SIO_CFG)
    #define DBG_FS_SIO_DIFF           (* (reg8 *) DBG_FS__SIO_DIFF)
#endif /* (DBG_FS__SIO_CFG) */

/* Interrupt Registers */
#if defined(DBG_FS__INTSTAT)
    #define DBG_FS_INTSTAT            (* (reg8 *) DBG_FS__INTSTAT)
    #define DBG_FS_SNAP               (* (reg8 *) DBG_FS__SNAP)
    
	#define DBG_FS_0_INTTYPE_REG 		(* (reg8 *) DBG_FS__0__INTTYPE)
#endif /* (DBG_FS__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_DBG_FS_H */


/* [] END OF FILE */
