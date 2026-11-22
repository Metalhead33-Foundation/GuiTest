#ifndef ELVRINGBUFFER_HPP
#define ELVRINGBUFFER_HPP
/**
 * @file ElvRingBuffer.hpp
 * @brief Fixed-capacity circular buffer for history and rolling windows.
 *
 * Networking keeps bounded histories (clock samples, snapshot ticks, input
 * commands, checksums). This container preallocates its slots at construction
 * and never allocates on push, pop or overwrite, which makes it suitable for
 * per-tick code paths. Elements are stored in `std::optional` slots so move-only
 * and non-default-constructible types are supported.
 */
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace Elv {
namespace Util {

/**
 * @class RingBuffer
 * @brief A fixed-capacity, logical-order circular buffer.
 *
 * Logical index 0 is the oldest element; `size() - 1` is the newest. When the
 * buffer is full, `push`/`emplace` reject the new element and return false;
 * `pushOverwrite`/`emplaceOverwrite` evict the oldest element instead.
 *
 * @tparam T Stored element type.
 * @tparam Allocator Allocator rebound to the internal optional slots.
 *
 * @note Not thread-safe. The buffer never grows or reallocates.
 */
template <typename T, typename Allocator = std::allocator<T>>
class RingBuffer {
public:
	/// @brief Stored value type.
	using value_type = T;
	/// @brief Allocator supplied by the caller.
	using allocator_type = Allocator;
	/// @brief Internal slot type.
	using Slot = std::optional<T>;
	/// @brief Allocator rebound to the slot type.
	using SlotAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Slot>;

	/**
	 * @brief Creates a buffer with a fixed number of empty slots.
	 * @param capacity Maximum number of elements; may be zero.
	 * @param alloc Allocator for the slot storage.
	 */
	explicit RingBuffer(size_t capacity, const Allocator& alloc = Allocator())
		: slots(capacity, SlotAllocator(alloc)), head(0), count(0) {}

	/** @brief Returns the maximum number of elements. */
	[[nodiscard]] size_t capacity() const noexcept { return slots.size(); }

	/** @brief Returns the current number of elements. */
	[[nodiscard]] size_t size() const noexcept { return count; }

	/** @brief Returns whether the buffer is empty. */
	[[nodiscard]] bool empty() const noexcept { return count == 0; }

	/** @brief Returns whether the buffer is full. */
	[[nodiscard]] bool full() const noexcept { return count == slots.size(); }

	/**
	 * @brief Appends a copy if there is room.
	 * @param value Element to copy.
	 * @return True on success, false when full or capacity is zero.
	 */
	bool push(const T& value) { return emplace(value); }

	/**
	 * @brief Appends a moved value if there is room.
	 * @param value Element to move.
	 * @return True on success, false when full or capacity is zero.
	 */
	bool push(T&& value) { return emplace(std::move(value)); }

	/**
	 * @brief Appends an element if there is room, evicting the oldest when full.
	 * @param value Element to copy.
	 * @return True on success, false only when capacity is zero.
	 */
	bool pushOverwrite(const T& value) { return emplaceOverwrite(value); }

	/**
	 * @brief Appends a moved element if there is room, evicting the oldest when full.
	 * @param value Element to move.
	 * @return True on success, false only when capacity is zero.
	 */
	bool pushOverwrite(T&& value) { return emplaceOverwrite(std::move(value)); }

	/**
	 * @brief Constructs an element in place if there is room.
	 * @tparam Args Constructor argument types.
	 * @param args Arguments forwarded to T's constructor.
	 * @return True on success, false when full or capacity is zero.
	 */
	template <typename... Args>
	bool emplace(Args&&... args) {
		return emplaceImpl(false, std::forward<Args>(args)...);
	}

	/**
	 * @brief Constructs an element in place, evicting the oldest when full.
	 * @tparam Args Constructor argument types.
	 * @param args Arguments forwarded to T's constructor.
	 * @return True on success, false only when capacity is zero.
	 */
	template <typename... Args>
	bool emplaceOverwrite(Args&&... args) {
		return emplaceImpl(true, std::forward<Args>(args)...);
	}

	/**
	 * @brief Removes and returns the oldest element.
	 * @param out Receives the moved element.
	 * @return True on success, false when empty.
	 */
	bool pop(T& out) {
		if (count == 0) return false;
		out = std::move(*slots[head]);
		slots[head].reset();
		head = (head + 1) % slots.size();
		--count;
		return true;
	}

	/**
	 * @brief Removes and returns the newest element.
	 * @param out Receives the moved element.
	 * @return True on success, false when empty.
	 */
	bool popBack(T& out) {
		if (count == 0) return false;
		const size_t index = (head + count - 1) % slots.size();
		out = std::move(*slots[index]);
		slots[index].reset();
		--count;
		return true;
	}

	/**
	 * @brief Accesses an element by logical index.
	 * @param index 0 is the oldest element.
	 * @return Reference to the element.
	 * @pre @p index must be less than size().
	 */
	T& operator[](size_t index) { return *slots[(head + index) % slots.size()]; }

	/**
	 * @brief Accesses an element by logical index.
	 * @param index 0 is the oldest element.
	 * @return Const reference to the element.
	 * @pre @p index must be less than size().
	 */
	const T& operator[](size_t index) const { return *slots[(head + index) % slots.size()]; }

	/**
	 * @brief Accesses an element by logical index with a bounds assertion.
	 * @param index 0 is the oldest element.
	 * @return Reference to the element.
	 */
	T& at(size_t index) {
		assert(index < count);
		return (*this)[index];
	}

	/**
	 * @brief Accesses an element by logical index with a bounds assertion.
	 * @param index 0 is the oldest element.
	 * @return Const reference to the element.
	 */
	const T& at(size_t index) const {
		assert(index < count);
		return (*this)[index];
	}

	/** @brief Returns a pointer to the oldest element, or nullptr when empty. */
	T* front() { return count == 0 ? nullptr : &*slots[head]; }
	/** @brief Returns a const pointer to the oldest element, or nullptr when empty. */
	const T* front() const { return count == 0 ? nullptr : &*slots[head]; }
	/** @brief Returns a pointer to the newest element, or nullptr when empty. */
	T* back() { return count == 0 ? nullptr : &*slots[(head + count - 1) % slots.size()]; }
	/** @brief Returns a const pointer to the newest element, or nullptr when empty. */
	const T* back() const { return count == 0 ? nullptr : &*slots[(head + count - 1) % slots.size()]; }

	/** @brief Destroys all stored elements and resets the buffer. */
	void clear() noexcept {
		for (size_t index = 0; index < count; ++index) {
			slots[(head + index) % slots.size()].reset();
		}
		head = 0;
		count = 0;
	}

private:
	/**
	 * @brief Shared implementation for append and overwrite.
	 * @param overwrite Whether to evict the oldest element when full.
	 * @param args Constructor arguments for the new element.
	 * @return True when an element was inserted.
	 */
	template <typename... Args>
	bool emplaceImpl(bool overwrite, Args&&... args) {
		if (slots.empty()) return false;
		if (count == slots.size()) {
			if (!overwrite) return false;
			slots[head].reset();
			head = (head + 1) % slots.size();
			--count;
		}
		const size_t index = (head + count) % slots.size();
		slots[index].emplace(std::forward<Args>(args)...);
		++count;
		return true;
	}

	std::vector<Slot, SlotAllocator> slots; ///< Fixed-size storage; engaged entries are live.
	size_t head = 0;                         ///< Logical index of the oldest element.
	size_t count = 0;                        ///< Number of live elements.
};

} // namespace Util
} // namespace Elv

#endif // ELVRINGBUFFER_HPP

