/**
* \file wav_parser.c
* \author V S Agilan
* \date 2025
*
* \brief WAV file header parser — emFile version with dynamic chunk search
*
* Handles WAV files that have extra chunks (LIST, INFO etc.) between
* the fmt chunk and data chunk. Searches for 'data' chunk dynamically
* instead of assuming it is always at byte 36.
*/

#include "wav_parser.h"
#include "project.h"
#include <string.h>
#include <stdio.h>

/*****************************************************************************/
/* Local pre-processor symbols/macros                                        */
/*****************************************************************************/

#define WAV__OFF_FILE_ID         (0u)
#define WAV__OFF_FILE_SIZE       (4u)
#define WAV__OFF_FORMAT          (8u)
#define WAV__OFF_SUBCHUNK_ID     (12u)
#define WAV__OFF_SUBCHUNK_SIZE   (16u)
#define WAV__OFF_AUDIO_FORMAT    (20u)
#define WAV__OFF_NUM_CHANNELS    (22u)
#define WAV__OFF_SAMPLE_RATE     (24u)
#define WAV__OFF_BYTE_RATE       (28u)
#define WAV__OFF_BLOCK_ALIGN     (32u)
#define WAV__OFF_BITS_PER_SAMPLE (34u)

/* First 36 bytes: RIFF header + fmt chunk */
#define WAV__RIFF_FMT_SIZE       (36u)

/* Max bytes to scan forward looking for data chunk */
#define WAV__MAX_SCAN            (256u)

/*****************************************************************************/
/* Local function prototypes                                                 */
/*****************************************************************************/

static uint32_t WAV__ReadU32(uint8_t const * const data);
static uint16_t WAV__ReadU16(uint8_t const * const data);
static void     WAV__BytesToString(uint8_t const * const src,
                                   char          * const dst,
                                   size_t                n);
static RC_t     WAV__ValidateFormat(WAVHeader const * const hdr);

/*****************************************************************************/
/* Private helpers                                                           */
/*****************************************************************************/

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
    if (strcmp(hdr->file_id, "RIFF") != 0)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not RIFF\n");
        return RC_ERROR;
    }
    if (strcmp(hdr->format, "WAVE") != 0)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not WAVE\n");
        return RC_ERROR;
    }
    if (hdr->audio_format != WAV_AUDIO_FORMAT_PCM)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not PCM\n");
        return RC_ERROR;
    }
    if (hdr->number_of_channels != WAV_REQUIRED_CHANNELS)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not mono\n");
        return RC_ERROR;
    }
    if (hdr->bits_per_sample != WAV_REQUIRED_BITS_PER_SAMPLE)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not 8-bit\n");
        return RC_ERROR;
    }
    return RC_SUCCESS;
}

/*****************************************************************************/
/* Public API                                                                */
/*****************************************************************************/

RC_t WAV_ParseHeader(FS_FILE * const pFile, WAVFile * const wav)
{
    uint8_t  raw[WAV__RIFF_FMT_SIZE];
    U32      bytesRead = 0u;
    char     logBuf[48];

    /* Read first 36 bytes — RIFF header + fmt chunk */
    bytesRead = FS_Read(pFile, raw, WAV__RIFF_FMT_SIZE);
    if (bytesRead != WAV__RIFF_FMT_SIZE)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: header read failed\n");
        return RC_ERROR;
    }

    /* Decode fixed fields */
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

    /* Validate format constraints for VDAC8 hardware */
    if (WAV__ValidateFormat(hdr) != RC_SUCCESS)
    {
        return RC_ERROR;
    }

    /* ------------------------------------------------------------------ */
    /* Search dynamically for the 'data' chunk                             */
    /* WAV files can have extra chunks (LIST, INFO) before 'data'          */
    /* This file has: fmt(byte 12) -> LIST(byte 36) -> data(byte 70)      */
    /* ------------------------------------------------------------------ */
    uint8_t  chunkHdr[8];
    char     chunkID[5];
    uint32_t chunkSize   = 0u;
    uint32_t bytesScanned = 0u;
    uint8_t  found       = 0u;

    while (bytesScanned < WAV__MAX_SCAN)
    {
        bytesRead = FS_Read(pFile, chunkHdr, 8u);
        if (bytesRead != 8u) break;

        WAV__BytesToString(chunkHdr, chunkID, 4u);
        chunkSize = WAV__ReadU32(&chunkHdr[4]);

        sprintf(logBuf, "\r\n[WAV] Chunk: '%s' size=%lu\n", chunkID, chunkSize);
        UART_LOG_PutString(logBuf);

        if (strcmp(chunkID, "data") == 0)
        {
            hdr->data_size   = chunkSize;
            wav->data_length = chunkSize;
            found = 1u;
            break;
        }

        /* Skip this chunk — not 'data' */
        uint8_t  skipBuf[16];
        uint32_t remaining = chunkSize;
        while (remaining > 0u)
        {
            uint32_t toRead = (remaining > 16u) ? 16u : remaining;
            FS_Read(pFile, skipBuf, toRead);
            remaining -= toRead;
        }
        bytesScanned += 8u + chunkSize;
    }

    if (found == 0u)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: data chunk not found\n");
        return RC_ERROR;
    }

    /* File pointer now sits at first PCM sample — ready for streaming */
    return RC_SUCCESS;
}

void WAV_PrintHeader(WAVFile const * const wav)
{
    char buf[64];
    WAVHeader const * const hdr = &wav->header;

    UART_LOG_PutString("\r\n\n--- WAV Header ---\n");
    sprintf(buf, "\r\n  FileID        : %s\n",       hdr->file_id);            UART_LOG_PutString(buf);
    sprintf(buf, "\r\n  Format        : %s\n",       hdr->format);             UART_LOG_PutString(buf);
    sprintf(buf, "\r\n  AudioFormat   : %u\n",       hdr->audio_format);       UART_LOG_PutString(buf);
    sprintf(buf, "\r\n  Channels      : %u\n",       hdr->number_of_channels); UART_LOG_PutString(buf);
    sprintf(buf, "\r\n  SampleRate    : %lu Hz\n",   hdr->sample_rate);        UART_LOG_PutString(buf);
    sprintf(buf, "\r\n  BitsPerSample : %u\n",       hdr->bits_per_sample);    UART_LOG_PutString(buf);
    sprintf(buf, "\r\n  DataSize      : %lu bytes\n",hdr->data_size);          UART_LOG_PutString(buf);
    sprintf(buf, "\r\n  Duration      : ~%lu s\n",
            hdr->data_size / hdr->sample_rate);                                UART_LOG_PutString(buf);
    UART_LOG_PutString("\r\n------------------\n");
}