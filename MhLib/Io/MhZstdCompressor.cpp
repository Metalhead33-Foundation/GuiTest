#include <MhLib/Io/MhZstdCompressor.hpp>
#include <zstd.h>
#include <cassert>
#include <stdexcept>

#define MCHANDLE reinterpret_cast<ZSTD_CCtx*>(handle)

namespace MH33 {
namespace Io {


ZstdCompressor::ZstdCompressor() : ProxyWriteStream(ZSTD_CStreamInSize(), ZSTD_CStreamOutSize()), handle(ZSTD_createCCtx())
{
	if(!handle) throw std::runtime_error("Failed to create ZSTD compressor!");
	ZSTD_CCtx_setParameter(MCHANDLE, ZSTD_c_nbWorkers, 0);
}

ZstdCompressor::ZstdCompressor(Device *input) : ProxyWriteStream(input, ZSTD_CStreamInSize(), ZSTD_CStreamOutSize()), handle(ZSTD_createCCtx())
{
	if(!handle) throw std::runtime_error("Failed to create ZSTD compressor!");
	ZSTD_CCtx_setParameter(MCHANDLE, ZSTD_c_nbWorkers, 0);
}

void ZstdCompressor::fillBuffers(const void *input, size_t inSize, void *outBuff, size_t &outBuffCursor, size_t outBuffMaxSize)
{
	ZSTD_inBuffer znput = { input, inSize, 0 };
	ZSTD_outBuffer zutput = { outBuff, outBuffMaxSize, outBuffCursor };
	ZSTD_compressStream2(MCHANDLE, &zutput , &znput, ZSTD_e_continue);
	ZSTD_compressStream2(MCHANDLE, &zutput , &znput, ZSTD_e_flush);
	ZSTD_compressStream2(MCHANDLE, &zutput , &znput, ZSTD_e_end);
	outBuffCursor = znput.pos;
}

ZstdCompressor::~ZstdCompressor()
{
	if(handle) ZSTD_freeCCtx(MCHANDLE);
}

/*void ZstdCompressor::compress()
{
	assert(handle);
	for (;;) {
		const size_t read = input->read(inBuff.data(),inBuff.size());
		const bool lastChunk = read < inBuff.size();
		const ZSTD_EndDirective mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;
		ZSTD_inBuffer znput = { inBuff.data(), read, 0 };
		bool finished;
		do {
			ZSTD_outBuffer zutput = { outBuff.data(), outBuff.size(), 0 };
			const size_t remaining = ZSTD_compressStream2(MCHANDLE, &zutput , &znput, mode);
			output->write(outBuff.data(),zutput.pos);
			finished = lastChunk ? (remaining == 0) : (znput.pos == znput.size);
		} while(!finished);
		if (lastChunk) {
			break;
		}
	}
}*/

void ZstdCompressor::setCompressionLevel(float value)
{
	assert(handle);
	auto bounds = ZSTD_cParam_getBounds(ZSTD_c_compressionLevel);
	const float range = float(bounds.upperBound) - float(bounds.lowerBound);
	int nval = int( (value*range)+float(bounds.lowerBound) );
	ZSTD_CCtx_setParameter(MCHANDLE,ZSTD_c_compressionLevel,nval);
}

void ZstdCompressor::setChecksum(bool value)
{
	assert(handle);
	ZSTD_CCtx_setParameter(MCHANDLE,ZSTD_c_checksumFlag,value);
}

}
}
