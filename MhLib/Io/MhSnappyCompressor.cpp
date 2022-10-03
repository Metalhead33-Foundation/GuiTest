#include <MhLib/Io/MhSnappyCompressor.hpp>
#include <limits>
#include <snappy.h>
#include <MhLib/Io/MhFixedBufferWrapper.hpp>
#include <MhLib/Io/MhDataStream.hpp>

namespace MH33 {
namespace Io {

static constexpr const uint16_t MAX_SNAPPY_BLOCK_SIZE = (std::numeric_limits<uint16_t>::max() / 2) - 6;

SnappyCompressor::SnappyCompressor() : ProxyWriteStream( MAX_SNAPPY_BLOCK_SIZE, snappy::MaxCompressedLength(MAX_SNAPPY_BLOCK_SIZE)+6 )
{

}

SnappyCompressor::SnappyCompressor(Device *input) : ProxyWriteStream(input, MAX_SNAPPY_BLOCK_SIZE, snappy::MaxCompressedLength(MAX_SNAPPY_BLOCK_SIZE)+6 )
{

}

void SnappyCompressor::fillBuffers(const void *input, size_t inSize, void *outBuff, size_t &outBuffCursor, size_t outBuffMaxSize)
{
	FixedBufferWrapper head(outBuff,outBuffMaxSize);
	DataStreamBE buffStream(head);
	buffStream << uint16_t(0xAFFE) << uint16_t(inSize);
	char* out = &reinterpret_cast<char*>(outBuff)[6];
	snappy::RawCompress(reinterpret_cast<const char*>(input),inSize,out,&outBuffCursor);
	buffStream << uint16_t(outBuffCursor);
}

SnappyCompressor::~SnappyCompressor()
{

}

}
}
