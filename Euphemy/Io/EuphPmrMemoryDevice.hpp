#ifndef EUPHPMRMEMORYDEVICE_HPP
#define EUPHPMRMEMORYDEVICE_HPP
#include <vector>
#include <memory_resource>
#include <cstring>
#include <algorithm>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
namespace Euph {
namespace Io {

/**
 * @brief In-memory implementation of Device using a polymorphic memory resource vector.
 */
class MH_EUPH_API PmrMemoryDevice : public Elv::Io::Device {
private:
	std::pmr::vector<std::byte> m_data;
	size_t m_position;
	Elv::Io::Mode m_mode;
	bool m_valid;

	// Helper to check bitwise enum flags
	bool hasFlag(Elv::Io::Mode flag) const;

public:
	/**
	 * @brief Constructs a new Memory Device.
	 * * @param mode Access mode for the device.
	 * @param resource Pointer to the polymorphic memory resource to use for allocations.
	 */
	PmrMemoryDevice(Elv::Io::Mode mode, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
	/**
	 * @brief Constructs a new Memory Device.
	 * * @param mode Access mode for the device.
	 * * @param moved_data Vector whose ownership is acquired.
	 */
	PmrMemoryDevice(Elv::Io::Mode mode, std::pmr::vector<std::byte>&& moved_data);

	// Delete copy semantics as device handles are typically unique or shared via pointers
	PmrMemoryDevice(const PmrMemoryDevice&) = delete;
	PmrMemoryDevice& operator=(const PmrMemoryDevice&) = delete;

	// Allow move semantics
	/**
	 * @brief Move constructor.
	 * @param mov Source device to move from.
	 */
	PmrMemoryDevice(PmrMemoryDevice&& mov) noexcept;
	/**
	 * @brief Move assignment operator.
	 * @param mov Source device to move from.
	 * @return Reference to this device.
	 */
	PmrMemoryDevice& operator=(PmrMemoryDevice&& mov) noexcept;

	~PmrMemoryDevice() override;

	/**
	 * @brief Reads elements from the internal memory buffer.
	 * @param buffer Destination memory.
	 * @param size Size of each element.
	 * @param count Number of elements to read.
	 * @return Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes elements into the internal memory buffer.
	 * @param buffer Source memory.
	 * @param size Size of each element.
	 * @param count Number of elements to write.
	 * @return Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Moves the current cursor position.
	 * @param offset Byte offset relative to @p whence.
	 * @param whence Seek origin.
	 * @return `0` on success, non-zero on error.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Returns the current cursor position.
	 * @return Current byte offset.
	 */
	long tell() override;

	/**
	 * @brief Returns the current size of the memory buffer.
	 * @return Buffer size in bytes.
	 */
	size_t size() override;

	/**
	 * @brief Checks whether the cursor reached end of buffer.
	 * @return `true` when at or beyond end-of-buffer.
	 */
	bool eof() override;

	/**
	 * @brief Returns the open mode of this device.
	 * @return Device mode flags.
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes buffered writes.
	 * @return `true` because writes are immediately applied in memory.
	 */
	bool flush() override;

	/**
	 * @brief Reports whether this device is in a valid state.
	 * @return `true` when initialized correctly.
	 */
	bool isValid() const override;

	// --- Specific to MemoryDevice ---

	/**
	 * @brief Direct read-only access to the underlying memory buffer.
	 */
	const std::pmr::vector<std::byte>& getBuffer() const;
	/**
	 * @brief Direct read-and-write access to the underlying memory buffer.
	 */
	std::pmr::vector<std::byte>& getBuffer();
	/**
	 * @brief Yields ownership of the underlying memory buffer. Warning - DO NOT USE DEVICE AFTERWARDS!
	 */
	void yield(std::pmr::vector<std::byte>& target);
};

} // namespace Io
} // namespace Euph
#endif // EUPHPMRMEMORYDEVICE_HPP
