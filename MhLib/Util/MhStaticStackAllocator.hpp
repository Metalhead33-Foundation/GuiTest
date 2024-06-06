#ifndef MHSTATICSTACKALLOCATOR_HPP
#define MHSTATICSTACKALLOCATOR_HPP
#include <cstddef>
#include <cstdint>
#include <array>
namespace MH33 {
namespace Util {

template <typename T, size_t N, int id> struct StaticStackAllocator {
public:
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef void* void_pointer;
	typedef const void* const_void_pointer;
	typedef T value_type;
	typedef uintptr_t size_type;
	typedef intptr_t difference_type;
	typedef StaticStackAllocator allocator_type;
	typedef std::false_type propagate_on_container_copy_assignment;
	typedef std::false_type propagate_on_container_move_assignment;
	typedef std::false_type propagate_on_container_swap;
	typedef std::true_type is_always_equal;
	template <typename U>
	struct rebind {
		using other = StaticStackAllocator<U, N, id>;
	};
private:
	//static std::array<T,N> buffer;
	static T buffer[N];
	static size_t offset;
public:
	StaticStackAllocator() {

	}
	template<class U, size_t N2, int id2> constexpr StaticStackAllocator(const StaticStackAllocator <U, N2, id2>&) noexcept {}
	[[nodiscard]] pointer allocate(size_t n) {
		pointer toReturn = &buffer[offset];
		offset += n;
		return toReturn;
	}
	void deallocate(pointer p, size_t n) {
		offset -= n;
	}
	friend bool operator==(const StaticStackAllocator& lhs, const StaticStackAllocator& rhs) {
		return true;
	}
	friend bool operator!=(const StaticStackAllocator& lhs, const StaticStackAllocator& rhs) {
		return false;
	}
};

}
}
#endif // MHSTATICSTACKALLOCATOR_HPP
