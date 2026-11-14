#ifndef ELVALEXANDRESCURESOURCE_HPP
#define ELVALEXANDRESCURESOURCE_HPP
/**
 * @file ElvAlexandrescuResource.hpp
 * @brief Declares the ElvAlexandrescuResource API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <new>
#include <utility>
namespace Elv {
namespace Util {

/**
 * @brief A custom memory resource that uses an allocator conforming to Alexandrescu's allocator requirements.
 *
 * This class implements the `std::pmr::memory_resource` interface using a custom allocator.
 * The allocator must satisfy the requirements of Alexandrescu's allocator.
 *
 * The underlying allocator only exposes block-granularity allocation, so this
 * resource over-allocates and stores an `AllocationHeader` in front of every
 * returned pointer. That header records the original block pointer and its
 * actual size, which lets the resource honor arbitrary `std::pmr` alignment
 * requests and return the correct block to the allocator on deallocation.
 * The cost is one small header per allocation; use this resource for chunky,
 * arena-style allocations rather than for many tiny objects.
 *
 * @tparam Alloc The allocator type that must conform to Alexandrescu's allocator requirements.
 */
template <class Alloc>
	requires AlexandrescuAllocator<Alloc>
class AlexandrescuResource : public std::pmr::memory_resource {
private:
	/// The allocator instance used for memory allocation and deallocation.
	Alloc alloc;

	/**
	 * @brief Bookkeeping stored immediately before every returned allocation.
	 */
	struct AllocationHeader {
		void* base;       ///< Pointer returned by the underlying allocator.
		std::size_t size; ///< Actual block size returned by the underlying allocator.
	};

	/**
	 * @brief Aligns an address up to the requested power-of-two alignment.
	 * @param value Address to align.
	 * @param alignment Required alignment; must be a power of two.
	 * @return The aligned address.
	 */
	static std::uintptr_t alignUp(std::uintptr_t value, std::size_t alignment) noexcept
	{
		return (value + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
	}

	/**
	 * @brief Allocates memory with the requested size and alignment.
	 *
	 * @param __bytes Number of bytes requested by the PMR container.
	 * @param __alignment Required alignment of the returned pointer.
	 * @return A pointer with the requested alignment.
	 * @throws std::bad_alloc When the request overflows or the underlying allocator cannot satisfy it.
	 */
	void* do_allocate(std::size_t __bytes, std::size_t __alignment) override
	{
		const std::size_t alignment = __alignment != 0 ? __alignment : alignof(void*);
		if (__bytes > std::numeric_limits<std::size_t>::max() - alignment - sizeof(AllocationHeader))
			throw std::bad_alloc();

		const std::size_t total = __bytes + alignment + sizeof(AllocationHeader);
		Blk block = alloc.allocateBlock(total);
		if (block.ptr == nullptr)
			throw std::bad_alloc();

		const std::uintptr_t start = reinterpret_cast<std::uintptr_t>(block.ptr) + sizeof(AllocationHeader);
		const std::uintptr_t aligned = alignUp(start, alignment);
		AllocationHeader* header = reinterpret_cast<AllocationHeader*>(aligned - sizeof(AllocationHeader));
		header->base = block.ptr;
		header->size = block.size;
		return reinterpret_cast<void*>(aligned);
	}

	/**
	 * @brief Deallocates memory previously returned by this resource.
	 *
	 * The original base pointer and block size are read from the allocation
	 * header, so the caller-supplied size and alignment are not used.
	 *
	 * @param __p A pointer previously returned by @ref do_allocate.
	 * @param __bytes Caller-reported size (unused).
	 * @param __alignment Caller-reported alignment (unused).
	 */
	void do_deallocate(void* __p, std::size_t __bytes, std::size_t __alignment) override
	{
		(void)__bytes;
		(void)__alignment;
		if (__p == nullptr)
			return;

		const AllocationHeader* header = reinterpret_cast<const AllocationHeader*>(
			reinterpret_cast<const char*>(__p) - sizeof(AllocationHeader));
		Blk block { header->base, header->size };
		alloc.deallocateBlock(block);
	}

	/**
	 * @brief Checks if this resource is equal to another memory resource.
	 *
	 * This method overrides the `do_is_equal` method from `std::pmr::memory_resource`.
	 * Two `AlexandrescuResource` instances are considered equal if they are the same object.
	 *
	 * @param __other The memory resource to compare with.
	 * @return `true` if the resources are the same object, `false` otherwise.
	 */
	bool do_is_equal(const memory_resource& __other) const noexcept override
	{
		return static_cast<const void*>(this) == static_cast<const void*>(&__other);
	}

public:
	/**
	 * @brief Constructs an `AlexandrescuResource` with the specified allocator arguments.
	 *
	 * This constructor forwards the provided arguments to the allocator's constructor.
	 *
	 * @tparam Args The types of the arguments to be forwarded to the allocator.
	 * @param args The arguments to be forwarded to the allocator.
	 */
	template<class... Args>
	AlexandrescuResource(Args&&... args)
		: alloc(std::forward<Args>(args)...)
	{
	}
};

/** @brief Backwards-compatible alias for the older misspelled type name. */
template <class Alloc>
	requires AlexandrescuAllocator<Alloc>
using AlexandrescsuResource = AlexandrescuResource<Alloc>;

}
}

#endif // ELVALEXANDRESCURESOURCE_HPP
