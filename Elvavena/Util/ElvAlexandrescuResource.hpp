#ifndef ELVALEXANDRESCURESOURCE_HPP
#define ELVALEXANDRESCURESOURCE_HPP
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <memory_resource>
namespace Elv {
namespace Util {

/**
 * @brief A custom memory resource that uses an allocator conforming to Alexandrescu's allocator requirements.
 *
 * This class implements the `std::pmr::memory_resource` interface using a custom allocator.
 * The allocator must satisfy the requirements of Alexandrescu's allocator.
 *
 * @tparam Alloc The allocator type that must conform to Alexandrescu's allocator requirements.
 */
template <class Alloc>
	requires AlexandrescuAllocator<Alloc>
class AlexandrescsuResource : public std::pmr::memory_resource {
private:
	/// The allocator instance used for memory allocation and deallocation.
	Alloc alloc;

	/**
	 * @brief Allocates memory of a specified size and alignment.
	 *
	 * This method overrides the `do_allocate` method from `std::pmr::memory_resource`.
	 * The alignment parameter is ignored in this implementation.
	 *
	 * @param __bytes The number of bytes to allocate.
	 * @param __alignment The required alignment of the allocated memory (ignored).
	 * @return A pointer to the allocated memory.
	 */
	void* do_allocate(std::size_t __bytes, std::size_t __alignment) override
	{
		(void)__alignment; // Ignore alignment
		Blk block = alloc.allocateBlock(__bytes);
		return block.ptr;
	}

	/**
	 * @brief Deallocates memory previously allocated by this resource.
	 *
	 * This method overrides the `do_deallocate` method from `std::pmr::memory_resource`.
	 * The alignment parameter is ignored in this implementation.
	 *
	 * @param __p A pointer to the memory to be deallocated.
	 * @param __bytes The size of the memory block to be deallocated.
	 * @param __alignment The alignment of the memory block (ignored).
	 */
	void do_deallocate(void* __p, std::size_t __bytes, std::size_t __alignment) override
	{
		(void)__alignment; // Ignore alignment
		Blk block { __p, __bytes };
		alloc.deallocateBlock(block);
	}

	/**
	 * @brief Checks if this resource is equal to another memory resource.
	 *
	 * This method overrides the `do_is_equal` method from `std::pmr::memory_resource`.
	 * Two `AlexandrescsuResource` instances are considered equal if they are the same object.
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
	 * @brief Constructs an `AlexandrescsuResource` with the specified allocator arguments.
	 *
	 * This constructor forwards the provided arguments to the allocator's constructor.
	 *
	 * @tparam Args The types of the arguments to be forwarded to the allocator.
	 * @param args The arguments to be forwarded to the allocator.
	 */
	template<class... Args>
	AlexandrescsuResource(Args&&... args)
		: alloc(std::forward<Args>(args)...)
	{
	}
};

}
}

#endif // ELVALEXANDRESCURESOURCE_HPP
