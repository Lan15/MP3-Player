/**
* \file    wav_parser.h
* \author  V S Agilan
* \date    2025 (INC 11A: 2026-10)
*
* \brief   WAV file header parser (emFile), refactored for SD card streaming
*
* \details
* Parses the header of a RIFF/WAVE file that was opened with FS_FOpen() and
* leaves the file position on the first PCM byte, ready for streaming.
*
* The header is NOT assumed to be a fixed 44 bytes. The parser walks the RIFF
* chunk list, so files with extra chunks (LIST, INFO, JUNK, ...) or with a
* WAVE_FORMAT_EXTENSIBLE fmt chunk are accepted.
*
* Constraints enforced by this driver (match the VDAC8 output and the
* 22.05 kHz sample timer):
*   - AudioFormat   == 1 (PCM; WAVE_FORMAT_EXTENSIBLE with PCM sub-format is resolved to 1)
*   - NumChannels   == 1 (mono) or 2 (stereo, mixed to mono by the player)
*   - BitsPerSample == 8, 16 or 24
*   - SampleRate    == 22050 Hz (played directly) or 44100 Hz (decimated by 2)
*
* \note
*   Recommended Audacity export settings:
*   Format : WAV (Microsoft), Rate : 22050 Hz, Channels : 1 (mono),
*   Encoding : Unsigned 8-bit PCM (any of the supported bit depths works).
*
* \warning This software is a PROTOTYPE version and is not designed or
* intended for use in production, especially not for safety-critical
* applications!
*
* \warning WAV_ParseHeader() uses one static read window and is therefore not
* re-entrant. It must only be called from one task (tsk_reader).
*
* Changelog:
*   - 2025    : initial version, emFile streaming API, Doxygen style
*   - INC 5A  : 16-bit PCM accepted
*   - INC 6A  : 24-bit PCM, stereo, fmt extension (WAVE_FORMAT_EXTENSIBLE)
*   - INC 6D  : 44100 Hz accepted (decimated by the player)
*   - INC 11A : chunk walker with a single read window (fewer SD reads),
*               RIFF pad byte handled, data length clamped to the file size,
*               fmt chunk accepted at any position, no stdio (MISRA 21.6),
*               Doxygen comments, notes N1..N9 at the end of wav_parser.c
*
* \copyright Copyright 2025
* Department of electrical engineering and information technology,
* Hochschule Darmstadt - University of applied sciences (h_da).
*/

#ifndef WAV_PARSER_H
#define WAV_PARSER_H

#include "global.h"
#include <FS.h>          /* emFile - FS_FILE, FS_Read, FS_FSeek */
#include <stdint.h>
#include <stddef.h>

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

#define WAV_AUDIO_FORMAT_PCM         (1u)   /**< \brief WAVE format tag for uncompressed PCM */
#define WAV_HEADER_SIZE_BYTES        (44u)  /**< \brief Size of the canonical header (informational only, the parser does not rely on it) */

/**
 * \brief Decoded WAV header fields (little-endian values already converted).
 *
 * The byte positions in the comments are those of the canonical 44-byte
 * header. With extra chunks the positions of the fields behind fmt move.
 */
typedef struct WAVHeader_t
{
    char     file_id[5];         /**< "RIFF" + NUL         [0..3]   */
    uint32_t file_size;          /**< RIFF size            [4..7]   */
    char     format[5];          /**< "WAVE" + NUL         [8..11]  */
    char     subchunk_id[5];     /**< "fmt " + NUL         [12..15] */
    uint32_t subchunk_size;      /**< fmt size, 16 for PCM [16..19] */
    uint16_t audio_format;       /**< 1 = PCM              [20..21] */
    uint16_t number_of_channels; /**< 1 = mono, 2 = stereo [22..23] */
    uint32_t sample_rate;        /**< 22050 or 44100       [24..27] */
    uint32_t byte_rate;          /**< bytes per second     [28..31] */
    uint16_t block_align;        /**< bytes per frame      [32..33] */
    uint16_t bits_per_sample;    /**< 8, 16 or 24          [34..35] */
    char     data_id[5];         /**< "data" + NUL         [36..39] */
    uint32_t data_size;          /**< PCM bytes as declared in the file [40..43] */
} WAVHeader;

/**
 * \brief A parsed WAV file: header plus the number of playable PCM bytes.
 */
typedef struct WAVFile_t
{
    WAVHeader header;        /**< decoded header                                          */
    uint32_t  data_length;   /**< PCM bytes that can really be read: header.data_size,
                                  limited to what is left in the file. Used for EOF.     */
} WAVFile;

/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/

/**
 * \brief Parse the header of an open emFile WAV file.
 *
 * Walks the RIFF chunk list, decodes the fmt chunk, validates the format
 * for the VDAC8 hardware and positions the file on the first PCM byte of
 * the data chunk. The function seeks to offset 0 itself.
 *
 * \param[in]  pFile  Open emFile FS_FILE handle (mode "r")
 * \param[out] wav    WAVFile structure to populate
 * \return RC_SUCCESS header valid, file position = first PCM byte (FS_FTell() gives the data offset)
 * \return RC_ERROR   read/seek failed, not a RIFF/WAVE file, unsupported format or no data chunk
 *
 * \note On error a line "[WAV] ERROR: ..." is written to the UART log.
 */
RC_t WAV_ParseHeader(FS_FILE * const pFile, WAVFile * const wav);

/**
 * \brief Print all WAV header fields to the UART log.
 *
 * \param[in] wav  Parsed WAV file (result of WAV_ParseHeader())
 */
void WAV_PrintHeader(WAVFile const * const wav);

#endif /* WAV_PARSER_H */