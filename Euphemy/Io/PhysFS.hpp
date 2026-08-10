#ifndef PHYSFS_HPP
#define PHYSFS_HPP
/**
 * @file PhysFS.hpp
 * @brief Declares the PhysFS API in the Euphemy/Io module.
 *
 * This header is part of the public declaration surface for Euphemy/Io.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
namespace PhysFS {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(Device)
/**
 * @class Device
 * @brief Implementation of Elv::Io::Device for PhysicsFS
 *
 * This class provides a concrete implementation of the Elv::Io::Device interface,
 * utilizing the PhysicsFS library for file operations.
 */
class MH_EUPH_API Device : public Elv::Io::Device {
private:
	/**
	 * @var handle
	 * @brief Underlying PhysicsFS file handle
	 */
	void* handle;

	/**
	 * @var mode
	 * @brief Mode in which the file was opened (e.g., read, write, append)
	 */
	Elv::Io::Mode mode;

	/**
	 * @brief Disabled copy constructor to prevent unintended copying
	 */
	Device(const Device& cpy) = delete;

	/**
	 * @brief Disabled copy assignment operator to prevent unintended copying
	 */
	Device& operator=(const Device& cpy) = delete;

public:
	/**
	 * @brief Constructor for opening a file at the specified path with the given mode
	 *
	 * @param path	 Path to the file to open
	 * @param mode	 Mode in which to open the file (e.g., read, write, append)
	 * @param bufferSize Optional buffer size for file operations (default: 0)
	 */
	Device(const char* path, Elv::Io::Mode mode, int bufferSize = 0);

	/**
	 * @brief Move constructor for transferring ownership of the file handle
	 *
	 * @param mov	  Device instance to move from
	 */
	Device(Device&& mov);

	/**
	 * @brief Move assignment operator for transferring ownership of the file handle
	 *
	 * @param mov	  Device instance to move from
	 * @return		 Reference to the assigned Device instance
	 */
	Device& operator=(Device&& mov);

	/**
	 * @brief Destructor for closing the file handle and releasing resources
	 */
	~Device();

	/**
	 * @brief Reads data from the open file into a buffer
	 *
	 * @param buffer   Pointer to the buffer to fill with data from the file
	 * @param size	 Size of each element to read
	 * @param count	Number of elements to read
	 * @return		 Number of elements successfully read
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data from a buffer to the open file
	 *
	 * @param buffer   Pointer to the buffer containing data to write
	 * @param size	 Size of each element to write
	 * @param count	Number of elements to write
	 * @return		 Number of elements successfully written
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Seeks to a position in the open file
	 *
	 * @param offset   Number of bytes to offset from the origin
	 * @param whence   Origin from which to seek (e.g., beginning, current, end)
	 * @return		 0 upon success, non-zero on failure
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current file position
	 *
	 * @return Current position in the file in bytes from the beginning
	 */
	long tell() override;

	/**
	 * @brief Retrieves the complete file size
	 *
	 * @return File size in bytes
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the file has been reached
	 *
	 * @return True if the end of the file has been reached, false otherwise
	 */
	bool eof() override;

	/**
	 * @brief Retrieves the current file access mode
	 *
	 * @return Mode in which the file was opened (e.g., read, write, append)
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes the file buffer to ensure written data is committed
	 *
	 * @return True if the flush operation was successful, false otherwise
	 */
	bool flush() override;

	/**
	 * @brief Checks if the file device is in a valid state
	 *
	 * @return True if the file device is valid, false otherwise
	 */
	bool isValid() const override;
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(System)
/**
 * @class System
 * @brief Implementation of Elv::Io::System for PhysicsFS
 *
 * This class provides a concrete implementation of the Elv::Io::System interface,
 * utilizing the PhysicsFS library for file system operations.
 */
class System : public Elv::Io::System {
private:
	/**
	 * @var isInit
	 * @brief Flag indicating whether the PhysicsFS library has been initialized
	 */
	bool isInit;

public:
	/**
	 * @brief Initializes the PhysicsFS library
	 *
	 * This must be called before any other PhysicsFS function. It should be called
	 * prior to any attempts to change your process's current working directory.
	 *
	 * @param argv0	Path to the executable (e.g., argv <sup> </sup> in main())
	 */
	static void init(const char* argv0);

	/**
	 * @brief Deinitializes the PhysicsFS library
	 *
	 * This closes any files opened via PhysicsFS, blanks the search/write paths,
	 * frees memory, and invalidates all of your file handles.
	 */
	static void deinit();

	/**
	 * @brief Adds an archive or directory to the search path
	 *
	 * @param path		 Directory or archive to add to the path
	 * @param mountPoint   Location in the interpolated tree where this archive will be "mounted"
	 * @param append	   True to append to search path, false to prepend (default: false)
	 */
	void mount(const char* path, const char* mountPoint, bool append = false);

	/**
	 * @brief Removes a directory or archive from the search path
	 *
	 * @param path	 Directory or archive to remove from the path
	 */
	void unmount(const char* path);

	/**
	 * @brief Enumerates available CD-ROM drives
	 *
	 * Retrieves an array of paths to available CD-ROM drives. The dirs returned are
	 * platform-dependent ("D:\" on Win32, "/cdrom" or whatnot on Unix). Dirs are only
	 * returned if there is a disc ready and accessible in the drive.
	 *
	 * @param functor  Callback function to invoke for each CD-ROM drive found
	 */
	void enumerateCdDrives(FilenameCallback functor);

	/**
	 * @brief Retrieves the path where the application resides
	 *
	 * Helper function to get the "base dir". This is the directory where the application
	 * was run from, which is probably the installation directory, and may or may not be
	 * the process's current working directory.
	 *
	 * @return Path to the application's base directory
	 */
	const char* getBaseDir();

	/**
	 * @brief Retrieves the user-and-app-specific path for writing files
	 *
	 * Helper function to get the "pref dir". This is meant to be where users can write
	 * personal files (preferences and save games, etc) that are specific to your application.
	 * This directory is unique per user, per application.
	 *
	 * @param org  Name of your organization (or a suitable substitute)
	 * @param app  Name of your application
	 * @return	 Path to the user-and-app-specific directory
	 */
	const char* getPrefDir(const char* org, const char* app);

	/**
	 * @brief Retrieves the current write directory
	 *
	 * @return Path to the current write directory
	 */
	const char* getWriteDir();

	/**
	 * @brief Sets a new write directory
	 *
	 * @param dir  Path to the new write directory
	 */
	void setWriteDir(const char* dir);

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
	 * @brief Checks if a file or directory exists at the specified path
	 *
	 * @param path	 Path to the file or directory to check
	 * @return		 True if the file or directory exists, false otherwise
	 */
	bool exists(const char* path) override;

	/**
	 * @brief Retrieves the platform-specific directory separator
	 *
	 * @return Directory separator character (e.g., '/', '\')
	 */
	char separator() const override;

	/**
	 * @brief Enumerates files and directories at the specified path
	 *
	 * @param path		 Path to the directory to enumerate
	 * @param withPath	 True to include the path in the enumeration, false otherwise
	 * @param functor	  Callback function to invoke for each file/directory found
	 */
	void enumerate(const char* path, bool withPath, FilenameCallback functor) override;

	/**
	 * @brief Enumerates files and directories at the specified path with additional filesystem information
	 *
	 * @param path		 Path to the directory to enumerate
	 * @param functor	  Callback function to invoke for each file/directory found, providing filesystem information
	 */
	void enumerate(const char* path, FilesystemCallback functor) override;

	/**
	 * @brief Checks if the specified path is a directory
	 *
	 * @param path	 Path to the file or directory to check
	 * @return		 True if the path is a directory, false otherwise
	 */
	bool isDirectory(const char* path) override;

	/**
	 * @brief Checks if the specified path is a symbolic link
	 *
	 * @param path	 Path to the file or directory to check
	 * @return		 True if the path is a symbolic link, false otherwise
	 */
	bool isSymlink(const char* path) override;

	/**
	 * @brief Checks if the specified path is a regular file
	 *
	 * @param path	 Path to the file or directory to check
	 * @return		 True if the path is a regular file, false otherwise
	 */
	bool isFile(const char* path) override;

	/**
	 * @brief Creates a new directory at the specified path
	 *
	 * @param dir	  Path to the directory to create
	 * @return		 True if the directory was created successfully, false otherwise
	 */
	bool mkdir(const char* dir) override;

	/**
	 * @brief Removes a file or directory at the specified path
	 *
	 * @param path	 Path to the file or directory to remove
	 * @return		 True if the file or directory was removed successfully, false otherwise
	 */
	bool remove(const char* path) override;
};

} // namespace PhysFS
#endif // PHYSFS_HPP
