/**
* \file stackanalysis_cfg.h
* \author P. Fromm
* \date 09.12.2024
*
* \brief stack analysis configuration file
*
* \todo <todos>
* \warning <warnings, e.g. dependencies, order of execution etc.>
*
*  Changelog:\n
*  - <version; data of change; author>
*            - <description of the change>
*
* \copyright Copyright ©2016
* Department of electrical engineering and information technology, Hochschule Darmstadt - University of applied sciences (h_da). All Rights Reserved.
* Permission to use, copy, modify, and distribute this software and its documentation for educational, and research purposes in the context of non-commercial
* (unless permitted by h_da) and official h_da projects, is hereby granted for enrolled students of h_da, provided that the above copyright notice,
* this paragraph and the following paragraph appear in all copies, modifications, and distributions.
* Contact Prof.Dr.-Ing. Peter Fromm, peter.fromm@h-da.de, Birkenweg 8 64295 Darmstadt - GERMANY for commercial requests.
*
* \warning This software is a PROTOTYPE version and is not designed or intended for use in production, especially not for safety-critical applications!
* The user represents and warrants that it will NOT use or redistribute the Software for such purposes.
* This prototype is for research purposes only. This software is provided "AS IS," without a warranty of any kind.
**/


 
#ifndef STACK_ANALYSIS_CFG_H
#define STACK_ANALYSIS_CFG_H

#include "global.h"
#include "project.h"
    
/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#define STACK_PATTERN 0xcc00ffee    
    
typedef struct {
    
    char *const     m_name;             //!< Name of the stack section
    uint32_t*       m_stackStart;       //!< Upper (start) adress of the stack
    uint32_t*       m_stackEnd;         //!< Lower (end) adress of the stack
    uint32_t        m_used;             //!< The number of bytes used
} STACK_region_t;


/*****************************************************************************/
/* Stack Adresses                                                            */
/*****************************************************************************/

    
/**
 * Symbols from the linker file containing the stack boundaries
 * Note: The stack goes from high adresses to low adresses
 */
extern uint32_t const __cy_stack[];            //High adress, stack start
extern uint32_t const __cy_stack_limit[];      //Low adress, stack end


//extern uint32_t const EE_ApplicationMode[];   //Object linked after the stack, represents the high adress  
extern uint32_t const EE_cortex_mx_stack_5[];  //Stack Object for the event task, Low adress, stack end

/*****************************************************************************/
/* Stack Objects                                                             */
/*****************************************************************************/

extern STACK_region_t STACK_system;
extern STACK_region_t STACK_EE_tsk_event;
    
#endif /* STACK_ANALYSIS_CFG_H */
