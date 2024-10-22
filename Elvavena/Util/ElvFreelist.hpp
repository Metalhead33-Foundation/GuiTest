#ifndef ELVFREELIST_HPP
#define ELVFREELIST_HPP
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
//! An Andrei Alexandrescu-style memory allocator that uses a freelist for keeping track of free blocks of memory within the entire memory space.
/*!
	\tparam Size The size of the entire memory space.
*/
template <std::size_t Size> class FixedFreeListAllocator {
private:
	struct FreeBlock {
		std::size_t size;
		FreeBlock* next;
	};

	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);
	alignas(std::max_align_t) char buffer_[Size];
	FreeBlock* freeList_;
	mutable std::recursive_mutex _mutex;
	typedef std::lock_guard<std::recursive_mutex> Lock;

public:
	//! Constructor.
	FixedFreeListAllocator() noexcept : freeList_(nullptr) {
		initializeFreeList();
	}

	//! Allocates memory.
	/*!
	\param n The amount of memory - in bytes - to allocate.
	\return The allocated memory block. Contains a null-pointer and zero size if the allocation was unsuccessful.
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
	//! Deallocates memory.
	/*!
	\param blk A reference to the memory block being allocated. Ensure that the block is owned by the given allocator!
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
	//! Checks if the allocator owns the memory.
	/*!
	\param blk A reference to the memory block being checked.
	\return True if the allocator owns the block of memory, false otherwise.
	*/
	bool ownsBlock(const Blk& blk) const noexcept {
		Lock lock(_mutex);
		return blk.ptr >= buffer_ &&
			   blk.ptr < (buffer_ + Size);
	}

private:
	void initializeFreeList() noexcept {
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = Size;
		freeList_->next = nullptr;
	}

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

	static std::size_t alignUp(std::size_t n) noexcept {
		return (n + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
	}
};
//! An Andrei Alexandrescu-style memory allocator that uses a freelist for keeping track of free blocks of memory within the entire memory space.
/*!
	\tparam Size The size of the entire memory space.
	\tparam id Only used for having separate same-sized allocators.
*/
template <std::size_t Size, int id> class StaticFreeListAllocator {
private:
	static FixedFreeListAllocator<Size> alloc;
public:
	//! Allocates memory.
	/*!
	\param n The amount of memory - in bytes - to allocate.
	\return The allocated memory block. Contains a null-pointer and zero size if the allocation was unsuccessful.
	*/
	Blk allocateBlock(std::size_t n) noexcept {
		return alloc.allocateBlock(n);
	}
	//! Deallocates memory.
	/*!
	\param blk A reference to the memory block being allocated. Ensure that the block is owned by the given allocator!
	*/
	void deallocateBlock(const Blk& blk) noexcept {
		alloc.deallocateBlock(blk);
	}
	//! Checks if the allocator owns the memory.
	/*!
	\param blk A reference to the memory block being checked.
	\return True if the allocator owns the block of memory, false otherwise.
	*/
	bool ownsBlock(const Blk& blk) const noexcept {
		return alloc.ownsBlock(blk);
	}
};
template <std::size_t Size, int id> FixedFreeListAllocator<Size> StaticFreeListAllocator<Size, id>::alloc;

//! An Andrei Alexandrescu-style memory allocator that uses a freelist for keeping track of free blocks of memory within the entire memory space.
/*!
	\tparam BaseAlloc The underlying allocator for allocating the memory space.
*/
template <typename BaseAlloc = std::allocator<std::byte>> requires Allocator<BaseAlloc,std::byte> struct DynamicFreeListAllocator {
private:
	struct FreeBlock {
		std::size_t size;
		FreeBlock* next;
	};

	//! The underlying allocator for allocating the memory space.
	BaseAlloc baseAlloc;
	//! Size for alignment.
	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);
	//! The total memory space's size.
	size_t totalSize_;
	//! Pointer to the size of the memory space.
	char* buffer_;
	//! Pointer to the first block of free memory.
	FreeBlock* freeList_;
	//! A mutex for keeping it all thread-safe.
	mutable std::recursive_mutex _mutex;
	//! A mutex for keeping it all thread-safe.
	typedef std::lock_guard<std::recursive_mutex> Lock;
public:
	//! Constructor.
	/*!
	\param totalSize The size of the total memory space.
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
	//! Allocates memory.
	/*!
	\param n The amount of memory - in bytes - to allocate.
	\return The allocated memory block. Contains a null-pointer and zero size if the allocation was unsuccessful.
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
	//! Deallocates memory.
	/*!
	\param blk A reference to the memory block being allocated. Ensure that the block is owned by the given allocator!
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
	//! Checks if the allocator owns the memory.
	/*!
	\param blk A reference to the memory block being checked.
	\return True if the allocator owns the block of memory, false otherwise.
	*/
	bool ownsBlock(const Blk& blk) const noexcept {
		Lock lock(_mutex);
		return blk.ptr >= buffer_ &&
			   blk.ptr < (buffer_ + totalSize_);
	}

private:
	void initializeFreeList() noexcept {
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

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

	static std::size_t alignUp(std::size_t n) noexcept {
		return (n + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
	}
};

// VERY DANGEROUS! USE CAUTIOUSLY!
class ContiguousFreeListAllocator {
private:
	struct FreeBlock {
		std::size_t size;
		FreeBlock* next;
	};

	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);
	std::size_t totalSize_;
	char* buffer_;
	FreeBlock* freeList_;
	mutable std::recursive_mutex _mutex;
	typedef std::lock_guard<std::recursive_mutex> Lock;

public:
	ContiguousFreeListAllocator(std::size_t totalSize, char* buffer)
		: totalSize_(totalSize), buffer_(buffer), freeList_(nullptr)
	{
		initializeFreeList();
	}

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

	bool ownsBlock(const Blk& blk) const noexcept {
		Lock lock(_mutex);
		return blk.ptr >= buffer_ &&
			   blk.ptr < (buffer_ + totalSize_);
	}

private:
	void initializeFreeList() noexcept {
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

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

	static std::size_t alignUp(std::size_t n) noexcept {
		return (n + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
	}
};

class FreelistMemoryManager {
private:
	void* memoryBlock;
	std::vector<ContiguousFreeListAllocator*> allocators;
	std::mutex allocMutex; // For thread safety

public:
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

	~FreelistMemoryManager() {
		for (auto allocator : allocators) {
			allocator->~ContiguousFreeListAllocator();
		}
		std::free(memoryBlock);
	}

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
