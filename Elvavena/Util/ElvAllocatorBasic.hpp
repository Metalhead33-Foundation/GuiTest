#ifndef ELVALLOCATORBASIC_HPP
#define ELVALLOCATORBASIC_HPP
#include <cstddef>
#include <concepts>
#include <new>
namespace Elv {
namespace Util {

template <typename Alloc, typename T> concept Allocator = requires(Alloc alloc, T* ptr, std::size_t n, T t) {
	typename Alloc::value_type;               // Allocator must have a value_type
	requires std::same_as<typename Alloc::value_type, T>;

	{ alloc.allocate(n) } -> std::same_as<T*>; // Must have allocate function
	alloc.deallocate(ptr, n);                  // Must have deallocate function
};

template <typename Alloc, typename T> concept ConstructingAllocator = Allocator<Alloc, T> && requires(Alloc alloc, T* ptr, std::size_t n, T t) {
	{ alloc.construct(ptr, t) };               // Optional: Must have construct function
	{ alloc.destroy(ptr) };                    // Optional: Must have destroy function
};

struct Blk
{
	void* ptr;
	size_t size;
};

template <typename Alloc> concept AlexandrescuAllocator = requires(Alloc alloc, std::size_t n, Blk blk, const Blk& blkcref) {
	{ alloc.allocateBlock(n) } -> std::same_as<Blk>;
	{ alloc.deallocateBlock(blkcref) };
	{ alloc.ownsBlock(blkcref) } -> std::same_as<bool>;
};

template <typename Alloc, typename T> requires AlexandrescuAllocator<Alloc> struct AlexandrescuAllocatorAdapter {
	using value_type = T;   // Required by Allocator concept
	Alloc alloc_;           // The underlying AlexandrescuAllocator

	AlexandrescuAllocatorAdapter() = default;

	// Allocate memory for 'n' objects of type T
	T* allocate(std::size_t n) {
		std::size_t total_size = n * sizeof(T);  // Calculate total size needed
		Blk blk = alloc_.allocateBlock(total_size);
		if (!blk.ptr) throw std::bad_alloc();
		return static_cast<T*>(blk.ptr);  // Return the pointer cast to T*
	}

	// Deallocate memory for 'n' objects of type T
	void deallocate(T* ptr, std::size_t n) {
		Blk blk{ static_cast<void*>(ptr), n * sizeof(T) };
		alloc_.deallocateBlock(blk);
	}

	/*
	// Optional for ConstructingAllocator: construct an object of type T at the given pointer
	void construct(T* ptr, const T& value) {
		::new(static_cast<void*>(ptr)) T(value);  // Placement new
	}

	// Optional for ConstructingAllocator: destroy an object of type T at the given pointer
	void destroy(T* ptr) {
		ptr->~T();  // Call the destructor of T
	}
	*/
};

template <typename Alloc, typename T> requires AlexandrescuAllocator<Alloc> struct StaticAlexandrescuAllocatorAdapter {
	using value_type = T;   // Required by Allocator concept
	static Alloc alloc_;           // The underlying AlexandrescuAllocator

	StaticAlexandrescuAllocatorAdapter() = default;

	// Allocate memory for 'n' objects of type T
	T* allocate(std::size_t n) {
		std::size_t total_size = n * sizeof(T);  // Calculate total size needed
		Blk blk = alloc_.allocateBlock(total_size);
		if (!blk.ptr) throw std::bad_alloc();
		return static_cast<T*>(blk.ptr);  // Return the pointer cast to T*
	}

	// Deallocate memory for 'n' objects of type T
	void deallocate(T* ptr, std::size_t n) {
		Blk blk{ static_cast<void*>(ptr), n * sizeof(T) };
		alloc_.deallocateBlock(blk);
	}

	// Optional for ConstructingAllocator: construct an object of type T at the given pointer
	void construct(T* ptr, const T& value) {
		::new(static_cast<void*>(ptr)) T(value);  // Placement new
	}

	// Optional for ConstructingAllocator: destroy an object of type T at the given pointer
	void destroy(T* ptr) {
		ptr->~T();  // Call the destructor of T
	}
};

template <typename Primary, typename Fallback> requires AlexandrescuAllocator<Primary> && AlexandrescuAllocator<Fallback>
struct FallbackAllocator : private Primary, private Fallback {
	Blk allocateBlock(std::size_t n) {
		Blk r = Primary::allocateBlock(n);
		if(!r.ptr) r = Fallback::allocateBlock(n);
		return r;
	}
	void deallocateBlock(const Blk& blk) {
		if(Primary::ownsBlock(blk)) Primary::deallocateBlock(blk);
		else Fallback::deallocateBlock(blk);
	}
	bool ownsBlock(const Blk& blk) {
		return Primary::ownsBlock(blk) || Fallback::ownsBlock(blk);
	}
};

template <size_t threshold, typename SmallAllocator, typename LargeAllocator> requires AlexandrescuAllocator<SmallAllocator> && AlexandrescuAllocator<LargeAllocator>
struct SegregatorAllocator : private SmallAllocator, private LargeAllocator {
	Blk allocateBlock(std::size_t n) {
		if(n <= threshold) return SmallAllocator::allocateBlock(n);
		else return LargeAllocator::allocateBlock(n);
	}
	void deallocateBlock(const Blk& blk) {
		if(blk.size <= threshold) return SmallAllocator::deallocateBlock(blk);
		else return LargeAllocator::deallocateBlock(blk);
	}
	bool ownsBlock(const Blk& blk) {
		return SmallAllocator::ownsBlock(blk) || LargeAllocator::ownsBlock(blk);
	}
};

}
}

#endif // ELVALLOCATORBASIC_HPP
