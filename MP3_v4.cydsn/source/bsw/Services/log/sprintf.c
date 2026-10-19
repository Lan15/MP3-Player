/**
* \file sprintf.c
* \author Duyen
* \date 14.02.2925
*
* \brief snprintf like API implementation for converting numbers into strings without heap and global data usage.
*
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
* \copyright Copyright ©2025
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
/* Includes                                                                  */
/*****************************************************************************/

#include "sprintf.h"

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/




/*****************************************************************************/
/* Global variable definitions (declared in header file with 'extern')       */
/*****************************************************************************/

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/


/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/


/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/


/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/**
 * \brief Reverse a string
 *
 * \param uint8_t *str		: [IN/OUT] string to revert
 * \param uint16_t len 		: [IN] length of the string
 * \return RC_t				: [OUT] RC_ERROR_BAD_PARAM if length is 0, RC_SUCCESS if success
 */
RC_t IO_reverse(uint8_t *str, uint16_t len) {
    
    if (0 == len)
    {
        return RC_ERROR_BAD_PARAM;
    }

    uint16_t i = 0;
    uint16_t j = len - 1;

    while (i < j) {
        uint8_t temp = str[i];
        str[i++] = str[j];
        str[j--] = temp;
    }

    return RC_SUCCESS;
}

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
RC_t IO_padding(uint8_t *buffer, uint16_t buffSize, uint8_t minWidth, uint8_t zeroPad)
{
    if (0 == minWidth) {
        // Padding not required. Return
        return RC_SUCCESS;
    }
    
    if ((0 == buffSize) || (minWidth > buffSize))
    {
        return RC_ERROR_BAD_PARAM;
    }
    
    // Get length of string in buffer
    uint8_t length = 0;
    while (buffer[length] != '\0') length++;
    
    // Check if padding is required
    if (length < minWidth)
    {
        uint8_t paddingSize = minWidth - length;
        
        // Move content to the right
        for (uint8_t i = 0; i <= length; i++)
        {
            buffer[length + paddingSize - i] = buffer[length - i];
        }
        
        // Fill with spaces or zeros
        char_t paddingChar = zeroPad ? '0' : ' ';
        for (uint8_t i = 0; i < paddingSize; i++)
        {
            buffer[i] = paddingChar;
        }
    }
    else
    {
        // Width/zero-padding not required
    }
    
    return RC_SUCCESS;
}

/**
 * \brief Convert an integer into a string
 *
 * \param sint32_t value	: [IN] Integer value to be converted
 * \param uint8_t *buffer	: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize	: [IN] Size of the input buffer
 * \return RC_t				: [OUT] RC_ERROR_OVERRUN if the converted string is
 * larger than the maximum size, RC_ERROR_BAD_PARAM if provided buffer length is 0, RC_SUCCESS if success
 */
RC_t IO_intToString(sint32_t value, uint8_t *buffer, uint16_t buffSize) {

    if (0 == buffSize)
    {
        return RC_ERROR_BAD_PARAM;
    }

    uint16_t i = 0;
    uint8_t isNegative = 0;

    if (value < 0) {
        isNegative = 1;
        value = -value;
    }

    // count for the '-' symbol
    uint16_t strLen = isNegative ? 1 : 0;

    // use the last slot to store the null terminated character
    uint16_t maxSpace = buffSize - 1;
    RC_t ret = RC_SUCCESS;

    do {

        if (strLen >= maxSpace)
        {
		   ret = RC_ERROR_OVERRUN;
		   break;
        }

        buffer[i++] = (value % 10) + '0';
        value /= 10;
        strLen++;

    } while (value);

    if (isNegative) {
        buffer[i++] = '-';
    }
    IO_reverse(buffer, i);
    buffer[i] = '\0';

    return ret;
}

/**
 * \brief Convert an integer into a string
 *
 * \param uint32_t value	: [IN]  Convert an unsigned integer into a string
 * \param uint8_t *buffer	: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize	: [IN] Size of the input buffer
 * \return RC_t				: [OUT] RC_ERROR_OVERRUN if the converted string is
 * larger than the maximum size, RC_SUCCESS if success
 */
RC_t IO_uintToString(uint32_t value, uint8_t *buffer, uint16_t buffSize) {

    if (0 == buffSize)
    {
        return RC_ERROR_BAD_PARAM;
    }

    uint16_t i = 0;

    // use the last slot to store the null terminated character
    uint16_t maxSpace = buffSize - 1;
    RC_t ret = RC_SUCCESS;

    do {

        if (i >= maxSpace)
        {
		   ret = RC_ERROR_OVERRUN;
		   break;
        }

        buffer[i++] = (value % 10) + '0';
        value /= 10;
    } while (value);

    IO_reverse(buffer, i);
    buffer[i] = '\0';

    return ret;
}

/**
 * \brief Convert an unsigned integer into a string
 *
 * \param uint32_t value	    : [IN] Convert an integer into a hex string
 * \param uint8_t *buffer		: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize		: [IN] Size of the input buffer
 * \param uint8_t uppercase		: [IN] Flag to notify that this function should generate an uppercase string
 * \return RC_t					: [OUT] RC_ERROR_OVERRUN if the converted string is
 * larger than the maximum size, RC_ERROR_BAD_PARAM if provided buffer length is 0, RC_SUCCESS if success
 */
RC_t IO_intToHex(uint32_t value, uint8_t *buffer, uint16_t buffSize, uint8_t uppercase) {

    if (0 == buffSize)
    {
        return RC_ERROR_BAD_PARAM;
    }

    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    uint16_t i = 0;

    // use the last slot to store the null terminated character
    uint16_t maxSpace = buffSize - 1;
    RC_t ret = RC_SUCCESS;

    do {

        if (i >= maxSpace)
        {
		   ret = RC_ERROR_OVERRUN;
		   break;
        }

    	buffer[i++] = digits[value % 16];
        value /= 16;

    } while (value);

   IO_reverse(buffer, i);
   buffer[i] = '\0';

   return ret;
}

/**
 * \brief Convert a float into a string
 *
 * \param double value			: [IN] Convert a float into a hex string
 * \param uint8_t *buffer		: [IN/OUT] Buffer to store the converted integer into string
 * \param uint16_t buffSize		: [IN] Size of the input buffer
 * \param uint16_t precision	: [IN] Number of decimal points required
 * \return RC_t					: [OUT] RC_ERROR_OVERRUN if the converted string is
 * larger than the maximum size, RC_ERROR_BAD_PARAM if provided buffer length is 0, RC_SUCCESS if success
 */
RC_t IO_floatToString(double value, uint8_t *buffer, uint16_t buffSize, uint16_t precision) {

    if (0 == buffSize)
    {
        return RC_ERROR_BAD_PARAM;
    }

    uint16_t i = 0;
	uint8_t isNegative = 0;

	// convert the integer part
	if (value < 0) {
		isNegative = 1;
		value = -value;
	}
	uint32_t intPart = (uint32_t)value;

    // count for the '-' symbol and '.' symbol
	uint16_t strLen = isNegative ? 2 : 1;

    // use the last slot to store the null terminated character
    uint16_t maxSpace = buffSize - 1;
	RC_t ret = RC_SUCCESS;

	do {

        // count for the null terminated char
        if (strLen >= maxSpace)
        {
        	ret = RC_ERROR_OVERRUN;
        	break;
        }

		buffer[i++] = (intPart % 10) + '0';
		intPart /= 10;
		strLen++;

	} while (intPart);

	if (isNegative) {
	   buffer[i++] = '-';
	}
	IO_reverse(buffer, i);

    buffer[i++] = '.';

    if (ret == RC_SUCCESS)
    {
    	// convert the fractional part
		double fracPart = value - (uint32_t)value;
		for (uint16_t p = 0; p < precision; p++) {

	        // count for the null terminated char
			if (strLen >= maxSpace)
			{
				ret = RC_ERROR_OVERRUN;
				break;
			}

			fracPart *= 10;
			buffer[i++] = ((uint32_t)fracPart % 10) + '0';
			strLen++;
		}
    }

    buffer[i] = '\0';
    return ret;
}

/**
 * \brief snprintf like function. This function takes the initialized variable
 * list as parameter. It is called inside the LOG function to create the string
 *
 * \param uint8_t *buffer				: [IN/OUT] Buffer contains the converted string
 * \param const char *format value		: [IN] String with the format like sprintf
 * \param uint16_t buffSize				: [IN] Size of the input buffer
 * \param va_list args					: [IN] List of the initialized arguments
 * \return RC_t							: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not
 * large enough, RC_ERROR_BAD_PARAM if provided buffer length is 0, RC_SUCCESS if success
 */
RC_t IO_snprintf_va(uint8_t *buffer, uint16_t buffSize, const char *format, va_list *args) {

    if (0 == buffSize)
    {
        return RC_ERROR_BAD_PARAM;
    }

	va_list ap;
	va_copy(ap, *args);

    const uint8_t *p = (uint8_t*)format;
    uint8_t *buffPtr = buffer;

    /**
     * For the conversion from int to str, doulbe to str, etc, we need to have a temporary buffer
     * to store the intermediate converted string before assigning it to the resulted buffer.
     * The size of the array is kept to 40 + precision. The maximum number of characters that
     * could be obtained from the conversion is coming from the double conversion.
     *
     * For double value, it contains the integer part and floating part. We keep the max buffer size
     * to 40 + precision, so it should be fine if the user use the SPRINTF_MAX_NUM_BUFF_SIZE_CONV
     * as the size of the array for all kind of conversions with the helper functions.
     */
    uint8_t numBuffer[SPRINTF_NUMBER_BUFF_SIZE];

    // use the last slot to store the null terminated character
    uint16_t maxSpace = buffSize-1;
    uint16_t index = 0;
    RC_t ret = RC_SUCCESS;

    while (*p) {
        if (*p == '%') {
            p++;
            
            // Check for zero-padding
            uint8_t zeroPad = 0;
            if('0' == *p) {
                zeroPad = 1;
                p++;
            }
            
            // Get minimum width
            uint8_t minWidth = 0;
            while (*p >= '0' && *p <= '9') {
                minWidth = minWidth * 10 + (*p - '0');
                p++;
            }
            
            switch (*p) {
                case 'd': { // Signed integer
                    sint32_t val = va_arg(ap, sint32_t);
                    ret = IO_intToString(val, numBuffer, SPRINTF_NUMBER_BUFF_SIZE);
                    if (RC_SUCCESS != ret) return ret;
                    
                    ret = IO_padding(numBuffer, SPRINTF_NUMBER_BUFF_SIZE, minWidth, zeroPad);
                    if (RC_SUCCESS != ret) return ret;
                    
                    for (uint8_t *s = numBuffer; *s; s++)
					{
                    	if (index >= maxSpace)
                    	{
                    		ret = RC_ERROR_OVERRUN;
                    		break;
                    	}
                    	buffPtr[index++] = *s;
					}
                    
                    break;
                }
                case 'l': { // Long integer
                    p++;
                    if (*p == 'd') {        // Long signed integer
                        sint32_t val = va_arg(ap, sint32_t);
                        ret = IO_intToString(val, numBuffer, SPRINTF_NUMBER_BUFF_SIZE);
                        if (RC_SUCCESS != ret) return ret;
                    
                        ret = IO_padding(numBuffer, SPRINTF_NUMBER_BUFF_SIZE, minWidth, zeroPad);
                        if (RC_SUCCESS != ret) return ret;

                        for (uint8_t *s = numBuffer; *s; s++)
    					{
                        	if (index >= maxSpace)
                        	{
                        		ret = RC_ERROR_OVERRUN;
                        		break;
                        	}
                        	buffPtr[index++] = *s;
    					}
                    } else if (*p == 'u') { // Long unsigned integer
                        uint32_t val = va_arg(ap, uint32_t);
                        ret = IO_uintToString(val, numBuffer, SPRINTF_NUMBER_BUFF_SIZE);
                        if (RC_SUCCESS != ret) return ret;
                    
                        ret = IO_padding(numBuffer, SPRINTF_NUMBER_BUFF_SIZE, minWidth, zeroPad);
                        if (RC_SUCCESS != ret) return ret;

                        for (uint8_t *s = numBuffer; *s; s++)
    					{
                        	if (index >= maxSpace)
                        	{
                        		ret = RC_ERROR_OVERRUN;
                        		break;
                        	}
                        	buffPtr[index++] = *s;
    					}
                    }
                    break;
                }
                case 'u': { // Unsigned integer
                    uint32_t val = va_arg(ap, uint32_t);
                    ret = IO_uintToString(val, numBuffer, SPRINTF_NUMBER_BUFF_SIZE);
                    if (RC_SUCCESS != ret) return ret;
                    
                    ret = IO_padding(numBuffer, SPRINTF_NUMBER_BUFF_SIZE, minWidth, zeroPad);
                    if (RC_SUCCESS != ret) return ret;

                    for (uint8_t *s = numBuffer; *s; s++)
					{
                    	if (index >= maxSpace)
                    	{
                    		ret = RC_ERROR_OVERRUN;
                    		break;
                    	}
                    	buffPtr[index++] = *s;
					}

                    break;
                }
                case 'x': { // Hex lowercase
                    unsigned int val = va_arg(ap, unsigned int);
                    ret = IO_intToHex(val, numBuffer, SPRINTF_NUMBER_BUFF_SIZE, 0);
                    if (RC_SUCCESS != ret) return ret;
                    
                    ret = IO_padding(numBuffer, SPRINTF_NUMBER_BUFF_SIZE, minWidth, zeroPad);
                    if (RC_SUCCESS != ret) return ret;

                    for (uint8_t *s = numBuffer; *s; s++)
					{
                    	if (index >= maxSpace)
                    	{
                    		ret = RC_ERROR_OVERRUN;
                    		break;
                    	}
                    	buffPtr[index++] = *s;
					}
                    break;
                }
                case 'X': { // Hex uppercase
                    uint32_t val = va_arg(ap, uint32_t);
                    ret = IO_intToHex(val, numBuffer, SPRINTF_NUMBER_BUFF_SIZE, 1);
                    if (RC_SUCCESS != ret) return ret;
                    
                    ret = IO_padding(numBuffer, SPRINTF_NUMBER_BUFF_SIZE, minWidth, zeroPad);
                    if (RC_SUCCESS != ret) return ret;

                    for (uint8_t *s = numBuffer; *s; s++)
					{
                    	if (index >= maxSpace)
                    	{
                    		ret = RC_ERROR_OVERRUN;
                    		break;
                    	}
                    	buffPtr[index++] = *s;
					}
                    break;
                }
                case 'f': { // Float
                	// float is promoted to double when passing through ...
                    double val = va_arg(ap, double);
                    ret = IO_floatToString(val, numBuffer, SPRINTF_NUMBER_BUFF_SIZE, SPRINTF_FLOAT_PRECISION);

                    if (RC_SUCCESS != ret) {
                    	return ret;
                    }

                    for (uint8_t *s = numBuffer; *s; s++)
					{
                    	if (index >= maxSpace)
                    	{
                    		ret = RC_ERROR_OVERRUN;
                    		break;
                    	}
                    	buffPtr[index++] = *s;
					}

                    break;
                }
                case 'c': { // Character
                	// char and short are automatically promoted to int when passed through ....
                    uint8_t c = (uint8_t)va_arg(ap, int);

                    if (index >= maxSpace)
					{
                    	ret = RC_ERROR_OVERRUN;
						break;
					}

                    buffPtr[index++] = c;

                    break;
                }
                case 's': { // String
                    char *s = va_arg(ap, char *);

					while (*s)
					{
						if (index >= maxSpace)
						{
							ret = RC_ERROR_OVERRUN;
							break;
						}

						buffPtr[index++] = *s++;
					}

                    break;
                }
                default:
                	/**
                	 * Here we handle the identifier that is not defined yet
                	*/
                {
                	if (index >= maxSpace)
					{
                		ret = RC_ERROR_OVERRUN;
						break;
					}

                	 buffPtr[index++] = *p;

                    break;
                }
            }
        } else {

        	if (index >= maxSpace)
			{
        		ret = RC_ERROR_OVERRUN;
				break;
			}

        	buffPtr[index++] = *p;
        }
        p++;
    }

    buffPtr[index] = '\0';
    va_end(ap);

    return ret;
}

/**
 * \brief snprintf like function. This is a stand alone function, can be used
 * to construct a string from the given format
 *
 * \param uint8_t *buffer				: [IN/OUT] Buffer contains the converted string
 * \param const char *format value		: [IN] String with the format like sprintf
 * \param uint16_t buffSize				: [IN] Size of the input buffer
 * \param ...							: [IN] List of the arguments
 * \return RC_t							: [OUT] RC_ERROR_UNDERRUN if the provided buffer size is not
 * large enough, RC_SUCCESS if success
 */
RC_t IO_snprintf(uint8_t *buffer, uint16_t buffSize, const char *format, ...) {
    va_list args;
    va_start(args, format);

    RC_t ret = IO_snprintf_va(buffer, buffSize, format, &args);

    va_end(args);

    return ret;
}

//--------------------------------------------------------------------------
