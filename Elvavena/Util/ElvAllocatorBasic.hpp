#ifndef ELVALLOCATORBASIC_HPP
#define ELVALLOCATORBASIC_HPP
#include <cstddef>
#include <concepts>
#include <new>
#include <memory>
namespace Elv {
namespace Util {

//! Concept for a standard C++ allocator.
/*!
  \tparam Alloc The allocator.
  \tparam T The type that the allocator allocates.
*/
template <typename Alloc, typename T> concept Allocator = requires(Alloc alloc, T* ptr, std::size_t n, T t) {
	typename Alloc::value_type;               // Allocator must have a value_type
	requires std::same_as<typename Alloc::value_type, T>;
	//! Allocate function. Returns a pointer to the allocated data.
	/*!
	\param n The amount of objects to allocate memory for. Size is given in object-count, NOT bytes!
	\return A pointer to the memory allocated.
	*/
	{ alloc.allocate(n) } -> std::same_as<T*>;
	//! Deallocates memory.
	/*!
	\param ptr A pointer to the memory to deallocate.
	\param n The amount of objects to deallocate. Size is given in object-count, NOT bytes!
	*/
	alloc.deallocate(ptr, n);
};

//! A wrapper for smart pointers to use the given allocator.
/*!
	\tparam T The type that the allocator allocates.
	\tparam Alloc The allocator for allocating and deallocating memory for objects of type T
*/
template <typename T, typename Alloc = std::allocator<T>> requires Allocator<Alloc, T> struct SmartPointerWrappersForAlloc {
public:
	//! A wrapper for deleting the allocated memory.
	struct Deleter {
		//! Pointer to allocator, nullptr if default
		Alloc* allocator;
		//! Deallocates memory.
		/*!
		\param ptr A pointer to the memory to deallocate.
		*/
		void operator()(T* ptr) const {
			if (allocator) {
				std::destroy_at(ptr);
				allocator->deallocate(ptr, 1);
			} else {
				Alloc tempAlloc;
				std::destroy_at(ptr);
				tempAlloc.deallocate(ptr, 1);
			}
		}
	};
private:
	//! Allocates and constructs object of type T. Assumes a singleton allocator.
	/*!
		\tparam Args The various arguments the constructor of T takes in
		\param args The various arguments the constructor of T takes in
		\return A pointer to the newly allocated instance of T
	*/
	template <typename... Args> static T* create(Args&&... args) {
		Alloc allocator;
		T* toReturn = allocator.allocate(1);
		std::construct_at(toReturn, std::forward(args)...);
		return toReturn;
	}
	//! Allocates and constructs object of type T. Assumes a stateful, non-singleton allocator that will outlive the allocated object.
	/*!
		\tparam Args The various arguments the constructor of T takes in
		\param allocator The allocator for allocating and deallocating the memory.
		\param args The various arguments the constructor of T takes in
		\return A pointer to the newly allocated instance of T
	*/
	template <typename... Args> static T* createWithAllocator(Alloc& allocator, Args&&... args) {
		T* toReturn = allocator.allocate(1);
		std::construct_at(toReturn, std::forward(args)...);
		return toReturn;
	}
public:
	//! Unique pointer.
	typedef std::unique_ptr<T,Deleter> unique_ptr;
	//! Shared pointer.
	typedef std::shared_ptr<T> shared_ptr;
	//! Weak pointer.
	typedef std::weak_ptr<T> weak_ptr;
	//! Allocates and constructs object of type T. Assumes a stateful, non-singleton allocator that will outlive the allocated object.
	/*!
		\tparam Args The various arguments the constructor of T takes in
		\param allocator The allocator for allocating and deallocating the memory.
		\param args The various arguments the constructor of T takes in
		\return A unique pointer to the newly allocated instance of T
	*/
	template <typename... Args > static unique_ptr make_unique(Alloc& allocator, Args&&... args) {
		return unique_ptr(createWithAllocator(allocator,std::forward(args)...), Deleter{ &allocator } );
	}
	//! Allocates and constructs object of type T. Assumes a singleton allocator.
	/*!
		\tparam Args The various arguments the constructor of T takes in
		\param args The various arguments the constructor of T takes in
		\return A unique pointer to the newly allocated instance of T
	*/
	template <typename... Args > static unique_ptr make_unique(Args&&... args) {
		return unique_ptr(create(std::forward(args)...), Deleter {nullptr} );
	}
	//! Allocates and constructs object of type T. Assumes a stateful, non-singleton allocator that will outlive the allocated object.
	/*!
		\tparam Args The various arguments the constructor of T takes in
		\param allocator The allocator for allocating and deallocating the memory.
		\param args The various arguments the constructor of T takes in
		\return A shared pointer to the newly allocated instance of T
	*/
	template <typename... Args > static shared_ptr make_shared(Alloc& allocator, Args&&... args) {
		return shared_ptr(createWithAllocator(allocator,std::forward(args)...), Deleter{ &allocator });
	}
	//! Allocates and constructs object of type T. Assumes a singleton allocator.
	/*!
		\tparam Args The various arguments the constructor of T takes in
		\param args The various arguments the constructor of T takes in
		\return A shared pointer to the newly allocated instance of T
	*/
	template <typename... Args > static shared_ptr make_shared(Args&&... args) {
		return shared_ptr(create(std::forward(args)...),  Deleter {nullptr} );
	}
};
//! Allocates and constructs object of type T. Assumes a stateful, non-singleton allocator that will outlive the allocated object.
/*!
	\tparam T The type of object to allocate and construct
	\tparam Alloc The allocator for allocating and deallocating memory for objects of type T
	\tparam Args The various arguments the constructor of T takes in
	\param allocator The allocator for allocating and deallocating the memory.
	\param args The various arguments the constructor of T takes in
	\return A unique pointer to the newly allocated instance of T
*/
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::unique_ptr make_unique(Alloc & allocator, Args&&... args) {
	return SmartPointerWrappersForAlloc<T,Alloc>::make_unique(allocator,std::forward(args)...);
}
//! Allocates and constructs object of type T. Assumes a singleton allocator.
/*!
	\tparam T The type of object to allocate and construct
	\tparam Alloc The allocator for allocating and deallocating memory for objects of type T
	\tparam Args The various arguments the constructor of T takes in
	\param args The various arguments the constructor of T takes in
	\return A unique pointer to the newly allocated instance of T
*/
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::unique_ptr make_unique(Args&&... args) {
	return SmartPointerWrappersForAlloc<T,Alloc>::make_unique(std::forward(args)...);
}
//! Allocates and constructs object of type T. Assumes a stateful, non-singleton allocator that will outlive the allocated object.
/*!
	\tparam T The type of object to allocate and construct
	\tparam Alloc The allocator for allocating and deallocating memory for objects of type T
	\tparam Args The various arguments the constructor of T takes in
	\param allocator The allocator for allocating and deallocating the memory.
	\param args The various arguments the constructor of T takes in
	\return A shared pointer to the newly allocated instance of T
*/
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::shared_ptr make_shared(Alloc & allocator, Args&&... args) {
	return SmartPointerWrappersForAlloc<T,Alloc>::make_shared(allocator,std::forward(args)...);
}
//! Allocates and constructs object of type T. Assumes a singleton allocator.
/*!
	\tparam T The type of object to allocate and construct
	\tparam Alloc The allocator for allocating and deallocating memory for objects of type T
	\tparam Args The various arguments the constructor of T takes in
	\param args The various arguments the constructor of T takes in
	\return A shared pointer to the newly allocated instance of T
*/
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::shared_ptr make_shared(Args&&... args) {
	return SmartPointerWrappersForAlloc<T,Alloc>::make_shared(std::forward(args)...);
}

//! Basic unit of memory allocation.
struct Blk
{
	//! A pointer to the block of memory that has been allocated.
	void* ptr;
	//! The size of the memory block in bytes.
	size_t size;
};

//! Allocator concept based on Andrei Alexandrescu's 2015 presentation on allocators.
/*!
\tparam Alloc The allocator.
*/
template <typename Alloc> concept AlexandrescuAllocator = requires(Alloc alloc, std::size_t n, Blk blk, const Blk& blkcref) {
		//! Allocates memory.
		/*!
		\param n The amount of memory - in bytes - to allocate.
		\return The allocated memory block. Contains a null-pointer and zero size if the allocation was unsuccessful.
		*/
	{ alloc.allocateBlock(n) } -> std::same_as<Blk>;
		//! Deallocates memory.
		/*!
		\param blkcref A reference to the memory block being allocated. Ensure that the block is owned by the given allocator!
		*/
	{ alloc.deallocateBlock(blkcref) };
		//! Checks if the allocator owns the memory. Be careful, some allocators (e.g. the Mallocator) might consistently return true!
		/*!
		\param blkcref A reference to the memory block being checked.
		\return True if the allocator owns the block of memory, false otherwise. Be careful, some allocators (e.g. the Mallocator) might consistently return true!
		*/
	{ alloc.ownsBlock(blkcref) } -> std::same_as<bool>;
};
//! A wrapper that wraps Andrei Alexandrescu-style allocators to STL-style allocators.
/*!
  \tparam Alloc The allocator.
  \tparam T The type that the allocator allocates.
*/
template <typename Alloc, typename T> requires AlexandrescuAllocator<Alloc> struct AlexandrescuAllocatorAdapter {
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef void* void_pointer;
	typedef const void* const_void_pointer;
	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef AlexandrescuAllocatorAdapter allocator_type;
	typedef std::false_type propagate_on_container_copy_assignment;
	typedef std::false_type propagate_on_container_move_assignment;
	typedef std::false_type propagate_on_container_swap;
	typedef std::true_type is_always_equal;
	//! The underlying AlexandrescuAllocator
	Alloc alloc_;

	//! Constructor
	AlexandrescuAllocatorAdapter() = default;

	//! Allocate function. Returns a pointer to the allocated data.
	/*!
	\param n The amount of objects to allocate memory for. Size is given in object-count, NOT bytes!
	\return A pointer to the memory allocated.
	*/
	T* allocate(std::size_t n) {
		std::size_t total_size = n * sizeof(T);  // Calculate total size needed
		Blk blk = alloc_.allocateBlock(total_size);
		if (!blk.ptr) throw std::bad_alloc();
		return static_cast<T*>(blk.ptr);  // Return the pointer cast to T*
	}

	//! Deallocates memory.
	/*!
	\param ptr A pointer to the memory to deallocate.
	\param n The amount of objects to deallocate. Size is given in object-count, NOT bytes!
	*/
	void deallocate(T* ptr, std::size_t n) {
		Blk blk{ static_cast<void*>(ptr), n * sizeof(T) };
		alloc_.deallocateBlock(blk);
	}

	//! For STL compatibility
	template <typename U>
	struct rebind {
		using other = AlexandrescuAllocatorAdapter<Alloc, U>;
	};
	//! For STL compatibility
	template <typename UAlloc, typename UT> requires AlexandrescuAllocator<UAlloc> constexpr AlexandrescuAllocatorAdapter(const AlexandrescuAllocatorAdapter <UAlloc, UT>&) noexcept {}
	friend bool operator==(const AlexandrescuAllocatorAdapter& lhs, const AlexandrescuAllocatorAdapter& rhs) {
		return true;
	}
	//! For STL compatibility
	friend bool operator!=(const AlexandrescuAllocatorAdapter& lhs, const AlexandrescuAllocatorAdapter& rhs) {
		return false;
	}
};

//! A wrapper that wraps Andrei Alexandrescu-style allocators to STL-style allocators.
/*!
  \tparam Alloc The allocator.
  \tparam T The type that the allocator allocates.
*/
template <typename Alloc, typename T> requires AlexandrescuAllocator<Alloc> struct StaticAlexandrescuAllocatorAdapter {
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef void* void_pointer;
	typedef const void* const_void_pointer;
	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef StaticAlexandrescuAllocatorAdapter allocator_type;
	typedef std::false_type propagate_on_container_copy_assignment;
	typedef std::false_type propagate_on_container_move_assignment;
	typedef std::false_type propagate_on_container_swap;
	typedef std::true_type is_always_equal;
	//! The underlying AlexandrescuAllocator
	static Alloc alloc_;

	//! Constructor
	StaticAlexandrescuAllocatorAdapter() = default;

	//! Allocate function. Returns a pointer to the allocated data.
	/*!
	\param n The amount of objects to allocate memory for. Size is given in object-count, NOT bytes!
	\return A pointer to the memory allocated.
	*/
	T* allocate(std::size_t n) {
		std::size_t total_size = n * sizeof(T);  // Calculate total size needed
		Blk blk = alloc_.allocateBlock(total_size);
		if (!blk.ptr) throw std::bad_alloc();
		return static_cast<T*>(blk.ptr);  // Return the pointer cast to T*
	}

	//! Deallocates memory.
	/*!
	\param ptr A pointer to the memory to deallocate.
	\param n The amount of objects to deallocate. Size is given in object-count, NOT bytes!
	*/
	void deallocate(T* ptr, std::size_t n) {
		Blk blk{ static_cast<void*>(ptr), n * sizeof(T) };
		alloc_.deallocateBlock(blk);
	}
	//! For STL compatibility
	template <typename U>
	struct rebind {
		using other = StaticAlexandrescuAllocatorAdapter<Alloc, U>;
	};
	//! For STL compatibility
	template <typename UAlloc, typename UT> requires AlexandrescuAllocator<UAlloc> constexpr StaticAlexandrescuAllocatorAdapter(const StaticAlexandrescuAllocatorAdapter <UAlloc, UT>&) noexcept {}
	friend bool operator==(const StaticAlexandrescuAllocatorAdapter& lhs, const StaticAlexandrescuAllocatorAdapter& rhs) {
		return true;
	}
	//! For STL compatibility
	friend bool operator!=(const StaticAlexandrescuAllocatorAdapter& lhs, const StaticAlexandrescuAllocatorAdapter& rhs) {
		return false;
	}
};

//! A wrapper that tries allocating with Primary before using Fallback as a backup.
/*!
  \tparam Primary The primary allocator that we preferably allocate and deallocate with.
  \tparam Fallback The backup allocator that we use in case Primary fails.
*/
template <typename Primary, typename Fallback> requires AlexandrescuAllocator<Primary> && AlexandrescuAllocator<Fallback>
struct FallbackAllocator : private Primary, private Fallback {
	//! Allocates memory.
	/*!
	\param n The amount of memory - in bytes - to allocate.
	\return The allocated memory block. Contains a null-pointer and zero size if the allocation was unsuccessful.
	*/
	Blk allocateBlock(std::size_t n) {
		Blk r = Primary::allocateBlock(n);
		if(!r.ptr) r = Fallback::allocateBlock(n);
		return r;
	}
	//! Deallocates memory.
	/*!
	\param blk A reference to the memory block being allocated. Ensure that the block is owned by the given allocator!
	*/
	void deallocateBlock(const Blk& blk) {
		if(Primary::ownsBlock(blk)) Primary::deallocateBlock(blk);
		else Fallback::deallocateBlock(blk);
	}
	//! Checks if the allocator owns the memory. Be careful, some allocators (e.g. the Mallocator) might consistently return true!
	/*!
	\param blk A reference to the memory block being checked.
	\return True if the allocator owns the block of memory, false otherwise. Be careful, some allocators (e.g. the Mallocator) might consistently return true!
	*/
	bool ownsBlock(const Blk& blk) {
		return Primary::ownsBlock(blk) || Fallback::ownsBlock(blk);
	}
};

//! A wrapper that tries allocating with Primary before using Fallback as a backup.
/*!
	\tparam threshold The byte-size delineation between the two allocator types. Below this number, we use the SmallAllocator. Above this number we use the LargeAllocator.
	\tparam SmallAllocator When the memory we want to allocate is below the given threshold, we use this to allocate and deallocate memory.
	\tparam LargeAllocator When the memory we want to allocate is above the given threshold, we use this to allocate and deallocate memory.
*/
template <size_t threshold, typename SmallAllocator, typename LargeAllocator> requires AlexandrescuAllocator<SmallAllocator> && AlexandrescuAllocator<LargeAllocator>
struct SegregatorAllocator : private SmallAllocator, private LargeAllocator {
	//! Allocates memory.
	/*!
	\param n The amount of memory - in bytes - to allocate.
	\return The allocated memory block. Contains a null-pointer and zero size if the allocation was unsuccessful.
	*/
	Blk allocateBlock(std::size_t n) {
		if(n <= threshold) return SmallAllocator::allocateBlock(n);
		else return LargeAllocator::allocateBlock(n);
	}
	//! Deallocates memory.
	/*!
	\param blk A reference to the memory block being allocated. Ensure that the block is owned by the given allocator!
	*/
	void deallocateBlock(const Blk& blk) {
		if(blk.size <= threshold) return SmallAllocator::deallocateBlock(blk);
		else return LargeAllocator::deallocateBlock(blk);
	}
	//! Checks if the allocator owns the memory. Be careful, some allocators (e.g. the Mallocator) might consistently return true!
	/*!
	\param blk A reference to the memory block being checked.
	\return True if the allocator owns the block of memory, false otherwise. Be careful, some allocators (e.g. the Mallocator) might consistently return true!
	*/
	bool ownsBlock(const Blk& blk) {
		return SmallAllocator::ownsBlock(blk) || LargeAllocator::ownsBlock(blk);
	}
};

}
}
//! Defines smart pointer alieses for a given struct with a given allocator type.
/*!
	\tparam Klass The struct itself.
	\tparam Alloc The allocator for the struct.
*/
#define DEFINE_STRUCT_PTRS_WITH_ALLOC(Klass,Alloc) struct Klass; \
	typedef Elv::Util::SmartPointerWrappersForAlloc<Klass,Alloc> Klass##_Alloc; \
	typedef Klass##_Alloc::unique_ptr u##Klass; \
	typedef Klass##_Alloc::shared_ptr s##Klass; \
	typedef Klass##_Alloc::weak_ptr w##Klass; \

//! Defines smart pointer alieses for a given class with a given allocator type.
/*!
	\tparam Klass The class itself.
	\tparam Alloc The allocator for the struct.
*/
#define DEFINE_CLASS_PTRS_WITH_ALLOC(Klass,Alloc) class Klass; \
	typedef Elv::Util::SmartPointerWrappersForAlloc<Klass,Alloc> Klass##_Alloc; \
	typedef Klass##_Alloc::unique_ptr u##Klass; \
	typedef Klass##_Alloc::shared_ptr s##Klass; \
	typedef Klass##_Alloc::weak_ptr w##Klass; \

#endif // ELVALLOCATORBASIC_HPP
