/** ***********************************************
 * @file cIntegrityDriver_test.cpp
 * @brief Unit tests for cIntegrityDriver.c
 * @author Anthony Garza
 * @copyright All rights reserved 2026
*************************************************/

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <cstdarg>   // va_list, va_start, va_end
#include <cstdio>    // vsnprintf
#include "commonMacros.h"
#include "commonTypes.h"
#include "../publicInclude/cIntegrityDriverPub.h"
#include "../publicInclude/cIntegrityDriverConfig.h"

using namespace std;
namespace
{
    /**
     * @brief Converts a digest/buffer to a lowercase hex string for comparing
     *        against known-answer test vectors.
     */
    string bufferToHex( uint8_t const * const buffer, size_t length )
    {
        static char const hexChars[] = "0123456789abcdef";
        string result;
        result.reserve( length * 2U );
        for ( size_t i = 0; i < length; i++ )
        {
            result.push_back( hexChars[( buffer[i] >> 4 ) & 0x0FU] );
            result.push_back( hexChars[buffer[i] & 0x0FU] );
        }
        return ( result );
    }

    /**
     * @brief Builds the default CRC16 config via positional initialization.
     * @note See defaultCRC32Config() below for why this file avoids
     *       DEFAULT_CRC16_CONFIG's designated initializers directly.
     */
    sCRC16Config_t defaultCRC16Config()
    {
        sCRC16Config_t config = { CRC_16_MODBUS_POLY, CRC_16_SEED, CRC_16_XOROUT,
                                  CRC_16_INPUT_REFLECTED, CRC_16_OUTPUT_REFLECTED };
        return ( config );
    }

    /**
     * @brief Builds the default CRC32 config via positional initialization.
     * @note DEFAULT_CRC32_CONFIG uses designated initializers ( .poly = ... ),
     *       which is a C feature not available until C++20 -- this project
     *       targets C++14, so this file builds the struct positionally instead.
     *       (cCRC32.c itself is plain C and can use the macro directly.)
     */
    sCRC32Config_t defaultCRC32Config()
    {
        sCRC32Config_t config = { CRC_32_POLY, CRC_32_SEED, CRC_32_XOROUT,
                                  CRC_32_INPUT_REFLECTED, CRC_32_OUTPUT_REFLECTED };
        return ( config );
    }

    /**
     * @brief Placeholder AES stand-in for testing/demo purposes only.
     * @warning This is NOT real AES. It XORs each byte of the input against a
     *          repeating key, so calling it a second time with the same key
     *          recovers the original data. It exists purely to exercise an
     *          encrypt/decrypt calling pattern in tests until a real AES
     *          implementation is wired into the library.
     * @param key Pointer to the key bytes to XOR against.
     * @param keyLength Length of the key in bytes. Must be non-zero.
     * @param input Pointer to the input buffer.
     * @param length Length of the input buffer in bytes.
     * @param output Pointer to a buffer of at least length bytes to receive the result.
     */
    void dummyAesEncrypt( uint8_t const * const key, size_t const keyLength,
                          uint8_t const * const input, size_t const length,
                          uint8_t * const output )
    {
        size_t i = 0;
        for ( i = 0; i < length; i++ )
        {
            output[i] = ( uint8_t )( input[i] ^ key[i % keyLength] );
        }
    }

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

    /** @brief Known-stable identifiers mirrored from cIntegrityDriver.c (private #define/static there). */
    constexpr uint16_t kIntegrityDriverModuleId = 55240U;
    const char * const kIntegrityDriverModuleName = "cIntegrityDriver";

    /**
     * @brief Fixture that gives every test a freshly-uninitialized driver.
     *        Relies on resetIntegrityDriverForTest(), a UNIT_TESTS-only hook
     *        (see cIntegrityDriverPub.h), because the driver's control struct
     *        is a file-scope static singleton with no production way to
     *        de-initialize it -- without this, only the first TEST_F to call
     *        initIntegrityDriver() would ever see it succeed.
     */
    class IntegrityDriverTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            resetIntegrityDriverForTest();
        }
    };

    /**
     * @brief Same reset-per-test behavior as IntegrityDriverTest, kept as a
     *        separate fixture (rather than reusing IntegrityDriverTest itself)
     *        purely so these tests keep showing up under a "cCRC16" suite name
     *        in test output instead of "IntegrityDriverTest".
     * @note calculateCRC16's null-pointer error path goes through CREATE_ERROR,
     *       which since createIntegrityDriverErrorSafe() was added is safe to
     *       call whether or not the driver has been initialized -- but which
     *       branch it takes differs (registered callback vs. manual fallback),
     *       so tests below deliberately control init state rather than relying
     *       on whatever an earlier test happened to leave behind.
     */
    class cCRC16 : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            resetIntegrityDriverForTest();
        }
    };

    /** @brief See cCRC16 above -- same reasoning, mirrored for calculateCRC32(). */
    class cCRC32 : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            resetIntegrityDriverForTest();
        }
    };

} // namespace

/*****************************************************************************
 * initIntegrityDriver()
 ****************************************************************************/

TEST_F( IntegrityDriverTest, InitIntegrityDriverWithValidParametersSucceeds )
{
    sErrorCompact_t errorInfo = initIntegrityDriver(
        (createErrorCallback_t)&fakeCreateErrorCallback,
        (logCallback_t)&fakeLogCallback );

    EXPECT_EQ( ERROR_NONE, errorInfo._errorCode );
}

TEST_F( IntegrityDriverTest, InitIntegrityDriverWithNullCreateErrorCallbackFailsWithNullPointer )
{
    sErrorCompact_t errorInfo = initIntegrityDriver( NULL, (logCallback_t)&fakeLogCallback );

    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
}

TEST_F( IntegrityDriverTest, InitCalledTwiceReturnsAlreadyInitialized )
{
    sErrorCompact_t firstInit = initIntegrityDriver(
        (createErrorCallback_t)&fakeCreateErrorCallback,
        (logCallback_t)&fakeLogCallback );
    ASSERT_EQ( ERROR_NONE, firstInit._errorCode );

    sErrorCompact_t secondInit = initIntegrityDriver(
        (createErrorCallback_t)&fakeCreateErrorCallback,
        (logCallback_t)&fakeLogCallback );
    EXPECT_EQ( ERROR_ALREADY_INITIALIZED, secondInit._errorCode );
}

/*****************************************************************************
 * getIntegrityDriverInfoAccessors()
 *
 * Covers the accessor struct an app would use to version-check this driver:
 * module ID, module name, the human-readable version string, the structured
 * major/minor/patch/build-type fields, and the initialized flag.
 ****************************************************************************/

TEST_F( IntegrityDriverTest, AccessorsReturnNonNullStruct )
{
    sCommonDriverAccessorStruct_t const * const accessors = getIntegrityDriverInfoAccessors();

    ASSERT_NE( nullptr, accessors );
    EXPECT_NE( nullptr, accessors->getModuleIdFunction );
    EXPECT_NE( nullptr, accessors->getModuleVersionStringFunction );
    EXPECT_NE( nullptr, accessors->getModuleNameFunction );
    EXPECT_NE( nullptr, accessors->getModuleVersionFunction );
    EXPECT_NE( nullptr, accessors->isDriverInitializedFunction );
}

TEST_F( IntegrityDriverTest, AccessorsReportModuleIdAndName )
{
    sCommonDriverAccessorStruct_t const * const accessors = getIntegrityDriverInfoAccessors();

    EXPECT_EQ( kIntegrityDriverModuleId, accessors->getModuleIdFunction() );
    EXPECT_STREQ( kIntegrityDriverModuleName, reinterpret_cast<char const *>( accessors->getModuleNameFunction() ) );
}

TEST_F( IntegrityDriverTest, AccessorsReportVersionInfoUsableForVersionChecking )
{
    sCommonDriverAccessorStruct_t const * const accessors = getIntegrityDriverInfoAccessors();

    uint8_t const * const versionString = accessors->getModuleVersionStringFunction();
    ASSERT_NE( nullptr, versionString );
    string versionStr( reinterpret_cast<char const *>( versionString ) );
    EXPECT_GT( versionStr.size(), 0U );

    sCommonVersionStruct_t version = accessors->getModuleVersionFunction();
    EXPECT_EQ( STATIC_LIBRARY_BUILD, version._buildType ); // COMPILE_INTEGRITY_DRIVER_LIBRARY_STATIC is ON

    // An app might version-check via the structured major/minor/patch fields, or
    // by parsing/logging the human-readable string -- both should agree. The
    // generated string's format is "MAJOR.MINOR.PATCH.BUILDTYPE.TIMESTAMP".
    string expectedPrefix = to_string( version._major ) + "." +
                            to_string( version._minor ) + "." +
                            to_string( version._patch ) + ".";
    EXPECT_EQ( 0U, versionStr.rfind( expectedPrefix, 0U ) )
        << "version string '" << versionStr << "' did not start with '" << expectedPrefix << "'";
}

TEST_F( IntegrityDriverTest, AccessorsIsDriverInitializedReflectsState )
{
    sCommonDriverAccessorStruct_t const * const accessors = getIntegrityDriverInfoAccessors();

    EXPECT_FALSE( accessors->isDriverInitializedFunction() );

    sErrorCompact_t errorInfo = initIntegrityDriver(
        (createErrorCallback_t)&fakeCreateErrorCallback,
        (logCallback_t)&fakeLogCallback );
    ASSERT_EQ( ERROR_NONE, errorInfo._errorCode );

    EXPECT_TRUE( accessors->isDriverInitializedFunction() );
}

/*****************************************************************************
 * calculateCRC16() / CRC16TableCCITT_False()
 *
 * DEFAULT_CRC16_CONFIG (poly=MODBUS 0x8005, init=0xFFFF, xorOut=0x0000,
 * reflectIn/Out=false) doesn't match any of the named variants documented at
 * the top of cIntegrityDriverPub.h -- that table lists MODBUS as needing
 * reflectIn/reflectOut=true -- so there's no published check value for the
 * *default* config to assert against. Instead, the known-answer tests below
 * build the well-documented CCITT-FALSE config explicitly (poly 0x1021, init
 * 0xFFFF, xorOut 0x0000, no reflection; check("123456789") = 0x29B1, per both
 * cIntegrityDriverPub.h's own comment table and cCRC16.c's table-header
 * comment), reusing CRC_16_XMODEM_POLY since it happens to carry the same
 * 0x1021 polynomial value as CCITT-FALSE.
 ****************************************************************************/

TEST_F( cCRC16, CalculateCRC16MatchesKnownCheckValueForCcittFalse )
{
    sCRC16Config_t config = { CRC_16_XMODEM_POLY, 0xFFFFU, 0x0000U, false, false };
    const char * const checkString = "123456789";
    uint16_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC16( &config, checkString, strlen( checkString ), &crc );

    EXPECT_EQ( ERROR_NONE, errorInfo._errorCode );
    EXPECT_EQ( 0x29B1U, crc );
}

TEST_F( cCRC16, CalculateCRC16WithNullConfigWhenInitializedReturnsNullPointerError )
{
    // Explicitly initialize the driver first so this exercises the "normal"
    // null-pointer path, where CREATE_ERROR routes through the registered
    // createErrorCallback (fakeCreateErrorCallback here) rather than the
    // driver's own uninitialized fallback (see the "WhenNotInitialized" test below).
    ASSERT_EQ( ERROR_NONE, initIntegrityDriver( (createErrorCallback_t)&fakeCreateErrorCallback,
                                                (logCallback_t)&fakeLogCallback )._errorCode );
    uint16_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC16( NULL, "abc", 3U, &crc );

    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
}

TEST_F( cCRC16, CalculateCRC16WithNullConfigWhenNotInitializedReturnsNullPointerErrorWithoutCrashing )
{
    // Driver deliberately left uninitialized (fresh from SetUp): no
    // createErrorCallback has been registered, so THIS->createErrorFunction is
    // null. calculateCRC16's internal CREATE_ERROR call must not try to call
    // through that null pointer -- createIntegrityDriverErrorSafe() falls back
    // to filling the struct directly instead. This is the regression test for
    // exactly the crash scenario that fallback was added to prevent.
    uint16_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC16( NULL, "abc", 3U, &crc );

    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
    // The manual fallback still computes a real CRC16 over the struct, so this
    // should not be left at BLANK_ERROR_STRUCT's default of 0.
    EXPECT_NE( 0U, errorInfo._crc16 );
}

TEST_F( cCRC16, CalculateCRC16WithNullCrc16ValueReturnsNullPointerError )
{
    ASSERT_EQ( ERROR_NONE, initIntegrityDriver( (createErrorCallback_t)&fakeCreateErrorCallback,
                                                (logCallback_t)&fakeLogCallback )._errorCode );
    sCRC16Config_t config = { CRC_16_XMODEM_POLY, 0xFFFFU, 0x0000U, false, false };

    sErrorCompact_t errorInfo = calculateCRC16( &config, "abc", 3U, NULL );

    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
}

TEST_F( cCRC16, CalculateCRC16WithNullBufferAndZeroLengthIsSafe )
{
    // Per calculateCRC16's doc comment, a null buffer is explicitly allowed as
    // long as length is 0 -- the byte loop never dereferences it in that case.
    sCRC16Config_t config = { CRC_16_XMODEM_POLY, 0xFFFFU, 0x0000U, false, false };
    uint16_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC16( &config, NULL, 0U, &crc );

    EXPECT_EQ( ERROR_NONE, errorInfo._errorCode );
    EXPECT_EQ( 0xFFFFU, crc ); // no bytes processed: result is just init XOR xorOut
}

TEST_F( cCRC16, CalculateCRC16DiffersForDifferentBuffers )
{
    sCRC16Config_t config = defaultCRC16Config();
    uint16_t crcOne = 0U;
    uint16_t crcTwo = 0U;

    sErrorCompact_t errorInfoOne = calculateCRC16( &config, "buffer one", strlen( "buffer one" ), &crcOne );
    sErrorCompact_t errorInfoTwo = calculateCRC16( &config, "buffer two", strlen( "buffer two" ), &crcTwo );

    EXPECT_EQ( ERROR_NONE, errorInfoOne._errorCode );
    EXPECT_EQ( ERROR_NONE, errorInfoTwo._errorCode );
    EXPECT_NE( crcOne, crcTwo );
}

TEST_F( cCRC16, CRC16TableCCITT_FalseMatchesKnownCheckValue )
{
    const char * const checkString = "123456789";

    // CCITT-FALSE has a zero xorOut, so unlike CRC32TableIEEE this needs no
    // final XOR to compare against the published check value.
    uint16_t crc = CRC16TableCCITT_False( checkString, strlen( checkString ), CRC_16_SEED );

    EXPECT_EQ( 0x29B1U, crc );
}

TEST_F( cCRC16, CRC16TableCCITT_FalseChainedAcrossTwoCallsMatchesOneShot )
{
    const char * const checkString = "123456789";
    const size_t splitPoint = 4U; // arbitrary split within "123456789"

    uint16_t oneShotCrc = CRC16TableCCITT_False( checkString, strlen( checkString ), CRC_16_SEED );

    uint16_t runningCrc = CRC16TableCCITT_False( checkString, splitPoint, CRC_16_SEED );
    runningCrc = CRC16TableCCITT_False( checkString + splitPoint, strlen( checkString ) - splitPoint, runningCrc );

    EXPECT_EQ( oneShotCrc, runningCrc );
}

TEST_F( cCRC16, CalculateCRC16AgreesWithTableFunctionForCcittFalseConfig )
{
    // Two independently-written code paths (the generic bit-by-bit algorithm
    // in calculateCRC16 vs. the fast table lookup in CRC16TableCCITT_False,
    // which calculateCRC16 itself switches to under an embedded-optimized
    // build) should agree for the config the table function hard-codes.
    sCRC16Config_t config = { CRC_16_XMODEM_POLY, 0xFFFFU, 0x0000U, false, false };
    const char * const checkString = "123456789";
    uint16_t crcFromConfig = 0U;

    sErrorCompact_t errorInfo = calculateCRC16( &config, checkString, strlen( checkString ), &crcFromConfig );
    uint16_t crcFromTable = CRC16TableCCITT_False( checkString, strlen( checkString ), CRC_16_SEED );

    ASSERT_EQ( ERROR_NONE, errorInfo._errorCode );
    EXPECT_EQ( crcFromTable, crcFromConfig );
}

/*****************************************************************************
 * calculateCRC32() / CRC32TableIEEE()
 ****************************************************************************/

/**
 * @brief The standard CRC-32/ISO-HDLC check value for the ASCII string "123456789".
 */
TEST_F( cCRC32, CalculateCRC32MatchesKnownCheckValue )
{
    sCRC32Config_t config = defaultCRC32Config();
    const char * const checkString = "123456789";
    uint32_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC32( &config, checkString, strlen( checkString ), &crc );

    EXPECT_EQ( ERROR_NONE, errorInfo._errorCode );
    EXPECT_EQ( 0xCBF43926U, crc );
}

TEST_F( cCRC32, CalculateCRC32OfEmptyBufferReturnsInitXorXorOut )
{
    sCRC32Config_t config = defaultCRC32Config();
    uint32_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC32( &config, "", 0U, &crc );

    // With no bytes processed, the result is just the configured init value
    // (reflected, since reflectOut is set) XORed with xorOut.
    EXPECT_EQ( ERROR_NONE, errorInfo._errorCode );
    EXPECT_EQ( 0x00000000U, crc );
}

TEST_F( cCRC32, CalculateCRC32DiffersForDifferentBuffers )
{
    sCRC32Config_t config = defaultCRC32Config();
    uint32_t crcOne = 0U;
    uint32_t crcTwo = 0U;

    sErrorCompact_t errorInfoOne = calculateCRC32( &config, "buffer one", strlen( "buffer one" ), &crcOne );
    sErrorCompact_t errorInfoTwo = calculateCRC32( &config, "buffer two", strlen( "buffer two" ), &crcTwo );

    EXPECT_EQ( ERROR_NONE, errorInfoOne._errorCode );
    EXPECT_EQ( ERROR_NONE, errorInfoTwo._errorCode );
    EXPECT_NE( crcOne, crcTwo );
}

TEST_F( cCRC32, CalculateCRC32WithNullConfigWhenInitializedReturnsNullPointerError )
{
    // See the matching cCRC16 test above for why init state is set explicitly
    // rather than relying on test execution order.
    ASSERT_EQ( ERROR_NONE, initIntegrityDriver( (createErrorCallback_t)&fakeCreateErrorCallback,
                                                (logCallback_t)&fakeLogCallback )._errorCode );
    uint32_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC32( NULL, "abc", 3U, &crc );

    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
}

TEST_F( cCRC32, CalculateCRC32WithNullConfigWhenNotInitializedReturnsNullPointerErrorWithoutCrashing )
{
    // Regression test mirroring cCRC16's: the driver is left uninitialized, so
    // calculateCRC32's CREATE_ERROR call must fall back to filling the struct
    // directly instead of calling through a null createErrorFunction pointer.
    uint32_t crc = 0U;

    sErrorCompact_t errorInfo = calculateCRC32( NULL, "abc", 3U, &crc );

    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
    EXPECT_NE( 0U, errorInfo._crc16 );
}

TEST_F( cCRC32, CalculateCRC32WithNullCrc32ValueReturnsNullPointerError )
{
    ASSERT_EQ( ERROR_NONE, initIntegrityDriver( (createErrorCallback_t)&fakeCreateErrorCallback,
                                                (logCallback_t)&fakeLogCallback )._errorCode );
    sCRC32Config_t config = defaultCRC32Config();

    sErrorCompact_t errorInfo = calculateCRC32( &config, "abc", 3U, NULL );

    EXPECT_EQ( ERROR_NULL_POINTER, errorInfo._errorCode );
}

TEST_F( cCRC32, CRC32TableIEEEMatchesKnownCheckValue )
{
    const char * const checkString = "123456789";

    // CRC32TableIEEE (like CRC16TableCCITT_False) returns the raw running CRC
    // without the final XOR-out applied, matching calculateCRC32's own
    // embedded-optimized code path in cCRC32.c.
    uint32_t crc = CRC32TableIEEE( checkString, strlen( checkString ), CRC_32_SEED ) ^ CRC_32_XOROUT;

    EXPECT_EQ( 0xCBF43926U, crc );
}

TEST_F( cCRC32, CRC32TableIEEEChainedAcrossTwoCallsMatchesOneShot )
{
    const char * const checkString = "123456789";
    const size_t splitPoint = 4U; // arbitrary split within "123456789"

    uint32_t oneShotCrc = CRC32TableIEEE( checkString, strlen( checkString ), CRC_32_SEED );

    uint32_t runningCrc = CRC32TableIEEE( checkString, splitPoint, CRC_32_SEED );
    runningCrc = CRC32TableIEEE( checkString + splitPoint, strlen( checkString ) - splitPoint, runningCrc );

    EXPECT_EQ( oneShotCrc, runningCrc );
}

/*****************************************************************************
 * calculateSHA256()
 *
 * Known-answer vectors are from FIPS 180-4 / NIST's published SHA-256
 * examples. The three padding-boundary vectors (55/56/64 bytes of 'a') were
 * additionally cross-checked against Python's hashlib.sha256 while this file
 * was written, since off-by-one errors in the padding math are the easiest
 * way to get a SHA-256 implementation subtly wrong.
 ****************************************************************************/

TEST( cSHA256, CalculateSHA256KnownVectorAbc )
{
    sSHA256Digest_t digest = calculateSHA256( "abc", 3U );

    EXPECT_EQ( "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
               bufferToHex( digest.digest, SHA_256_DIGEST_LENGTH_BYTES ) );
}

TEST( cSHA256, CalculateSHA256KnownVectorEmptyString )
{
    sSHA256Digest_t digest = calculateSHA256( "", 0U );

    EXPECT_EQ( "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
               bufferToHex( digest.digest, SHA_256_DIGEST_LENGTH_BYTES ) );
}

TEST( cSHA256, CalculateSHA256KnownVectorNistMultiBlockMessage )
{
    const char * const message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    sSHA256Digest_t digest = calculateSHA256( message, strlen( message ) );

    EXPECT_EQ( "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
               bufferToHex( digest.digest, SHA_256_DIGEST_LENGTH_BYTES ) );
}

TEST( cSHA256, CalculateSHA256PaddingBoundaryFiftyFiveBytes )
{
    // 55 bytes: the pad byte lands exactly at the 56-byte threshold, no spill block needed.
    string message( 55U, 'a' );

    sSHA256Digest_t digest = calculateSHA256( message.data(), message.size() );

    EXPECT_EQ( "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318",
               bufferToHex( digest.digest, SHA_256_DIGEST_LENGTH_BYTES ) );
}

TEST( cSHA256, CalculateSHA256PaddingBoundaryFiftySixBytes )
{
    // 56 bytes: adding the pad byte pushes past the 56-byte threshold, forcing the spill branch.
    string message( 56U, 'a' );

    sSHA256Digest_t digest = calculateSHA256( message.data(), message.size() );

    EXPECT_EQ( "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a",
               bufferToHex( digest.digest, SHA_256_DIGEST_LENGTH_BYTES ) );
}

TEST( cSHA256, CalculateSHA256PaddingBoundarySixtyFourBytes )
{
    // 64 bytes: exactly one full block, so padding needs an entire block of its own.
    string message( 64U, 'a' );

    sSHA256Digest_t digest = calculateSHA256( message.data(), message.size() );

    EXPECT_EQ( "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb",
               bufferToHex( digest.digest, SHA_256_DIGEST_LENGTH_BYTES ) );
}

TEST( cSHA256, CalculateSHA256DigestIsThirtyTwoBytes )
{
    EXPECT_EQ( 32U, sizeof( sSHA256Digest_t::digest ) );
    EXPECT_EQ( 32U, SHA_256_DIGEST_LENGTH_BYTES );
}

/*****************************************************************************
 * dummyAesEncrypt() -- placeholder AES stand-in (see comment at definition)
 ****************************************************************************/

TEST( cAESDummy, RoundTripRecoversOriginalPlaintext )
{
    uint8_t const key[] = { 0xDEU, 0xADU, 0xBEU, 0xEFU };
    uint8_t const plaintext[] = "Attack at dawn";
    const size_t length = sizeof( plaintext );
    uint8_t ciphertext[sizeof( plaintext )] = { 0 };
    uint8_t decrypted[sizeof( plaintext )] = { 0 };

    dummyAesEncrypt( key, sizeof( key ), plaintext, length, ciphertext );
    // XOR is its own inverse, so "decrypt" is just encrypting the ciphertext again.
    dummyAesEncrypt( key, sizeof( key ), ciphertext, length, decrypted );

    EXPECT_EQ( 0, memcmp( plaintext, decrypted, length ) );
    EXPECT_NE( 0, memcmp( plaintext, ciphertext, length ) );
}

TEST( cAESDummy, DifferentKeysProduceDifferentCiphertext )
{
    uint8_t const keyOne[] = { 0x01U, 0x02U, 0x03U, 0x04U };
    uint8_t const keyTwo[] = { 0xFFU, 0xEEU, 0xDDU, 0xCCU };
    uint8_t const plaintext[] = "Same plaintext";
    const size_t length = sizeof( plaintext );
    uint8_t ciphertextOne[sizeof( plaintext )] = { 0 };
    uint8_t ciphertextTwo[sizeof( plaintext )] = { 0 };

    dummyAesEncrypt( keyOne, sizeof( keyOne ), plaintext, length, ciphertextOne );
    dummyAesEncrypt( keyTwo, sizeof( keyTwo ), plaintext, length, ciphertextTwo );

    EXPECT_NE( 0, memcmp( ciphertextOne, ciphertextTwo, length ) );
}

TEST( cAESDummy, EmptyBufferIsANoOp )
{
    uint8_t const key[] = { 0xAAU };
    uint8_t output[1] = { 0x55U }; // sentinel value; should be untouched

    dummyAesEncrypt( key, sizeof( key ), NULL, 0U, output );

    EXPECT_EQ( 0x55U, output[0] );
}

