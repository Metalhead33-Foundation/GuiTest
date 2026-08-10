#ifndef MEMORYMAPPED_HPP
#define MEMORYMAPPED_HPP
/**
 * @file EuphMemoryMapped.hpp
 * @brief Declares the EuphMemoryMapped API in the Euphemy/Io module.
 *
 * This header is part of the public declaration surface for Euphemy/Io.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <cstddef>
#include <span>
#include <Euphemy/Io/EuphPlatformDependentFileBase.hpp>
#ifdef _WIN32
	#include <windows.h>
#endif
namespace Euph {
namespace Io {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(MemoryMapped)
/**
 * @brief Abstract base for memory-mapped files.
 *
 * Owns platform-specific mapping resources and exposes span/iterator access
 * to the mapped byte range.
 */
class MH_EUPH_API MemoryMapped
{
protected:
	/**
	 * @name Platform-specific file handles and mappings
	 * @{
	 */
#ifdef _WIN32
	HANDLE mappingHandle; ///< Handle for the file mapping on Windows.
	void* mappedView; ///< Mapped view of the file on Windows.
#else
	void* mappedAddress; ///< Mapped address of the file on Unix-like systems.
#endif
	/// @}

	size_t fileSize; ///< Size of the temporary file.

	/**
	 * @name Disabled copy constructor and assignment operator
	 * @{
	 */
	MemoryMapped(const MemoryMapped& cpy) = delete;
	MemoryMapped& operator=(const MemoryMapped& cpy) = delete;
	/// @}
	/**
	 * @name Allowed move constructor and assignment operator
	 * @{
	 */
	/**
	 * @brief Move constructor.
	 * @param mov Source object to move from.
	 */
	MemoryMapped(MemoryMapped&& mov);
	/**
	 * @brief Move assignment operator.
	 * @param mov Source object to move from.
	 * @return Reference to this object.
	 */
	MemoryMapped& operator=(MemoryMapped&& mov);
	/// @}
	/**
	 * @brief Default constructor.
	 */
	MemoryMapped(size_t fileSize = 0);
	/**
	 * @brief Gets the platform-dependent basic file handle.
	 *
	 * @return The platform-dependent basic file handle.
	 */
	virtual PlatformDependentFileHandleBase& getFileHandle() = 0;
	/**
	 * @brief Gets the platform-dependent basic file handle.
	 *
	 * @return The platform-dependent basic file handle.
	 */
	virtual const PlatformDependentFileHandleBase& getFileHandle() const = 0;
	/**
	 * @brief Maps the file to memory.
	 *
	 * @param readOnly Whether the file is opened in read-only mode or not.
	 */
	void mapFile(bool readOnly);
	/**
	 * @brief Unmaps the memory-mapped file. Useful for remapping.
	 */
	void unmapFile();
public:
	/**
	 * @name Type aliases for convenience and clarity
	 * @{
	 */
	typedef std::byte value_type; ///< Type of a single byte in the file.
	typedef size_t size_type; ///< Type for sizes and indices.
	typedef ptrdiff_t difference_type; ///< Type for differences between indices.
	typedef std::byte& reference; ///< Reference to a byte in the file.
	typedef const std::byte& const_reference; ///< Constant reference to a byte in the file.
	typedef void* pointer; ///< Pointer to the beginning of the file.
	typedef const void* const_pointer; ///< Constant pointer to the beginning of the file.
	typedef std::span<std::byte> span; ///< Span of bytes in the file.
	typedef std::span<const std::byte> const_span; ///< Constant span of bytes in the file.
	typedef span::iterator iterator; ///< Iterator for the file's bytes.
	typedef span::reverse_iterator reverse_iterator; ///< Reverse iterator for the file's bytes.
	typedef const_span::iterator const_iterator; ///< Constant iterator for the file's bytes.
	typedef const_span::reverse_iterator reverse_const_iterator; ///< Constant reverse iterator for the file's bytes.
	/// @}
	virtual ~MemoryMapped();
	/**
	 * @brief Gets whether this memory-mapping is read-only or not.
	 *
	 * @return Whether the memory-mapping is read-only or not.
	 */
	virtual bool readOnly() const = 0;
	/**
	 * @brief Retrieves the size of the temporary file.
	 *
	 * @return The file size.
	 */
	size_t size() const;

	/**
	 * @brief Resizes the memory-mapped file to the given size.
	 * @param newSize The new size to be resized to.
	 * @warning This invalidates the pointer originally pointed to!
	 */
	void resize(size_t newSize);

	/**
	 * @brief Returns a span of bytes representing the entire file.
	 *
	 * Allows for non-constant access to the file's contents.
	 *
	 * @return A span of bytes.
	 */
	std::span<std::byte> as_span();

	/**
	 * @brief Returns a constant span of bytes representing the entire file.
	 *
	 * Allows for constant access to the file's contents.
	 *
	 * @return A constant span of bytes.
	 */
	std::span<const std::byte> as_span() const;

	/**
	 * @brief Implicit conversion to a non-constant span of bytes.
	 *
	 * @return A span of bytes.
	 */
	operator std::span<std::byte>();

	/**
	 * @brief Implicit conversion to a constant span of bytes.
	 *
	 * @return A constant span of bytes.
	 */
	operator std::span<const std::byte>() const;

	/**
	 * @brief Retrieves a pointer to the beginning of the file's mapped memory.
	 *
	 * @return A void pointer to the file's data.
	 */
	void* data();

	/**
	 * @brief Retrieves a constant pointer to the beginning of the file's mapped memory.
	 *
	 * @return A constant void pointer to the file's data.
	 */
	const void* data() const;

	/**
	 * @brief Asynchronously flushes the mmemory-mapped range by scheduling a flush and then returning immediately.
	 * @param invalidate When set to true, asks to invalidate other mappings of the same file (so that they can be updated with the fresh values just written).
	 */
	void flushAsync(bool invalidate = false);
	/**
	 * @brief Synchronously flushes the memory-mapped range by requesting a flush and waiting for it to completely.
	 * @param invalidate When set to true, asks to invalidate other mappings of the same file (so that they can be updated with the fresh values just written).
	 */
	void flushSync(bool invalidate = false);

	/**
	 * @name Iterator access
	 * @{
	 */
	/**
	 * @brief Returns an iterator to the first byte in the mapping.
	 */
	iterator begin();
	/**
	 * @brief Returns a const iterator to the first byte in the mapping.
	 */
	const_iterator begin() const;
	/**
	 * @brief Returns a reverse iterator to the last byte in the mapping.
	 */
	reverse_iterator rbegin();
	/**
	 * @brief Returns a const reverse iterator to the last byte in the mapping.
	 */
	reverse_const_iterator rbegin() const;
	/**
	 * @brief Returns an iterator one past the last byte in the mapping.
	 */
	iterator end();
	/**
	 * @brief Returns a const iterator one past the last byte in the mapping.
	 */
	const_iterator end() const;
	/**
	 * @brief Returns a reverse iterator one past the first byte in reverse traversal.
	 */
	reverse_iterator rend();
	/**
	 * @brief Returns a const reverse iterator one past the first byte in reverse traversal.
	 */
	reverse_const_iterator rend() const;
	/// @}

	/**
	 * @brief Subscript operator for non-constant access to a byte in the file.
	 *
	 * @param i The index of the byte to access.
	 * @return A reference to the byte at the specified index.
	 */
	reference operator[](size_t i);

	/**
	 * @brief Subscript operator for constant access to a byte in the file.
	 *
	 * @param i The index of the byte to access.
	 * @return A constant reference to the byte at the specified index.
	 */
	const_reference operator[](size_t i) const;
};

}
}
#endif // MEMORYMAPPED_HPP
