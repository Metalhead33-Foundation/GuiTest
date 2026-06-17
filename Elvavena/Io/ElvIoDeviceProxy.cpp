/**
 * @file ElvIoDeviceProxy.cpp
 * @brief Implementations for buffered device proxy classes.
 */

#include "ElvIoDeviceProxy.hpp"

#include <cstring>

namespace Elv {
namespace Io {

/**
 * @brief Flushes pending buffered write data to the underlying device.
 * @return `true` when underlying flush succeeds, otherwise `false`.
 */
bool BufferedWriteDeviceProxy::flushBufferToUnderlyingDevice()
{
	if (bufferCursor)
	{
		size_t bytesToWrite = processInBufferToOutBuffer();
		writePrefix(*underlyingDevice);
		underlyingDevice->write(outBuffer.data(), 1, bytesToWrite);
		writePostfix(*underlyingDevice);
		bufferCursor = 0;
	}
	return underlyingDevice->flush();
}

/**
 * @brief Move constructor.
 * @param mov Source instance.
 */
BufferedWriteDeviceProxy::BufferedWriteDeviceProxy(BufferedWriteDeviceProxy&& mov)
	: BufferedDeviceProxy(std::move(mov))
{
}

/**
 * @brief Move assignment operator.
 * @param mov Source instance.
 * @return Reference to `*this`.
 */
BufferedWriteDeviceProxy& BufferedWriteDeviceProxy::operator=(BufferedWriteDeviceProxy&& mov)
{
	BufferedDeviceProxy::operator=(std::move(mov));
	return *this;
}

/**
 * @brief Constructs a buffered write proxy.
 * @param underlyingDevice Device to proxy.
 * @param inBufferSize Input buffer size in bytes.
 * @param outBufferSize Output buffer size in bytes.
 */
BufferedWriteDeviceProxy::BufferedWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize)
	: BufferedDeviceProxy(underlyingDevice, inBufferSize, outBufferSize)
{
}

/**
 * @brief Unsupported read operation for write proxy.
 * @param buffer Ignored.
 * @param size Ignored.
 * @param count Ignored.
 * @return Always `0`.
 */
size_t BufferedWriteDeviceProxy::read(void* buffer, size_t size, size_t count)
{
	(void)buffer;
	(void)size;
	(void)count;
	return 0;
}

/**
 * @brief Buffers write data and flushes when necessary.
 * @param buffer Source bytes to write.
 * @param size Element size.
 * @param count Number of elements.
 * @return Number of elements accepted (`count`).
 */
size_t BufferedWriteDeviceProxy::write(const void* buffer, size_t size, size_t count)
{
	size_t bytesToWrite = size * count;
	size_t bytesWritten = 0;
	const std::byte* const bytePtr = static_cast<const std::byte*>(buffer);
	do {
		const size_t writeableBytes = std::min(static_cast<size_t>(inBuffer.size() - bufferCursor), bytesToWrite);
		if (!writeableBytes) {
			flushBufferToUnderlyingDevice();
			continue;
		} else {
			std::memcpy(&inBuffer[bufferCursor], &bytePtr[bytesWritten], writeableBytes);
			bufferCursor += writeableBytes;
			bytesToWrite -= writeableBytes;
			bytesWritten += writeableBytes;
			if (static_cast<size_t>(bufferCursor) == inBuffer.size()) flushBufferToUnderlyingDevice();
		}
	} while (bytesToWrite);
	return count;
}

/**
 * @brief Delegates seek to underlying device.
 * @param offset Byte offset.
 * @param whence Seek origin.
 * @return Underlying result, or `-1` if there is no underlying device.
 */
int BufferedDeviceProxy::seek(long offset, SeekOrigin whence)
{
	return underlyingDevice ? underlyingDevice->seek(offset, whence) : -1;
}

/**
 * @brief Delegates tell to underlying device.
 * @return Current position or `-1` when unavailable.
 */
long BufferedDeviceProxy::tell()
{
	return underlyingDevice ? underlyingDevice->tell() : -1;
}

/**
 * @brief Delegates size query to underlying device.
 * @return Device size, or `0` when unavailable.
 */
size_t BufferedDeviceProxy::size()
{
	return underlyingDevice ? underlyingDevice->size() : 0;
}

/**
 * @brief Delegates EOF query to underlying device.
 * @return `true` if EOF or if no device is attached.
 */
bool BufferedDeviceProxy::eof()
{
	return underlyingDevice ? underlyingDevice->eof() : 1;
}

/**
 * @brief Returns write mode.
 * @return `Mode::WRITE`.
 */
Mode BufferedWriteDeviceProxy::getMode() const
{
	return Mode::WRITE;
}

/**
 * @brief Flushes buffered write contents.
 * @return `true` on success, otherwise `false`.
 */
bool BufferedWriteDeviceProxy::flush()
{
	if (underlyingDevice) return flushBufferToUnderlyingDevice();
	else return false;
}

/**
 * @brief Checks whether the underlying device is valid.
 * @return `true` if valid, otherwise `false`.
 */
bool BufferedDeviceProxy::isValid() const
{
	return underlyingDevice ? underlyingDevice->isValid() : false;
}

/**
 * @brief Returns the currently proxied device.
 * @return Underlying device pointer.
 */
Device* BufferedDeviceProxy::getUnderlyingDevice() const
{
	return underlyingDevice;
}

/**
 * @brief Replaces the underlying proxied device.
 * @param newUnderlyingDevice New device pointer.
 */
void BufferedDeviceProxy::setUnderlyingDevice(Device* newUnderlyingDevice)
{
	underlyingDevice = newUnderlyingDevice;
}

/**
 * @brief Fills the read buffers from the underlying device.
 * @return `true` if output buffer contains data after processing.
 */
bool BufferedReadDeviceProxy::fillBuffer()
{
	size_t bytesRead = underlyingDevice->read(inBuffer.data(), 1, inBuffer.size());
	outBufferFence = processInBufferToOutBuffer(bytesRead);
	bufferCursor = 0;
	return outBufferFence != 0;
}

/**
 * @brief Move constructor.
 * @param mov Source instance.
 */
BufferedReadDeviceProxy::BufferedReadDeviceProxy(BufferedReadDeviceProxy&& mov)
	: BufferedDeviceProxy(std::move(mov)), outBufferFence(mov.outBufferFence)
{
	mov.outBufferFence = 0;
}

/**
 * @brief Move assignment operator.
 * @param mov Source instance.
 * @return Reference to `*this`.
 */
BufferedReadDeviceProxy& BufferedReadDeviceProxy::operator=(BufferedReadDeviceProxy&& mov)
{
	this->outBufferFence = mov.outBufferFence;
	mov.outBufferFence = 0;
	BufferedDeviceProxy::operator=(std::move(mov));
	return *this;
}

/**
 * @brief Constructs a buffered read proxy.
 * @param underlyingDevice Device to proxy.
 * @param inBufferSize Input buffer size in bytes.
 * @param outBufferSize Output buffer size in bytes.
 */
BufferedReadDeviceProxy::BufferedReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize)
	: BufferedDeviceProxy(underlyingDevice, inBufferSize, outBufferSize), outBufferFence(0)
{
}

/**
 * @brief Reads buffered data from underlying device.
 * @param buffer Destination memory.
 * @param size Element size.
 * @param count Element count.
 * @return Number of elements read.
 */
size_t BufferedReadDeviceProxy::read(void* buffer, size_t size, size_t count)
{
	size_t bytesToRead = size * count;
	size_t bytesRead = 0;
	std::byte* const byteBuffer = static_cast<std::byte*>(buffer);
	do {
		const size_t readableBytes = std::min(outBufferFence - static_cast<size_t>(bufferCursor), bytesToRead);
		if (!readableBytes) {
			fillBuffer();
			if (!outBufferFence) break;
		} else {
			memcpy(&byteBuffer[bytesRead], &outBuffer[bufferCursor], readableBytes);
			bytesToRead -= readableBytes;
			bytesRead += readableBytes;
			bufferCursor += readableBytes;
			if (outBufferFence == bufferCursor) fillBuffer();
		}
	} while (bytesToRead);
	return bytesRead / size;
}

/**
 * @brief Unsupported write operation for read proxy.
 * @param buffer Ignored.
 * @param size Ignored.
 * @param count Ignored.
 * @return Always `0`.
 */
size_t BufferedReadDeviceProxy::write(const void* buffer, size_t size, size_t count)
{
	(void)buffer;
	(void)size;
	(void)count;
	return 0;
}

/**
 * @brief Returns read mode.
 * @return `Mode::READ`.
 */
Mode BufferedReadDeviceProxy::getMode() const
{
	return Mode::READ;
}

/**
 * @brief Flushes the underlying device.
 * @return Result of underlying flush call.
 */
bool BufferedReadDeviceProxy::flush()
{
	return underlyingDevice->flush();
}

/**
 * @brief Constructs base buffered proxy.
 * @param underlyingDevice Device to proxy.
 * @param inBufferSize Input buffer size in bytes.
 * @param outBufferSize Output buffer size in bytes.
 */
BufferedDeviceProxy::BufferedDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize)
	: underlyingDevice(underlyingDevice), inBuffer(inBufferSize), outBuffer(outBufferSize), bufferCursor(0)
{
}

/**
 * @brief Move constructor.
 * @param mov Source instance.
 */
BufferedDeviceProxy::BufferedDeviceProxy(BufferedDeviceProxy&& mov)
	: underlyingDevice(mov.underlyingDevice), inBuffer(std::move(mov.inBuffer)), outBuffer(std::move(mov.outBuffer)), bufferCursor(mov.bufferCursor)
{
	mov.underlyingDevice = nullptr;
	mov.bufferCursor = 0;
}

/**
 * @brief Move assignment operator.
 * @param mov Source instance.
 * @return Reference to `*this`.
 */
BufferedDeviceProxy& BufferedDeviceProxy::operator=(BufferedDeviceProxy&& mov)
{
	this->underlyingDevice = mov.underlyingDevice;
	mov.underlyingDevice = nullptr;
	this->bufferCursor = mov.bufferCursor;
	mov.bufferCursor = 0;
	this->inBuffer = std::move(mov.inBuffer);
	this->outBuffer = std::move(mov.outBuffer);
	return *this;
}

} // namespace Io
} // namespace Elv
