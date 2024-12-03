#ifndef ELVALEXANDRESCURESOURCE_HPP
#define ELVALEXANDRESCURESOURCE_HPP
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <memory_resource>
namespace Elv {
namespace Util {

template <class Alloc> requires AlexandrescuAllocator<Alloc> class AlexandrescsuResource : public std::pmr::memory_resource {
private:
	Alloc alloc;
	void* do_allocate(std::size_t __bytes, std::size_t __alignment) override
	{
		(void)__alignment;
		Blk block = alloc.allocateBlock(__bytes);
		return block.ptr;
	}
	void do_deallocate(void* __p, std::size_t __bytes, std::size_t __alignment) override
	{
		(void)__alignment;
		Blk block { __p, __bytes };
		alloc.deallocateBlock (block);
	}
	bool do_is_equal(const memory_resource& __other) const noexcept override
	{
		return static_cast<const void*>(this) == static_cast<const void*>(&__other);
	}
public:
	template<class... Args> AlexandrescsuResource(Args&&... args)
		: alloc(std::forward(args)...)
	{

	}
};

}
}

#endif // ELVALEXANDRESCURESOURCE_HPP
