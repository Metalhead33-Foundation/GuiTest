#ifndef ELVSTACKALLOCATOR_H
#define ELVSTACKALLOCATOR_H
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <array>
#include <cassert>
#include <stdexcept>
namespace Elv {
namespace Util {

/**
 * @defgroup StackAllocators Stack Allocators
 * @brief Memory allocators designed for stack-based memory management.
 * @{
 */

/**
 * @struct StaticStackAllocator
 * @brief A static stack allocator with a fixed size, allowing for efficient allocation and deallocation of blocks.
 *
 * This allocator is designed for scenarios where the maximum memory requirement is known at compile time.
 * It uses a static buffer of a specified size for allocations.
 *
 * @tparam Size The total size of the allocator's buffer in bytes.
 * @tparam id An optional identifier (default=1) for distinguishing multiple allocators of the same size.
 */
template <size_t Size, int id = 1>
struct StaticStackAllocator {
private:
	/**
	 * @var offset_
	 * @brief The current offset into the underlying buffer, indicating the next free byte.
	 *
	 * @note This variable is static, meaning all instances of the same Size and id share the same offset.
	 */
	static std::size_t offset_;

	/**
	 * @var underlyingBuffer
	 * @brief The static buffer used for memory allocations.
	 *
	 * The size of this buffer is determined by the @p Size template parameter.
	 */
	static std::array<std::byte, Size> underlyingBuffer;

public:
	/**
	 * @fn allocateBlock
	 * @brief Allocates a block of memory of size @p n bytes.
	 *
	 * If the allocation would exceed the buffer's capacity, returns a block indicating failure.
	 *
	 * @param n The size of the block to allocate in bytes.
	 * @return A @p Blk struct containing a pointer to the allocated memory and its size, or {nullptr, 0} on failure.
	 */
	Blk allocateBlock(std::size_t n) {
		if (offset_ + n > Size) {
			return { nullptr, 0 }; // Allocation failed
		}
		void* ptr = static_cast<std::byte*>(underlyingBuffer.data()) + offset_;
		Blk blk = { ptr, n };
		offset_ += n;
		return blk;
	}

	/**
	 * @fn deallocateBlock
	 * @brief Deallocates a previously allocated block of memory.
	 *
	 * Only the most recently allocated block can be deallocated. Attempting to deallocate any other block results in an exception.
	 *
	 * @param blk The block to deallocate.
	 * @throws std::invalid_argument If the block is not the most recently allocated one.
	 */
	void deallocateBlock(const Blk& blk) {
		assert(static_cast<std::byte*>(blk.ptr) == static_cast<std::byte*>(underlyingBuffer.data()) + (offset_ - blk.size));
		if (static_cast<std::byte*>(blk.ptr)!= static_cast<std::byte*>(underlyingBuffer.data()) + (offset_ - blk.size)) {
			throw std::invalid_argument("StackAllocator can only deallocate the most recent block.");
		}
		offset_ -= blk.size;
	}

	/**
	 * @fn ownsBlock
	 * @brief Checks if a given block of memory is owned by this allocator.
	 *
	 * A block is considered owned if its pointer falls within the range of the allocator's buffer.
	 *
	 * @param blk The block to check.
	 * @return True if the block is owned by this allocator, false otherwise.
	 */
	bool ownsBlock(const Blk& blk) const {
		return blk.ptr >= underlyingBuffer.data() && blk.ptr < static_cast<std::byte*>(underlyingBuffer.data()) + Size;
	}
};

/**
 * @var offset_
 * @relates StaticStackAllocator
 * @brief Initialization of the static offset variable.
 */
template <size_t Size, int id>
std::size_t StaticStackAllocator<Size, id>::offset_ = 0;

/**
 * @var underlyingBuffer
 * @relates StaticStackAllocator
 * @brief Initialization of the static buffer variable.
 */
template <size_t Size, int id>
std::array<std::byte, Size> StaticStackAllocator<Size, id>::underlyingBuffer;

/**
 * @struct DynamicStackAllocator
 * @brief A dynamic stack allocator that allocates memory from a base allocator at runtime.
 *
 * This allocator is useful when the maximum memory requirement is not known until runtime.
 * It uses a dynamically allocated buffer for its memory pool.
 *
 * @tparam BaseAlloc The base allocator type (default: std::allocator<std::byte>).
 * @requires Allocator<BaseAlloc, std::byte>
 */
template <typename BaseAlloc = std::allocator<std::byte>> requires Allocator<BaseAlloc, std::byte>
struct DynamicStackAllocator {
private:
	/**
	 * @var baseAlloc
	 * @brief The base allocator used for dynamic memory allocation.
	 */
	BaseAlloc baseAlloc;

	/**
	 * @var buffer_
	 * @brief Pointer to the dynamically allocated memory buffer.
	 */
	std::byte* buffer_;

	/**
	 * @var totalSize_
	 * @brief The total size of the dynamically allocated buffer.
	 */
	std::size_t totalSize_;

	/**
	 * @var offset_
	 * @brief The current offset into the buffer, indicating the next free byte.
	 */
	std::size_t offset_;

public:
	/**
	 * @fn DynamicStackAllocator
	 * @brief Constructs a DynamicStackAllocator with a buffer of @p totalSize bytes.
	 *
	 * The buffer is allocated using the base allocator.
	 *
	 * @param totalSize The size of the buffer to allocate in bytes.
	 */
	DynamicStackAllocator(std::size_t totalSize)
		: buffer_(baseAlloc.allocate(totalSize)),
		  totalSize_(totalSize),
		  offset_(0)
	{
	}

	/**
	 * @fn ~DynamicStackAllocator
	 * @brief Destructor, deallocates the buffer using the base allocator.
	 */
	~DynamicStackAllocator() {
		baseAlloc.deallocate(buffer_, totalSize_);
	}

	/**
	 * @fn allocateBlock
	 * @brief Allocates a block of memory of size @p n bytes from the dynamic buffer.
	 *
	 * If the allocation would exceed the buffer's capacity, returns a block indicating failure.
	 *
	 * @param n The size of the block to allocate in bytes.
	 * @return A @p Blk struct containing a pointer to the allocated memory and its size, or {nullptr, 0} on failure.
	 */
	Blk allocateBlock(std::size_t n) {
		if (offset_ + n > totalSize_) {
			return { nullptr, 0 }; // Allocation failed
		}
		void* ptr = static_cast<std::byte*>(buffer_) + offset_;
		Blk blk = { ptr, n };
		offset_ += n;
		return blk;
	}

	/**
	 * @fn deallocateBlock
	 * @brief Deallocates a previously allocated block of memory.
	 *
	 * Only the most recently allocated block can be deallocated. Attempting to deallocate any other block results in an exception.
	 *
	 * @param blk The block to deallocate.
	 * @throws std::invalid_argument If the block is not the most recently allocated one.
	 */
	void deallocateBlock(const Blk& blk) {
		assert(static_cast<std::byte*>(blk.ptr) == static_cast<std::byte*>(buffer_) + (offset_ - blk.size));
		if (static_cast<std::byte*>(blk.ptr)!= static_cast<std::byte*>(buffer_) + (offset_ - blk.size)) {
			throw std::invalid_argument("StackAllocator can only deallocate the most recent block.");
		}
		offset_ -= blk.size;
	}

	/**
	 * @fn ownsBlock
	 * @brief Checks if a given block of memory is owned by this allocator.
	 *
	 * A block is considered owned if its pointer falls within the range of the allocator's buffer.
	 *
	 * @param blk The block to check.
	 * @return True if the block is owned by this allocator, false otherwise.
	 */
	bool ownsBlock(const Blk& blk) const {
		return blk.ptr >= buffer_ && blk.ptr < static_cast<std::byte*>(buffer_) + totalSize_;
	}
};

/** @} */ // end of StackAllocators group

}
}
#endif // ELVSTACKALLOCATOR_H
