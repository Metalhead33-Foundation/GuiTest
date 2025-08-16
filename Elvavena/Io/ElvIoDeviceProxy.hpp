#ifndef ELVIODEVICEPROXY_HPP
#define ELVIODEVICEPROXY_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
namespace Elv {
namespace Io {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(BufferedDeviceProxy)
/**
 * @brief Proxy device that provides buffered read and write operations for an underlying Device.
 *
 * The BufferedDeviceProxy class is a proxy device that adds buffering capabilities to an underlying Device.
 * It maintains input and output buffers to optimize read and write operations, reducing the number of
 * direct interactions with the underlying device. This can improve performance, especially for devices
 * with high latency or small block sizes.
 *
 * @note This class is not meant to be used directly. Use derived classes such as BufferedReadDeviceProxy
 *	   and BufferedWriteDeviceProxy instead.
 */
class BufferedDeviceProxy : public Device {
private:
	/**
	 * @brief Deleted copy constructor.
	 *
	 * BufferedDeviceProxy should not be copy-constructed.
	 */
	BufferedDeviceProxy(const BufferedDeviceProxy& cpy) = delete;

	/**
	 * @brief Deleted copy assignment operator.
	 *
	 * BufferedDeviceProxy should not be copy-assigned.
	 */
	BufferedDeviceProxy& operator=(const BufferedDeviceProxy& cpy) = delete;

protected:
	/**
	 * @brief Pointer to the underlying Device.
	 */
	Device* underlyingDevice;

	/**
	 * @brief Input buffer for read operations.
	 */
	std::vector<std::byte> inBuffer;

	/**
	 * @brief Output buffer for write operations.
	 */
	std::vector<std::byte> outBuffer;

	/**
	 * @brief Current position in the buffer.
	 */
	intptr_t bufferCursor;

	/**
	 * @brief Move constructor.
	 *
	 * Constructs a BufferedDeviceProxy by moving the contents of another BufferedDeviceProxy.
	 *
	 * @param mov The BufferedDeviceProxy to move from.
	 */
	BufferedDeviceProxy(BufferedDeviceProxy&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Assigns the contents of another BufferedDeviceProxy to this one by moving its resources.
	 *
	 * @param mov The BufferedDeviceProxy to move from.
	 * @return A reference to this BufferedDeviceProxy.
	 */
	BufferedDeviceProxy& operator=(BufferedDeviceProxy&& mov);

public:
	/**
	 * @brief Constructor for BufferedDeviceProxy.
	 *
	 * Constructs a BufferedDeviceProxy with specified buffer sizes for the underlying Device.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 * @param inBufferSize Size of the input buffer in bytes.
	 * @param outBufferSize Size of the output buffer in bytes.
	 */
	BufferedDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize);

	/**
	 * @brief Seeks to a position in the open file.
	 *
	 * Delegates the seek operation to the underlying Device.
	 *
	 * @param offset Number of bytes to offset from the origin.
	 * @param whence Origin from which to seek.
	 * @return 0 upon success, non-zero on failure.
	 */
	int seek(long offset, SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current file position.
	 *
	 * Delegates the tell operation to the underlying Device.
	 *
	 * @return Current position in the file in bytes from the beginning.
	 */
	long tell() override;

	/**
	 * @brief Retrieves the complete file size.
	 *
	 * Delegates the size operation to the underlying Device.
	 *
	 * @return File size.
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the file has been reached.
	 *
	 * Delegates the eof operation to the underlying Device.
	 *
	 * @return True if the end of the file has been reached, false otherwise.
	 */
	bool eof() override;

	/**
	 * @brief Checks if the file device is in a valid state.
	 *
	 * Delegates the isValid operation to the underlying Device.
	 *
	 * @return True if the file device is valid, false otherwise.
	 */
	bool isValid() const override;

	/**
	 * @brief Retrieves the underlying Device.
	 *
	 * @return Pointer to the underlying Device.
	 */
	Device* getUnderlyingDevice() const;

	/**
	 * @brief Sets the underlying Device.
	 *
	 * @param newUnderlyingDevice Pointer to the new underlying Device.
	 */
	void setUnderlyingDevice(Device* newUnderlyingDevice);
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(BufferedWriteDeviceProxy)
/**
 * @brief Buffered write proxy device for an underlying Device.
 *
 * The BufferedWriteDeviceProxy class is a buffered write proxy for an underlying Device. It provides
 * write buffering and allows for additional processing of the data before it is written to the underlying
 * device. This class is particularly useful for optimizing write operations and adding custom data
 * transformations during the write process.
 */
class BufferedWriteDeviceProxy : public BufferedDeviceProxy {
private:
	/**
	 * @brief Deleted copy constructor.
	 *
	 * BufferedWriteDeviceProxy should not be copy-constructed.
	 */
	BufferedWriteDeviceProxy(const BufferedWriteDeviceProxy& cpy) = delete;

	/**
	 * @brief Deleted copy assignment operator.
	 *
	 * BufferedWriteDeviceProxy should not be copy-assigned.
	 */
	BufferedWriteDeviceProxy& operator=(const BufferedWriteDeviceProxy& cpy) = delete;

protected:
	/**
	 * @brief Flushes the buffer to the underlying Device.
	 *
	 * This function processes the data in the buffer and writes it to the underlying Device.
	 *
	 * @return True if the flush operation was successful, false otherwise.
	 */
	bool flushBufferToUnderlyingDevice();

	/**
	 * @brief Processes the input buffer and writes the result to the output buffer.
	 *
	 * Derived classes must implement this function to define how the input buffer is processed and
	 * written to the output buffer.
	 *
	 * @return Number of bytes written to the output buffer.
	 */
	virtual size_t processInBufferToOutBuffer() = 0;

	/**
	 * @brief Writes a prefix before writing data to the underlying Device.
	 *
	 * Derived classes can implement this function to write a custom prefix before writing data.
	 *
	 * @param device Reference to the underlying Device.
	 * @return Number of bytes written as the prefix.
	 */
	virtual size_t writePrefix(Device& device) = 0;

	/**
	 * @brief Writes a postfix after writing data to the underlying Device.
	 *
	 * Derived classes can implement this function to write a custom postfix after writing data.
	 *
	 * @param device Reference to the underlying Device.
	 * @return Number of bytes written as the postfix.
	 */
	virtual size_t writePostfix(Device& device) = 0;

	/**
	 * @brief Move constructor.
	 *
	 * Constructs a BufferedWriteDeviceProxy by moving the contents of another BufferedWriteDeviceProxy.
	 *
	 * @param mov The BufferedWriteDeviceProxy to move from.
	 */
	BufferedWriteDeviceProxy(BufferedWriteDeviceProxy&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Assigns the contents of another BufferedWriteDeviceProxy to this one by moving its resources.
	 *
	 * @param mov The BufferedWriteDeviceProxy to move from.
	 * @return A reference to this BufferedWriteDeviceProxy.
	 */
	BufferedWriteDeviceProxy& operator=(BufferedWriteDeviceProxy&& mov);

public:
	/**
	 * @brief Constructor for BufferedWriteDeviceProxy.
	 *
	 * Constructs a BufferedWriteDeviceProxy with specified buffer sizes for the underlying Device.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 * @param inBufferSize Size of the input buffer in bytes.
	 * @param outBufferSize Size of the output buffer in bytes.
	 */
	BufferedWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize);

	/**
	 * @brief Reads data from the open file into a buffer.
	 *
	 * Reads are not supported by this class and will always return 0.
	 *
	 * @param buffer Pointer to the buffer to fill with data from the file.
	 * @param size Size of each element to read.
	 * @param count Number of elements to read.
	 * @return 0, indicating no data was read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data from a buffer to the open file.
	 *
	 * Writes data to the internal buffer and may not immediately write to the underlying device.
	 * Call flush() to ensure data is written to the underlying device.
	 *
	 * @param buffer Pointer to the buffer containing data to write.
	 * @param size Size of each element to write.
	 * @param count Number of elements to write.
	 * @return Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Retrieves the current file access mode.
	 *
	 * @return WRITE mode, indicating write-only access.
	 */
	Mode getMode() const override;

	/**
	 * @brief Flushes the file buffer to ensure written data is committed.
	 *
	 * Flushes the internal buffer to the underlying device.
	 *
	 * @return True if the flush operation was successful, false otherwise.
	 */
	bool flush() override;

	/**
	 * @brief Callback function to be called when the device is finished.
	 *
	 * Derived classes must implement this function to perform any necessary cleanup or finalization.
	 */
	virtual void onFinish() = 0;
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(BufferedReadDeviceProxy)
/**
 * @brief Buffered read proxy device for an underlying Device.
 *
 * The BufferedReadDeviceProxy class is a buffered read proxy for an underlying Device. It provides
 * read buffering and allows for additional processing of the data after it is read from the underlying
 * device. This class is useful for optimizing read operations and adding custom data transformations
 * during the read process.
 */
class BufferedReadDeviceProxy : public BufferedDeviceProxy {
private:
	/**
	 * @brief Deleted copy constructor.
	 *
	 * BufferedReadDeviceProxy should not be copy-constructed.
	 */
	BufferedReadDeviceProxy(const BufferedReadDeviceProxy& cpy) = delete;

	/**
	 * @brief Deleted copy assignment operator.
	 *
	 * BufferedReadDeviceProxy should not be copy-assigned.
	 */
	BufferedReadDeviceProxy& operator=(const BufferedReadDeviceProxy& cpy) = delete;
protected:
	/**
	 * @brief Fence indicating the end of valid data in the output buffer.
	 */
	intptr_t outBufferFence;

	/**
	 * @brief Fills the buffer with data from the underlying Device.
	 *
	 * This function reads data from the underlying Device and processes it into the output buffer.
	 *
	 * @return True if the buffer was successfully filled, false otherwise.
	 */
	bool fillBuffer();

	/**
	 * @brief Processes the input buffer and writes the result to the output buffer.
	 *
	 * Derived classes must implement this function to define how the input buffer is processed and
	 * written to the output buffer.
	 *
	 * @param bytesRead Number of bytes read from the underlying Device.
	 * @return Number of bytes written to the output buffer.
	 */
	virtual size_t processInBufferToOutBuffer(size_t bytesRead) = 0;
	/**
	 * @brief Move constructor.
	 *
	 * Constructs a BufferedReadDeviceProxy by moving the contents of another BufferedReadDeviceProxy.
	 *
	 * @param mov The BufferedReadDeviceProxy to move from.
	 */
	BufferedReadDeviceProxy(BufferedReadDeviceProxy&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Assigns the contents of another BufferedReadDeviceProxy to this one by moving its resources.
	 *
	 * @param mov The BufferedReadDeviceProxy to move from.
	 * @return A reference to this BufferedReadDeviceProxy.
	 */
	BufferedReadDeviceProxy& operator=(BufferedReadDeviceProxy&& mov);

public:
	/**
	 * @brief Constructor for BufferedReadDeviceProxy.
	 *
	 * Constructs a BufferedReadDeviceProxy with specified buffer sizes for the underlying Device.
	 *
	 * @param underlyingDevice Pointer to the underlying Device.
	 * @param inBufferSize Size of the input buffer in bytes.
	 * @param outBufferSize Size of the output buffer in bytes.
	 */
	BufferedReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize);

	/**
	 * @brief Reads data from the open file into a buffer.
	 *
	 * Reads data from the underlying Device and processes it through the buffer.
	 *
	 * @param buffer Pointer to the buffer to fill with data from the file.
	 * @param size Size of each element to read.
	 * @param count Number of elements to read.
	 * @return Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data from a buffer to the open file.
	 *
	 * Writes are not supported by this class and will always return 0.
	 *
	 * @param buffer Pointer to the buffer containing data to write.
	 * @param size Size of each element to write.
	 * @param count Number of elements to write.
	 * @return 0, indicating no data was written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Retrieves the current file access mode.
	 *
	 * @return READ mode, indicating read-only access.
	 */
	Mode getMode() const override;

	/**
	 * @brief Flushes the file buffer to ensure written data is committed.
	 *
	 * Delegates the flush operation to the underlying Device.
	 *
	 * @return True if the flush operation was successful, false otherwise.
	 */
	bool flush() override;
};

}
}
#endif // ELVIODEVICEPROXY_HPP
