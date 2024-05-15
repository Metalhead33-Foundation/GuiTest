#include <MhLib/Io/MhSnappyDecompressor.hpp>
#include <limits>
#include <snappy.h>
#include <MhLib/Io/MhFixedBufferWrapper.hpp>
#include <MhLib/Io/MhDataStream.hpp>

namespace MH33 {
namespace Io {

void SnappyDecompressor::fillBuffers(const void *inBuff, size_t &inBuffCursor, size_t inBuffSize, void *outBuff, size_t &outBuffEnd, size_t outBuffSize)
{
	uint16_t magicNumber;
	FixedBufferWrapper head(const_cast<void*>(inBuff),inBuffSize);
	DataStreamBE buffStream(head);
	buffStream >> magicNumber;
	if(magicNumber == 0xAFFE) {
		uint16_t uncompressedSize, compressedSize;
		buffStream >> uncompressedSize >> compressedSize;
		const char* in = &static_cast<const char*>(inBuff)[6];
		snappy::RawUncompress(in,compressedSize,static_cast<char*>(outBuff));
		outBuffEnd = uncompressedSize;
		inBuffCursor = compressedSize + 6;
	}
}

static constexpr const uint16_t MAX_SNAPPY_BLOCK_SIZE = (std::numeric_limits<uint16_t>::max() / 2) - 6;

SnappyDecompressor::SnappyDecompressor() : ProxyReadStream( snappy::MaxCompressedLength(MAX_SNAPPY_BLOCK_SIZE)+6, MAX_SNAPPY_BLOCK_SIZE )
{

}

SnappyDecompressor::SnappyDecompressor(Device *input) : ProxyReadStream( input, snappy::MaxCompressedLength(MAX_SNAPPY_BLOCK_SIZE)+6, MAX_SNAPPY_BLOCK_SIZE )
{

}

SnappyDecompressor::~SnappyDecompressor()
{

}

}
}
