#ifndef EUPHNULLDEVICE_HPP
#define EUPHNULLDEVICE_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
namespace Euph {
namespace Io {

/**
 * @class NullDevice
 * @brief A null implementation of the Elv::Io::Device interface.
 *
 * This class provides a mock device that does not perform any actual I/O operations.
 * It is useful for testing, debugging, or situations where a device is required but no actual I/O is needed.
 *
 * @inherits Elv::Io::Device
 */
class MH_EUPH_API NullDevice : public Elv::Io::Device
{
public:
	/**
	 * @brief Default constructor for NullDevice.
	 *
	 * Initializes the null device in a valid state, ready for use.
	 */
	NullDevice();

	// Device interface implementation
public:
	/**
	 * @copydoc Elv::Io::Device::read
	 *
	 * @note In NullDevice, this function simply zeroes out the provided buffer and returns the requested count.
	 *	   No actual data is read from any source.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @copydoc Elv::Io::Device::write
	 *
	 * @note In NullDevice, this function ignores the input data and returns 0, indicating no data was written.
	 *	   No actual data is written to any destination.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @copydoc Elv::Io::Device::seek
	 *
	 * @note In NullDevice, this function does not alter the device's state and always returns success (0).
	 *	   The device's position is effectively always at the beginning (offset 0).
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @copydoc Elv::Io::Device::tell
	 *
	 * @note In NullDevice, this function always returns 0, indicating the device's position is at the beginning.
	 */
	long tell() override;

	/**
	 * @copydoc Elv::Io::Device::size
	 *
	 * @note In NullDevice, this function always returns 0, indicating the device has no content.
	 */
	size_t size() override;

	/**
	 * @copydoc Elv::Io::Device::eof
	 *
	 * @note In NullDevice, this function always returns false, indicating the end of the "file" is never reached.
	 */
	bool eof() override;

	/**
	 * @copydoc Elv::Io::Device::getMode
	 *
	 * @note In NullDevice, this function always returns Elv::Io::Mode::READ, even though no actual reading is possible.
	 *	   This is to maintain a consistent, albeit futile, device mode.
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @copydoc Elv::Io::Device::flush
	 *
	 * @note In NullDevice, this function always returns true, indicating a successful (but meaningless) flush operation.
	 */
	bool flush() override;

	/**
	 * @copydoc Elv::Io::Device::isValid
	 *
	 * @note In NullDevice, this function always returns true, indicating the device is always in a valid state.
	 */
	bool isValid() const override;
};

}
}
#endif // EUPHNULLDEVICE_HPP
