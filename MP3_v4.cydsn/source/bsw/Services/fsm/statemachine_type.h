/**
* \file statemachine.h
* \author P. Fromm
* \date 2.4.20
*
* \brief Generic, OS independant state machine implementation
*
* detailed description what the file does
*
* \note <notes>
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


 
#ifndef STATEMACHINE_TYPE_H
#define STATEMACHINE_TYPE_H
    
#include "global.h"
    
    
//####################### Mapping of Application Event and State Enumerations

    enum eEvent{
    ev_none,               /**< /brief no event */
    ev_safe_allOk,         /**< /brief All ok event, indicating, no monitor reported an error */
    ev_safe_warn,          /**< /brief At least one monitor issued a warning, speed will slow down to 50% */
    ev_safe_error,         /**< /brief At least one monitor issued an error, speed will slow down to 0% */        
};
typedef enum eEvent STATE_event_t;

enum eState{
    state_safe_ok,          /**< /brief No errors occured, full speed is possible */
    state_safe_warn,        /**< /brief Warning state, engine woll slow donw */
    state_safe_error,       /**< /brief Error state, engine will stop */
    state_safe_fatal_error, /**< /brief Fatal error state, engine will stop, complete system will enter error state, no return until reboot */
};
typedef enum eState STATE_state_t;  


#endif /* STATEMACHINE_TYPE_H */
