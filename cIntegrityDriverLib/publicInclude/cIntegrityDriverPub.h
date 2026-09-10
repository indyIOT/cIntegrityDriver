/*********************************************************************
 * @file cIntegrityDriverPub.h
 * @brief Header file for the integrity driver
 * @author Anthony Garza
 * @copyright All rights reserved 2026
 *********************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "commonTypes.h"
#include "commonMacros.h"

#ifndef C_INTEGRITY_DRIVER_PUB_H
#define C_INTEGRITY_DRIVER_PUB_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef SCOMMON_CRC_16
#define SCOMMON_CRC_16
/**
 * @brief Structure that contains the configuration for CRC16 calculation.
 */
typedef struct
{
    uint16_t poly;
    uint16_t init;
    uint16_t xorOut;
    bool reflectIn;
    bool reflectOut;
} sCRC16Config_t;
/*
 *   CCITT-FALSE: 0x1021, 0xFFFF, 0x0000, false, false -> 0x29B1
 *   KERMIT:      0x1021, 0x0000, 0x0000, true,  true  -> 0x2189
 *   X-25:        0x1021, 0xFFFF, 0xFFFF, true,  true  -> 0x906E
 *   MODBUS:      0x8005, 0xFFFF, 0x0000, true,  true  -> 0x4B37
 *   XMODEM:      0x1021, 0x0000, 0x0000, false, false -> 0x31C3
 */
typedef enum
{
    CRC_16_MODBUS_POLY = 0x8005,
    CRC_16_XMODEM_POLY = 0x1021
} eCRCPolynomial_t;
#define CRC_16_SEED                                                       0xFFFF
#define CRC_16_XOROUT                                                    0x0000
#define CRC_16_INPUT_REFLECTED                                              FALSE
#define CRC_16_OUTPUT_REFLECTED                                             FALSE

#define DEFAULT_CRC16_CONFIG { .poly = CRC_16_MODBUS_POLY, \
                             .init = CRC_16_SEED, \
                             .xorOut = CRC_16_XOROUT, \
                             .reflectIn = CRC_16_INPUT_REFLECTED, \
                             .reflectOut = CRC_16_OUTPUT_REFLECTED }
#endif // SCOMMON_CRC_16

/********************************* Integrity Driver Public Interface ********/
/**
 * @brief Function to initialize the integrity driver. This should be called 
 *        before any other functions are used.
 * @param createErrorCallback Pointer to a function for creating errors for the integrity driver.
 * @param logCallback Pointer to a function for logging messages for the integrity driver.
 * @return sErrorCompact_t structure containing the integrity information if an error occurred.
 */
extern sErrorCompact_t initIntegrityDriver( createErrorCallback_t createErrorCallback,
                                            logCallback_t logCallback );


/**
 * @brief Function to calculate the CRC16 of a given buffer using the specified configuration.
 * @note if Embedded Optimized is set in the configuration then this 
 * function will always just call the embedded function with the CCITT-FALSE values.
 * @param config The CRC16 configuration to use.
 * @param buffer Pointer to the buffer to calculate the CRC16 of.
 * @param length The length of the buffer in bytes.
 * @return The calculated CRC16 value.
 */
extern uint16_t calculateCRC16( sCRC16Config_t const * const config,
                                void const * const buffer,
                                size_t length );
/**
 * @brief Function to calculate the CRC16 of a given buffer using an embedded optimized algorithm.
 * @note if Embedded Optimized is set in the configuration then this function will always be used.
 * @note Initial CRC value is set to 0xFFFF unless this is an update of an existing
 * CRC calculation, in which case the initial CRC value should be the previous CRC value.
 * @param initialCrc The initial CRC value to use for the calculation.
 * @param buffer Pointer to the buffer to calculate the CRC16 of.
 * @param length The length of the buffer in bytes.
 * @return The calculated CRC16 value.
 */
extern uint16_t CRC16TableCCITT_False( void const * const buffer,
                                        size_t const length,
                                        uint16_t const initialCrc );

/**
 * @brief Function to get the Integrity driver information. 
 *        This will return a structure containing accessors to
 *       get the module ID, version string, and other information 
 *  about the integrity driver.
 * @return sCommonDriverAccessorStruct_t Structure containing accessors to 
 *             get the module ID, version string, and other information about the integrity driver.
 */
extern sCommonDriverAccessorStruct_t const * const getIntegrityDriverInfoAccessors( void );

#ifdef __cplusplus
}  /* extern "C" */
#endif


 #endif /* C_INTEGRITY_DRIVER_PUB_H */