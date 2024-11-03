#ifndef EUPHBUFFERDEVICE_HPP
#define EUPHBUFFERDEVICE_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstring>
#include <vector>
#include <Elvavena/Util/ElvChunkyArray.hpp>
namespace Euph {
namespace Io {

/**
 * @brief A performant, read-only wrapper around fixed-size memory buffers.
 *
 * **Warning:** This class does not perform bounds checking for performance reasons.
 * Use with caution and ensure that buffer access is properly managed to prevent
 * out-of-bounds errors.
 *
 * @inherits Elv::Io::Device
 */
class ReadOnlyBufferWrapper : public Elv::Io::Device {
private:
	/**
	 * @brief Pointer to the beginning of the read-only buffer.
	 */
	const std::byte* ptr;

	/**
	 * @brief Current read cursor position within the buffer.
	 */
	intptr_t cursor;

	/**
	 * @brief Total size of the buffer (used for size() and internal tracking).
	 */
	const size_t buffSize;

public:
	/**
	 * @brief Constructs a ReadOnlyBufferWrapper from a raw pointer and buffer size.
	 *
	 * @param ptr      Raw pointer to the beginning of the read-only buffer.
	 * @param buffSize Total size of the buffer in bytes.
	 */
	ReadOnlyBufferWrapper(const void* ptr, size_t buffSize);

	/**
	 * @brief Constructs a ReadOnlyBufferWrapper from a std::span representing the buffer.
	 *
	 * @param span std::span containing the read-only buffer.
	 */
	ReadOnlyBufferWrapper(const std::span<const std::byte>& span);

	/**
	 * @brief Reads data from the buffer into a provided buffer.
	 *
	 * @param buffer   Pointer to the buffer to fill with data from the wrapped buffer.
	 * @param size     Size of each element to read.
	 * @param count    Number of elements to read.
	 * @return         Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Attempts to write data to the buffer (always fails, as the buffer is read-only).
	 *
	 * @param buffer   Pointer to the buffer containing data to write (ignored).
	 * @param size     Size of each element to write (ignored).
	 * @param count    Number of elements to write (ignored).
	 * @return         0, indicating failure (write operation not supported).
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Seeks to a position within the buffer.
	 *
	 * @param offset   Number of bytes to offset from the origin.
	 * @param whence   Origin from which to seek (see Elv::Io::SeekOrigin).
	 * @return         0 upon success, non-zero on failure.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current read position within the buffer.
	 *
	 * @return Current position in bytes from the beginning of the buffer.
	 */
	long tell() override;

	/**
	 * @brief Retrieves the total size of the wrapped buffer.
	 *
	 * @return Total size of the buffer in bytes.
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the buffer has been reached.
	 *
	 * @return True if the end of the buffer has been reached, false otherwise.
	 */
	bool eof() override;

	/**
	 * @brief Retrieves the current access mode (always read-only).
	 *
	 * @return Elv::Io::Mode::READ, indicating read-only access.
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes the buffer (no-op, as this is a read-only wrapper).
	 *
	 * @return True, indicating a successful no-op.
	 */
	bool flush() override;

	/**
	 * @brief Checks if the wrapper is in a valid state.
	 *
	 * @return Always true.
	 */
	bool isValid() override;
};


/**
 * @brief A performant wrapper around fixed-size, writable memory buffers.
 *
 * **Warning:** This class does not perform bounds checking for performance reasons.
 * Use with caution and ensure that buffer access is properly managed to prevent
 * out-of-bounds errors.
 *
 * @inherits Elv::Io::Device
 */
class StaticBufferWrapper : public Elv::Io::Device {
private:
	/**
	 * @brief Pointer to the beginning of the writable buffer.
	 */
	std::byte* ptr;

	/**
	 * @brief Current write/read cursor position within the buffer.
	 */
	intptr_t cursor;

	/**
	 * @brief Total size of the buffer (used for size() and internal tracking).
	 */
	const size_t buffSize;

public:
	/**
	 * @brief Constructs a StaticBufferWrapper from a raw pointer and buffer size.
	 *
	 * @param ptr      Raw pointer to the beginning of the writable buffer.
	 * @param buffSize Total size of the buffer in bytes.
	 */
	StaticBufferWrapper(void* ptr, size_t buffSize);

	/**
	 * @brief Reads data from the buffer into a provided buffer.
	 *
	 * @param buffer   Pointer to the buffer to fill with data from the wrapped buffer.
	 * @param size     Size of each element to read.
	 * @param count    Number of elements to read.
	 * @return         Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data from a provided buffer to the wrapped buffer.
	 *
	 * @param buffer   Pointer to the buffer containing data to write.
	 * @param size     Size of each element to write.
	 * @param count    Number of elements to write.
	 * @return         Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Seeks to a position within the buffer.
	 *
	 * @param offset   Number of bytes to offset from the origin.
	 * @param whence   Origin from which to seek (see Elv::Io::SeekOrigin).
	 * @return         0 upon success, non-zero on failure.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current read/write position within the buffer.
	 *
	 * @return Current position in bytes from the beginning of the buffer.
	 */
	long tell() override;

	/**
	 * @brief Retrieves the total size of the wrapped buffer.
	 *
	 * @return Total size of the buffer in bytes.
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the buffer has been reached.
	 *
	 * @return True if the end of the buffer has been reached, false otherwise.
	 */
	bool eof() override;

	/**
	 * @brief Retrieves the current access mode (read-write).
	 *
	 * @return Elv::Io::Mode::READ_WRITE, indicating read-write access.
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes the buffer (no-op, as this is an in-memory wrapper).
	 *
	 * @return True, indicating a successful no-op.
	 */
	bool flush() override;

	/**
	 * @brief Checks if the wrapper is in a valid state.
	 *
	 * @return Always true.
	 */
	bool isValid() override;
};

/**
 * @brief A dynamic, in-memory buffer class that adapts to large writes, inheriting from Elv::Io::Device.
 *
 * This class provides a flexible array buffer for reading and writing data, automatically resizing to accommodate large writes.
 * It utilizes a std::vector as its underlying storage, ensuring efficient memory management.
 *
 * @tparam LargeWriteThreshold The minimum capacity increase (in bytes) when a large write exceeds current capacity. Default: 128.
 * @tparam Alloc The allocator type used for the underlying std::vector. Default: std::allocator<std::byte>.
 */
template <size_t LargeWriteThreshold = 128, typename Alloc = std::allocator<std::byte>>
requires Elv::Util::Allocator<Alloc, std::byte>
class FlexibleArrayBuffer : public Elv::Io::Device {
public:
	/**
	 * @brief The threshold value for large writes, as specified by the template parameter.
	 */
	static constexpr const size_t threshold = LargeWriteThreshold;

	/**
	 * @brief Type alias for the underlying storage vector.
	 */
	typedef std::vector<std::byte, Alloc> Storage;

private:
	/**
	 * @brief The underlying storage for the buffer's data.
	 */
	Storage storage;

	/**
	 * @brief The current cursor position within the buffer.
	 */
	intptr_t cursor;

public:
	/**
	 * @brief Default constructor, initializing the cursor to the beginning of the buffer.
	 */
	FlexibleArrayBuffer()
		: cursor(0)
	{

	}

	/**
	 * @brief Reads data from the buffer into a provided buffer.
	 *
	 * @param buffer Pointer to the buffer to fill with data from the FlexibleArrayBuffer.
	 * @param size   Size of each element to read.
	 * @param count  Number of elements to read.
	 * @return       Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override
	{
		const size_t bytesToRead = size * count;
		std::memcpy(buffer, &storage[cursor], bytesToRead);
		cursor += bytesToRead;
		return count;
	}

	/**
	 * @brief Writes data from a provided buffer into the FlexibleArrayBuffer.
	 *
	 * Automatically resizes the buffer if the write operation exceeds its current capacity.
	 *
	 * @param buffer Pointer to the buffer containing data to write.
	 * @param size   Size of each element to write.
	 * @param count  Number of elements to write.
	 * @return       Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override
	{
		const size_t bytesToWrite = size * count;
		const size_t writtenOffset = static_cast<size_t>(cursor) + bytesToWrite;
		if (writtenOffset > storage.capacity()) storage.reserve(std::max(writtenOffset, storage.capacity() + threshold));
		if (writtenOffset > storage.size()) storage.resize(writtenOffset);
		std::memcpy(&storage[cursor], buffer, bytesToWrite);
		return count;
	}

	/**
	 * @brief Seeks to a position within the buffer.
	 *
	 * @param offset  Number of bytes to offset from the origin.
	 * @param whence  Origin from which to seek (beginning, current position, or end).
	 * @return        0 upon success, non-zero on failure.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override
	{
		switch (whence) {
			case Elv::Io::SeekOrigin::SET:
				cursor = offset;
				break;
			case Elv::Io::SeekOrigin::CUR:
				cursor += offset;
				break;
			case Elv::Io::SeekOrigin::END:
				cursor = static_cast<intptr_t>(storage.size()) + offset;
				break;
		}
		return 0;
	}

	/**
	 * @brief Retrieves the current cursor position within the buffer.
	 *
	 * @return Current position in bytes from the beginning of the buffer.
	 */
	long tell() override
	{
		return static_cast<long>(cursor);
	}

	/**
	 * @brief Retrieves the total size of the buffer's data.
	 *
	 * @return Current size of the buffer in bytes.
	 */
	size_t size() override
	{
		return storage.size();
	}

	/**
	 * @brief Checks if the end of the buffer has been reached.
	 *
	 * @return True if the end of the buffer has been reached, false otherwise.
	 */
	bool eof() override
	{
		return cursor == static_cast<intptr_t>(storage.size());
	}

	/**
	 * @brief Retrieves the current access mode of the buffer.
	 *
	 * Always returns READ_WRITE, as this buffer supports both reading and writing.
	 *
	 * @return The current access mode (always Elv::Io::Mode::READ_WRITE).
	 */
	Elv::Io::Mode getMode() const override
	{
		return Elv::Io::Mode::READ_WRITE;
	}

	/**
	 * @brief Flushes the buffer to ensure written data is committed.
	 *
	 * Since this is an in-memory buffer, flushing is always successful.
	 *
	 * @return True, indicating a successful flush operation.
	 */
	bool flush() override
	{
		return true;
	}

	/**
	 * @brief Checks if the buffer is in a valid state.
	 *
	 * Always returns true, as this buffer does not have an invalid state.
	 *
	 * @return True, indicating the buffer is valid.
	 */
	bool isValid() override
	{
		return true;
	}

	/**
	 * @brief Provides direct access to the underlying storage vector.
	 *
	 * @return Reference to the storage vector.
	 */
	Storage& getStorage() { return storage; }

	/**
	 * @brief Provides const access to the underlying storage vector.
	 *
	 * @return Const reference to the storage vector.
	 */
	const Storage& getStorage() const { return storage; }

	/**
	 * @brief Transfers ownership of the underlying storage to another vector.
	 *
	 * Resets the cursor to the beginning after transferring the storage.
	 *
	 * @param takingStorage The vector to receive the transferred storage.
	 */
	void yield(Storage& takingStorage) {
		takingStorage = std::move(storage);
		cursor = 0;
	}
};

/**
 * @brief Template class for a unique, chunked array buffer, inheriting from Elv::Io::Device.
 *
 * This class provides a dynamic, chunked storage solution for reading and writing data in a buffered manner.
 * It utilizes a unique, chunky array (ChunkedStorage) to manage memory efficiently.
 *
 * @tparam ChunkSize  The size of each chunk in bytes (default is 256).
 * @tparam Alloc      The allocator type used for managing ChunkedStorage (default is std::allocator).
 */
template<size_t ChunkSize = 256, class Alloc = std::allocator<std::array<std::byte,ChunkSize>>>
requires Elv::Util::Allocator<Alloc, std::array<std::byte,ChunkSize>>
class UniqueChunkedArrayBuffer : public Elv::Io::Device {
public:
	/**
	 * @brief Typedef for the underlying chunked storage container.
	 */
	typedef Elv::Util::UniqueChunkyArray<std::byte,ChunkSize,Alloc> ChunkedStorage;

private:
	/**
	 * @brief The chunked storage container for holding the buffered data.
	 */
	ChunkedStorage storage;

	/**
	 * @brief The current cursor position within the storage, in bytes.
	 */
	intptr_t cursor;

public:
	/**
	 * @brief Default constructor, initializing the cursor to the beginning of the storage.
	 */
	UniqueChunkedArrayBuffer() : storage(1), cursor(0) {}

	/**
	 * @brief Reads data from the buffer into a provided memory location.
	 *
	 * @param buffer  Pointer to the buffer to fill with data from the storage.
	 * @param size    Size of each element to read.
	 * @param count   Number of elements to read.
	 * @return        Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override
	{
		const size_t bytesToRead = size * count;
		std::memcpy(buffer, &storage[cursor], bytesToRead);
		cursor += bytesToRead;
		return count;
	}

	/**
	 * @brief Writes data from a provided memory location into the buffer.
	 *
	 * If the write operation exceeds the current storage size, the storage is dynamically resized.
	 *
	 * @param buffer  Pointer to the buffer containing data to write into the storage.
	 * @param size    Size of each element to write.
	 * @param count   Number of elements to write.
	 * @return        Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override
	{
		const size_t bytesToWrite = size * count;
		const size_t writtenOffset = static_cast<size_t>(cursor) + bytesToWrite;
		if (writtenOffset > storage.size()) storage.resize(writtenOffset);
		std::memcpy(&storage[cursor], buffer, bytesToWrite);
		cursor += bytesToWrite; // Added cursor update for write operation
		return count;
	}

	/**
	 * @brief Seeks to a specified position within the buffer.
	 *
	 * @param offset  Number of bytes to offset from the origin.
	 * @param whence  Origin from which to seek (beginning, current position, or end).
	 * @return        0 upon success, non-zero on failure.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override
	{
		switch (whence) {
			case Elv::Io::SeekOrigin::SET:
				cursor = offset;
				break;
			case Elv::Io::SeekOrigin::CUR:
				cursor += offset;
				break;
			case Elv::Io::SeekOrigin::END:
				cursor = static_cast<intptr_t>(storage.size()) + offset;
				break;
		}
		return 0;
	}

	/**
	 * @brief Retrieves the current position within the buffer.
	 *
	 * @return Current position in bytes from the beginning of the storage.
	 */
	long tell() override
	{
		return static_cast<long>(cursor);
	}

	/**
	 * @brief Retrieves the total size of the buffer.
	 *
	 * @return Current size of the storage in bytes.
	 */
	size_t size() override
	{
		return storage.size();
	}

	/**
	 * @brief Checks if the end of the buffer has been reached.
	 *
	 * @return True if the end of the storage has been reached, false otherwise.
	 */
	bool eof() override
	{
		return cursor == static_cast<intptr_t>(storage.size());
	}

	/**
	 * @brief Retrieves the current access mode of the buffer.
	 *
	 * This implementation always returns READ_WRITE mode.
	 *
	 * @return The current access mode (always Elv::Io::Mode::READ_WRITE for this class).
	 */
	Elv::Io::Mode getMode() const override
	{
		return Elv::Io::Mode::READ_WRITE;
	}

	/**
	 * @brief Flushes the buffer to ensure written data is committed.
	 *
	 * Since this is a memory-based buffer, flushing is always successful.
	 *
	 * @return True, indicating the flush operation was successful.
	 */
	bool flush() override
	{
		return true;
	}

	/**
	 * @brief Checks if the buffer device is in a valid state.
	 *
	 * This implementation always returns true, assuming the device is valid by default.
	 *
	 * @return True, indicating the device is in a valid state.
	 */
	bool isValid() override
	{
		return true;
	}

	/**
	 * @brief Provides direct access to the underlying chunked storage container.
	 *
	 * @return Reference to the ChunkedStorage instance.
	 */
	ChunkedStorage& getStorage() { return storage; }

	/**
	 * @brief Provides const access to the underlying chunked storage container.
	 *
	 * @return Const reference to the ChunkedStorage instance.
	 */
	const ChunkedStorage& getStorage() const { return storage; }

	/**
	 * @brief Transfers ownership of the underlying chunked storage container.
	 *
	 * After calling this method, the UniqueChunkedArrayBuffer instance is left in an unspecified state.
	 *
	 * @return Rvalue reference to the ChunkedStorage instance, transferring ownership.
	 */
	ChunkedStorage&& yieldStorage() { return std::move(storage); }
};

/**
 * @brief A buffered file device implementation using a shared, chunked array.
 *
 * This class provides a buffered file device using a shared, chunked array for storage.

 * @tparam ChunkSize The size of each chunk in bytes (default is 256).
 * @tparam Alloc    The allocator type used for the chunked array (default is std::allocator<std::array<std::byte,ChunkSize>>).
 */
template<size_t ChunkSize = 256, class Alloc = std::allocator<std::array<std::byte,ChunkSize>>>
requires Elv::Util::Allocator<Alloc, std::array<std::byte,ChunkSize>>
class SharedChunkedArrayBuffer : public Elv::Io::Device {
public:
	/**
	 * @brief Type alias for the underlying chunked storage.
	 */
	typedef Elv::Util::SharedChunkyArray<std::byte,ChunkSize,Alloc> ChunkedStorage;

private:
	/**
	 * @brief The underlying chunked storage.
	 */
	ChunkedStorage storage;

	/**
	 * @brief The current read/write cursor.
	 */
	intptr_t cursor;

public:
	/**
	 * @brief Default constructor initializing the cursor.
	 */
	SharedChunkedArrayBuffer() : storage(1), cursor(0) {}

	/**
	 * @brief Reads data from the buffer into a user-provided buffer.
	 *
	 * @param buffer   Pointer to the buffer to fill with data from the file.
	 * @param size     Size of each element to read.
	 * @param count    Number of elements to read.
	 * @return         Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override {
		const size_t bytesToRead = size * count;
		std::memcpy(buffer, &storage[cursor], bytesToRead);
		cursor += bytesToRead;
		return count;
	}

	/**
	 * @brief Writes data from a user-provided buffer to the file.
	 *
	 * @param buffer   Pointer to the buffer containing data to write.
	 * @param size     Size of each element to write.
	 * @param count    Number of elements to write.
	 * @return         Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override {
		const size_t bytesToWrite = size * count;
		const size_t writtenOffset = static_cast<size_t>(cursor) + bytesToWrite;
		if (writtenOffset > storage.size()) storage.resize(writtenOffset);
		std::memcpy(&storage[cursor], buffer, bytesToWrite);
		cursor += bytesToWrite; // Added cursor update for write operation
		return count;
	}

	/**
	 * @brief Seeks to a position in the file.
	 *
	 * @param offset   Number of bytes to offset from the origin.
	 * @param whence Origin from which to seek (beginning, current, or end).
	 * @return 0 upon success, non-zero on failure.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override {
		switch (whence) {
			case Elv::Io::SeekOrigin::SET:
				cursor = offset;
				break;
			case Elv::Io::SeekOrigin::CUR:
				cursor += offset;
				break;
			case Elv::Io::SeekOrigin::END:
				cursor = static_cast<intptr_t>(storage.size()) + offset;
				break;
		}
		return 0;
	}

	/**
	 * @brief Returns the current file position.
	 *
	 * @return Current position in the file in bytes from the beginning.
	 */
	long tell() override {
		return static_cast<long>(cursor);
	}

	/**
	 * @brief Returns the size of the file.
	 *
	 * @return Size of the file in bytes.
	 */
	size_t size() override {
		return storage.size();
	}

	/**
	 * @brief Checks if the end of the file has been reached.
	 *
	 * @return True if the end of the file has been reached, false otherwise.
	 */
	bool eof() override {
		return cursor == static_cast<intptr_t>(storage.size());
	}

	/**
	 * @brief Returns the current file access mode.
	 *
	 * @return The mode in which the file was opened (e.g., read, write, append, or a combination).
	 */
	Elv::Io::Mode getMode() const override {
		return Elv::Io::Mode::READ_WRITE;
	}

	/**
	 * @brief Flushes the file buffer to ensure written data is committed.
	 *
	 * @return True if the flush operation was successful,false otherwise.
	 */
	bool flush() override {
		return true;
	}

	/**
	 * @brief Checks if the file device is in a valid state.
	 *
	 * @return True if the file device is valid,false otherwise.
	 */
	bool isValid() override {
		// TODO: Implement actual validation logic here
		return true; // Placeholder for actual implementation
	}

	/**
	 * @brief Provides access to the underlying chunked storage.
	 *
	 * @note This is not const-correct since the storage is modifiable.
	 */
	ChunkedStorage& getStorage() { return storage; }

	/**
	 * @brief Provides const access to the underlying chunked storage.
	 *
	 * @note This is not const-correct since the storage is modifiable.
	 */
	const ChunkedStorage& getStorage() const { return storage; }

	/**
	 * @brief Transfers ownership of the underlying chunked storage container.
	 *
	 * After calling this method, the UniqueChunkedArrayBuffer instance is left in an unspecified state.
	 *
	 * @return Rvalue reference to the ChunkedStorage instance, transferring ownership.
	 */
	ChunkedStorage&& yieldStorage() { return std::move(storage); }
};


}
}
#endif // EUPHBUFFERDEVICE_HPP
