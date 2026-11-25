#ifndef LTEASSIMPIO_HPP
#define LTEASSIMPIO_HPP

/**
 * @file LteAssimpIO.hpp
 * @brief Declares Assimp I/O adapter classes wrapping the Elvavena/Io module.
 *
 * This header provides @ref Lotte::Asset::AssimpIOStream and @ref Lotte::Asset::AssimpIOSystem,
 * bridging Assimp's custom file I/O abstractions (`Assimp::IOStream` and `Assimp::IOSystem`)
 * to `Elv::Io::Device` and `Elv::Io::System`.
 */

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <utility>
#include <string_view>
#include <memory_resource>

namespace Lotte {
namespace Asset {

/**
 * @class AssimpIOStream
 * @brief Assimp stream adapter wrapping an @c Elv::Io::Device.
 *
 * Implements the @c Assimp::IOStream interface by delegating read, write, seek,
 * and query operations to an owned polymorphic @c Elv::Io::uDevice handle.
 */
class AssimpIOStream final : public Assimp::IOStream {
public:
	/**
	 * @brief Constructs a stream wrapper taking ownership of an open I/O device.
	 *
	 * @param device Unique pointer to an @c Elv::Io::Device instance.
	 */
	explicit AssimpIOStream(Elv::Io::uDevice device) noexcept;

	/**
	 * @brief Destroys the stream and releases the underlying @c Elv::Io::Device.
	 */
	~AssimpIOStream() override;

	/**
	 * @brief Reads data from the underlying device into a buffer.
	 *
	 * @param pvBuffer Pointer to the destination buffer.
	 * @param pSize Size in bytes of each element to read.
	 * @param pCount Number of elements to read.
	 * @return Number of elements successfully read, or @c 0 if the device is invalid.
	 */
	size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override;

	/**
	 * @brief Writes data from a buffer to the underlying device.
	 *
	 * @param pvBuffer Pointer to the source buffer containing data to write.
	 * @param pSize Size in bytes of each element to write.
	 * @param pCount Number of elements to write.
	 * @return Number of elements successfully written, or @c 0 if the device is invalid.
	 */
	size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override;

	/**
	 * @brief Sets the current read/write cursor position within the device.
	 *
	 * Maps Assimp's @c aiOrigin values to @c Elv::Io::SeekOrigin.
	 *
	 * @param pOffset Byte offset relative to @p pOrigin.
	 * @param pOrigin Reference origin for the seek operation (@c aiOrigin_SET, @c aiOrigin_CUR, or @c aiOrigin_END).
	 * @return @c aiReturn_SUCCESS on success, or @c aiReturn_FAILURE if seeking fails or the device is invalid.
	 */
	aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override;

	/**
	 * @brief Retrieves the current byte offset of the I/O cursor from the beginning of the stream.
	 *
	 * @return Current position in bytes, or @c 0 if the device is invalid or reports a negative offset.
	 */
	size_t Tell() const override;

	/**
	 * @brief Retrieves the total size of the underlying file/device.
	 *
	 * @return Total size in bytes, or @c 0 if the device is invalid.
	 */
	size_t FileSize() const override;

	/**
	 * @brief Flushes any buffered write operations to the underlying device.
	 */
	void Flush() override;

private:
	/**
	 * @brief Owned polymorphic pointer to the wrapped @c Elv::Io::Device.
	 */
	Elv::Io::uDevice m_device;
};

/**
 * @class AssimpIOSystem
 * @brief Assimp filesystem adapter wrapping an @c Elv::Io::System.
 *
 * Bridges Assimp's file discovery, stream creation, and directory management calls
 * to an @c Elv::Io::System backend using polymorphic memory resources for device allocation.
 */
class AssimpIOSystem final : public Assimp::IOSystem {
public:
	/**
	 * @brief Constructs an Assimp filesystem handler backed by an @c Elv::Io::System.
	 *
	 * @param system Reference to the underlying @c Elv::Io::System implementation. Must outlive this instance.
	 * @param memRes Memory resource used when allocating @c Elv::Io::Device handles via @c openUnique().
	 */
	explicit AssimpIOSystem(
		Elv::Io::System& system,
		std::pmr::memory_resource* memRes = std::pmr::get_default_resource()
		) noexcept;

	/**
	 * @brief Destroys the filesystem adapter.
	 */
	~AssimpIOSystem() override;

	/**
	 * @brief Checks whether a file or directory exists at the given path.
	 *
	 * @param pFile Path to query.
	 * @return @c true if the path exists in the underlying system, @c false otherwise.
	 */
	bool Exists(const char* pFile) const override;

	/**
	 * @brief Retrieves the directory separator character used by the underlying system.
	 *
	 * @return Path separator character (e.g., @c '/' or @c '\\').
	 */
	char getOsSeparator() const override;

	/**
	 * @brief Opens a file stream at the specified path with the given C-style access mode.
	 *
	 * Ownership of the returned @c Assimp::IOStream pointer is transferred to the caller
	 * (typically Assimp) and must be released via @ref Close.
	 *
	 * @param pFile Path to the file to open.
	 * @param pMode Standard C @c fopen mode string (e.g., @c "rb", @c "wb", @c "r+"). Defaults to @c "rb".
	 * @return Pointer to a newly allocated @ref AssimpIOStream, or @c nullptr if opening fails.
	 */
	Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override;

	/**
	 * @brief Closes and destroys a stream previously opened by @ref Open.
	 *
	 * @param pFile Pointer to the stream to close. Safe to pass @c nullptr.
	 */
	void Close(Assimp::IOStream* pFile) override;

	/**
	 * @brief Compares two file paths for equivalence.
	 *
	 * @param one First path to compare.
	 * @param second Second path to compare.
	 * @return @c true if both paths refer to the same location, @c false otherwise.
	 */
	bool ComparePaths(const char* one, const char* second) const override;

	/**
	 * @brief Creates a new directory at the specified path.
	 *
	 * @param path Path of the directory to create.
	 * @return @c true on success, @c false on failure.
	 */
	bool CreateDirectory(const std::string& path) override;

	/**
	 * @brief Changes the current working directory.
	 *
	 * @note @c Elv::Io::System is stateless with respect to working directories, so this
	 *       operation is unsupported and typically returns @c false.
	 *
	 * @param path Target working directory path.
	 * @return @c true if the working directory was changed, @c false otherwise.
	 */
	bool ChangeDirectory(const std::string& path) override;

	/**
	 * @brief Deletes the file or directory at the specified path
	 *
	 * @param file Path of the entry to remove
	 * @return @c true on success, @c false on failure
	 */
	bool DeleteFile(const std::string& file) override;

private:
	/**
	 * @brief Converts a C-style @c fopen mode string into @c Elv::Io::Mode bitflags.
	 *
	 * @param modeStr Mode string containing characters such as @c 'r', @c 'w', @c 'a', and @c '+'.
	 * @return Corresponding @c Elv::Io::Mode enumeration value (defaults to @c Elv::Io::Mode::READ)
	 */
	static Elv::Io::Mode parseMode(std::string_view modeStr) noexcept;

	/**
	 * @brief Reference to the wrapped @c Elv::Io::System instance.
	 */
	Elv::Io::System& m_system;

	/**
	 * @brief Polymorphic memory resource used for allocating @c Elv::Io::Device objects.
	 */
	std::pmr::memory_resource* m_memRes;
};

} // namespace Asset
} // namespace Lotte

#endif // LTEASSIMPIO_HPP