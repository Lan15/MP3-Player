/**
* \file wav.c
* \author V S Agilan (original reference), refactored for SD card streaming
* \date 2025
*
* \brief WAV file header parser — implementation
*
* Reads the 44-byte RIFF/WAV header from a FatFs FIL handle, converts all
* multi-byte fields from little-endian wire format to host byte order, and
* validates format constraints required by the VDAC8 hardware.
*
* \copyright Copyright 2025
* Department of electrical engineering and information technology,
* Hochschule Darmstadt - University of applied sciences (h_da).
*
* \warning This software is a PROTOTYPE version and is not designed or
* intended for use in production, especially not for safety-critical
* applications!
*/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "wav_parser.h"
#include "project.h"    /* UART_LOG_PutString                               */
#include <string.h>     /* memcpy, strcmp                                   */
#include <stdio.h>      /* sprintf (debug only)                             */

/*****************************************************************************/
/* Local pre-processor symbols/macros ('#define')                            */
/*****************************************************************************/

/** \brief Raw header byte layout — total must equal WAV_HEADER_SIZE_BYTES */
#define WAV__OFF_FILE_ID            (0u)
#define WAV__OFF_FILE_SIZE          (4u)
#define WAV__OFF_FORMAT             (8u)
#define WAV__OFF_SUBCHUNK_ID        (12u)
#define WAV__OFF_SUBCHUNK_SIZE      (16u)
#define WAV__OFF_AUDIO_FORMAT       (20u)
#define WAV__OFF_NUM_CHANNELS       (22u)
#define WAV__OFF_SAMPLE_RATE        (24u)
#define WAV__OFF_BYTE_RATE          (28u)
#define WAV__OFF_BLOCK_ALIGN        (32u)
#define WAV__OFF_BITS_PER_SAMPLE    (34u)
#define WAV__OFF_DATA_ID            (36u)
#define WAV__OFF_DATA_SIZE          (40u)

/*****************************************************************************/
/* Local type definitions ('typedef')                                        */
/*****************************************************************************/

/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Local function prototypes ('static')                                      */
/*****************************************************************************/

/**
 * \brief Read a 32-bit little-endian value from a byte array.
 *
 * The PSoC 5LP (Cortex-M3) is itself little-endian, so this is a safe
 * byte-by-byte reconstruction that avoids unaligned access and is
 * endian-explicit rather than relying on cast behaviour.
 *
 * \param[in] data  Pointer to the first of four bytes
 * \return          32-bit value in host (little-endian) byte order
 */
static uint32_t WAV__ReadU32(uint8_t const * const data);

/**
 * \brief Read a 16-bit little-endian value from a byte array.
 *
 * \param[in] data  Pointer to the first of two bytes
 * \return          16-bit value in host byte order
 */
static uint16_t WAV__ReadU16(uint8_t const * const data);

/**
 * \brief Copy n bytes to a char buffer and null-terminate.
 *
 * \param[in]  src   Source byte array
 * \param[out] dst   Destination char buffer (must be at least n+1 bytes)
 * \param[in]  n     Number of bytes to copy
 */
static void WAV__BytesToString(uint8_t const * const src,
                               char          * const dst,
                               size_t                n);

/**
 * \brief Validate that format constraints match VDAC8 hardware requirements.
 *
 * \param[in] hdr  Pointer to a decoded WAVHeader
 * \return RC_SUCCESS if all constraints pass, RC_ERROR otherwise
 */
static RC_t WAV__ValidateFormat(WAVHeader const * const hdr);

/*****************************************************************************/
/* Function implementation - global ('extern') and local ('static')          */
/*****************************************************************************/

/* --------------------------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------------------------- */

static uint32_t WAV__ReadU32(uint8_t const * const data)
{
    return (uint32_t)data[0]
         | ((uint32_t)data[1] << 8u)
         | ((uint32_t)data[2] << 16u)
         | ((uint32_t)data[3] << 24u);
}

static uint16_t WAV__ReadU16(uint8_t const * const data)
{
    return (uint16_t)data[0]
         | ((uint16_t)data[1] << 8u);
}

static void WAV__BytesToString(uint8_t const * const src,
                               char          * const dst,
                               size_t                n)
{
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static RC_t WAV__ValidateFormat(WAVHeader const * const hdr)
{
    if (strcmp(hdr->file_id,     "RIFF") != 0)
    {
        UART_LOG_PutString("[WAV] ERROR: not a RIFF file\n");
        return RC_ERROR;
    }
    if (strcmp(hdr->format,      "WAVE") != 0)
    {
        UART_LOG_PutString("[WAV] ERROR: RIFF format is not WAVE\n");
        return RC_ERROR;
    }
    if (strcmp(hdr->data_id,     "data") != 0)
    {
        UART_LOG_PutString("[WAV] ERROR: data sub-chunk not found\n");
        return RC_ERROR;
    }
    if (hdr->audio_format != WAV_AUDIO_FORMAT_PCM)
    {
        UART_LOG_PutString("[WAV] ERROR: not PCM (compressed audio)\n");
        return RC_ERROR;
    }
    if (hdr->number_of_channels != WAV_REQUIRED_CHANNELS)
    {
        UART_LOG_PutString("[WAV] ERROR: not mono\n");
        return RC_ERROR;
    }
    if (hdr->bits_per_sample != WAV_REQUIRED_BITS_PER_SAMPLE)
    {
        UART_LOG_PutString("[WAV] ERROR: not 8-bit\n");
        return RC_ERROR;
    }

    return RC_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * Parse the 44-byte RIFF/WAV header from an open FatFs file.
 * On success, the FIL read pointer is at byte 44 (first PCM sample).
 */
RC_t WAV_ParseHeader(FIL * const file, WAVFile * const wav)
{
    uint8_t  raw[WAV_HEADER_SIZE_BYTES];
    UINT     bytes_read = 0u;
    FRESULT  fr;

    /* ------------------------------------------------------------------ */
    /* 1. Read the raw 44-byte header from SD card via FatFs               */
    /* ------------------------------------------------------------------ */
    fr = f_read(file, raw, WAV_HEADER_SIZE_BYTES, &bytes_read);

    if (fr != FR_OK)
    {
        UART_LOG_PutString("[WAV] ERROR: f_read failed on header\n");
        return RC_ERROR;
    }
    if (bytes_read != WAV_HEADER_SIZE_BYTES)
    {
        UART_LOG_PutString("[WAV] ERROR: header too short (file truncated?)\n");
        return RC_ERROR;
    }

    /* ------------------------------------------------------------------ */
    /* 2. Decode each field from the raw byte array                        */
    /*    Using explicit byte-by-byte reads to avoid unaligned access and  */
    /*    to be endian-explicit (WAV is always little-endian on disk)       */
    /* ------------------------------------------------------------------ */
    WAVHeader * const hdr = &wav->header;

    WAV__BytesToString(&raw[WAV__OFF_FILE_ID],     hdr->file_id,     4u);
    hdr->file_size          = WAV__ReadU32(&raw[WAV__OFF_FILE_SIZE]);
    WAV__BytesToString(&raw[WAV__OFF_FORMAT],      hdr->format,      4u);
    WAV__BytesToString(&raw[WAV__OFF_SUBCHUNK_ID], hdr->subchunk_id, 4u);
    hdr->subchunk_size      = WAV__ReadU32(&raw[WAV__OFF_SUBCHUNK_SIZE]);
    hdr->audio_format       = WAV__ReadU16(&raw[WAV__OFF_AUDIO_FORMAT]);
    hdr->number_of_channels = WAV__ReadU16(&raw[WAV__OFF_NUM_CHANNELS]);
    hdr->sample_rate        = WAV__ReadU32(&raw[WAV__OFF_SAMPLE_RATE]);
    hdr->byte_rate          = WAV__ReadU32(&raw[WAV__OFF_BYTE_RATE]);
    hdr->block_align        = WAV__ReadU16(&raw[WAV__OFF_BLOCK_ALIGN]);
    hdr->bits_per_sample    = WAV__ReadU16(&raw[WAV__OFF_BITS_PER_SAMPLE]);
    WAV__BytesToString(&raw[WAV__OFF_DATA_ID],     hdr->data_id,     4u);
    hdr->data_size          = WAV__ReadU32(&raw[WAV__OFF_DATA_SIZE]);

    /* ------------------------------------------------------------------ */
    /* 3. Validate format constraints for VDAC8 hardware                   */
    /* ------------------------------------------------------------------ */
    if (WAV__ValidateFormat(hdr) != RC_SUCCESS)
    {
        return RC_ERROR;
    }

    /* ------------------------------------------------------------------ */
    /* 4. Populate the WAVFile descriptor                                  */
    /*    FIL is now at byte 44 — first PCM sample                        */
    /* ------------------------------------------------------------------ */
    wav->data_length = hdr->data_size;

    return RC_SUCCESS;
}

/**
 * Print all decoded header fields to UART for debugging.
 */
void WAV_PrintHeader(WAVFile const * const wav)
{
    char buf[64];
    WAVHeader const * const hdr = &wav->header;

    UART_LOG_PutString("\n--- WAV Header ---\n");

    sprintf(buf, "  FileID        : %s\n",   hdr->file_id);          UART_LOG_PutString(buf);
    sprintf(buf, "  Format        : %s\n",   hdr->format);            UART_LOG_PutString(buf);
    sprintf(buf, "  SubchunkID    : %s\n",   hdr->subchunk_id);       UART_LOG_PutString(buf);
    sprintf(buf, "  AudioFormat   : %u\n",   hdr->audio_format);      UART_LOG_PutString(buf);
    sprintf(buf, "  Channels      : %u\n",   hdr->number_of_channels);UART_LOG_PutString(buf);
    sprintf(buf, "  SampleRate    : %lu Hz\n", hdr->sample_rate);     UART_LOG_PutString(buf);
    sprintf(buf, "  ByteRate      : %lu\n",  hdr->byte_rate);         UART_LOG_PutString(buf);
    sprintf(buf, "  BlockAlign    : %u\n",   hdr->block_align);       UART_LOG_PutString(buf);
    sprintf(buf, "  BitsPerSample : %u\n",   hdr->bits_per_sample);   UART_LOG_PutString(buf);
    sprintf(buf, "  DataID        : %s\n",   hdr->data_id);           UART_LOG_PutString(buf);
    sprintf(buf, "  DataSize      : %lu bytes\n", hdr->data_size);    UART_LOG_PutString(buf);
    sprintf(buf, "  Duration      : ~%lu s\n",
            hdr->data_size / hdr->sample_rate);                        UART_LOG_PutString(buf);
    UART_LOG_PutString("------------------\n");
}
