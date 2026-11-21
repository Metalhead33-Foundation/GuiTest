#ifndef ELVCHECKSUM_HPP
#define ELVCHECKSUM_HPP
/**
 * @file ElvChecksum.hpp
 * @brief Stable wire checksums used by the networking layer.
 *
 * Standard-library hashes are allowed to vary between implementations and are
 * therefore unsuitable for values exchanged on the wire. This header provides
 * two small, fully specified algorithms:
 *
 * - CRC-32C, which detects accidental corruption and is the recommended state
 *   checksum for deterministic-lockstep verification.
 * - FNV-1a, which is cheap and useful for non-cryptographic identifiers and
 *   hash tables.
 */
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Elv {
namespace Util {

/**
 * @brief Builds the reflected CRC-32C (Castagnoli) lookup table at compile time.
 * @return 256-entry table for polynomial 0x82F63B78.
 */
consteval std::array<std::uint32_t, 256> makeCrc32cTable() {
	std::array<std::uint32_t, 256> table {};
	for (std::uint32_t i = 0; i < table.size(); ++i) {
		std::uint32_t crc = i;
		for (unsigned bit = 0; bit < 8; ++bit) {
			crc = (crc & 1u) ? ((crc >> 1) ^ 0x82F63B78u) : (crc >> 1);
		}
		table[i] = crc;
	}
	return table;
}

/** @brief Compile-time CRC-32C lookup table. */
inline constexpr std::array<std::uint32_t, 256> crc32cTable = makeCrc32cTable();

/**
 * @brief Incrementally folds bytes into a running CRC-32C state.
 * @param crc Running state, initialized to 0xFFFFFFFF by the caller.
 * @param data Bytes to fold in.
 * @return Updated running state (not yet inverted).
 */
inline std::uint32_t crc32cUpdate(std::uint32_t crc, std::span<const std::byte> data) noexcept {
	for (const std::byte value : data) {
		const std::uint8_t byte = std::to_integer<std::uint8_t>(value);
		crc = crc32cTable[(crc ^ byte) & 0xFFu] ^ (crc >> 8);
	}
	return crc;
}

/**
 * @brief Computes the CRC-32C checksum of a byte range.
 * @param data Bytes to checksum.
 * @return Finalized CRC-32C value.
 */
inline std::uint32_t crc32c(std::span<const std::byte> data) noexcept {
	return ~crc32cUpdate(0xFFFFFFFFu, data);
}

/**
 * @class Crc32C
 * @brief Incremental CRC-32C accumulator for streaming or chunked payloads.
 */
class Crc32C {
public:
	/** @brief Creates a new accumulator with the standard initial state. */
	Crc32C() noexcept = default;

	/**
	 * @brief Folds a chunk into the checksum.
	 * @param data Chunk to process.
	 */
	void update(std::span<const std::byte> data) noexcept {
		state = crc32cUpdate(state, data);
	}

	/**
	 * @brief Finalizes the checksum without changing the accumulator.
	 * @return CRC-32C value for everything fed so far.
	 */
	[[nodiscard]] std::uint32_t finalize() const noexcept { return ~state; }

	/** @brief Returns the accumulator to its initial state. */
	void reset() noexcept { state = 0xFFFFFFFFu; }

private:
	std::uint32_t state = 0xFFFFFFFFu; ///< Running, not-yet-inverted CRC state.
};

/**
 * @brief Computes the 64-bit FNV-1a hash of a byte range.
 * @param data Bytes to hash.
 * @return FNV-1a 64-bit value.
 */
inline std::uint64_t fnv1a64(std::span<const std::byte> data) noexcept {
	std::uint64_t hash = 14695981039346656037ull;
	for (const std::byte value : data) {
		hash ^= std::to_integer<std::uint8_t>(value);
		hash *= 1099511628211ull;
	}
	return hash;
}

/**
 * @brief Computes the 32-bit FNV-1a hash of a byte range.
 * @param data Bytes to hash.
 * @return FNV-1a 32-bit value.
 */
inline std::uint32_t fnv1a32(std::span<const std::byte> data) noexcept {
	std::uint32_t hash = 2166136261u;
	for (const std::byte value : data) {
		hash ^= std::to_integer<std::uint8_t>(value);
		hash *= 16777619u;
	}
	return hash;
}

} // namespace Util
} // namespace Elv

#endif // ELVCHECKSUM_HPP
