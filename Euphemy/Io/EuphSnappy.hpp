#ifndef EUPHSNAPPY_HPP
#define EUPHSNAPPY_HPP
/**
 * @file EuphSnappy.hpp
 * @brief Declares Snappy compression and decompression proxy devices.
 *
 * @section euph_snappy_role What these classes are
 *
 * `SnappyCompressor` and `SnappyDecompressor` are the Snappy counterparts of
 * @ref Euph::Io::ZstdCompressor and @ref Euph::Io::ZstdDecompressor - buffered proxy devices that
 * compress everything written to them (respectively decompress everything read from them) while
 * leaving the underlying @ref Elv::Io::Device untouched. They exist so that asset payloads can be
 * stored Snappy-compressed and read back without the caller ever handling a codec.
 *
 * @section euph_snappy_format The container format
 *
 * Snappy compresses one buffer at a time and has no streaming format of its own, so this module
 * defines a deliberately small chunked container (all integers little-endian):
 *
 * @code
 * stream := magic chunk*
 * magic  := "SNPY" version(1 byte, currently 1)
 * chunk  := compressedSize(uint32) uncompressedSize(uint32) payload[compressedSize]
 * @endcode
 *
 * Every chunk is an independent Snappy block, so a reader decodes incrementally and a truncated
 * tail only loses the last chunk. `uncompressedSize` is stored explicitly instead of being
 * recovered with `snappy_uncompressed_length()` so that a decoder can reject a chunk before
 * copying into its output buffer.
 *
 * @section euph_snappy_units Sizes and compatibility
 *
 * The compressor's chunk size (constructor argument, 64 KiB by default) is the granularity of the
 * container. The decompressor must be able to hold one whole chunk, so its `maxChunkSize` (4 MiB by
 * default) has to be at least as large as the writer's chunk size; a larger chunk is rejected with
 * an exception instead of being silently truncated.
 *
 * @warning Like @ref Euph::Io::ZstdDecompressor, `SnappyDecompressor` inherits `size()`, `tell()`
 *          and `eof()` from the buffered proxy base class, so they describe the *compressed*
 *          device, not the decompressed stream. Read until `read()` produces less than requested
 *          instead of trusting `size()`.
 *
 * @code
 * Euph::Io::File raw("/tmp/payload.bin", Euph::Io::File::Mode::WRITE);
 * {
 *     Euph::Io::SnappyCompressor compressor(&raw);
 *     compressor.write(bytes.data(), 1, bytes.size());
 * }   // the destructor flushes the final chunk
 *
 * Euph::Io::File stored("/tmp/payload.bin", Euph::Io::File::Mode::READ);
 * Euph::Io::SnappyDecompressor decompressor(&stored);
 * while (const size_t got = decompressor.read(out.data() + filled, 1, out.size() - filled))
 *     filled += got;
 * @endcode
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDeviceProxy.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace Euph {
namespace Io {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(SnappyCompressor)
/**
 * @brief Snappy compression proxy device for an underlying Device.
 *
 * Data written to this device is buffered into chunks; each time a chunk fills up (or `flush()` is
 * called, or the object is destroyed) the chunk is Snappy-compressed and appended to the underlying
 * device as one framed record. See @ref euph_snappy_format for the byte layout.
 *
 * The proxy is write-only: `read()` always returns `0`.
 */
class MH_EUPH_API SnappyCompressor : public Elv::Io::BufferedWriteDeviceProxy {
private:
	/**
	 * @brief Whether the stream header has already been written to the underlying device.
	 *
	 * The header is emitted lazily - on the first chunk, or in `onFinish()` for an empty stream - so
	 * that constructing a compressor never touches the device.
	 */
	bool headerWritten;

	/** @brief Whether @ref onFinish already flushed the final chunk. */
	bool finalized;

	/**
	 * @brief Deleted copy constructor.
	 *
	 * A compressor owns the chunk currently being filled, so copying it makes no sense.
	 */
	SnappyCompressor(const SnappyCompressor& cpy) = delete;

	/** @brief Deleted copy assignment operator. */
	SnappyCompressor& operator=(const SnappyCompressor& cpy) = delete;
protected:
	/**
	 * @brief Compresses the pending input buffer into one framed chunk.
	 * @return Number of bytes written to the output buffer: the 8-byte chunk header plus the
	 *         compressed payload.
	 * @note `snappy_compress` never needs more than `snappy_max_compressed_length(chunkSize)`
	 *       bytes, which is what the constructor reserves, so a chunk always fits.
	 */
	size_t processInBufferToOutBuffer() override;

	/**
	 * @brief Writes the stream header if it has not been written yet.
	 * @param device Underlying device that receives the header.
	 * @return Number of bytes written (`5` on the first call, `0` afterwards).
	 * @note The base class ignores this return value, which is why the header is written to
	 *       @p device directly instead of being placed in the output buffer.
	 */
	size_t writePrefix(Device& device) override;

	/**
	 * @brief Writes a per-chunk trailer.
	 * @param device Unused.
	 * @return Always `0`; this container has no trailer.
	 */
	size_t writePostfix(Device& device) override;

public:
	/**
	 * @brief Constructs a compressor writing to underlyingDevice.
	 * @param underlyingDevice Device that receives the compressed stream; must outlive this object.
	 * @param chunkSize Number of uncompressed bytes buffered before a chunk is compressed. Larger
	 *        values compress slightly better and use more memory. `0` is clamped to `1`.
	 */
	explicit SnappyCompressor(Device* underlyingDevice, size_t chunkSize = 65536);

	/**
	 * @brief Flushes the pending chunk and releases the compressor.
	 * @note The destructor is the flush point: destroying the object is what makes the stream
	 *       complete, exactly like @ref Euph::Io::ZstdCompressor.
	 */
	~SnappyCompressor();

	/**
	 * @brief Move constructor.
	 * @param mov Compressor to take over; left finalised so its destructor writes nothing.
	 */
	SnappyCompressor(SnappyCompressor&& mov);

	/**
	 * @brief Move assignment operator.
	 * @param mov Compressor to take over; left finalised.
	 * @return Reference to this compressor.
	 */
	SnappyCompressor& operator=(SnappyCompressor&& mov);

	/**
	 * @brief Flushes the pending chunk, writes the header if the stream is still empty, and marks
	 *        the stream complete.
	 *
	 * Called automatically by the destructor. Calling it earlier is allowed and makes the output
	 * complete for a reader that is opened in the meantime; a second call does nothing.
	 */
	void onFinish() override;
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(SnappyDecompressor)
/**
 * @brief Snappy decompression proxy device for an underlying Device.
 *
 * Reads the framed container produced by @ref SnappyCompressor, validating the header and the
 * per-chunk sizes. One `read()` request may span several chunks: the proxy decodes chunk by chunk
 * and hands out one decompressed chunk per internal fill.
 *
 * The proxy is read-only: `write()` always returns `0`.
 */
class MH_EUPH_API SnappyDecompressor : public Elv::Io::BufferedReadDeviceProxy {
private:
	/** @brief Bytes read from the device that do not yet form a complete chunk. */
	std::vector<std::byte> pendingInput;

	/** @brief Whether the 5-byte stream header was already validated and consumed. */
	bool headerConsumed;

	/**
	 * @brief Deleted copy constructor.
	 *
	 * A decompressor owns partially decoded input, so copying it makes no sense.
	 */
	SnappyDecompressor(const SnappyDecompressor& cpy) = delete;

	/** @brief Deleted copy assignment operator. */
	SnappyDecompressor& operator=(const SnappyDecompressor& cpy) = delete;

	/**
	 * @brief Appends one more block from the underlying device to @ref pendingInput.
	 * @return `true` when bytes were appended, `false` at the end of the device.
	 */
	bool readMoreInput();

	/**
	 * @brief Validates and consumes the stream header.
	 * @return `false` when the device ended before the header was complete, `true` afterwards.
	 * @throws std::runtime_error when the magic or the version does not match.
	 */
	bool consumeHeader();
protected:
	/**
	 * @brief Decodes at most one framed chunk into the output buffer.
	 * @param bytesRead Number of bytes the base class just read into the input buffer.
	 * @return Decompressed byte count for this chunk, or `0` at the end of the stream.
	 * @throws std::runtime_error on a malformed header, an impossible chunk size, a truncated chunk
	 *         or a Snappy payload error.
	 * @note The function keeps reading from the underlying device until it holds a complete chunk, so
	 *       it does not depend on the input buffer's size matching the writer's chunk size.
	 */
	size_t processInBufferToOutBuffer(size_t bytesRead) override;

public:
	/**
	 * @brief Constructs a decompressor reading from underlyingDevice.
	 * @param underlyingDevice Device holding the compressed stream; must outlive this object.
	 * @param maxChunkSize Largest uncompressed chunk the proxy can produce, in bytes. Must be at
	 *        least the writer's chunk size, otherwise decoding fails with an exception. It is also
	 *        the size of the output buffer, so it bounds the memory one fill may need.
	 */
	explicit SnappyDecompressor(Device* underlyingDevice, size_t maxChunkSize = 4 * 1024 * 1024);

	/** @brief Releases the decompressor. */
	~SnappyDecompressor();

	/**
	 * @brief Move constructor.
	 * @param mov Decompressor to take over; left empty.
	 */
	SnappyDecompressor(SnappyDecompressor&& mov);

	/**
	 * @brief Move assignment operator.
	 * @param mov Decompressor to take over; left empty.
	 * @return Reference to this decompressor.
	 */
	SnappyDecompressor& operator=(SnappyDecompressor&& mov);
};

}
}
#endif // EUPHSNAPPY_HPP
