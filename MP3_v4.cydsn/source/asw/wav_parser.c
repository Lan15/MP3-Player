/**
* \file    wav_parser.c
* \author  V S Agilan
* \date    2025 (INC 11A: 2026-10)
*
* \brief   WAV file header parser (emFile) with RIFF chunk walker
*
* \details
* Reads the RIFF/WAVE header of an open emFile file and leaves the file
* position on the first PCM byte.
*
* The chunk list is walked instead of assuming that 'data' sits at byte 36:
*   RIFF header (12) -> chunk -> chunk -> ... -> 'data'
* Chunks other than 'fmt ' and 'data' (LIST, INFO, JUNK, ...) are skipped.
*
* Header bytes are fetched through one 256 byte read window (N1), so a
* typical file costs a single SD sector read instead of one read per field.
*
* Design notes N1..N9 are collected at the end of this file.
*/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "wav_parser.h"
#include "project.h"        /* UART_LOG_PutString() */
#include <string.h>         /* memcpy() */

/*****************************************************************************/
/* Local pre-processor symbols/macros                                        */
/*****************************************************************************/

/** \name RIFF container tags
 *  Four ASCII characters read as one little-endian 32 bit value (N2).
 *  \{ */
#define WAV__TAG_RIFF            (0x46464952u)  /**< "RIFF" */
#define WAV__TAG_WAVE            (0x45564157u)  /**< "WAVE" */
#define WAV__TAG_FMT             (0x20746D66u)  /**< "fmt " (with trailing blank) */
#define WAV__TAG_DATA            (0x61746164u)  /**< "data" */
/** \} */

/** \name RIFF file header (first 12 bytes)
 *  \{ */
#define WAV__OFF_FILE_ID         (0u)           /**< "RIFF" tag                        */
#define WAV__OFF_FILE_SIZE       (4u)           /**< RIFF size field                   */
#define WAV__OFF_FORMAT          (8u)           /**< "WAVE" tag                        */
#define WAV__RIFF_HDR_SIZE       (12u)          /**< size of the RIFF header = offset of the first chunk */
/** \} */

/** \name Generic chunk header (tag + size)
 *  \{ */
#define WAV__CHUNK_HDR_SIZE      (8u)           /**< 4 byte tag + 4 byte size          */
#define WAV__OFF_CHUNK_SIZE      (4u)           /**< offset of the size field          */
/** \} */

/** \name Offsets inside the fmt chunk body
 *  \{ */
#define WAV__FMT_OFF_FORMAT_TAG  (0u)           /**< audio format tag (1 = PCM)        */
#define WAV__FMT_OFF_CHANNELS    (2u)           /**< number of channels                */
#define WAV__FMT_OFF_SAMPLE_RATE (4u)           /**< sample rate in Hz                 */
#define WAV__FMT_OFF_BYTE_RATE   (8u)           /**< bytes per second                  */
#define WAV__FMT_OFF_BLOCK_ALIGN (12u)          /**< bytes per frame                   */
#define WAV__FMT_OFF_BITS        (14u)          /**< bits per sample                   */
#define WAV__FMT_BASE_SIZE       (16u)          /**< size of the plain PCM fmt chunk   */
/** \} */

/** \name WAVE_FORMAT_EXTENSIBLE (N4)
 *  fmt body: 16 base bytes | cbSize(2) validBits(2) channelMask(4) SubFormat GUID(16)
 *  \{ */
#define WAV__FORMAT_EXTENSIBLE   (0xFFFEu)      /**< format tag of an extensible fmt chunk          */
#define WAV__FMT_OFF_SUBFORMAT   (24u)          /**< GUID start = 16 + 2 + 2 + 4; first 2 bytes = real format tag */
#define WAV__FMT_EXT_MIN_SIZE    (26u)          /**< fmt size needed to read the 2 byte sub-format tag */
#define WAV__FMT_MAX_READ        (40u)          /**< largest fmt body that is looked at (16 + 24)   */
/** \} */

/** \name Read window and scan limits (N1)
 *  \{ */
#define WAV__WIN_SIZE            (256u)         /**< bytes fetched per window refill   */
#define WAV__MAX_CHUNKS          (16u)          /**< chunks looked at before giving up */
#define WAV__SEEK_MAX            (0x7FFFFFFFu)  /**< FS_FSeek() takes a signed 32 bit offset */
/** \} */

/** \name Supported PCM formats (INC 5A, 6A, 6D)
 *  \{ */
#define WAV__BITS_8              (8u)           /**< 8 bit unsigned PCM                */
#define WAV__BITS_16             (16u)          /**< 16 bit signed PCM                 */
#define WAV__BITS_24             (24u)          /**< 24 bit signed PCM                 */
#define WAV__CH_MONO             (1u)           /**< one channel                       */
#define WAV__CH_STEREO           (2u)           /**< two channels, mixed to mono by the player */
#define WAV__SAMPLE_RATE         (22050u)       /**< rate of the SOUND_SAMPLE timer; 2x this rate is decimated (N9) */
/** \} */

#define WAV__DEC_DIGITS          (10u)          /**< decimal digits of a 32 bit value  */

/*****************************************************************************/
/* Local variable definitions ('static')                                     */
/*****************************************************************************/

static uint8_t  WAV__win[WAV__WIN_SIZE];    /**< header read window (N1, N8)                     */
static uint32_t WAV__winBase = 0u;          /**< file offset of WAV__win[0]                      */
static uint32_t WAV__winLen  = 0u;          /**< number of valid bytes in WAV__win               */

/*****************************************************************************/
/* Local function prototypes                                                 */
/*****************************************************************************/

static uint32_t        WAV__ReadU32(uint8_t const * const data);
static uint16_t        WAV__ReadU16(uint8_t const * const data);
static void            WAV__BytesToString(uint8_t const * const src,
                                          char          * const dst,
                                          uint32_t        const n);
static boolean_t       WAV__InWindow(uint32_t const offset, uint32_t const n);
static RC_t            WAV__Window(FS_FILE * const pFile,
                                   uint32_t   const offset,
                                   uint32_t   const n,
                                   uint8_t const * * const ppData);
static RC_t            WAV__ValidateFormat(WAVHeader const * const hdr);
static RC_t            WAV__ParseFmt(FS_FILE   * const pFile,
                                     WAVHeader * const hdr,
                                     uint32_t    const body,
                                     uint32_t    const size);
static void            WAV__LogText(char const * const label, char const * const text);
static void            WAV__LogNumber(char     const * const label,
                                      uint32_t         const value,
                                      char     const * const unit);
static RC_t            WAV__ParseChunks(FS_FILE * const pFile, WAVFile * const wav);

/*****************************************************************************/
/* Private helpers                                                           */
/*****************************************************************************/

/**
 * \brief Read a little-endian 32 bit value from a byte buffer.
 * \param uint8_t const *const data    : [IN] pointer to at least 4 bytes, not NULL
 * \return decoded 32 bit value
 */
static uint32_t WAV__ReadU32(uint8_t const * const data)
{
    return  (uint32_t)data[0]
         | ((uint32_t)data[1] << 8u)
         | ((uint32_t)data[2] << 16u)
         | ((uint32_t)data[3] << 24u);
}

/**
 * \brief Read a little-endian 16 bit value from a byte buffer.
 * \param uint8_t const *const data    : [IN] pointer to at least 2 bytes, not NULL
 * \return decoded 16 bit value
 */
static uint16_t WAV__ReadU16(uint8_t const * const data)
{
    /* Casts keep every operand 16 bit: no implicit narrowing from int (MISRA 10.3) */
    return (uint16_t)((uint16_t)data[0] | (uint16_t)((uint16_t)data[1] << 8u));
}

/**
 * \brief Copy n bytes and append a terminating NUL.
 * \param uint8_t const *const src     : [IN]  source bytes
 * \param char *const dst              : [OUT] destination, must hold n + 1 characters (all callers pass char[5] and n = 4)
 * \param uint32_t const n             : [IN]  number of bytes to copy
 */
static void WAV__BytesToString(uint8_t const * const src,
                               char          * const dst,
                               uint32_t        const n)
{
    (void)memcpy(dst, src, n);
    dst[n] = '\0';
}

/**
 * \brief Check whether n bytes at a file offset are inside the read window.
 *
 * The comparison uses subtraction only, so it cannot overflow for offsets
 * close to 4 GB.
 *
 * \param uint32_t const offset        : [IN] file offset of the first byte
 * \param uint32_t const n             : [IN] number of bytes needed
 * \return TRUE when all n bytes are in WAV__win,
 *         FALSE otherwise
 */
static boolean_t WAV__InWindow(uint32_t const offset, uint32_t const n)
{
    boolean_t hit = FALSE;

    if (offset >= WAV__winBase)
    {
        uint32_t const rel = offset - WAV__winBase;

        if ((rel <= WAV__winLen) && (n <= (WAV__winLen - rel)))
        {
            hit = TRUE;
        }
    }
    return hit;
}

/**
 * \brief Return a pointer to n header bytes at a file offset (N1).
 *
 * If the bytes are not in the window the window is reloaded at that offset
 * (one FS_FSeek and one FS_Read of WAV__WIN_SIZE bytes). The returned pointer
 * is only valid until the next call, because a reload overwrites the window.
 *
 * \param FS_FILE *const pFile         : [IN]  open file, not NULL
 * \param uint32_t const offset        : [IN]  file offset of the first byte, 0..WAV__SEEK_MAX
 * \param uint32_t const n             : [IN]  number of bytes needed, at most WAV__WIN_SIZE
 * \param uint8_t const **const ppData : [OUT] pointer to the bytes inside WAV__win, NULL on error, not NULL itself
 * \return RC_SUCCESS when all n bytes are available at *ppData,
 *         RC_ERROR_BAD_PARAM when n is larger than the window or offset is above WAV__SEEK_MAX,
 *         RC_ERROR_READ_FAILS when the seek or read failed or the file ends before offset + n
 */
static RC_t WAV__Window(FS_FILE * const pFile,
                        uint32_t   const offset,
                        uint32_t   const n,
                        uint8_t const * * const ppData)
{
    RC_t rc = RC_ERROR_BAD_PARAM;
 
    *ppData = NULL;
 
    if ((n <= WAV__WIN_SIZE) && (offset <= WAV__SEEK_MAX))
    {
        rc = RC_ERROR_READ_FAILS;
 
        if (WAV__InWindow(offset, n) == FALSE)
        {
            if (FS_FSeek(pFile, (I32)offset, FS_SEEK_SET) == 0)
            {
                WAV__winLen  = FS_Read(pFile, WAV__win, WAV__WIN_SIZE);
                WAV__winBase = offset;
            }
            else
            {
                WAV__winLen  = 0u;
            }
        }
 
        if (WAV__InWindow(offset, n) == TRUE)
        {
            *ppData = &WAV__win[offset - WAV__winBase];
            rc      = RC_SUCCESS;
        }
    }
    return rc;
}

/**
 * \brief Check the decoded fmt fields against what the player can output (N9).
 *
 * Writes one UART line for the first violated constraint.
 *
 * \param WAVHeader const *const hdr   : [IN] header with the fmt fields filled in (extensible already resolved)
 * \return RC_SUCCESS when the format is supported,
 *         RC_ERROR when a constraint is violated (reason on the UART)
 */
static RC_t WAV__ValidateFormat(WAVHeader const * const hdr)
{
    RC_t rc = RC_ERROR;

    if (hdr->audio_format != WAV_AUDIO_FORMAT_PCM)          /* EXTENSIBLE is already resolved to its SubFormat */
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not PCM\n");
    }
    else if ((hdr->number_of_channels != WAV__CH_MONO) &&
             (hdr->number_of_channels != WAV__CH_STEREO))
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not mono/stereo\n");
    }
    else if ((hdr->bits_per_sample != WAV__BITS_8)  &&
             (hdr->bits_per_sample != WAV__BITS_16) &&
             (hdr->bits_per_sample != WAV__BITS_24))
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not 8/16/24-bit\n");
    }
    else if ((uint32_t)hdr->block_align !=
             ((uint32_t)hdr->number_of_channels * ((uint32_t)hdr->bits_per_sample / 8u)))
    {
        /* One frame = one sample for every channel */
        UART_LOG_PutString("\r\n[WAV] ERROR: bad block_align\n");
    }
    else if ((hdr->sample_rate != WAV__SAMPLE_RATE) &&
             (hdr->sample_rate != (2u * WAV__SAMPLE_RATE)))
    {
        /* 22050 Hz is played directly, 44100 Hz is decimated by 2 in MP3__readBlock() */
        WAV__LogNumber("\r\n[WAV] ERROR: ", hdr->sample_rate, " Hz, need 22050/44100\n");
    }
    else
    {
        rc = RC_SUCCESS;
    }
    return rc;
}

/**
 * \brief Decode and validate the fmt chunk.
 *
 * Also resolves WAVE_FORMAT_EXTENSIBLE to the real format tag (N4).
 *
 * \param char const *const label      : [IN] text before the value, NUL terminated
 * \param FS_FILE *const pFile         : [IN]     open file, not NULL
 * \param WAVHeader *const hdr         : [IN/OUT] header whose fmt fields are filled in, not NULL
 * \param uint32_t const body          : [IN]     file offset of the fmt chunk body
 * \param uint32_t const size          : [IN]     size of the fmt chunk body as declared in the file (min. 16)
 * \return RC_SUCCESS when the chunk was decoded and the format is supported,
 *         RC_ERROR_INVALID when the chunk is too small (or WAV__ValidateFormat() says so),
 *         RC_ERROR_READ_FAILS when the chunk cannot be read,
 *         RC_ERROR_NOT_IMPLEMENTED when the format is not supported (reason on the UART)
 */
static RC_t WAV__ParseFmt(FS_FILE   * const pFile,
                          WAVHeader * const hdr,
                          uint32_t    const body,
                          uint32_t    const size)
{
    RC_t            rc   = RC_ERROR_INVALID;
    uint32_t const  need = (size > WAV__FMT_MAX_READ) ? WAV__FMT_MAX_READ : size;
    uint8_t const * p    = NULL;
 
    if (size < WAV__FMT_BASE_SIZE)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: fmt chunk too small\n");
    }
    else
    {
        rc = WAV__Window(pFile, body, need, &p);
 
        if (rc != RC_SUCCESS)
        {
            UART_LOG_PutString("\r\n[WAV] ERROR: fmt read failed\n");
        }
        else
        {
            hdr->subchunk_size      = size;
            hdr->audio_format       = WAV__ReadU16(&p[WAV__FMT_OFF_FORMAT_TAG]);
            hdr->number_of_channels = WAV__ReadU16(&p[WAV__FMT_OFF_CHANNELS]);
            hdr->sample_rate        = WAV__ReadU32(&p[WAV__FMT_OFF_SAMPLE_RATE]);
            hdr->byte_rate          = WAV__ReadU32(&p[WAV__FMT_OFF_BYTE_RATE]);
            hdr->block_align        = WAV__ReadU16(&p[WAV__FMT_OFF_BLOCK_ALIGN]);
            hdr->bits_per_sample    = WAV__ReadU16(&p[WAV__FMT_OFF_BITS]);
 
            /* N4: the real format tag of an extensible file is in the SubFormat GUID */
            if ((hdr->audio_format == WAV__FORMAT_EXTENSIBLE) &&
                (need >= WAV__FMT_EXT_MIN_SIZE))
            {
                hdr->audio_format = WAV__ReadU16(&p[WAV__FMT_OFF_SUBFORMAT]);
            }
 
            rc = WAV__ValidateFormat(hdr);
        }
    }
    return rc;
}

/**
 * \brief Write "label text\n" to the UART log.
 * \param char const *const label      : [IN] text before the value, NUL terminated
 * \param char const *const text       : [IN] NUL terminated value, followed by a line feed
 */
static void WAV__LogText(char const * const label, char const * const text)
{
    UART_LOG_PutString(label);
    UART_LOG_PutString(text);
    UART_LOG_PutString("\n");
}

/**
 * \brief Write "label<decimal value>unit" to the UART log without stdio (N7).
 * \param char const *const label      : [IN] text before the number, NUL terminated
 * \param uint32_t const value         : [IN] number to print in decimal, 0..4294967295
 * \param char const *const unit       : [IN] text after the number (may contain the line feed), NUL terminated
 */
static void WAV__LogNumber(char     const * const label,
                           uint32_t         const value,
                           char     const * const unit)
{
    char     digits[WAV__DEC_DIGITS + 1u];
    uint32_t rest = value;
    uint8_t  pos  = (uint8_t)WAV__DEC_DIGITS;

    digits[pos] = '\0';
    do
    {
        pos--;
        digits[pos] = (char)((uint32_t)'0' + (rest % 10u));
        rest       /= 10u;
    } while (rest != 0u);

    UART_LOG_PutString(label);
    UART_LOG_PutString(&digits[pos]);
    UART_LOG_PutString(unit);
}

/*****************************************************************************/
/* Public API                                                                */
/*****************************************************************************/

/**
 * \brief Walk the RIFF chunk list and fill the WAVFile (body of WAV_ParseHeader).
 *
 * Reads the RIFF header, walks the chunk list (at most WAV__MAX_CHUNKS chunks),
 * decodes and validates the 'fmt ' chunk (N4, N9), finds the 'data' chunk and
 * leaves the file position on its first PCM byte (N6). Header bytes come through
 * one read window (N1). The return codes are described in N10.
 *
 * \param FS_FILE *const pFile         : [IN]  open emFile handle, not NULL (checked by WAV_ParseHeader())
 * \param WAVFile *const wav           : [OUT] structure to fill, not NULL (checked by WAV_ParseHeader())
 * \return RC_SUCCESS, RC_ERROR_READ_FAILS, RC_ERROR_INVALID or RC_ERROR_NOT_IMPLEMENTED,
 *         see WAV_ParseHeader()
 */
static RC_t WAV__ParseChunks(FS_FILE * const pFile, WAVFile * const wav)
{
    RC_t              rc       = RC_SUCCESS;                /* status of the walk, first error stops it */
    WAVHeader * const hdr      = &wav->header;
    uint32_t const    fileSize = FS_GetFileSize(pFile);
    uint8_t const *   p        = NULL;
    uint32_t          offset   = WAV__RIFF_HDR_SIZE;    /* file offset of the next chunk header */
    uint8_t           chunks   = 0u;
    boolean_t         gotFmt   = FALSE;                 /* fmt chunk decoded and valid          */
    boolean_t         done     = FALSE;                 /* data chunk found, file positioned    */
 
    WAV__winBase = 0u;                                  /* drop whatever a previous file left in the window */
    WAV__winLen  = 0u;
 
    /* ---- RIFF header: one window read covers header, fmt and usually the data chunk header (N1) ---- */
    rc = WAV__Window(pFile, 0u, WAV__RIFF_HDR_SIZE, &p);
 
    if (rc != RC_SUCCESS)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: header read failed\n");
    }
    else if (WAV__ReadU32(&p[WAV__OFF_FILE_ID]) != WAV__TAG_RIFF)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not RIFF\n");
        rc = RC_ERROR_INVALID;
    }
    else if (WAV__ReadU32(&p[WAV__OFF_FORMAT]) != WAV__TAG_WAVE)
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: not WAVE\n");
        rc = RC_ERROR_INVALID;
    }
    else
    {
        WAV__BytesToString(&p[WAV__OFF_FILE_ID], hdr->file_id, 4u);
        hdr->file_size = WAV__ReadU32(&p[WAV__OFF_FILE_SIZE]);
        WAV__BytesToString(&p[WAV__OFF_FORMAT],  hdr->format,  4u);
    }
 
    /* ---- Chunk walker: fmt -> (anything) -> data. The offset test also stops the loop at the end of the file ---- */
    while ((rc     == RC_SUCCESS) &&
           (done   == FALSE) &&
           (chunks <  WAV__MAX_CHUNKS) &&
           (fileSize >= WAV__CHUNK_HDR_SIZE) &&
           (offset <= (fileSize - WAV__CHUNK_HDR_SIZE)))
    {
        rc = WAV__Window(pFile, offset, WAV__CHUNK_HDR_SIZE, &p);
 
        if (rc != RC_SUCCESS)
        {
            UART_LOG_PutString("\r\n[WAV] ERROR: chunk read failed\n");
        }
        else
        {
            uint32_t const tag   = WAV__ReadU32(&p[0]);
            uint32_t const size  = WAV__ReadU32(&p[WAV__OFF_CHUNK_SIZE]);
            uint32_t const body  = offset + WAV__CHUNK_HDR_SIZE;    /* cannot overflow: offset + 8 <= fileSize */
            uint32_t const avail = fileSize - body;                 /* bytes behind this chunk header          */
 
            if (tag == WAV__TAG_FMT)
            {
                WAV__BytesToString(p, hdr->subchunk_id, 4u);        /* p is still valid, no window reload yet  */
 
                rc = WAV__ParseFmt(pFile, hdr, body, size);         /* on error the reason is already on the UART */
 
                if (rc == RC_SUCCESS)
                {
                    gotFmt = TRUE;
                }
            }
            else if (tag == WAV__TAG_DATA)
            {
                if (gotFmt == FALSE)
                {
                    UART_LOG_PutString("\r\n[WAV] ERROR: data before fmt\n");
                    rc = RC_ERROR_INVALID;
                }
                else
                {
                    WAV__BytesToString(p, hdr->data_id, 4u);
                    hdr->data_size   = size;
                    wav->data_length = (size > avail) ? avail : size;   /* N5: never promise more than the file holds */
 
                    /* N6: the window read left the file position behind the header - put it on the first PCM byte */
                    if ((body <= WAV__SEEK_MAX) &&
                        (FS_FSeek(pFile, (I32)body, FS_SEEK_SET) == 0))
                    {
                        done = TRUE;
                    }
                    else
                    {
                        UART_LOG_PutString("\r\n[WAV] ERROR: seek to data failed\n");
                        rc = RC_ERROR_READ_FAILS;
                    }
                }
            }
            else
            {
                /* LIST, INFO, JUNK, ... : not needed, skipped by the offset update below */
            }
 
            /* Next chunk header. A chunk that claims more than the file holds ends the walk. */
            if (size > avail)
            {
                offset = fileSize;
            }
            else
            {
                offset = body + size;
                if (((size & 1u) != 0u) && (offset < fileSize))     /* N3: odd chunks are followed by a pad byte */
                {
                    offset++;
                }
            }
            chunks++;
        }
    }
 
    if ((rc == RC_SUCCESS) && (done == FALSE))
    {
        UART_LOG_PutString("\r\n[WAV] ERROR: data chunk not found\n");
        rc = RC_ERROR_INVALID;
    }
    /* else: RC_SUCCESS with done == TRUE = file position on the first PCM sample, or the error was reported where it happened */
    return rc;
}
 
/**
 * \brief Parse the header of an open emFile WAV file.
 *
 * Checks the pointers, then lets WAV__ParseChunks() do the work. After a successful
 * call the file position is on the first PCM byte (N6). The function seeks to
 * offset 0 itself. Header bytes come through one read window (N1).
 *
 * Not re-entrant (static read window), call from tsk_reader only (N8).
 *
 * \param FS_FILE *const pFile         : [IN]  open emFile handle (mode "r"), not NULL
 * \param WAVFile *const wav           : [OUT] structure to fill, not NULL; header fields and data_length
 *                                     (limited to the bytes left in the file, N5) are valid on RC_SUCCESS
 * \return RC_SUCCESS when the header is valid, FS_FTell() gives the offset of the
 *         first PCM byte,
 *         RC_ERROR_NULL when pFile or wav is NULL (nothing is read, no UART output),
 *         RC_ERROR_READ_FAILS when a seek or read failed (reason on the UART),
 *         RC_ERROR_INVALID when the file is not RIFF/WAVE, the fmt chunk is too small,
 *         the header fields contradict each other (block_align), data comes before
 *         fmt or there is no data chunk (reason on the UART),
 *         RC_ERROR_NOT_IMPLEMENTED when the file is valid but not playable: not PCM,
 *         not 1/2 channels, not 8/16/24 bit or not 22050/44100 Hz (reason on the UART)
 */
RC_t WAV_ParseHeader(FS_FILE * const pFile, WAVFile * const wav)
{
    RC_t rc = RC_ERROR_NULL;
 
    if ((pFile != NULL) && (wav != NULL))
    {
        rc = WAV__ParseChunks(pFile, wav);
    }
    return rc;
}

/**
 * \brief Print all WAV header fields to the UART log (debug aid).
 *
 * Prints file id, format, fmt size, audio format, channels, sample rate, bits per
 * sample, block align, data size and the duration (data_length / byte rate, not
 * printed if the byte rate is 0). Uses WAV__LogText() and WAV__LogNumber(), no
 * stdio (N7).
 *
 * \param WAVFile const *const wav     : [IN] parsed WAV file (result of WAV_ParseHeader())
 * \return RC_SUCCESS when the header was printed,
 *         RC_ERROR_NULL when wav is NULL (nothing is printed)
 */
RC_t WAV_PrintHeader(WAVFile const * const wav)
{
    RC_t rc = RC_ERROR_NULL;
 
    if (wav != NULL)
    {
        WAVHeader const * const hdr         = &wav->header;
        uint32_t const          bytesPerSec = hdr->sample_rate * (uint32_t)hdr->block_align;
 
        UART_LOG_PutString("\r\n\n--- WAV Header ---\n");
        WAV__LogText  ("\r\n  FileID        : ", hdr->file_id);
        WAV__LogText  ("\r\n  Format        : ", hdr->format);
        WAV__LogNumber("\r\n  FmtSize       : ", hdr->subchunk_size,                "\n");
        WAV__LogNumber("\r\n  AudioFormat   : ", (uint32_t)hdr->audio_format,       "\n");
        WAV__LogNumber("\r\n  Channels      : ", (uint32_t)hdr->number_of_channels, "\n");
        WAV__LogNumber("\r\n  SampleRate    : ", hdr->sample_rate,                  " Hz\n");
        WAV__LogNumber("\r\n  BitsPerSample : ", (uint32_t)hdr->bits_per_sample,    "\n");
        WAV__LogNumber("\r\n  BlockAlign    : ", (uint32_t)hdr->block_align,        "\n");
        WAV__LogNumber("\r\n  DataSize      : ", hdr->data_size,                    " bytes\n");
 
        if (bytesPerSec != 0u)                                      /* PrintHeader is public: never divide by zero */
        {
            WAV__LogNumber("\r\n  Duration      : ~", wav->data_length / bytesPerSec, " s\n");
        }
        UART_LOG_PutString("\r\n------------------\n");
        rc = RC_SUCCESS;
    }
    return rc;
}
 
/*----------------------------------------------------------------------------------------
* Notes:
* N1 - One window read instead of one read per field. The emFile configuration has no sector
*      cache (FS_SUPPORT_CACHE 0) and no file buffer (FS_USE_FILE_BUFFER 0), so every FS_Read()
*      goes to the SD card and transfers at least one 512 byte sector, even for 8 bytes.
*      The old parser needed 5 reads for the usual Audacity file (36 byte header, LIST chunk
*      header, 16 + 10 byte skip, data chunk header). Now the first 256 bytes are read once and
*      all chunk headers are decoded from RAM. The window is only reloaded (FS_FSeek + FS_Read)
*      when a chunk header lies outside it, for example behind a LIST chunk larger than 256 bytes.
*      FS_FSeek() only moves the file position, it does not read a data sector.
*
* N2 - Tags are compared as 32 bit integers instead of strings. The bytes 'R' 'I' 'F' 'F' =
*      0x52 0x49 0x46 0x46 read as little-endian are 0x46464952. This avoids strcmp() on buffers
*      that are not guaranteed to be NUL terminated and is a single compare instruction.
*
* N3 - RIFF chunks are word aligned: a chunk with an odd size is followed by one pad byte that
*      is not counted in the size field. The old parser ignored it and then read the next chunk
*      header one byte off, so files with an odd-sized LIST/INFO chunk were rejected.
*
* N4 - WAVE_FORMAT_EXTENSIBLE (format tag 0xFFFE) has a 40 byte fmt chunk:
*      16 base bytes | cbSize(2) validBits(2) channelMask(4) | SubFormat GUID(16).
*      The first 2 bytes of the GUID hold the real format tag (1 = PCM) and start at body offset
*      16 + 2 + 2 + 4 = 24. Only 40 bytes of the fmt chunk are looked at, a larger chunk is
*      simply skipped.
*
* N5 - data_length = min(declared data size, bytes left in the file). Recorders that stream to
*      disk write 0xFFFFFFFF (or a too large value) into the data size field, and cut or
*      damaged files are shorter than declared. Without the limit the player would report a
*      wrong play time and only stop on an SD read error. header.data_size keeps the declared value.
*
* N6 - mp3.c aligns its first PCM read to a 512 byte sector boundary with FS_FTell(). That needs
*      the file position to be exactly on the first PCM byte when this function returns, so the
*      parser seeks there explicitly after the window read moved the position behind the header.
*
* N7 - No <stdio.h>: sprintf() violates MISRA C:2012 Rule 21.6, needs a lot of stack and drags
*      in the printf family. WAV__LogNumber() prints unsigned decimals with a small digit buffer
*      (max 10 digits for 32 bit) and the log text is written with UART_LOG_PutString() only.
*
* N8 - The read window is a static buffer (256 bytes RAM) instead of a stack array, so the
*      tsk_reader stack stays small. The price: WAV_ParseHeader() is not re-entrant. It is only
*      called from tsk_reader (MP3__openWavFile), so no lock is needed.
*
* N9 - Why these formats: the player outputs one 8 bit sample per SOUND_SAMPLE tick (22050 Hz,
*      VDAC8). 22050 Hz files map 1:1, 44100 Hz files are decimated by 2 in MP3__readBlock().
*      Higher bit depths use the MSB only, stereo is averaged to mono. The block_align check
*      (channels * bits / 8) rejects files whose header contradicts itself, because the
*      conversion loop in mp3.c computes its read size from block_align.
*
* N10 - Return codes (INC 11A.1). Every function that can fail returns RC_t. WAV_ParseHeader()
*      and the private WAV__ParseChunks() / WAV__ParseFmt() / WAV__ValidateFormat() report the
*      reason with the project codes, the UART text stays as it was:
*        RC_ERROR_NULL            pFile or wav is NULL (new check, before: crash)
*        RC_ERROR_READ_FAILS      FS_FSeek() / FS_Read() failed or the file ends too early
*        RC_ERROR_INVALID         not RIFF/WAVE, fmt chunk too small, block_align contradicts
*                                 channels * bits, data before fmt, no data chunk
*        RC_ERROR_NOT_IMPLEMENTED valid WAV, but not playable: not PCM, not 1/2 channels,
*                                 not 8/16/24 bit, not 22050/44100 Hz
*      WAV__Window() returns RC_t now (RC_SUCCESS, RC_ERROR_BAD_PARAM for n / offset out of
*      range, RC_ERROR_READ_FAILS) and gives the pointer through an out parameter. WAV_PrintHeader()
*      returns RC_SUCCESS or RC_ERROR_NULL. No RC_t for: WAV__ReadU32/U16 (decode a value, cannot
*      fail), WAV__BytesToString (cannot fail, the callers copy 4 bytes), WAV__InWindow (a yes/no
*      question, boolean_t), WAV__LogText/LogNumber (UART_LOG_PutString() reports nothing).
*      Callers: mp3.c only tests "!= RC_SUCCESS", all codes above are different from 0, so nothing
*      changes there. A caller of WAV_PrintHeader() must cast the result to void (MISRA 17.7).
*
* Time (estimated, not yet measured on the target):
*      One sector read = about 4.5 ms (measured: 2 sectors = 9.2 ms in MP3__readBlock, SCLK 4 MHz).
*      Header parse, typical file with a 26 byte LIST chunk:
*          old: 5 reads = about 23 ms        new: 1 read = about 4.6 ms
*      Header parse, plain 44 byte header:
*          old: 2 reads = about 9 ms         new: 1 read = about 4.6 ms
*      This is song start-up time only. The streaming deadline in MP3__readBlock() is not affected.
*      Verify with the DBG_FS pin around MP3__openWavFile().
*/