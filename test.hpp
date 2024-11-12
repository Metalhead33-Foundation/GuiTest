#ifndef TEST_HPP
#define TEST_HPP

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
void testMemoryMappedTempDlopenNoClose();
void testMemoryMappedTempDlopenClose();

#endif // TEST_HPP
