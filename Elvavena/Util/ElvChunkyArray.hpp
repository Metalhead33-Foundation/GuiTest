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
//! A variable-size array that allocates data in chunks.
/*!
	\tparam T The type of data the array is filled with.
	\tparam ChunkSize The size of chunks to allocate the data in.
	\tparam Alloc Allocator to use for allocating memory for the data.
*/
template<class T, size_t ChunkSize = 256, class Alloc = std::allocator<std::array<T,ChunkSize>>>
requires Allocator<Alloc, std::array<T,ChunkSize>> class UniqueChunkyArray {
public:
	//! A chunk type representing an array of T with the specified ChunkSize.
	typedef std::array<T,ChunkSize> Chunk;
	//! The value type stored in the array.
	typedef T value_type;
	//! The allocator type used for memory allocation.
	typedef Alloc allocator_type;
	//! The type used for sizes.
	typedef size_t size_type;
	//! The type used for differences between pointers.
	typedef ptrdiff_t difference_type;
	//! A reference to a value type.
	typedef T& reference;
	//! A const reference to a value type.
	typedef const T& const_reference;
	//! A pointer to a value type.
	typedef T* pointer;
	//! A const pointer to a value type.
	typedef const T* const_pointer;
	//! A span representing a view of the data.
	typedef std::span<T> span;
	//! A const span representing a view of the data.
	typedef std::span<const T> const_span;
	//! An iterator for non-const elements.
	typedef continuous_iterator<T> iterator;
	//! An iterator for const elements.
	typedef continuous_iterator<const T> const_iterator;
	//! A reverse iterator for non-const elements.
	typedef std::reverse_iterator<iterator> reverse_iterator;
	//! A reverse iterator for const elements.
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	//! Traits for allocator operations.
	typedef std::allocator_traits<Alloc> AllocTraits;
private:
	Alloc alloc;		  //!< The allocator used for memory management.
	Chunk* m_data;	   //!< Pointer to the allocated chunks of data.
	size_t m_chunkCount; //!< The current number of chunks allocated.
	size_t m_size;	   //!< The number of elements currently stored in the array.
	size_t m_capacity;   //!< The total capacity of the array in terms of elements.

	//! Grow the array to accommodate additional chunks.
	/*!
		\param newChunks The number of new chunks to allocate.
	*/
	void grow(size_t newChunks) {
		Chunk* newChunkStore = Alloc().allocate(m_chunkCount + newChunks);
		std::memcpy(newChunkStore,m_data,m_chunkCount * sizeof(Chunk));
		alloc.deallocate(m_data, m_chunkCount);
		m_data = newChunkStore;
		m_chunkCount += newChunks;
		m_capacity = m_chunkCount * ChunkSize;
	}

	//! Shrink the array to a new number of chunks.
	/*!
		\param newChunkCount The new number of chunks to keep.
	*/
	void shrink(size_t newChunkCount) {
		Chunk* newChunkStore = alloc.allocate(newChunkCount);
		std::memcpy(newChunkStore,m_data,std::min(m_chunkCount,newChunkCount) * sizeof(Chunk));
		Alloc().deallocate(m_data, m_chunkCount);
		m_data = newChunkStore;
		m_chunkCount = newChunkCount;
		m_capacity = m_chunkCount * ChunkSize;
	}

	//! Copy constructor (deleted)
	UniqueChunkyArray(const UniqueChunkyArray& cpy) = delete;

	//! Copy assignment operator (deleted)
	UniqueChunkyArray& operator=(const UniqueChunkyArray& cpy) = delete;

public:
	//! Constructor
	/*!
		\param size Initial size of the array (number of chunks).
		\param countAsInitialized If true, initializes size with allocated elements.
	*/
	template <typename... Args> UniqueChunkyArray(size_t size, bool countAsInitialized = false, Args&&... args) :
		m_chunkCount(size),
		m_capacity(size * ChunkSize),
		m_size(countAsInitialized ? (size * ChunkSize) : 0),
		alloc(std::forward(args)...)
	{
		m_data = alloc.allocate(size);
	}

	//! Destructor
	/*!
		Cleans up the allocated resources.
	*/
	~UniqueChunkyArray() {
		if(m_data) {
			for (size_t i = 0; i < m_size; ++i) {
				AllocTraits::destroy(alloc, &m_data[i]); // Call the destructor
			}
			alloc.deallocate(m_data, m_chunkCount);
		}
	}

	//! Move constructor
	/*!
		\param mov The object to move from.
	*/
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

	//! Move assignment operator
	/*!
		\param mov The object to move from.
		\return A reference to this object.
	*/
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

	//! Converts the array to a span for easy access.
	operator span() { return span(reinterpret_cast<T*>(m_data) , m_size); }

	//! Converts the array to a const span for easy access.
	operator const_span() const { return const_span(reinterpret_cast<const T*>(m_data), m_size); }

	// Size of the array
	/*!
		\return The number of elements currently stored in the array.
	*/
	size_t size() const { return m_size; }

	/*!
		\return The current number of chunks allocated.
	*/
	size_t chunkCount() const { return m_chunkCount; }

	/*!
		\return The total capacity of the array in terms of elements.
	*/
	size_type capacity() const { return m_capacity; }

	// Access data
	/*!
		\return Pointer to the underlying data.
	*/
	T* data() { return reinterpret_cast<T*>(m_data); }

	/*!
		\return Const pointer to the underlying data.
	*/
	const T* data() const { return reinterpret_cast<const T*>(m_data); }

	/*!
		\param i Index of the element to access.
		\return Reference to the element at index i.
	*/
	reference operator[](size_type i) { return reinterpret_cast<T*>(m_data)[i]; }

	/*!
		\param i Index of the element to access.
		\return Const reference to the element at index i.
	*/
	const_reference operator[](size_type i) const { return reinterpret_cast<const T*>(m_data)[i]; }

	/*!
		\return Reference to the last element.
	*/
	reference back() { return reinterpret_cast<T*>(m_data)[m_size-1]; }

	/*!
		\return Reference to the first element.
	*/
	reference front() { return *reinterpret_cast<T*>(m_data); }

	/*!
		\return Const reference to the last element.
	*/
	const_reference back() const { return reinterpret_cast<const T*>(m_data)[m_size-1]; }

	/*!
		\return Const reference to the first element.
	*/
	const_reference front() const { return *reinterpret_cast<const T*>(m_data); }

	// Begin and end functions for iterator
	/*!
		\return An iterator to the beginning of the array.
	*/
	iterator begin() { return iterator(reinterpret_cast<T*>(m_data)); }

	/*!
		\return An iterator to the end of the array.
	*/
	iterator end() { return iterator(reinterpret_cast<T*>(m_data) + m_size); }

	/*!
		\return A const iterator to the beginning of the array.
	*/
	const_iterator begin() const { return iterator(reinterpret_cast<T*>(m_data)); }

	/*!
		\return A const iterator to the end of the array.
	*/
	const_iterator end() const { return iterator(reinterpret_cast<T*>(m_data) + m_size); }

	// Begin and end functions for reverse_iterator
	/*!
		\return A reverse iterator to the beginning of the array.
	*/
	reverse_iterator rbegin() { return iterator(reinterpret_cast<T*>(m_data) + m_size); }

	/*!
		\return A reverse iterator to the end of the array.
	*/
	reverse_iterator rend() { return iterator(reinterpret_cast<T*>(m_data)); }

	/*!
		\return A const reverse iterator to the beginning of the array.
	*/
	const_reverse_iterator rbegin() const { return iterator(reinterpret_cast<T*>(m_data) + m_size); }

	/*!
		\return A const reverse iterator to the end of the array.
	*/
	const_reverse_iterator rend() const { return iterator(reinterpret_cast<T*>(m_data)); }

	// Capacity
	/*!
		\return True if the array is empty; otherwise, false.
	*/
	bool empty() const { return m_size == 0; }

	//! Clears the array, setting size to 0.
	void clear() { m_size = 0; }

	//! Shrinks the array to fit its size.
	void shrink_to_fit() {
		shrink(div_ceil(std::max(m_size,size_t(1)), ChunkSize));
	}

	/*!
		Adds a new element to the end of the array, constructing it in place.
		\param args Arguments to forward to the constructor of T.
		\return A reference to the newly added element.
	*/
	template <typename... Args> reference emplace_back(Args&&... args) {
		if(m_size >= m_capacity) {
			grow(1);
		}
		return *new (reinterpret_cast<T*>(m_data) + m_size++) T(std::forward<Args>(args)...);
	}

	//! Adds a copy of a new element to the end of the array.
	/*!
		\param newElement The element to add.
	*/
	void push_back(const T& newElement) {
		return emplace_back(newElement);
	}

	//! Adds a moved element to the end of the array.
	/*!
		\param newElement The element to add.
	*/
	void push_back(T&& newElement) {
		return emplace_back(std::move(newElement));
	}

	//! Removes the last element from the array.
	void pop_back() {
		m_size = std::max(m_size-1, size_t(0));
	}

	/*!
		Resizes the array to a new size.
		\param newSize The new size for the array.
	*/
	void resize(size_t newSize) {
		m_size = newSize;
		size_t idealSize = div_ceil(std::max(newSize,size_t(1)), ChunkSize);
		if(idealSize == m_chunkCount) return;
		else if(idealSize > m_chunkCount) grow(idealSize);
	}
};


//! A variable-size array that allocates data in chunks. This variant is shared, with reference-counting.
/*!
	\tparam T The type of data the array is filled with.
	\tparam ChunkSize The size of chunks to allocate the data in.
	\tparam Alloc Allocator to use for allocating memory for the data.
*/
template<class T, size_t ChunkSize = 256, class Alloc = std::allocator<std::array<T,ChunkSize>>>
requires Allocator<Alloc, std::array<T,ChunkSize>> class SharedChunkyArray {
public:
	typedef std::array<T,ChunkSize> Chunk; //!< Type of chunk.
	typedef T value_type; //!< Type of value stored in the array.
	typedef Alloc allocator_type; //!< Type of allocator used for memory allocation.
	typedef size_t size_type; //!< Type for size representation.
	typedef ptrdiff_t difference_type; //!< Type for pointer differences.
	typedef T& reference; //!< Reference to a value in the array.
	typedef const T& const_reference; //!< Constant reference to a value in the array.
	typedef T* pointer; //!< Pointer to a value in the array.
	typedef const T* const_pointer; //!< Constant pointer to a value in the array.
	typedef std::span<T> span; //!< Span type for non-const access.
	typedef std::span<const T> const_span; //!< Span type for const access.

	typedef continuous_iterator<T> iterator; //!< Iterator type for non-const access.
	typedef continuous_iterator<const T> const_iterator; //!< Iterator type for const access.

	typedef std::reverse_iterator<iterator> reverse_iterator; //!< Reverse iterator type for non-const access.
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator; //!< Reverse iterator type for const access.
	typedef std::allocator_traits<Alloc> AllocTraits; //!< Traits for allocator.

private:
	/*!
		Structure that holds the container data and metadata.
	*/
	struct Container {
		Alloc alloc; //!< Allocator instance.
		Chunk* m_data; //!< Pointer to the data chunks.
		size_t m_chunkCount; //!< Number of allocated chunks.
		size_t m_size; //!< Current size of the array.
		size_t m_capacity; //!< Current capacity of the array.
		std::atomic<size_t> m_refCount; //!< Reference count for shared ownership.

		/*!
			Constructor for the container.
			\param size Initial number of chunks.
			\param countAsInitialized Flag indicating if the array is initialized.
		*/
		template <typename... Args> Container(size_t size, bool countAsInitialized = false, Args&&... args) :
			m_chunkCount(size),
			m_capacity(size * ChunkSize),
			m_size(countAsInitialized ? (size * ChunkSize) : 0),
			m_refCount(1),
			alloc(std::forward(args)...)
		{
			m_data = alloc.allocate(m_chunkCount);
		}
		/*!
			Constructor for the container.
			\param size Initial number of chunks.
			\param countAsInitialized Flag indicating if the array is initialized.
		*/
		Container(Alloc&& movAlloc, size_t size, bool countAsInitialized = false) :
			m_chunkCount(size),
			m_capacity(size * ChunkSize),
			m_size(countAsInitialized ? (size * ChunkSize) : 0),
			m_refCount(1),
			alloc(std::move(movAlloc))
		{
			m_data = alloc.allocate(m_chunkCount);
		}

		//! Destructor for the container.
		~Container() {
			for (size_t i = 0; i < m_size; ++i) {
				AllocTraits::destroy(alloc, &m_data[i]); // Call the destructor
			}
			alloc.deallocate(m_data, m_chunkCount);
		}

		/*!
			Grow the container by allocating new chunks.
			\param newChunks Number of new chunks to allocate.
		*/
		void grow(size_t newChunks) {
			Chunk* newChunkStore = alloc.allocate(m_chunkCount + newChunks);
			std::memcpy(newChunkStore,m_data,m_chunkCount * sizeof(Chunk));
			alloc.deallocate(m_data, m_chunkCount);
			m_data = newChunkStore;
			m_chunkCount += newChunks;
			m_capacity = m_chunkCount * ChunkSize;
		}

		/*!
			Shrink the container to the specified number of chunks.
			\param newChunkCount New number of chunks to keep.
		*/
		void shrink(size_t newChunkCount) {
			Chunk* newChunkStore = alloc.allocate(newChunkCount);
			std::memcpy(newChunkStore,m_data,std::min(m_chunkCount,newChunkCount) * sizeof(Chunk));
			alloc.deallocate(m_data, m_chunkCount);
			m_data = newChunkStore;
			m_chunkCount = newChunkCount;
			m_capacity = m_chunkCount * ChunkSize;
		}

		//! Shrink the container to fit the current size.
		void  shrink_to_fit() {
			shrink(div_ceil(std::max(m_size,size_t(1)), ChunkSize));
		}

		/*!
			Emplace a new element at the back of the array.
			\param args Arguments to forward to the constructor of T.
			\return Reference to the newly emplaced element.
		*/
		template <typename... Args> reference emplace_back(Args&&... args) {
			if(m_size >= m_capacity) {
				grow(1);
			}
			return *new (reinterpret_cast<T*>(m_data) + m_size++) T(std::forward<Args>(args)...);
		}

		/*!
			Push a new element to the back of the array.
			\param newElement Element to be added.
		*/
		void push_back(const T& newElement) {
			return emplace_back(newElement);
		}

		/*!
			Push a new element to the back of the array.
			\param newElement Element to be moved.
		*/
		void push_back(T&& newElement) {
			return emplace_back(std::move(newElement));
		}

		//! Remove the last element from the array.
		void pop_back() {
			m_size = std::max(m_size-1, size_t(0));
		}

		/*!
			Resize the array to a new size.
			\param newSize The new size of the array.
		*/
		void resize(size_t newSize) {
			m_size = newSize;
			size_t idealSize = div_ceil(std::max(newSize,size_t(1)), ChunkSize);
			if(idealSize == m_chunkCount) return;
			else if(idealSize > m_chunkCount) grow(idealSize);
		}
	};
	typedef AllocTraits::template rebind_alloc<Container> ContainerAllocator;
	typedef std::allocator_traits<ContainerAllocator> ContainerAllocatorTraits;

	Container* m_container; //!< Pointer to the container.

public:
	// Constructor
	/*!
		Constructor for SharedChunkyArray.
		\param size Initial number of chunks.
		\param countAsInitialized Flag indicating if the array is initialized.
	*/
	template<typename... Args> SharedChunkyArray(size_t size, bool countAsInitialized = false, Args&&... args) {
		Alloc tmpAlloc(std::forward(args)...);
		const Alloc& tmpAllocRef = tmpAlloc;
		ContainerAllocator cntr(tmpAllocRef);
		m_container = ContainerAllocatorTraits::allocate(cntr, 1);
		ContainerAllocatorTraits::construct(cntr,m_container, size, countAsInitialized, std::move(tmpAlloc));
		//m_container = new Container(size,countAsInitialized, std::move(tmpAlloc));
	}

	//! Destructor for SharedChunkyArray.
	~SharedChunkyArray() {
		if(m_container) {
			--m_container->m_refCount;
			if(m_container->m_refCount <= 0) {
				ContainerAllocator cntr(std::move(m_container->alloc));
				/// Delete the container if no longer shared.
				ContainerAllocatorTraits::destroy(cntr,m_container);
				ContainerAllocatorTraits::deallocate(cntr,m_container);
			}
		}
	}

	//! Move constructor for SharedChunkyArray.
	SharedChunkyArray(SharedChunkyArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
	}

	/*!
		Move assignment operator for SharedChunkyArray.
		\param mov The SharedChunkyArray to move from.
		\return Reference to the current instance.
	*/
	SharedChunkyArray& operator=(SharedChunkyArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
		return *this;
	}

	//! Copy constructor for SharedChunkyArray.
	SharedChunkyArray(const SharedChunkyArray& cpy) {
		m_container = cpy.m_container;
		++m_container->m_refCount;
	}

	/*!
		Copy assignment operator for SharedChunkyArray.
		\param cpy The SharedChunkyArray to copy from.
		\return Reference to the current instance.
	*/
	SharedChunkyArray& operator=(const SharedChunkyArray& cpy)  {
		m_container = cpy.m_container;
		++m_container->m_refCount;
		return *this;
	}

	//! Convert to non-const span.
	operator span() { return span(reinterpret_cast<T*>(m_container->m_data) , m_container->m_size); }

	//! Convert to const span.
	operator const_span() const { return const_span(reinterpret_cast<const T*>(m_container->m_data), m_container->m_size); }

	//! Get the size of the array.
	//! \return Size of the array.
	size_t size() const { return m_container->m_size; }

	//! Get the number of chunks allocated.
	//! \return Number of chunks allocated.
	size_t chunkCount() const { return m_container->m_chunkCount; }

	//! Get the capacity of the array.
	//! \return Current capacity of the array.
	size_type capacity() const { return m_container->m_capacity; }

	//! Access data.
	//! \return Pointer to the data.
	T* data() { return reinterpret_cast<T*>(m_container->m_data); }

	//! Access data (const).
	//! \return Pointer to the data (const).
	const T* data() const { return reinterpret_cast<const T*>(m_container->m_data); }

	/*!
		Access element at specified index.
		\param i Index of the element.
		\return Reference to the element at the specified index.
	*/
	reference operator[](size_type i) { return reinterpret_cast<T*>(m_container->m_data)[i]; }

	/*!
		Access element at specified index (const).
		\param i Index of the element.
		\return Constant reference to the element at the specified index.
	*/
	const_reference operator[](size_type i) const { return reinterpret_cast<const T*>(m_container->m_data)[i]; }

	//! Access the last element.
	//! \return Reference to the last element.
	reference back() { return reinterpret_cast<T*>(m_container->m_data)[m_container->m_size-1]; }

	//! Access the first element.
	//! \return Reference to the first element.
	reference front() { return *reinterpret_cast<T*>(m_container->m_data); }

	//! Access the last element (const).
	//! \return Constant reference to the last element.
	const_reference back() const { return reinterpret_cast<const T*>(m_container->m_data)[m_container->m_size-1]; }

	//! Access the first element (const).
	//! \return Constant reference to the first element.
	const_reference front() const { return *reinterpret_cast<const T*>(m_container->m_data); }

	//! Begin and end functions for iterator
	/*!
		Returns an iterator to the beginning of the array.
		\return Iterator to the beginning.
	*/
	iterator begin() { return iterator(reinterpret_cast<T*>(m_container->m_data)); }

	/*!
		Returns an iterator to the end of the array.
		\return Iterator to the end.
	*/
	iterator end() { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }

	/*!
		Returns a constant iterator to the beginning of the array.
		\return Constant iterator to the beginning.
	*/
	const_iterator begin() const { return iterator(reinterpret_cast<T*>(m_container->m_data)); }

	/*!
		Returns a constant iterator to the end of the array.
		\return Constant iterator to the end.
	*/
	const_iterator end() const { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }

	// Begin and end functions for reverse_iterator
	/*!
		Returns a reverse iterator to the end of the array.
		\return Reverse iterator to the end.
	*/
	reverse_iterator rbegin() { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }

	/*!
		Returns a reverse iterator to the beginning of the array.
		\return Reverse iterator to the beginning.
	*/
	reverse_iterator rend() { return iterator(reinterpret_cast<T*>(m_container->m_data)); }

	/*!
		Returns a constant reverse iterator to the end of the array.
		\return Constant reverse iterator to the end.
	*/
	const_reverse_iterator rbegin() const { return iterator(reinterpret_cast<T*>(m_container->m_data) + m_container->m_size); }

	/*!
		Returns a constant reverse iterator to the beginning of the array.
		\return Constant reverse iterator to the beginning.
	*/
	const_reverse_iterator rend() const { return iterator(reinterpret_cast<T*>(m_container->m_data)); }

	//! Check if the array is empty.
	//! \return True if empty, false otherwise.
	bool empty() const { return m_container->m_size == 0; }

	//! Clear the array.
	void clear() { m_container->m_size = 0; }

	//! Shrink the container to fit the current size.
	void  shrink_to_fit() {
		m_container->shrink_to_fit();
	}

	/*!
		Emplace a new element at the back of the array.
		\param args Arguments to forward to the constructor of T.
		\return Reference to the newly emplaced element.
	*/
	template <typename... Args> reference emplace_back(Args&&... args) {
		return m_container->emplace_back(std::forward<Args>(args)...);
	}

	/*!
		Push a new element to the back of the array.
		\param newElement Element to be added.
	*/
	void push_back(const T& newElement) {
		m_container->emplace_back(newElement);
	}

	/*!
		Push a new element to the back of the array.
		\param newElement Element to be moved.
	*/
	void push_back(T&& newElement) {
		m_container->emplace_back(std::move(newElement));
	}

	//! Remove the last element from the array.
	void pop_back() {
		m_container->pop_back();
	}

	/*!
		Resize the array to a new size.
		\param newSize The new size of the array.
	*/
	void resize(size_t newSize) {
		m_container->resize(newSize);
	}
};


}
}

#endif // ELVCHUNKYARRAY_HPP
