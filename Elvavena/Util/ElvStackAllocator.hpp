#ifndef ELVSTACKALLOCATOR_H
#define ELVSTACKALLOCATOR_H
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <array>
#include <cassert>
#include <stdexcept>
namespace Elv {
namespace Util {

template <size_t Size, int id = 1> struct StaticStackAllocator {
private:
	static std::size_t offset_;
	static std::array<std::byte, Size> underlyingBuffer;
public:
	Blk allocateBlock(std::size_t n) {
		if (offset_ + n > Size) {
			return { nullptr, 0 }; // Allocation failed
		}
		void* ptr = static_cast<std::byte*>(underlyingBuffer.data()) + offset_;
		Blk blk = { ptr, n };
		offset_ += n;
		return blk;
	}
	void deallocateBlock(const Blk& blk) {
		assert(static_cast<std::byte*>(blk.ptr) == static_cast<std::byte*>(underlyingBuffer.data()) + (offset_ - blk.size));
		if (static_cast<std::byte*>(blk.ptr) != static_cast<std::byte*>(underlyingBuffer.data()) + (offset_ - blk.size)) {
			throw std::invalid_argument("StackAllocator can only deallocate the most recent block.");
		}
		offset_ -= blk.size;
	}

	bool ownsBlock(const Blk& blk) const {
		return blk.ptr >= underlyingBuffer.data() && blk.ptr < static_cast<std::byte*>(underlyingBuffer.data()) + Size;
	}
};
template <size_t Size, int id> std::size_t StaticStackAllocator<Size,id>::offset_ = 0;
template <typename BaseAlloc = std::allocator<std::byte>> requires Allocator<BaseAlloc,std::byte> struct DymnamicStackAllocator {
private:
	BaseAlloc baseAlloc;
	std::byte* buffer_;
	std::size_t totalSize_;
	std::size_t offset_;
public:
	DymnamicStackAllocator(std::size_t totalSize)
		: buffer_(baseAlloc.allocate(totalSize)),
		  totalSize_(totalSize),
		  offset_(0)
	{

	}
	~DymnamicStackAllocator() {
		baseAlloc.deallocate(buffer_, totalSize_);
	}
	Blk allocateBlock(std::size_t n) {
		if (offset_ + n > totalSize_) {
			return { nullptr, 0 }; // Allocation failed
		}
		void* ptr = static_cast<std::byte*>(buffer_) + offset_;
		Blk blk = { ptr, n };
		offset_ += n;
		return blk;
	}
	void deallocateBlock(const Blk& blk) {
		assert(static_cast<std::byte*>(blk.ptr) == static_cast<std::byte*>(buffer_) + (offset_ - blk.size));
		if (static_cast<std::byte*>(blk.ptr) != static_cast<std::byte*>(buffer_) + (offset_ - blk.size)) {
			throw std::invalid_argument("StackAllocator can only deallocate the most recent block.");
		}
		offset_ -= blk.size;
	}

	bool ownsBlock(const Blk& blk) const {
		return blk.ptr >= buffer_ && blk.ptr < static_cast<std::byte*>(buffer_) + totalSize_;
	}
};

}
}
#endif // ELVSTACKALLOCATOR_H
