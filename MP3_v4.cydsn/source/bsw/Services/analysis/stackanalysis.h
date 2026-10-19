/**
* \file stackanalysis.h
* \author P. Fromm
* \date 
*
* \brief stack analysis based on the the prefilled pattern approach
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


 
#ifndef STACKANALYSIS_H
#define STACKANALYSIS_H

#include "global.h"
#include "stackanalysis_cfg.h"
    
    
/**
 * Will fill the stack region with the pattern
 * Note: This function is implemented inline to ensure a low intrinsic implementation
 * \param STACK_region_t *const me      : [IN/OUT] the stack object
 */
RC_t inline STACK_init(STACK_region_t *const me)
{

    uint32_t localStack = 0;
    
    if (me->m_stackStart < me->m_stackEnd)
    {
        //rising stack
        for (uint32_t* pAdr =  &localStack; pAdr < me->m_stackEnd; pAdr++)
        {
            *pAdr = STACK_PATTERN;
        }
    }
    else
    {
        //falling stack (default)
        for (uint32_t* pAdr =  me->m_stackEnd; pAdr < &localStack; pAdr++)
        {
            *pAdr = STACK_PATTERN;
        }
    }

    return RC_SUCCESS;
}

/**
 * Will check how much of the pattern is corrupted
 * Todo: Check offset values especially in Erika tasks
 *       Furthermore, this algo does not provide 100% precise values, as we only count obviously overwritten 
 *       memory locations. I.e. a correct pattern between 2 overwritten adresses is not counted as used.
 * Note: This function is implemented inline to ensure a low intrinsic implementation
 * \param STACK_region_t *const me      : [IN/OUT] the stack object
 */
RC_t inline STACK_check(STACK_region_t *const me)
{

    uint32_t used = 0;
    
    if (me->m_stackStart < me->m_stackEnd)
    {
        //rising stack
        for (uint32_t* pAdr =  me->m_stackStart; pAdr < me->m_stackEnd; pAdr++)
        {
            if (*pAdr != STACK_PATTERN)
            {
                used +=4;
            }
        }
    }
    else
    {
        //falling stack (default)
        for (uint32_t* pAdr =  me->m_stackEnd; pAdr < me->m_stackStart; pAdr++)
        {
            if (*pAdr != STACK_PATTERN)
            {
                used +=4;
            }
        }
    }

    if (used > me->m_used)
    {
        me->m_used = used;
    }
    
    return RC_SUCCESS;
}


/**
 * Will print the stack object
 * \param STACK_region_t *const me      : [IN/OUT] the stack object
 */
RC_t STACK_print(STACK_region_t *const me);


#endif /* STACKANALYSIS_H */
