#ifndef EUPHZSTD_H
#define EUPHZSTD_H
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDeviceProxy.hpp>
namespace Euph {
namespace Io {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ZstdCompressor)
/**
 * @brief Zstandard compression proxy device for an underlying Device.
 *
 * The ZstdCompressor class is a buffered write proxy device that provides Zstandard compression for data
 * written to an underlying Device. It compresses data in blocks and writes the compressed data to the
 * underlying device. This class is useful for efficiently compressing data while writing to files or
 * other devices.
 */
class MH_EUPH_API ZstdCompressor : public Elv::Io::BufferedWriteDeviceProxy {
private:
	/**
	 * @brief Handle to the Zstandard compression context.
	 */
	void* handle;

	/**
	 * @brief Flag indicating if the compressor has been finalized.
	 */
	bool finalized;

	/**
	 * @brief Deleted copy constructor.
	 *
	 * ZstdCompressor should not be copy-constructed.
	 */
	ZstdCompressor(const ZstdCompressor& cpy) = delete;

	/**
	 * @brief Deleted copy assignment operator.
	 *
	 * ZstdCompressor should not be copy-assigned.
	 */
	ZstdCompressor& operator=(const ZstdCompressor& cpy) = delete;
protected:
	/**
	 * @brief Processes the input buffer and writes the compressed data to the output buffer.
	 *
	 * This function compresses the data in the input buffer and writes the compressed data to the output buffer.
	 *
	 * @return Number of bytes written to the output buffer.
	 */
	size_t processInBufferToOutBuffer() override;

	/**
	 * @brief Writes a prefix before writing compressed data to the underlying Device.
	 *
	 * This function is currently a no-op and returns 0.
	 *
	 * @param device Reference to the underlying Device.
	 * @return 0, indicating no prefix is written.
	 */
	size_t writePrefix(Device& device) override;

	/**
	 * @brief Writes a postfix after writing compressed data to the underlying Device.
	 *
	 * This function is currently a no-op and returns 0.
	 *
	 * @param device Reference to the underlying Device.
	 * @return 0, indicating no postfix is written.
	 */
	size_t writePostfix(Device& device) override;

public:
	/**
	 * @brief Constructor for ZstdCompressor.
	 *
	 * Constructs a ZstdCompressor with the specified underlying Device.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 */
	ZstdCompressor(Device* underlyingDevice);

	/**
	 * @brief Destructor for ZstdCompressor.
	 *
	 * Finalizes the compression and releases resources.
	 */
	~ZstdCompressor();


	/**
	 * @brief Move constructor.
	 *
	 * Constructs a ZstdCompressor by moving the contents of another ZstdCompressor.
	 *
	 * @param mov The ZstdCompressor to move from.
	 */
	ZstdCompressor(ZstdCompressor&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Assigns the contents of another ZstdCompressor to this one by moving its resources.
	 *
	 * @param mov The ZstdCompressor to move from.
	 * @return A reference to this ZstdCompressor.
	 */
	ZstdCompressor& operator=(ZstdCompressor&& mov);

	/**
	 * @brief Sets the compression level for the Zstandard compressor.
	 *
	 * @param value Compression level as a floating-point value between 0 and 1, inclusive.
	 */
	void setCompressionLevel(float value);

	/**
	 * @brief Enables or disables the checksum for the Zstandard compressor.
	 *
	 * @param value True to enable checksum, false to disable.
	 */
	void setChecksum(bool value);

	/**
	 * @brief Finalizes the compression and writes the remaining data to the underlying Device.
	 *
	 * This function should be called before destroying the ZstdCompressor to ensure all data is flushed.
	 */
	void onFinish() override;
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ZstdDecompressor)
/**
 * @brief Zstandard decompression proxy device for an underlying Device.
 *
 * The ZstdDecompressor class is a buffered read proxy device that provides Zstandard decompression for data
 * read from an underlying Device. It decompresses data in blocks and provides the decompressed data to the
 * caller. This class is useful for efficiently decompressing data while reading from files or other devices.
 */
class MH_EUPH_API ZstdDecompressor : public Elv::Io::BufferedReadDeviceProxy {
private:
	/**
	 * @brief Handle to the Zstandard decompression context.
	 */
	void* handle;
	/**
	 * @brief Deleted copy constructor.
	 *
	 * ZstdDecompressor should not be copy-constructed.
	 */
	ZstdDecompressor(const ZstdDecompressor& cpy) = delete;

	/**
	 * @brief Deleted copy assignment operator.
	 *
	 * ZstdDecompressor should not be copy-assigned.
	 */
	ZstdDecompressor& operator=(const ZstdDecompressor& cpy) = delete;

protected:
	/**
	 * @brief Processes the input buffer and writes the decompressed data to the output buffer.
	 *
	 * This function decompresses the data in the input buffer and writes the decompressed data to the output buffer.
	 *
	 * @param bytesRead Number of bytes read from the underlying Device.
	 * @return Number of bytes written to the output buffer.
	 */
	size_t processInBufferToOutBuffer(size_t bytesRead) override;

public:
	/**
	 * @brief Constructor for ZstdDecompressor.
	 *
	 * Constructs a ZstdDecompressor with the specified underlying Device.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 */
	ZstdDecompressor(Device* underlyingDevice);

	/**
	 * @brief Destructor for ZstdDecompressor.
	 *
	 * Releases resources associated with the Zstandard decompression context.
	 */
	~ZstdDecompressor();

	/**
	 * @brief Move constructor.
	 *
	 * Constructs a ZstdDecompressor by moving the contents of another ZstdCompressor.
	 *
	 * @param mov The ZstdDecompressor to move from.
	 */
	ZstdDecompressor(ZstdDecompressor&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Assigns the contents of another ZstdDecompressor to this one by moving its resources.
	 *
	 * @param mov The ZstdDecompressor to move from.
	 * @return A reference to this ZstdCompressor.
	 */
	ZstdDecompressor& operator=(ZstdDecompressor&& mov);
};

}
}
#endif // EUPHZSTD_H
