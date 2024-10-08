#ifndef ELVFREELIST_HPP
#define ELVFREELIST_HPP
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <cassert>
#include <vector>
#include <span>
#include <stdexcept>
#include <initializer_list>
#include <iostream>
namespace Elv {
namespace Util {

/*struct Freelist {
	struct ll_head {
		 ll_head *next;
		 ll_head *prev;
	};
	struct alloc_node_t {
		 ll_head node;
		 size_t size;
		 std::byte* block;
	};
};*/

// FreeListAllocator conforms to AlexandrescuAllocator
template <std::size_t Size> class FixedFreeListAllocator {
private:
	struct FreeBlock {
		std::size_t size;
		FreeBlock* next;
	};

	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);
	alignas(std::max_align_t) char buffer_[Size];
	FreeBlock* freeList_;

public:
	FixedFreeListAllocator() noexcept : freeList_(nullptr) {
		initializeFreeList();
	}

	Blk allocateBlock(std::size_t n) noexcept {
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

	bool ownsBlock(const Blk& blk) const noexcept {
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
template <std::size_t Size> class StaticFreeListAllocator {
private:
	static FixedFreeListAllocator<Size> alloc;
public:
	Blk allocateBlock(std::size_t n) noexcept {
		return alloc.allocateBlock(n);
	}
	void deallocateBlock(const Blk& blk) noexcept {
		alloc.deallocateBlock(blk);
	}
	bool ownsBlock(const Blk& blk) const noexcept {
		return alloc.ownsBlock(blk);
	}
};
template <std::size_t Size> FixedFreeListAllocator<Size> StaticFreeListAllocator<Size>::alloc;

// FreeListAllocator conforms to AlexandrescuAllocator
template <typename BaseAlloc = std::allocator<std::byte>> requires Allocator<BaseAlloc,std::byte> struct DynamicFreeListAllocator {
private:
	struct FreeBlock {
		std::size_t size;
		FreeBlock* next;
	};

	BaseAlloc baseAlloc;
	static constexpr std::size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);
	size_t totalSize_;
	char* buffer_;
	FreeBlock* freeList_;

public:
	DynamicFreeListAllocator(std::size_t totalSize)
			: buffer_(baseAlloc.allocate(totalSize)),
			  totalSize_(totalSize)
	{
		// Initialize the free list with one large free block
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

	Blk allocateBlock(std::size_t n) noexcept {
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

	bool ownsBlock(const Blk& blk) const noexcept {
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

public:
	ContiguousFreeListAllocator(std::size_t totalSize)
		: totalSize_(totalSize), buffer_(reinterpret_cast<char*>(this) + sizeof(ContiguousFreeListAllocator))
	{
		initializeFreeList();
	}

	Blk allocateBlock(std::size_t n) noexcept {
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

	bool ownsBlock(const Blk& blk) const noexcept {
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

public:
	FreelistMemoryManager(const std::initializer_list<std::size_t>& subsystemSizes) {
		std::size_t totalSize = sizeof(ContiguousFreeListAllocator) * subsystemSizes.size();
		for (std::size_t size : subsystemSizes) {
			totalSize += size;
		}

		memoryBlock = std::malloc(totalSize);
		if (!memoryBlock) {
			throw std::bad_alloc();
		}

		char* currentPtr = static_cast<char*>(memoryBlock);
		for (std::size_t size : subsystemSizes) {
			ContiguousFreeListAllocator* allocator = new (currentPtr) ContiguousFreeListAllocator(size);
			allocators.push_back(allocator);
			currentPtr += sizeof(ContiguousFreeListAllocator) + size;
		}
	}
	FreelistMemoryManager(const std::span<const std::size_t>& subsystemSizes) {
		std::size_t totalSize = sizeof(ContiguousFreeListAllocator) * subsystemSizes.size();
		for (std::size_t size : subsystemSizes) {
			totalSize += size;
		}

		memoryBlock = std::malloc(totalSize);
		if (!memoryBlock) {
			throw std::bad_alloc();
		}

		char* currentPtr = static_cast<char*>(memoryBlock);
		for (std::size_t size : subsystemSizes) {
			ContiguousFreeListAllocator* allocator = new (currentPtr) ContiguousFreeListAllocator(size);
			allocators.push_back(allocator);
			currentPtr += sizeof(ContiguousFreeListAllocator) + size;
		}
	}

	~FreelistMemoryManager() {
		for (auto allocator : allocators) {
			allocator->~ContiguousFreeListAllocator();
		}
		std::free(memoryBlock);
	}

	ContiguousFreeListAllocator* getAllocator(std::size_t index) {
		if (index >= allocators.size()) {
			throw std::out_of_range("Allocator index out of range");
		}
		return allocators[index];
	}
};


}
}
#endif // ELVFREELIST_HPP
