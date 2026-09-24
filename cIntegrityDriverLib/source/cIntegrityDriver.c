/** ***********************************************
 * @file cIntegrityDriver.c
 * @brief Source file for the integrity driver
 * @author Anthony Garza
 * @copyright All rights reserved 2026
*************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "commonMacros.h"
#include "commonTypes.h"
#include "cIntegrityDriverConfig.h"
#include "cIntegrityDriverPub.h"
#include "cIntegrityDriverVersion.h"
#include "cIntegrityDriver.h"
#ifdef __cplusplus
extern "C" {
#endif

/******************************** Type definitions ****************************/


/********************************Static functions Prototypes *************/
static uint16_t getModuleId( void );
static uint8_t const * getModuleVersionString( void );
static sCommonVersionStruct_t getModuleVersion( void );
static uint8_t const * getModuleName( void );
static bool isDriverInitialized( void );


/******************************** Static Global Variables **********************/
static const uint8_t moduleName[] = "cIntegrityDriver";
#define MODULE_ID 55240

static sIntegrityDriverControlStruct_t integrityDriverControlStruct = { 
    ._driverControl = { 
        ._driverInfo = {
                        ._moduleName = moduleName,
                        ._moduleVersionString = INTEGRITY_DRIVER_VERSION_STRING,
                        ._moduleID = MODULE_ID,                        
                        ._moduleVersion = { ._major = INTEGRITY_DRIVER_VERSION_MAJOR,
                                            ._minor = INTEGRITY_DRIVER_VERSION_MINOR,
                                            ._patch = INTEGRITY_DRIVER_VERSION_PATCH,
                                            ._buildType = INTEGRITY_DRIVER_VERSION_BUILD_TYPE_ENUM
                        },
                        ._isInitialized = false
        },
        ._driverAccessors = { 
                            .getModuleIdFunction = getModuleId,
                            .getModuleVersionStringFunction = getModuleVersionString,
                            .getModuleNameFunction = getModuleName,
                            .getModuleVersionFunction = getModuleVersion,
                            .isDriverInitializedFunction = isDriverInitialized },
    },
#if ( INTEGRITY_LOGGING_ENABLED == DEF_TRUE )
    .logMessageFunction = NULL,
#endif
    .createErrorFunction = NULL
};

/************************************Driver wide variables ********************/
 sIntegrityDriverControlStruct_t * const THIS = &integrityDriverControlStruct;

/**************************** HELPER MACROS ************************************/

/****************************** Function implementations ***************/
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
sErrorCompact_t createIntegrityDriverErrorSafe( uint16_t errorCode,
                                                uint16_t fileModuleEnum,
                                                uint16_t lineNumber,
                                                uint8_t const * const errorMessage,
                                                uint8_t const * const callerModuleName )
{
    sErrorCompact_t retValue = BLANK_ERROR_STRUCT;
    sCRC16Config_t defaultCRC16Config = DEFAULT_CRC16_CONFIG;

    if( THIS->createErrorFunction != NULL )
    {
        retValue = THIS->createErrorFunction( errorCode, fileModuleEnum, lineNumber, false, errorMessage, callerModuleName );
    }
    else
    {
        /* Driver isn't initialized (or createErrorFunction was never set), so
           there is no registered callback to call through -- fill the struct
           directly instead of dereferencing a null function pointer. */
        retValue._errorCode = errorCode;
        retValue._fileModuleEnum = fileModuleEnum;
        retValue._lineNumber = lineNumber;
        retValue._flags = 0;
        (void)calculateCRC16( &defaultCRC16Config,
                              (void const *)&retValue,
                              offsetof( sErrorCompact_t, _crc16 ),
                              &retValue._crc16 );
    }

    return ( retValue );
}

/**
 * @brief Function to initialize the integrity driver. This should be called before any other functions are used.
 * @param createErrorCallback Pointer to a function for creating errors for the integrity driver.
 * @param logCallback Pointer to a function for logging messages for the integrity driver.
 * @return sErrorCompact_t structure containing the integrity information if an error occurred.
 */
sErrorCompact_t initIntegrityDriver( createErrorCallback_t createErrorCallback,
                                     logCallback_t logCallback )
{
    sErrorCompact_t retValue = BLANK_ERROR_STRUCT;
    if( THIS->_driverControl._driverInfo._isInitialized == false )
    {
        // If the create error callback is null populate an error manually
        // (createIntegrityDriverErrorSafe handles this since createErrorFunction
        // hasn't been set yet at this point).
        if( createErrorCallback == NULL )
        {
            retValue = CREATE_ERROR( ERROR_NULL_POINTER,
                                     NULL);
#if ( INTEGRITY_LOGGING_ENABLED == DEF_TRUE )
            if( logCallback != NULL )
            {
                (void)logCallback( THIS->_driverControl._driverInfo._moduleID,
                                   __LINE__,
                                   LOGGING_TYPE_CRITICAL,
                                   "Integrity Driver Initialization Failed: Create Error Callback function pointer is NULL." );
            }
#endif
        }
#if ( INTEGRITY_LOGGING_ENABLED == DEF_TRUE )
        else if( logCallback == NULL )
        {
            if( createErrorCallback != NULL )
            {
                retValue = CREATE_ERROR( ERROR_NULL_POINTER, 
                                         NULL );
            }      
        }
#endif
        else
        {
            THIS->createErrorFunction = createErrorCallback;
#if ( INTEGRITY_LOGGING_ENABLED == DEF_TRUE )
            THIS->logMessageFunction = logCallback;
#endif
            THIS->_driverControl._driverInfo._isInitialized = true;
        }
    }
    else
    {
        retValue = CREATE_ERROR( ERROR_ALREADY_INITIALIZED, NULL );
    }

    return ( retValue );
}

#ifdef UNIT_TESTS
/**
 * @brief Test-only hook that resets the integrity driver back to an uninitialized state.
 * @note Compiled only when UNIT_TESTS is defined. See cIntegrityDriverPub.h.
 */
void resetIntegrityDriverForTest( void )
{
    THIS->_driverControl._driverInfo._isInitialized = false;
    THIS->logMessageFunction = NULL;
    THIS->createErrorFunction = NULL;
}
#endif

/**
 * @brief Function to get the Integrity driver information.
 *        This will return a structure containing accessors to
 *       get the module ID, version string, and other information 
 *  about the integrity driver.
 * @return sCommonDriverAccessorStruct_t Structure containing accessors to 
 *             get the module ID, version string, and other information about the integrity driver.
 */
sCommonDriverAccessorStruct_t const * const getIntegrityDriverInfoAccessors( void )
{
    return (sCommonDriverAccessorStruct_t const * const)( &THIS->_driverControl._driverAccessors );
}

/************************ Static Function Implementations ***************/
/**
 * @brief Function to get the module ID of the integrity driver.
 */
static uint16_t getModuleId( void )
{
    return ( THIS->_driverControl._driverInfo._moduleID );
}

/**
 * @brief Function to get the module version string of the integrity driver.
 */
static uint8_t const * getModuleVersionString( void )
{
    return ( THIS->_driverControl._driverInfo._moduleVersionString );
}

/**
 * @brief Function to get the module version of the integrity driver.
 */
static sCommonVersionStruct_t getModuleVersion( void )
{
    return ( THIS->_driverControl._driverInfo._moduleVersion );
}

/**
 * @brief Function to get the module name of the integrity driver.
 */
static uint8_t const * getModuleName( void )
{
    return ( THIS->_driverControl._driverInfo._moduleName );
}

/**
 * @brief Function to check if the integrity driver is initialized.
 */
static bool isDriverInitialized( void )
{
    return ( THIS->_driverControl._driverInfo._isInitialized );
}
#ifdef __cplusplus
}  /* extern "C" */
#endif

