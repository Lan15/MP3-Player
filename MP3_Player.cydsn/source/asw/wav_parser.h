/**
* \file wav_parser.h
* \author V S Agilan (original reference), refactored for SD card streaming
* \date 2025
*
* \brief WAV file header parser
*
* Parses the 44-byte RIFF/WAV header from a FatFs file handle.
* After a successful parse the FIL read pointer is positioned at byte 44,
* i.e. the first PCM sample, ready for direct streaming into the dual buffer.
*
* Constraints enforced by this driver (matching VDAC8 hardware):
*   - AudioFormat   == 1   (PCM, no compression)
*   - NumChannels   == 1   (mono)
*   - BitsPerSample == 8   (unsigned, 0-255)
*
* \note
*   The WAV file must be exported with the following Audacity settings:
*   Format : WAV (Microsoft)
*   Encoding: Unsigned 8-bit PCM
*   Rate    : 22050 Hz
*   Channels: 1 (mono)
*
* \warning This software is a PROTOTYPE version and is not designed or
* intended for use in production, especially not for safety-critical
* applications!
*
* Changelog:
*   - 2025: initial version, FatFs streaming API, Doxygen style
*
* \copyright Copyright 2025
* Department of electrical engineering and information technology,
* Hochschule Darmstadt - University of applied sciences (h_da).
*/

#ifndef WAV_PARSER_H
#define WAV_PARSER_H

#include "global.h"
#include <FS.h>          /* emFile — FS_FILE, FS_Read */
#include <stdint.h>
#include <stddef.h>

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#define WAV_AUDIO_FORMAT_PCM         (1u)
#define WAV_REQUIRED_CHANNELS        (1u)
#define WAV_REQUIRED_BITS_PER_SAMPLE (8u)
#define WAV_HEADER_SIZE_BYTES        (44u)

typedef struct WAVHeader_t
{
    char     file_id[5];         /* "RIFF"        [0..3]   */
    uint32_t file_size;          /*               [4..7]   */
    char     format[5];          /* "WAVE"        [8..11]  */
    char     subchunk_id[5];     /* "fmt "        [12..15] */
    uint32_t subchunk_size;      /* 16 for PCM   [16..19] */
    uint16_t audio_format;       /* 1 = PCM      [20..21] */
    uint16_t number_of_channels; /* 1 = mono     [22..23] */
    uint32_t sample_rate;        /* e.g. 22050   [24..27] */
    uint32_t byte_rate;          /*               [28..31] */
    uint16_t block_align;        /*               [32..33] */
    uint16_t bits_per_sample;    /* must be 8    [34..35] */
    char     data_id[5];         /* "data"       [36..39] */
    uint32_t data_size;          /* PCM bytes    [40..43] */
} WAVHeader;

typedef struct WAVFile_t
{
    WAVHeader header;
    uint32_t  data_length;   /* copy of header.data_size — used for EOF */
} WAVFile;

/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/

/**
 * \brief Parse the 44-byte RIFF/WAV header from an open emFile file.
 *
 * Reads exactly 44 bytes from \p pFile (must be at offset 0),
 * decodes all fields, validates format constraints for VDAC8 hardware.
 * On success the file pointer is at byte 44 — first PCM sample.
 *
 * \param[in]  pFile  Open emFile FS_FILE handle (position must be 0)
 * \param[out] wav    WAVFile struct to populate
 * \return RC_SUCCESS — header valid, file at byte 44
 * \return RC_ERROR   — read failed or format mismatch
 */
RC_t WAV_ParseHeader(FS_FILE * const pFile, WAVFile * const wav);

/**
 * \brief Print all WAV header fields to UART.
 */
void WAV_PrintHeader(WAVFile const * const wav);

#endif /* WAV_PARSER_H */