#ifndef ELVBITMAPALLOCATOR_HPP
#define ELVBITMAPALLOCATOR_HPP
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <Elvavena/Util/ElvMathUtil.hpp>
#include <bitset>
#include <stdexcept>
namespace Elv {
namespace Util {

// BitmapAllocator conforms to AlexandrescuAllocator
template <std::size_t BlockSize, std::size_t NumBlocks>
struct BitmapAllocator {
	static_assert(BlockSize > 0, "BlockSize must be greater than 0");
	static_assert(NumBlocks > 0, "NumBlocks must be greater than 0");
private:
	std::bitset<NumBlocks> bitmap_;
	char buffer_[BlockSize * NumBlocks];
	bool canFitIntoSpace(size_t startingIndex, size_t blockCount) {
		if((startingIndex+blockCount) <= NumBlocks) return false;
		for(size_t i = 0; (i < blockCount) && ((startingIndex+i) < NumBlocks); ++i) {
			if(bitmap_.test(startingIndex+i)) return false;
		}
		return true;
	}
public:
	static constexpr size_t getBlockCount(std::size_t bytes) {
		return div_ceil(bytes, BlockSize);
	}
	BitmapAllocator()
		: bitmap_() // All bits initialized to 0 (free)
	{

	}
	Blk allocateBlock(std::size_t n) {
		const size_t blocksToAllocate = getBlockCount(n);
		if (blocksToAllocate > NumBlocks) {
			return { nullptr, 0 }; // Allocation request exceeds block count
		}
		for (std::size_t i = 0; i < NumBlocks; ++i) {
			if (!bitmap_.test(i)) {
				if(canFitIntoSpace(i,blocksToAllocate)) {
					for(size_t k = 0; k < blocksToAllocate; ++k) bitmap_.set(i+k);
					void* ptr = &buffer_[i * BlockSize];
					return { ptr, n };
				}
			}
		}
		return { nullptr, 0 }; // No free blocks available
	}

	void deallocateBlock(const Blk& blk) {
		if (!ownsBlock(blk)) {
		}
		std::size_t index = (reinterpret_cast<size_t>(blk.ptr) - reinterpret_cast<size_t>(&buffer_[0])) / BlockSize;
		if (index >= NumBlocks || !bitmap_.test(index) ) {
			throw std::invalid_argument("Block not allocated or out of range.");
		}
		const size_t blocksToDeallocate = getBlockCount(blk.size);
		for(size_t i = 0; i < blocksToDeallocate; ++i) {
			bitmap_.reset(index + i);
		}
	}

	bool ownsBlock(const Blk& blk) {
		return blk.ptr >= buffer_ &&
			   blk.ptr < static_cast<char*>(buffer_) + (BlockSize * NumBlocks) &&
			   (static_cast<char*>(blk.ptr) - static_cast<char*>(buffer_)) % BlockSize == 0;
	}
};

template <std::size_t BlockSize, std::size_t NumBlocks> struct StaticBitmapAllocator {
private:
	static BitmapAllocator<BlockSize,NumBlocks> _allocator;
public:
	Blk allocateBlock(std::size_t n) {
		return _allocator.allocateBlock(n);
	}
	void deallocateBlock(const Blk& blk) {
		_allocator.deallocateBlock(blk);
	}
	bool ownsBlock(const Blk& blk) {
		return _allocator.ownsBlock(blk);
	}
};
template <std::size_t BlockSize, std::size_t NumBlocks> BitmapAllocator<BlockSize,NumBlocks> StaticBitmapAllocator<BlockSize,NumBlocks>::_allocator;

}
}
#endif // ELVBITMAPALLOCATOR_HPP
