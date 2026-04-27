#ifndef TEST_HPP
#define TEST_HPP
/**
 * @file test.hpp
 * @brief Declares the test API in the Test module.
 *
 * This header is part of the public declaration surface for Test.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Io/EuphTempFile.hpp>

/** @brief Documents the testCustomAllocator declaration. */
void testCustomAllocator();
/** @brief Documents the testMapDatastream declaration. */
void testMapDatastream();
/** @brief Documents the randomDeviceTest declaration. */
void randomDeviceTest();
/** @brief Documents the testMemoryAllocator declaration. */
void testMemoryAllocator();
/** @brief Documents the testIntegralIterable declaration. */
void testIntegralIterable();
/** @brief Documents the testZstdWithMaps declaration. */
void testZstdWithMaps();
/** @brief Documents the testZstdWithLargeData declaration. */
void testZstdWithLargeData();
/** @brief Documents the testEncryptionWithMaps declaration. */
void testEncryptionWithMaps();
/** @brief Documents the testEncryptionWithLargeData declaration. */
void testEncryptionWithLargeData();
/** @brief Documents the testCompressionAndEncryptionWithMaps declaration. */
void testCompressionAndEncryptionWithMaps();
/** @brief Documents the testCompressionAndEncryptionWithLargeData declaration. */
void testCompressionAndEncryptionWithLargeData();
/** @brief Documents the testMemoryMapped declaration. */
void testMemoryMapped();
/** @brief Documents the testMemoryMappedTemp declaration. */
void testMemoryMappedTemp();
/** @brief Documents the testMemoryMappedTempDlopenNoClose declaration. */
void testMemoryMappedTempDlopenNoClose(Euph::Io::TemporaryFileCreationMode mode = Euph::Io::TemporaryFileCreationMode::MKSTEMP);
/** @brief Documents the testMemoryMappedTempDlopenClose declaration. */
void testMemoryMappedTempDlopenClose(Euph::Io::TemporaryFileCreationMode mode = Euph::Io::TemporaryFileCreationMode::MKSTEMP);
/** @brief Documents the testConfigFile declaration. */
void testConfigFile();
/** @brief Documents the testGlobalConfigFile declaration. */
void testGlobalConfigFile();
/** @brief Documents the testInterpolationFloat declaration. */
void testInterpolationFloat();
/** @brief Documents the testInterpolationFixed declaration. */
void testInterpolationFixed();
/** @brief Documents the testTGA declaration. */
void testTGA();
/** @brief Documents the testPNG declaration. */
void testPNG();
/** @brief Exercises the Euphemy asset streaming substrate. */
void testAssetStreaming();

#endif // TEST_HPP
