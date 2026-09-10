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

typedef struct
{
    sCommonDriverControlStruct_t _driverControl; /* Control structure for the Integrity driver */
    logCallback_t logMessageFunction; /* Pointer to a function for logging information */
    createErrorCallback_t createErrorFunction; /* Pointer to a function for creating errors */
} sIntegrityDriverControlStruct_t;

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
    .logMessageFunction = NULL,
    .createErrorFunction = NULL
};


static sIntegrityDriverControlStruct_t * const THIS = &integrityDriverControlStruct;
/**************************** HELPER MACROS ************************************/
#ifndef ERROR_NONE
#define ERROR_NONE 0U
#endif

#ifndef NO_ERROR
#define NO_ERROR 0U
#endif

#ifndef CREATE_ERROR
#define CREATE_ERROR( errorCode, errorMessage ) \
    THIS->createErrorFunction( errorCode, MODULE_ID, __LINE__, false, errorMessage, moduleName )
#endif
/****************************** Function implementations ***************/
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
    sCRC16Config_t defaultCRC16Config = DEFAULT_CRC16_CONFIG;
    if( THIS->_driverControl._driverInfo._isInitialized == false )
    {
        // If the create error callback is null populate an error manually.
        if( createErrorCallback == NULL )
        {
            retValue._errorCode = ERROR_NULL_POINTER;
            retValue._fileModuleEnum = MODULE_ID;
            retValue._lineNumber = __LINE__;
            retValue._flags = 0;
            retValue._crc16 = calculateCRC16( &defaultCRC16Config, (void const *)&retValue, offsetof( sErrorCompact_t, _crc16 ) );
            if( logCallback != NULL )
            {
                (void)logCallback( THIS->_driverControl._driverInfo._moduleID, 
                                   __LINE__,
                                   LOGGING_TYPE_CRITICAL,
                                   "Integrity Driver Initialization Failed: Create Error Callback function pointer is NULL." );
            }      
        }
        else if( logCallback == NULL )
        {
            if( createErrorCallback != NULL )
            {
                (void)createErrorCallback( ERROR_NULL_POINTER, MODULE_ID, __LINE__, false, "Integrity Driver Initialization Failed: Log Callback function pointer is NULL.", moduleName );
            }      
        }
        else
        {
            THIS->createErrorFunction = createErrorCallback;
            THIS->logMessageFunction = logCallback;
            THIS->_driverControl._driverInfo._isInitialized = true;
        }
    }
    else
    {
        retValue = CREATE_ERROR( ERROR_ALREADY_INITIALIZED, NULL );
    }

    return ( retValue );
}

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

