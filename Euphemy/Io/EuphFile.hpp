#ifndef FILE_HPP
#define FILE_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Euphemy/Io/EuphMemoryMapped.hpp>
#include <Euphemy/Io/EuphPlatformDependentFileBase.hpp>

/**
 * @namespace Euph::Io
 * @brief Input/Output operations namespace for the Euph project.
 */
namespace Euph {
namespace Io {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(File)
/**
 * @class File
 * @brief Implementation of Elv::Io::Device for file operations.
 */
class MH_EUPH_API File : public Elv::Io::Device
{
private:
	/**
	 * @var fileHandle
	 * @brief Handle to the file.
	 */
	PlatformDependentFileBase fileHandle;

	/**
	 * @var mode
	 * @brief Mode in which the file was opened.
	 */
	Elv::Io::Mode mode;

	/**
	 * @brief Copy constructor (deleted to prevent copying).
	 */
	File(const File& cpy) = delete;

	/**
	 * @brief Assignment operator (deleted to prevent copying).
	 */
	File& operator=(const File& cpy) = delete;

public:
	/**
	 * @brief Constructor for opening a file.
	 * @param path Path to the file.
	 * @param mode Mode in which to open the file (see Elv::Io::Mode).
	 */
	File(const char* path, Elv::Io::Mode mode);

	/**
	 * @brief Move constructor.
	 */
	File(File&& mov);

	/**
	 * @brief Move assignment operator.
	 */
	File& operator=(File&& mov);

	/**
	 * @copydoc Elv::Io::Device::read
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @copydoc Elv::Io::Device::write
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @copydoc Elv::Io::Device::seek
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @copydoc Elv::Io::Device::tell
	 */
	long tell() override;

	/**
	 * @copydoc Elv::Io::Device::size
	 */
	size_t size() override;

	/**
	 * @copydoc Elv::Io::Device::eof
	 */
	bool eof() override;

	/**
	 * @copydoc Elv::Io::Device::getMode
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @copydoc Elv::Io::Device::flush
	 */
	bool flush() override;

	/**
	 * @copydoc Elv::Io::Device::isValid
	 */
	bool isValid() const override;
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(MemoryMappedFile)
/**
 * @warning **THIS CLASS IS NOT A SUBCLASS OF `File` AND DOES NOT IMPLEMENT `Elv::Io::Device`!**
 *		  **IT IS A COMPLETELY SEPARATE CLASS, USE ACCORDINGLY!**
 *
 * @class MemoryMappedFile
 * @brief A class representing a regular file mapped into memory for efficient access.
 */
class MH_EUPH_API MemoryMappedFile : public MemoryMapped
{
private:
	/**
	 * @var fileHandle
	 * @brief Handle to the file.
	 */
	PlatformDependentFileBase fileHandle;
	/**
	 * @var mode
	 * @brief Mode in which the file was opened.
	 */
	Elv::Io::Mode mode;

	/**
	 * @name Disabled copy constructor and assignment operator
	 * @{
	 */
	MemoryMappedFile(const MemoryMappedFile& cpy) = delete;
	MemoryMappedFile& operator=(const MemoryMappedFile& cpy) = delete;
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
	 * @brief Constructs a MemoryMappedFile with the specified size.
	 *
	 * Creates a temporary file of the given size, maps it into memory, and sets up the object for access.
	 *
	 * @param fileSize The size of the temporary file to create.
	 * @throws std::runtime_error or std::system_error if file creation, mapping, or setup fails.
	 */
	MemoryMappedFile(const char* path, Elv::Io::Mode mode, size_t minSize=0);

	/**
	 * @brief Move constructor for MemoryMappedFile.
	 *
	 * Transfers ownership of the temporary file and its mapping from another instance.
	 *
	 * @param mov The instance to move from.
	 */
	MemoryMappedFile(MemoryMappedFile&& mov);

	/**
	 * @brief Move assignment operator for MemoryMappedFile.
	 *
	 * Transfers ownership of the temporary file and its mapping from another instance, releasing any previously held resources.
	 *
	 * @param mov The instance to move from.
	 * @return Reference to the assigned instance.
	 */
	MemoryMappedFile& operator=(MemoryMappedFile&& mov);

	/**
	 * @copydoc Elv::Io::Device::getMode
	 */
	Elv::Io::Mode getMode() const;

	/**
	 * @brief Gets whether this memory-mapping is read-only or not.
	 *
	 * @return Whether the memory-mapping is read-only or not.
	 */
	bool readOnly() const override;
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(Filesystem)
/**
 * @class Filesystem
 * @brief Implementation of Elv::Io::System for filesystem operations.
 */
class MH_EUPH_API Filesystem : public Elv::Io::System
{
public:
	/**
	 * @brief Opens a file at the specified path with the given mode
	 *
	 * @param path	 Path to the file to open
	 * @param mode	 Mode in which to open the file (e.g., read, write, append)
	 * @return		 Pointer to the opened Device instance, or nullptr on failure
	 */
	Elv::Io::Device* open(const char* path, Elv::Io::Mode mode) override;
	/**
	 * @brief Opens a device (e.g., file, directory) at the specified path with the given mode.
	 *
	 * @param path The path to the device.
	 * @param mode The mode in which to open the device (e.g., read, write, append)
	 * @param memRes The memory resource used for allocating the device.
	 * @return A unique pointer to the opened Device, or nullptr on failure.
	 */
	Elv::Io::uDevice openUnique(const char* path, Elv::Io::Mode mode, std::pmr::memory_resource* memRes) override;

	/**
	 * @brief Opens a device (e.g., file, directory) at the specified path with the given mode.
	 *
	 * @param path The path to the device.
	 * @param mode The mode in which to open the device (e.g., read, write, append)
	 * @param memRes The memory resource used for allocating the device.
	 * @return A shared pointer to the opened Device, or nullptr on failure.
	 */
	Elv::Io::sDevice openShared(const char* path, Elv::Io::Mode mode, std::pmr::memory_resource* memRes) override;
	/**
	 * @brief Opens a memory-mapped file within the filesystem.
	 * @param path Path to the file.
	 * @param mode Mode in which to open the file (see Elv::Io::Mode).
	 * @return Pointer to the opened memory-mapped file device.
	 */
	MemoryMappedFile* openMemoryMapped(const char* path, Elv::Io::Mode mode);
	/**
	 * @brief Opens a memory-mapped file within the filesystem.
	 * @param path Path to the file.
	 * @param mode Mode in which to open the file (see Elv::Io::Mode).
	 * @param memRes The memory resource used for allocating the device.
	 * @return Unique pointer to the opened memory-mapped file device.
	 */
	uMemoryMappedFile openMemoryMappedUnique(const char* path, Elv::Io::Mode mode, std::pmr::memory_resource* memRes = std::pmr::get_default_resource() );
	/**
	 * @brief Opens a memory-mapped file within the filesystem.
	 * @param path Path to the file.
	 * @param mode Mode in which to open the file (see Elv::Io::Mode).
	 * @param memRes The memory resource used for allocating the device.
	 * @return Shared pointer to the opened memory-mapped file device.
	 */
	sMemoryMappedFile openMemoryMappedShared(const char* path, Elv::Io::Mode mode, std::pmr::memory_resource* memRes = std::pmr::get_default_resource() );

	/**
	 * @brief Checks if a path exists within the filesystem.
	 * @param path Path to check.
	 * @return True if the path exists, false otherwise.
	 */
	bool exists(const char* path) override;

	/**
	 * @brief Returns the separator used in the filesystem's paths.
	 * @return Character representing the path separator.
	 */
	char separator() const override;

	/**
	 * @brief Enumerates files/directories within a given path.
	 * @param path Base path for enumeration.
	 * @param withPath Whether to include the full path in the callback.
	 * @param functor Callback function for each enumerated item (FilenameCallback type).
	 */
	void enumerate(const char* path, bool withPath, FilenameCallback functor) override;

	/**
	 * @brief Enumerates files/directories within a given path with detailed information.
	 * @param path Base path for enumeration.
	 * @param functor Callback function for each enumerated item (FilesystemCallback type).
	 */
	void enumerate(const char* path, FilesystemCallback functor) override;

	/**
	 * @brief Checks if a path is a directory.
	 * @param path Path to check.
	 * @return True if the path is a directory, false otherwise.
	 */
	bool isDirectory(const char* path) override;

	/**
	 * @brief Checks if a path is a symbolic link.
	 * @param path Path to check.
	 * @return True if the path is a symbolic link, false otherwise.
	 */
	bool isSymlink(const char* path) override;

	/**
	 * @brief Checks if a path is a regular file.
	 * @param path Path to check.
	 * @return True if the path is a file, false otherwise.
	 */
	bool isFile(const char* path) override;

	/**
	 * @brief Creates a new directory.
	 * @param dir Path to the new directory.
	 * @return True on success, false on failure.
	 */
	bool mkdir(const char* dir) override;

	/**
	 * @brief Removes a file or directory.
	 * @param path Path to the item to remove.
	 * @return True on success, false on failure.
	 */
	bool remove(const char* path) override;
};

}
}

#endif // FILE_HPP
