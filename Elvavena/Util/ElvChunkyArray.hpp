#ifndef ELVCHUNKYARRAY_HPP
#define ELVCHUNKYARRAY_HPP
#include <array>
#include <cstring>
#include <span>
#include <atomic>
#include <Elvavena/Util/ElvContinuousIterator.hpp>
#include <Elvavena/Util/ElvMathUtil.hpp>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>

namespace Elv {
namespace Util {

template<class T, size_t ChunkSize = 256, class Alloc = std::allocator<std::array<T,ChunkSize>>>
requires Allocator<Alloc, std::array<T,ChunkSize>> class UniqueChunkyArray {
public:
	typedef std::array<T,ChunkSize> Chunk;
	typedef T value_type;
	typedef Alloc allocator_type;
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
	typedef std::allocator_traits<Alloc> AllocTraits;
private:
	Alloc alloc;
	Chunk* m_data;
	size_t m_chunkCount;
	size_t m_size;
	size_t m_capacity;
	void grow(size_t newChunks) {
		Chunk* newChunkStore = Alloc().allocate(m_chunkCount + newChunks);
		std::memcpy(newChunkStore,m_data,m_chunkCount * sizeof(Chunk));
		alloc.deallocate(m_data, m_chunkCount);
		m_data = newChunkStore;
		m_chunkCount += newChunks;
		m_capacity = m_chunkCount * ChunkSize;
	}
	void shrink(size_t newChunkCount) {
		Chunk* newChunkStore = alloc.allocate(newChunkCount);
		std::memcpy(newChunkStore,m_data,std::min(m_chunkCount,newChunkCount) * sizeof(Chunk));
		Alloc().deallocate(m_data, m_chunkCount);
		m_data = newChunkStore;
		m_chunkCount = newChunkCount;
		m_capacity = m_chunkCount * ChunkSize;
	}
	// Copy constructor
	UniqueChunkyArray(const UniqueChunkyArray& cpy) = delete;
	UniqueChunkyArray& operator=(const UniqueChunkyArray& cpy) = delete;
public:
	// Constructor
	UniqueChunkyArray(size_t size, bool countAsInitialized = false) :
		m_chunkCount(size),
		m_capacity(size * ChunkSize),
		m_size(countAsInitialized ? (size * ChunkSize) : 0) {
		m_data = alloc.allocate(size);
	}
	// Destructor
	~UniqueChunkyArray() {
		if(m_data) {
			for (size_t i = 0; i < m_size; ++i) {
				AllocTraits::destroy(alloc, &m_data[i]); // Call the destructor
			}
			alloc.deallocate(m_data, m_chunkCount);
		}
	}
	UniqueChunkyArray(UniqueChunkyArray&& mov) {
		m_data = mov.m_data;
		m_chunkCount = mov.m_chunkCount;
		m_size = mov.m_size;
		m_capacity = mov.m_capacity;
		mov.m_data = nullptr;
		mov.m_chunkCount = 0;
		mov.m_size = 0;
		mov.m_capacity = 0;
	}
	UniqueChunkyArray& operator=(UniqueChunkyArray&& mov) {
		m_data = mov.m_data;
		m_chunkCount = mov.m_chunkCount;
		m_size = mov.m_size;
		m_capacity = mov.m_capacity;
		mov.m_data = nullptr;
		mov.m_chunkCount = 0;
		mov.m_size = 0;
		mov.m_capacity = 0;
		return *this;
	}

	operator span() { return span(reinterpret_cast<T*>(m_data) , m_size); }
	operator const_span() const { return const_span(reinterpret_cast<const T*>(m_data), m_size); }

	// Size of the array
	size_t size() const { return m_size; }
	size_t chunkCount() const { return m_chunkCount; }
	size_type capacity() const { return m_capacity; }

	// Access data
	T* data() { return reinterpret_cast<T*>(m_data); }
	const T* data() const { return reinterpret_cast<const T*>(m_data); }

	reference operator[](size_type i) { return reinterpret_cast<T*>(m_data)[i]; }
	const_reference operator[](size_type i) const { return reinterpret_cast<const T*>(m_data)[i]; }
	reference back() { return reinterpret_cast<T*>(m_data)[m_size-1]; }
	reference front() { return *reinterpret_cast<T*>(m_data) ;}
	const_reference back() const { return reinterpret_cast<const T*>(m_data)[m_size-1]; }
	const_reference front() const { return *reinterpret_cast<const T*>(m_data) ;}

	// Begin and end functions for iterator
	iterator begin() { return iterator(reinterpret_cast<T*>(m_data)); }
	iterator end() { return iterator(reinterpret_cast<T*>(m_data) + m_size); }
	const_iterator begin() const { return iterator(reinterpret_cast<T*>(m_data)); }
	const_iterator end() const { return iterator(reinterpret_cast<T*>(m_data) + m_size); }
	// Begin and end functions for reverse_iterator
	reverse_iterator rbegin() { return iterator(reinterpret_cast<T*>(m_data) + m_size); }
	reverse_iterator rend() { return iterator(reinterpret_cast<T*>(m_data)); }
	const_reverse_iterator rbegin() const { return iterator(reinterpret_cast<T*>(m_data) + m_size); }
	const_reverse_iterator rend() const { return iterator(reinterpret_cast<T*>(m_data)); }
	// Capacity
	bool empty() const { return m_size == 0; }
	// Let's start adding data
	void clear() { m_size = 0; }
	void  shrink_to_fit() {
		shrink(div_ceil(std::max(m_size,size_t(1)), ChunkSize));
	}
	template <typename... Args> reference emplace_back(Args&&... args) {
		if(m_size >= m_capacity) {
			grow(1);
		}
		return *new (reinterpret_cast<T*>(m_data) + m_size++) T(std::forward<Args>(args)...);
	}
	void push_back(const T& newElement) {
		return emplace_back(newElement);
	}
	void push_back(T&& newElement) {
		return emplace_back(std::move(newElement));
	}
	void pop_back() {
		m_size = std::max(m_size-1, size_t(0));
	}
	void resize(size_t newSize) {
		m_size = newSize;
		size_t idealSize = div_ceil(std::max(newSize,size_t(1)), ChunkSize);
		if(idealSize == m_chunkCount) return;
		else if(idealSize > m_chunkCount) grow(idealSize);
	}
};

template<class T, size_t ChunkSize = 256, class Alloc = std::allocator<std::array<T,ChunkSize>>>
requires Allocator<Alloc, std::array<T,ChunkSize>> class SharedChunkyArray {
public:
	typedef std::array<T,ChunkSize> Chunk;
	typedef T value_type;
	typedef Alloc allocator_type;
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
	typedef std::allocator_traits<Alloc> AllocTraits;
private:
	struct Container {
		Alloc alloc;
		Chunk* m_data;
		size_t m_chunkCount;
		size_t m_size;
		size_t m_capacity;
		std::atomic<size_t> m_refCount;
		Container(size_t size, bool countAsInitialized = false) :
			m_chunkCount(size),
			m_capacity(size * ChunkSize),
			m_size(countAsInitialized ? (size * ChunkSize) : 0),
			m_refCount(1) {
			m_data = alloc.allocate(m_chunkCount);
		}
		~Container() {
			for (size_t i = 0; i < m_size; ++i) {
				AllocTraits::destroy(alloc, &m_data[i]); // Call the destructor
			}
			alloc.deallocate(m_data, m_chunkCount);
		}
		void grow(size_t newChunks) {
			Chunk* newChunkStore = alloc.allocate(m_chunkCount + newChunks);
			std::memcpy(newChunkStore,m_data,m_chunkCount * sizeof(Chunk));
			alloc.deallocate(m_data, m_chunkCount);
			m_data = newChunkStore;
			m_chunkCount += newChunks;
			m_capacity = m_chunkCount * ChunkSize;
		}
		void shrink(size_t newChunkCount) {
			Chunk* newChunkStore = alloc.allocate(newChunkCount);
			std::memcpy(newChunkStore,m_data,std::min(m_chunkCount,newChunkCount) * sizeof(Chunk));
			alloc.deallocate(m_data, m_chunkCount);
			m_data = newChunkStore;
			m_chunkCount = newChunkCount;
			m_capacity = m_chunkCount * ChunkSize;
		}
		void  shrink_to_fit() {
			shrink(div_ceil(std::max(m_size,size_t(1)), ChunkSize));
		}
		template <typename... Args> reference emplace_back(Args&&... args) {
			if(m_size >= m_capacity) {
				grow(1);
			}
			return *new (reinterpret_cast<T*>(m_data) + m_size++) T(std::forward<Args>(args)...);
		}
		void push_back(const T& newElement) {
			return emplace_back(newElement);
		}
		void push_back(T&& newElement) {
			return emplace_back(std::move(newElement));
		}
		void pop_back() {
			m_size = std::max(m_size-1, size_t(0));
		}
		void resize(size_t newSize) {
			m_size = newSize;
			size_t idealSize = div_ceil(std::max(newSize,size_t(1)), ChunkSize);
			if(idealSize == m_chunkCount) return;
			else if(idealSize > m_chunkCount) grow(idealSize);
		}
	};
	Container* m_container;
public:
	// Constructor
	SharedChunkyArray(size_t size, bool countAsInitialized = false) {
		m_container = new Container(size,countAsInitialized);
	}
	// Destructor
	~SharedChunkyArray() {
		if(m_container) {
			--m_container->m_refCount;
			if(m_container->m_refCount <= 0) {
				delete m_container;
			}
		}
	}
	// Move constructor
	SharedChunkyArray(SharedChunkyArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
	}
	SharedChunkyArray& operator=(SharedChunkyArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
		return *this;
	}
	SharedChunkyArray(const SharedChunkyArray& cpy) {
		m_container = cpy.m_container;
		++m_container->m_refCount;
	}
	SharedChunkyArray& operator=(const SharedChunkyArray& cpy)  {
		m_container = cpy.m_container;
		++m_container->m_refCount;
		return *this;
	}

	operator span() { return span(reinterpret_cast<T*>(m_container->m_data) , m_container->m_size); }
	operator const_span() const { return const_span(reinterpret_cast<const T*>(m_container->m_data), m_container->m_size); }

	// Size of the array
	size_t size() const { return m_container->m_size; }
	size_t chunkCount() const { return m_container->m_chunkCount; }
	size_type capacity() const { return m_container->m_capacity; }

	// Access data
	T* data() { return reinterpret_cast<T*>(m_container->m_data); }
	const T* data() const { return reinterpret_cast<const T*>(m_container->m_data); }

	reference operator[](size_type i) { return reinterpret_cast<T*>(m_container->m_data)[i]; }
	const_reference operator[](size_type i) const { return reinterpret_cast<const T*>(m_container->m_data)[i]; }
	reference back() { return reinterpret_cast<T*>(m_container->m_data)[m_container->m_size-1]; }
	reference front() { return *reinterpret_cast<T*>(m_container->m_data) ;}
	const_reference back() const { return reinterpret_cast<const T*>(m_container->m_data)[m_container->m_size-1]; }
	const_reference front() const { return *reinterpret_cast<const T*>(m_container->m_data) ;}

	// Begin and end functions for iterator
	iterator begin() { return iterator(reinterpret_cast<T*>(m_container->m_data)); }
	iterator end() { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }
	const_iterator begin() const { return iterator(reinterpret_cast<T*>(m_container->m_data)); }
	const_iterator end() const { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }
	// Begin and end functions for reverse_iterator
	reverse_iterator rbegin() { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }
	reverse_iterator rend() { return iterator(reinterpret_cast<T*>(m_container->m_data)); }
	const_reverse_iterator rbegin() const { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }
	const_reverse_iterator rend() const { return iterator(reinterpret_cast<T*>(m_container->m_data)); }
	// Capacity
	bool empty() const { return m_container->m_size == 0; }
	// Let's start adding data
	void clear() { m_container->m_size = 0; }
	void  shrink_to_fit() {
		m_container->shrink_to_fit();
	}
	template <typename... Args> reference emplace_back(Args&&... args) {
		return m_container->emplace_back(std::forward(args)...);
	}
	void push_back(const T& newElement) {
		m_container->emplace_back(newElement);
	}
	void push_back(T&& newElement) {
		m_container->emplace_back(std::move(newElement));
	}
	void pop_back() {
		m_container->pop_back();
	}
	void resize(size_t newSize) {
		m_container->resize(newSize);
	}
};

}
}

#endif // ELVCHUNKYARRAY_HPP
