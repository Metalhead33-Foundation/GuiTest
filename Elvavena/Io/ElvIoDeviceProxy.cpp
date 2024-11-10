#include "ElvIoDeviceProxy.hpp"
#include <cstring>
namespace Elv {
namespace Io {

bool BufferedWriteDeviceProxy::flushBufferToUnderlyingDevice()
{
	if(bufferCursor)
	{
		size_t bytesToWrite = processInBufferToOutBuffer();
		writePrefix(*underlyingDevice);
		underlyingDevice->write(outBuffer.data(),1,bytesToWrite);
		writePostfix(*underlyingDevice);
		bufferCursor = 0;
	}
	return underlyingDevice->flush();
}

BufferedWriteDeviceProxy::BufferedWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize)
	: BufferedDeviceProxy(underlyingDevice, inBufferSize, outBufferSize)
{

}
size_t BufferedWriteDeviceProxy::read(void* buffer, size_t size, size_t count)
{
	(void)buffer;
	(void)size;
	(void)count;
	return 0;
}

size_t BufferedWriteDeviceProxy::write(const void* buffer, size_t size, size_t count)
{
	size_t bytesToWrite = size * count;
	size_t bytesWritten = 0;
	const std::byte* const bytePtr = static_cast<const std::byte*>(buffer);
	do {
		const size_t writeableBytes = std::min(static_cast<size_t>(inBuffer.size() - bufferCursor),bytesToWrite);
		if(!writeableBytes) {
			flushBufferToUnderlyingDevice();
			continue;
		} else {
			std::memcpy(&inBuffer[bufferCursor],&bytePtr[bytesWritten],writeableBytes);
			bufferCursor += writeableBytes;
			bytesToWrite -= writeableBytes;
			bytesWritten += writeableBytes;
			if(static_cast<size_t>(bufferCursor) == inBuffer.size()) flushBufferToUnderlyingDevice();
		}
	} while(bytesToWrite);
	return count;
}

int BufferedDeviceProxy::seek(long offset, SeekOrigin whence)
{
	return underlyingDevice ? underlyingDevice->seek(offset, whence) : -1;
}

long BufferedDeviceProxy::tell()
{
	return underlyingDevice ? underlyingDevice->tell() : -1;
}

size_t BufferedDeviceProxy::size()
{
	return underlyingDevice ? underlyingDevice->size() : 0;
}

bool BufferedDeviceProxy::eof()
{
	return underlyingDevice ? underlyingDevice->eof() : 1;
}

Mode BufferedWriteDeviceProxy::getMode() const
{
	return Mode::WRITE;
}

bool BufferedWriteDeviceProxy::flush()
{
	if(underlyingDevice) return flushBufferToUnderlyingDevice();
	else return false;
}

bool BufferedDeviceProxy::isValid() const
{
	return underlyingDevice ? underlyingDevice->isValid() : false;
}

Device* BufferedDeviceProxy::getUnderlyingDevice() const
{
	return underlyingDevice;
}

void BufferedDeviceProxy::setUnderlyingDevice(Device* newUnderlyingDevice)
{
	underlyingDevice = newUnderlyingDevice;
}


bool BufferedReadDeviceProxy::fillBuffer()
{
	size_t bytesRead = underlyingDevice->read(inBuffer.data(),1,inBuffer.size());
	outBufferFence = processInBufferToOutBuffer(bytesRead);
	bufferCursor = 0;
	return outBufferFence != 0;
}

BufferedReadDeviceProxy::BufferedReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize)
	: BufferedDeviceProxy(underlyingDevice, inBufferSize, outBufferSize), outBufferFence(0)
{

}

size_t BufferedReadDeviceProxy::read(void* buffer, size_t size, size_t count)
{
	size_t bytesToRead = size * count;
	size_t bytesRead = 0;
	std::byte* const byteBuffer = static_cast<std::byte*>(buffer);
	do {
		const size_t readableBytes = std::min(outBufferFence - static_cast<size_t>(bufferCursor),bytesToRead);
		if(!readableBytes) {
			fillBuffer();
			if(!outBufferFence) break;
		} else {
			memcpy(&byteBuffer[bytesRead],&outBuffer[bufferCursor],readableBytes);
			bytesToRead -= readableBytes;
			bytesRead += readableBytes;
			bufferCursor += readableBytes;
			if(outBufferFence == bufferCursor) fillBuffer();
		}
	} while(bytesToRead);
	return bytesRead / size;
}

size_t BufferedReadDeviceProxy::write(const void* buffer, size_t size, size_t count)
{
	(void)buffer;
	(void)size;
	(void)count;
	return 0;
}

Mode BufferedReadDeviceProxy::getMode() const
{
	return Mode::READ;
}

bool BufferedReadDeviceProxy::flush()
{
	return underlyingDevice->flush();
}


BufferedDeviceProxy::BufferedDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize)
	: underlyingDevice(underlyingDevice), inBuffer(inBufferSize), outBuffer(outBufferSize), bufferCursor(0)
{

}


}
}
