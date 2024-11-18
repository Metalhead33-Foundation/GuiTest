#ifndef EUPHTEMPFILE_HPP
#define EUPHTEMPFILE_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Io/EuphMemoryMapped.hpp>
namespace Euph {
namespace Io {

struct PlatformDependentFileHandleBase {
public:
#ifdef _WIN32
	HANDLE fileHandle = INVALID_HANDLE_VALUE; ///< Handle for the temporary file on Windows.
#else
	int fileDescriptor = -1; ///< File descriptor for the temporary file on Unix-like systems.
#endif
	std::string filePath; ///< Path of the temporary file.
private:
	PlatformDependentFileHandleBase(const PlatformDependentFileHandleBase& cpy) = delete;
	PlatformDependentFileHandleBase& operator=(const PlatformDependentFileHandleBase& cpy) = delete;
public:
	PlatformDependentFileHandleBase(size_t fileSize = 0);
	PlatformDependentFileHandleBase(PlatformDependentFileHandleBase&& mov);
	PlatformDependentFileHandleBase& operator=(PlatformDependentFileHandleBase&& mov) = delete;
};

/**
 * @class TempFile
 * @brief Represents a temporary file that supports basic file operations.
 *
 * The TempFile class creates and manages a temporary file that is automatically deleted
 * when the object is destroyed. This class is platform-independent and works on both
 * Windows and Unix-like systems.
 */
class TempFile : public Elv::Io::Device
{
private:
#ifdef _WIN32
	HANDLE fileHandle = INVALID_HANDLE_VALUE; ///< Handle for the temporary file on Windows.
#else
	int fileDescriptor = -1; ///< File descriptor for the temporary file on Unix-like systems.
#endif
	std::string filePath; ///< Path of the temporary file.

	TempFile(const TempFile& cpy) = delete;
	TempFile& operator=(const TempFile& cpy) = delete;

public:
	/**
	 * @brief Constructs a TempFile and creates a temporary file.
	 *
	 * Throws an exception if the temporary file cannot be created.
	 */
	TempFile();

	/**
	 * @brief Destroys the TempFile and deletes the temporary file.
	 */
	~TempFile();

	/**
	 * @brief Move constructor.
	 *
	 * Transfers ownership of the temporary file to the new TempFile object.
	 * @param mov The TempFile object to move from.
	 */
	TempFile(TempFile&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Transfers ownership of the temporary file to the current TempFile object.
	 * @param mov The TempFile object to move from.
	 * @return A reference to the current TempFile object.
	 */
	TempFile& operator=(TempFile&& mov);

	/**
	 * @brief Reads data from the file.
	 *
	 * @param buffer Pointer to the buffer to store read data.
	 * @param size Size of each element to read.
	 * @param count Number of elements to read.
	 * @return Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data to the file.
	 *
	 * @param buffer Pointer to the buffer containing data to write.
	 * @param size Size of each element to write.
	 * @param count Number of elements to write.
	 * @return Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Moves the file position indicator.
	 *
	 * Sets the position indicator relative to the specified origin.
	 * @param offset Number of bytes to offset from the origin.
	 * @param whence Reference point for the offset (beginning, current position, or end).
	 * @return 0 on success, non-zero on failure.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current file position.
	 *
	 * @return The position in the file from the beginning in bytes.
	 */
	long tell() override;

	/**
	 * @brief Retrieves the total size of the file.
	 *
	 * @return The size of the file in bytes.
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the file has been reached.
	 *
	 * @return True if the end of the file has been reached, false otherwise.
	 */
	bool eof() override;

	/**
	 * @brief Retrieves the file access mode.
	 *
	 * @return The current access mode of the file (read/write).
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes the file's buffer.
	 *
	 * Ensures that all buffered data is written to the file.
	 * @return True if the flush operation is successful, false otherwise.
	 */
	bool flush() override;

	/**
	 * @brief Checks if the file is in a valid state.
	 *
	 * Indicates whether the file can be used for read/write operations.
	 * @return True if the file is valid, false otherwise.
	 */
	bool isValid() const override;
};

/**
 * @warning **THIS CLASS IS NOT A SUBCLASS OF `TempFile` AND DOES NOT IMPLEMENT `Elv::Io::Device`!**
 *          **IT IS A COMPLETELY SEPARATE CLASS, USE ACCORDINGLY!**
 *
 * @class MemoryMappedTempFile
 * @brief A class representing a temporary file mapped into memory for efficient access.
 *
 * This class provides an interface for creating, managing, and accessing a temporary file
 * that is mapped into the process's memory space, allowing for fast and efficient data manipulation.
 */
class MemoryMappedTempFile : public MemoryMapped
{
private:

	std::string filePath; ///< Path of the temporary file.

	/**
	 * @name Disabled copy constructor and assignment operator
	 * @{
	 */
	MemoryMappedTempFile(const MemoryMappedTempFile& cpy) = delete;
	MemoryMappedTempFile& operator=(const MemoryMappedTempFile& cpy) = delete;
	/// @}

public:
	/**
	 * @brief Constructs a MemoryMappedTempFile with the specified size.
	 *
	 * Creates a temporary file of the given size, maps it into memory, and sets up the object for access.
	 *
	 * @param fileSize The size of the temporary file to create.
	 * @throws std::runtime_error or std::system_error if file creation, mapping, or setup fails.
	 */
	MemoryMappedTempFile(size_t fileSize);
	/**
	 * @brief Destructor
	 */
	~MemoryMappedTempFile();

	/**
	 * @brief Move constructor for MemoryMappedTempFile.
	 *
	 * Transfers ownership of the temporary file and its mapping from another instance.
	 *
	 * @param mov The instance to move from.
	 */
	MemoryMappedTempFile(MemoryMappedTempFile&& mov);

	/**
	 * @brief Move assignment operator for MemoryMappedTempFile.
	 *
	 * Transfers ownership of the temporary file and its mapping from another instance, releasing any previously held resources.
	 *
	 * @param mov The instance to move from.
	 * @return Reference to the assigned instance.
	 */
	MemoryMappedTempFile& operator=(MemoryMappedTempFile&& mov);

	/**
	 * @brief Retrieves the path of the temporary file.
	 *
	 * @return The file path as a constant string reference.
	 */
	const std::string& getFilePath() const;
};

}
}

#endif // EUPHTEMPFILE_HPP
