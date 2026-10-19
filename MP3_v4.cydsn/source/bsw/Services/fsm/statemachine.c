/**
* \file <filename>
* \author <author-name>
* \date <date>
*
* \brief <Symbolic File name>
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
*/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "statemachine.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/




/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/







/*****************************************************************************/
/* Local function prototypes ('static') / Guards                             */
/*****************************************************************************/



/*****************************************************************************/
/* Local function prototypes ('static') / Helper                             */
/*****************************************************************************/


/**
 * Get the index of the state in the table
 */
sint16_t STATE__getIndexOfState(STATE_stateOuterTransitionTable_t const * const  transitionTable,
                                uint16_t transitionTableSize,
                                STATE_state_t state);



/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
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
                STATE_state_t state)
{
    me->transitionTable = transitionTable;
    me->transitionTableSize = transitionTableSize;
    me->actionTable = actionTable;
    me->actionTableSize = actionTableSize;
    me->state = state;
    
    return RC_SUCCESS;
}



#define STATE_INVALIDINDEX -1
sint16_t STATE__getIndexOfState(STATE_stateOuterTransitionTable_t const * const  transitionTable,
                                uint16_t transitionTableSize,
                                STATE_state_t state)
{
    sint16_t result = STATE_INVALIDINDEX; //error
    
    for (uint16_t i = 0; i < transitionTableSize; i++)
    {
        if ((*transitionTable)[i].fromState == state)
        {
            return i;
        }
    }
    
    return STATE_INVALIDINDEX; 
}

/**
 * This function will process the event with the defined state machine
 * The state machine is implemented as a singleton object, only the configuration may differ
 * \param STATEMACHINE_t* const me              : IN - Pointer to the state machine
 * \param STATE_event_t ev                      : IN - Event to be processed
 * \return RC_SUCCESS or error
 */
RC_t STATE_processEvent(STATEMACHINE_t* const me, STATE_event_t ev)
{    
    STATE_GuardPtr_t guard = 0;
    STATE_ActionPtr_t acTransition = 0;
    STATE_ActionPtr_t acEntry = 0;
    STATE_ActionPtr_t acExit = 0;
    
    
    //Read the state from the watchdog object;
    STATE_state_t currentState = me->state;
    STATE_state_t toState = currentState; //Just in case
    
    //Calculate the table index of the outer table
    sint16_t index = STATE__getIndexOfState(me->transitionTable, me->transitionTableSize, currentState);
    
    //This should never happen
    if (STATE_INVALIDINDEX == index)
    {
        return RC_ERROR_RANGE;
    }
    
    STATE_stateInnerTransitionTable_t const* const pInnerTable = (*(me->transitionTable))[index].pInnerTransitionTable;
    
    uint16_t innerTableSize = 0;
    if (0 != pInnerTable)
	{
		innerTableSize = (*me->transitionTable)[index].size;
	}
    
    if (0 == pInnerTable || 0 == innerTableSize)
    {
        //No transitions in this state
        return RC_ERROR_INVALID_STATE;
    }
    
    //Iterate through the inner table and find correct transition
    for (uint16_t i = 0; i < innerTableSize; i++)
    {
        if ((*pInnerTable)[i].event == ev)
        {
            //Transition found, let's check the guard
            guard = (*pInnerTable)[i].guard;
            if (0 == guard || TRUE == guard())
            {
                //Transition is valid
                acTransition = (*pInnerTable)[i].actionTransition;
                toState = (*pInnerTable)[i].toState;
                
                //Now lets find the entry and exit actions
                uint16_t actionTableSize = me->actionTableSize;
                for (uint16_t j = 0; j < actionTableSize; j++)
                {
                    if (currentState == (*me->actionTable)[j].state)
                    {
                        acExit = (*me->actionTable)[j].actionExit;
                    }
                    if (toState == (*me->actionTable)[j].state)
                    {
                        acEntry = (*me->actionTable)[j].actionEntry;
                    }
                }

                //Lets call the actions
                if (0 != acExit) acExit();
                if (0 != acTransition) acTransition();
                if (0 != acEntry) acEntry();
                

                
                //If we want to fire only one transition, the break must be actiavted
                //Note: More than one transition only make sense, if the taregt states are identical
                //This must be ensured by the user
                //break;
            }
        }
    }
    
    //Copy local toState to external State Variable
    me->state = toState;
    
 
    return RC_SUCCESS;
}

/**
 * This function returns the current state, mainly for debugging and visualitsation purposes
 *
 * \param STATEMACHINE_t* const me              : IN - Pointer to the state machine
 * \return STATE_state_t
 */
STATE_state_t STATE_getState(STATEMACHINE_t const* const me)
{
    return me->state;
}
