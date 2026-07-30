/** ***********************************************
 * @file cIntegrityDriver_test.cpp
 * @brief Unit tests for cIntegrityDriver.c
 * @author Anthony Garza
 * @copyright All rights reserved 2026
*************************************************/

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include "commonMacros.h"
#include "commonTypes.h"
#include "../publicInclude/cIntegrityDriverPub.h"
#include "../publicInclude/cIntegrityDriverConfig.h"

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

        sErrorCompact_t retValue = BLANK_ERROR_STRUCT;
        return retValue;
    }

} // namespace

/**
 * @brief Test case for initializing the integrity driver with valid parameters.
 */
TEST( cIntegrityDriver, initIntegrityDriver )
{
    sErrorCompact_t errorInfo = initIntegrityDriver(
        (createErrorCallback_t)&fakeCreateErrorCallback );
    
    EXPECT_EQ( ERROR_NONE, errorInfo._errorCode );
}
