/*******************************************************************************
* File Name: crc.c
*
* Version: 1.10
*
* author : marzi
* Description: the crc8 and crc16 calculation (table calculation is also included)
*
********************************************************************************
*******************************************************************************/

#include "crc.h"



/**
 * calculating crc8 
 * \param   CRC8_t const * const me     : [IN] configuration for the CRC calculation
 * \param   uint8_t* data               : [IN] pointer to data
 * \param   uint16_t len                : [IN] size of data
 * \return calculated CRC8 value
*/
uint8_t CRC_calc8(CRC8_t const * const me, uint8_t* data, uint16_t len)
{
     uint8_t crc = 0x00; // init value
     for (uint16_t i = 0; i < len; i++) {
          crc =  me->table[data[i] ^ crc];
      }
    return crc;
 }


/**
 * calculating crc16
 * \param   CRC16_t const * const me    : [IN] configuration for the CRC calculation
 * \param   uint8_t* data               : [IN] pointer to data
 * \param   uint16_t len                : [IN] size of data
 * \return calculated CRC8 value
*/
uint16_t CRC_calc16(CRC16_t const * const me, uint8_t* data, uint16_t len)
{
	uint16_t crc = 0;   //Initial value
    for (uint16_t i = 0; i < len; i++)
    {
        /* XOR-in next input byte into MSB of crc, that's our new intermediate divident */
    	uint8_t pos = (uint8_t)( (crc >> 8) ^ data[i]); /* equal: ((crc ^ (b << 8)) >> 8) */
        /* Shift out the MSB used for division per lookuptable and XOR with the remainder */
        crc = (uint16_t)((crc << 8) ^ (uint16_t)(me->table[pos]));

    }

    return crc;
}

#if 0
#
/**
* this function calculate the table for crc8 ,if you want to calculate crc8 of a different generator 
* you can invoke this function,otherwise do not call this function since the pre-computed table is in crc_table.h
*/
static void CalulateTable_CRC8()
{
    const uint8_t generator = 0x07;

    /* iterate over all byte values 0 - 255 */
    for (int divident = 0; divident < 256; divident++)
    {
    	uint8_t currByte = (uint8_t)divident;
        /* calculate the CRC-8 value for current byte */
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((currByte & 0x80) != 0)
            {
                currByte <<= 1;
                currByte ^= generator;
            }
            else
            {
                currByte <<= 1;
            }
        }
        /* store CRC value in lookup table */
        crctable8[divident] = currByte;

    }
}
/**
* this function calculate the table for crc16 ,if you want to calculate crc16 of a different generator 
* (different types of crc16)you can invoke this function,otherwise do not call this function
* since the pre-computed table is
* in crc_table.h
*/
void CalculateTable_CRC16()
{
    const uint16_t generator = 0x1021;


    for (int divident = 0; divident < 256; divident++) /* iterate over all possible input byte values 0 - 255 */
    {
    	uint16_t curByte = (uint16_t)(divident << 8); /* move divident byte into MSB of 16Bit CRC */

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((curByte & 0x8000) != 0)
            {
                curByte <<= 1;
                curByte ^= generator;
            }
            else
            {
                curByte <<= 1;
            }
        }
        crctable16[divident] = curByte;
          
    }
}

#endif