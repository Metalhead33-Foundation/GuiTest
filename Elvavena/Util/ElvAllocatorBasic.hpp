#ifndef ELVALLOCATORBASIC_HPP
#define ELVALLOCATORBASIC_HPP
#include <cstddef>
#include <concepts>
#include <new>
#include <memory>

/**
 * @defgroup MemoryManagement Memory Management Utilities
 * @{
 */
namespace Elv {
namespace Util {

/**
 * @concept Allocator
 * @brief Concept for an allocator type, ensuring it meets the necessary interface for memory management.
 *
 * @tparam Alloc Allocator type to check.
 * @tparam T Type of objects to be allocated.
 */
template <typename Alloc, typename T> concept Allocator = requires(Alloc alloc, T* ptr, std::size_t n, T t) {
	/// @brief Ensure Alloc has a value_type member.
	typename Alloc::value_type;
	/// @brief Require Alloc's value_type to be the same as T.
	requires std::same_as<typename Alloc::value_type, T>;
	/// @brief Verify alloc can allocate n elements of T, returning a T*.
	{ alloc.allocate(n) } -> std::same_as<T*>;
	/// @brief Ensure alloc can deallocate ptr with size n.
	alloc.deallocate(ptr, n);
};

/**
 * @class SmartPointerWrappersForAlloc
 * @brief Provides smart pointer wrappers (unique_ptr, shared_ptr, weak_ptr) for a given type T and allocator Alloc.
 *
 * @tparam T Type of the objects to be managed.
 * @tparam Alloc Allocator type (defaults to std::allocator<T>).
 */
template <typename T, typename Alloc = std::allocator<T>> requires Allocator<Alloc, T> struct SmartPointerWrappersForAlloc {
public:
	/**
	 * @struct Deleter
	 * @brief Custom deleter for smart pointers, handling object destruction and memory deallocation.
	 */
	struct Deleter {
		Alloc* allocator; ///< Allocator instance (or nullptr for default-constructed temporary allocator).
		/**
		 * @brief Calls destruction on the object and then deallocates the memory using the provided allocator.
		 * @param ptr Pointer to the object to delete.
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
	/**
	 * @brief Creates a new object of type T using the default allocator.
	 * @tparam Args Parameter pack for T's constructor.
	 * @param args Forwarded arguments for T's constructor.
	 * @return Pointer to the newly created T object.
	 */
	template <typename... Args> static T* create(Args&&... args) {
		Alloc allocator;
		T* toReturn = allocator.allocate(1);
		std::construct_at(toReturn, std::forward(args)...);
		return toReturn;
	}

	/**
	 * @brief Creates a new object of type T using a provided allocator.
	 * @tparam Args Parameter pack for T's constructor.
	 * @param allocator Allocator instance to use.
	 * @param args Forwarded arguments for T's constructor.
	 * @return Pointer to the newly created T object.
	 */
	template <typename... Args> static T* createWithAllocator(Alloc& allocator, Args&&... args) {
		T* toReturn = allocator.allocate(1);
		std::construct_at(toReturn, std::forward(args)...);
		return toReturn;
	}

public:
	/// @brief Unique pointer type with custom Deleter.
	typedef std::unique_ptr<T, Deleter> unique_ptr;
	/// @brief Shared pointer type.
	typedef std::shared_ptr<T> shared_ptr;
	/// @brief Weak pointer type.
	typedef std::weak_ptr<T> weak_ptr;

	/**
	 * @brief Creates a unique_ptr to a new T object, using the provided allocator.
	 * @tparam Args Parameter pack for T's constructor.
	 * @param allocator Allocator instance to use.
	 * @param args Forwarded arguments for T's constructor.
	 * @return unique_ptr to the newly created T object.
	 */
	template <typename... Args> static unique_ptr make_unique(Alloc& allocator, Args&&... args) {
		return unique_ptr(createWithAllocator(allocator, std::forward(args)...), Deleter{ &allocator });
	}

	/**
	 * @brief Creates a unique_ptr to a new T object, using a temporary default allocator.
	 * @tparam Args Parameter pack for T's constructor.
	 * @param args Forwarded arguments for T's constructor.
	 * @return unique_ptr to the newly created T object.
	 */
	template <typename... Args> static unique_ptr make_unique(Args&&... args) {
		return unique_ptr(create(std::forward(args)...), Deleter { nullptr });
	}

	/**
	 * @brief Creates a shared_ptr to a new T object, using the provided allocator.
	 * @tparam Args Parameter pack for T's constructor.
	 * @param allocator Allocator instance to use.
	 * @param args Forwarded arguments for T's constructor.
	 * @return shared_ptr to the newly created T object.
	 */
	template <typename... Args> static shared_ptr make_shared(Alloc& allocator, Args&&... args) {
		return shared_ptr(createWithAllocator(allocator, std::forward(args)...), Deleter{ &allocator });
	}

	/**
	 * @brief Creates a shared_ptr to a new T object, using a temporary default allocator.
	 * @tparam Args Parameter pack for T's constructor.
	 * @param args Forwarded arguments for T's constructor.
	 * @return shared_ptr to the newly created T object.
	 */
	template <typename... Args> static shared_ptr make_shared(Args&&... args) {
		return shared_ptr(create(std::forward(args)...), Deleter { nullptr });
	}
};

/**
 * @overload make_unique
 * @brief Free function variant of make_unique, mirroring the member function of the same name.
 */
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::unique_ptr make_unique(Alloc & allocator, Args&&... args) {
	return SmartPointerWrappersForAlloc<T, Alloc>::make_unique(allocator, std::forward(args)...);
}

/**
 * @overload make_unique
 * @brief Free function variant of make_unique, using a temporary default allocator.
 */
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::unique_ptr make_unique(Args&&... args) {
	return SmartPointerWrappersForAlloc<T, Alloc>::make_unique(std::forward(args)...);
}

/**
 * @overload make_shared
 * @brief Free function variant of make_shared, mirroring the member function of the same name.
 */
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::shared_ptr make_shared(Alloc & allocator, Args&&... args) {
	return SmartPointerWrappersForAlloc<T, Alloc>::make_shared(allocator, std::forward(args)...);
}

/**
 * @overload make_shared
 * @brief Free function variant of make_shared, using a temporary default allocator.
 */
template <typename T, typename Alloc = std::allocator<T>, typename... Args> requires Allocator<Alloc, T>
SmartPointerWrappersForAlloc<T, Alloc>::shared_ptr make_shared(Args&&... args) {
	return SmartPointerWrappersForAlloc<T, Alloc>::make_shared(std::forward(args)...);
}

/**
 * @struct Blk
 * @brief Structure representing a memory block.
 */
struct Blk {
	void* ptr; ///< Pointer to the memory block.
	size_t size; ///< Size of the memory block.
};

/**
 * @concept AlexandrescuAllocator
 * @brief Concept for an allocator type following the Alexandrescu style, providing block-level memory management.
 *
 * @tparam Alloc Allocator type to check.
 */
template <typename Alloc> concept AlexandrescuAllocator = requires(Alloc alloc, std::size_t n, Blk blk, const Blk& blkcref) {
	/// @brief Ensure alloc can allocate a block of size n, returning a Blk.
	{ alloc.allocateBlock(n) } -> std::same_as<Blk>;
	/// @brief Verify alloc can deallocate a block.
	{ alloc.deallocateBlock(blkcref) };
	/// @brief Require alloc to check ownership of a block, returning a bool.
	{ alloc.ownsBlock(blkcref) } -> std::same_as<bool>;
};

/**
 * @class AlexandrescuAllocatorAdapter
 * @brief Adapts an Alexandrescu-style allocator to the Standard Library's allocator interface.
 *
 * @tparam Alloc Alexandrescu-style allocator type.
 * @tparam T Type of objects to be allocated.
 */
template <typename Alloc, typename T> requires AlexandrescuAllocator<Alloc> struct AlexandrescuAllocatorAdapter {
	/// @brief Reference type.
	typedef T& reference;
	/// @brief Const reference type.
	typedef const T& const_reference;
	/// @brief Pointer type.
	typedef T* pointer;
	/// @brief Const pointer type.
	typedef const T* const_pointer;
	/// @brief Void pointer type.
	typedef void* void_pointer;
	/// @brief Const void pointer type.
	typedef const void* const_void_pointer;
	/// @brief Value type.
	typedef T value_type;
	/// @brief Size type.
	typedef std::size_t size_type;
/// @brief Difference type.
typedef std::ptrdiff_t difference_type;
/// @brief Allocator type itself.
typedef AlexandrescuAllocatorAdapter allocator_type;
/// @brief Propagation trait for container copy assignment.
typedef std::false_type propagate_on_container_copy_assignment;
/// @brief Propagation trait for container move assignment.
typedef std::false_type propagate_on_container_move_assignment;
/// @brief Propagation trait for container swap.
typedef std::false_type propagate_on_container_swap;
/// @brief Equality trait (always equal for this adapter).
typedef std::true_type is_always_equal;

/// @brief Underlying Alexandrescu-style allocator instance.
Alloc alloc_;

/**
 * @brief Default constructor.
 */
AlexandrescuAllocatorAdapter() = default;

/**
 * @brief Allocates memory for n objects of type T.
 * @param n Number of objects to allocate memory for.
 * @return Pointer to the beginning of the allocated memory.
 * @throws std::bad_alloc if allocation fails.
 */
T* allocate(std::size_t n) {
	std::size_t total_size = n * sizeof(T);
	Blk blk = alloc_.allocateBlock(total_size);
	if (!blk.ptr) throw std::bad_alloc();
	return static_cast<T*>(blk.ptr);
}

/**
 * @brief Deallocates memory previously allocated for n objects of type T.
 * @param ptr Pointer to the memory to deallocate.
 * @param n Number of objects the memory was allocated for.
 */
void deallocate(T* ptr, std::size_t n) {
	Blk blk{ static_cast<void*>(ptr), n * sizeof(T) };
	alloc_.deallocateBlock(blk);
}

/**
 * @struct rebind
 * @brief Helper for rebinding the allocator to a different type U.
 *
 * @tparam U New type to rebind the allocator to.
 */
template <typename U>
struct rebind {
	/// @brief The rebound allocator type.
	using other = AlexandrescuAllocatorAdapter<Alloc, U>;
};

/**
 * @brief Constructs an allocator from another AlexandrescuAllocatorAdapter instance.
 * @tparam UAlloc Allocator type of the other instance.
 * @tparam UT Type associated with the other instance.
 * @param other Other AlexandrescuAllocatorAdapter instance.
 */
template <typename UAlloc, typename UT> requires AlexandrescuAllocator<UAlloc> constexpr AlexandrescuAllocatorAdapter(const AlexandrescuAllocatorAdapter <UAlloc, UT>&) noexcept {}

/**
 * @brief Equality operator (always returns true for this adapter).
 * @param lhs Left-hand side allocator.
 * @param rhs Right-hand side allocator.
 * @return True.
 */
friend bool operator==(const AlexandrescuAllocatorAdapter& lhs, const AlexandrescuAllocatorAdapter& rhs) {
	return true;
}

/**
 * @brief Inequality operator (always returns false for this adapter).
 * @param lhs Left-hand side allocator.
 * @param rhs Right-hand side allocator.
 * @return False.
 */
friend bool operator!=(const AlexandrescuAllocatorAdapter& lhs, const AlexandrescuAllocatorAdapter& rhs) {
	return false;
}
};

/**
 * @class StaticAlexandrescuAllocatorAdapter
 * @brief Static adaptation of AlexandrescuAllocatorAdapter, sharing a single allocator instance across all instances.
 *
 * @tparam Alloc Alexandrescu-style allocator type.
 * @tparam T Type of objects to be allocated.
 */
template <typename Alloc, typename T> requires AlexandrescuAllocator<Alloc> struct StaticAlexandrescuAllocatorAdapter {
	//... (Documentation for this class is similar to AlexandrescuAllocatorAdapter, with the key difference being the static allocator instance)

	/// @brief Shared, static allocator instance.
	static Alloc alloc_;
};

/**
 * @class FallbackAllocator
 * @brief Allocator that falls back to a secondary allocator if the primary one fails to allocate or deallocate.
 *
 * @tparam Primary Primary allocator type.
 * @tparam Fallback Secondary allocator type.
 */
template <typename Primary, typename Fallback> requires AlexandrescuAllocator<Primary> && AlexandrescuAllocator<Fallback>
struct FallbackAllocator : private Primary, private Fallback {
	/**
	 * @brief Allocates a block of memory, falling back to Fallback if Primary fails.
	 * @param n Size of the block to allocate.
	 * @return Allocated block.
	 */
	Blk allocateBlock(std::size_t n) {
		Blk r = Primary::allocateBlock(n);
		if(!r.ptr) r = Fallback::allocateBlock(n);
		return r;
	}

	/**
	 * @brief Deallocates a block, trying Primary first, then Fallback.
	 * @param blk Block to deallocate.
	 */
	void deallocateBlock(const Blk& blk) {
		if(Primary::ownsBlock(blk)) Primary::deallocateBlock(blk);
		else Fallback::deallocateBlock(blk);
	}

	/**
	 * @brief Checks ownership of a block, considering both Primary and Fallback.
	 * @param blk Block to check.
	 * @return True if either Primary or Fallback owns the block.
	 */
	bool ownsBlock(const Blk& blk) {
		return Primary::ownsBlock(blk) || Fallback::ownsBlock(blk);
	}
};

/**
 * @class SegregatorAllocator
 * @brief Allocator that segregates allocations based on size, using SmallAllocator for smaller allocations and LargeAllocator for larger ones.
 *
 * @tparam threshold Size threshold for allocation segregation.
 * @tparam SmallAllocator Allocator for smaller allocations.
 * @tparam LargeAllocator Allocator for larger allocations.
 */
template <size_t threshold, typename SmallAllocator, typename LargeAllocator> requires AlexandrescuAllocator<SmallAllocator> && AlexandrescuAllocator<LargeAllocator>
struct SegregatorAllocator : private SmallAllocator, private LargeAllocator {
	/**
	 * @brief Allocates a block of memory, choosing the allocator based on the size.
	 * @param n Size of the block to allocate.
	 * @return Allocated block.
	 */
	Blk allocateBlock(std::size_t n) {
		if(n <= threshold) return SmallAllocator::allocateBlock(n);
		else return LargeAllocator::allocateBlock(n);
	}

	/**
	 * @brief Deallocates a block, selecting the allocator based on the block's size.
	 * @param blk Block to deallocate.
	 */
	void deallocateBlock(const Blk& blk) {
		if(blk.size <= threshold) return SmallAllocator::deallocateBlock(blk);
		else return LargeAllocator::deallocateBlock(blk);
	}

	/**
	 * @brief Checks ownership of a block, considering both allocators.
	 * @param blk Block to check.
	 * @return True if either allocator owns the block.
	 */
	bool ownsBlock(const Blk& blk) {
		return SmallAllocator::ownsBlock(blk) || LargeAllocator::ownsBlock(blk);
	}
};

}
}

/**
 * @def DEFINE_STRUCT_PTRS_WITH_ALLOC
 * @brief Macro to define smart pointer types (unique, shared, weak) for a struct with a custom allocator.
 *
 * @param Klass Struct name.
 * @param Alloc Allocator type.
 */
#define DEFINE_STRUCT_PTRS_WITH_ALLOC(Klass,Alloc) struct Klass; \
	typedef Elv::Util::SmartPointerWrappersForAlloc<Klass,Alloc> Klass##_Alloc; \
	typedef Klass##_Alloc::unique_ptr u##Klass; \
	typedef Klass##_Alloc::shared_ptr s##Klass; \
	typedef Klass##_Alloc::weak_ptr w##Klass;

/**
 * @def DEFINE_CLASS_PTRS_WITH_ALLOC
 * @brief Macro to define smart pointer types (unique, shared, weak) for a class with a custom allocator.
 *
 * @param Klass Class name.
 * @param Alloc Allocator type.
 */
#define DEFINE_CLASS_PTRS_WITH_ALLOC(Klass,Alloc) class Klass; \
	typedef Elv::Util::SmartPointerWrappersForAlloc<Klass,Alloc> Klass##_Alloc; \
	typedef Klass##_Alloc::unique_ptr u##Klass; \
	typedef Klass##_Alloc::shared_ptr s##Klass; \
	typedef Klass##_Alloc::weak_ptr w##Klass;

/// @} // End of MemoryManagement group
#endif // ELVALLOCATORBASIC_HPP
