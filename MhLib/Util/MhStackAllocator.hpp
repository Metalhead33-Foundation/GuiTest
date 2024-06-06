#ifndef MHSTACKALLOCATOR_HPP
#define MHSTACKALLOCATOR_HPP
#include <cstddef>
#include <cstdint>
#include <array>
namespace MH33 {
namespace Util {

template <typename T, size_t N> struct StackAllocator {
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
	typedef StackAllocator allocator_type;
	typedef std::false_type propagate_on_container_copy_assignment;
	typedef std::false_type propagate_on_container_move_assignment;
	typedef std::false_type propagate_on_container_swap;
	typedef std::false_type is_always_equal;
	template <typename U>
	struct rebind {
		using other = StackAllocator<U, N>;
	};
private:
	//std::array<T,N> buffer;
	T buffer[N];
	size_t offset;
public:
	StackAllocator() : offset(0) {

	}
	template<class U, size_t N2> constexpr StackAllocator(const StackAllocator <U, N2>&) noexcept {}
	[[nodiscard]] pointer allocate(size_t n) {
		pointer toReturn = &buffer[offset];
		offset += n;
		return toReturn;
	}
	void deallocate(pointer p, size_t n) {
		offset -= n;
	}
	friend bool operator==(const StackAllocator& lhs, const StackAllocator& rhs) {
		return lhs.buffer.data() == rhs.buffer.data();
	}
	friend bool operator!=(const StackAllocator& lhs, const StackAllocator& rhs) {
		return lhs.buffer.data() != rhs.buffer.data();
	}

};

}
}
#endif // MHSTACKALLOCATOR_HPP
