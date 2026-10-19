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


 
#ifndef STATEMACHINE_H
#define STATEMACHINE_H
    
#include "global.h"
#include "statemachine_type.h"
    
/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

//####################### Defines/Macros/Typedef's

/**
 * function pointer used for actions
 */
typedef void (*STATE_ActionPtr_t)();

/**
 * function pointer used for guards
 */
typedef boolean_t (*STATE_GuardPtr_t)();
    


//####################### Structures

/**
 * State Transition (Inner table)
 */
typedef struct
{
    STATE_event_t       event;                          /**< \brief The event firing the transition */
    STATE_state_t       toState;                        /**< \brief The target state */
	STATE_GuardPtr_t    guard;                          /**< \brief The guard function */
	STATE_ActionPtr_t   actionTransition;               /**< \brief The transition action */
	
} STATE_stateInnerTransition_t;

typedef STATE_stateInnerTransition_t  STATE_stateInnerTransitionTable_t[];

/**
 * State Transition (Outer table)
 */
typedef struct
{
    STATE_state_t                                    fromState;                 /**< \brief Leaving state */ 
    STATE_stateInnerTransitionTable_t const *  const pInnerTransitionTable;     /**< \brief Pointer to the transitions valid for this state */ 
    uint16_t                          const          size;                      /**< \brief Size of the inner table in elements */
} STATE_stateOuterTransition_t;

typedef STATE_stateOuterTransition_t  STATE_stateOuterTransitionTable_t[];



/**
 * State Action (Inner table)
 */
typedef struct
{
    STATE_state_t       state;              /**< \brief The state */
	STATE_ActionPtr_t   actionEntry;        /**< \brief Entry Action of the state */
	STATE_ActionPtr_t   actionExit;         /**< \brief Exit Action of the state */
} STATE_stateAction_t;

typedef STATE_stateAction_t  STATE_stateActionTable_t[];


typedef struct
{   STATE_stateOuterTransitionTable_t const * transitionTable;
    uint16_t transitionTableSize;
    STATE_stateActionTable_t const * actionTable;
    uint16_t actionTableSize;
    STATE_state_t state;
} STATEMACHINE_t;


/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/

/**
 * Initialise the state machine, especially the internal object
 * \param STATEMACHINE_t* const me                                          : OUT - Pointer to the state machine
 * \param STATE_stateOuterTransitionTable_t const *const transitionTable    : IN - Pointer to transition table
 * \param uint16_t transitionTableSize                                      : IN - Size of the transition table
 * \param STATE_stateActionTable_t const * const actionTable                : IN - Pointer to action table
 * \param uint16_t actionTableSize                                          : IN - Size of action table
 * \param STATE_state_t state                                               : IN - Initial State
 * \return RC_SUCCESS;
 */
RC_t STATE_init(STATEMACHINE_t* const me,
                STATE_stateOuterTransitionTable_t const * const transitionTable,
                uint16_t transitionTableSize,
                STATE_stateActionTable_t const * const actionTable,
                uint16_t actionTableSize,
                STATE_state_t state);


/**
 * This function will process the event with the defined state machine
 * The state machine is implemented as a singleton object, only the configuration may differ
 * \param STATEMACHINE_t const* const me        : IN - Pointer to the state machine
 * \param STATE_event_t ev                      : IN - Event to be processed
 * \return RC_SUCCESS or error
 */
RC_t STATE_processEvent(STATEMACHINE_t* const me, STATE_event_t ev);


/**
 * This function returns the current state, mainly for debugging and visualitsation purposes
 *
 * \param STATEMACHINE_t* const me              : IN - Pointer to the state machine
 * \return STATE_state_t
 */
STATE_state_t STATE_getState(STATEMACHINE_t const* const me);


/*****************************************************************************/
/* Private stuff, only visible for Together, declared static in cpp - File   */
/*****************************************************************************/

#endif /* STATEMACHINE_H */
