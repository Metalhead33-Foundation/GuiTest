#include <iostream>
#include <span>
#include <limits>
#include <Elvavena/Util/ElvHeapArray.hpp>
#include <Elvavena/Util/ElvChunkyArray.hpp>

template<class T>
struct Mallocator
{
	typedef T value_type;

	Mallocator() = default;

	template<class U>
	constexpr Mallocator(const Mallocator <U>&) noexcept {}

	[[nodiscard]] T* allocate(std::size_t n)
	{
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
			throw std::bad_array_new_length();

		if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
		{
			report(p, n);
			return p;
		}

		throw std::bad_alloc();
	}

	void deallocate(T* p, std::size_t n) noexcept
	{
		report(p, n, 0);
		std::free(p);
	}
private:
	void report(T* p, std::size_t n, bool alloc = true) const
	{
		std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n
				  << " bytes at " << std::hex << std::showbase
				  << reinterpret_cast<void*>(p) << std::dec << '\n';
	}
};

int main(void)
{
	Elv::Util::UniqueChunkyArray<int,256,Mallocator<std::array<int,256>>> intarr(2);
	std::span<int> intspan(intarr);
	for(int i = 0; i < 600;++i) {
		intarr.emplace_back(i);
	}
	/*for(const auto& it : intspan) {
		std::cout << it << std::endl;
	}*/
	std::cout << "Hello world!" << std::endl;
	return 0;
}
