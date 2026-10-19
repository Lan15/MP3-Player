/**
* \file sprintf.h
* \author Duyen
* \date 14.02.2925
*
* \brief snprintf like API for converting numbers into strings without heap and global data usage.
*
*
* \note <notes>
* Programming rules (may be deleted in the final release of the file)
* ===================================================================
*
* 1. Naming conventions:
*    - Prefix of your module in front of every function and static data. 
*    - Scope _ for public and __ for private functions / data / types, e.g. 
*       Public:  void CONTROL_straightPark_Init();
*       Private: static void CONTROL__calcDistance();
*       Public:  typedef enum {RED, GREEN, YELLOW} CONTROL_color_t
*    - Own type definitions e.g. for structs or enums get a postfix _t
*    - #define's and enums are written in CAPITAL letters
* 2. Code structure
*    - Be aware of the scope of your modules and functions. Provide only functions which belong to your module to your files
*    - Prepare your design before starting to code
*    - Implement the simple most solution (Too many if then else nestings are an indicator that you have not properly analysed your task)
*    - Avoid magic numbers, use enums and #define's instead
*    - Make sure, that all error conditions are properly handled
*    - If your module provides data structures, which are required in many other files, it is recommended to place them in a file_type.h file
*	  - If your module contains configurable parts, is is recommended to place these in a file_config.h|.c file
* 3. Data conventions
*    - Minimize the scope of data (and functions)
*    - Global data is not allowed outside of the signal layer (in case a signal layer is part of your design)
*    - All static objects have to be placed in a valid linker sections
*    - Data which is accessed in more than one task has to be volatile and needs to be protected (e.g. by using messages or semaphores)
*    - Do not mix signed and unsigned data in the same operation
* 4. Documentation
*    - Use self explaining function and variable names
*    - Use proper indentation
*    - Provide Javadoc / Doxygen compatible comments in your header file and C-File
*    		- Every  File has to be documented in the header
*			- Every function parameter and return value must be documented, the valid range needs to be specified
*     		- Logical code blocks in the C-File must be commented
*    - For a detailed list of doxygen commands check http://www.stack.nl/~dimitri/doxygen/index.html 
* 5. Qualification
*    - Perform and document design and code reviews for every module
*    - Provide test specifications for every module (focus on error conditions)
*
* Further information:
*    - Check the programming rules defined in the MIMIR project guide
*         - Code structure: https://fromm.eit.h-da.de/intern/mimir/methods/eng_codestructure/method.htm
*         - MISRA for C: https://fromm.eit.h-da.de/intern/mimir/methods/eng_c_rules/method.htm
*         - MISRA for C++: https://fromm.eit.h-da.de/intern/mimir/methods/eng_cpp_rules/method.htm 
*
* \todo <todos>
* \warning <warnings, e.g. dependencies, order of execution etc.>
*
*  Changelog:\n
*  - <version; data of change; author>
*            - <description of the change>
*
* \copyright Copyright Â©2025
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

#ifndef SPRINTF_H_
#define SPRINTF_H_

#ifdef __cplusplus
 extern "C" {
#endif

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

#include <stdarg.h>
#include "global.h"
#include "sprintf_cfg.h"


/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/


/*****************************************************************************/
/* Type declarations                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* User functions                                                            */
/*****************************************************************************/

/**
 * \brief Reverse a string
 *
 * \param uint8_t *str		: [IN/OUT] string to revert
 * \param uint16_t len 		: [IN] length of the string
 * \return RC_t				: [OUT] RC_ERROR_BAD_PARAM if length is 0
 *                                  RC_SUCCESS if success
 */
RC_t IO_reverse(uint8_t *str, uint16_t len);

/**
 * \brief Applies minimum width and zero padding to a given buffer that contains a numeric string
 *
 * \param uint8_t *buffer	: [IN/OUT] Buffer to apply minimum width and zero padding
 * \param uint16_t buffSize	: [IN] Size of the input buffer
 * \param uint8_t minWidth	: [IN] Minimum width to apply to the buffer
 * \param uint8_t zeroPad	: [IN] Flag to indicate if zero padding is to be applied or not
 * \return RC_t				: [OUT] RC_ERROR_BAD_PARAM if provided buffer length is 0
 *                                  RC_SUCCESS if success
 */
RC_t IO_padding(uint8_t *buffer, uint16_t buffSize, uint8_t minWidth, uint8_t zeroPad);

/**
 * \brief Convert an integer into a string
 *
 * \param sint32_t value	: [IN] Integer value to be converted
 * \param uint8_t *buffer	: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize	: [IN] Size of the input buffer
 * \return RC_t				: [OUT] RC_ERROR_OVERRUN if the converted string is larger than the maximum size
 *                                  RC_ERROR_BAD_PARAM if provided buffer length is 0
 *                                  RC_SUCCESS if success
 */
RC_t IO_intToString(sint32_t value, uint8_t *buffer, uint16_t buffSize);

/**
 * \brief Convert an integer into a string
 *
 * \param uint32_t value	: [IN]  Convert an unsigned integer into a string
 * \param uint8_t *buffer	: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize	: [IN] Size of the input buffer
 * \return RC_t				: [OUT] RC_ERROR_OVERRUN if the converted string is larger than the maximum size
 *                                  RC_SUCCESS if success
 */
RC_t IO_uintToString(uint32_t value, uint8_t *buffer, uint16_t buffSize);

/**
 * \brief Convert an integer into a string
 *
 * \param uint32_t value	: [IN] Convert an integer into a hex string
 * \param uint8_t *buffer	: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize	: [IN] Size of the input buffer
 * \param uint8_t uppercase	: [IN] Flag to notify that this function should generate an uppercase string
 * \return RC_t				: [OUT] RC_ERROR_OVERRUN if the converted string is larger than the maximum size
 *                                  RC_ERROR_BAD_PARAM if provided buffer length is 0
 *                                  RC_SUCCESS if success
 */
RC_t IO_intToHex(uint32_t value, uint8_t *buffer, uint16_t buffSize, uint8_t uppercase);

/**
 * \brief Convert a float into a string
 *
 * \param double value			: [IN] Convert a float into a hex string
 * \param uint8_t *buffer		: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize		: [IN] Size of the input buffer
 * \param uint16_t precision	: [IN] Number of decimal points required
 * \return RC_t					: [OUT] RC_ERROR_OVERRUN if the converted string is larger than the maximum size
 *                                      RC_ERROR_BAD_PARAM if provided buffer length is 0
 *                                      RC_SUCCESS if success
 */
RC_t IO_floatToString(double value, uint8_t *buffer, uint16_t buffSize, uint16_t precision);

/**
 * \brief snprintf like function. This function takes the initialized variable
 * list as parameter. It is called inside the LOG function to create the string
 *
 * \param uint8_t *buffer				: [IN/OUT] Buffer contains the converted string
 * \param const char *format value		: [IN] String with the format like sprintf
 * \param uint16_t buffSize				: [IN] Size of the input buffer
 * \param va_list args					: [IN] List of the initialized arguments
 * \return RC_t							: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not large enough
 *                                              RC_ERROR_BAD_PARAM if provided buffer length is 0
 *                                              RC_SUCCESS if success
 */
RC_t IO_snprintf_va(uint8_t *buffer, uint16_t buffSize, const char *format, va_list *args);

/**
 * \brief snprintf like function. This is a stand alone function, can be used
 * to construct a string from the given format
 *
 * \param uint8_t *buffer				: [IN/OUT] Buffer contains the converted string
 * \param const char *format value		: [IN] String with the format like sprintf
 * \param uint16_t buffSize				: [IN] Size of the input buffer
 * \param ...							: [IN] List of the arguments
 * \return RC_t							: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not large enough
 *                                              RC_ERROR_BAD_PARAM if provided buffer length is 0
 *                                              RC_SUCCESS if success
 */
RC_t IO_snprintf(uint8_t *buffer, uint16_t buffSize, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* LOG_SPRINTF_H_ */
