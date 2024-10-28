#ifndef ELVIOSYSTEM_HPP
#define ELVIOSYSTEM_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <functional>
namespace Elv {
namespace Io {
DEFINE_CLASS(System)
/**
 * @class System
 * @brief Abstract base class providing an interface for interacting with the file system.
 *
 * This class defines the necessary methods for managing files, directories, and symlinks.
 * Concrete subclasses must implement these pure virtual functions to provide platform-specific functionality.
 */
class System {
public:
	/**
	 * @typedef FilenameCallback
	 * @brief Callback function type for filename enumeration, taking a const char* as an argument.
	 */
	typedef std::function<void(const char*)> FilenameCallback;

	/**
	 * @typedef FilesystemCallback
	 * @brief Callback function type for filesystem enumeration, taking a System*, const char* (full path), and const char* (just filename) as arguments.
	 */
	typedef std::function<void(System*, const char*, const char*)> FilesystemCallback;

	/**
	 * @brief Virtual destructor to ensure proper cleanup of derived classes.
	 */
	virtual ~System() = default;

	/**
	 * @brief Opens a device (e.g., file, directory) at the specified path with the given mode.
	 *
	 * @param path The path to the device.
	 * @param mode The mode in which to open the device.
	 * @return A pointer to the opened Device, or nullptr on failure.
	 */
	virtual Device* open(const char* path, Mode mode) = 0;

	/**
	 * @brief Checks if a file or directory exists at the specified path.
	 *
	 * @param path The path to check.
	 * @return True if the path exists, false otherwise.
	 */
	virtual bool exists(const char* path) = 0;

	/**
	 * @brief Returns the character used as the directory separator for this system.
	 *
	 * @return The directory separator character.
	 */
	virtual char separator() const = 0;

	/**
	 * @brief Enumerates the contents of the directory at the specified path, invoking the provided callback for each item.
	 *
	 * @param path The directory path to enumerate.
	 * @param withPath Whether to include the full path in the callback invocation.
	 * @param functor The callback function to invoke for each enumerated item (const char* version).
	 */
	virtual void enumerate(const char* path, bool withPath, FilenameCallback functor) = 0;

	/**
	 * @brief Enumerates the contents of the directory at the specified path, invoking the provided callback for each item.
	 *
	 * @param path The directory path to enumerate.
	 * @param functor The callback function to invoke for each enumerated item, including full path and filename (const char* version).
	 */
	virtual void enumerate(const char* path, FilesystemCallback functor) = 0;

	/**
	 * @brief Checks if the specified path is a directory.
	 *
	 * @param path The path to check.
	 * @return True if the path is a directory, false otherwise.
	 */
	virtual bool isDirectory(const char* path) = 0;

	/**
	 * @brief Checks if the specified path is a symlink.
	 *
	 * @param path The path to check.
	 * @return True if the path is a symlink, false otherwise.
	 */
	virtual bool isSymlink(const char* path) = 0;

	/**
	 * @brief Checks if the specified path is a file.
	 *
	 * @param path The path to check.
	 * @return True if the path is a file, false otherwise.
	 */
	virtual bool isFile(const char* path) = 0;

	/**
	 * @brief Creates a new directory at the specified path.
	 *
	 * @param dir The path to the new directory.
	 * @return True on success, false on failure.
	 */
	virtual bool mkdir(const char* dir) = 0;

	/**
	 * @brief Removes the file or directory at the specified path.
	 *
	 * @param path The path to remove.
	 * @return True on success, false on failure.
	 */
	virtual bool remove(const char* path) = 0;

	// -------------------------------------------------------------------------
	// Convenience Wrappers
	// -------------------------------------------------------------------------
	template <typename Alloc> using AllocString = std::basic_string<char, std::char_traits<char>, Alloc>;
	/**
	 * @brief Opens a device (e.g., file, directory) at the specified path with the given mode (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The path to the device as a std::string.
	 * @param mode The mode in which to open the device.
	 * @return A pointer to the opened Device, or nullptr on failure.
	 */
	template <typename Alloc>
	inline Device* open(const AllocString<Alloc>& path, Mode mode) {
		return open(path.c_str(), mode);
	}

	/**
	 * @brief Checks if a file or directory exists at the specified path (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The path to check as a std::string.
	 * @return True if the path exists, false otherwise.
	 */
	template <typename Alloc>
	inline bool exists(const AllocString<Alloc>& path) {
		return exists(path.c_str());
	}

	/**
	 * @brief Enumerates the contents of the directory at the specified path, collecting results in a vector (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @tparam Alloc2 The allocator type for the output vector's elements.
	 * @tparam Alloc3 The allocator type for the output vector itself.
	 * @param path The directory path to enumerate as a std::string.
	 * @param withPath Whether to include the full path in the output.
	 * @param output The vector to populate with enumerated items.
	 */
	template <typename Alloc, typename Alloc2, typename Alloc3>
	inline void enumerate(const AllocString<Alloc>& path,
						  bool withPath,
						  std::vector<AllocString<Alloc2>, Alloc3>& output) {
		enumerate(path.c_str(), withPath, [&output](const char* pth) {
			output.push_back(AllocString<Alloc2>(pth));
		});
	}

	/**
	 * @brief Enumerates the contents of the directory at the specified path, returning results in a vector (const char* version).
	 *
	 * @tparam Alloc2 The allocator type for the output vector's elements (defaults to std::allocator<char>).
	 * @tparam Alloc3 The allocator type for the output vector itself (defaults to std::allocator<std::string>).
	 * @param path The directory path to enumerate.
	 * @param withPath Whether to include the full path in the output.
	 * @return A vector of enumerated items.
	 */
	template <typename Alloc2 = std::allocator<char>, typename Alloc3 = std::allocator<AllocString<Alloc2>>>
	inline std::vector<std::string> enumerate(const char* path, bool withPath) {
		std::vector<AllocString<Alloc2>, Alloc3> tmp;
		enumerate(path, withPath, tmp);
		return tmp;
	}

	/**
	 * @brief Enumerates the contents of the directory at the specified path, returning results in a vector (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @tparam Alloc2 The allocator type for the output vector's elements (defaults to std::allocator<char>).
	 * @tparam Alloc3 The allocator type for the output vector itself (defaults to std::allocator<std::string>).
	 * @param path The directory path to enumerate as a std::string.
	 * @param withPath Whether to include the full path in the output.
	 * @return A vector of enumerated items.
	 */
	template <typename Alloc, typename Alloc2 = std::allocator<char>, typename Alloc3 = std::allocator<AllocString<Alloc2>>>
	inline std::vector<std::string> enumerate(const AllocString<Alloc>& path, bool withPath) {
		std::vector<AllocString<Alloc2>, Alloc3> tmp;
		enumerate(path, withPath, tmp);
		return tmp;
	}

	/**
	 * @brief Enumerates the contents of the directory at the specified path, invoking the provided callback for each item (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The directory path to enumerate as a std::string.
	 * @param withPath Whether to include the full path in the callback invocation.
	 * @param functor The callback function to invoke for each enumerated item (const char* version).
	 */
	template <typename Alloc>
	inline void enumerate(const AllocString<Alloc>& path, bool withPath, FilenameCallback functor) {
		enumerate(path.c_str(), withPath, functor);
	}

	/**
	 * @brief Enumerates the contents of the directory at the specified path, invoking the provided callback for each item (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The directory path to enumerate as a std::string.
	 * @param functor The callback function to invoke for each enumerated item, including full path and filename (const char* version).
	 */
	template <typename Alloc>
	inline void enumerate(const AllocString<Alloc>& path, FilesystemCallback functor) {
		enumerate(path.c_str(), functor);
	}

	/**
	 * @brief Checks if the specified path is a directory (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The path to check as a std::string.
	 * @return True if the path is a directory, false otherwise.
	 */
	template <typename Alloc>
	inline bool isDirectory(const AllocString<Alloc>& path) {
		return isDirectory(path.c_str());
	}

	/**
	 * @brief Checks if the specified path is a symlink (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The path to check as a std::string.
	 * @return True if the path is a symlink, false otherwise.
	 */
	template <typename Alloc>
	inline bool isSymlink(const AllocString<Alloc>& path) {
		return isSymlink(path.c_str());
	}

	/**
	 * @brief Checks if the specified path is a file (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The path to check as a std::string.
	 * @return True if the path is a file, false otherwise.
	 */
	template <typename Alloc>
	inline bool isFile(const AllocString<Alloc>& path) {
		return isFile(path.c_str());
	}

	/**
	 * @brief Creates a new directory at the specified path (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param dir The path to the new directory as a std::string.
	 * @return True on success, false on failure.
	 */
	template <typename Alloc>
	inline bool mkdir(const AllocString<Alloc>& dir) {
		return mkdir(dir.c_str());
	}

	/**
	 * @brief Removes the file or directory at the specified path (std::string version).
	 *
	 * @tparam Alloc The allocator type for the input string.
	 * @param path The path to remove as a std::string.
	 * @return True on success, false on failure.
	 */
	template <typename Alloc>
	inline bool remove(const AllocString<Alloc>& path) {
		return remove(path.c_str());
	}
};
}
}
#endif // ELVIOSYSTEM_HPP
