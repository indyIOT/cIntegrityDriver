/*********************************************************************
 * @file cCRC16.c
 * @brief Source file for the CRC16 implementation, both of which were taken
 * from different reddit posts or Claude code examples.
 * The code was modified to fit the needs of this project and to be more readable.
 * @author Anthony Garza
 * @copyright All rights reserved 2026
 *********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "commonTypes.h"
#include "commonMacros.h"
#include "cIntegrityDriverConfig.h"
#include "cIntegrityDriverPub.h"
#include "cIntegrityDriver.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************* Static Global Variables **********************/
static const uint8_t moduleName[] = "cCRC16";
#define MODULE_ID 56525



/*
 * crc16_ccitt.c - fast table-driven CRC16-CCITT for embedded
 * Poly 0x1021, init 0xFFFF, no reflection, no final XOR (CCITT-FALSE).
 * check("123456789") = 0x29B1
 * Table cost: 512 bytes flash. One lookup + shift + xor per byte.
 */
static const uint16_t crc16_tab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
};


/********************************Static Function Prototypes  ******************/

static uint8_t reflect8( uint8_t b );
static uint16_t reflect16( uint16_t v );

/**
 * @brief Reflects the bits of an 8-bit value.
 * @param b The 8-bit value to reflect.
 * @return The reflected 8-bit value.
 */
static uint8_t reflect8( uint8_t b )
{
    b = ( uint8_t )( ( b & 0xF0u ) >> 4 | ( b & 0x0Fu ) << 4 );
    b = ( uint8_t )( ( b & 0xCCu ) >> 2 | ( b & 0x33u ) << 2 );
    b = ( uint8_t )( ( b & 0xAAu ) >> 1 | ( b & 0x55u ) << 1 );
    return ( b );
}

/**
 * @brief Reflects the bits of a 16-bit value.
 * @param v The 16-bit value to reflect.
 * @return The reflected 16-bit value.
 */
static uint16_t reflect16( uint16_t v )
{
    uint16_t r = 0;
    for ( int i = 0; i < 16; i++ )
     {
        r = ( uint16_t )( ( r << 1 ) | ( v & 1u ) );
        v >>= 1;
    }
    return ( r );
}

/**
 * @brief Calculates the CRC16 of a given buffer using the specified 
 *        configuration.
 * @note passing in a null pointer for buffer should be allowed because
 *  some crc calculations are could be calculated over the first address in
 * 
 * @param config Pointer to the CRC16 configuration.
 * @param buffer Pointer to the buffer to calculate the CRC16 of.
 * @param len The length of the buffer in bytes.
 * @param crc16Value Pointer to the location where the calculated CRC16 value will be stored.
 * @return The error code indicating the success or failure of the operation.
 */
sErrorCompact_t calculateCRC16( sCRC16Config_t const * const config, 
                         void const * const buffer, size_t length, 
                         uint16_t * const crc16Value )
{
    uint8_t const * p = ( uint8_t const *)buffer;
    uint16_t crc = 0;
    uint8_t byte = 0;
    sErrorCompact_t retValue = BLANK_ERROR_STRUCT;
    if( ( config == NULL )  || ( crc16Value == NULL ) )
    {
        retValue = CREATE_ERROR( ERROR_NULL_POINTER, NULL );
    }
    else
    {
        crc = config->init;
#if ( EMBEDDED_OPTIMIZED_BUILD != DEF_TRUE )
        while ( length-- ) 
        {
            byte = config->reflectIn ? reflect8( *p++ ) : *p++;
            crc ^= ( uint16_t )byte << 8;
            for ( int i = 0; i < 8; i++ )
            {
                crc = ( crc & 0x8000u ) ? ( uint16_t )( ( crc << 1 ) ^ config->poly )
                                    : ( uint16_t )( crc << 1 );
            }
        }

        if ( config->reflectOut )
        {
            crc = reflect16( crc );
        }
    
        crc ^= config->xorOut;
#else
    crc = CRC16TableCCITT_False( buffer, length, CRC_16_SEED );
#endif
        *crc16Value = crc;
    }
    return ( retValue );
}

/**
 * @brief Calculates the CRC16 of a given buffer using the CCITT-FALSE polynomial.
 * @param buffer Pointer to the buffer to calculate the CRC16 of.
 * @param length The length of the buffer in bytes.
 * @return The calculated CRC16 value. 
 */
uint16_t CRC16TableCCITT_False( void const * const buffer, 
                                size_t length, 
                                uint16_t initialCrc )
{
    uint8_t const * p = ( uint8_t const * )buffer;
    uint16_t crc = initialCrc;

    while (length--)
    {
        crc = (uint16_t)((crc << 8) ^ crc16_tab[(crc >> 8) ^ *p++]);
    }

    return crc;
}



#ifdef __cplusplus
}  /* extern "C" */
#endif
