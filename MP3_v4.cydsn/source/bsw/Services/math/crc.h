/**
 * \file 	crc_cfg.h
 * \author 	Marzieh Gharibi	- Hochschule Darmstadt 
 * \date 	13.01.2023
 * \version	1.0
 *
 * \brief CRC calculations
 *
 *
 *  Changelog:\n
 *  - 1.0 13.01.23 Gharibi
 *  	- Initial creation
 *
 * \copyright Copyright ©2018
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

#include "global.h"
#include "crc_type.h"
#include "crc_cfg.h"

/**
 * calculating crc8 
 * \param   CRC8_t const * const me     : [IN] configuration for the CRC calculation
 * \param   uint8_t* data               : [IN] pointer to data
 * \param   uint16_t len                : [IN] size of data
 * \return calculated CRC8 value
*/
uint8_t CRC_calc8(CRC8_t const * const me, uint8_t* data, uint16_t len);

/**
 * calculating crc16
 * \param   CRC16_t const * const me    : [IN] configuration for the CRC calculation
 * \param   uint8_t* data               : [IN] pointer to data
 * \param   uint16_t len                : [IN] size of data
 * \return calculated CRC8 value
*/
uint16_t CRC_calc16(CRC16_t const * const me, uint8_t* data, uint16_t len);
