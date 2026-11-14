#include "EuphSnappy.hpp"
/**
 * @file EuphSnappy.cpp
 * @brief Implements the Snappy compression and decompression proxy devices.
 */
#include <snappy-c.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace Euph {
namespace Io {

namespace {

/** @brief Magic bytes that start every stream produced by @ref SnappyCompressor. */
constexpr std::byte snappyMagic[4] { std::byte { 'S' }, std::byte { 'N' }, std::byte { 'P' },
									 std::byte { 'Y' } };
/** @brief Container version written after the magic; bumped when the framing changes. */
constexpr std::byte snappyVersion { 1 };
/** @brief Size of the stream header (magic plus version). */
constexpr size_t streamHeaderSize = 5;
/** @brief Size of one chunk header (compressed size plus uncompressed size). */
constexpr size_t chunkHeaderSize = 8;
/** @brief Input read granularity used by the decompressor. */
constexpr size_t decompressorInputSize = 65536;

/**
 * @brief Writes a 32-bit little-endian value into a byte buffer.
 * @param destination Buffer receiving four bytes.
 * @param value Value to write.
 */
void writeLe32(std::byte* destination, std::uint32_t value)
{
	for (unsigned index = 0; index < 4; ++index)
		destination[index] = static_cast<std::byte>((value >> (8 * index)) & 0xFF);
}

/**
 * @brief Reads a 32-bit little-endian value from a byte buffer.
 * @param source Buffer holding at least four bytes.
 * @return The decoded value.
 */
std::uint32_t readLe32(const std::byte* source)
{
	std::uint32_t value = 0;
	for (unsigned index = 0; index < 4; ++index)
		value |= static_cast<std::uint32_t>(static_cast<unsigned char>(source[index])) << (8 * index);
	return value;
}

/**
 * @brief Converts a Snappy status code into an exception message.
 * @param status Status returned by the Snappy C API.
 * @param action Description of the failed operation, used as the message prefix.
 * @return Human-readable message.
 */
std::string snappyError(snappy_status status, const char* action)
{
	switch (status) {
		case SNAPPY_INVALID_INPUT:
			return std::string(action) + ": invalid input";
		case SNAPPY_BUFFER_TOO_SMALL:
			return std::string(action) + ": buffer too small";
		default:
			return std::string(action) + ": unknown Snappy error";
	}
}

}

size_t SnappyCompressor::processInBufferToOutBuffer()
{
	const size_t inputSize = static_cast<size_t>(bufferCursor);
	size_t compressedSize = outBuffer.size() - chunkHeaderSize;
	const snappy_status status = snappy_compress(reinterpret_cast<const char*>(inBuffer.data()), inputSize,
		reinterpret_cast<char*>(outBuffer.data()) + chunkHeaderSize, &compressedSize);
	if (status != SNAPPY_OK)
		throw std::runtime_error(snappyError(status, "Snappy compression failed"));
	writeLe32(outBuffer.data(), static_cast<std::uint32_t>(compressedSize));
	writeLe32(outBuffer.data() + 4, static_cast<std::uint32_t>(inputSize));
	return chunkHeaderSize + compressedSize;
}

size_t SnappyCompressor::writePrefix(Device& device)
{
	if (headerWritten)
		return 0;
	headerWritten = true;
	device.write(snappyMagic, 1, sizeof(snappyMagic));
	device.write(&snappyVersion, 1, 1);
	return streamHeaderSize;
}

size_t SnappyCompressor::writePostfix(Device& device)
{
	(void)device;
	return 0;
}

SnappyCompressor::SnappyCompressor(Device* underlyingDevice, size_t chunkSize)
	: Elv::Io::BufferedWriteDeviceProxy(underlyingDevice, std::max<size_t>(1, chunkSize),
		snappy_max_compressed_length(std::max<size_t>(1, chunkSize)) + chunkHeaderSize)
	, headerWritten(false)
	, finalized(false)
{
}

SnappyCompressor::~SnappyCompressor()
{
	onFinish();
}

SnappyCompressor::SnappyCompressor(SnappyCompressor&& mov)
	: Elv::Io::BufferedWriteDeviceProxy(std::move(mov))
{
	headerWritten = mov.headerWritten;
	finalized = mov.finalized;
	mov.headerWritten = true;
	mov.finalized = true;
}

SnappyCompressor& SnappyCompressor::operator=(SnappyCompressor&& mov)
{
	if (this != &mov) {
		onFinish();
		Elv::Io::BufferedWriteDeviceProxy::operator=(std::move(mov));
		headerWritten = mov.headerWritten;
		finalized = mov.finalized;
		mov.headerWritten = true;
		mov.finalized = true;
	}
	return *this;
}

void SnappyCompressor::onFinish()
{
	if (finalized || !underlyingDevice)
		return;
	// Writes the pending chunk (which also emits the header on the first flush) ...
	flushBufferToUnderlyingDevice();
	// ... and keeps an empty stream well-formed by emitting the header on its own.
	if (!headerWritten)
		writePrefix(*underlyingDevice);
	underlyingDevice->flush();
	finalized = true;
}

bool SnappyDecompressor::readMoreInput()
{
	const size_t bytesRead = underlyingDevice->read(inBuffer.data(), 1, inBuffer.size());
	if (bytesRead == 0)
		return false;
	pendingInput.insert(pendingInput.end(), inBuffer.begin(), inBuffer.begin() + bytesRead);
	return true;
}

bool SnappyDecompressor::consumeHeader()
{
	if (headerConsumed)
		return true;
	while (pendingInput.size() < streamHeaderSize) {
		if (!readMoreInput())
			return false;
	}
	if (std::memcmp(pendingInput.data(), snappyMagic, sizeof(snappyMagic)) != 0)
		throw std::runtime_error("Snappy stream has an unknown magic value");
	if (pendingInput[sizeof(snappyMagic)] != snappyVersion)
		throw std::runtime_error("Snappy stream has an unsupported version");
	pendingInput.erase(pendingInput.begin(), pendingInput.begin() + streamHeaderSize);
	headerConsumed = true;
	return true;
}

size_t SnappyDecompressor::processInBufferToOutBuffer(size_t bytesRead)
{
	if (bytesRead)
		pendingInput.insert(pendingInput.end(), inBuffer.begin(), inBuffer.begin() + bytesRead);

	if (!consumeHeader()) {
		if (pendingInput.empty())
			return 0; // a device that ends before its header: treat as empty input
		throw std::runtime_error("Snappy stream ended inside its header");
	}

	if (pendingInput.empty() && !readMoreInput())
		return 0;
	while (pendingInput.size() < chunkHeaderSize) {
		if (!readMoreInput())
			throw std::runtime_error("Snappy stream ended inside a chunk header");
	}

	const std::uint32_t compressedSize = readLe32(pendingInput.data());
	const std::uint32_t uncompressedSize = readLe32(pendingInput.data() + 4);
	if (compressedSize == 0 || uncompressedSize == 0)
		throw std::runtime_error("Snappy stream contains an empty chunk");
	if (uncompressedSize > outBuffer.size())
		throw std::runtime_error("Snappy chunk is larger than the decompressor's output buffer");

	while (pendingInput.size() < chunkHeaderSize + compressedSize) {
		if (!readMoreInput())
			throw std::runtime_error("Snappy stream ended inside a chunk payload");
	}

	size_t decodedSize = uncompressedSize;
	const snappy_status status = snappy_uncompress(
		reinterpret_cast<const char*>(pendingInput.data() + chunkHeaderSize), compressedSize,
		reinterpret_cast<char*>(outBuffer.data()), &decodedSize);
	if (status != SNAPPY_OK)
		throw std::runtime_error(snappyError(status, "Snappy decompression failed"));
	if (decodedSize != uncompressedSize)
		throw std::runtime_error("Snappy chunk decoded to an unexpected size");

	pendingInput.erase(pendingInput.begin(), pendingInput.begin() + chunkHeaderSize + compressedSize);
	return decodedSize;
}

SnappyDecompressor::SnappyDecompressor(Device* underlyingDevice, size_t maxChunkSize)
	: Elv::Io::BufferedReadDeviceProxy(underlyingDevice, decompressorInputSize,
		std::max<size_t>(1, maxChunkSize))
	, headerConsumed(false)
{
}

SnappyDecompressor::~SnappyDecompressor() = default;

SnappyDecompressor::SnappyDecompressor(SnappyDecompressor&& mov)
	: Elv::Io::BufferedReadDeviceProxy(std::move(mov))
	, pendingInput(std::move(mov.pendingInput))
	, headerConsumed(mov.headerConsumed)
{
	mov.pendingInput.clear();
	mov.headerConsumed = false;
}

SnappyDecompressor& SnappyDecompressor::operator=(SnappyDecompressor&& mov)
{
	if (this != &mov) {
		Elv::Io::BufferedReadDeviceProxy::operator=(std::move(mov));
		pendingInput = std::move(mov.pendingInput);
		headerConsumed = mov.headerConsumed;
		mov.pendingInput.clear();
		mov.headerConsumed = false;
	}
	return *this;
}

}
}
