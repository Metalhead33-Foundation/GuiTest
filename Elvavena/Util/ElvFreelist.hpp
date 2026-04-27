#ifndef ELVFREELIST_HPP
#define ELVFREELIST_HPP
/**
 * @file ElvFreelist.hpp
 * @brief Declares the ElvFreelist API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <cassert>
#include <vector>
#include <span>
#include <stdexcept>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <memory>
namespace Elv {
namespace Util {
/**
 * @class FixedFreeListAllocator
 * @brief Andrei Alexandrescu-style memory allocator utilizing a freelist for managing free memory blocks within a fixed-size memory space.
 *
 * @tparam Size The total size of the managed memory space.
 */
template <std::size_t Size>
class FixedFreeListAllocator {
private:
	/**
	 * @struct FreeBlock
	 * @brief Represents a free block of memory in the freelist.
	 */
	struct FreeBlock {
		std::size_t size; /**< Size of the free block in bytes. */
		FreeBlock* next; /**< Pointer to the next free block in the list. */
	};

	/**
	 * @var MIN_BLOCK_SIZE
	 * @brief Minimum size of a block in the freelist (sizeof(FreeBlock)).
	 */
	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);

	/**
	 * @var buffer_
	 * @brief Aligned character buffer representing the entire memory space.
	 */
	alignas(std::max_align_t) char buffer_[Size];

	/**
	 * @var freeList_
	 * @brief Pointer to the head of the freelist.
	 */
	FreeBlock* freeList_;

	/**
	 * @var _mutex
	 * @brief Recursive mutex for thread-safe allocations and deallocations.
	 */
	mutable std::recursive_mutex _mutex;

	/**
	 * @typedef Lock
	 * @brief Convenience alias for a lock guard on the recursive mutex.
	 */
	typedef std::lock_guard<std::recursive_mutex> Lock;

public:
	/**
	 * @brief Default constructor, initializing the freelist.
	 *
	 * @exception None (noexcept)
	 */
	FixedFreeListAllocator() noexcept : freeList_(nullptr) {
		initializeFreeList();
	}

	/**
	 * @brief Allocates a block of memory from the freelist.
	 *
	 * @param n Requested size of the memory block in bytes.
	 * @return Allocated memory block (contains null-pointer and zero size on failure).
	 *
	 * @exception None (noexcept)
	 */
	Blk allocateBlock(std::size_t n) noexcept {
		Lock lock(_mutex);
		n = std::max(n, MIN_BLOCK_SIZE);
		n = alignUp(n);

		FreeBlock* prev = nullptr;
		FreeBlock* current = freeList_;

		while (current) {
			if (current->size >= n) {
				if (current->size >= n + MIN_BLOCK_SIZE) {
					// Split the block
					FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(reinterpret_cast<char*>(current) + n);
					newBlock->size = current->size - n;
					newBlock->next = current->next;

					if (prev) {
						prev->next = newBlock;
					} else {
						freeList_ = newBlock;
					}

					return {current, n};
				} else {
					// Use the entire block
					if (prev) {
						prev->next = current->next;
					} else {
						freeList_ = current->next;
					}

					return {current, current->size};
				}
			}

			prev = current;
			current = current->next;
		}

		return {nullptr, 0}; // Allocation failed
	}

	/**
	 * @brief Deallocates a previously allocated memory block, returning it to the freelist.
	 *
	 * @param blk Reference to the memory block to deallocate (must be owned by this allocator).
	 *
	 * @exception None (noexcept)
	 */
	void deallocateBlock(const Blk& blk) noexcept {
		Lock lock(_mutex);
		if (!ownsBlock(blk)) {
			return; // Silently ignore non-owned blocks
		}

		FreeBlock* newFree = reinterpret_cast<FreeBlock*>(blk.ptr);
		newFree->size = blk.size;

		// Insert the block back into the freelist, sorted by address
		FreeBlock** current = &freeList_;
		while (*current && *current < newFree) {
			current = &((*current)->next);
		}

		newFree->next = *current;
		*current = newFree;

		mergeFreeBlocks();
	}

	/**
	 * @brief Checks if this allocator owns the specified memory block.
	 *
	 * @param blk Reference to the memory block to check.
	 * @return True if the block is owned by this allocator, false otherwise.
	 *
	 * @exception None (noexcept)
	 */
	bool ownsBlock(const Blk& blk) const noexcept {
		Lock lock(_mutex);
		return blk.ptr >= buffer_ &&
			   blk.ptr < (buffer_ + Size);
	}

private:
	/**
	 * @brief Initializes the freelist with the entire memory space.
	 *
	 * @exception None (noexcept)
	 */
	void initializeFreeList() noexcept {
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = Size;
		freeList_->next = nullptr;
	}

	/**
	 * @brief Merges adjacent free blocks in the freelist to reduce fragmentation.
	 *
	 * @exception None (noexcept)
	 */
	void mergeFreeBlocks() noexcept {
		FreeBlock* current = freeList_;
		while (current && current->next) {
			if (reinterpret_cast<char*>(current) + current->size == reinterpret_cast<char*>(current->next)) {
				// Merge adjacent blocks
				current->size += current->next->size;
				current->next = current->next->next;
			} else {
				current = current->next;
			}
		}
	}

	/**
	 * @brief Aligns a size up to the nearest multiple of std::max_align_t.
	 *
	 * @param n Size to align.
	 * @return Aligned size.
	 *
	 * @exception None (noexcept)
	 */
	static std::size_t alignUp(std::size_t n) noexcept {
		return (n + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
	}
};

/**
 * @brief Andrei Alexandrescu-style memory allocator utilizing a freelist to manage free memory blocks within a fixed-size memory space.
 *
 * @tparam Size The total size of the memory space (in bytes) managed by this allocator.
 * @tparam id Unique identifier for distinguishing between allocators of the same size.
 */
template <std::size_t Size, int id>
class StaticFreeListAllocator {
private:
	/**
	 * @brief Static instance of the underlying FixedFreeListAllocator, managing the memory space of size 'Size'.
	 */
	static FixedFreeListAllocator<Size> alloc;

public:
	/**
	 * @brief Allocates a block of memory of the specified size.
	 *
	 * @param[in] n The size of the memory block to allocate (in bytes).
	 * @return Blk The allocated memory block. If allocation fails, returns a block with a null pointer and zero size.
	 * @throws None (noexcept)
	 */
	Blk allocateBlock(std::size_t n) noexcept {
		return alloc.allocateBlock(n);
	}

	/**
	 * @brief Deallocates a previously allocated memory block.
	 *
	 * @param[in] blk Reference to the memory block to deallocate. **Ensure this block was allocated by this instance.**
	 * @throws None (noexcept)
	 */
	void deallocateBlock(const Blk& blk) noexcept {
		alloc.deallocateBlock(blk);
	}

	/**
	 * @brief Checks if this allocator owns the specified memory block.
	 *
	 * @param[in] blk Reference to the memory block to check ownership for.
	 * @return true If this allocator owns the block, false otherwise.
	 * @throws None (noexcept)
	 */
	bool ownsBlock(const Blk& blk) const noexcept {
		return alloc.ownsBlock(blk);
	}
};

/**
 * @brief Definition of the static allocator instance for each instantiation of StaticFreeListAllocator.
 */
template <std::size_t Size, int id>
FixedFreeListAllocator<Size> StaticFreeListAllocator<Size, id>::alloc;

/**
 * @brief An Andrei Alexandrescu-style memory allocator that uses a freelist for keeping track of free blocks of memory within the entire memory space.
 *
 * @tparam BaseAlloc The underlying allocator for allocating the memory space. (Default: std::allocator<std::byte>)
 * @requires Allocator<BaseAlloc, std::byte>
 */
template <typename BaseAlloc = std::allocator<std::byte>> requires Allocator<BaseAlloc, std::byte>
struct DynamicFreeListAllocator {
private:
	/**
	 * @struct FreeBlock
	 * @brief Represents a free block of memory in the freelist.
	 */
	struct FreeBlock {
		/**
		 * @var size
		 * @brief The size of the free block in bytes.
		 */
		std::size_t size;

		/**
		 * @var next
		 * @brief Pointer to the next free block in the freelist.
		 */
		FreeBlock* next;
	};

	/**
	 * @var baseAlloc
	 * @brief The underlying allocator for allocating the memory space.
	 */
	BaseAlloc baseAlloc;

	/**
	 * @var MIN_BLOCK_SIZE
	 * @brief The minimum size of a block (aligned to the size of FreeBlock).
	 */
	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);

	/**
	 * @var totalSize_
	 * @brief The total size of the memory space in bytes.
	 */
	size_t totalSize_;

	/**
	 * @var buffer_
	 * @brief Pointer to the beginning of the memory space.
	 */
	char* buffer_;

	/**
	 * @var freeList_
	 * @brief Pointer to the first block of free memory in the freelist.
	 */
	FreeBlock* freeList_;

	/**
	 * @var _mutex
	 * @brief A mutex for ensuring thread-safety.
	 */
	mutable std::recursive_mutex _mutex;

	/**
	 * @typedef Lock
	 * @brief A lock guard for the mutex.
	 */
	typedef std::lock_guard<std::recursive_mutex> Lock;

public:
	/**
	 * @brief Constructor.
	 *
	 * Initializes the allocator with a memory space of the specified size.
	 *
	 * @param totalSize The size of the total memory space in bytes.
	 */
	DynamicFreeListAllocator(std::size_t totalSize)
		: buffer_(baseAlloc.allocate(totalSize)),
		  totalSize_(totalSize)
	{
		// Initialize the free list with one large free block
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

	/**
	 * @brief Allocates a block of memory.
	 *
	 * Attempts to allocate a block of memory of at least the specified size.
	 *
	 * @param n The minimum size of the memory block to allocate in bytes.
	 * @return Blk A block descriptor containing the allocated memory pointer and size. Returns a null pointer and zero size if allocation fails.
	 */
	Blk allocateBlock(std::size_t n) noexcept {
		Lock lock(_mutex);
		n = std::max(n, MIN_BLOCK_SIZE);
		n = alignUp(n);

		FreeBlock* prev = nullptr;
		FreeBlock* current = freeList_;

		while (current) {
			if (current->size >= n) {
				if (current->size >= n + MIN_BLOCK_SIZE) {
					// Split the block
					FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(reinterpret_cast<char*>(current) + n);
					newBlock->size = current->size - n;
					newBlock->next = current->next;

					if (prev) {
						prev->next = newBlock;
					} else {
						freeList_ = newBlock;
					}

					return {current, n};
				} else {
					// Use the entire block
					if (prev) {
						prev->next = current->next;
					} else {
						freeList_ = current->next;
					}

					return {current, current->size};
				}
			}

			prev = current;
			current = current->next;
		}

		return {nullptr, 0}; // Allocation failed
	}

	/**
	 * @brief Deallocates a block of memory.
	 *
	 * Returns a previously allocated memory block to the freelist.
	 *
	 * @param blk A reference to the memory block being deallocated. Ensure the block is owned by this allocator.
	 */
	void deallocateBlock(const Blk& blk) noexcept {
		Lock lock(_mutex);
		if (!ownsBlock(blk)) {
			return; // Silently ignore blocks that don't belong to us
		}

		FreeBlock* newFree = reinterpret_cast<FreeBlock*>(blk.ptr);
		newFree->size = blk.size;

		// Insert the block back into the free list, sorted by address
		FreeBlock** current = &freeList_;
		while (*current && *current < newFree) {
			current = &((*current)->next);
		}

		newFree->next = *current;
		*current = newFree;

		mergeFreeBlocks();
	}

	/**
	 * @brief Checks if the allocator owns a memory block.
	 *
	 * Verifies whether a given memory block falls within the allocator's memory space.
	 *
	 * @param blk A reference to the memory block being checked.
	 * @return True if the allocator owns the block, false otherwise.
	 */
	bool ownsBlock(const Blk& blk) const noexcept {
		Lock lock(_mutex);
		return blk.ptr >= buffer_ &&
			   blk.ptr < (buffer_ + totalSize_);
	}

private:
	/**
	 * @brief Initializes the freelist with the entire memory space.
	 *
	 * (Internal use only)
	 */
	void initializeFreeList() noexcept {
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

	/**
	 * @brief Merges adjacent free blocks in the freelist.
	 *
	 * (Internal use only)
	 */
	void mergeFreeBlocks() noexcept {
		FreeBlock* current = freeList_;
		while (current && current->next) {
			if (reinterpret_cast<char*>(current) + current->size == reinterpret_cast<char*>(current->next)) {
				// Merge adjacent blocks
				current->size += current->next->size;
				current->next = current->next->next;
			} else {
				current = current->next;
			}
		}
	}

	/**
	 * @brief Aligns a size up to the maximum alignment boundary.
	 *
	 * (Internal use only)
	 *
	 * @param n The size to align.
	 * @return The aligned size.
	 */
	static std::size_t alignUp(std::size_t n) noexcept {
		return (n + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
	}
};

/**
 * @class ContiguousFreeListAllocator
 * @brief A memory allocator that manages a contiguous block of memory using a free list.
 *		**WARNING: VERY DANGEROUS! USE CAUTIOUSLY!**
 *
 * This allocator is not suitable for all use cases due to its lack of robustness against
 * certain types of memory errors or misuse. Use with extreme caution and only when
 * the benefits of its specific characteristics (e.g., low overhead, contiguous allocation)
 * outweigh the risks.
 */
class ContiguousFreeListAllocator {
private:
	/**
	 * @struct FreeBlock
	 * @brief Represents a block of free memory in the allocator's free list.
	 */
	struct FreeBlock {
		std::size_t size; ///< Size of the free block in bytes.
		FreeBlock* next; ///< Pointer to the next free block in the list.
	};

	/**
	 * @var MIN_BLOCK_SIZE
	 * @brief The minimum size of a block that can be allocated, set to the size of a FreeBlock.
	 */
	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);

	std::size_t totalSize_; ///< Total size of the managed memory buffer in bytes.
	char* buffer_; ///< Pointer to the beginning of the managed memory buffer.
	FreeBlock* freeList_; ///< Pointer to the head of the free block list.
	mutable std::recursive_mutex _mutex; ///< Mutex for thread-safe access.
	typedef std::lock_guard<std::recursive_mutex> Lock; ///< Convenience type for locking the mutex.

public:
	/**
	 * @brief Constructor for ContiguousFreeListAllocator.
	 * @param totalSize Total size of the memory buffer to manage.
	 * @param buffer Pointer to the memory buffer to manage.
	 */
	ContiguousFreeListAllocator(std::size_t totalSize, char* buffer)
		: totalSize_(totalSize), buffer_(buffer), freeList_(nullptr)
	{
		initializeFreeList();
	}

	/**
	 * @brief Allocates a block of memory of at least the requested size.
	 * @param n Requested size of the block in bytes.
	 * @return A Blk struct containing a pointer to the allocated block and its size.
	 *		 Returns {nullptr, 0} if allocation fails.
	 * @note The actual size of the allocated block may be larger than requested
	 *	   due to alignment and minimum block size requirements.
	 */
	Blk allocateBlock(std::size_t n) noexcept {
		Lock lock(_mutex);
		n = std::max(n, MIN_BLOCK_SIZE);
		n = alignUp(n);

		FreeBlock* prev = nullptr;
		FreeBlock* current = freeList_;

		while (current) {
			if (current->size >= n) {
				if (current->size >= n + MIN_BLOCK_SIZE) {
					// Split the block
					FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(reinterpret_cast<char*>(current) + n);
					newBlock->size = current->size - n;
					newBlock->next = current->next;

					if (prev) {
						prev->next = newBlock;
					} else {
						freeList_ = newBlock;
					}

					return {current, n};
				} else {
					// Use the entire block
					if (prev) {
						prev->next = current->next;
					} else {
						freeList_ = current->next;
					}

					return {current, current->size};
				}
			}

			prev = current;
			current = current->next;
		}

		return {nullptr, 0}; // Allocation failed
	}

	/**
	 * @brief Deallocates a previously allocated block of memory.
	 * @param blk The block to deallocate, as returned by allocateBlock().
	 * @note If the block does not belong to this allocator, the operation is silently ignored.
	 */
	void deallocateBlock(const Blk& blk) noexcept {
		Lock lock(_mutex);
		if (!ownsBlock(blk)) {
			#ifndef NDEBUG
			std::cerr << "Warning: Attempted to deallocate a block not owned by this allocator." << std::endl;
			#endif
			return; // Silently ignore blocks that don't belong to us
		}

		FreeBlock* newFree = reinterpret_cast<FreeBlock*>(blk.ptr);
		newFree->size = blk.size;

		// Insert the block back into the free list, sorted by address
		FreeBlock** current = &freeList_;
		while (*current && *current < newFree) {
			current = &((*current)->next);
		}

		newFree->next = *current;
		*current = newFree;

		mergeFreeBlocks();
	}

	/**
	 * @brief Checks if a given block of memory is owned by this allocator.
	 * @param blk The block to check.
	 * @return True if the block is within the managed memory range, false otherwise.
	 */
	bool ownsBlock(const Blk& blk) const noexcept {
		Lock lock(_mutex);
		return blk.ptr >= buffer_ &&
			   blk.ptr < (buffer_ + totalSize_);
	}

private:
	/**
	 * @brief Initializes the free list with the entire buffer as a single free block.
	 */
	void initializeFreeList() noexcept {
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

	/**
	 * @brief Merges adjacent free blocks in the free list to reduce fragmentation.
	 */
	void mergeFreeBlocks() noexcept {
		FreeBlock* current = freeList_;
		while (current && current->next) {
			if (reinterpret_cast<char*>(current) + current->size == reinterpret_cast<char*>(current->next)) {
				// Merge adjacent blocks
				current->size += current->next->size;
				current->next = current->next->next;
			} else {
				current = current->next;
			}
		}
	}

	/**
	 * @brief Aligns a size up to the nearest multiple of the maximum alignment requirement.
	 * @param n Size to align.
	 * @return The aligned size.
	 */
	static std::size_t alignUp(std::size_t n) noexcept {
		return (n + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
	}
};

/**
 * @class FreelistMemoryManager
 * @brief Manages a contiguous block of memory, partitioning it among multiple
 *		ContiguousFreeListAllocator instances for subsystems with varying memory needs.
 *
 * This class ensures proper alignment and deallocation of the managed memory and its allocators.
 * It provides thread-safe access to the allocators via the getAllocator() method.
 */
class FreelistMemoryManager {
private:
	/**
	 * @var memoryBlock
	 * @brief Pointer to the contiguous block of memory managed by this instance.
	 */
	void* memoryBlock;

	/**
	 * @var allocators
	 * @brief Vector of pointers to ContiguousFreeListAllocator instances, each managing a portion of memoryBlock.
	 */
	std::vector<ContiguousFreeListAllocator*> allocators;

	/**
	 * @var allocMutex
	 * @brief Mutex ensuring thread-safe access to the allocators.
	 */
	std::mutex allocMutex;

public:
	/**
	 * @brief Constructor initializing the memory manager with a span of subsystem sizes.
	 * @param subsystemSizes Span of sizes, each representing the memory requirement of a subsystem.
	 * @throws std::bad_alloc If memory allocation or alignment fails.
	 *
	 * Calculates the total memory required, allocates it, and initializes a ContiguousFreeListAllocator
	 * for each subsystem, ensuring proper alignment.
	 */
	FreelistMemoryManager(const std::span<const std::size_t>& subsystemSizes) {
		// Calculate total size with alignment
		std::size_t totalSize = 0;
		for (std::size_t size : subsystemSizes) {
			totalSize += sizeof(ContiguousFreeListAllocator) + size;
		}

		memoryBlock = std::aligned_alloc(alignof(ContiguousFreeListAllocator), totalSize);
		if (!memoryBlock) {
			throw std::bad_alloc();
		}

		char* currentPtr = static_cast<char*>(memoryBlock);
		for (std::size_t size : subsystemSizes) {
			// Align the allocator's address
			std::size_t space = totalSize - (currentPtr - static_cast<char*>(memoryBlock));
			void* alignedAllocatorPtr = std::align(alignof(ContiguousFreeListAllocator), sizeof(ContiguousFreeListAllocator), reinterpret_cast<void*&>(currentPtr), space);
			if (!alignedAllocatorPtr) {
				throw std::bad_alloc(); // Alignment failed
			}

			ContiguousFreeListAllocator* allocator = new (alignedAllocatorPtr) ContiguousFreeListAllocator(size, currentPtr + sizeof(ContiguousFreeListAllocator));
			allocators.push_back(allocator);
			currentPtr = static_cast<char*>(alignedAllocatorPtr) + sizeof(ContiguousFreeListAllocator) + size;
		}
	}

	/**
	 * @brief Constructor initializing the memory manager with an initializer list of subsystem sizes.
	 * @param subsystemSizes Initializer list of sizes, each representing the memory requirement of a subsystem.
	 * @throws std::bad_alloc If memory allocation or alignment fails.
	 *
	 * Identical to the span constructor, provided for convenience when subsystem sizes are known at compile-time.
	 */
	FreelistMemoryManager(const std::initializer_list<std::size_t>& subsystemSizes) {
		// Calculate total size with alignment
		std::size_t totalSize = 0;
		for (std::size_t size : subsystemSizes) {
			totalSize += sizeof(ContiguousFreeListAllocator) + size;
		}

		memoryBlock = std::aligned_alloc(alignof(ContiguousFreeListAllocator), totalSize);
		if (!memoryBlock) {
			throw std::bad_alloc();
		}

		char* currentPtr = static_cast<char*>(memoryBlock);
		for (std::size_t size : subsystemSizes) {
			// Align the allocator's address
			std::size_t space = totalSize - (currentPtr - static_cast<char*>(memoryBlock));
			void* alignedAllocatorPtr = std::align(alignof(ContiguousFreeListAllocator), sizeof(ContiguousFreeListAllocator), reinterpret_cast<void*&>(currentPtr), space);
			if (!alignedAllocatorPtr) {
				throw std::bad_alloc(); // Alignment failed
			}

			ContiguousFreeListAllocator* allocator = new (alignedAllocatorPtr) ContiguousFreeListAllocator(size, currentPtr + sizeof(ContiguousFreeListAllocator));
			allocators.push_back(allocator);
			currentPtr = static_cast<char*>(alignedAllocatorPtr) + sizeof(ContiguousFreeListAllocator) + size;
		}
	}

	/**
	 * @brief Destructor, ensuring proper deallocation of managed memory and allocators.
	 */
	~FreelistMemoryManager() {
		for (auto allocator : allocators) {
			allocator->~ContiguousFreeListAllocator();
		}
		std::free(memoryBlock);
	}

	/**
	 * @brief Retrieves a ContiguousFreeListAllocator instance by its index.
	 * @param index Index of the desired allocator.
	 * @return Pointer to the ContiguousFreeListAllocator at the specified index.
	 * @throws std::out_of_range If the index exceeds the number of allocators.
	 *
	 * Thread-safely returns an allocator, allowing for concurrent memory management across subsystems.
	 */
	ContiguousFreeListAllocator* getAllocator(std::size_t index) {
		std::lock_guard<std::mutex> lock(allocMutex);
		if (index >= allocators.size()) {
			throw std::out_of_range("Allocator index out of range");
		}
		return allocators[index];
	}
};


}
}
#endif // ELVFREELIST_HPP
