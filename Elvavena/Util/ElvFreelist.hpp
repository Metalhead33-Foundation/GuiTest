#ifndef ELVFREELIST_HPP
#define ELVFREELIST_HPP
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <array>
#include <cassert>
#include <stdexcept>
namespace Elv {
namespace Util {

// FreeListAllocator conforms to AlexandrescuAllocator
template <size_t Size> struct StaticFreeListAllocator {
	StaticFreeListAllocator()
	{
		// Initialize the free list with one large free block
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

	Blk allocateBlock(std::size_t n) {
		FreeBlock* prev = nullptr;
		FreeBlock* current = freeList_;

		while (current) {
			if (current->size >= n) {
				if (current->size > n + sizeof(FreeBlock)) {
					// Split the block
					FreeBlock* newFree = reinterpret_cast<FreeBlock*>(
								reinterpret_cast<char*>(current) + n
					);
					newFree->size = current->size - n;
					newFree->next = current->next;

					if (prev) {
						prev->next = newFree;
					} else {
						freeList_ = newFree;
					}
				} else {
					// Use the entire block
					n = current->size;
					if (prev) {
						prev->next = current->next;
					} else {
						freeList_ = current->next;
					}
				}
				return { current, n };
			}
			prev = current;
			current = current->next;
		}
		return { nullptr, 0 }; // Allocation failed
	}

	void deallocateBlock(const Blk& blk) {
		if (!ownsBlock(blk)) {
			throw std::invalid_argument("Block does not belong to this allocator.");
		}
		// Insert the block back into the free list, sorted by address
		FreeBlock* newFree = reinterpret_cast<FreeBlock*>(blk.ptr);
		newFree->size = blk.size;
		newFree->next = nullptr;

		if (!freeList_) {
			freeList_ = newFree;
			return;
		}

		FreeBlock* prev = nullptr;
		FreeBlock* current = freeList_;

		while (current && current < newFree) {
			prev = current;
			current = current->next;
		}

		newFree->next = current;
		if (prev) {
			prev->next = newFree;
		} else {
			freeList_ = newFree;
		}

		// Optionally, merge adjacent free blocks
		mergeFreeBlocks();
	}

	bool ownsBlock(const Blk& blk) const {
		return blk.ptr >= buffer_ &&
			   blk.ptr < static_cast<char*>(buffer_) + totalSize_;
	}

private:
	struct FreeBlock {
		std::size_t size;
		FreeBlock* next;
	};

	void mergeFreeBlocks() {
		FreeBlock* current = freeList_;
		while (current && current->next) {
			char* currentEnd = reinterpret_cast<char*>(current) + current->size;
			if (currentEnd == reinterpret_cast<char*>(current->next)) {
				current->size += current->next->size;
				current->next = current->next->next;
			} else {
				current = current->next;
			}
		}
	}

	static char buffer_[Size];
	static const std::size_t totalSize_ = Size;
	FreeBlock* freeList_;
};
template <size_t Size> char StaticFreeListAllocator<Size>::buffer_[Size];

// FreeListAllocator conforms to AlexandrescuAllocator
template <typename BaseAlloc = std::allocator<std::byte>> requires Allocator<BaseAlloc,std::byte> struct DynamicFreeListAllocator {
	DynamicFreeListAllocator(std::size_t totalSize)
		: buffer_(baseAlloc.allocate(totalSize)),
		  totalSize_(totalSize)
	{
		// Initialize the free list with one large free block
		freeList_ = reinterpret_cast<FreeBlock*>(buffer_);
		freeList_->size = totalSize_;
		freeList_->next = nullptr;
	}

	~DynamicFreeListAllocator() {
		baseAlloc.deallocate(buffer_,totalSize_);
	}

	Blk allocateBlock(std::size_t n) {
		FreeBlock* prev = nullptr;
		FreeBlock* current = freeList_;

		while (current) {
			if (current->size >= n) {
				if (current->size > n + sizeof(FreeBlock)) {
					// Split the block
					FreeBlock* newFree = reinterpret_cast<FreeBlock*>(
						static_cast<char*>(current) + n
					);
					newFree->size = current->size - n;
					newFree->next = current->next;

					if (prev) {
						prev->next = newFree;
					} else {
						freeList_ = newFree;
					}
				} else {
					// Use the entire block
					n = current->size;
					if (prev) {
						prev->next = current->next;
					} else {
						freeList_ = current->next;
					}
				}
				return { current, n };
			}
			prev = current;
			current = current->next;
		}
		return { nullptr, 0 }; // Allocation failed
	}

	void deallocateBlock(const Blk& blk) {
		if (!ownsBlock(blk)) {
			throw std::invalid_argument("Block does not belong to this allocator.");
		}
		// Insert the block back into the free list, sorted by address
		FreeBlock* newFree = reinterpret_cast<FreeBlock*>(blk.ptr);
		newFree->size = blk.size;
		newFree->next = nullptr;

		if (!freeList_) {
			freeList_ = newFree;
			return;
		}

		FreeBlock* prev = nullptr;
		FreeBlock* current = freeList_;

		while (current && current < newFree) {
			prev = current;
			current = current->next;
		}

		newFree->next = current;
		if (prev) {
			prev->next = newFree;
		} else {
			freeList_ = newFree;
		}

		// Optionally, merge adjacent free blocks
		mergeFreeBlocks();
	}

	bool ownsBlock(const Blk& blk) const {
		return blk.ptr >= buffer_ &&
			   blk.ptr < static_cast<char*>(buffer_) + totalSize_;
	}

private:
	struct FreeBlock {
		std::size_t size;
		FreeBlock* next;
	};

	void mergeFreeBlocks() {
		FreeBlock* current = freeList_;
		while (current && current->next) {
			char* currentEnd = static_cast<char*>(current) + current->size;
			if (currentEnd == static_cast<char*>(current->next)) {
				current->size += current->next->size;
				current->next = current->next->next;
			} else {
				current = current->next;
			}
		}
	}

	BaseAlloc baseAlloc;
	void* buffer_;
	std::size_t totalSize_;
	FreeBlock* freeList_;
};

}
}
#endif // ELVFREELIST_HPP
