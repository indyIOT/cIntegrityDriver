/*********************************************************************
 * @file cSHA256.c
 * @brief Source file for the SHA-256 implementation (FIPS 180-4). Exposes a
 *        single one-shot public function, calculateSHA256(), backed by the
 *        standard init/update/pad/finalize steps kept private to this file.
 * @author Anthony Garza
 * @date 2026-09-14
 * @details This implementation is designed to work with the Raspberry Pi Pico
 * and the C programming language.
 * @copyright All rights reserved 2026
 *********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "commonTypes.h"
#include "cIntegrityDriverConfig.h"
#include "cIntegrityDriverPub.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * CONSTANTS AND DEFINITIONS
 *============================================================================*/
static const uint8_t moduleName[] = "cSHA256";
#define MODULE_ID 56527

#define SHA_256_BLOCK_LENGTH_BYTES                                             64
#define SHA_256_STATE_WORD_COUNT                                                8
#define SHA_256_ROUND_COUNT                                                    64
#define SHA_256_MESSAGE_SCHEDULE_FIRST_WORDS                                   16
#define SHA_256_LENGTH_FIELD_LENGTH_BYTES                                       8
/* Below this many buffered bytes, the 0x80 pad byte + zero pad + 8-byte
 * length field can all fit in the current 64-byte block. At or above it,
 * padding spills into a second block. */
#define SHA_256_PAD_SPILL_THRESHOLD_BYTES ( SHA_256_BLOCK_LENGTH_BYTES - SHA_256_LENGTH_FIELD_LENGTH_BYTES )

/*============================================================================
 * TYPES AND STRUCTURES
 *============================================================================*/

/**
 * @brief Running state for a SHA-256 calculation. Kept private to this file --
 *        callers only ever see the one-shot calculateSHA256() entry point.
 */
typedef struct
{
    uint32_t state[SHA_256_STATE_WORD_COUNT];
    uint64_t bitLength;
    uint8_t buffer[SHA_256_BLOCK_LENGTH_BYTES];
    size_t bufferLength;
} sSHA256Context_t;

/*============================================================================
 * STATIC VARIABLES
 *============================================================================*/

/* FIPS 180-4 section 4.2.2 round constants: the fractional parts of the cube
 * roots of the first 64 prime numbers. */
static const uint32_t sha256RoundConstants[SHA_256_ROUND_COUNT] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* FIPS 180-4 section 5.3.3 initial hash values: the fractional parts of the
 * square roots of the first 8 prime numbers. */
static const uint32_t sha256InitialState[SHA_256_STATE_WORD_COUNT] =
{
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

/*============================================================================
 * STATIC FUNCTION PROTOTYPES
 *============================================================================*/
static uint32_t rotateRight32( uint32_t value, uint32_t bitCount );
static void sha256Init( sSHA256Context_t * const context );
static void sha256Transform( sSHA256Context_t * const context, uint8_t const * const block );
static void sha256Update( sSHA256Context_t * const context, uint8_t const * data, size_t length );
static void sha256Final( sSHA256Context_t * const context, uint8_t digest[SHA_256_DIGEST_LENGTH_BYTES] );

/*============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Calculates the SHA-256 digest of a given buffer.
 * @param buffer Pointer to the buffer to calculate the SHA-256 digest of.
 * @param length The length of the buffer in bytes.
 * @return sSHA256Digest_t structure containing the 32-byte SHA-256 digest.
 */
sSHA256Digest_t calculateSHA256( void const * const buffer, size_t length )
{
    sSHA256Digest_t retValue = { 0 };
    sSHA256Context_t context = { 0 };

    sha256Init( &context );
    sha256Update( &context, ( uint8_t const * )buffer, length );
    sha256Final( &context, retValue.digest );

    return ( retValue );
}

/*============================================================================
 * STATIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Rotates a 32-bit value right by the given number of bits.
 * @param value The value to rotate.
 * @param bitCount The number of bits to rotate by.
 * @return The rotated value.
 */
static uint32_t rotateRight32( uint32_t value, uint32_t bitCount )
{
    return ( ( value >> bitCount ) | ( value << ( 32 - bitCount ) ) );
}

/**
 * @brief Initializes a SHA-256 context to the FIPS 180-4 initial state.
 * @param context Pointer to the context to initialize.
 */
static void sha256Init( sSHA256Context_t * const context )
{
    memcpy( context->state, sha256InitialState, sizeof( sha256InitialState ) );
    context->bitLength = 0;
    context->bufferLength = 0;
}

/**
 * @brief Processes a single 64-byte block, updating the running hash state.
 * @param context Pointer to the context whose state should be updated.
 * @param block Pointer to the 64-byte block to process.
 */
static void sha256Transform( sSHA256Context_t * const context, uint8_t const * const block )
{
    uint32_t messageSchedule[SHA_256_ROUND_COUNT];
    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];
    uint32_t e = context->state[4];
    uint32_t f = context->state[5];
    uint32_t g = context->state[6];
    uint32_t h = context->state[7];
    int i = 0;

    for ( i = 0; i < SHA_256_MESSAGE_SCHEDULE_FIRST_WORDS; i++ )
    {
        messageSchedule[i] = ( ( uint32_t )block[i * 4] << 24 ) |
                              ( ( uint32_t )block[i * 4 + 1] << 16 ) |
                              ( ( uint32_t )block[i * 4 + 2] << 8 ) |
                              ( ( uint32_t )block[i * 4 + 3] );
    }

    for ( i = SHA_256_MESSAGE_SCHEDULE_FIRST_WORDS; i < SHA_256_ROUND_COUNT; i++ )
    {
        uint32_t sigma0 = rotateRight32( messageSchedule[i - 15], 7 ) ^
                           rotateRight32( messageSchedule[i - 15], 18 ) ^
                           ( messageSchedule[i - 15] >> 3 );
        uint32_t sigma1 = rotateRight32( messageSchedule[i - 2], 17 ) ^
                           rotateRight32( messageSchedule[i - 2], 19 ) ^
                           ( messageSchedule[i - 2] >> 10 );
        messageSchedule[i] = messageSchedule[i - 16] + sigma0 + messageSchedule[i - 7] + sigma1;
    }

    for ( i = 0; i < SHA_256_ROUND_COUNT; i++ )
    {
        uint32_t bigSigma1 = rotateRight32( e, 6 ) ^ rotateRight32( e, 11 ) ^ rotateRight32( e, 25 );
        uint32_t choice = ( e & f ) ^ ( ( ~e ) & g );
        uint32_t temp1 = h + bigSigma1 + choice + sha256RoundConstants[i] + messageSchedule[i];
        uint32_t bigSigma0 = rotateRight32( a, 2 ) ^ rotateRight32( a, 13 ) ^ rotateRight32( a, 22 );
        uint32_t majority = ( a & b ) ^ ( a & c ) ^ ( b & c );
        uint32_t temp2 = bigSigma0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

/**
 * @brief Feeds data into a SHA-256 context, transforming any complete
 *        64-byte blocks that accumulate and buffering the remainder.
 * @param context Pointer to the context to update.
 * @param data Pointer to the data to add to the digest.
 * @param length The number of bytes pointed to by data.
 */
static void sha256Update( sSHA256Context_t * const context, uint8_t const * data, size_t length )
{
    context->bitLength += ( uint64_t )length * 8u;

    while ( length > 0u )
    {
        size_t spaceInBuffer = SHA_256_BLOCK_LENGTH_BYTES - context->bufferLength;
        size_t chunkLength = ( spaceInBuffer < length ) ? spaceInBuffer : length;

        memcpy( context->buffer + context->bufferLength, data, chunkLength );
        context->bufferLength += chunkLength;
        data += chunkLength;
        length -= chunkLength;

        if ( context->bufferLength == SHA_256_BLOCK_LENGTH_BYTES )
        {
            sha256Transform( context, context->buffer );
            context->bufferLength = 0;
        }
    }
}

/**
 * @brief Applies FIPS 180-4 padding (a single 0x80 bit, zero bits, then the
 *        64-bit big-endian message length) and produces the final digest.
 * @param context Pointer to the context to finalize. Its buffered/partial
 *                state is consumed and is not valid to use afterward.
 * @param digest Buffer that receives the SHA_256_DIGEST_LENGTH_BYTES digest.
 */
static void sha256Final( sSHA256Context_t * const context, uint8_t digest[SHA_256_DIGEST_LENGTH_BYTES] )
{
    uint64_t const originalBitLength = context->bitLength;
    uint8_t lengthField[SHA_256_LENGTH_FIELD_LENGTH_BYTES] = { 0 };
    uint8_t padByte = 0x80u;
    int i = 0;

    sha256Update( context, &padByte, 1u );

    if ( context->bufferLength > SHA_256_PAD_SPILL_THRESHOLD_BYTES )
    {
        uint8_t zeroPad[SHA_256_BLOCK_LENGTH_BYTES] = { 0 };
        size_t remainingInBlock = SHA_256_BLOCK_LENGTH_BYTES - context->bufferLength;
        sha256Update( context, zeroPad, remainingInBlock );
    }

    {
        uint8_t zeroPad[SHA_256_BLOCK_LENGTH_BYTES] = { 0 };
        size_t remainingInBlock = SHA_256_PAD_SPILL_THRESHOLD_BYTES - context->bufferLength;
        sha256Update( context, zeroPad, remainingInBlock );
    }

    for ( i = 0; i < SHA_256_LENGTH_FIELD_LENGTH_BYTES; i++ )
    {
        lengthField[i] = ( uint8_t )( originalBitLength >> ( 56 - ( 8 * i ) ) );
    }
    memcpy( context->buffer + context->bufferLength, lengthField, sizeof( lengthField ) );
    context->bufferLength += sizeof( lengthField );
    sha256Transform( context, context->buffer );

    for ( i = 0; i < SHA_256_STATE_WORD_COUNT; i++ )
    {
        digest[i * 4] = ( uint8_t )( context->state[i] >> 24 );
        digest[i * 4 + 1] = ( uint8_t )( context->state[i] >> 16 );
        digest[i * 4 + 2] = ( uint8_t )( context->state[i] >> 8 );
        digest[i * 4 + 3] = ( uint8_t )( context->state[i] );
    }
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

