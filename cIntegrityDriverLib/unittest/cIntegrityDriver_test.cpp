/** ***********************************************
 * @file cIntegrityDriver_test.cpp
 * @brief Unit tests for cIntegrityDriver.c
 * @author Anthony Garza
 * @copyright All rights reserved 2026
*************************************************/

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <cstdarg>   // va_list, va_start, va_end
#include <cstdio>    // vsnprintf
#include "commonMacros.h"
#include "commonTypes.h"
#include "../publicInclude/cIntegrityDriverPub.h"
#include "../publicInclude/cIntegrityDriverConfig.h"

using namespace std;
namespace 
{
    sErrorCompact_t fakeCreateErrorCallback( uint16_t errorCode, 
                                                  uint16_t fileModuleEnum, 
                                                  uint16_t lineNumber,
                                                  bool autoStoreError,
                                                  uint8_t const * const errorMessage, 
                                                  uint8_t const * const moduleName )
    {
        (void)errorCode;
        (void)fileModuleEnum;
        (void)lineNumber;
        (void)autoStoreError;
        (void)errorMessage;
        (void)moduleName;

        sErrorCompact_t retValue = { 0 };
        if( errorCode != ERROR_NONE )
        {
            retValue._errorCode = errorCode;
            retValue._fileModuleEnum = fileModuleEnum;
            retValue._lineNumber = lineNumber;
            retValue._flags = 0;
            retValue._crc16 = 0; // For testing purposes, we can set this to 0
            
            cout << "Fake Create Error Callback called with parameters: " << endl;  
            cout << "Error Code: " << errorCode << endl;
            cout << "File Module Enum: " << fileModuleEnum << endl;
            cout << "Line Number: " << lineNumber << endl;
            cout << "Auto Store Error: " << (autoStoreError ? "true" : "false") << endl;
            cout << "Error Message: " << (errorMessage ? reinterpret_cast<const char*>(errorMessage) : "NULL") << endl;
            cout << "Module Name: " << (moduleName ? reinterpret_cast<const char*>(moduleName) : "NULL") << endl;
        }
        else
        {
            retValue._errorCode = ERROR_NONE;
            retValue._fileModuleEnum = 0;
            retValue._lineNumber = 0;
            retValue._flags = 0;
            retValue._crc16 = 0; // For testing purposes, we can set this to 0
        }
        return retValue;
    }

    sErrorCompact_t fakeLogCallback( uint16_t moduleId,
                                     uint16_t line,
                                     eLoggingType_t type,
                                     const char *message, ... )
    {
        sErrorCompact_t retValue = { 0 };
        char buffer[256];

        va_list args;
        va_start( args, message );          // start reading args right after 'message'
        vsnprintf( buffer, sizeof(buffer), message, args );  // does the %d/%s/etc. substitution
        va_end( args );

        cout << "[Module " << moduleId << ", Line " << line
            << ", Type " << type << "] " << buffer << endl;

    
        return retValue;

    }

} // namespace

/**
 * @brief Test case for initializing the integrity driver with valid parameters.
 */
TEST( cIntegrityDriver, initIntegrityDriver )
{
    sErrorCompact_t errorInfo = initIntegrityDriver(
        (createErrorCallback_t)&fakeCreateErrorCallback,
        (logCallback_t)&fakeLogCallback );
    
    EXPECT_EQ( ERROR_NONE, errorInfo._errorCode );
}

/**
 * @brief Test case for initializing the integrity driver with a NULL createErrorCallback.   
 */
TEST( cIntegrityDriver, initIntegrityDriverWithNullCreateErrorCallback )
{
    sErrorCompact_t errorInfo = initIntegrityDriver( NULL, (logCallback_t)&fakeLogCallback );
    
    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
}

