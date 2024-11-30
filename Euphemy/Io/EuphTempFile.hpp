#ifndef EUPHTEMPFILE_HPP
#define EUPHTEMPFILE_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Io/EuphMemoryMapped.hpp>
#include <Euphemy/Io/EuphPlatformDependentFileBase.hpp>
namespace Euph {
namespace Io {

/**
 * @class TempFile
 * @brief Represents a temporary file that supports basic file operations.
 *
 * The TempFile class creates and manages a temporary file that is automatically deleted
 * when the object is destroyed. This class is platform-independent and works on both
 * Windows and Unix-like systems.
 */
class MH_EUPH_API TempFile : public Elv::Io::Device
{
private:
	/**
	 * @var fileHandle
	 * @brief Handle to the file.
	 */
	PlatformDependentFileBase fileHandle;

	TempFile(const TempFile& cpy) = delete;
	TempFile& operator=(const TempFile& cpy) = delete;

public:
	/**
	 * @brief Constructs a TempFile and creates a temporary file.
	 *
	 * Throws an exception if the temporary file cannot be created.
	 */
	TempFile(TemporaryFileCreationMode creationMode = TemporaryFileCreationMode::MKSTEMP, const char* npath = nullptr);

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
class MH_EUPH_API MemoryMappedTempFile : public MemoryMapped
{
private:
	/**
	 * @var fileHandle
	 * @brief Handle to the file.
	 */
	PlatformDependentFileBase fileHandle;

	/**
	 * @name Disabled copy constructor and assignment operator
	 * @{
	 */
	MemoryMappedTempFile(const MemoryMappedTempFile& cpy) = delete;
	MemoryMappedTempFile& operator=(const MemoryMappedTempFile& cpy) = delete;
	/// @}
protected:
	/**
	 * @brief Gets the platform-dependent basic file handle.
	 *
	 * @return The platform-dependent basic file handle.
	 */
	PlatformDependentFileHandleBase& getFileHandle() override;
	/**
	 * @brief Gets the platform-dependent basic file handle.
	 *
	 * @return The platform-dependent basic file handle.
	 */
	const PlatformDependentFileHandleBase& getFileHandle() const override;
public:
	/**
	 * @brief Constructs a MemoryMappedTempFile with the specified size.
	 *
	 * Creates a temporary file of the given size, maps it into memory, and sets up the object for access.
	 *
	 * @param fileSize The size of the temporary file to create.
	 * @throws std::runtime_error or std::system_error if file creation, mapping, or setup fails.
	 */
	MemoryMappedTempFile(size_t fileSize, TemporaryFileCreationMode creationMode = TemporaryFileCreationMode::MKSTEMP, const char* npath = nullptr);

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
	/**
	 * @brief Gets whether this memory-mapping is read-only or not.
	 *
	 * @return Whether the memory-mapping is read-only or not.
	 */
	bool readOnly() const override;
};

}
}

#endif // EUPHTEMPFILE_HPP
