#ifndef ELVHEAPARRAY_HPP
#define ELVHEAPARRAY_HPP
#include <memory>
#include <cstddef>
#include <iterator>
#include <span>
#include <atomic>
#include <Elvavena/Util/ElvContinuousIterator.hpp>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
namespace Elv {
namespace Util {

/**
 * @class UniqueHeapArray
 * @brief A dynamically allocated array class with unique ownership, providing a
 *		std::span-like interface and supporting iteration.
 *
 * @tparam T The type of elements stored in the array.
 * @tparam Alloc The allocator type (default: std::allocator<T>).
 * @requires Allocator<Alloc, T> The allocator must satisfy the Allocator concept.
 */
template<class T, class Alloc = std::allocator<T>> requires Allocator<Alloc, T>
class UniqueHeapArray {
public:
	/**
	 * @name Type Aliases
	 * @{
	 */
	/// The type of elements stored in the array.
	typedef T value_type;
	/// The allocator type used for memory management.
	typedef Alloc allocator_type;
	/// The type used to represent the size of the array.
	typedef size_t size_type;
	/// The type used to represent the difference between two iterators.
	typedef ptrdiff_t difference_type;
	/// Reference to an element in the array.
	typedef T& reference;
	/// Constant reference to an element in the array.
	typedef const T& const_reference;
	/// Pointer to an element in the array.
	typedef T* pointer;
	/// Constant pointer to an element in the array.
	typedef const T* const_pointer;
	/// A view of the array's elements (non-owning).
	typedef std::span<T> span;
	/// A constant view of the array's elements (non-owning).
	typedef std::span<const T> const_span;
	/// Iterator type for iterating over the array's elements.
	typedef continuous_iterator<T> iterator;
	/// Constant iterator type for iterating over the array's elements.
	typedef continuous_iterator<const T> const_iterator;
	/// Reverse iterator type for iterating over the array's elements in reverse.
	typedef std::reverse_iterator<iterator> reverse_iterator;
	/// Constant reverse iterator type for iterating over the array's elements in reverse.
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	/// Traits type for the allocator.
	typedef std::allocator_traits<Alloc> AllocTraits;
	/// @} (end of Type Aliases)

private:
	/// The dynamically allocated array of elements.
	T* m_data;
	/// The number of elements in the array.
	size_t m_size;
	/// The allocator instance used for memory management.
	Alloc alloc;

	/**
	 * @brief Copy constructor (deleted).
	 *
	 * UniqueHeapArray is a non-copyable class to ensure unique ownership.
	 */
	UniqueHeapArray(const UniqueHeapArray& cpy) = delete;
	/**
	 * @brief Copy assignment operator (deleted).
	 *
	 * UniqueHeapArray is a non-copyable class to ensure unique ownership.
	 */
	UniqueHeapArray& operator=(const UniqueHeapArray& cpy) = delete;

public:
	/**
	 * @brief Constructor.
	 *
	 * Allocates memory for the specified number of elements and default-constructs each element.
	 *
	 * @param size The number of elements to allocate.
	 */
	template <class... Args> UniqueHeapArray(size_t size, Args&&... args) : m_size(size), alloc(std::forward(args)...) {
		m_data = alloc.allocate(size);
		for (size_t i = 0; i < m_size; ++i) {
			AllocTraits::construct(alloc, &m_data[i]); // Construct each element
		}
	}

	/**
	 * @brief Destructor.
	 *
	 * Destroys each element and deallocates the memory.
	 */
	~UniqueHeapArray() {
		if(m_data) {
			for (size_t i = 0; i < m_size; ++i) {
				AllocTraits::destroy(alloc, &m_data[i]); // Call the destructor
			}
			alloc.deallocate(m_data, m_size);
		}
	}

	/**
	 * @brief Move constructor.
	 *
	 * Transfers ownership of the array from the moved instance.
	 *
	 * @param mov The instance to move from.
	 */
	UniqueHeapArray(UniqueHeapArray&& mov) {
		m_data = mov.m_data;
		mov.m_data = nullptr;
		m_size = mov.m_size;
		mov.m_size = 0;
	}

	/**
	 * @brief Move assignment operator.
	 *
	 * Transfers ownership of the array from the moved instance.
	 *
	 * @param mov The instance to move from.
	 * @return A reference to the assigned instance.
	 */
	UniqueHeapArray& operator=(UniqueHeapArray&& mov) {
		m_data = mov.m_data;
		mov.m_data = nullptr;
		m_size = mov.m_size;
		mov.m_size = 0;
		return *this;
	}

	/**
	 * @brief Implicit conversion to a non-owning view of the array.
	 *
	 * @return A std::span<T> viewing the array's elements.
	 */
	operator span() { return span(m_data, m_size); }

	/**
	 * @brief Implicit conversion to a non-owning constant view of the array.
	 *
	 * @return A std::span<const T> viewing the array's elements.
	 */
	operator const_span() const { return const_span(m_data, m_size); }

	/**
	 * @brief Returns the number of elements in the array.
	 *
	 * @return The size of the array.
	 */
	size_t size() const { return m_size; }

	/**
	 * @brief Returns a pointer to the first element in the array.
	 *
	 * @return A pointer to the first element.
	 */
	T* data() { return m_data; }

	/**
	 * @brief Returns a constant pointer to the first element in the array.
	 *
	 * @return A constant pointer to the first element.
	 */
	const T* data() const { return m_data; }

	/**
	 * @brief Subscript operator for accessing elements.
	 *
	 * @param i The index of the element to access.
	 * @return A reference to the element at the specified index.
	 */
	reference operator[](size_type i) { return m_data[i]; }

	/**
	 * @brief Subscript operator for accessing constant elements.
	 *
	 * @param i The index of the element to access.
	 * @return A constant reference to the element at the specified index.
	 */
	const_reference operator[](size_type i) const { return m_data[i]; }

	/**
	 * @brief Returns a reference to the last element in the array.
	 *
	 * @return A reference to the last element.
	 */
	reference back() { return m_data[m_size-1]; }

	/**
	 * @brief Returns a constant reference to the last element in the array.
	 *
	 * @return A constant reference to the last element.
	 */
	const_reference back() const { return m_data[m_size-1]; }

	/**
	 * @brief Returns a reference to the first element in the array.
	 *
	 * @return A reference to the first element.
	 */
	reference front() { return *m_data ;}

	/**
	 * @brief Returns a constant reference to the first element in the array.
	 *
	 * @return A constant reference to the first element.
	 */
	const_reference front() const { return *m_data ;}

	/**
	 * @name Iterators
	 * @{
	 */
	/**
	 * @brief Returns an iterator pointing to the first element in the array.
	 *
	 * @return An iterator pointing to the first element.
	 */
	iterator begin() { return iterator(m_data); }

	/**
	 * @brief Returns an iterator pointing to the end of the array.
	 *
	 * @return An iterator pointing to the end.
	 */
	iterator end() { return iterator(m_data + m_size); }

	/**
	 * @brief Returns a constant iterator pointing to the first element in the array.
	 *
	 * @return A constant iterator pointing to the first element.
	 */
	const_iterator begin() const { return iterator(m_data); }

	/**
	 * @brief Returns a constant iterator pointing to the end of the array.
	 *
	 * @return A constant iterator pointing to the end.
	 */
	const_iterator end() const { return iterator(m_data + m_size); }

	/**
	 * @brief Returns a reverse iterator pointing to the last element in the array.
	 *
	 * @return A reverse iterator pointing to the last element.
	 */
	reverse_iterator rbegin() { return iterator(m_data + m_size); }

	/**
	 * @brief Returns a reverse iterator pointing to the beginning of the array.
	 *
	 * @return A reverse iterator pointing to the beginning.
	 */
	reverse_iterator rend() { return iterator(m_data); }

	/**
	 * @brief Returns a constant reverse iterator pointing to the last element in the array.
	 *
	 * @return A constant reverse iterator pointing to the last element.
	 */
	const_reverse_iterator rbegin() const { return iterator(m_data + m_size); }

	/**
	 * @brief Returns a constant reverse iterator pointing to the beginning of the array.
	 *
	 * @return A constant reverse iterator pointing to the beginning.
	 */
	const_reverse_iterator rend() const { return iterator(m_data); }
	/// @} (end of Iterators)

	/**
	 * @brief Checks whether the array is empty.
	 *
	 * @return True if the array is empty, false otherwise.
	 */
	bool empty() const { return m_size == 0; }
};


/**
 * @class SharedHeapArray
 * @brief A shared array class that manages a dynamically allocated array on the heap,
 *		providing shared ownership and random access to its elements.
 *
 * @tparam T The type of elements stored in the array.
 * @tparam Alloc The allocator type used for memory management (default: std::allocator<T>).
 * @requires Allocator<Alloc, T> The allocator must meet the Allocator requirements for T.
 */
template<class T, class Alloc = std::allocator<T>> requires Allocator<Alloc, T>
class SharedHeapArray {
public:
	/**
	 * @name Type Aliases
	 * @{
	 */
	/// The type of elements stored in the array.
	typedef T value_type;
	/// The allocator type used for memory management.
	typedef Alloc allocator_type;
	/// The unsigned integer type for sizes and indices.
	typedef size_t size_type;
	/// The signed integer type for differences between indices.
	typedef ptrdiff_t difference_type;
	/// Reference to an element.
	typedef T& reference;
	/// Constant reference to an element.
	typedef const T& const_reference;
	/// Pointer to an element.
	typedef T* pointer;
	/// Constant pointer to an element.
	typedef const T* const_pointer;
	/// A non-owning view of a contiguous sequence of elements.
	typedef std::span<T> span;
	/// A non-owning view of a contiguous sequence of constant elements.
	typedef std::span<const T> const_span;
	/// Iterator for accessing elements in the array.
	typedef continuous_iterator<T> iterator;
	/// Constant iterator for accessing elements in the array.
	typedef continuous_iterator<const T> const_iterator;
	/// Reverse iterator for accessing elements in the array in reverse order.
	typedef std::reverse_iterator<iterator> reverse_iterator;
	/// Constant reverse iterator for accessing elements in the array in reverse order.
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	/// Traits for the allocator type.
	typedef std::allocator_traits<Alloc> AllocTraits;
	/// @} // end of Type Aliases

private:
	/**
	 * @struct Container
	 * @brief Internal container struct managing the dynamically allocated array.
	 */
	struct Container {
		/// Pointer to the dynamically allocated array.
		T* m_data;
		/// The size of the allocated array.
		size_t m_size;
		/// The allocator instance used for memory management.
		Alloc alloc;
		/// Atomic reference count for shared ownership.
		std::atomic<size_t> m_refCount;

		/**
		 * @brief Constructor for the internal container.
		 * @param size The size of the array to allocate.
		 */
		template <class... Args> Container(size_t size, Args&&... args) : m_size(size), m_refCount(1), alloc(std::forward(args)...) {
			m_data = alloc.allocate(size);
			for (size_t i = 0; i < m_size; ++i) {
				/// Construct each element in the allocated array.
				AllocTraits::construct(alloc, &m_data[i]);
			}
		}
		/**
		 * @brief Constructor for the internal container.
		 * @param size The size of the array to allocate.
		 */
		Container(size_t size, Alloc&& allocMov) : m_size(size), m_refCount(1), alloc(std::move(allocMov)) {
			m_data = alloc.allocate(size);
			for (size_t i = 0; i < m_size; ++i) {
				/// Construct each element in the allocated array.
				AllocTraits::construct(alloc, &m_data[i]);
			}
		}

		/**
		 * @brief Destructor for the internal container.
		 */
		~Container() {
			for (size_t i = 0; i < m_size; ++i) {
				/// Call the destructor for each element in the array.
				AllocTraits::destroy(alloc, &m_data[i]);
			}
			/// Deallocate the array.
			alloc.deallocate(m_data, m_size);
		}
	};
	typedef AllocTraits::template rebind_alloc<Container> ContainerAllocator;
	typedef std::allocator_traits<ContainerAllocator> ContainerAllocatorTraits;

	/// Pointer to the internal container instance.
	Container* m_container;

public:
	/**
	 * @brief Constructor for SharedHeapArray.
	 * @param size The size of the array to allocate.
	 */
	template<typename... Args> SharedHeapArray(size_t size, Args&&... args) {
		Alloc tmpAlloc(std::forward(args)...);
		const Alloc& tmpAllocRef = tmpAlloc;
		ContainerAllocator cntr(tmpAllocRef);
		m_container = ContainerAllocatorTraits::allocate(cntr, 1);
		ContainerAllocatorTraits::construct(cntr,m_container, size, std::move(tmpAlloc));
		//m_container = cntr.allocate(size, std::move(tmpAlloc));
	}

	/**
	 * @brief Destructor for SharedHeapArray.
	 */
	~SharedHeapArray() {
		if (m_container) {
			/// Decrement the reference count.
			--m_container->m_refCount;
			if (m_container->m_refCount <= 0) {
				ContainerAllocator cntr(std::move(m_container->alloc));
				/// Delete the container if no longer shared.
				ContainerAllocatorTraits::destroy(cntr,m_container);
				ContainerAllocatorTraits::deallocate(cntr,m_container,1);
			}
		}
	}

	/**
	 * @brief Move constructor for SharedHeapArray.
	 * @param mov The SharedHeapArray instance to move from.
	 */
	SharedHeapArray(SharedHeapArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
	}

	/**
	 * @brief Move assignment operator for SharedHeapArray.
	 * @param mov The SharedHeapArray instance to move from.
	 * @return *this
	 */
	SharedHeapArray& operator=(SharedHeapArray&& mov) {
		m_container = mov.m_container;
		mov.m_container = nullptr;
		return *this;
	}

	/**
	 * @brief Copy constructor for SharedHeapArray.
	 * @param cpy The SharedHeapArray instance to copy from.
	 */
	SharedHeapArray(const SharedHeapArray& cpy) {
		m_container = cpy.m_container;
		/// Increment the reference count.
		++m_container->m_refCount;
	}

	/**
	 * @brief Copy assignment operator for SharedHeapArray.
	 * @param cpy The SharedHeapArray instance to copy from.
	 * @return *this
	 */
	SharedHeapArray& operator=(const SharedHeapArray& cpy) {
		m_container = cpy.m_container;
		/// Increment the reference count.
		++m_container->m_refCount;
		return *this;
	}

	/**
	 * @brief Implicit conversion to a non-owning view of the array.
	 * @return A span representing the entire array.
	 */
	operator span() { return span(m_container->m_data, m_container->m_size); }

	/**
	 * @brief Implicit conversion to a non-owning view of the constant array.
	 * @return A const_span representing the entire array.
	 */
	operator const_span() const { return const_span(m_container->m_data, m_container->m_size); }

	/**
	 * @brief Returns the size of the array.
	 * @return The number of elements in the array.
	 */
	size_t size() const { return m_container->m_size; }

	/**
	 * @brief Returns a pointer to the first element in the array.
	 * @return A pointer to the first element.
	 */
	T* data() { return m_container->m_data; }

	/**
	 * @brief Returns a constant pointer to the first element in the array.
	 * @return A constant pointer to the first element.
	 */
	const T* data() const { return m_container->m_data; }

	/**
	 * @brief Subscript operator for accessing elements.
	 * @param i The index of the element to access.
	 * @return A reference to the element at index i.
	 */
	reference operator[](size_type i) { return m_container->m_data[i]; }

	/**
	 * @brief Subscript operator for accessing constant elements.
	 * @param i The index of the element to access.
	 * @return A constant reference to the element at index i.
	 */
	const_reference operator[](size_type i) const { return m_container->m_data[i]; }

	/**
	 * @brief Returns a reference to the last element in the array.
	 * @return A reference to the last element.
	 */
	reference back() { return m_container->m_data[m_container->m_size - 1]; }

	/**
	 * @brief Returns a reference to the first element in the array.
	 * @return A reference to the first element.
	 */
	reference front() { return *(m_container->m_data); }

	/**
	 * @brief Returns a constant reference to the last element in the array.
	 * @return A constant reference to the last element.
	 */
	const_reference back() const { return m_container->m_data[m_container->m_size - 1]; }

	/**
	 * @brief Returns a constant reference to the first element in the array.
	 * @return A constant reference to the first element.
	 */
	const_reference front() const { return *(m_container->m_data); }

	/**
	 * @name Iterator Access
	 * @{
	 */
	/**
	 * @brief Returns an iterator pointing to the first element in the array.
	 * @return An iterator pointing to the first element.
	 */
	iterator begin() { return iterator(m_container->m_data); }

	/**
	 * @brief Returns an iterator pointing to the end of the array.
	 * @return An iterator pointing to the end.
	 */
	iterator end() { return iterator(m_container->m_data + m_container->m_size); }

	/**
	 * @brief Returns a constant iterator pointing to the first element in the array.
	 * @return A constant iterator pointing to the first element.
	 */
	const_iterator begin() const { return iterator(m_container->m_data); }

	/**
	 * @brief Returns a constant iterator pointing to the end of the array.
	 * @return A constant iterator pointing to the end.
	 */
	const_iterator end() const { return iterator(m_container->m_data + m_container->m_size); }

	/**
	 * @brief Returns a reverse iterator pointing to the last element in the array.
	 * @return A reverse iterator pointing to the last element.
	 */
	reverse_iterator rbegin() { return reverse_iterator(iterator(m_container->m_data + m_container->m_size)); }

	/**
	 * @brief Returns a reverse iterator pointing to the beginning of the array.
	 * @return A reverse iterator pointing to the beginning.
	 */
	reverse_iterator rend() { return reverse_iterator(iterator(m_container->m_data)); }

	/**
	 * @brief Returns a constant reverse iterator pointing to the last element in the array.
	 * @return A constant reverse iterator pointing to the last element.
	 */
	const_reverse_iterator rbegin() const { return reverse_iterator(const_iterator(m_container->m_data + m_container->m_size)); }

	/**
	 * @brief Returns a constant reverse iterator pointing to the beginning of the array.
	 * @return A constant reverse iterator pointing to the beginning.
	 */
	const_reverse_iterator rend() const { return reverse_iterator(const_iterator(m_container->m_data)); }
	/// @} // end of Iterator Access

	/**
	 * @brief Checks if the array is empty.
	 * @return True if the array is empty, false otherwise.
	 */
	bool empty() const { return m_container->m_size == 0; }
};

}
}
#endif // ELVHEAPARRAY_HPP
