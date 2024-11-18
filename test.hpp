#ifndef TEST_HPP
#define TEST_HPP
#include <Euphemy/Io/EuphTempFile.hpp>

void testMapDatastream();
void randomDeviceTest();
void testMemoryAllocator();
void testIntegralIterable();
void testZstdWithMaps();
void testZstdWithLargeData();
void testEncryptionWithMaps();
void testEncryptionWithLargeData();
void testCompressionAndEncryptionWithMaps();
void testCompressionAndEncryptionWithLargeData();
void testMemoryMapped();
void testMemoryMappedTemp();
void testMemoryMappedTempDlopenNoClose(Euph::Io::TemporaryFileCreationMode mode = Euph::Io::TemporaryFileCreationMode::MKSTEMP);
void testMemoryMappedTempDlopenClose(Euph::Io::TemporaryFileCreationMode mode = Euph::Io::TemporaryFileCreationMode::MKSTEMP);

#endif // TEST_HPP
