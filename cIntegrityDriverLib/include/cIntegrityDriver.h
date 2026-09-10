/*********************************************************************
 * @file cIntegrityDriver.h
 * @brief Header file for the integrity driver
 * @author Anthony Garza
 * @copyright All rights reserved 2026
 *********************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "commonTypes.h"
#include "commonMacros.h"
#ifndef C_INTEGRITY_DRIVER_H
#define C_INTEGRITY_DRIVER_H
#ifdef __cplusplus
extern "C" {
#endif

/*********************  ***External Integrity Driver Private Interface ********/
/**
 * @brief Function to get the error message corresponding to a common error code.
 * @param errorCode The common error code to get the message for.
 * @param errorMessage Pointer to a buffer to store the error message.
 * @returns A pointer to the error message string.
 */
extern sErrorCompact_t getIntegrityDriverErrorMessageFromCode( uint16_t const errorCode,
                                                               uint8_t const * errorMessage );
#ifdef __cplusplus
}  /* extern "C" */
#endif

 

#endif /* C_INTEGRITY_DRIVER_H */