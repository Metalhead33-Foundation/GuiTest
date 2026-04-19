#ifndef ELVINTRUSIVEARRAY_HPP
#define ELVINTRUSIVEARRAY_HPP
#include <atomic>
#include <memory_resource>
#include <algorithm>
#include <cstddef>
#include <new>
#include <utility>
#include <span>
namespace Elv {
namespace Util {

/**
 * @brief Reference-counted intrusive array with metadata in one contiguous slab.
 *
 * The allocation layout is:
 * `[Header][Element[count]]`.
 * Multiple `IntrusiveArray` handles can share ownership of the same slab via
 * an internal atomic reference count.
 *
 * @tparam Element Element type stored in the trailing array.
 * @tparam Meta Metadata type stored inside the allocation header.
 */
template<typename Element, typename Meta>
class IntrusiveArray {
public:
	/** @brief Mutable span type over the element range. */
	typedef std::span<Element> ElementSpan;
	/** @brief Const span type over the element range. */
	typedef std::span<const Element> ElementConstSpan;

	/**
	 * @brief Allocation header stored before the trailing element array.
	 *
	 * This structure is aligned to satisfy both `Meta` and `Element` alignment
	 * requirements so the trailing element storage is correctly aligned.
	 */
	// The header lives at the top of the slab. We align it to the stricter
	// of the two alignment requirements so the trailing Element array is
	// always correctly aligned without explicit padding calculation.
	struct alignas(std::max(alignof(Meta), alignof(Element))) Header {
		std::atomic<uint32_t>      refCount;
		std::pmr::memory_resource* allocator;
		size_t                     elementCount; // stored so release() can compute slab size
		Meta                       meta;

		// Trailing elements are accessed via pointer arithmetic past the header.
		/** @brief Returns a mutable pointer to the first trailing element. */
		Element* data() {
			return reinterpret_cast<Element*>(this + 1);
		}
		/** @brief Returns a const pointer to the first trailing element. */
		const Element* data() const {
			return reinterpret_cast<const Element*>(this + 1);
		}

	private:
		// Construction only via IntrusiveArray::allocate()
		Header(std::pmr::memory_resource* res, size_t count, Meta m)
			: refCount(1), allocator(res), elementCount(count), meta(std::move(m)) {}

		~Header() = default;

		friend class IntrusiveArray;
	};

	// --- Handle interface (this is what users actually hold and pass around) ---

	/** @brief Constructs an invalid/null handle. */
	IntrusiveArray() : hdr_(nullptr) {}

	// Takes ownership of a freshly-allocated header (refCount already == 1).
	/**
	 * @brief Takes ownership of a raw header whose reference count is already 1.
	 * @param raw Header pointer produced by one of the `allocate_*` factories.
	 */
	explicit IntrusiveArray(Header* raw) : hdr_(raw) {}

	/**
	 * @brief Copy constructor that increments the shared reference count.
	 * @param other Source handle.
	 */
	IntrusiveArray(const IntrusiveArray& other) : hdr_(other.hdr_) {
		if (hdr_)
			hdr_->refCount.fetch_add(1, std::memory_order_relaxed);
	}

	/**
	 * @brief Move constructor that transfers ownership of the handle.
	 * @param other Source handle, left invalid after move.
	 */
	IntrusiveArray(IntrusiveArray&& other) noexcept : hdr_(other.hdr_) {
		other.hdr_ = nullptr;
	}

	/** @brief Releases one reference, potentially freeing the slab. */
	~IntrusiveArray() { release(); }

	/**
	 * @brief Assignment via copy-and-swap.
	 * @param other Source handle (copy or move).
	 * @return `*this`.
	 */
	IntrusiveArray& operator=(IntrusiveArray other) noexcept {
		std::swap(hdr_, other.hdr_); // copy-and-swap: other's destructor releases old ref
		return *this;
	}

	// --- Element access ---

	/**
	 * @brief Returns mutable pointer to element storage.
	 * @pre `valid() == true`.
	 */
	Element* data()             { return hdr_->data(); }
	/**
	 * @brief Returns const pointer to element storage.
	 * @pre `valid() == true`.
	 */
	const Element* data() const { return hdr_->data(); }
	/**
	 * @brief Returns number of elements in the array.
	 * @pre `valid() == true`.
	 */
	size_t   size()       const { return hdr_->elementCount; }

	/**
	 * @brief Returns mutable element at index.
	 * @param i Element index.
	 * @pre `valid() == true`.
	 */
	Element& operator[](size_t i)       { return hdr_->data()[i]; }
	/**
	 * @brief Returns const element at index.
	 * @param i Element index.
	 * @pre `valid() == true`.
	 */
	const Element& operator[](size_t i) const { return hdr_->data()[i]; }

	/**
	 * @brief Returns a mutable span over all elements.
	 * @pre `valid() == true`.
	 */
	ElementSpan asSpan() { return ElementSpan(hdr_->data(), hdr_->elementCount); }
	/**
	 * @brief Implicit conversion to mutable span.
	 * @pre `valid() == true`.
	 */
	operator ElementSpan() { return ElementSpan(hdr_->data(), hdr_->elementCount); }
	/**
	 * @brief Returns a const span over all elements.
	 * @pre `valid() == true`.
	 */
	ElementConstSpan asSpan() const { return ElementConstSpan(hdr_->data(), hdr_->elementCount); }
	/**
	 * @brief Implicit conversion to const span.
	 * @pre `valid() == true`.
	 */
	operator ElementConstSpan() const { return ElementConstSpan(hdr_->data(), hdr_->elementCount); }

	// Direct access to the metadata struct, so callers can write
	// handle.meta().frameCount rather than handle.hdr_->meta.frameCount.
	/**
	 * @brief Returns mutable metadata stored in the shared header.
	 * @pre `valid() == true`.
	 */
	Meta&       meta()       { return hdr_->meta; }
	/**
	 * @brief Returns const metadata stored in the shared header.
	 * @pre `valid() == true`.
	 */
	const Meta& meta() const { return hdr_->meta; }

	/** @brief Returns true if this handle references a valid allocation. */
	bool valid() const { return hdr_ != nullptr; }

	// --- Factory ---

	// 'meta' is passed by value intentionally: it's plain data, and moving it
	// into the header is cleaner than forcing callers to think about lifetimes.

	// For trivially-filled buffers like audio samples — elements are raw memory,
	// caller is responsible for writing before reading.
	/**
	 * @brief Allocates a slab without constructing `Element` objects.
	 *
	 * Use this when `Element` will be written as raw storage before any reads.
	 *
	 * @param res Memory resource used for allocation and deallocation.
	 * @param count Number of trailing elements.
	 * @param meta Metadata value copied/moved into the header.
	 * @return New handle with reference count initialized to 1.
	 */
	static IntrusiveArray allocate_uninitialised(
		std::pmr::memory_resource* res, size_t count, Meta meta)
	{
		size_t slabSize = sizeof(Header) + count * sizeof(Element);
		void*  mem      = res->allocate(slabSize, alignof(Header));
		Header* hdr = new(mem) Header(res, count, std::move(meta));
		return IntrusiveArray(hdr);
	}

	// For types that need their constructors run — strings, mutexes, anything
	// with non-trivial default construction. Slightly more expensive but safe.
	/**
	 * @brief Allocates a slab and default-constructs all `Element` objects.
	 *
	 * @param res Memory resource used for allocation and deallocation.
	 * @param count Number of trailing elements.
	 * @param meta Metadata value copied/moved into the header.
	 * @return New handle with reference count initialized to 1.
	 */
	static IntrusiveArray allocate_default_constructed(
		std::pmr::memory_resource* res, size_t count, Meta meta)
	{
		size_t slabSize = sizeof(Header) + count * sizeof(Element);
		void*  mem      = res->allocate(slabSize, alignof(Header));
		Header* hdr = new(mem) Header(res, count, std::move(meta));
		for(size_t i = 0; i < count; ++i) {
			new(hdr->data() + i) Element();
		}
		return IntrusiveArray(hdr);
	}

private:
	Header* hdr_;

	void release() {
		if (!hdr_) return;
		if (hdr_->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			std::pmr::memory_resource* res   = hdr_->allocator;
			size_t slabSize = sizeof(Header) + hdr_->elementCount * sizeof(Element);
			hdr_->~Header();
			res->deallocate(hdr_, slabSize, alignof(Header));
		}
		hdr_ = nullptr;
	}
};

}
}
#endif // ELVINTRUSIVEARRAY_HPP
