#ifndef ELVNETFRAGMENT_HPP
#define ELVNETFRAGMENT_HPP
/**
 * @file ElvNetFragment.hpp
 * @brief Bounded reassembly of one fragmented datagram sequence.
 *
 * A message larger than a single datagram is split into indexed slices. This
 * class holds the slices for one message, detects duplicates and invalid
 * metadata, and produces the concatenated payload once every slice has arrived.
 * Session-level code owns the per-peer/per-channel map and the timeout policy.
 */
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @enum FragmentStatus
 * @brief Result of feeding a slice into a reassembler.
 */
enum class FragmentStatus : std::uint8_t {
	Incomplete, ///< More slices are needed.
	Complete,   ///< Every slice has arrived and the payload can be assembled.
	Invalid,    ///< Metadata or slice content violates the fragment contract.
};

/**
 * @class FragmentReassembler
 * @brief Reassembles a single message from indexed byte slices.
 *
 * @note Not thread-safe. A reassembler is reused by calling @ref begin again.
 */
class FragmentReassembler {
public:
	/// @brief Default maximum number of slices in one message.
	static constexpr std::uint16_t DefaultMaxFragments = 256;

	/// @brief Default maximum reassembled size in bytes.
	static constexpr std::uint32_t DefaultMaxTotalSize = 64 * 1024;

	/**
	 * @brief Starts a new message.
	 * @param count Number of slices.
	 * @param totalSize Expected reassembled size in bytes.
	 * @param maxFragments Upper bound on @p count.
	 * @param maxTotalSize Upper bound on @p totalSize.
	 * @return True when the metadata is acceptable and the reassembler is ready.
	 */
	bool begin(std::uint16_t count, std::uint32_t totalSize,
		std::uint16_t maxFragments = DefaultMaxFragments,
		std::uint32_t maxTotalSize = DefaultMaxTotalSize) {
		reset();
		if (count == 0 || count > maxFragments) return false;
		if (totalSize > maxTotalSize) return false;
		fragmentCount = count;
		expectedSize = totalSize;
		pieces.assign(count, {});
		received.assign(count, false);
		active = true;
		return true;
	}

	/**
	 * @brief Adds one slice.
	 * @param index Zero-based slice index.
	 * @param payload Slice bytes.
	 * @return @ref FragmentStatus::Complete once the last missing slice arrives,
	 *         @ref FragmentStatus::Incomplete otherwise, or
	 *         @ref FragmentStatus::Invalid for malformed input.
	 * @note Adding an already received index is treated as a duplicate and
	 *       returns @ref FragmentStatus::Incomplete.
	 */
	FragmentStatus add(std::uint16_t index, std::span<const std::byte> payload) {
		if (!active || index >= fragmentCount) return FragmentStatus::Invalid;
		if (received[index]) return FragmentStatus::Incomplete;
		if (payload.size() > expectedSize) return FragmentStatus::Invalid;

		std::uint64_t projected = 0;
		for (const auto& piece : pieces) projected += piece.size();
		projected += payload.size();
		if (projected > expectedSize) return FragmentStatus::Invalid;

		pieces[index].assign(payload.begin(), payload.end());
		received[index] = true;
		++receivedCount;
		return receivedCount == fragmentCount
			? FragmentStatus::Complete
			: FragmentStatus::Incomplete;
	}

	/**
	 * @brief Concatenates the received slices.
	 * @param output Receives the reassembled payload.
	 * @return True when complete and the total size matches the metadata.
	 */
	bool assemble(std::vector<std::byte>& output) const {
		if (!complete()) return false;
		output.clear();
		output.reserve(expectedSize);
		for (const auto& piece : pieces) {
			output.insert(output.end(), piece.begin(), piece.end());
		}
		return output.size() == expectedSize;
	}

	/** @brief Returns whether every slice has arrived. */
	[[nodiscard]] bool complete() const {
		return active && fragmentCount != 0 && receivedCount == fragmentCount;
	}

	/** @brief Returns whether a message is in progress. */
	[[nodiscard]] bool isActive() const { return active; }

	/** @brief Returns the declared total size. */
	[[nodiscard]] std::uint32_t totalSize() const { return expectedSize; }

	/** @brief Returns the declared fragment count. */
	[[nodiscard]] std::uint16_t count() const { return fragmentCount; }

	/** @brief Discards all state. */
	void reset() {
		pieces.clear();
		received.clear();
		fragmentCount = 0;
		expectedSize = 0;
		receivedCount = 0;
		active = false;
	}

private:
	std::vector<std::vector<std::byte>> pieces; ///< Per-index slice storage.
	std::vector<bool> received;                 ///< Which indices have arrived.
	std::uint16_t fragmentCount = 0;            ///< Declared slice count.
	std::uint32_t expectedSize = 0;             ///< Declared payload size.
	std::uint16_t receivedCount = 0;            ///< Number of slices received.
	bool active = false;                        ///< Whether a message is in progress.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETFRAGMENT_HPP
