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
 * 
 * For calculation of tables please check //http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
 */
#include "crc_type.h"


#define CRC_TABLESIZE   256

/**
 * CRC8 CCITT 
 *
 * Initial Value    0
 * Polynomial       0x07
 *
 **/
extern const uint8_t  CRC_table8_ccitt[CRC_TABLESIZE];

/**
 * CRC16 CCITT 
 *
 * Initial Value    0
 * Polynomial       0x2021
 *
 **/
extern const uint16_t CRC_table16_ccitt[CRC_TABLESIZE];


//----- CRC Objects

// CRC for SPI DU/DECU communication
extern const CRC16_t CRC16_SPI_FPD;

// CRC for I2C SIS95 transfer, note, only lowbyte is used
extern const CRC16_t CRC16_I2C_SIS95;