/**
* \file wav_parser.h
* \author V S Agilan (original reference), refactored for SD card streaming
* \date 2025
*
* \brief WAV file header parser for PSoC 5LP MP3 player
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
#include "Libraries\FatFs_Library\diskio.h"
#include "Libraries\FatFs_Library\ff.h"     /* FatFs f_read / FIL                           */
#include <stdint.h>
#include <stddef.h>

/*****************************************************************************/
/* Global pre-processor symbols/macros and type declarations                 */
/*****************************************************************************/

/** \brief Expected value of WAVHeader.audio_format for uncompressed PCM */
#define WAV_AUDIO_FORMAT_PCM        (1u)

/** \brief Required number of channels (mono) */
#define WAV_REQUIRED_CHANNELS       (1u)

/** \brief Required bits per sample matching VDAC8 */
#define WAV_REQUIRED_BITS_PER_SAMPLE (8u)

/** \brief Total size of the RIFF WAV header in bytes */
#define WAV_HEADER_SIZE_BYTES       (44u)


/**
 * \brief Normalised representation of the 44-byte RIFF WAV header.
 *
 * All multi-byte fields are stored in host byte order (little-endian on
 * Cortex-M3) after the little2big conversion performed during parsing.
 * String fields carry a null terminator so they can be used with strcmp().
 */
typedef struct WAVHeader_t
{
    /** \brief "RIFF" — identifies RIFF container format               [0..3]  */
    char     file_id[5];

    /** \brief Total file size minus 8 (ChunkID + ChunkSize)           [4..7]  */
    uint32_t file_size;

    /** \brief "WAVE" — identifies WAV sub-format                      [8..11] */
    char     format[5];

    /** \brief "fmt " — format sub-chunk marker                        [12..15]*/
    char     subchunk_id[5];

    /** \brief Size of fmt sub-chunk body: 16 for PCM                  [16..19]*/
    uint32_t subchunk_size;

    /** \brief Audio format: 1 = PCM (uncompressed)                    [20..21]*/
    uint16_t audio_format;

    /** \brief Number of channels: 1 = mono, 2 = stereo                [22..23]*/
    uint16_t number_of_channels;

    /** \brief Samples per second, e.g. 22050                          [24..27]*/
    uint32_t sample_rate;

    /** \brief sample_rate * number_of_channels * bits_per_sample / 8  [28..31]*/
    uint32_t byte_rate;

    /** \brief number_of_channels * bits_per_sample / 8                [32..33]*/
    uint16_t block_align;

    /** \brief Bits per single sample (not bytes). Must be 8.          [34..35]*/
    uint16_t bits_per_sample;

    /** \brief "data" — data sub-chunk marker                          [36..39]*/
    char     data_id[5];

    /**
     * \brief Exact byte count of the PCM data that follows.           [40..43]
     *
     * = number_of_samples * number_of_channels * bits_per_sample / 8
     *
     * Used by tsk_reader to detect the final (possibly partial) buffer
     * fill at end-of-file.
     */
    uint32_t data_size;

} WAVHeader;


/**
 * \brief Aggregated WAV file descriptor returned by WAV_ParseHeader().
 *
 * The header field contains a fully decoded copy of the 44-byte RIFF header.
 * After WAV_ParseHeader() returns RC_SUCCESS the associated FIL handle is
 * positioned at byte 44 (the first PCM sample) and may be passed directly
 * to f_read() for streaming.
 */
typedef struct WAVFile_t
{
    /** \brief Decoded WAV header with all fields in host byte order */
    WAVHeader header;

    /**
     * \brief Total number of PCM bytes in the data chunk.
     *
     * Convenience copy of header.data_size.
     * tsk_reader uses this to compute remaining bytes before each f_read()
     * and to detect when the last buffer fill is partial.
     */
    uint32_t data_length;

} WAVFile;


/*****************************************************************************/
/* API functions                                                             */
/*****************************************************************************/

/**
 * \brief Parse the 44-byte RIFF/WAV header from an open FatFs file.
 *
 * Reads exactly WAV_HEADER_SIZE_BYTES (44) bytes from the current position
 * of \p file (which must be at offset 0), decodes all fields using
 * little-endian conversion, validates format constraints required by the
 * VDAC8 hardware, and populates \p wav.
 *
 * On success the FIL read pointer is advanced to byte 44, ready for
 * streaming PCM data directly into the dual buffer via f_read().
 *
 * \param[in]  file   Pointer to an open FatFs FIL handle (must be at offset 0)
 * \param[out] wav    Pointer to a WAVFile struct to populate
 *
 * \return RC_SUCCESS  Header valid, \p wav populated, FIL at byte 44
 * \return RC_ERROR    File read failed, magic bytes wrong, or format mismatch
 *                     (not PCM / not mono / not 8-bit)
 */
RC_t WAV_ParseHeader(FIL * const file, WAVFile * const wav);


/**
 * \brief Print WAV header fields to UART for debugging.
 *
 * Outputs all decoded header fields via UART_LOG_PutString().
 * Call after WAV_ParseHeader() returns RC_SUCCESS.
 *
 * \param[in] wav  Pointer to a populated WAVFile struct
 */
void WAV_PrintHeader(WAVFile const * const wav);


#endif /* WAV_PARSER_H */
