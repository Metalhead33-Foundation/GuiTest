#ifndef EUPHNULLDEVICE_HPP
#define EUPHNULLDEVICE_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
namespace Euph {
namespace Io {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(NullDevice)
/**
 * @class NullDevice
 * @brief A null implementation of the Elv::Io::Device interface.
 *
 * This class provides a mock device that does not perform any actual I/O operations.
 * It is useful for testing, debugging, or situations where a device is required but no actual I/O is needed.
 *
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
	 * @brief Fills the destination with zeroes and reports a successful read count.
	 * @note No external source is read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Ignores write data and reports that nothing was written.
	 * @note No destination is modified.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Accepts seek requests without changing any internal state.
	 * @return Always returns `0`.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Returns the current cursor position.
	 * @return Always `0`.
	 */
	long tell() override;

	/**
	 * @brief Returns the virtual size of this device.
	 * @return Always `0`.
	 */
	size_t size() override;

	/**
	 * @brief Reports end-of-file state.
	 * @return Always `false`.
	 */
	bool eof() override;

	/**
	 * @brief Returns the virtual mode of this device.
	 * @return Always `Elv::Io::Mode::READ`.
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Performs a no-op flush.
	 * @return Always `true`.
	 */
	bool flush() override;

	/**
	 * @brief Reports whether this device is valid.
	 * @return Always `true`.
	 */
	bool isValid() const override;
};

}
}
#endif // EUPHNULLDEVICE_HPP
