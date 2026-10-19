/**
* \file sound_data.h
* \author P. Fromm
* \date 6.11.18
*
* \brief WAV Tables
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


 
#ifndef SOUND_DATA_H
#define SOUND_DATA_H

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#include "global.h"

//####################### Structures
/**
* \brief The purpose as well as the members of a structure have to be documented.
*
* Make clear what the structure is used for and what is the purpose of the members.
*/
struct sSoundRecourd {
    char * m_title;          /**< \brief title */
    uint16_t m_length;       /**< \brief duration in [s], song will be repeated for this time */
    uint32_t m_size;         /**< \brief size in [byte] */
    uint8_t * m_data;        /**< \brief pointer to the data */
  
  
};
typedef struct sSoundRecourd SOUND_Record_t;

// Wrapper to allow representing the file in Together as class
#ifdef TOGETHER

class SOUNDDATA
{
public:
#endif /* Together */

/*****************************************************************************/
/* Extern global variables                                                   */
/*****************************************************************************/

/**
 * Our flashdriver
 **/
extern SOUND_Record_t const SOUND_Records[];

/**
 * Number of songs we have
 **/
extern const uint16_t SOUND_NoOfRecords;

/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/




/*****************************************************************************/
/* Private stuff, only visible for Together, declared static in cpp - File   */
/*****************************************************************************/


#ifdef TOGETHER



};
#endif /* Together */

#endif /* FILE_H */
