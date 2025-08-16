#ifndef RANDOMDEVICE_HPP
#define RANDOMDEVICE_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#ifdef _WIN32
#include <Wincrypt.h>
#else
#include <cstdio>
#include <memory>
#endif
namespace Euph {
namespace Io {

/**
 * @brief Enumeration defining the source for random number generation.
 *
 * These values determine the underlying system resource used for generating random numbers.
 */
enum class RandomSource : uint8_t {
	/**
	 * @brief Use /dev/random or CryptGenRandom (blocking, more secure).
	 *
	 * This source is suitable for applications requiring high entropy, but may block if the system's entropy pool is depleted.
	 */
	RANDOM,

	/**
	 * @brief Use /dev/urandom or BCryptGenRandom (non-blocking, suitable for most use cases).
	 *
	 * This source provides a non-blocking alternative, suitable for most applications, but with potentially lower entropy.
	 */
	URANDOM
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(RandomDevice)
/**
 * @class RandomDevice
 * @brief A file-like device for generating random numbers.
 *
 * This class provides an interface for reading random numbers from the system, while adhering to the Elv::Io::Device API.
 * Note that certain operations (e.g., seeking, writing) are not applicable to random devices and will fail or return default values.
 */
class MH_EUPH_API RandomDevice : public Elv::Io::Device
{
private:
	/**
	 * @brief The source of random number generation.
	 */
	RandomSource source;

#ifdef _WIN32
	/**
	 * @brief Handle to the cryptographic provider (for CryptGenRandom).
	 */
	HCRYPTPROV hCryptProv_ = nullptr;
#else
	/**
	 * @brief File pointer to the random device (/dev/random or /dev/urandom).
	 */
	std::unique_ptr<FILE,decltype(&fclose)> randomFile_;
#endif

	/**
	 * @brief Disallow copying.
	 */
	RandomDevice(const RandomDevice& cpy) = delete;
	RandomDevice& operator=(const RandomDevice& cpy) = delete;

public:
	/**
	 * @brief Constructs a RandomDevice instance with the specified source.
	 *
	 * @param source The source of random number generation (RANDOM or URANDOM).
	 */
	RandomDevice(RandomSource source);

	/**
	 * @brief Destructs the RandomDevice instance, releasing system resources.
	 */
	~RandomDevice();

	/**
	 * @brief Move constructor.
	 *
	 * @param mov The instance to move from.
	 */
	RandomDevice(RandomDevice&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * @param mov The instance to move from.
	 * @return Reference to the assigned instance.
	 */
	RandomDevice& operator=(RandomDevice&& mov);

	/**
	 * @brief Reads random data from the device into a buffer.
	 *
	 * @param buffer   Pointer to the buffer to fill with random data.
	 * @param size	 Size of each element to read.
	 * @param count	Number of elements to read.
	 * @return		 Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data to the random device (not applicable, will fail).
	 *
	 * Random devices do not support writing; this operation will always fail.
	 *
	 * @param buffer   Pointer to the buffer containing data to write.
	 * @param size	 Size of each element to write.
	 * @param count	Number of elements to write.
	 * @return		 Always 0 (failure).
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Seeks to a position in the random device (not applicable, will fail).
	 *
	 * Random devices do not support seeking; this operation will always fail.
	 *
	 * @param offset   Number of bytes to offset from the origin.
	 * @param whence   Origin from which to seek (ignored).
	 * @return		 Always -1 (failure).
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current file position (always 0).
	 *
	 * Random devices do not maintain a file position.
	 *
	 * @return Always 0.
	 */
	long tell() override;

	/**
	 * @brief Retrieves the complete file size (always 0).
	 *
	 * Random devices do not have a fixed size.
	 *
	 * @return Always 0.
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the file has been reached (always false).
	 *
	 * Random devices are infinite sources of data.
	 *
	 * @return Always false.
	 */
	bool eof() override;

	/**
	 * @brief Retrieves the current file access mode (always READ).
	 *
	 * Random devices are only accessible for reading.
	 *
	 * @return Always Elv::Io::Mode::READ.
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes the file buffer (no-op, always true).
	 *
	 * Random devices do not maintain a buffer.
	 *
	 * @return Always true.
	 */
	bool flush() override;

	/**
	 * @brief Checks if the file device is in a valid state.
	 *
	 * Verifies the internal state of the random device, indicating whether it can be used for operations.
	 *
	 * @return True if the random device is valid, false otherwise.
	 */
	bool isValid() const override;
};

}
}
#endif // RANDOMDEVICE_HPP
