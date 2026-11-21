#ifndef ELVNETID_HPP
#define ELVNETID_HPP
/**
 * @file ElvNetId.hpp
 * @brief Generation-checked network entity identifiers.
 *
 * A raw reuse-prone integer is a bad handle for replicated objects: after an
 * entity is destroyed its slot is recycled, and a late packet carrying the old
 * id must not resurrect or mutate the new occupant. `NetId` packs a 20-bit slot
 * index and a 12-bit generation. Destroying an entity bumps the generation, so a
 * stale handle no longer matches the live slot.
 *
 * @note 4096 reuses of one slot can wrap the generation. That is ample for a
 * session, but it is still an ABA window; long-running servers should rotate
 * sessions before it matters.
 */
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @class NetId
 * @brief Packed slot index + generation handle.
 */
class NetId {
public:
	/// @brief Number of low bits used by the slot index.
	static constexpr std::uint32_t IndexBits = 20;
	/// @brief Number of high bits used by the generation.
	static constexpr std::uint32_t GenerationBits = 12;
	/// @brief Largest valid slot index.
	static constexpr std::uint32_t MaxIndex = (1u << IndexBits) - 1u;
	/// @brief Mask covering the generation bits.
	static constexpr std::uint32_t GenerationMask = (1u << GenerationBits) - 1u;

	/** @brief Creates an invalid handle. */
	constexpr NetId() noexcept = default;

	/**
	 * @brief Builds a handle from its parts.
	 * @param index Slot index; zero means invalid.
	 * @param generation Generation counter.
	 * @return Packed handle.
	 */
	static constexpr NetId fromParts(std::uint32_t index, std::uint32_t generation) noexcept {
		NetId id;
		if (index == 0 || index > MaxIndex || generation == 0) return id;
		id.raw = ((generation & GenerationMask) << IndexBits) | index;
		return id;
	}

	/** @brief Returns the slot index. */
	[[nodiscard]] constexpr std::uint32_t index() const noexcept {
		return raw & MaxIndex;
	}

	/** @brief Returns the generation counter. */
	[[nodiscard]] constexpr std::uint32_t generation() const noexcept {
		return (raw >> IndexBits) & GenerationMask;
	}

	/** @brief Returns the packed raw value. */
	[[nodiscard]] constexpr std::uint32_t value() const noexcept { return raw; }

	/** @brief Returns whether the handle carries a non-zero slot index. */
	[[nodiscard]] constexpr bool isValid() const noexcept { return index() != 0; }

	/** @brief Equality compares the packed value. */
	[[nodiscard]] constexpr bool operator==(const NetId& other) const noexcept {
		return raw == other.raw;
	}

	/** @brief Inequality compares the packed value. */
	[[nodiscard]] constexpr bool operator!=(const NetId& other) const noexcept {
		return raw != other.raw;
	}

private:
	std::uint32_t raw = 0; ///< Packed index/generation.
};

/**
 * @struct NetIdHash
 * @brief Hash functor for `NetId` map keys.
 */
struct NetIdHash {
	/**
	 * @brief Hashes the packed handle.
	 * @param id Handle to hash.
	 * @return Hash value.
	 */
	[[nodiscard]] std::size_t operator()(NetId id) const noexcept {
		return std::hash<std::uint32_t> {}(id.value());
	}
};

/**
 * @class NetIdAllocator
 * @brief Recycles slot indices while advancing their generations.
 *
 * @note Not thread-safe. A registry that shares an allocator must serialize
 * access.
 */
class NetIdAllocator {
public:
	/** @brief Creates an empty allocator. */
	NetIdAllocator() = default;

	/**
	 * @brief Acquires a fresh or recycled handle.
	 * @return Valid handle, or an invalid handle when the index space is
	 *         exhausted.
	 */
	[[nodiscard]] NetId acquire() {
		std::uint32_t index = 0;
		if (!freeList.empty()) {
			index = freeList.back();
			freeList.pop_back();
		} else {
			if (nextIndex > NetId::MaxIndex) return NetId {};
			index = nextIndex++;
		}
		if (generations.size() <= index) generations.resize(index + 1, 0);
		if (generations[index] == 0) generations[index] = 1;
		return NetId::fromParts(index, generations[index]);
	}

	/**
	 * @brief Releases a handle and advances the slot generation.
	 * @param id Handle to release.
	 * @return True when the handle was live; false for stale or double releases.
	 */
	bool release(NetId id) {
		if (!isAlive(id)) return false;
		const std::uint32_t index = id.index();
		std::uint32_t next = (generations[index] + 1u) & NetId::GenerationMask;
		if (next == 0) next = 1;
		generations[index] = next;
		freeList.push_back(index);
		return true;
	}

	/**
	 * @brief Returns whether a handle matches the live generation of its slot.
	 * @param id Handle to test.
	 */
	[[nodiscard]] bool isAlive(NetId id) const noexcept {
		if (!id.isValid() || id.generation() == 0 || id.index() >= generations.size()) return false;
		return generations[id.index()] != 0 && generations[id.index()] == id.generation();
	}

	/** @brief Returns the number of currently free slots. */
	[[nodiscard]] std::size_t freeCount() const noexcept { return freeList.size(); }

	/** @brief Clears all allocation state. */
	void reset() noexcept {
		generations.clear();
		freeList.clear();
		nextIndex = 1;
	}

private:
	std::vector<std::uint32_t> generations; ///< Current generation per slot; 0 means never used.
	std::vector<std::uint32_t> freeList;    ///< Recyclable slot indices.
	std::uint32_t nextIndex = 1;             ///< Next never-used slot index.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETID_HPP
