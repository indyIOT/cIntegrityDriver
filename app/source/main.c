/** **************************************************************
 * @file: main.c
 * @brief Main entry point of the test application.
 * @author Anthony Garza
 * @copyright Copyright 2023 All Rights Reserved.
****************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <cstdarg>   // va_list, va_start, va_end
#include <cstdio>    // vsnprintf
#include "commonMacros.h"
#include "cIntegrityDriverPub.h"

#define FAKE_MEMORY_WORDS 256U
static uint32_t gFakeMemory[FAKE_MEMORY_WORDS];

static uint16_t fakeReadMemory( uint32_t address, uint8_t * const readValue, size_t readSize )
{
    if( address >= FAKE_MEMORY_WORDS )
    {
        return -1;
    }
    else if( readValue == NULL )
    {
        return -2;
    }
    else if( readSize == 0 )
    {
        return -3;
    }
    else if( ( address + readSize ) > FAKE_MEMORY_WORDS )
    {
        return -4;
    }
    else
    {
        (void)memcpy( readValue, &gFakeMemory[address], readSize );
    }

    return 0;
}

static uint16_t fakeWriteMemory( uint32_t address, uint8_t * value, size_t writeSize )
{
    if( address >= FAKE_MEMORY_WORDS )
    {
        return -1;
    }
    else if( value == NULL )
    {
        return -2;
    }
    else if( writeSize == 0 )
    {
        return -3;
    }
    else if( ( address + writeSize ) > FAKE_MEMORY_WORDS )
    {
        return -4;
    }
    else
    {
        (void)memcpy( &gFakeMemory[address], value, writeSize );
    }
    return 0;
}


static sErrorCompact_t fakeLogCallback( uint16_t moduleId,
                                 uint16_t line,
                                 eLoggingType_t type,
                                 const char *message, ... )
{
    char buffer[256];

    va_list args;
    va_start( args, message );          // start reading args right after 'message'
    vsnprintf( buffer, sizeof(buffer), message, args );  // does the %d/%s/etc. substitution
    va_end( args );

    printf("[Module %u, Line %u, Type %u] %s\n", moduleId, line, type, buffer);

    sErrorCompact_t retValue = { 0 };
    return retValue;
}

static sErrorCompact_t fakeCreateErrorCallback( uint16_t errorCode, 
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

        sErrorCompact_t retValue = BLANK_ERROR_STRUCT;
        printf("Fake Create Error Callback called with parameters: " );
        printf("Error Code: %u\n", (unsigned int)errorCode);
        printf("File Module Enum: %u\n", (unsigned int)fileModuleEnum);
        printf("Line Number: %u\n", (unsigned int)lineNumber);
        printf("Auto Store Error: %s\n", autoStoreError ? "true" : "false");
        printf("Error Message: %s\n", (errorMessage ? errorMessage : "NULL"));
        printf("Module Name: %s\n", (moduleName ? moduleName : "NULL"));
        return retValue;
    }

/**
 * @brief Program Main entry for testing libraries.
 * 
 * @return int 
 */
int main( void )
{
    int retValue = ERROR_NONE;
    sErrorCompact_t errorInfo = initIntegrityDriver( (createErrorCallback_t)&fakeCreateErrorCallback,
                                     (logCallback_t)&fakeLogCallback );
    return( retValue );
}
