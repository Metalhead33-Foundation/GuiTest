#include <iostream>
#include <span>
#include <limits>
#include <Elvavena/Util/ElvHeapArray.hpp>
#include <Elvavena/Util/ElvChunkyArray.hpp>
#include <Elvavena/Util/ElvIntegralIterator.hpp>
#include <Elvavena/Util/ElvFreelist.hpp>
#include <Elvavena/Util/ElvBitmapAllocator.hpp>
#include <vector>

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

enum class AllocatorSubsystem {
	AUDIO,
	VIDEO,
	SCRIPT
};
struct WodMemoryManager {
	typedef std::unique_ptr<Elv::Util::FreelistMemoryManager> uManager;
private:
	static uManager manager;
public:
	static Elv::Util::FreelistMemoryManager& getStaticManager() {
		if(!manager) {
			manager = uManager(new Elv::Util::FreelistMemoryManager(
								   { 1024*8, 1024, 1024 }
								   ));
		}
		return *manager;
	}
};
WodMemoryManager::uManager WodMemoryManager::manager = nullptr;
template <AllocatorSubsystem subsysId> struct WodMemoryManagerSubsystem {
private:
	static Elv::Util::ContiguousFreeListAllocator* alloc;
public:
	WodMemoryManagerSubsystem() {
		if(!alloc) {
			alloc = WodMemoryManager::getStaticManager().getAllocator(static_cast<size_t>(subsysId));
		}
	}
	Elv::Util::Blk allocateBlock(std::size_t n) noexcept {
		return alloc->allocateBlock(n);
	}
	void deallocateBlock(const Elv::Util::Blk& blk) noexcept {
		alloc->deallocateBlock(blk);
	}
	bool ownsBlock(const Elv::Util::Blk& blk) const noexcept {
		return alloc->ownsBlock(blk);
	}
};
template <AllocatorSubsystem subsysId> Elv::Util::ContiguousFreeListAllocator* WodMemoryManagerSubsystem<subsysId>::alloc = nullptr;

//typedef Elv::Util::AlexandrescuAllocatorAdapter<Elv::Util::StaticBitmapAllocator<8,1024>,int> IntAllocator;
//typedef Elv::Util::AlexandrescuAllocatorAdapter<Elv::Util::StaticFreeListAllocator<sizeof(int)*20000>,int> IntAllocator;
typedef Elv::Util::AlexandrescuAllocatorAdapter<WodMemoryManagerSubsystem<AllocatorSubsystem::AUDIO>,int> IntAllocator;

int main(void)
{
	//Elv::Util::UniqueChunkyArray<int,256,Mallocator<std::array<int,256>>> intarr(2);
	std::vector<int,IntAllocator> intarr(2);
	intarr.reserve(600);
	for(int i = 0; i < 600;++i) {
		intarr.emplace_back(i);
	}
	std::span<int> intspan(intarr);
	for(const auto& it : intspan) {
		std::cout << it << std::endl;
	}
	/*Elv::Util::IntegralIterable<int> range(10);
	Elv::Util::IntegralIterable<int> range2(20);
	range = range2;
	for(auto it = std::rbegin(range); it != std::rend(range); ++it) {
		std::cout << *it << std::endl;
	}*/

	/*for(const auto& it : range) {
		std::cout << it << std::endl;
	}*/
	std::cout << "Hello world!" << std::endl;
	return 0;
}
