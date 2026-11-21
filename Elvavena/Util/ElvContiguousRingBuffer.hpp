#ifndef ELVCONTIGUOUSRINGBUFFER_HPP
#define ELVCONTIGUOUSRINGBUFFER_HPP
/**
 * @file ElvContiguousRingBuffer.hpp
 * @brief Fixed-capacity ring of contiguous elements with read/write spans.
 *
 * `Elv::Util::RingBuffer<T>` stores one optional element per logical slot and is
 * the right choice for histories and rolling windows. It cannot hand a decoder
 * or a socket a contiguous block of free or filled storage, because its slots
 * are individually constructed.
 *
 * This container is the streaming counterpart extracted from Haruka's
 * `RingBufferedStreamer`: one preallocated `std::vector<T>`, a head index and a
 * live count. `writeSpan()` and `readSpan()` return the longest contiguous
 * region at the current tail/head, allowing callers to fill or drain a wrapped
 * buffer with at most two `memcpy`-style copies.
 *
 * The two classes are intentionally not unified: the slot buffer supports
 * non-default-constructible/move-only element histories, while this one requires
 * a default-constructible element type and is optimized for bulk contiguous
 * access (audio samples, bytes, packet payloads).
 */
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace Elv {
namespace Util {

/**
 * @class ContiguousRingBuffer
 * @brief Fixed-capacity, contiguous-region streaming ring buffer.
 *
 * Logical index 0 is the oldest element. `writeSpan()` returns the free region
 * following the newest element; `readSpan()` returns the filled region starting
 * at the oldest element. A wrapped buffer may need a second call after
 * `commitWrite`/`consume`.
 *
 * @tparam T Element type; must be default-constructible.
 * @tparam Allocator Allocator for the backing vector.
 *
 * @note Not thread-safe. The capacity is fixed until `resize()`, which clears
 *       the contents.
 */
template <typename T, typename Allocator = std::allocator<T>>
class ContiguousRingBuffer {
public:
	/// @brief Element type.
	using value_type = T;
	/// @brief Allocator type.
	using allocator_type = Allocator;
	/// @brief Mutable element span.
	using Span = std::span<T>;
	/// @brief Const element span.
	using ConstSpan = std::span<const T>;

	/**
	 * @brief Creates a buffer with a fixed element capacity.
	 * @param capacity Number of elements.
	 * @param alloc Allocator for the backing storage.
	 */
	explicit ContiguousRingBuffer(std::size_t capacity, const Allocator& alloc = Allocator())
		: storage(capacity, alloc), head(0), count(0) {}

	/** @brief Returns the element capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return storage.size(); }

	/** @brief Returns the number of live elements. */
	[[nodiscard]] std::size_t size() const noexcept { return count; }

	/** @brief Returns the number of free elements. */
	[[nodiscard]] std::size_t free() const noexcept { return storage.size() - count; }

	/** @brief Returns whether no elements are live. */
	[[nodiscard]] bool empty() const noexcept { return count == 0; }

	/** @brief Returns whether every element is live. */
	[[nodiscard]] bool full() const noexcept { return count == storage.size(); }

	/**
	 * @brief Returns the longest contiguous free region after the tail.
	 * @param offset Element offset into the free region; 0 is the tail.
	 * @return Writable span, empty when full or @p offset is out of range.
	 */
	[[nodiscard]] Span writeSpan(std::size_t offset = 0) noexcept {
		if (storage.empty() || offset >= free()) return {};
		const std::size_t start = (head + count + offset) % storage.size();
		const std::size_t length = std::min(free() - offset, storage.size() - start);
		return Span(storage.data() + start, length);
	}

	/**
	 * @brief Returns the longest contiguous live region starting at the head.
	 * @param offset Element offset into the live region; 0 is the oldest element.
	 * @return Readable span, empty when empty or @p offset is out of range.
	 */
	[[nodiscard]] ConstSpan readSpan(std::size_t offset = 0) const noexcept {
		if (count == 0 || offset >= count) return {};
		const std::size_t start = (head + offset) % storage.size();
		const std::size_t length = std::min(count - offset, storage.size() - start);
		return ConstSpan(storage.data() + start, length);
	}

	/**
	 * @brief Marks elements at the tail as live.
	 * @param elements Number of elements just written via @ref writeSpan.
	 * @pre @p elements must not exceed @ref free().
	 */
	void commitWrite(std::size_t elements) noexcept {
		assert(elements <= free());
		count += elements;
	}

	/**
	 * @brief Marks elements at the head as consumed.
	 * @param elements Number of elements to remove from the oldest end.
	 * @pre @p elements must not exceed @ref size().
	 */
	void consume(std::size_t elements) noexcept {
		assert(elements <= count);
		if (storage.empty()) return;
		head = (head + elements) % storage.size();
		count -= elements;
	}

	/**
	 * @brief Copies as many elements as fit and commits them.
	 * @param source Source elements.
	 * @param elements Number of elements to copy.
	 * @return Number of elements actually copied.
	 */
	std::size_t write(const T* source, std::size_t elements) {
		if (source == nullptr || elements == 0) return 0;
		std::size_t copied = 0;
		while (copied < elements) {
			Span target = writeSpan();
			if (target.empty()) break;
			const std::size_t chunk = std::min(target.size(), elements - copied);
			std::copy_n(source + copied, chunk, target.begin());
			commitWrite(chunk);
			copied += chunk;
		}
		return copied;
	}

	/**
	 * @brief Copies as many live elements as available and consumes them.
	 * @param destination Destination elements.
	 * @param elements Maximum number of elements to read.
	 * @return Number of elements actually read.
	 */
	std::size_t read(T* destination, std::size_t elements) {
		if (destination == nullptr || elements == 0) return 0;
		std::size_t copied = 0;
		while (copied < elements) {
			ConstSpan source = readSpan();
			if (source.empty()) break;
			const std::size_t chunk = std::min(source.size(), elements - copied);
			std::copy_n(source.begin(), chunk, destination + copied);
			consume(chunk);
			copied += chunk;
		}
		return copied;
	}

	/**
	 * @brief Appends one element if there is room.
	 * @param value Element to copy.
	 * @return True on success, false when full or capacity is zero.
	 */
	bool push(const T& value) {
		if (free() == 0) return false;
		storage[(head + count) % storage.size()] = value;
		++count;
		return true;
	}

	/**
	 * @brief Appends one element if there is room.
	 * @param value Element to move.
	 * @return True on success, false when full or capacity is zero.
	 */
	bool push(T&& value) {
		if (free() == 0) return false;
		storage[(head + count) % storage.size()] = std::move(value);
		++count;
		return true;
	}

	/**
	 * @brief Removes the oldest element.
	 * @param out Receives the element.
	 * @return True on success, false when empty.
	 */
	bool pop(T& out) {
		if (count == 0) return false;
		out = std::move(storage[head]);
		head = (head + 1) % storage.size();
		--count;
		return true;
	}

	/**
	 * @brief Accesses an element by logical index.
	 * @param index 0 is the oldest element.
	 * @return Reference to the element.
	 * @pre @p index must be less than @ref size().
	 */
	T& operator[](std::size_t index) {
		assert(index < count);
		return storage[(head + index) % storage.size()];
	}

	/**
	 * @brief Accesses an element by logical index.
	 * @param index 0 is the oldest element.
	 * @return Const reference to the element.
	 * @pre @p index must be less than @ref size().
	 */
	const T& operator[](std::size_t index) const {
		assert(index < count);
		return storage[(head + index) % storage.size()];
	}

	/** @brief Removes all live elements without releasing storage. */
	void clear() noexcept {
		head = 0;
		count = 0;
	}

	/**
	 * @brief Changes the capacity and clears the contents.
	 * @param capacity New element capacity.
	 */
	void resize(std::size_t capacity) {
		storage.resize(capacity);
		head = 0;
		count = 0;
	}

	/** @brief Returns the backing allocator. */
	[[nodiscard]] allocator_type get_allocator() const { return storage.get_allocator(); }

private:
	std::vector<T, Allocator> storage; ///< Fixed-capacity backing storage.
	std::size_t head = 0;              ///< Logical index of the oldest element.
	std::size_t count = 0;             ///< Number of live elements.
};

} // namespace Util
} // namespace Elv

#endif // ELVCONTIGUOUSRINGBUFFER_HPP

