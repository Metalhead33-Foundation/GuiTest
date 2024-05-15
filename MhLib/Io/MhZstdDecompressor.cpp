#include <MhLib/Io/MhZstdDecompressor.hpp>
#include <zstd.h>
#include <cassert>
#include <stdexcept>

#define MHDHANDLE reinterpret_cast<ZSTD_DCtx*>(handle)

namespace MH33 {
namespace Io {

ZstdDecompressor::ZstdDecompressor() : ProxyReadStream(ZSTD_DStreamInSize(), ZSTD_DStreamOutSize()), handle(ZSTD_createDCtx())
{
	if(!handle) throw std::runtime_error("Failed to create ZSTD decompressor!");
}

ZstdDecompressor::ZstdDecompressor(Device *input) : ProxyReadStream(input, ZSTD_DStreamInSize(), ZSTD_DStreamOutSize()), handle(ZSTD_createDCtx())
{
	if(!handle) throw std::runtime_error("Failed to create ZSTD decompressor!");
}

void ZstdDecompressor::fillBuffers(const void* inBuff, size_t& inBuffCursor, size_t inBuffSize, void* outBuff, size_t& outBuffEnd, size_t outBuffSize)
{
	ZSTD_inBuffer znput = { inBuff, inBuffSize, inBuffCursor };
	ZSTD_outBuffer zutput = { outBuff, outBuffSize, 0 };
	ZSTD_decompressStream(MHDHANDLE, &zutput , &znput);
	inBuffCursor = znput.pos;
	outBuffEnd = zutput.pos;
}

ZstdDecompressor::~ZstdDecompressor()
{
	if(handle) ZSTD_freeDCtx(MHDHANDLE);
}

/*void ZstdDecompressor::decompress()
{
	assert(handle);
	if(!input || !output) return;
	size_t read = 0;
	//size_t lastRet = 0;
	while( (read = input->read(inBuff.data(),inBuff.size() )) ) {
		ZSTD_inBuffer znput = { inBuff.data(), read, 0 };
		while (znput.pos < znput.size) {
			ZSTD_outBuffer zutput = { outBuff.data(), outBuff.size(), 0 };
			ZSTD_decompressStream(MHDHANDLE, &zutput , &znput);
			output->write(outBuff.data(),zutput.pos);
			//lastRet = ret;
		}
	}
}*/

}
}
