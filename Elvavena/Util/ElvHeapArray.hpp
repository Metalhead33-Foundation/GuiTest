#ifndef ELVHEAPARRAY_HPP
#define ELVHEAPARRAY_HPP
#include <memory>
#include <cstddef>
#include <iterator>
#include <span>
#include <atomic>
#include <Elvavena/Util/ElvContinuousIterator.hpp>
namespace Elv {
namespace Util {

template<class T, class Allocator = std::allocator<T>> class UniqueHeapArray {
public:
	typedef T value_type;
	typedef Allocator allocator_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef std::span<T> span;
	typedef std::span<const T> const_span;

	typedef continuous_iterator<T> iterator;
	typedef continuous_iterator<const T> const_iterator;

	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
	T* m_data;
	size_t m_size;
	// Copy constructor
	UniqueHeapArray(const UniqueHeapArray& cpy) = delete;
	UniqueHeapArray& operator=(const UniqueHeapArray& cpy) = delete;
public:
	// Constructor
	UniqueHeapArray(size_t size) : m_size(size) {
		m_data = Allocator().allocate(size);
	}
	// Destructor
	~UniqueHeapArray() {
		if(m_data) Allocator().deallocate(m_data, m_size);
	}
	// Move constructor
	UniqueHeapArray(UniqueHeapArray&& mov) {
		m_data = mov.m_data;
		mov.m_data = nullptr;
		m_size = mov.m_size;
		mov.m_size = 0;
	}
	UniqueHeapArray& operator=(UniqueHeapArray&& mov) {
		m_data = mov.m_data;
		mov.m_data = nullptr;
		m_size = mov.m_size;
		mov.m_size = 0;
		return *this;
	}

	operator span() { return span(m_data, m_size); }
	operator const_span() const { return const_span(m_data, m_size); }

	// Size of the array
	size_t size() const { return m_size; }

	// Access data
	T* data() { return m_data; }
	const T* data() const { return m_data; }

	reference operator[](size_type i) { return m_data[i]; }
	const_reference operator[](size_type i) const { return m_data[i]; }
	reference back() { return m_data[m_size-1]; }
	reference front() { return *m_data ;}
	const_reference back() const { return m_data[m_size-1]; }
	const_reference front() const { return *m_data ;}

	// Begin and end functions for iterator
	iterator begin() { return iterator(m_data); }
	iterator end() { return iterator(m_data + m_size); }
	const_iterator begin() const { return iterator(m_data); }
	const_iterator end() const { return iterator(m_data + m_size); }
	// Begin and end functions for reverse_iterator
	reverse_iterator rbegin() { return iterator(m_data + m_size); }
	reverse_iterator rend() { return iterator(m_data); }
	const_reverse_iterator rbegin() const { return iterator(m_data + m_size); }
	const_reverse_iterator rend() const { return iterator(m_data); }
	// Capacity
	bool empty() const { return m_size == 0; }
};

template<class T, class Allocator = std::allocator<T>> class SharedHeapArray {
public:
	typedef T value_type;
	typedef Allocator allocator_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef std::span<T> span;
	typedef std::span<const T> const_span;
	typedef continuous_iterator<T> iterator;
	typedef continuous_iterator<const T> const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
private:
	struct Container {
		T* m_data;
		size_t m_size;
		std::atomic<size_t> m_refCount;
		Container(size_t size) : m_size(size), m_refCount(1) {
			m_data = Allocator().allocate(size);
		}
		~Container() {
			Allocator().deallocate(m_data, m_size);
		}
	};
	Container* m_container;
public:
	// Constructor
	SharedHeapArray(size_t size) {
		m_container = new Container(size);
	}
	// Destructor
	~SharedHeapArray() {
		if(m_container) {
			--m_container->m_refCount;
			if(m_container->m_refCount <= 0) {
				delete m_container;
			}
		}
	}
	// Move constructor
	SharedHeapArray(SharedHeapArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
	}
	SharedHeapArray& operator=(SharedHeapArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
		return *this;
	}
	SharedHeapArray(const SharedHeapArray& cpy) {
		m_container = cpy.m_container;
		++m_container->m_refCount;
	}
	SharedHeapArray& operator=(const SharedHeapArray& cpy)  {
		m_container = cpy.m_container;
		++m_container->m_refCount;
		return *this;
	}

	operator span() { return span(m_container->m_data, m_container->m_size); }
	operator const_span() const { return const_span(m_container->m_data, m_container->m_size); }

	// Size of the array
	size_t size() const { return m_container->m_size; }

	// Access data
	T* data() { return m_container->m_data; }
	const T* data() const { return m_container->m_data; }

	reference operator[](size_type i) { return m_container->m_data[i]; }
	const_reference operator[](size_type i) const { return m_container->m_data[i]; }
	reference back() { return m_container->m_data[m_container->m_size-1]; }
	reference front() { return *(m_container->m_data);}
	const_reference back() const { return m_container->m_data[m_container->m_size-1]; }
	const_reference front() const { return *(m_container->m_data);}

	// Begin and end functions for iterator
	iterator begin() { return iterator(m_container->m_data); }
	iterator end() { return iterator(m_container->m_data + m_container->m_size); }
	const_iterator begin() const { return iterator(m_container->m_data); }
	const_iterator end() const { return iterator(m_container->m_data + m_container->m_size); }
	// Begin and end functions for reverse_iterator
	reverse_iterator rbegin() { return iterator(m_container->m_data + m_container->m_size); }
	reverse_iterator rend() { return iterator(m_container->m_data); }
	const_reverse_iterator rbegin() const { return iterator(m_container->m_data + m_container->m_size); }
	const_reverse_iterator rend() const { return iterator(m_container->m_data); }
	// Capacity
	bool empty() const { return m_container->m_size == 0; }
};

}
}
#endif // ELVHEAPARRAY_HPP
