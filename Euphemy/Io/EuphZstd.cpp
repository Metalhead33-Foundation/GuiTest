#include "EuphZstd.hpp"
#include <zstd.h>
#include <exception>

#define MCHANDLE reinterpret_cast<ZSTD_CCtx*>(handle)
#define MHDHANDLE reinterpret_cast<ZSTD_DCtx*>(handle)
namespace Euph {
namespace Io {

class ZstdException : public std::exception {
	const char* errorString;
public:
	ZstdException(size_t code)
	{
		errorString = ZSTD_getErrorName(code);
	}
	const char* what() const noexcept override {
		return errorString;
	}
};

static void checkForError(size_t code)
{
	if(ZSTD_isError(code)) {
		throw ZstdException(code);
	}
}

size_t ZstdCompressor::processInBufferToOutBuffer()
{
	ZSTD_inBuffer zinput { inBuffer.data(), static_cast<size_t>(bufferCursor), 0 };
	ZSTD_outBuffer zoutput { outBuffer.data(), outBuffer.size(), 0 };
	checkForError(ZSTD_compressStream2(MCHANDLE, &zoutput , &zinput, ZSTD_e_continue));
	checkForError(ZSTD_compressStream2(MCHANDLE, &zoutput , &zinput, ZSTD_e_flush));
	//checkForError(ZSTD_compressStream2(MCHANDLE, &zoutput , &zinput, ZSTD_e_end));
	return zoutput.pos;
}

size_t ZstdCompressor::writePrefix(Device& device)
{
	return 0;
}

size_t ZstdCompressor::writePostfix(Device& device)
{
	return 0;
}

ZstdCompressor::ZstdCompressor(Device* underlyingDevice)
	: Elv::Io::BufferedWriteDeviceProxy(underlyingDevice, ZSTD_CStreamInSize(), ZSTD_CStreamOutSize()), handle(ZSTD_createCCtx()), finalized(false)
{

}

ZstdCompressor::~ZstdCompressor()
{
	if(handle) {
		onFinish();
		ZSTD_freeCCtx(MCHANDLE);
	}
}

ZstdCompressor::ZstdCompressor(ZstdCompressor&& mov)
	: BufferedWriteDeviceProxy(std::move(mov))
{
	this->finalized = mov.finalized;
	mov.finalized = true;
	this->handle = mov.handle;
	mov.handle = nullptr;
}

ZstdCompressor& ZstdCompressor::operator=(ZstdCompressor&& mov)
{
	this->finalized = mov.finalized;
	mov.finalized = true;
	this->handle = mov.handle;
	mov.handle = nullptr;
	BufferedWriteDeviceProxy::operator=(std::move(mov));
	return *this;
}

void ZstdCompressor::setCompressionLevel(float value)
{
	auto bounds = ZSTD_cParam_getBounds(ZSTD_c_compressionLevel);
	const float range = float(bounds.upperBound) - float(bounds.lowerBound);
	int nval = int( (value*range)+float(bounds.lowerBound) );
	ZSTD_CCtx_setParameter(MCHANDLE,ZSTD_c_compressionLevel,nval);
}

void ZstdCompressor::setChecksum(bool value)
{
	ZSTD_CCtx_setParameter(MCHANDLE,ZSTD_c_checksumFlag,value);
}

void ZstdCompressor::onFinish()
{
	if (!finalized) {
		ZSTD_inBuffer zinput { inBuffer.data(), static_cast<size_t>(bufferCursor), 0 };
		ZSTD_outBuffer zoutput { outBuffer.data(), outBuffer.size(), 0 };

		checkForError(ZSTD_compressStream2(MCHANDLE, &zoutput, &zinput, ZSTD_e_end));

		// Write only the actual compressed bytes in the output buffer
		underlyingDevice->write(zoutput.dst, 1, zoutput.pos);
		underlyingDevice->flush();

		finalized = true;
	}
}

size_t ZstdDecompressor::processInBufferToOutBuffer(size_t bytesRead)
{
	ZSTD_inBuffer zinput { inBuffer.data(), bytesRead, 0 };
	ZSTD_outBuffer zoutput { outBuffer.data(), outBuffer.size(), 0 };
	checkForError(ZSTD_decompressStream(MHDHANDLE, &zoutput, &zinput));
	return zoutput.pos;
}

ZstdDecompressor::ZstdDecompressor(Device* underlyingDevice)
	: BufferedReadDeviceProxy(underlyingDevice, ZSTD_DStreamInSize(), ZSTD_DStreamOutSize()), handle(ZSTD_createDCtx())
{

}

ZstdDecompressor::~ZstdDecompressor()
{
	if(handle) ZSTD_freeDCtx(MHDHANDLE);
}

ZstdDecompressor::ZstdDecompressor(ZstdDecompressor&& mov)
	: BufferedReadDeviceProxy(std::move(mov))
{
	this->handle = mov.handle;
	mov.handle = nullptr;
}

ZstdDecompressor& ZstdDecompressor::operator=(ZstdDecompressor&& mov)
{
	this->handle = mov.handle;
	mov.handle = nullptr;
	BufferedReadDeviceProxy::operator=(std::move(mov));
	return *this;
}

}
}
