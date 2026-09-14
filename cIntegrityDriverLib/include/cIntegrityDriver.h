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

/*************************************** Type Definitions used by driver */
typedef struct
{
    sCommonDriverControlStruct_t _driverControl; /* Control structure for the Integrity driver */
    logCallback_t logMessageFunction; /* Pointer to a function for logging information */
    createErrorCallback_t createErrorFunction; /* Pointer to a function for creating errors */
} sIntegrityDriverControlStruct_t;

/*********************  ***External Integrity Driver Private Interface ********/

extern  sIntegrityDriverControlStruct_t * const THIS;

/**
 * @brief Creates an sErrorCompact_t, going through the registered create-error
 *        callback if the driver has been initialized, or filling the struct
 *        directly (still complete with a CRC16 over it) if it has not.
 * @note This exists so CREATE_ERROR is safe to use even when the driver was
 *       never initialized -- calling straight through THIS->createErrorFunction
 *       in that state would dereference a null function pointer, since nothing
 *       has set it yet.
 * @param errorCode The error code for this error.
 * @param fileModuleEnum The module ID where the error occurred.
 * @param lineNumber The source line where the error occurred.
 * @param errorMessage A message describing the error.
 * @param callerModuleName The name of the module reporting the error.
 * @return sErrorCompact_t structure containing the error information.
 */
extern sErrorCompact_t createIntegrityDriverErrorSafe( uint16_t errorCode,
                                                        uint16_t fileModuleEnum,
                                                        uint16_t lineNumber,
                                                        uint8_t const * const errorMessage,
                                                        uint8_t const * const callerModuleName );

/**************************** HELPER MACROS ************************************/
#ifndef ERROR_NONE
#define ERROR_NONE 0U
#endif

#ifndef NO_ERROR
#define NO_ERROR 0U
#endif

#ifndef CREATE_ERROR
#define CREATE_ERROR( errorCode, errorMessage ) \
    createIntegrityDriverErrorSafe( errorCode, MODULE_ID, __LINE__, errorMessage, moduleName )
#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif

 

#endif /* C_INTEGRITY_DRIVER_H */