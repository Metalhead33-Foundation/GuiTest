#ifndef EUPHPLATFORMDEPENDENTFILEBASE_HPP
#define EUPHPLATFORMDEPENDENTFILEBASE_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstddef>
#include <span>
#ifdef _WIN32
	#include <windows.h>
#endif
#include <string>
namespace Euph {
namespace Io {

/**
 * @brief Base class for platform-dependent file handles.
 *
 * This class encapsulates the platform-specific file handle or file descriptor.
 */
class MH_EUPH_API PlatformDependentFileHandleBase {
public:
#ifdef _WIN32
	HANDLE fileHandle; ///< Handle for the file on Windows.
#else
	int fileDescriptor; ///< File descriptor for the file on Unix-like systems.
#endif

private:
	PlatformDependentFileHandleBase(const PlatformDependentFileHandleBase& cpy) = delete;
	PlatformDependentFileHandleBase& operator=(const PlatformDependentFileHandleBase& cpy) = delete;

public:
	/**
	 * @brief Default constructor.
	 *
	 * Initializes the file handle or file descriptor to an invalid state.
	 */
	PlatformDependentFileHandleBase();

#ifdef _WIN32
	/**
	 * @brief Constructor initializing the file handle.
	 *
	 * @param fileHandle Windows file handle.
	 */
	PlatformDependentFileBase(HANDLE fileHandle);
#else
	/**
	 * @brief Constructor initializing the file descriptor.
	 *
	 * @param fileDescriptor Unix-like file descriptor.
	 */
	PlatformDependentFileHandleBase(int fileDescriptor);
#endif

	/**
	 * @brief Destructor.
	 *
	 * Closes the file handle or file descriptor if it is valid.
	 */
	~PlatformDependentFileHandleBase();

	/**
	 * @brief Move constructor.
	 *
	 * Transfers ownership of the file handle or file descriptor from another instance.
	 *
	 * @param mov The instance to move from.
	 */
	PlatformDependentFileHandleBase(PlatformDependentFileHandleBase&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Transfers ownership of the file handle or file descriptor from another instance.
	 *
	 * @param mov The instance to move from.
	 * @return Reference to the current instance.
	 */
	PlatformDependentFileHandleBase& operator=(PlatformDependentFileHandleBase&& mov);

	/**
	 * @brief Reads elements from the underlying platform file handle.
	 * @param buffer Destination memory.
	 * @param size Size of each element.
	 * @param count Number of elements to read.
	 * @return Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count);

	/**
	 * @brief Writes elements to the underlying platform file handle.
	 * @param buffer Source memory.
	 * @param size Size of each element.
	 * @param count Number of elements to write.
	 * @return Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count);

	/**
	 * @brief Repositions the file cursor.
	 * @param offset Byte offset relative to @p whence.
	 * @param whence Seek origin.
	 * @return `0` on success, non-zero on failure.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence);

	/**
	 * @brief Returns the current file cursor position.
	 * @return Current byte offset from the beginning.
	 */
	long tell();

	/**
	 * @brief Returns the current file size.
	 * @return File size in bytes.
	 */
	size_t size();

	/**
	 * @brief Checks whether the cursor is at end-of-file.
	 * @return `true` when EOF has been reached.
	 */
	bool eof();

	/**
	 * @brief Flushes buffered writes to the underlying file.
	 * @return `true` on success.
	 */
	bool flush();

	/**
	 * @brief Truncates the file to a new size.
	 *
	 * @param newSize New intended size of the file.
	 */
	void truncate(size_t newSize);
};

/**
 * @brief Enum class for specifying the mode of temporary file creation.
 *
 * This enumeration is used to specify different methods for creating temporary files on Unix-like systems.
 */
enum class TemporaryFileCreationMode : uint8_t {
	MKSTEMP,	///< Regular POSIX mkstemp()
	SHM_OPEN,   ///< POSIX shm_open()
	MEMFD_CREATE///< Linux-specific memfd_create()
};

/**
 * @brief Class for managing platform-dependent files.
 *
 * This class encapsulates the platform-specific file handle or file descriptor along with additional file information.
 */
class MH_EUPH_API PlatformDependentFileBase : public PlatformDependentFileHandleBase {
public:
	typedef void (*Deleter)(const char*); ///< Type alias for the file deleter function.
	std::string path;								 ///< Path to the file.
	Deleter deleter;								  ///< Deleter function to clean up the file.

private:
	PlatformDependentFileBase(const PlatformDependentFileHandleBase& cpy) = delete;
	PlatformDependentFileBase& operator=(const PlatformDependentFileHandleBase& cpy) = delete;

	void initializeViaRegularLoad(const char* cpath, Elv::Io::Mode mode); ///< Initializes the file via regular file loading.
	void initializeViaMkstemp();									  ///< Initializes the file via mkstemp.
	void initialiteViaShmOpen(const char* cpath);					   ///< Initializes the file via shm_open.
	void initializeViaMemfdCreate(const char* cpath);				   ///< Initializes the file via memfd_create.

public:
	/**
	 * @brief Destructor.
	 *
	 * Deletes the file if a deleter function is provided.
	 */
	~PlatformDependentFileBase();

	/**
	 * @brief Move constructor.
	 *
	 * Transfers ownership of the file handle or file descriptor from another instance.
	 *
	 * @param mov The instance to move from.
	 */
	PlatformDependentFileBase(PlatformDependentFileBase&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * Transfers ownership of the file handle or file descriptor from another instance.
	 *
	 * @param mov The instance to move from.
	 * @return Reference to the current instance.
	 */
	PlatformDependentFileBase& operator=(PlatformDependentFileBase&& mov);

	/**
	 * @brief Default constructor.
	 *
	 * Initializes the file handle or file descriptor to an invalid state and sets the deleter to nullptr.
	 */
	PlatformDependentFileBase();

#ifdef _WIN32
	/**
	 * @brief Constructor initializing the file with a handle, path, and deleter.
	 *
	 * @param fileHandle Windows file handle.
	 * @param path File path.
	 * @param deleter Deleter function.
	 */
	PlatformDependentFileBase(HANDLE fileHandle, std::string&& path, Deleter deleter);

	/**
	 * @brief Constructor initializing the file with a handle, path, and deleter.
	 *
	 * @param fileHandle Windows file handle.
	 * @param path File path.
	 * @param deleter Deleter function.
	 */
	PlatformDependentFileBase(HANDLE fileHandle, std::string&& path, Deleter deleter);

	/**
	 * @brief Constructor initializing the file with a handle, path, and deleter.
	 *
	 * @param fileHandle Windows file handle.
	 * @param path File path.
	 * @param deleter Deleter function.
	 */
	PlatformDependentFileBase(HANDLE fileHandle, const std::string& path, Deleter deleter);
#else
	/**
	 * @brief Constructor initializing the file with a descriptor, path, and deleter.
	 *
	 * @param fileDescriptor Unix-like file descriptor.
	 * @param path File path.
	 * @param deleter Deleter function.
	 */
	PlatformDependentFileBase(int fileDescriptor, std::string&& path, Deleter deleter);

	/**
	 * @brief Constructor initializing the file with a descriptor, path, and deleter.
	 *
	 * @param fileDescriptor Unix-like file descriptor.
	 * @param path File path.
	 * @param deleter Deleter function.
	 */
	PlatformDependentFileBase(int fileDescriptor, const std::string& path, Deleter deleter);
#endif

	/**
	 * @brief Constructor initializing the file via regular file loading.
	 *
	 * @param path File path.
	 * @param mode File access mode.
	 */
	PlatformDependentFileBase(const std::string& path, Elv::Io::Mode mode);

	/**
	 * @brief Constructor initializing the file via regular file loading.
	 *
	 * @param path File path.
	 * @param mode File access mode.
	 */
	PlatformDependentFileBase(std::string&& path, Elv::Io::Mode mode);

	/**
	 * @brief Constructor initializing the file via temporary file creation.
	 *
	 * @param cpath File path.
	 * @param mode Mode of temporary file creation.
	 */
	PlatformDependentFileBase(const char* cpath, TemporaryFileCreationMode mode);
};

}
}

#endif // EUPHPLATFORMDEPENDENTFILEBASE_HPP
