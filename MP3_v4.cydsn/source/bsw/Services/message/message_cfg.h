/**
* \file message_cfg.h
* \author Peter Fromm
* \date 26.7.22
*
* \brief Simple messaging mechanism for ERIKA
*
*
* \note <notes>

* \todo <todos>
* \warning <warnings, e.g. dependencies, order of execution etc.>
*
*  Changelog:\n
*  - <version; data of change; author>
*            - <description of the change>
*
* \copyright Copyright ©2022
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


 
#ifndef MSG_CFG_H
#define MSG_CFG_H

#include "global.h"
#include "project.h"
 
/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#undef MSG_TIME_ANALYSIS                        //Add debugging verbosity
#define MSG_STATIC_ALLOCATION                   //select static (maxsize) or dynamic (heap) allocation for messages
#define MSG_STATIC_ALLOCATION_SIZE      50      //Max message size for static messages
#define MSG_IRQ                                 //Deactivate interrupts for critical region (as ressources do not work in IRQ in Erika)
    
#ifdef MSG_IRQ   
    #define MSG_DI CyGlobalIntDisable           //Deactivate interrupts 
    #define MSG_EI CyGlobalIntEnable            //Enable interrupts
#else
    #define MSG_DI asm("nop")                   //Ignore Deactivate interrupts 
    #define MSG_EI asm("nop")                   //Ignore Enable interrupts
#endif    
    
    
//####################### Structures
/**
* \brief The purpose as well as the members of a structure have to be documented.
*
* Make clear what the structure is used for and what is the purpose of the members.
*/

    
//Application specific value
typedef struct
    {
        uint8_t m_signal;        //!< Signal identifier
        uint8_t m_event;         //!< Type of event which happened to the signal
    } MSG_id_t;

//Set this define to add additional payload to the METADATA
#define MSG_OPTIONAL_METADATA MSG_id_t m_msgId
    
#ifdef MSG_TIME_ANALYSIS
    
    //Change the content of the following functions as needed by the application
    //MSG_startMeasurement will be called upon writing a message
    //MSG_stopMeasurement will be called upon reading a message
    static inline void MSG_startMeasurement()
    {
        //Pin_GPIO4_Write(1);
    }

    static inline void MSG_stopMeasurement()
    {
        //Pin_GPIO4_Write(0);
    }
    
#endif    
    
#endif /* MSG_H */
